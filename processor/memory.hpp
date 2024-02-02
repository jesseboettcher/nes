#pragma once

#include "io/cartridge.hpp"
#include "io/joypads.hpp"
#include "processor/nes_ppu.hpp"

#include <array>
#include <cassert>
#include <optional>
#include <span>

#include <glog/logging.h>

class AddressBus
{
public:
    enum class AccessType
    {
        NONE,
        READ,
        WRITE,
    };

    static const int32_t ADDRESSABLE_MEMORY_SIZE = 64 * 1024;

    using CPUMemory = std::array<uint8_t, 2 * 1024>; // 2kb ram
    using AccessNotifier = std::function<void()>;
    using PeripheralRead = std::function<uint8_t()>;

    struct View
    {
        const uint8_t operator [] (int32_t i) const
        {
            // enforce view bounds
            assert(i >= address() & i < address() + size());

            return memory_[i];
        }
        int32_t address() const { return address_; }
        int32_t size() const { return size_; }

    protected:
        friend class AddressBus;

        View(int32_t address, int32_t size, const CPUMemory& memory)
         : address_(address)
         , size_(size)
         , memory_(memory) {}

    private:
        int32_t address_;
        int32_t size_;
        const CPUMemory& memory_;

        friend std::ostream& operator << (std::ostream& os, const View v);
    };
    struct StackView : public View
    {
        using View::View;
    };


    AddressBus()
    {
        memory_.fill(0);
    }

    const uint8_t read(int32_t a) const
    {
        assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

        if (a < 0x2000) // CPU memory
        {
            return memory_[a % 0x0800]; // mirrored after 0x07FF up to 0x1FFF
        }
        else if (a <= 0x3FFF) // PPU registers, mirrored after 0x2000 - 0x2007
        {
            return ppu_->read(0x2000 + (a % 8));
        }
        else if (a <= 0x4017) // APU, IO registers
        {
            if (a == 0x4016 || a == 0x4017)
            {
                return joypads_->read(a);
            }
            if (a == 0x4014) // OAM DMA
            {
                return ppu_->read(a);
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
    uint8_t& write(int32_t a)
    {
        assert(a >= 0 && a < ADDRESSABLE_MEMORY_SIZE);

        if (a < 0x2000) // CPU memory
        {
            return memory_[a % 0x0800]; // mirrored after 0x07FF up to 0x1FFF
        }
        else if (a <= 0x3FFF) // PPU registers, mirrored after 0x2000 - 0x2007
        {
            return ppu_->write(0x2000 + (a % 8));
        }
        else if (a <= 0x4017) // APU, IO registers
        {
            if (a == 0x4016 || a == 0x4017)
            {
                return joypads_->write(a);
            }
            if (a == 0x4014) // OAM DMA
            {
                return ppu_->write(a);
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

    const uint8_t operator [] (int32_t i) const
    {
        check_notifiers(read_notifiers_, i);

        return read(i);
    }

    uint8_t& operator [] (int32_t i)
    {
        check_notifiers(write_notifiers_, i);

        return write(i);
    }

    const View view(int32_t address, int32_t size) const
    {
        if (address + size > memory_.size())
        {
            size = memory_.size() - address;
        }
        return View(address, size, memory_);
    }

    const StackView stack_view(uint16_t stack_pointer) const
    {
        uint16_t stack_size = 0xFF - stack_pointer;
        stack_pointer++;
        uint16_t stack_bottom = 0x0100 + stack_pointer;
        return StackView(stack_bottom, stack_size, memory_);
    }

    void stack_push(uint8_t& sp, uint8_t data)
    {
        (*this)[0x0100 + sp] = data;
        sp--;
    }

    uint8_t stack_pop(uint8_t& sp)
    {
        sp++;
        return (*this)[0x0100 + sp];
    }

    CPUMemory::iterator begin() { return memory_.begin(); }
    CPUMemory::iterator end() { return memory_.end(); }

    bool add_read_notifier(uint16_t address, AccessNotifier notifier)
    {
        read_notifiers_.push_back({address, notifier});
        return true;
    }

    bool add_write_notifier(uint16_t address, AccessNotifier notifier)
    {
        write_notifiers_.push_back({address, notifier});
        return true;
    }

    void attach_cartridge(std::shared_ptr<Cartridge> cartridge) { cartridge_ = cartridge; }
    void attach_joypads(std::shared_ptr<Joypads> joypads) { joypads_ = joypads; }
    void attach_ppu(std::shared_ptr<NesPPU> ppu) { ppu_ = ppu; }

private:
    void check_notifiers(const std::vector<std::pair<uint16_t, AccessNotifier>>& notifiers, const uint16_t access_addr) const
    {
        for (auto& [addr, notifier] : notifiers)
        {
            if (addr == access_addr)
            {
                notifier();
            }
        }
    }

    CPUMemory memory_;

    std::vector<std::pair<uint16_t, AccessNotifier>> read_notifiers_;
    std::vector<std::pair<uint16_t, AccessNotifier>> write_notifiers_;

    std::shared_ptr<Cartridge> cartridge_;
    std::shared_ptr<Joypads> joypads_;
    std::shared_ptr<NesPPU> ppu_;
};
