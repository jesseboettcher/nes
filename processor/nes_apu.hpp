#pragma once

#include <cstdint>

class NesAPU
{
public:
	static constexpr uint16_t PULSE1_REG1 = 0x4000; // square waves
	static constexpr uint16_t PULSE1_REG2 = 0x4001;
	static constexpr uint16_t PULSE1_REG3 = 0x4002;
	static constexpr uint16_t PULSE1_REG4 = 0x4003;
	static constexpr uint16_t PULSE2_REG1 = 0x4004;
	static constexpr uint16_t PULSE2_REG2 = 0x4005;
	static constexpr uint16_t PULSE2_REG3 = 0x4006;
	static constexpr uint16_t PULSE2_REG4 = 0x4007;
	static constexpr uint16_t TRIANGLE_REG1 = 0x4008; // triangle wave
	static constexpr uint16_t TRIANGLE_REG2 = 0x4009;
	static constexpr uint16_t TRIANGLE_REG3 = 0x400A;
	static constexpr uint16_t TRIANGLE_REG4 = 0x400B;
	static constexpr uint16_t NOISE_REG1 = 0x400C;
	static constexpr uint16_t NOISE_REG2 = 0x400D;
	static constexpr uint16_t NOISE_REG3 = 0x400E;
	static constexpr uint16_t NOISE_REG4 = 0x400F;
	static constexpr uint16_t DMC_REG1 = 0x4010; // digital modulation channel (DMC)
	static constexpr uint16_t DMC_REG2 = 0x4011;
	static constexpr uint16_t DMC_REG3 = 0x4012;
	static constexpr uint16_t DMC_REG4 = 0x4013;
	static constexpr uint16_t APU_STATUS = 0x4015;
	static constexpr uint16_t APU_FRAME_COUNTER = 0x4017; // for writes, reads come from JOYPAD7

	NesAPU();

    // Accessors for the APU registers from AddressBus
    uint8_t read_register(uint16_t a) const;
    uint8_t& write_register(uint16_t a);
};
