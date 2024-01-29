#include "io/cartridge_interface.hpp"

#include <glog/logging.h>

bool CartridgeInterface::load(Processor6502& processor, NesPPU& ppu, const NesFileParser& parsed_file)
{
	if (!parsed_file.valid())
	{
		LOG(ERROR) << "Invalid file, skipping load";
		return false;
	}
	if (parsed_file.mapper() != 0)
	{
		LOG(ERROR) << "Unsupported cartidge mapper";
		return false;
	}

	std::copy(parsed_file.prg_rom().begin(), parsed_file.prg_rom().end(),
			  processor.memory().begin() + 0x8000);

	if (parsed_file.sizeof_prg_rom() == 0x4000) // 16kb
	{
		// cartridges with 16kb of PRG ROM have their data mirrored at 0x8000, roms with 32kb will
		// full the entire space
		std::copy(parsed_file.prg_rom().begin(), parsed_file.prg_rom().end(),
				  processor.memory().begin() + 0x8000 + 0x4000);
	}

	if (auto maybe_chr_rom = parsed_file.chr_rom())
	{
		std::copy(maybe_chr_rom->begin(), maybe_chr_rom->end(), ppu.memory().begin());
	}
}

void CartridgeInterface::load(Processor6502& processor, NesPPU&, const MappedFile& generic_rom_file)
{
	auto dest_location = processor.memory().end() - generic_rom_file.buffer().size();
	std::copy(generic_rom_file.buffer().begin(), generic_rom_file.buffer().end(), dest_location);
}
