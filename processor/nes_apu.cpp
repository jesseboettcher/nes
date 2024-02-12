#include "processor/nes_apu.hpp"

#include "platform/audio_player.hpp"

#include <glog/logging.h>

NesAPU::NesAPU()
{
	LOG(INFO) << "NesAPU created";
}

void NesAPU::reset()
{
    registers_.clear();
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

void NesAPU::step(uint64_t clock_ticks)
{
    // mode 0:    mode 1:       function
    // ---------  -----------  -----------------------------
    //  - - - f    - - - - -    IRQ (if bit 6 is clear)
    //  - l - l    - l - - l    Length counter and sweep
    //  e e e e    e e e - e    Envelope and linear counter

    int32_t steps_per_frame = get_frame_counter_mode_steps(registers_[APU_FRAME_COUNTER]);

    if (clock_ticks % (60 * steps_per_frame) == 0)
    {
        frame_counter_steps_++;

        int32_t local_step = frame_counter_steps_ % steps_per_frame;

        // Decrement counters
        if (local_step == 1 ||
            (local_step == 3 && steps_per_frame == 4) ||
            (local_step == 4 && steps_per_frame == 5))
        {
            // update length, sweep
            // TODO only if active
            player_.decrement_counter(Audio::Channel::Square_Pulse_1);
            player_.decrement_counter(Audio::Channel::Square_Pulse_2);
            player_.decrement_counter(Audio::Channel::Triangle);
            player_.decrement_counter(Audio::Channel::Noise);
        }

        // Decrement volume envelopes
        if (steps_per_frame == 4 || (steps_per_frame == 5 && local_step != 3))
        {
            if (!get_constant_volume(registers_[PULSE1_REG1]))
            {
                player_.decrement_volume_envelope(Audio::Channel::Square_Pulse_1);
            }
            if (!get_constant_volume(registers_[PULSE2_REG1]))
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
            params_[to_index(Audio::Channel::Square_Pulse_1)].volume = get_volume_envelope(registers_[PULSE1_REG1]);
        }
        if (registers_.had_write(PULSE1_REG2))
        {
            // todo sweep unit
        }
        if (registers_.had_write(PULSE1_REG3))
        {
            // counter low 8 bits, updated when the hi 3 bits are written via register 4
        }
        if (registers_.had_write(PULSE1_REG4))
        {
            params_[to_index(Audio::Channel::Square_Pulse_1)].frequency =
                                    get_frequency(registers_[PULSE1_REG3], registers_[PULSE1_REG4]);

            params_[to_index(Audio::Channel::Square_Pulse_1)].counter =
                                length_counter_lookup(get_length_counter(registers_[PULSE1_REG4]));

            player_.update_parameters(Audio::Channel::Square_Pulse_1,
                                      params_[to_index(Audio::Channel::Square_Pulse_1)]);
        }

        // Square_Pulse_2
        if (registers_.had_write(PULSE2_REG1))
        {
            params_[to_index(Audio::Channel::Square_Pulse_2)].loop = get_loop(registers_[PULSE2_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].duty_cycle = get_duty(registers_[PULSE2_REG1]);
            params_[to_index(Audio::Channel::Square_Pulse_2)].volume = get_volume_envelope(registers_[PULSE2_REG1]);
        }
        if (registers_.had_write(PULSE2_REG2))
        {
            // todo sweep unit
        }
        if (registers_.had_write(PULSE2_REG3))
        {
            // counter low 8 bits, updated when the hi 3 bits are written via register 4
        }
        if (registers_.had_write(PULSE2_REG4))
        {
            params_[to_index(Audio::Channel::Square_Pulse_2)].frequency =
                                    get_frequency(registers_[PULSE2_REG3], registers_[PULSE2_REG4]);

            params_[to_index(Audio::Channel::Square_Pulse_2)].counter =
                                length_counter_lookup(get_length_counter(registers_[PULSE2_REG4]));

            player_.update_parameters(Audio::Channel::Square_Pulse_2,
                                      params_[to_index(Audio::Channel::Square_Pulse_2)]);
        }

        if (registers_.had_write(APU_STATUS))
        {
            player_.set_enabled(Audio::Channel::Square_Pulse_1, registers_[APU_STATUS] & APU_STATUS_PULSE1_ENABLE);
            player_.set_enabled(Audio::Channel::Square_Pulse_2, registers_[APU_STATUS] & APU_STATUS_PULSE2_ENABLE);
        }

        registers_.clear_write_flags();
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
        case 3:
            return 0.5;
        case 4:
            return -0.25;
    }
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
    return (r & 0x08) >> 3;
}

bool NesAPU::get_shift(uint8_t r)
{
    return r & 0x07;
}

int16_t NesAPU::get_frequency(uint8_t r1, uint8_t r2)
{
    static constexpr int32_t CLOCK_FREQ = 1789773;
    int32_t timer = ((r2 & 0x03) << 8) | r1;

    return CLOCK_FREQ / (16 * (timer + 1));
}

int16_t NesAPU::get_length_counter(uint8_t r)
{
    return r >> 3;
}

uint16_t NesAPU::get_frame_counter_mode_steps(uint8_t r)
{
    if (r & 0x80)
    {
        return 4;
    }
    return 5;
}
