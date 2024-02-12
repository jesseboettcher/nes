#pragma once

#include <cstdint>

namespace Audio
{
	
enum class Channel
{
    Square_Pulse_1,
    Square_Pulse_2,
    Triangle,
    Noise,
    Recorded_Sample
};

struct Parameters
{
    int32_t counter; // countdown ticks
    int32_t frequency;
    int32_t volume;
    bool constant_volume;
    float duty_cycle; // square waves only
    bool loop;
    // todo envelope, volume, sweep
};

}
