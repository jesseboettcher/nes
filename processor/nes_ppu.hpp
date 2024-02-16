#pragma once

#include "io/display.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>

class AddressBus;
class PPUAddressBus;

class NesPPU
{
public:
    static constexpr uint16_t PPUCTRL   = 0x2000;
    static constexpr uint8_t  PPUCTRL_Generate_NMI = 0x80;
    static constexpr uint8_t  PPUCTRL_Nametable_Select = 0x03;
    static constexpr uint8_t  PPUCTRL_Incrmement_Direction = 0x04;
    static constexpr uint8_t  PPUCTRL_SpriteTable_Addr = 0x08;
    static constexpr uint8_t  PPUCTRL_BackgroundTable_Addr = 0x10;
    static constexpr uint8_t  PPUCTRL_Backgroundtable_Select = 0x10;
    static constexpr uint8_t  PPUCTRL_SpriteSize_Select = 0x20;
    static constexpr uint16_t PPUMASK   = 0x2001;
    static constexpr uint16_t PPUMASK_GREYSCALE   = 0x01; // TODO
    static constexpr uint16_t PPUMASK_SHOW_BACKGROUND_LEFT_EDGE   = 0x02; // TODO
    static constexpr uint16_t PPUMASK_SHOW_SPRITES_LEFT_EDGE   = 0x04; // TODO
    static constexpr uint16_t PPUMASK_BACKGROUND   = 0x08; // TODO
    static constexpr uint16_t PPUMASK_SPRITES   = 0x10; // TODO
    static constexpr uint16_t PPUMASK_COLOR_EMPHASIS   = 0xE0; // TODO
    static constexpr uint16_t PPUSTATUS = 0x2002;
    static constexpr uint8_t  PPUSTATUS_vblank = 0x80;
    static constexpr uint8_t  PPUSTATUS_sprite0_hit = 0x40;
    static constexpr uint16_t OAMADDR   = 0x2003;
    static constexpr uint16_t OAMDATA   = 0x2004;
    static constexpr uint16_t PPUSCROLL = 0x2005;
    static constexpr uint16_t PPUADDR   = 0x2006;
    static constexpr uint16_t PPUDATA   = 0x2007;
    static constexpr uint16_t OAMDMA    = 0x4014;

    static constexpr uint16_t SCANLINES = 262;
    static constexpr uint16_t PIXELS_PER_LINE = 341;

    static constexpr uint16_t NAMETABLE_WIDTH = 32;
    static constexpr uint16_t NAMETABLE_HEIGHT = 30;
    static constexpr uint16_t NAMETABLE_TILE_SIZE = 8;

    using OAMMemory = std::array<uint8_t, 256>;
    using PaletteRam = std::array<uint8_t, 0x20>;
    using VideoMemory = std::array<uint8_t, 2 * 1024>;


    class OamDmaRegister
    {
    public:
        uint8_t value() { return value_; }
        bool had_write() { return had_write_; }

        uint8_t& write()
        {
            had_write_ = true;
            return value_;
        }

        void clear_write_flag()
        {
            had_write_ = false;
        }

    private:
        uint8_t value_;
        bool had_write_;
    };

    class Registers
    {
        // Fast lookup for registers with [] notation. Relies on the PPU register addresses
        // being contiguous from 0x2000 through 0x2007, special case for OAMDMA which is at
        // 0x4014 and actually supposed to be handled by the main CPU.

    public:
        static constexpr int32_t REGISTER_COUNT = 8;

        Registers() : values_(REGISTER_COUNT, 0x00), had_write_flags_(REGISTER_COUNT, false) {}

        // Operator[] for assignment
        uint8_t& operator[](uint16_t reg)
        {
            return values_[to_index(reg)];
        }

        uint8_t operator[](uint16_t reg) const { return values_[to_index(reg)]; }

        bool had_write(uint16_t reg) { return had_write_flags_[to_index(reg)]; }
        void set_had_write(uint16_t reg) { had_write_flags_[to_index(reg)] = true; }

        void clear_write_flag(int16_t reg)
        {
             had_write_flags_[to_index(reg)] = false;
        }

    private:
        inline uint16_t to_index(uint16_t reg) const
        {
            return reg - 0x2000;
        }

        std::vector<uint8_t> values_;
        std::vector<bool> had_write_flags_;
    };

    enum class SpriteType
    {
        Sprite_8x8,
        Sprite_8x16
    };

    struct Sprite
    {
        using Canvas = NesDisplay::Color[8][8];

        enum class Layer
        {
            Background,
            Foreground
        };

        Sprite(uint8_t y, uint8_t index, uint8_t attr, uint8_t x, uint16_t base_addr)
        : y_pos(y)
        , x_pos(x)
        , attributes(attr)
        , tile_index(index)
        , pattern_table_base_address(base_addr)
        {}

        uint8_t y_pos;
        uint8_t tile_index;
        uint8_t attributes;
        uint8_t x_pos;

