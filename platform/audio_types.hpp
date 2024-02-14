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
    Channel channel;
    int32_t counter{0}; // countdown ticks
    int32_t frequency{1};
    int32_t volume{0};
    bool constant_volume{false};
    float duty_cycle; // square waves only
    bool loop{false};

    bool sweep_enabled{false};
    int32_t sweep_period{0};
    bool sweep_negate{false};
    int32_t sweep_shift_count{0};
};

}
