#pragma once

#include <array>
#include <cstdint>
#include <iostream>

class VideoMemory
{
public:
	using AddressableMemory = std::array<uint8_t, 16 * 1024>;

	VideoMemory()
	{
		clear();
	}

	struct View
	{
		uint16_t address;
		uint16_t size;
		const AddressableMemory& memory;

		friend std::ostream& operator << (std::ostream& os, const View v);
	};
	struct NametableView : public View {};

	const uint8_t& operator [] (int i) const
	{
		return memory_[handle_address_mirrors(i)];
	}

	uint8_t& operator [] (int i)
	{
		return memory_[handle_address_mirrors(i)];
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

	AddressableMemory::iterator begin() { return memory_.begin(); }
	AddressableMemory::iterator end() { return memory_.end(); }

	void clear()
	{
		memory_.fill(0);
	}

private:
	uint16_t handle_address_mirrors(uint16_t addr) const
	{
		if (addr >= 0x3000 && addr <= 0x3EFF)
		{
			// mirrors of names tables between $2000-$2EFF
			return 0x2000 + ((addr - 0x3000) % 0x0400);
		}
		else if (addr >= 0x3F20 && addr <= 0x3FFF)
		{
			// mirrors of palette ram indices
			return 0x3F00 + ((addr - 0x3F20) % 0x20);
		}
		return addr;
	}

	AddressableMemory memory_;
};
