#pragma once

#include "io/cartridge.hpp"

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
        const VideoMemory& memory;

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
            return memory_[(a - 0x2000) % 0x1000];
        }
        else if (a >= 0x3F00 && a < 0x4000) // Palette RAM indices
        {
            return palette_ram_[(a - 0x3F00) % 0x20];
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
            return memory_[(a - 0x2000) % 0x1000];
        }
        else if (a >= 0x3F00 && a < 0x4000) // Palette RAM indices
        {
            return palette_ram_[(a - 0x3F00) % 0x20];
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
        if (address + size > memory_.size())
        {
            size = memory_.size() - address;
        }
        return View({address, size, memory_});
    }

    const NametableView nametable_view(uint16_t base_addr) const
    {
        return NametableView({base_addr, 0x0440, memory_});
    }

    VideoMemory::iterator begin() { return memory_.begin(); }
    VideoMemory::iterator end() { return memory_.end(); }

    void clear()
    {
        memory_.fill(0);
    }

    void attach_cartridge(std::shared_ptr<Cartridge> cartridge) { cartridge_ = cartridge; }

private:
    std::shared_ptr<Cartridge> cartridge_;

    VideoMemory memory_;
    PaletteRam palette_ram_;
};
