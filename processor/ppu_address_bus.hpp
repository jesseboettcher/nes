#pragma once

#include "io/cartridge.hpp"
#include "processor/nes_ppu.hpp"

#include <array>
#include <cstdint>
#include <iostream>

class PPUAddressBus
{
public:
    using VideoMemory = std::array<uint8_t, 2 * 1024>;
    using PaletteRam = std::array<uint8_t, 0x20>;

    static const int32_t ADDRESSABLE_MEMORY_SIZE = 16 * 1024;

    PPUAddressBus()
    {
        clear();
    }

    struct View
    {
        uint16_t address;
        uint16_t size;
        const PPUAddressBus& memory;

        friend std::ostream& operator << (std::ostream& os, const View v);
    };
    struct NametableView : public View {};

    const uint8_t read(int32_t a) const
    {
        assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

        if (a < 0x2000) // cartridge
        {
            return cartridge_->ppu_read(a);
        }
        else if (a <= 0x3EFF) // video memory
        {
            return ppu_->read(handle_nametable_mirroring(a, vertical_mirroring_));
        }
        else if (a >= 0x3F00 && a < 0x4000) // Palette RAM indices
        {
            // return palette_ram_[(a - 0x3F00) % 0x20];
            return ppu_->read_palette_ram((a - 0x3F00) % 0x20);
        }

        assert(false);
        static uint8_t dummy = 0;
        return dummy;
    }

    // returns reference to memory to be written
    uint8_t& write(int32_t a)
    {
        assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

        if (a < 0x2000) // cartridge
        {
            assert(false);
            static uint8_t cartridge_nope = 0;
            return cartridge_nope;
        }
        else if (a <= 0x3EFF) // video memory
        {
            return ppu_->write(handle_nametable_mirroring(a, vertical_mirroring_));
        }
        else if (a >= 0x3F00 && a < 0x4000) // Palette RAM indices
        {
            // return palette_ram_[(a - 0x3F00) % 0x20];
            return ppu_->write_palette_ram((a - 0x3F00) % 0x20);
        }

        assert(false);
        static uint8_t dummy = 0;
        return dummy;
    }

    const uint8_t operator [] (int i) const
    {
        return read(i);
    }

    uint8_t& operator [] (int i)
    {
        return write(i);
    }

    const View view(uint16_t address, uint16_t size) const
    {
        if (address + size > ADDRESSABLE_MEMORY_SIZE)
        {
            size = ADDRESSABLE_MEMORY_SIZE - address;
        }
        return View({address, size, *this});
    }

    const NametableView nametable_view(uint16_t base_addr) const
    {
        return NametableView({base_addr, 0x0400, *this});
    }

    void clear()
    {
    }

    void attach_cartridge(std::shared_ptr<Cartridge> cartridge)
    {
        cartridge_ = cartridge;
        vertical_mirroring_ = cartridge_->vertical_nametable_mirroring();
    }
    void attach_ppu(std::shared_ptr<NesPPU> ppu) { ppu_ = ppu; }

private:
    uint16_t handle_nametable_mirroring(uint16_t addr, bool vertical_mirroring) const
    {
        // There are two levels of mirroring. The 4 nametable address space 0x2000 - 0x2FFF is
        // mirrored to the memory above it. Then the 4 nametable address spaces are reflected to
        // only 2 banks of 0x400 sized vram.

        // $2000-$23FF $0400   Nametable 0
        // $2400-$27FF $0400   Nametable 1
        // $2800-$2BFF $0400   Nametable 2
        // $2C00-$2FFF $0400   Nametable 3
        // $3000-$3EFF $0F00   Mirrors of $2000-$2EFF

        const uint16_t addr_unmirrored = addr >= 0x3000 ? addr - 0x1000 : addr; // shift everything to 0x2000 space
        const uint16_t nametable_number = (addr_unmirrored - 0x2000) / 0x400;
        const uint16_t nametable_offset = addr_unmirrored % 0x400;

        if (vertical_mirroring)
        {
            // vertical mirroring
            // A = nametable 0,1
            // B = nametable 2,3

            static const std::array<int32_t, 4> vertical_mirroring_vram_map = {0x0, 0x0, 0x400, 0x400};

            // map to physical vram
            return vertical_mirroring_vram_map[nametable_number] + nametable_offset;
        }
        else
        {
            // horizontal mirroring
            // A = nametable 0,2
            // B = nametable 1,4

            static const std::array<int32_t, 4> vertical_mirroring_vram_map = {0x0, 0x400, 0x00, 0x400};

            // map to physical vram
            return vertical_mirroring_vram_map[nametable_number] + nametable_offset;
        }
    }

    std::shared_ptr<Cartridge> cartridge_;
    std::shared_ptr<NesPPU> ppu_;

    bool vertical_mirroring_{false};
};