        uint16_t pattern_table_base_address;

        std::shared_ptr<Canvas> canvas;

        bool flip_vertical() const { return attributes & 0x80; }
        bool flip_horizontal() const { return attributes & 0x40; }
        Layer layer() const { return (attributes & 0x20) ? Layer::Background : Layer::Foreground; }
    };
    
    NesPPU(AddressBus& address_bus, PPUAddressBus& ppu_address_bus, NesDisplay& display, bool& nmi_signal);
    ~NesPPU();

    // Reset registers and initialize PC to values specified by reset vector
    void reset();

    // Run and execute instructions from memory
    void run();

    // Step the processor 1 cycle. Returns true if the processor should continue running
    bool step();

    // Accessors for the PPU registers from AddressBus
    uint8_t read_register(uint16_t a) const;
    uint8_t& write_register(uint16_t a);

    const PPUAddressBus& cmemory() { return ppu_address_bus_; }

    // get the base address of the current nametable
    uint16_t nametable_base_address();
    uint16_t nametable_base_address_for_pixel(uint16_t pixel_x, uint16_t pixel_y);

    // get the base address of the current pattern table
    uint16_t pattern_table_base_address() const;

    // base address of the pattern table used for sprites
    uint16_t sprite_pattern_table_address(std::optional<uint8_t> tile_byte_1 = std::nullopt) const;

    SpriteType sprite_type() const;

    // Get index into the pattern table for the provided pixel, retreived from the nametable
    uint8_t get_pattern_tile_index_for_pixel(uint16_t pixel_x, uint16_t pixel_y);

    // Get the index into the color table from the pattern table tile
    uint8_t get_colortable_index_for_tile_and_pixel(uint16_t pattern_table_base_address,
                                                    uint16_t pixel_x, uint16_t pixel_y,
                                                    uint8_t pattern_tile_index) const;
    // Determine which palette to use
    uint8_t get_palette_index_for_pixel(uint16_t pixel_x, uint16_t pixel_y);

    // Retrieve the RGB color for the provided palette and color table index
    NesDisplay::Color fetch_color_from_palette(uint8_t palette_index, uint8_t colortable_index) const;

    // Retrieve sprite data from OAM memory
    Sprite sprite(uint16_t index) const;

protected:
    friend class Nes;
    PPUAddressBus& memory() { return ppu_address_bus_; }

    friend class PPUAddressBus;
    uint8_t read(uint16_t a) const;
    uint8_t& write(uint16_t a);
    uint8_t read_palette_ram(uint16_t a) const;
    uint8_t& write_palette_ram(uint16_t a);

private:
    // Draws the background pixel for the current cycle
    void render_pixel();

    // Read oam sprite data
    // Populates a list of Sprite structures with data from OAM
    void read_sprite_oam();

    // Draws all the foreground or background sprites at once
    void render_sprites(Sprite::Layer layer);

    // Make vram updates based on any activity on OAMADDR and OAMDATA
    void handle_oam_data_register();

    // Make vram updates based on any activity on PPUADDR and PPUDATA
    void handle_ppu_data_register();
    uint8_t read_ppu_data();

    // Make vram updates based on any activity on OAMDMA
    void handle_oam_dma_register();

    void handle_scroll_register();

    void increment_cycle();

    // check the current scanline and cycle and return true if it represents the start or end
    // of vertical blanking
    bool check_vblank_raising_edge() const;
    bool check_vblank_falling_edge() const;
    bool is_rendering_scanline() const;
    bool check_rendering_falling_edge() const;

    // amount to increment PPUADDR after a write to PPUDATA
    uint16_t ppu_addr_increment_amount();
    
    bool oamaddr_written_{false};
    bool oamdata_written_{false};
    bool ppuaddr_written_{false};
    bool ppudata_written_{false};
    bool oamdma_written_{false};

    AddressBus& address_bus_;
    PPUAddressBus& ppu_address_bus_;

    NesDisplay& display_;
    OAMMemory oam_memory_;
    bool& nmi_signal_;

    Registers registers_;
    OamDmaRegister oam_dma_register_;
    VideoMemory internal_memory_;
    PaletteRam palette_ram_;

    std::vector<Sprite> sprites_;

    uint16_t cached_nametable_address_{0};
    uint16_t cached_patterntable_address{0};

    // scanline_
    // 0-239 rendering
    // 240 idle
    // 241-260 vertical blanking
    // 261 pre-render
    uint32_t    cycle_{341};
    uint32_t    scanline_{260};
    uint64_t    frame_{0};

    uint16_t oam_data_addr_{0};
    uint64_t oam_addr_write_count{0};

    uint16_t ppu_data_addr_{0};
    uint64_t ppu_addr_write_count{0};

    uint64_t scroll_write_count_;
    std::pair<int32_t, int32_t> scroll_;
    int32_t pending_scroll_x_{0};
    int32_t pending_scroll_y_{0};
};
