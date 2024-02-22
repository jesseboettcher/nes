#pragma once

#include "platform/audio_player.hpp"
#include "lib/magic_enum.hpp"

#include <glog/logging.h>

#include <array>
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
    static constexpr uint16_t APU_STATUS_PULSE1_ENABLE = 0x0001;
    static constexpr uint16_t APU_STATUS_PULSE2_ENABLE = 0x0002;
    static constexpr uint16_t APU_STATUS_TRIANGLE_ENABLE = 0x0004;
    static constexpr uint16_t APU_FRAME_COUNTER = 0x4017; // for writes, reads come from JOYPAD7

    class Registers
    {
        // Fast lookup for registers with [] notation. Relies on the APU register addresses
        // being contiguous from 0x4000 through 0x4013. The 2 non-contiguous addresses are
        // special cased

    public:
        static constexpr int32_t REGISTER_COUNT = 22;

        Registers() {}

        // Operator[] for assignment
        uint8_t& operator[](uint16_t reg)
        {
            return values_[to_index(reg)];
        }

        uint8_t operator[](uint16_t reg) const { return values_[to_index(reg)]; }

        void clear() { values_.fill(0); }

        bool had_write(uint16_t reg) { return had_write_flags_[to_index(reg)]; }
        void set_had_write(uint16_t reg) { had_write_flags_[to_index(reg)] = true; }

        void clear_write_flags()
        {
             had_write_flags_.fill(false);
        }

    private:
        inline uint16_t to_index(uint16_t reg) const
        {
            if (reg == APU_STATUS)
            {
                return REGISTER_COUNT - 2;
            }
            else if (reg == APU_FRAME_COUNTER)
            {
                return REGISTER_COUNT - 1;
            }
            return reg - 0x4000;
        }

        std::array<uint8_t, REGISTER_COUNT> values_;
        std::array<bool, REGISTER_COUNT> had_write_flags_;
    };

    NesAPU();

    void reset();
    void step(uint64_t clock_ticks);

    // Needs to be called from the QT UI thread. Easiest to call once at startup and let run.
    void start();
    void stop();

    void test();

protected:
    // Accessors for the APU registers from AddressBus
    friend class AddressBus;
    uint8_t read_register(uint16_t a) const;
    uint8_t& write_register(uint16_t a);

private:
    int32_t to_index(Audio::Channel channel)
    {
        return magic_enum::enum_integer<Audio::Channel>(channel);
    }

    // register data accessors
    float get_duty(uint8_t r);
    bool get_loop(uint8_t r);
    bool get_constant_volume(uint8_t r);
    int16_t get_volume_envelope(uint8_t r);
    bool get_sweep_enabled(uint8_t r);
    bool get_period(uint8_t r);
    bool get_negate(uint8_t r);
    bool get_shift(uint8_t r);
    int16_t get_square_pulse_frequency(uint8_t r1, uint8_t r2);
    int16_t get_triangle_frequency(uint8_t r1, uint8_t r2);
    int16_t get_length_counter(uint8_t r);
    bool get_length_counter_halt(uint8_t r);
    uint16_t get_frame_counter_mode_steps(uint8_t r);
    int32_t get_linear_counter_load(uint8_t r);
    bool get_linear_counter_halt(uint8_t r);

    Registers registers_;
    int32_t steps_per_frame_{4};
    int32_t cycles_per_step_{0};
    uint64_t frame_steps_{0};
    uint64_t frame_count_{0};

    AudioPlayer player_;
    std::array<Audio::Parameters, magic_enum::enum_count<Audio::Channel>()> params_;

    uint64_t cycles_{0};
};
