#include "processor/nes_ppu.hpp"

#include "platform/ui_properties.hpp"
#include "processor/address_bus.hpp"
#include "processor/ppu_address_bus.hpp"
#include "processor/utils.hpp"

#include <glog/logging.h>

NesPPU::NesPPU(AddressBus& address_bus, PPUAddressBus& ppu_address_bus, NesDisplay& display, bool& nmi_signal)
: address_bus_(address_bus)
, ppu_address_bus_(ppu_address_bus)
, display_(display)
, nmi_signal_(nmi_signal)
{
    std::cout << "Launching NesPPU...\n";

    registers_[PPUCTRL] = 0x00;
    registers_[PPUMASK] = 0x00;
    registers_[PPUSTATUS] = 0x00;
    registers_[OAMADDR] = 0x00;
    registers_[OAMDATA] = 0x00;
    registers_[PPUSCROLL] = 0x00;
    registers_[PPUADDR] = 0x00;
    registers_[PPUDATA] = 0x00;
    registers_[OAMDMA] = 0x00;
}

NesPPU::~NesPPU()
{
}

void NesPPU::reset()
{
    internal_memory_.fill(0);
    display_.clear_screen(NesDisplay::Color({0x00, 0x00, 0x00, 0xFF}));

    cached_nametable_address_ = nametable_base_address();
    cached_patterntable_address = pattern_table_base_address();
}

void NesPPU::run()
{
}

bool NesPPU::step()
{
    increment_cycle();

    handle_oam_data_register();
    handle_ppu_data_register();
    handle_oam_dma_register();

    if (is_rendering_scanline())
    {
        if (cycle_ < NesDisplay::WIDTH) // TODO proper timing
        {
            render_pixel();
        }
    }

    if (check_rendering_falling_edge())
    {
        render_sprites(Sprite::Layer::Foreground); // TODO integrate with proper timing and background rendering

        display_.render();
    }

    if (check_vblank_raising_edge()) // vsync
    {
        registers_[PPUSTATUS] |= PPUSTATUS_vblank;

        if (registers_[PPUCTRL] & PPUCTRL_Generate_NMI)
        {
            nmi_signal_ = true;
            display_.clear_screen(fetch_color_from_palette(0, 0)); // background color

            cached_nametable_address_ = nametable_base_address();
            cached_patterntable_address = pattern_table_base_address();

            read_sprite_oam(); // TODO proper timing for reading oam
            render_sprites(Sprite::Layer::Background); // TODO integrate with proper timing and background rendering
        }   
    }

    if (check_vblank_falling_edge())
    {
        registers_[PPUSTATUS] &= ~PPUSTATUS_vblank;
        registers_[PPUSTATUS] &= ~PPUSTATUS_sprite0_hit;
    }

    return true;
}

uint8_t NesPPU::read_register(uint16_t a) const
{
    assert(a == 0x4014 || (a >= 0x2000 && a <= 0x2007));

    return registers_[a];
}

uint8_t& NesPPU::write_register(uint16_t a)
{
    assert(a == 0x4014 || (a >= 0x2000 && a <= 0x2007));

    registers_.set_had_write(a);
    return registers_[a];
}

void NesPPU::render_pixel()
{
    // Determine which pixel we are working on
    const uint8_t pixel_x = cycle_;
    const uint8_t pixel_y = scanline_;

    // Get the index of the pattern tile from the nametable
    const uint8_t pattern_tile_index = get_pattern_tile_index_for_pixel(pixel_x, pixel_y);

    // Get the index into the color table from the pattern table tile
    const uint8_t colortable_index = get_colortable_index_for_tile_and_pixel(cached_patterntable_address,
                                                                             pixel_x % 8,
                                                                             pixel_y % NAMETABLE_TILE_SIZE,
                                                                             pattern_tile_index);
    // Determine which palette to use
    const uint8_t palette_index = get_palette_index_for_pixel(pixel_x, pixel_y);

    // Retrieve the RGB color for this pixel
    const NesDisplay::Color pixel_color = fetch_color_from_palette(palette_index, colortable_index);

    if (colortable_index == 0)
    {
        // transparent
        return;
    }

    // Draw the pixel!
    display_.draw_pixel(pixel_x, pixel_y, pixel_color);
}

