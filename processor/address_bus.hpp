#pragma once

#include "io/cartridge.hpp"
#include "io/joypads.hpp"
#include "processor/nes_apu.hpp"
#include "processor/nes_ppu.hpp"

#include <array>
#include <cassert>
#include <optional>
#include <span>

#include <glog/logging.h>

// forward def
class Processor6502;

class AddressBus
{
public:
    enum class AccessType
    {
        NONE,
        READ,
        WRITE,
    };

    static constexpr int32_t ADDRESSABLE_MEMORY_SIZE = 64 * 1024;

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

        View(int32_t address, int32_t size, const AddressBus& memory)
         : address_(address)
         , size_(size)
         , memory_(memory) {}

    private:
        int32_t address_;
        int32_t size_;
        const AddressBus& memory_;

        friend std::ostream& operator << (std::ostream& os, const View v);
    };
    struct StackView : public View
    {
        using View::View;
    };


    AddressBus()
    {
    }

    const uint8_t read(int32_t a) const;

    // returns reference to memory to be written
    uint8_t& write(int32_t a);

    const uint8_t operator [] (int32_t i) const
    {
        return read(i);
    }

    uint8_t& operator [] (int32_t i)
    {
        return write(i);
    }

    const View view(int32_t address, int32_t size) const
    {
        if (address + size > ADDRESSABLE_MEMORY_SIZE)
        {
            size = ADDRESSABLE_MEMORY_SIZE - address;
        }
        return View(address, size, *this);
    }

    const StackView stack_view(uint16_t stack_pointer) const
    {
        uint16_t stack_size = 0xFF - stack_pointer;
        stack_pointer++;
        uint16_t stack_bottom = 0x0100 + stack_pointer;
        return StackView(stack_bottom, stack_size, *this);
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

    void attach_cpu(std::shared_ptr<Processor6502> cpu) { cpu_ = cpu; }
    void attach_cartridge(std::shared_ptr<Cartridge> cartridge) { cartridge_ = cartridge; }
    void attach_joypads(std::shared_ptr<Joypads> joypads) { joypads_ = joypads; }
    void attach_ppu(std::shared_ptr<NesPPU> ppu) { ppu_ = ppu; }
    void attach_apu(std::shared_ptr<NesAPU> apu) { apu_ = apu; }

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

    std::shared_ptr<Processor6502> cpu_;
    std::shared_ptr<Cartridge> cartridge_;
    std::shared_ptr<Joypads> joypads_;
    std::shared_ptr<NesPPU> ppu_;
    std::shared_ptr<NesAPU> apu_;
};
