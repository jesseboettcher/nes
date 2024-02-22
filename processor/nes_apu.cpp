#include "processor/nes_apu.hpp"

#include "platform/audio_player.hpp"

#include <glog/logging.h>

int32_t get_cycles_per_frame_step(int32_t steps_per_frame)
{
    // master clock ticks -> 21477272 @ 21.477272 MHz
    return 21477272 / (60 * steps_per_frame);
}

int32_t get_cycles_per_frame()
{
    // master clock ticks -> 21477272 @ 21.477272 MHz
    return 21477272 / 60;
}

NesAPU::NesAPU()
{
    LOG(INFO) << "NesAPU created";

    reset();
}

void NesAPU::reset()
{
    registers_.clear();

    steps_per_frame_ = 4;
    cycles_per_step_ = get_cycles_per_frame_step(steps_per_frame_);

    frame_count_ = 0;
    frame_steps_ = 0;

    cycles_ = 0;

    for (auto channel : magic_enum::enum_values<Audio::Channel>())
    {
        params_[to_index(channel)] = Audio::Parameters();
        params_[to_index(channel)].channel = channel;
    }
}


void NesAPU::test()
{
    player_.test();
}

int32_t length_counter_lookup(int16_t v)
{
    // The 5-bit value written to each channels length counter register maps to this lookup table
    // that translate to longer values corresponding to musical note lengths.

    //          |  0   1   2   3   4   5   6   7    8   9   A   B   C   D   E   F
    // -----+----------------------------------------------------------------
    // 00-0F  10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    // 10-1F  12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30

    static constexpr std::array<uint8_t, 0x10> low_lookup =
    { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14 };
    static constexpr std::array<uint8_t, 0x10> hi_lookup =
    { 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

    if (v & 0x0010)
    {
        return hi_lookup[v & 0x000F];
    }
    return low_lookup[v & 0x000F];
}

void NesAPU::step(uint64_t clock_ticks) // master clock ticks -> 21477272 @ 21.477272 MHz
{
    cycles_++;

    // mode 0:    mode 1:       function
    // ---------  -----------  -----------------------------
    //  - - - f    - - - - -    IRQ (if bit 6 is clear)
    //  - l - l    - l - - l    Length counter and sweep
    //  e e e e    e e e - e    Envelope and linear counter

    if (clock_ticks / cycles_per_step_ > frame_steps_)
    {
        frame_steps_++;

        int32_t current_frame_step = frame_steps_ % steps_per_frame_;

        // Decrement counters
        if (current_frame_step == 1 ||
            (current_frame_step == 3 && steps_per_frame_ == 4) ||
            (current_frame_step == 4 && steps_per_frame_ == 5))
        {
            // update length, sweep
            if (!get_length_counter_halt(registers_[PULSE1_REG1]))
            {
                player_.decrement_counter(Audio::Channel::Square_Pulse_1);
            }
            if (!get_length_counter_halt(registers_[PULSE2_REG1]))
            {
                player_.decrement_counter(Audio::Channel::Square_Pulse_2);
            }
            if (!get_linear_counter_halt(registers_[TRIANGLE_REG1]))
            {
                player_.decrement_counter(Audio::Channel::Triangle);
            }
            player_.decrement_counter(Audio::Channel::Noise);

            if (get_sweep_enabled(registers_[PULSE1_REG2]))
            {
                player_.step_sweep(Audio::Channel::Square_Pulse_1);
            }
            if (get_sweep_enabled(registers_[PULSE2_REG2]))
            {
                player_.step_sweep(Audio::Channel::Square_Pulse_2);
            }
        }

        // Decrement volume envelopes
        if (steps_per_frame_ == 4 || (steps_per_frame_ == 5 && current_frame_step != 3))
        {
            if (!get_linear_counter_halt(registers_[TRIANGLE_REG1]))
            {
                player_.decrement_linear_counter();
            }
            // if (!get_constant_volume(registers_[PULSE1_REG1]))
            {
                player_.decrement_volume_envelope(Audio::Channel::Square_Pulse_1);
            }
            // if (!get_constant_volume(registers_[PULSE2_REG1]))
            {
                player_.decrement_volume_envelope(Audio::Channel::Square_Pulse_2);
            }
        }

        // handle register updates
        // Square_Pulse_1
        if (registers_.had_write(PULSE1_REG1))
        {
            params_[to_index(Audio::Channel::Square_Pulse_1)].loop = get_loop(registers_[PULSE1_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_1)].duty_cycle = get_duty(registers_[PULSE1_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_1)].constant_volume = get_constant_volume(registers_[PULSE1_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_1)].volume = get_volume_envelope(registers_[PULSE1_REG1]);
        }
        if (registers_.had_write(PULSE1_REG2))
        {
            params_[to_index(Audio::Channel::Square_Pulse_1)].sweep_enabled = get_sweep_enabled(registers_[PULSE1_REG2]);
            params_[to_index(Audio::Channel::Square_Pulse_1)].sweep_period = get_period(registers_[PULSE1_REG2]);
            params_[to_index(Audio::Channel::Square_Pulse_1)].sweep_negate = get_negate(registers_[PULSE1_REG2]);
            params_[to_index(Audio::Channel::Square_Pulse_1)].sweep_shift_count = get_shift(registers_[PULSE1_REG2]);
        }
        if (registers_.had_write(PULSE1_REG3))
        {
            params_[to_index(Audio::Channel::Square_Pulse_1)].frequency =
                                    get_square_pulse_frequency(registers_[PULSE1_REG3], registers_[PULSE1_REG4]);

            player_.update_parameters(Audio::Channel::Square_Pulse_1,
                                      params_[to_index(Audio::Channel::Square_Pulse_1)], false);
        }
        if (registers_.had_write(PULSE1_REG4))
        {
            params_[to_index(Audio::Channel::Square_Pulse_1)].frequency =
                                    get_square_pulse_frequency(registers_[PULSE1_REG3], registers_[PULSE1_REG4]);

            params_[to_index(Audio::Channel::Square_Pulse_1)].counter =
                                length_counter_lookup(get_length_counter(registers_[PULSE1_REG4]));

            player_.update_parameters(Audio::Channel::Square_Pulse_1,
                                      params_[to_index(Audio::Channel::Square_Pulse_1)], true);
        }

        // Square_Pulse_2
        if (registers_.had_write(PULSE2_REG1))
        {
            params_[to_index(Audio::Channel::Square_Pulse_2)].loop = get_loop(registers_[PULSE2_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].duty_cycle = get_duty(registers_[PULSE2_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].constant_volume = get_constant_volume(registers_[PULSE2_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].volume = get_volume_envelope(registers_[PULSE2_REG1]);
        }
        if (registers_.had_write(PULSE2_REG2))
        {
            params_[to_index(Audio::Channel::Square_Pulse_2)].sweep_enabled = get_sweep_enabled(registers_[PULSE2_REG2]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].sweep_period = get_period(registers_[PULSE2_REG2]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].sweep_negate = get_negate(registers_[PULSE2_REG2]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].sweep_shift_count = get_shift(registers_[PULSE2_REG2]);
        }
        if (registers_.had_write(PULSE2_REG3))
        {
            params_[to_index(Audio::Channel::Square_Pulse_2)].frequency =
                                    get_square_pulse_frequency(registers_[PULSE1_REG3], registers_[PULSE1_REG4]);

            player_.update_parameters(Audio::Channel::Square_Pulse_2,
                                      params_[to_index(Audio::Channel::Square_Pulse_2)], false);
        }
        if (registers_.had_write(PULSE2_REG4))
        {
            params_[to_index(Audio::Channel::Square_Pulse_2)].frequency =
                                    get_square_pulse_frequency(registers_[PULSE2_REG3], registers_[PULSE2_REG4]);

            params_[to_index(Audio::Channel::Square_Pulse_2)].counter =
                                length_counter_lookup(get_length_counter(registers_[PULSE2_REG4]));

            player_.update_parameters(Audio::Channel::Square_Pulse_2,
                                      params_[to_index(Audio::Channel::Square_Pulse_2)], true);
        }

        // Triangle
        if (registers_.had_write(TRIANGLE_REG1))
        {
            // linear counter loaded on write to reg4
        }
        if (registers_.had_write(TRIANGLE_REG2))
        {
            // unused
        }
        if (registers_.had_write(TRIANGLE_REG3))
        {
            params_[to_index(Audio::Channel::Triangle)].frequency =
                                    get_triangle_frequency(registers_[TRIANGLE_REG3], registers_[TRIANGLE_REG4]);

            player_.update_parameters(Audio::Channel::Triangle,
                                      params_[to_index(Audio::Channel::Triangle)], false);
        }
        if (registers_.had_write(TRIANGLE_REG4))
        {
            params_[to_index(Audio::Channel::Triangle)].frequency =
                                    get_triangle_frequency(registers_[TRIANGLE_REG3], registers_[TRIANGLE_REG4]);

            params_[to_index(Audio::Channel::Triangle)].counter =
                                length_counter_lookup(get_length_counter(registers_[TRIANGLE_REG4]));

            // TODO should take affect 1 cycle later
            if (!get_linear_counter_halt(registers_[TRIANGLE_REG1]))
            {
                params_[to_index(Audio::Channel::Triangle)].linear_counter_load =
                                        get_linear_counter_load(registers_[TRIANGLE_REG1]);
            }

            player_.update_parameters(Audio::Channel::Triangle,
                                      params_[to_index(Audio::Channel::Triangle)], true);
        }

        // Noise
        if (registers_.had_write(NOISE_REG1))
        {

        }
        if (registers_.had_write(NOISE_REG2))
        {

        }

        if (registers_.had_write(NOISE_REG3))
        {

        }

        if (registers_.had_write(NOISE_REG4))
        {
            // LOG(INFO) << "write NOISE reg 4";
        }

        // DMC (delta modulation channel)
        if (registers_.had_write(DMC_REG1))
        {

        }
        if (registers_.had_write(DMC_REG2))
        {

        }

        if (registers_.had_write(DMC_REG3))
        {

        }

        if (registers_.had_write(DMC_REG4))
        {
            // LOG(INFO) << "write DMC reg 4";
        }

        // Status register for channel enable/disable
        if (registers_.had_write(APU_STATUS))
        {
            player_.set_enabled(Audio::Channel::Square_Pulse_1, registers_[APU_STATUS] & APU_STATUS_PULSE1_ENABLE);
            player_.set_enabled(Audio::Channel::Square_Pulse_2, registers_[APU_STATUS] & APU_STATUS_PULSE2_ENABLE);
            player_.set_enabled(Audio::Channel::Triangle, registers_[APU_STATUS] & APU_STATUS_TRIANGLE_ENABLE);
        }

        if (registers_.had_write(APU_FRAME_COUNTER))
        {
            steps_per_frame_ = get_frame_counter_mode_steps(registers_[APU_FRAME_COUNTER]);
            cycles_per_step_ = get_cycles_per_frame_step(steps_per_frame_);
        }

        registers_.clear_write_flags();
    }

    // Push samples to output buffer
    if (clock_ticks / get_cycles_per_frame() > frame_count_)
    {
        frame_count_++;
        player_.step();
    }
}

void NesAPU::start()
{
    player_.start();
}

void NesAPU::stop()
{
    player_.stop();
}

uint8_t NesAPU::read_register(uint16_t a) const
{
    // assert(a == APU_STATUS || (a >= PULSE1_REG1 && a <= DMC_REG4));
    if (a == APU_STATUS)
    {
        return registers_[a];
    }
    return 0; // all APU registers are write only, except APU_STATUS. Should be open bus behavior
}

uint8_t& NesAPU::write_register(uint16_t a)
{
    assert(a == APU_STATUS || a == APU_FRAME_COUNTER || (a >= PULSE1_REG1 && a <= DMC_REG4));

    registers_.set_had_write(a);
    return registers_[a];
}

float NesAPU::get_duty(uint8_t r)
{
    switch ((r & 0xC0) >> 6)
    {
        case 0:
            return 0.125;
        case 1:
            return 0.25;
        case 2:
            return 0.5;
        case 3:
            return -0.25;
    }
    assert(false);
    return 0;
}

bool NesAPU::get_loop(uint8_t r)
{
    return r & 0x20;
}

bool NesAPU::get_constant_volume(uint8_t r)
{
    return r & 0x10;
}

int16_t NesAPU::get_volume_envelope(uint8_t r)
{
    return r & 0x0F;
}

bool NesAPU::get_sweep_enabled(uint8_t r)
{
    return r & 0x80;
}

bool NesAPU::get_period(uint8_t r)
{
    return (r & 0x70) >> 4;
}

bool NesAPU::get_negate(uint8_t r)
{
    return r & 0x08;
}

bool NesAPU::get_shift(uint8_t r)
{
    return r & 0x07;
}

int16_t NesAPU::get_square_pulse_frequency(uint8_t r1, uint8_t r2)
{
    static constexpr int32_t CLOCK_FREQ = 1789773;
    int32_t timer = ((r2 & 0x03) << 8) | r1;

    if (timer < 8)
    {
        return 0; // channel should be silenced
    }

    return CLOCK_FREQ / (16 * (timer + 1));
}

int16_t NesAPU::get_triangle_frequency(uint8_t r1, uint8_t r2)
{
    static constexpr int32_t CLOCK_FREQ = 1789773;
    int32_t timer = ((r2 & 0x03) << 8) | r1;

    return CLOCK_FREQ / (32 * (timer + 1));
}

int16_t NesAPU::get_length_counter(uint8_t r)
{
    return r >> 3;
}

bool NesAPU::get_length_counter_halt(uint8_t r)
{
    return r & 0x20;
}

uint16_t NesAPU::get_frame_counter_mode_steps(uint8_t r)
{
    if (r & 0x80)
    {
        return 4;
    }
    return 5;
}

int32_t NesAPU::get_linear_counter_load(uint8_t r)
{
    return r & 0x7F;
}

bool NesAPU::get_linear_counter_halt(uint8_t r)
{
    return r & 0x80;
}