uint8_t NesPPU::get_pattern_tile_index_for_pixel(uint8_t pixel_x, uint8_t pixel_y)
{
    // Retrieve the nametable data for this pixel. The nametable is broken up into 8x8 tiles
    // which contain an index into the pattern table.
    const uint8_t tile_y = pixel_y / NAMETABLE_TILE_SIZE;
    const uint8_t tile_x = pixel_x / NAMETABLE_TILE_SIZE;
    const uint16_t nt_addr = cached_nametable_address_ + tile_x + tile_y * NAMETABLE_WIDTH;
    
    return ppu_address_bus_.read(nt_addr);
}

uint8_t NesPPU::get_colortable_index_for_tile_and_pixel(uint16_t pattern_table_base_address,
                                                        uint8_t tile_pixel_x, uint8_t tile_pixel_y,
                                                        uint8_t pattern_tile_index) const
{
    // Get the pattern table tile from the value found in the nametable. The patterns have
    // 2 bits per pixel. Each pixel is stored in a different plane. Pull the appropriate
    // bit out of each plane and store it into a 2 bit number which represents the color
    // table index for the pixel in this tile.

    // 0HNNNN NNNNPyyy
    // |||||| |||||+++- T: Fine Y offset, the row number within a tile
    // |||||| ||||+---- P: Bit plane (0: less significant bit; 1: more significant bit)
    // ||++++-++++----- N: Tile number from name table
    // |+-------------- H: Half of pattern table (0: "left"; 1: "right")
    // +--------------- 0: Pattern table is at $0000-$1FFF
    const uint16_t pattern_tile_addr = pattern_table_base_address |
                                       (pattern_tile_index << 4) |
                                       tile_pixel_y;

    const uint16_t patterntable_lo_bit_plane_select = 0x0000;
    const uint16_t patterntable_hi_bit_plane_select = 0x0008;
    const uint8_t patterntable_bit_for_pixel = 7 - tile_pixel_x;

    uint16_t patterntable_value = 0;
    patterntable_value |= (ppu_address_bus_.read(pattern_tile_addr | patterntable_lo_bit_plane_select) &
                          (1 << patterntable_bit_for_pixel)) >> patterntable_bit_for_pixel;

    patterntable_value |= ((ppu_address_bus_.read(pattern_tile_addr | patterntable_hi_bit_plane_select) &
                          (1 << patterntable_bit_for_pixel)) >> (patterntable_bit_for_pixel))
                          << 1; // after getting bit and shifting to bit 0, shift left 1

    return static_cast<uint8_t>(patterntable_value);
}

uint8_t NesPPU::get_palette_index_for_pixel(uint8_t pixel_x, uint8_t pixel_y)
{
    // Get attribute table for the pixel in order to identify which palette should be used.
    // The palette indices for each nametable tile are stored with a byte for each 4x4 set tiles
    static constexpr uint16_t ATTRIBUTETABLE_TILE_SIZE = 32; // in pixels
    static constexpr uint16_t TILES_PER_ATTR_BYTE = 4; // bytes hold 4x4 tile data
    static constexpr uint16_t ATTRIBUTETABLE_WIDTH = 8; // bytes
    const uint8_t attributetable_x = pixel_x / ATTRIBUTETABLE_TILE_SIZE;
    const uint8_t attributetable_y = pixel_y / ATTRIBUTETABLE_TILE_SIZE;
    const uint16_t attribute_addr = cached_nametable_address_ + NAMETABLE_WIDTH * NAMETABLE_HEIGHT +
                                    attributetable_x + attributetable_y * ATTRIBUTETABLE_WIDTH;
    const uint8_t tile_y = pixel_y / NAMETABLE_TILE_SIZE;
    const uint8_t tile_x = pixel_x / NAMETABLE_TILE_SIZE;

    const uint8_t tile_offset_x = tile_x % TILES_PER_ATTR_BYTE;
    const uint8_t tile_offset_y = tile_y % TILES_PER_ATTR_BYTE;
    
    const uint8_t palette_index_byte = ppu_address_bus_.read(attribute_addr);

    if (tile_offset_x >= 2 && tile_offset_y >= 2) // bottom right
    {
        return (palette_index_byte >> 6) & 0x03;
    }
    else if (tile_offset_x <= 1 && tile_offset_y >= 2) // bottom left
    {
        return (palette_index_byte >> 4) & 0x03;
    }
    else if (tile_offset_x <= 1 && tile_offset_y <= 1) // top left
    {
        return (palette_index_byte >> 0) & 0x03;
    }
    else if (tile_offset_x >= 2 && tile_offset_y <= 1) // top right
    {
        return (palette_index_byte >> 2) & 0x03;
    }

    assert(false);
    return 0;
}

