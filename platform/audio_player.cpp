#include "platform/audio_player.hpp"

#include "config/flags.hpp"
#include "lib/magic_enum.hpp"

#include <glog/logging.h>
#include <QAudioDevice>
#include <QMediaDevices>

AudioPlayer::AudioPlayer()
{
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    QAudioFormat format = device.preferredFormat();

    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    generator_ = std::make_shared<Generator>(format);   
    audio_sink_ = std::make_shared<QAudioSink>(device, format);

    if constexpr (ENABLE_APU_PARAMETERS_LOGGING)
    {
        std::string path = std::string("/tmp/") +
                           "nes_audio_parameters" +
                           std::string(".log");
        std::filesystem::remove(path);
        log_.open(path);
        if (!log_.is_open())
        {
            LOG(WARNING) << "Could not write " << path;
        }
    }
}

AudioPlayer::~AudioPlayer()
{
    generator_->stop();
}

void AudioPlayer::update_parameters(Audio::Channel channel, Audio::Parameters params, bool reset_phase)
{
    if constexpr (ENABLE_APU_PARAMETERS_LOGGING)
    {
        log_ << magic_enum::enum_name<Audio::Channel>(channel) << " "
             << "cycle " << audio_player_cycles_ << " "
             << "freq " << params.frequency << " "
             << "duty " << params.duty_cycle << " "
             << "volume " << params.volume << " "
             << "counter " << params.counter << " "
             << "constant_vol " << +params.constant_volume << " "
             << "linear_counter " << params.linear_counter_load << std::endl;
    }

    // LOG(INFO) << "AudioPlayer update_parameters " << magic_enum::enum_name<Audio::Channel>(channel) << " "
    //           << " reset phase " << +reset_phase << " "
    //           << "freq " << params.frequency << " "
    //           << "duty " << params.duty_cycle << " "
    //           << "volume " << params.volume << " "
    //           << "constant vol " << +params.constant_volume << " "
    //           << "counter " << params.counter << " "
    //           << "loop " << +params.loop
    //           << " sweep_enabled " << +params.sweep_enabled
    //           << " sweep_period " << params.sweep_period
    //           << " sweep_negate " << +params.sweep_negate
    //           << " sweep_shift_count " << params.sweep_shift_count;

    generator_->update_parameters(channel, params, reset_phase);
}

void AudioPlayer::set_enabled(Audio::Channel channel, bool enabled)
{
    generator_->set_enabled(channel, enabled);
}

void AudioPlayer::decrement_counter(Audio::Channel channel)
{
    generator_->decrement_counter(channel);
}

void AudioPlayer::decrement_linear_counter()
{
    generator_->decrement_linear_counter();
}

void AudioPlayer::decrement_volume_envelope(Audio::Channel channel)
{
    generator_->decrement_volume_envelope(channel);
}

void AudioPlayer::step_sweep(Audio::Channel channel)
{
    generator_->step_sweep(channel);
}

void AudioPlayer::start()
{
    // Needs to be called from the QT UI thread

    generator_->start();
    audio_sink_->setVolume(.5);
    audio_sink_->start(generator_.get());
}

void AudioPlayer::stop()
{
    generator_->stop();
    audio_sink_->stop();
}

void AudioPlayer::reset()
{
    generator_->set_enabled(Audio::Channel::Square_Pulse_1, false);
    generator_->set_enabled(Audio::Channel::Square_Pulse_2, false);
    generator_->set_enabled(Audio::Channel::Triangle, false);
    generator_->set_enabled(Audio::Channel::Noise, false);
    generator_->set_enabled(Audio::Channel::Recorded_Sample, false);
}

void AudioPlayer::step()
{
    audio_player_cycles_++;
    generator_->step();
}

void AudioPlayer::test()
{
    generator_->start();
    generator_->set_enabled(Audio::Channel::Square_Pulse_1, true);

    audio_sink_->setVolume(.1);
    audio_sink_->start(generator_.get());

    LOG(INFO) << "test_signal";
}
