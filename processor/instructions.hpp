#pragma once

#include <functional>
#include <unordered_map>

#include <stdint.h>

// Interrupt vector addresses
static constexpr uint16_t NMI_low_addr = 0xFFFA;
static constexpr uint16_t NMI_hi_addr = 0xFFFB;
static constexpr uint16_t RESET_low_addr = 0xFFFC;
static constexpr uint16_t RESET_hi_addr = 0xFFFD;
static constexpr uint16_t BRK_low_addr = 0xFFFE;
static constexpr uint16_t BRK_hi_addr = 0xFFFF;

struct InstructionDetails;

std::array<InstructionDetails, 0xFF> make_instruction_table();
