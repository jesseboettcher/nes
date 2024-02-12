#pragma once

#include "platform/audio_types.hpp"
#include "lib/magic_enum.hpp"

#include <QAudioDevice>
#include <QIODevice>

#include <fstream>
#include <mutex>

// sample generators for square, sin, triangle waves
std::vector<uint8_t> square_wav(const QAudioFormat &format, qint64 duration_us, int frequency);
std::vector<uint8_t> sin_wav(const QAudioFormat &format, qint64 duration_us, int frequency);
std::vector<uint8_t> triangle_wav(const QAudioFormat &format, qint64 duration_us, int frequency);

// AudioStream holds the buffer and state representing a single audio stream. It must always
// return samples, so provides zeros if there is no audio data.
//
class AudioStream
{
public:
    AudioStream(Audio::Channel channel);
    AudioStream(AudioStream&& other);

    int16_t read_sample();
    qint64 size() const { return buffer_.size(); }

    bool enabled() { return enabled_; }
    void set_enabled(bool enabled);
    void decrement_counter();
    void decrement_volume_envelope();

    void reload(Audio::Parameters params, std::vector<uint8_t> buffer);

    int32_t volume() { return volume_; }

private:
    Audio::Channel channel_;
    int32_t counter_;    // countdown ticks
    int32_t volume_{15}; // 0-15
    int32_t volume_offset_; // for changing envelope
    bool constant_volume_;

    int64_t pos_ = 0;
    std::vector<uint8_t> buffer_;
    std::atomic<bool> enabled_{false};

    std::ofstream log_;
};

// Generator does the work of pushing sound samples out through QT's audio system.
// Once it is started it the audio system will constantly pull data from Generator using
// readData(). Generator will feed it zeros until a rom is running and any of the 5 NES
// audio streams have been activated (via update_parameters()). Then it manages the mixing of
// those channels to provide some sample stream to the speakers.
//
class Generator : public QIODevice
{
public:
    Generator(const QAudioFormat &format);

    void start();
    void stop();

    // Polled from a QT audio thread for new samples. Grabs samples from all streams
    // and mixes them to a single output
    qint64 readData(char *data, qint64 max_len) override;

    // unused
    qint64 writeData(const char *data, qint64 len) override { return 0; }

    // Data is aways available. AudioStreams will feed zeros if there is no actual audio
    qint64 bytesAvailable() const override;
    qint64 size() const override { return streams_[0].size(); }

    void set_enabled(Audio::Channel channel, bool enabled)
    {
        streams_[to_index(channel)].set_enabled(enabled);
    }

    // Called on APU frame steps. Stream will self-disable when it reaches zero
    void decrement_counter(Audio::Channel channel) { streams_[magic_enum::enum_integer<Audio::Channel>(channel)].decrement_counter(); }

    // Called on APU frame steps
    void decrement_volume_envelope(Audio::Channel channel) { streams_[magic_enum::enum_integer<Audio::Channel>(channel)].decrement_volume_envelope(); }

    // Updates stream to a new configuration (e.g. frequency, volume envelope, counter, etc)
    void update_parameters(Audio::Channel channel, Audio::Parameters params, std::vector<uint8_t> buffer);

private:
    int32_t to_index(Audio::Channel channel) { return magic_enum::enum_integer<Audio::Channel>(channel); }

    // For synchronization of stream buffer reading & changes
    std::mutex lock_;
    std::vector<AudioStream> streams_;
};
