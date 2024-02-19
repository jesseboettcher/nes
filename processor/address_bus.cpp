#include "processor/address_bus.hpp"

#include "processor/processor_6502.hpp"


const uint8_t AddressBus::read(int32_t a) const
{
    assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

    if (a < cpu_->internal_memory_size()) // CPU memory
    {
        return cpu_->read(a % cpu_->internal_memory_size()); // mirrored after 0x07FF up to 0x1FFF
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
        return apu_->read_register(a);
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

void AddressBus::write(int32_t a, uint8_t value)
{
    assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

    if (a < cpu_->internal_memory_size()) // CPU memory
    {
        cpu_->write(a % cpu_->internal_memory_size()) = value; // mirrored after 0x07FF up to 0x1FFF
    }
    else if (a <= 0x3FFF) // PPU registers, mirrored after 0x2000 - 0x2007
    {
        ppu_->write_register(0x2000 + (a % 8)) = value;
    }
    else if (a <= 0x4017) // APU, IO registers
    {
        if (a == 0x4016 || a == 0x4017)
        {
            joypads_->write(a) = value;
        }
        else if (a == 0x4014) // OAM DMA
        {
            ppu_->write_register(a) = value;
        }
        else
        {
            apu_->write_register(a) = value;
        }
    }
    else if (a <= 0x401F) // disabled APU, IO functions
    {
        assert(false);
    }
    else
    {
        cartridge_->write(a, value);
    }
}
