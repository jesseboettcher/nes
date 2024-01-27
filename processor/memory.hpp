#pragma once

#include <array>
#include <cassert>
#include <optional>
#include <span>

#include <glog/logging.h>

class Memory
{
public:
    enum class AccessType
    {
        NONE,
        READ,
        WRITE,
    };

    static const int32_t ADDRESSABLE_MEMORY_SIZE = 64 * 1024;

    using AddressableMemory = std::array<uint8_t, ADDRESSABLE_MEMORY_SIZE>;
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
        friend class Memory;

        View(int32_t address, int32_t size, const AddressableMemory& memory)
         : address_(address)
         , size_(size)
         , memory_(memory) {}

    private:
        int32_t address_;
        int32_t size_;
        const AddressableMemory& memory_;

        friend std::ostream& operator << (std::ostream& os, const View v);
    };
    struct StackView : public View
    {
        using View::View;
    };


    Memory()
    {
        memory_.fill(0);
    }

    const uint8_t operator [] (int32_t i) const
    {
        uint16_t addr = handle_address_mirrors(i);
        check_notifiers(read_notifiers_, i);

        // If a peripheral is connected at this address, read from it
        if (peripherals_.count(addr))
        {
            return peripherals_.at(addr)();
        }

        return memory_[addr];
    }

    uint8_t& operator [] (int32_t i)
    {
        uint16_t addr = handle_address_mirrors(i);
        check_notifiers(write_notifiers_, i);

        return memory_[addr];
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

    AddressableMemory::iterator begin() { return memory_.begin(); }
    AddressableMemory::iterator end() { return memory_.end(); }

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

    void add_peripheral_connection(uint16_t address, PeripheralRead fn)
    {
        peripherals_[address] = fn;
    }

private:
    uint16_t handle_address_mirrors(uint16_t addr) const
    {
//      if (addr >= 0x0800 && addr <= 0x1FFF)
//      {
//          // 2KB of CPU RAM is mirrored across the first 4 memory segments
//          return addr % 0x0800;
//      }
//      else if (addr >= 0x2008 && addr <= 0x3FFF)
//      {
//          // PPU registers at 0x2000-0x2007 are mirrored up to 0x3FFF
//          return 0x2000 + ((addr - 0x2000) % 8);
//      }
        return addr;
    }

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

    AddressableMemory memory_;

    // Peripherals are hw components that are connected at certain addresses
    // when the CPU reads from those addresses, it will return data peripheral data
    std::unordered_map<uint16_t, PeripheralRead> peripherals_;

    std::vector<std::pair<uint16_t, AccessNotifier>> read_notifiers_;
    std::vector<std::pair<uint16_t, AccessNotifier>> write_notifiers_;
};
