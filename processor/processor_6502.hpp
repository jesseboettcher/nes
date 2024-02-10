#ifndef __PROCESSOR_6502_H__
#define __PROCESSOR_6502_H__

#pragma once

#include "processor/instructions.hpp"
#include "processor/address_bus.hpp"

#include <array>
#include <cassert>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

#include <glog/logging.h>

struct Registers
{
	uint16_t PC;	// program counter
	uint8_t	A;		// accumulator
	uint8_t	X;		// x register
	uint8_t	Y;		// y register
	uint8_t SP;		// stack pointer

private:
    uint8_t	SR_;		// status register
public:
    
	// status register flags
	static constexpr uint8_t NEGATIVE_FLAG	= 1 << 7;			// N
	static constexpr uint8_t OVERFLOW_FLAG	= 1 << 6;			// V
    static constexpr uint8_t UNUSED_BIT     = 1 << 5;           // unused bit
    static constexpr uint8_t BREAK_FLAG 	= 1 << 4;			// B
	// 1 << 5 unused
	static constexpr uint8_t DECIMAL_FLAG 	= 1 << 3;			// D
	static constexpr uint8_t INTERRUPT_DISABLE_FLAG = 1 << 2;	// I
	static constexpr uint8_t ZERO_FLAG 		= 1 << 1;			// Z
	static constexpr uint8_t CARRY_FLAG 	= 1 << 0;			// C

    uint8_t SR() const
    {
        // The unused bit is floating in the hardware, so always appears set when read
        // The BREAK bit is often floating. TODO figure out when it should not be
        return SR_ | BREAK_FLAG | UNUSED_BIT;
    }
    
    void set_SR(uint8_t sr) { SR_ = sr; }
	bool is_status_register_flag_set(uint8_t flag) const { return SR_ & flag; }
	void set_status_register_flag(uint8_t flag) { SR_ |= flag; }
	void set_status_register_flag(uint8_t flag, bool enable)
	{
		if (enable)
		{
            SR_ |= flag;
		}
		else
		{
            SR_ &= ~flag;
		}
	}
	void clear_status_register_flag(uint8_t flag) { SR_ &= ~flag; }

	void update_zero_flag(Registers& r, const uint8_t compare_byte)
	{
		if (compare_byte == 0)
		{
			r.set_status_register_flag(Registers::ZERO_FLAG);
		}
		else
		{
			r.clear_status_register_flag(Registers::ZERO_FLAG);
		}
	}

	void update_negative_flag(Registers& r, const uint8_t compare_byte)
	{
		if (static_cast<int8_t>(compare_byte) < 0)
		{
			r.set_status_register_flag(Registers::NEGATIVE_FLAG);
		}
		else
		{
			r.clear_status_register_flag(Registers::NEGATIVE_FLAG);
		}
	}
};

enum class AddressingMode
{
	INVALID,
	ABSOLUTE,
	ABSOLUTE_X,
	ABSOLUTE_Y,
	ACCUMULATOR,
	IMMEDIATE,
	IMPLIED,
	INDIRECT,
	INDIRECT_X,
	INDIRECT_Y,
	RELATIVE,
	ZERO_PAGE,
	ZERO_PAGE_X,
	ZERO_PAGE_Y,
};

struct Instruction
{
	std::vector<uint8_t> values;
	AddressingMode addr_mode{AddressingMode::INVALID};
	bool fetch_crossed_page_boundary{false};
	bool nmi{false};

	uint8_t opcode() const { return values[0]; }
	uint8_t data() const { return values[1]; }
	uint16_t address() const { return values[1] | (values[2] << 8); }

	void reset()
	{
		values.clear();
        addr_mode = AddressingMode::INVALID;
		fetch_crossed_page_boundary = false;
		nmi = false;
	}
};

// Function type for executing instructions with the proivded instruction data, system registers,
// and system memory. Returns the number of extra cycles that were used to execute the instruction.
// That is cycles beyond the ones described in the InstructionDetails. Extra cycles are sometimes
// needed for things like crossing page boundaries.
using InstructionHandler = std::function<uint8_t(const Instruction& i, Registers& r, AddressBus& m)>;

