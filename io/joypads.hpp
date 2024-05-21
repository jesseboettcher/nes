#pragma once

#include <queue>

class Joypads
{
public:
    static constexpr uint16_t JOYPAD1 = 0x4016;
    static constexpr uint16_t JOYPAD2 = 0x4017; // for reads, writes go to APU frame counter

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

    Joypads();

    void step(uint64_t clock_ticks);

    uint8_t read(uint16_t a) const;
    uint8_t& write(uint16_t a);

    uint64_t last_press_clock_ticks() { return last_press_clock_ticks_; }

private:
    uint8_t read(std::queue<bool>& snapshot) const;

    inline uint8_t& data(uint16_t addr)
    {
        return addr == JOYPAD1 ? joypad_1_data_ : joypad_2_data_;
    }

    uint8_t read_joypad_1_callback() const;
    uint8_t read_joypad_2_callback() const;

    bool strobe_bit_{false};

    uint8_t joypad_1_data_;
    uint8_t joypad_2_data_;

    mutable std::queue<bool> joypad_1_snapshot_;
    mutable std::queue<bool> joypad_2_snapshot_;

    uint64_t last_press_clock_ticks_{0};
};
