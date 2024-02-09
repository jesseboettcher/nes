#include "processor/nes_apu.hpp"

#include <glog/logging.h>

#include <cassert>

NesAPU::NesAPU()
{
	LOG(INFO) << "NesAPU created";
}

uint8_t NesAPU::read_register(uint16_t a) const
{
    assert(a == APU_STATUS || (a >= PULSE1_REG1 && a <= DMC_REG4));

    return 0;
}

uint8_t& NesAPU::write_register(uint16_t a)
{
    assert(a == APU_STATUS || a == APU_FRAME_COUNTER || (a >= PULSE1_REG1 && a <= DMC_REG4));

    static uint8_t dummy = 0;
    return dummy;
}
