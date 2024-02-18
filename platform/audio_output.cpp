#include "platform/audio_output.hpp"

#include "config/flags.hpp"

#include <glog/logging.h>
#include <QAudioDevice>
#include <QMediaDevices>

#include <algorithm>

static constexpr bool VERBOSE = false;

class Waveform
{
public:
    virtual void set_frequency(int32_t frequency, float duty, bool reset_phase) { }
    virtual int16_t read_sample() { return 0; }
};

class SquareWave : public Waveform
{
public:
    SquareWave(const QAudioFormat &format, int32_t frequency, float duty)
     : format_(format)
    {
        max_value_ = static_cast<int16_t>(std::numeric_limits<int16_t>::max() * 1);

        set_frequency(frequency, duty, true);
    }

    void set_frequency(int32_t frequency, float duty, bool reset_phase) override
    {
        if (reset_phase)
        {
            sample_number_ = 0;
        }
        frequency_ = frequency;
        duty_ = duty;
        sample_number_ = 0;

        int32_t usec_per_period = 1000000 / (frequency_ * 2);

        samples_per_period_ = format_.bytesForDuration(usec_per_period) / format_.bytesPerSample();

        samples_hi_per_period_ = samples_per_period_ * std::abs(duty_);
    }

    int16_t read_sample() override
    {
        int32_t sample = max_value_;

        if ((sample_number_++ % samples_per_period_) > samples_hi_per_period_)
        {
            sample *= -1;
        }

        if (duty_ < 0)
        {
            sample *= -1;
        }

        return sample;
    }

private:
    QAudioFormat format_;

    int16_t max_value_{0};

    int64_t sample_number_{0};
    int32_t frequency_{0};
    int32_t samples_per_period_{0};
    int32_t samples_hi_per_period_{0};
    float duty_{0.0};
};

class TriangleWave : public Waveform
{
public:
    static constexpr int32_t MAX_TRIANGLE_STEPS = 64;

    TriangleWave(const QAudioFormat &format, int32_t frequency)
     : format_(format)
    {
        max_value_ = static_cast<int16_t>(std::numeric_limits<int16_t>::max() * .8);

        set_frequency(frequency, 0, false);
    }

    void set_frequency(int32_t frequency, float, bool) override
    {
        frequency_ = frequency;

        int32_t usec_per_period = 1000000 / (frequency_ * 2);
        samples_per_period_ = format_.bytesForDuration(usec_per_period) / format_.bytesPerSample();

        samples_per_triangle_step_ = samples_per_period_ / MAX_TRIANGLE_STEPS;
    }

    int16_t read_sample() override
    {
        int32_t current_triangle_step = (sample_number_++ / samples_per_triangle_step_) % MAX_TRIANGLE_STEPS;
        if (current_triangle_step >= MAX_TRIANGLE_STEPS / 2)
        {
            current_triangle_step = MAX_TRIANGLE_STEPS / 2 - (current_triangle_step - MAX_TRIANGLE_STEPS / 2);
        }

        double normalized_sample = -1 + 2 * (static_cast<double>(current_triangle_step) / (MAX_TRIANGLE_STEPS / 2));
        int16_t result_sample = normalized_sample * max_value_;

        return result_sample;
    }

private:
    QAudioFormat format_;

    int16_t max_value_{0};

    int64_t sample_number_{0};
    int32_t frequency_{-1};
    int32_t samples_per_period_{0};
    int32_t samples_per_triangle_step_;
};

