#pragma once

#include "processor/processor_6502.hpp"
#include "processor/video_memory.hpp"

#include <ostream>

std::ostream& operator << (std::ostream& os, const Instruction& i);
std::ostream& operator << (std::ostream& os, const AddressBus::View v);
std::ostream& operator << (std::ostream& os, const AddressBus::StackView v);
std::ostream& operator << (std::ostream& os, const Registers& r);
std::ostream& operator << (std::ostream& os, const PPUAddressBus::NametableView v);
std::ostream& operator << (std::ostream& os, const NesPPU::Sprite s);

template<class... Args>
std::string strformat(std::string_view format, Args&&... args)
{
	static char buffer[100];
	snprintf(buffer, 100, format.data(), std::forward<Args>(args)...);

    return buffer;
}
