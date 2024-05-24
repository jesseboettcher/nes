#pragma once

#include "platform/audio_types.hpp"
#include "lib/magic_enum.hpp"
#include "lib/utils.hpp"

#include <QAudioDevice>
#include <QIODevice>

#include <fstream>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>
#include <vector>

// sample generators for square, sin, triangle waves
std::vector<uint8_t> square_wav(const QAudioFormat &format, qint64 duration_us, int frequency);
std::vector<uint8_t> sin_wav(const QAudioFormat &format, qint64 duration_us, int frequency);
std::vector<uint8_t> triangle_wav(const QAudioFormat &format, qint64 duration_us, int frequency);

// forward defs
class Waveform;

// AudioStream holds the buffer and state representing a single audio stream. It must always
// return samples, so provides zeros if there is no audio data.
//
class AudioStream
{
public:
    AudioStream(Audio::Channel channel);
    AudioStream(AudioStream&& other);

    int16_t read_sample();
    qint64 size() const { return 1024; } // TODO review

    bool enabled() { return enabled_; }
    void set_enabled(bool enabled);
    void decrement_counter();
    void decrement_linear_counter();
    void decrement_volume_envelope();
    void step_sweep();

    void reload(Audio::Parameters params, bool reset_phase);

    int32_t volume() { return volume_; }

    void set_waveform(std::shared_ptr<Waveform> waveform) { waveform_ = waveform; }

private:
    Audio::Channel channel_;

    int32_t counter_;
    int32_t linear_counter_{0};

    int32_t volume_{15}; // 0-15
    int32_t volume_decay_rate_; // for changing envelope
    uint64_t volume_envelope_ticks_{0};
    bool volume_loop_{false};
    bool constant_volume_{true};

    bool sweep_enabled_{false};
    int32_t sweep_period_{0};
    bool sweep_negate_{false};
    int32_t sweep_shift_count_{0};
    uint64_t sweep_ticks_{0};

    int64_t pos_ = 0;
    std::shared_ptr<Waveform> waveform_;
    bool enabled_{false};

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

    enum class Event
    {
        Step,
        Parameter_Update,
        Parameter_Update_Reset_Phase,
        Decrement_Counter,
        Decrement_Linear_Counter,
        Decrement_Volume,
        Step_Sweep
    };
    using EventChannel = std::pair<Event, Audio::Channel>;

    Generator(const QAudioFormat &format);

    // open and close the QT audio output device
    void start();
    void stop();

    // indicates one APU frame (60hz)
    void step();

    // producer loop runs on a separate thread and handles the events_ queue
    void producer_loop();

    // retrieves samples from each stream, mixes them, and pushes them to the output buffer
    void produce_samples();

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
    void decrement_counter(Audio::Channel channel);

    // Called on APU frame steps. Stream will self-disable when it reaches zero
    void decrement_linear_counter();

    // Called on APU frame steps
    void decrement_volume_envelope(Audio::Channel channel);

    // Stepped at 2x frame frequency
    void step_sweep(Audio::Channel channel);

    // Updates stream to a new configuration (e.g. frequency, volume envelope, counter, etc)
    void update_parameters(Audio::Channel channel, Audio::Parameters params, bool reset_phase);

private:
    inline int32_t to_index(Audio::Channel channel) { return magic_enum::enum_integer<Audio::Channel>(channel); }

    // For synchronization of stream buffer reading & changes
    std::mutex streams_lock_;
    std::vector<AudioStream> streams_;

    std::shared_ptr<std::thread> producer_;

    // All actions on a stream are managed through this queue to make sure they happen in the proper
    // order. The step events come in at 60hz and the queue ensures that the right number of samples
    // are produced before actions like changing the frequency, volume, or disabling of the channel
    // are taken.
    std::mutex queues_lock_;
    std::queue<EventChannel> events_;
    std::queue<Audio::Parameters> param_updates_;
    std::counting_semaphore<1> sema_;

    std::mutex output_lock_;
    int32_t samples_per_step_; // 60hz
    CircularBuffer<uint16_t> output_buffer_;

    bool shutdown_;
};