Generator::Generator(const QAudioFormat &format)
 : sema_(0)
 , output_buffer_(1024*32)
{
    assert(QAudioFormat::Int16 == format.sampleFormat());

    samples_per_step_ = format.bytesForDuration(16666) * format.bytesPerSample(); // stepped at 60hz, 16.6ms

    if (format.isValid())
    {
        streams_.push_back(AudioStream(Audio::Channel::Square_Pulse_1));
        streams_.push_back(AudioStream(Audio::Channel::Square_Pulse_2));
        streams_.push_back(AudioStream(Audio::Channel::Triangle));
        streams_.push_back(AudioStream(Audio::Channel::Noise));
        streams_.push_back(AudioStream(Audio::Channel::Recorded_Sample));
    }

    // populate each stream with a valid waveform. will not produce actual audio samples until enabled
    streams_[to_index(Audio::Channel::Square_Pulse_1)].set_waveform(std::make_shared<SquareWave>(format, 440, 0.5));
    streams_[to_index(Audio::Channel::Square_Pulse_2)].set_waveform(std::make_shared<SquareWave>(format, 440, 0.5));
    streams_[to_index(Audio::Channel::Triangle)].set_waveform(std::make_shared<TriangleWave>(format, 440));

    producer_ = std::make_shared<std::thread>(&Generator::producer_loop, this);
}

void Generator::start()
{
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    close();
}

void Generator::producer_loop()
{
    while (true)
    {
        bool success = sema_.try_acquire_for(std::chrono::milliseconds(1000));

        if (success)
        {
            EventChannel event;
            {
                std::scoped_lock lock(queues_lock_);

                if (events_.empty())
                {
                    continue;
                }

                event = events_.front();
                events_.pop();
            }

            switch (event.first)
            {
                case Event::Step:
                {
                    produce_samples();
                }
                break;

                case Event::Parameter_Update:
                case Event::Parameter_Update_Reset_Phase:
                {
                    std::scoped_lock lock(streams_lock_);

                    Audio::Parameters params = param_updates_.front();
                    param_updates_.pop();

                    QAudioDevice device = QMediaDevices::defaultAudioOutput();
                    QAudioFormat format = device.preferredFormat();

                    if (params.channel == Audio::Channel::Square_Pulse_1 ||
                        params.channel == Audio::Channel::Square_Pulse_2)
                    {
                        streams_[to_index(params.channel)].reload(params, event.first == Event::Parameter_Update_Reset_Phase);
                    }
                    else if (params.channel == Audio::Channel::Triangle)
                    {
                        streams_[to_index(params.channel)].reload(params, false);
                    }
                    break;
                }

                case Event::Decrement_Counter:
                {
                    std::scoped_lock lock(streams_lock_);
                    streams_[to_index(event.second)].decrement_counter();
                    break;
                }

                case Event::Decrement_Volume:
                {
                    std::scoped_lock lock(streams_lock_);
                    streams_[to_index(event.second)].decrement_volume_envelope();
                    break;
                }
            }
        }
    }
}

void Generator::produce_samples()
{
    int32_t samples_produced = 0;

    while (samples_produced++ < samples_per_step_)
    {
        // Linear approximation of APU mixer
        // https://www.nesdev.org/wiki/APU_Mixer

        int32_t mixed_square_pulses = 0;
        int32_t mixed_tnd = 0; // triangle, noise, DMC
        int32_t mixed_sample = 0;

        {
            std::scoped_lock lock(streams_lock_);
            mixed_square_pulses += streams_[to_index(Audio::Channel::Square_Pulse_1)].read_sample();
            mixed_square_pulses += streams_[to_index(Audio::Channel::Square_Pulse_2)].read_sample();
            mixed_square_pulses *= 0.00752;

            mixed_tnd += 0.00851 * streams_[to_index(Audio::Channel::Triangle)].read_sample();
            mixed_tnd += 0.00494 * streams_[to_index(Audio::Channel::Noise)].read_sample();
            mixed_tnd += 0.00335 * streams_[to_index(Audio::Channel::Recorded_Sample)].read_sample();
        }

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

        {
            std::scoped_lock lock(output_lock_);
            output_buffer_.push(output_sample);
        }
    }
}

void Generator::step()
{
    {
        std::scoped_lock lock(queues_lock_);
        events_.push(std::make_pair(Event::Step, Audio::Channel::Square_Pulse_1));
    }
    sema_.release();
}

