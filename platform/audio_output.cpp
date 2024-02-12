#include "platform/audio_output.hpp"

#include <glog/logging.h>

#include <algorithm>

// sample generators for square, sin, triangle waves
std::vector<uint8_t> square_wav(const QAudioFormat &format, qint64 duration_us, int frequency)
{
    const int channel_bytes = format.bytesPerSample();
    const int sample_bytes = format.channelCount() * channel_bytes;
    qint64 length = format.bytesForDuration(duration_us);

    std::vector<uint8_t> buffer;
    buffer.resize(length);

    uint8_t *ptr = reinterpret_cast<uint8_t *>(buffer.data());
    int32_t sample_index = 0;

    while (length)
    {
        const int16_t max_volume = static_cast<int16_t>(std::numeric_limits<int16_t>::max() * 0.8);

        double x = std::sin(2 * M_PI * frequency * double(sample_index++ % format.sampleRate())
                                 / format.sampleRate());
        if (x < 0)
        {
            x = -max_volume;
        }
        else if (x > 0)
        {
            x = max_volume;
        }

        *reinterpret_cast<int16_t *>(ptr) = x;

        ptr += channel_bytes;
        length -= channel_bytes;
    }
    return buffer;
}

std::vector<uint8_t> sin_wav(const QAudioFormat &format, qint64 duration_us, int frequency)
{
    const int channel_bytes = format.bytesPerSample();
    const int sample_bytes = format.channelCount() * channel_bytes;
    qint64 length = format.bytesForDuration(duration_us);

    std::vector<uint8_t> buffer;
    buffer.resize(length);

    uint8_t *ptr = reinterpret_cast<uint8_t *>(buffer.data());
    int32_t sample_index = 0;

    while (length)
    {
        const int16_t max_volume = static_cast<int16_t>(std::numeric_limits<int16_t>::max() * 0.8);

        double x = std::sin(2 * M_PI * frequency * double(sample_index++ % format.sampleRate())
                                 / format.sampleRate());

        *reinterpret_cast<qint16 *>(ptr) = static_cast<qint16>(x * max_volume);

        ptr += channel_bytes;
        length -= channel_bytes;
    }
    return buffer;
}

std::vector<uint8_t> triangle_wav(const QAudioFormat &format, qint64 duration_us, int frequency)
{
    int32_t max_triangle_steps = 64;
    int32_t samples_per_triangle_step = 1000000 / frequency * 2 / max_triangle_steps;
    int32_t samples_per_period = max_triangle_steps * samples_per_triangle_step;
    int32_t triangle_step = max_triangle_steps - 1;

    const int channel_bytes = format.bytesPerSample();
    const int sample_bytes = format.channelCount() * channel_bytes;
    qint64 length = format.bytesForDuration(duration_us);

    // make sure the buffer length exactly matches the triangle period so
    // looping is seamless
    length -= length % (samples_per_period * sample_bytes);

    std::vector<uint8_t> buffer;
    buffer.resize(length);

    uint8_t *ptr = reinterpret_cast<uint8_t *>(buffer.data());
    int32_t sample_index = 0;

    int32_t last_log = 999;

    while (length)
    {
        const int16_t max_volume = static_cast<int16_t>(std::numeric_limits<int16_t>::max() * 0.8);

        int32_t triangle_step = (sample_index++ / samples_per_triangle_step) % max_triangle_steps;
        if (triangle_step >= max_triangle_steps / 2)
        {
            triangle_step = max_triangle_steps / 2 - (triangle_step - max_triangle_steps / 2);
        }

        double x = -1 + 2 * (static_cast<double>(triangle_step) / (max_triangle_steps / 2));
        int16_t sample = x * max_volume;

        *reinterpret_cast<qint16 *>(ptr) = static_cast<qint16>(sample);
        ptr += channel_bytes;
        length -= channel_bytes;
    }
    return buffer;
}

