#pragma once

#include "io/files.hpp"
#include "processor/processor_6502.hpp"
#include "processor/nes_ppu.hpp"

class CartridgeInterface
{
public:
	// Copies the PRG ROM to the last 16kb of the processor's memory
	static bool load(Processor6502& processor, NesPPU& ppu, const NesFileParser& parsed_file);

	// Copies the PRG ROM to the last 16kb of the processor's memory
	static void load(Processor6502& processor, NesPPU& ppu, const MappedFile& generic_rom_file);
};