NesDisplay::Color NesPPU::fetch_color_from_palette(uint8_t palette_index, uint8_t colortable_index) const
{
    const uint16_t palette_base_address = 0x3F00;
    const uint16_t palette_addr = palette_base_address +
                                  (3 + 1) * palette_index; // 3 colors per, one space between
    const uint8_t palette_value = ppu_address_bus_.read(palette_addr + colortable_index);

    // RGB with each represented from 0-7. Scale each component to fill the 0-255 range when
    // generating the NestDisplay color to return.
    // This is the 2C03 palette from the nesdev wiki: https://www.nesdev.org/wiki/PPU_palettes
    static constexpr uint16_t COLOR_PALETTE[] =
    {0x333,0x014,0x006,0x326,0x403,0x503,0x510,0x420,0x320,0x120,0x031,0x040,0x022,0x00,0x00,0x00,
     0x555,0x036,0x027,0x407,0x507,0x704,0x700,0x630,0x430,0x140,0x040,0x053,0x044,0x00,0x00,0x00,
     0x777,0x357,0x447,0x637,0x707,0x737,0x740,0x750,0x660,0x360,0x070,0x276,0x077,0x00,0x00,0x00,
     0x777,0x567,0x657,0x757,0x747,0x755,0x764,0x772,0x773,0x572,0x473,0x276,0x467,0x00,0x00,0x00};

    static constexpr uint8_t COLOR_SCALE_FACTOR = 30;

    NesDisplay::Color color;
    color.r = ((COLOR_PALETTE[palette_value] >> 8) & 0x000F) * COLOR_SCALE_FACTOR;
    color.g = ((COLOR_PALETTE[palette_value] >> 4) & 0x000F) * COLOR_SCALE_FACTOR;
    color.b = ((COLOR_PALETTE[palette_value] >> 0) & 0x000F) * COLOR_SCALE_FACTOR;
    color.a = 0xFF;
    return color;
}

void NesPPU::read_sprite_oam()
{
    sprites_.clear();

    for (int16_t i = 0;i < 64;i++)
    {
        sprites_.push_back(sprite(i));

        NesPPU::Sprite& s = sprites_.back();

        if (s.tile_index == 0)
        {
            continue;
        }

        s.canvas = std::make_shared<Sprite::Canvas>();
    }
}

