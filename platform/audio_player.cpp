#include "platform/audio_player.hpp"

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
}

void AudioPlayer::update_parameters(Audio::Channel channel, Audio::Parameters params)
{
    LOG(INFO) << "AudioPlayer update_parameters " << magic_enum::enum_name<Audio::Channel>(channel) << " "
              << "freq " << params.frequency << " "
              << "duty " << params.duty_cycle << " "
              << "volume " << params.volume << " "
              << "constant vol " << +params.constant_volume << " "
              << "counter " << params.counter << " "
              << "loop " << +params.loop;

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    QAudioFormat format = device.preferredFormat();

    generator_->update_parameters(channel, params,
                                  square_wav(format, 1 * 1000000, params.frequency));
}

void AudioPlayer::set_enabled(Audio::Channel channel, bool enabled)
{
    generator_->set_enabled(channel, enabled);
}

void AudioPlayer::decrement_counter(Audio::Channel channel)
{
    generator_->decrement_counter(channel);
}

void AudioPlayer::decrement_volume_envelope(Audio::Channel channel)
{
    generator_->decrement_volume_envelope(channel);
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

void AudioPlayer::test()
{
    generator_->start();
    generator_->set_enabled(Audio::Channel::Square_Pulse_1, true);

    audio_sink_->setVolume(.1);
    audio_sink_->start(generator_.get());

    LOG(INFO) << "test_signal";
}
