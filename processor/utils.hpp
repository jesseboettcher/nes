#pragma once

#include "processor/processor_6502.hpp"
#include "processor/nes_ppu.hpp"

#include <ostream>

std::ostream& operator << (std::ostream& os, const Instruction& i);
std::ostream& operator << (std::ostream& os, const Memory::View v);
std::ostream& operator << (std::ostream& os, const Memory::StackView v);
std::ostream& operator << (std::ostream& os, const Registers& r);
std::ostream& operator << (std::ostream& os, const VideoMemory::NametableView v);
std::ostream& operator << (std::ostream& os, const NesPPU::Sprite s);