void NesPPU::render_sprites(Sprite::Layer layer)
{
    LOG_IF(FATAL, sprite_type() == SpriteType::Sprite_8x16) << "sprite size 8x16 not supported yet";

    if (sprites_.size() == 0)
    {
        return;
    }

    for (int16_t i = 63;i >= 0;i--) // sprite with lower address wins with overlapping sprites
    {
        const NesPPU::Sprite& s = sprites_[i];

        if (layer != s.layer()) // foreground / background check
        {
            continue;
        }

        if (s.tile_index == 0)
        {
            continue;
        }

        for (uint8_t p = 0;p < NAMETABLE_TILE_SIZE * NAMETABLE_TILE_SIZE;p++)
        {
            uint8_t tile_pixel_x = p % NAMETABLE_TILE_SIZE;
            uint8_t tile_pixel_y = p / NAMETABLE_TILE_SIZE;
            uint8_t debug_tile_pixel_x = tile_pixel_x;
            uint8_t debug_tile_pixel_y = tile_pixel_y;

            uint8_t pixel_x = s.x_pos + tile_pixel_x;
            uint8_t pixel_y = s.y_pos + tile_pixel_y + 1; // sprite data is delayed by one scanline https://www.nesdev.org/wiki/PPU_OAM

            if (s.flip_horizontal())
            {
                tile_pixel_x = NAMETABLE_TILE_SIZE - (p % NAMETABLE_TILE_SIZE) - 1;
            }
            if (s.flip_vertical())
            {
                tile_pixel_y = NAMETABLE_TILE_SIZE - (p / NAMETABLE_TILE_SIZE) - 1;
            }
            
            // Get the index into the color table from the pattern table tile
            const uint8_t colortable_index = get_colortable_index_for_tile_and_pixel(sprite_pattern_table_address(),
                                                                                     tile_pixel_x,
                                                                                     tile_pixel_y,
                                                                                     s.tile_index);
            if (colortable_index == 0)
            {
                continue; // transparent
            }


            if (i == 0) // sprite 0 hit check
            {
                // TODO only when hitting a non-transparent background pixel
                if (!(registers_[PPUSTATUS] & PPUSTATUS_sprite0_hit))
                {
                    registers_[PPUSTATUS] |= PPUSTATUS_vblank;
                }
            }

            // Determine which palette to use
            const uint8_t palette_index = 4 + (s.attributes & 0x3);

            // Retrieve the RGB color for this pixel
            const NesDisplay::Color pixel_color = fetch_color_from_palette(palette_index, colortable_index);

            // Draw the pixel!
            display_.draw_pixel(pixel_x, pixel_y, pixel_color);
            s.canvas.get()[debug_tile_pixel_y][debug_tile_pixel_x] = pixel_color;
        }
    }

    if (layer == Sprite::Layer::Foreground)
    {
        update_ui_sprites_view(sprites_);
    }
}

NesPPU::Sprite NesPPU::sprite(uint16_t index) const
{
    struct OamSprite
    {
        // maps to the data layout of oam data
        uint8_t y_pos;
        uint8_t tile_index;
        uint8_t attributes;
        uint8_t x_pos;
    };

    const uint8_t* oam_p = std::addressof(oam_memory_[index * sizeof(OamSprite)]);
    const OamSprite* sprite_p = reinterpret_cast<const OamSprite*>(oam_p);

    return Sprite(sprite_p->y_pos, sprite_p->tile_index, sprite_p->attributes, sprite_p->x_pos);
}

void NesPPU::handle_oam_data_register()
{
    if (registers_.had_write(OAMADDR))
    {
        registers_.clear_write_flag(OAMADDR);
        oam_data_addr_ = registers_[OAMADDR];
    }

    if (registers_.had_write(OAMDATA))
    {
        registers_.clear_write_flag(OAMDATA);

        // If the processor wrote to the PPUDATA register, write that data to the address pointed to
        // by ppu_data_addr in video memory. Then increment the address by the amount specified by
        // the control register (either horizontal or down).
        LOG_IF(ERROR, oam_addr_write_count % 2 != 0) << "Error: "
             << " write to OAMDATA without a fully set OAMADDR";

        oam_memory_[oam_data_addr_] = registers_[OAMDATA];
        oam_data_addr_ += 1;
    }
}

void NesPPU::handle_ppu_data_register()
{
    if (registers_.had_write(PPUADDR))
    {
        registers_.clear_write_flag(PPUADDR);
        if (ppu_addr_write_count % 2 == 0)
        {
            ppu_data_addr_ = 0x0;

            // first write, high address byte
            ppu_data_addr_ = registers_[PPUADDR] << 8;
        }
        else
        {
            // second write, low address byte
            ppu_data_addr_ |= registers_[PPUADDR];
        }
        ppu_addr_write_count++;
    }

    if (registers_.had_write(PPUDATA))
    {
        registers_.clear_write_flag(PPUDATA);
        // If the processor wrote to the PPUDATA register, write that data to the address pointed to
        // by ppu_data_addr in video memory. Then increment the address by the amount specified by
        // the control register (either horizontal or down).
        LOG_IF(ERROR, ppu_addr_write_count % 2 != 0) << "Error: "
                << " write to PPUDATA without a fully set PPUADDR";

        ppu_address_bus_.write(ppu_data_addr_, registers_[PPUDATA]);
        ppu_data_addr_ += ppu_addr_increment_amount();
    }
}

