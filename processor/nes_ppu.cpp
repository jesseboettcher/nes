#include "processor/nes_ppu.hpp"

#include "platform/ui_properties.hpp"
#include "processor/processor_6502.hpp"
#include "processor/utils.hpp"

#include <glog/logging.h>

NesPPU::NesPPU(Processor6502& processor, NesDisplay& display)
: processor_(processor)
, display_(display)
{
    std::cout << "Launching NesPPU...\n";
    
    processor_.memory().add_write_notifier(OAMADDR, std::bind(&NesPPU::set_oamaddr_written, this));
    processor_.memory().add_write_notifier(OAMDATA, std::bind(&NesPPU::set_oamdata_written, this));
    processor_.memory().add_write_notifier(PPUADDR, std::bind(&NesPPU::set_ppuaddr_written, this));
    processor_.memory().add_write_notifier(PPUDATA, std::bind(&NesPPU::set_ppudata_written, this));
    processor_.memory().add_write_notifier(PPUDATA, std::bind(&NesPPU::set_ppudata_written, this));
    processor_.memory().add_write_notifier(OAMDMA, std::bind(&NesPPU::set_omadma_written, this));
}

NesPPU::~NesPPU()
{

}

void NesPPU::reset()
{
    memory_.clear();
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
        render_sprites(); // TODO integrate with proper timing and background rendering
        display_.render();
    }

    if (check_vblank_raising_edge()) // vsync
    {
        processor_.memory()[PPUSTATUS] |= PPUSTATUS_vblank;

        if (processor_.cmemory()[PPUCTRL] & PPUCTRL_Generate_NMI)
        {
            processor_.set_non_maskable_interrupt();
            display_.clear_screen();
        }   
    }

    if (check_vblank_falling_edge())
    {
        processor_.memory()[PPUSTATUS] &= ~PPUSTATUS_vblank;
    }

    oamaddr_written_ = false;
    oamdata_written_ = false;
    ppuaddr_written_ = false;
    ppudata_written_ = false;
    omadma_written_ = false;

    return true;
}

