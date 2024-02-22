# audio_gen
#
# Reads audio channel parameter change logs and uses them to create wav files, for each channel
# separately, and mixed together. Helpful when debugging APU issues to here <roughly> what the
# audio should sound like.

from enum import Enum
import matplotlib.pyplot as plt
import numpy as np
from pydub import AudioSegment
from pydub.generators import Sine, Square, Triangle, Pulse

PLOT_OUTPUTS = False

class ChannelType(Enum):
    SQUARE_PULSE_1 = 'Square_Pulse_1'
    SQUARE_PULSE_2 = 'Square_Pulse_2'
    TRIANGLE = 'Triangle'

    def from_string(str):
        for channel in ChannelType:
            if channel.value == str:
                return channel

class SquareDuty(Pulse):
    def __init__(self, freq, **kwargs):
        super(SquareDuty, self).__init__(freq, **kwargs)

def generate_waveform(waveform_type, frequency, duty_cycle, length_ms):

    sample_rate = 44100  # Hz

    if waveform_type == 'sine':
        wave = Sine(frequency).to_audio_segment(duration=length_ms)

    elif waveform_type == 'square' or waveform_type == 'Square_Pulse_1' or waveform_type == 'Square_Pulse_2':

        if (duty_cycle == -0.25):
            # should invert the outputs
            wave = SquareDuty(frequency, duty_cycle=0.25).to_audio_segment(duration=length_ms).apply_gain(-25)
        else:
            wave = SquareDuty(frequency, duty_cycle=duty_cycle).to_audio_segment(duration=length_ms).apply_gain(-25)

    elif waveform_type == 'triangle' or waveform_type == 'Triangle':
        wave = Triangle(frequency).to_audio_segment(duration=length_ms).apply_gain(-5)

    else:
        raise ValueError("Unsupported waveform type")

    return wave


def read_audio_log(path):

    print(f'Reading log at {path}')

    lines = []
    with open(audio_log_path, 'r') as file:
        lines = file.readlines();

    line_count = 1

    waves_by_type = {}
    for channel in ChannelType:
        waves_by_type[channel] = []

    for line in lines:

        line_count += 1

        wave_params = {}

        # parse wave parameters into a dictionary
        # format:
        #   Square_Pulse_2 cycle 59 freq 440 duty 0.5 volume 9 constant vol 0
        #   Triangle cycle 59 freq 87 duty 0 volume 0 constant vol 0

        split_params = line.strip().split(' ')

        wave_params['type'] = split_params[0]
        offset = 1;

        while offset < len(split_params):
            next_params = tuple(split_params[offset:offset + 2])

            value = next_params[1]
            if next_params[0] == 'duty':
                value = float(value)
            else:
                value = int(value)
            wave_params[next_params[0]] = value;

            offset += 2

        # filter out bogus wave params
        if wave_params['counter'] == 0:
            continue

        if wave_params['freq'] <= 0:
            continue

        waves_by_type[ChannelType.from_string(wave_params['type'])].append(wave_params)

    return waves_by_type


def generate_audio_channel(channel, waves, first_cycle):

    print(f'Generating audio for channel {channel}')

    output_wave = AudioSegment.silent(duration=100)

    last_cycle = first_cycle

    for i in range(len(waves) - 1):

        if i == 0:
            continue

        last_wave_params = waves[i - 1]
        wave_params = waves[i]
        next_wave_params = waves[i + 1]

        if next_wave_params['cycle'] == wave_params['cycle']:
            continue

        # create audio samples for waveform description
        time_since_last_waveform = 1000 / 120 * (wave_params['cycle'] - last_cycle)

        duration_by_counter = 1000 / 440 * wave_params['counter'] # ms
        duration_by_linear_counter = 1000 / 440 * wave_params['linear_counter'] # ms
        duration_by_next_waveform = 1000 / 120 * (next_wave_params['cycle'] - wave_params['cycle'])

        duration_by_counter = duration_by_counter if duration_by_counter > 0 else float('inf')
        duration_by_linear_counter = duration_by_linear_counter if duration_by_linear_counter > 0 else float('inf')
        duration_by_next_waveform = duration_by_next_waveform if duration_by_next_waveform > 0 else float('inf')

        this_waveform_duration = min(duration_by_counter, duration_by_linear_counter, duration_by_next_waveform)
        last_cycle = wave_params['cycle']

        wave = generate_waveform(wave_params['type'], wave_params['freq'], wave_params['duty'], this_waveform_duration)

        if wave_params['constant_vol'] == 0: # should fade out
            wave = wave.fade(to_gain=-wave_params['volume'], start=0, duration=int(this_waveform_duration))

        output_wave = output_wave + AudioSegment.silent(duration=time_since_last_waveform) + wave

    return output_wave


audio_log_path = '/private/tmp/nes_audio_parameters.log'

waves_by_type = read_audio_log(audio_log_path)
outputs_by_type = {}

first_cycle = min(
              waves_by_type[ChannelType.SQUARE_PULSE_1][0]['cycle'],\
              waves_by_type[ChannelType.SQUARE_PULSE_2][0]['cycle'],\
              waves_by_type[ChannelType.TRIANGLE][0]['cycle'])

for key in waves_by_type.keys():
    outputs_by_type[key] = generate_audio_channel(key, waves_by_type[key], first_cycle)
    outputs_by_type[key].export('output_channel_' + key.value + '.wav', format='wav') # write each channel as a separate file

# merge the channels and write the combined audio to a file
output_wave = outputs_by_type[ChannelType.SQUARE_PULSE_1].\
              overlay(outputs_by_type[ChannelType.SQUARE_PULSE_2].\
              overlay(outputs_by_type[ChannelType.TRIANGLE]))

print(f'Writing combined audio file output.wav')
output_wave.export("output.wav", format="wav")

if PLOT_OUTPUTS:
    for key in waves_by_type.keys():
        samples = np.array(outputs_by_type[key].get_array_of_samples())
        plt.plot(samples, label=str(key), alpha=0.5)

    plt.title("Audio Waveform")
    plt.ylabel("Amplitude")
    plt.xlabel("Sample")
    plt.legend()
    plt.show()
    plt.savefig(f"output_plot.png", dpi=300)
