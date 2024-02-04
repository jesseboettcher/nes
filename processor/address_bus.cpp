#include "processor/address_bus.hpp"

#include "processor/processor_6502.hpp"


const uint8_t AddressBus::read(int32_t a) const
{
    assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

    if (a < 0x2000) // CPU memory
    {
        return cpu_->read(a % 0x0800); // mirrored after 0x07FF up to 0x1FFF
    }
    else if (a <= 0x3FFF) // PPU registers, mirrored after 0x2000 - 0x2007
    {
        return ppu_->read_register(0x2000 + (a % 8));
    }
    else if (a <= 0x4017) // APU, IO registers
    {
        if (a == 0x4016 || a == 0x4017)
        {
            return joypads_->read(a);
        }
        if (a == 0x4014) // OAM DMA
        {
            return ppu_->read_register(a);
        }

        // APU
        return 0;
    }
    else if (a <= 0x401F) // disabled APU, IO functions
    {
        return 0;
    }
    else if (cartridge_)
    {
        return cartridge_->read(a);
    }
    else
    {
        return 0; // no cartridge, just return 0
    }
}

// returns reference to memory to be written
uint8_t& AddressBus::write(int32_t a)
{
    assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

    if (a < 0x2000) // CPU memory
    {
        return cpu_->write(a % 0x0800); // mirrored after 0x07FF up to 0x1FFF
    }
    else if (a <= 0x3FFF) // PPU registers, mirrored after 0x2000 - 0x2007
    {
        return ppu_->write_register(0x2000 + (a % 8));
    }
    else if (a <= 0x4017) // APU, IO registers
    {
        if (a == 0x4016 || a == 0x4017)
        {
            return joypads_->write(a);
        }
        if (a == 0x4014) // OAM DMA
        {
            return ppu_->write_register(a);
        }

        // APU
        static uint8_t apu_placeholder = 0;
        return apu_placeholder;
    }
    else if (a <= 0x401F) // disabled APU, IO functions
    {
        assert(false);
    }
    else
    {
        assert(false); // shouldn't happen with mapper zero
    }

    assert(false);
    static uint8_t dummy = 0;
    return dummy;
}