void NesPPU::render_pixel()
{
    // Determine which pixel we are working on
    const uint8_t pixel_x = cycle_;
    const uint8_t pixel_y = scanline_;

    // Get the index of the pattern tile from the nametable
    const uint8_t pattern_tile_index = get_pattern_tile_index_for_pixel(pixel_x, pixel_y);

    // Get the index into the color table from the pattern table tile
    const uint8_t colortable_index = get_colortable_index_for_tile_and_pixel(pattern_table_base_address(),
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
    const uint16_t nt_addr = nametable_base_address() + tile_x + tile_y * NAMETABLE_WIDTH;
    
    return memory_[nt_addr];;
}

uint8_t NesPPU::get_colortable_index_for_tile_and_pixel(uint16_t pattern_table_base_address,
                                                        uint8_t tile_pixel_x, uint8_t tile_pixel_y,
                                                        uint8_t pattern_tile_index) const
{
    // Get the pattern table tile from the value found in the nametable. The patterns have
    // 2 bits per pixel. Each pixel is stored in a different plane. Pull the appropriate
    // bit out of each plane and store it into a 2 bit number which represents the color
    // table index for the pixel in this tile.
    static constexpr uint16_t PATTERNTABLE_WIDTH = 16; // in bytes
    const uint8_t patterntable_x = pattern_tile_index % PATTERNTABLE_WIDTH;
    const uint8_t patterntable_y = pattern_tile_index / PATTERNTABLE_WIDTH;
    const uint16_t pattern_tile_addr = pattern_table_base_address |
                                       (patterntable_x << 4) |
                                       (patterntable_y << 8) |
                                       tile_pixel_y;

    const uint16_t patterntable_lo_bit_plane_select = 0x0000;
    const uint16_t patterntable_hi_bit_plane_select = 0x0008;
    const uint8_t patterntable_bit_for_pixel = 7 - tile_pixel_x;

    uint16_t patterntable_value = 0;
    patterntable_value |= (memory_[pattern_tile_addr | patterntable_lo_bit_plane_select] &
                          (1 << patterntable_bit_for_pixel)) >> patterntable_bit_for_pixel;

    patterntable_value |= ((memory_[pattern_tile_addr | patterntable_hi_bit_plane_select] &
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
    const uint16_t attribute_addr = nametable_base_address() + NAMETABLE_WIDTH * NAMETABLE_HEIGHT +
                                    attributetable_x + attributetable_y * ATTRIBUTETABLE_WIDTH;
    const uint8_t tile_y = pixel_y / NAMETABLE_TILE_SIZE;
    const uint8_t tile_x = pixel_x / NAMETABLE_TILE_SIZE;

    const uint8_t tile_offset_x = tile_x % TILES_PER_ATTR_BYTE;
    const uint8_t tile_offset_y = tile_y % TILES_PER_ATTR_BYTE;
    
    if (tile_offset_x >= 2 && tile_offset_y >= 2) // bottom right
    {
        return (memory_[attribute_addr] >> 6) & 0x03;
    }
    else if (tile_offset_x <= 1 && tile_offset_y >= 2) // bottom left
    {
        return (memory_[attribute_addr] >> 4) & 0x03;
    }
    else if (tile_offset_x <= 1 && tile_offset_y <= 1) // top left
    {
        return (memory_[attribute_addr] >> 0) & 0x03;
    }
    else if (tile_offset_x >= 2 && tile_offset_y <= 1) // top right
    {
        return (memory_[attribute_addr] >> 2) & 0x03;
    }

    assert(false);
    return 0;
}

NesDisplay::Color NesPPU::fetch_color_from_palette(uint8_t palette_index, uint8_t colortable_index) const
{
    const uint16_t palette_base_address = 0x3F00;
    const uint16_t palette_addr = palette_base_address +
                                  (3 + 1) * palette_index; // 3 colors per, one space between
    const uint8_t palette_value = memory_[palette_addr + colortable_index];

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

void NesPPU::render_sprites() const
{
    LOG_IF(FATAL, sprite_type() == SpriteType::Sprite_8x16) << "sprite size 8x16 not supported yet";

    std::vector<Sprite> sprites;

    for (int16_t i = 63;i >= 0;i--) // sprite with lower address wins with overlapping sprites
    {
        sprites.push_back(sprite(i));

        NesPPU::Sprite& s = sprites.back();

        if (s.tile_index == 0)
        {
            continue;
        }

        s.canvas = std::make_shared<Sprite::Canvas>();

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
            
            // Determine which palette to use
            const uint8_t palette_index = 4 + (s.attributes & 0x3);

            // Retrieve the RGB color for this pixel
            const NesDisplay::Color pixel_color = fetch_color_from_palette(palette_index, colortable_index);

            // Draw the pixel!
            display_.draw_pixel(pixel_x, pixel_y, pixel_color);
            s.canvas.get()[debug_tile_pixel_y][debug_tile_pixel_x] = pixel_color;
        }
    }
    update_ui_sprites_view(sprites);
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
    if (oamaddr_written_)
    {
        {
            oam_data_addr_ = processor_.cmemory()[OAMADDR];
            // LOG(INFO) << "write OAMADDR " << std::hex << "0x" << oam_data_addr_;
            // assert(false);
        }
        // if (oam_addr_write_count % 2 == 0)
        // {
        //  oam_data_addr_ = 0x0;

        //  // first write, high address byte
        //  oam_data_addr_ = processor_.cmemory()[OAMADDR] << 8;
        //  LOG(INFO) << "write OAMADDR high byte";
        // }
        // else
        // {
        //  // second write, low address byte
        //  oam_data_addr_ |= processor_.cmemory()[OAMADDR];
        //  LOG(INFO) << "write OAMADDR low byte " << std::hex << "0x" << oam_data_addr_;

        //  assert(false);
        // }
    }

    if (oamdata_written_)
    {
        // If the processor wrote to the PPUDATA register, write that data to the address pointed to
        // by ppu_data_addr in video memory. Then increment the address by the amount specified by
        // the control register (either horizontal or down).
        // LOG_IF(ERROR, oam_addr_write_count % 2 != 0) << "Error: "
        //      << " write to OAMDATA without a fully set OAMADDR";

        // memory_[oam_data_addr_] = processor_.cmemory()[OAMDATA];
        // oam_data_addr_ += oam_addr_increment_amount();
        LOG(INFO) << "write OAMDATA at " << std::hex << "0x" << oam_data_addr_ << " data 0x" << +memory_[oam_data_addr_];
//        assert(false);
    }
}

void NesPPU::handle_ppu_data_register()
{
    if (ppuaddr_written_)
    {
        if (ppu_addr_write_count % 2 == 0)
        {
            ppu_data_addr_ = 0x0;

            // first write, high address byte
            ppu_data_addr_ = processor_.cmemory()[PPUADDR] << 8;
        }
        else
        {
            // second write, low address byte
            ppu_data_addr_ |= processor_.cmemory()[PPUADDR];
        }
        ppu_addr_write_count++;
    }

    if (ppudata_written_)
    {
        // If the processor wrote to the PPUDATA register, write that data to the address pointed to
        // by ppu_data_addr in video memory. Then increment the address by the amount specified by
        // the control register (either horizontal or down).
        LOG_IF(ERROR, ppu_addr_write_count % 2 != 0) << "Error: "
                << " write to PPUDATA without a fully set PPUADDR";

        memory_[ppu_data_addr_] = processor_.cmemory()[PPUDATA];
        ppu_data_addr_ += ppu_addr_increment_amount();
    }
}

void NesPPU::handle_oam_dma_register()
{
    if (omadma_written_)
    {
        uint16_t oam_src_addr = processor_.cmemory()[OAMDMA] << 8;
        for (uint16_t i = 0;i < oam_memory_.size();i++)
        {
            oam_memory_[i] = processor_.cmemory()[oam_src_addr + i];
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
    switch (processor_.memory()[PPUCTRL] & PPUCTRL_Backgroundtable_Select)
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
    switch (processor_.memory()[PPUCTRL] & PPUCTRL_Nametable_Select)
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
    switch(processor_.memory()[PPUCTRL] & PPUCTRL_SpriteTable_Addr)
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
    if (processor_.memory()[PPUCTRL] & PPUCTRL_SpriteSize_Select)
    {
        return SpriteType::Sprite_8x16;
    }
    return SpriteType::Sprite_8x8;
}

uint16_t NesPPU::ppu_addr_increment_amount()
{
    switch (processor_.memory()[PPUCTRL] & PPUCTRL_Incrmement_Direction)
    {
        case 0:
            return 1; // increment horizontally across the buffer

        case PPUCTRL_Incrmement_Direction:
            return 32; // increment vertically down the buffer
    }
    assert(false);
    return 0;
}
