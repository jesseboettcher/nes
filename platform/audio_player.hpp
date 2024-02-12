#pragma once

#include "platform/audio_output.hpp"

#include <QAudioFormat>
#include <QAudioSink>

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

    void update_parameters(Audio::Channel channel, Audio::Parameters params);
    void set_enabled(Audio::Channel channel, bool enabled);
    void decrement_counter(Audio::Channel channel);

    void test();

private:
    std::shared_ptr<Generator> generator_;
    std::shared_ptr<QAudioSink> audio_sink_;
};