qint64 Generator::readData(char *data, qint64 max_len)
{
    // This function is being called from a QT audio thread. Take the generator lock to make sure
    // the streams_ buffers are not manipulated while reading.
    std::scoped_lock lock(output_lock_);

    static constexpr int32_t SAMPLE_SIZE_BYTES = 2;
    qint64 total = 0;

    while (max_len - total > SAMPLE_SIZE_BYTES && !output_buffer_.is_empty())
    {
        int16_t sample = output_buffer_.pop();
        memcpy(data + total, &sample, SAMPLE_SIZE_BYTES);

        total += SAMPLE_SIZE_BYTES;
    }

    return total;
}

qint64 Generator::bytesAvailable() const
{
    return size() + QIODevice::bytesAvailable();
}

void Generator::decrement_counter(Audio::Channel channel)
{
    std::scoped_lock lock(queues_lock_);
    events_.push(std::make_pair(Event::Decrement_Counter, channel));

    sema_.release();
}

void Generator::decrement_volume_envelope(Audio::Channel channel)
{
    std::scoped_lock lock(queues_lock_);
    events_.push(std::make_pair(Event::Decrement_Volume, channel));

    sema_.release();
}

void Generator::update_parameters(Audio::Channel channel, Audio::Parameters params, bool reset_phase)
{
    std::scoped_lock lock(queues_lock_);

    if (reset_phase)
    {
        events_.push(std::make_pair(Event::Parameter_Update_Reset_Phase, channel));
    }
    else
    {
        events_.push(std::make_pair(Event::Parameter_Update, channel));
    }
    param_updates_.push(params);

    sema_.release();
}

AudioStream::AudioStream(Audio::Channel channel)
 : channel_(channel)
{
    if constexpr (ENABLE_APU_LOGGING)
    {
        std::string path = std::string("/tmp/") +
                           std::string(magic_enum::enum_name<Audio::Channel>(channel_)) +
                           std::string(".log");
        std::filesystem::remove(path);
        log_.open(path);
        if (!log_.is_open())
        {
            LOG(WARNING) << "Could not write " << path;
        }
    }
    waveform_ = std::make_shared<Waveform>();
}

AudioStream::AudioStream(AudioStream&& other)
 : channel_(other.channel_)
 , counter_(other.counter_)
 , volume_(other.volume_)
 , volume_offset_(other.volume_offset_)
 , constant_volume_(other.constant_volume_)
 , pos_(other.pos_)
 , waveform_(other.waveform_)
 , log_(std::move(other.log_))
{
    enabled_ = other.enabled_;
}

int16_t AudioStream::read_sample()
{
    if (!enabled_)
    {
        return 0;
    }

    // int16_t result = *reinterpret_cast<int16_t*>(&buffer_[pos_]);
    // pos_ = (pos_ + 2) % buffer_.size();
    int16_t result = waveform_->read_sample();

    int16_t volume_adjusted = int16_t(result * (volume_ / 15.0));

    if constexpr (ENABLE_APU_LOGGING)
    {
        log_ << volume_adjusted << std::endl;
    }
    return volume_adjusted;
}

void AudioStream::set_enabled(bool enabled)
{
    enabled_ = enabled;

    if (VERBOSE)
    {
        LOG(INFO) << magic_enum::enum_name<Audio::Channel>(channel_) << " set_enabled " << +enabled << " counter " << counter_ << " vol " << volume_;
    }

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
    volume_ -= volume_offset_;

    if (volume_ < 0)
    {
        volume_ = 0;
    }

    if (volume_ == 0)
    {
        set_enabled(false);
    }
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
        set_enabled(false);
    }
}

void AudioStream::reload(Audio::Parameters params, bool reset_phase)
{
    waveform_->set_frequency(params.frequency, params.duty_cycle, reset_phase);
    pos_ = 0;
    counter_ = params.counter;

    // params.volume contains the volume, if constant, otherwise the offset for the envelope
    constant_volume_ = params.constant_volume;
    volume_ = constant_volume_ ? params.volume : 15;
    volume_offset_ = constant_volume_ ? 0 : params.volume;

    set_enabled(true);
}
