#ifndef __FLAGS_H__
#define __FLAGS_H__

#pragma once

// Enables writing raw output of each channel to /tmp/[channel name].log
static constexpr bool ENABLE_APU_WAVEFORM_LOGGING = false;

// Enables writing all channel paramter changes in a single log to /tmp.
// Written in a format that can be parsed by scripts/audio_gen.py. The script
// uses the parameters and a python audio package to generate wav files.
static constexpr bool ENABLE_APU_PARAMETERS_LOGGING = false;

// Enables a log of CPU instructions, cycle counts, and register values. e.g.:
// E684  4C B3 EA  JMP $EAB3    c14532915    i4977228     A:00 X:00 Y:20 P:27 SP:FC 
static constexpr bool ENABLE_CPU_LOGGING = false;

#endif  // __FLAGS_H__