Generator::Generator(const QAudioFormat &format)
{
    assert(QAudioFormat::Int16 == format.sampleFormat());

    if (format.isValid())
    {
        streams_.push_back(AudioStream()); // Square_Pulse_1
        streams_.push_back(AudioStream()); // Square_Pulse_2
        streams_.push_back(AudioStream()); // Triangle
        streams_.push_back(AudioStream()); // Noise
        streams_.push_back(AudioStream()); // Recorded_Sample
    }
}

void Generator::start()
{
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    close();
}

qint64 Generator::readData(char *data, qint64 max_len)
{
    // Mix the streams together to produce single set of output samples

    // This function is being called from a QT audio thread. Take the generator lock to make sure
    // the streams_ buffers are not manipulated while reading.
    std::scoped_lock lock(lock_);

    static constexpr int32_t SAMPLE_SIZE_BYTES = 2;
    qint64 total = 0;

    while (max_len - total > SAMPLE_SIZE_BYTES)
    {
        // Linear approximation of APU mixer
        // https://www.nesdev.org/wiki/APU_Mixer

        int32_t mixed_square_pulses = 0;
        int32_t mixed_tnd = 0; // triangle, noise, DMC
        int32_t mixed_sample = 0;

        mixed_square_pulses += streams_[to_index(Audio::Channel::Square_Pulse_1)].read_sample();
        mixed_square_pulses += streams_[to_index(Audio::Channel::Square_Pulse_2)].read_sample();
        mixed_square_pulses *= 0.0752; // adjusted these x10, same ratio but so they do not mute the audio

        mixed_tnd += 0.0851 * streams_[to_index(Audio::Channel::Triangle)].read_sample();
        mixed_tnd += 0.0494 * streams_[to_index(Audio::Channel::Noise)].read_sample();
        mixed_tnd += 0.0335 * streams_[to_index(Audio::Channel::Recorded_Sample)].read_sample();

        mixed_sample = mixed_square_pulses + mixed_tnd;

        if (mixed_sample > std::numeric_limits<int16_t>::max())
        {
            mixed_sample = std::numeric_limits<int16_t>::max();
        }
        if (mixed_sample < std::numeric_limits<int16_t>::min())
        {
            mixed_sample = std::numeric_limits<int16_t>::min();
        }

        int16_t output_sample = static_cast<int16_t>(mixed_sample);

        memcpy(data + total, &output_sample, SAMPLE_SIZE_BYTES);

        total += SAMPLE_SIZE_BYTES;
    }

    return total;
}

qint64 Generator::bytesAvailable() const
{
    return size() + QIODevice::bytesAvailable();
}

void Generator::update_parameters(Audio::Channel channel, Audio::Parameters params, std::vector<uint8_t> buffer)
{
    std::scoped_lock lock(lock_);
    streams_[to_index(channel)].reload(params, buffer);
}

AudioStream::AudioStream()
{
    // enough samples to feed zeros
    buffer_.push_back(0x00);
    buffer_.push_back(0x00);

    assert(buffer_.size() >= 2);
}

AudioStream::AudioStream(AudioStream&& other)
 : counter_(other.counter_)
 , volume_(other.volume_)
 , pos_(other.pos_)
 , buffer_(std::move(other.buffer_))
{
    enabled_ = other.enabled_.load();
}

int16_t AudioStream::read_sample()
{
    if (!enabled_)
    {
        return 0;
    }

    int16_t result = *reinterpret_cast<int16_t*>(&buffer_[pos_]);
    pos_ = (pos_ + 2) % buffer_.size();

    return int16_t(result * (volume_ / 15.0));
}

void AudioStream::set_enabled(bool enabled)
{
    enabled_ = enabled;

    if (!enabled_)
    {
        counter_ = 0;
    }
}

void AudioStream::decrement_volume_envelope()
{
    if (!volume_)
    {
        return;
    }
    volume_--;
}

void AudioStream::decrement_counter()
{
    if (!counter_)
    {
        return;
    }
    counter_--;

    if (!counter_)
    {
        enabled_ = false;
    }
}

void AudioStream::reload(Audio::Parameters params, std::vector<uint8_t> buffer)
{
    assert(buffer.size() >= 2);

    buffer_ = buffer;
    pos_ = 0;
    counter_ = params.counter;
    volume_ = params.volume;
    enabled_ = true;
}