struct InstructionDetails
{
	const char* assembler;
	uint8_t opcode;
	uint8_t bytes;
	uint8_t cycles;
	AddressingMode addr_mode;
	InstructionHandler handler;
};

class Processor6502
{
public:
	// Nes 6502 has 2kb of internal RAM, this array larger to support Test6502 unit tests
	using CPUMemory = std::array<uint8_t, AddressBus::ADDRESSABLE_MEMORY_SIZE>;
	static constexpr int32_t INTERNAL_MEMORY_SIZE = 2 * 1024;

	Processor6502(AddressBus& address_bus, bool& nmi_signal,
				  int32_t internal_memory_size = INTERNAL_MEMORY_SIZE);
	~Processor6502();

	// Reset registers and initialize PC to values specified by reset vector
	void reset();

	// Run and execute instructions from memory
	void run();

	// Step the processor 1 cycle. Returns true if the processor should continue running
	bool step();

	// Fetch instruction data via the specified addressing mode
	void calculate_instruction_address(Instruction& i);

	// Execute the single provided instruction, returns the number of cycles of work completed
	// less the cycles used to fetch the instructions
	uint8_t execute_instruction(const Instruction& i);

	// Set a breakpoint that will step execution during run;
	void breakpoint(const uint16_t address);
	void clear_breakpoint(const uint16_t address);

	// Set a watchpoint when a memory address is accessed
	void watchpoint(const uint16_t address);
	void clear_watchpoint(const uint16_t address);
	bool check_watchpoints(const Instruction& i);

	void set_verbose(bool verbose) { verbose_ = verbose; }
	bool verbose() { return verbose_; }

	void print_status();
	void dim_status();
	void print_memory(uint16_t address, uint16_t size) const;
	void print_stack() const;
	void print_registers();
	void print_breakpoints();
	void print_watchpoints();
	void print_history(const uint16_t num_instructions);
	void update_execution_log(const Instruction& i, uint16_t previous_pc);

	const AddressBus& cmemory() { return address_bus_; }
	const Registers& cregisters() { return registers_; }
	uint64_t instruction_count() { return instr_count_; }
	uint64_t cycle_count() { return cycle_count_; }

	// Generates an instruction that is ready to execute for the provided assembly string
	Instruction assemble_instruction(std::string inst_string);

protected:
	friend class CartridgeInterface;
	friend class Joypads;
	friend class NesPPU;
	friend class Test6502;
	friend class Nes;
	friend class CommandPrompt;
	AddressBus& memory() { return address_bus_; }
	Registers& registers() { return registers_; }

	// internal memory accessors
	friend class AddressBus;
	int32_t internal_memory_size() const { return internal_memory_size_; }
	uint8_t read(uint16_t a) const;
	uint8_t& write(uint16_t a);

	void set_non_maskable_interrupt() { non_maskable_interrupt_ = true; }

private:
	// Check whether all of the data has been loaded for the pending instruction
	bool ready_to_execute(const Instruction& pending_op);

	// Insert a BRK if the NMI flag is set, prior to running the next instruction
	bool check_nmi();

	// returns true if execution should continue, false if a breakpoint has been hit
	bool check_breakpoints();

	// Delay for the designated # of cycles
	void wait_for_cycle_count(uint8_t cycles);

	Instruction pending_operation_;
	uint64_t	cycle_count_{0};
	uint64_t	instr_count_{0};
	bool verbose_{true};

	int32_t cycles_to_wait_{0};

	AddressBus& address_bus_;
	Registers registers_{};
	bool& non_maskable_interrupt_;

    CPUMemory internal_memory_;
    int32_t internal_memory_size_;

    // Needs fast lookups
    std::array<InstructionDetails, 0xFF> instr_table_;

	std::unordered_map<uint16_t, bool> breakpoints_;
	std::unordered_map<uint16_t, bool> watchpoints_;

	std::vector<std::string> history_;
    std::ofstream log_;
};

#endif  // __PROCESSOR_6502_H__