uint8_t NesPPU::read_ppu_data()
{
    LOG(FATAL) << "assertion for notification when a game actually uses this";
    assert(ppu_data_addr_ <= 0x3FFF);

    ppu_data_addr_ += ppu_addr_increment_amount();

    return ppu_address_bus_.read(ppu_data_addr_);
}

void NesPPU::handle_oam_dma_register()
{
    if (registers_.had_write(OAMDMA))
    {
        registers_.clear_write_flag(OAMDMA);

        uint16_t oam_src_addr = registers_[OAMDMA] << 8;
        for (uint16_t i = 0;i < oam_memory_.size();i++)
        {
            oam_memory_[i] = address_bus_.read(oam_src_addr + i);
        }
    }
}

void NesPPU::increment_cycle()
{
    cycle_++;
    if ( (frame_ % 2 == 0 && cycle_ >= 341) ||
         (frame_ % 2 == 1 && cycle_ >= 340) ) // There are one fewer cycles for odd frames
    {
        cycle_ = 0;
        scanline_++;

        if (scanline_ > 261)
        {
            scanline_ = 0;
            frame_++;
        }
   }
}

bool NesPPU::check_vblank_raising_edge() const
{
    return (scanline_ == 241 && cycle_ == 1);
}

bool NesPPU::check_vblank_falling_edge() const
{
    return (scanline_ == 261 && cycle_ == 1);
}

bool NesPPU::is_rendering_scanline() const
{
    return (scanline_ >= 0 && scanline_ <= 239);
}

bool NesPPU::check_rendering_falling_edge() const
{
    return (scanline_ == 239 && cycle_ == 340);
}

uint16_t NesPPU::pattern_table_base_address() const
{
    switch (read_register(PPUCTRL) & PPUCTRL_Backgroundtable_Select)
    {
        case 0:
            return 0x0000;

        case PPUCTRL_Backgroundtable_Select:
            return 0x1000;
    }
    assert(false);
    return 0;
}

uint16_t NesPPU::nametable_base_address()
{
    switch (read_register(PPUCTRL) & PPUCTRL_Nametable_Select)
    {
        case 0:
            return 0x2000;

        case 1:
            return 0x2400;

        case 2:
            return 0x2800;

        case 3:
            return 0x2C00;
    }
    assert(false);
    return 0;
}

uint16_t NesPPU::sprite_pattern_table_address() const
{
    switch(read_register(PPUCTRL) & PPUCTRL_SpriteTable_Addr)
    {
        case 0:
            return 0x0000;

        case PPUCTRL_SpriteTable_Addr:
            return 0x1000;
    }
    assert(false);
    return 0;
}

NesPPU::SpriteType NesPPU::sprite_type() const
{
    if (read_register(PPUCTRL) & PPUCTRL_SpriteSize_Select)
    {
        return SpriteType::Sprite_8x16;
    }
    return SpriteType::Sprite_8x8;
}

uint16_t NesPPU::ppu_addr_increment_amount()
{
    switch (read_register(PPUCTRL) & PPUCTRL_Incrmement_Direction)
    {
        case 0:
            return 1; // increment horizontally across the buffer

        case PPUCTRL_Incrmement_Direction:
            return 32; // increment vertically down the buffer
    }
    assert(false);
    return 0;
}

uint8_t NesPPU::read(uint16_t a) const
{
    return internal_memory_[a];
}

uint8_t& NesPPU::write(uint16_t a)
{
    return internal_memory_[a];
}

uint8_t NesPPU::read_palette_ram(uint16_t a) const
{
    return palette_ram_[a];
}

uint8_t& NesPPU::write_palette_ram(uint16_t a)
{
    return palette_ram_[a];
}
