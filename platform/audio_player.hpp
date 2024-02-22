#pragma once

#include "platform/audio_output.hpp"

#include <QAudioFormat>
#include <QAudioSink>

#include <fstream>

// AudioPlayer is a lightweight, clean interface between the APU and the details of audio
// stream management, mixing, and output.
//
class AudioPlayer
{
public:
    AudioPlayer();

    // Needs to be called from the QT UI thread. Easiest to call once at startup and let run.
    void start();
    void stop();
    void reset();

    // Grab latest samples and queue them into the output buffer for the audio system to pull from
    void step();

    void update_parameters(Audio::Channel channel, Audio::Parameters params, bool reset_phase);
    void set_enabled(Audio::Channel channel, bool enabled);

    void decrement_counter(Audio::Channel channel);
    void decrement_linear_counter(); // Channel::Triangle only
    void decrement_volume_envelope(Audio::Channel channel);

    void step_sweep(Audio::Channel channel);

    void test();

private:
    std::shared_ptr<Generator> generator_;
    std::shared_ptr<QAudioSink> audio_sink_;

    uint64_t audio_player_cycles_{0};

    std::ofstream log_;
};
