#include "utils.hpp"

#include <bitset>
#include <iomanip>

std::ostream& operator << (std::ostream& os, const Instruction& i)
{
	os << " 0x" << std::hex << std::uppercase << std::setw(2) << +i.opcode();

	if (i.values.size() >= 2)
	{
		os	<< " 0x" << std::setw(2) << std::setfill('0') << +i.values[1];
	}
	if (i.values.size() == 3)
	{
		os	<< " 0x" << std::setw(2) << std::setfill('0') << +i.values[2];
	}
	return os;
}

std::ostream& operator << (std::ostream& os, const Memory::View v)
{
	os << std::hex << std::setfill('0');
	os << "\n-------------\n";
	os << "  Memory\n";
	os << "-------------\n";

	static constexpr uint8_t BYTES_PER_BLOCK = 2;
	static constexpr uint8_t BYTES_PER_LINE = 4;

	for (uint16_t i = 0; i < v.size; ++i)
	{
		if (i % BYTES_PER_LINE == 0)
		{
			if (i > 0)
			{
				os << std::endl;
			}

			// address prefix
			os << "0x" << std::setw(4) << v.address + i << "    ";
		}
		else if (i % BYTES_PER_BLOCK == 0)
		{
			os << " ";
		}

		os << std::setw(2) << +v.memory[v.address + i];
	}
	os << std::dec << std::endl << std::endl;

	return os;
}

std::ostream& operator << (std::ostream& os, const VideoMemory::View v)
{
	os << std::hex << std::setfill('0');
	os << "\n-------------\n";
	os << "  Video Memory\n";
	os << "-------------\n";

	static constexpr uint8_t BYTES_PER_BLOCK = 2;
	static constexpr uint8_t BYTES_PER_LINE = 4;

	for (uint16_t i = 0; i < v.size; ++i)
	{
		if (i % BYTES_PER_LINE == 0)
		{
			if (i > 0)
			{
				os << std::endl;
			}

			// address prefix
			os << "0x" << std::setw(4) << v.address + i << "    ";
		}
		else if (i % BYTES_PER_BLOCK == 0)
		{
			os << " ";
		}

		os << std::setw(2) << +v.memory[v.address + i];
	}
	os << std::dec << std::endl << std::endl;

	return os;
}

std::ostream& operator << (std::ostream& os, const Memory::StackView v)
{
	os << "\n-------------\n";
	os << "  Stack (" << v.size << ")\n";
	os << "-------------\n";
	os << std::hex << std::setfill('0');

	static constexpr uint8_t BYTES_PER_BLOCK = 1;
	static constexpr uint8_t BYTES_PER_LINE = 1;

	for (uint16_t i = 0; i < v.size; ++i)
	{
		if (i % BYTES_PER_LINE == 0)
		{
			if (i > 0)
			{
				os << std::endl;
			}

			// address prefix
			os << "0x" << std::setw(4) << v.address + i << "    ";
		}
		else if (i % BYTES_PER_BLOCK == 0)
		{
			os << " ";
		}

		os << std::setw(2) << +v.memory[v.address + i];
	}
	os << std::dec << std::endl << std::endl;

	return os;
}

std::ostream& operator << (std::ostream& os, const Registers& r)
{

	os << std::hex;
	os << "\n-------------\n";
	os << "  Registers\n";
	os << "-------------\n";
	os << "  PC: 0x" << std::setw(4) <<r.PC << std::endl;
	os << "   A: 0x" << std::setw(2) << +r.A << std::endl;
	os << "   X: 0x" << std::setw(2) << +r.X << std::endl;
	os << "   Y: 0x" << std::setw(2) << +r.Y << std::endl;
	os << "  SP: 0x" << std::setw(2) << +r.SP << std::endl;
	os << "  SR: b" << std::bitset<8>(r.SR()) << std::endl;
	os << "       " << "NV BDIZC";
	os << std::dec << std::endl << std::endl;

	return os;
}

std::ostream& operator << (std::ostream& os, const VideoMemory::NametableView v)
{
	os << std::hex << std::setfill('0');
	os << "\n-------------\n";
	os << "  Nametable\n";
	os << "-------------\n";

	static constexpr uint8_t BYTES_PER_BLOCK = 1;
	static constexpr uint8_t BYTES_PER_LINE = 32;

	for (uint16_t i = 0; i < v.size; ++i)
	{
		if (i % BYTES_PER_LINE == 0)
		{
			if (i > 0)
			{
				os << std::endl;
			}

			// address prefix
			os << "0x" << std::setw(4) << v.address + i << "    ";
		}
		else if (i % BYTES_PER_BLOCK == 0)
		{
			os << " ";
		}

		os << std::setw(2) << +v.memory[v.address + i];
	}
	os << std::dec << std::endl << std::endl;

	return os;

}

std::ostream& operator << (std::ostream& os, const NesPPU::Sprite s)
{
    os << std::dec;
    os << "pos " << +s.x_pos << "," << +s.y_pos;
    os << std::hex << std::setfill('0');
    os << " tile " << +s.tile_index << " attr " << +s.attributes;
    return os;
}
