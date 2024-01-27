#pragma once

#include "processor/processor_6502.hpp"

#include <queue>
#include <unordered_map>

class Joypads
{
public:
	static constexpr int16_t JOYPAD1 = 0x4016;
	static constexpr int16_t JOYPAD2 = 0x4017;

	enum class Button
	{
		// order is important. this is the order the buttons are returned
		A = 0,
		B,
		Select,
		Start,
		Up,
		Down,
		Left,
		Right,
	};

	Joypads(Processor6502& processor);

	void step();

	uint8_t read(std::queue<bool>& snapshot);

	uint8_t read_joypad_1_callback();
	uint8_t read_joypad_2_callback();

private:
	Processor6502& processor_;

	bool strobe_bit_{false};

	std::unordered_map<Button, bool> map_;
	std::queue<bool> joypad_1_snapshot_;
	std::queue<Button> joypad_1_snapshot_label_;
	std::queue<bool> joypad_2_snapshot_;
};
