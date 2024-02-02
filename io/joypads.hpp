#pragma once

#include <queue>
#include <unordered_map>

class Joypads
{
public:
    static constexpr uint16_t JOYPAD1 = 0x4016;
    static constexpr uint16_t JOYPAD2 = 0x4017;

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

    void step();

    uint8_t read(uint16_t a) const;
    uint8_t& write(uint16_t a);

private:
    uint8_t read(std::queue<bool>& snapshot) const;

    uint8_t read_joypad_1_callback() const;
    uint8_t read_joypad_2_callback() const;

    bool strobe_bit_{false};

    std::unordered_map<uint16_t, uint8_t> values_;

    mutable std::queue<bool> joypad_1_snapshot_;
    mutable std::queue<bool> joypad_2_snapshot_;
};
