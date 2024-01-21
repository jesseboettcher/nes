#include "processor_6502.hpp"

#include "processor/utils.hpp"

#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>

Processor6502::Processor6502()
{
	std::cout << "Launching Processor6502...\n";

    instr_table_ = make_instruction_table();
    
    log_.open("/home/jesse/Downloads/vnes.log");
    if (!log_.is_open())
    {
        assert(false);
    }
}

Processor6502::~Processor6502()
{

}

void Processor6502::reset()
{
	// startup sequence
	// set reset
	// disable IRQ flag
    registers_.PC = static_cast<uint16_t>(cmemory()[RESET_low_addr] | (cmemory()[RESET_hi_addr] << 8));
    registers_.A  = 0;
    registers_.X  = 0;
    registers_.Y  = 0;
    registers_.SP = 0xFF; // init stack pointer to the top of page 1
    registers_.set_SR(0);

    print_status();
}

void Processor6502::run()
{
}

bool Processor6502::step()
{
	bool should_continue = true;

	if (cycles_to_wait_ > 0)
	{
		// cycles_to_wait_ indicates the number of cycles that the processor should remain "busy"
		// executing the previous operation
		wait_for_cycle_count(1);
		return true;
	}

	if (!check_breakpoints())
	{
		// For a breakpoint, exit before changing any processor state
		return false;
	}

	if (check_nmi())
	{
		// BRK was inserted as the next operation, do not touch the PC
		should_continue = false; // DEBUG
	}
	else
	{
		uint8_t next_instruction = memory_[registers_.PC++];

        if (cycle_count_ == 265329)
        {
            print_memory(registers_.PC - 1, 64);
        }
        
		pending_operation_.values.push_back(next_instruction);
	}
	cycles_to_wait_ = 1;

	if (ready_to_execute(pending_operation_))
	{
		calculate_instruction_address(pending_operation_);

		cycles_to_wait_ += execute_instruction(pending_operation_);

		should_continue = check_watchpoints(pending_operation_);

		pending_operation_.reset();
	}

	wait_for_cycle_count(1);
	// print_status();
    
//    if (pending_operation_.values.size() == 2 &&
//        pending_operation_.values[0] == 0x6C &&
//        pending_operation_.values[1] == 0xE9)
//    if (registers_.PC == 0xE514)
//    {
//        return false;
//    }
	return should_continue;
}

void Processor6502::calculate_instruction_address(Instruction& i)
{
	// Look at the instruction data supplied along with the addressing mode for the opcode. If
	// the data or address needed by the instruction is not already fully populated, calculate and
	// update it.
	i.addr_mode = instr_table_[i.opcode()].addr_mode;

    uint16_t addr;
	switch(i.addr_mode)
	{
		case AddressingMode::ABSOLUTE:
			// i already has the address correctly populated in i.values
			break;

		case AddressingMode::ABSOLUTE_X:
			addr = i.address() + registers_.X;

			if ((addr & 0xFF00) != (i.address() & 0xFF00))
			{
				i.fetch_crossed_page_boundary = true;
			}

			i.values[1] = addr & 0x00FF;
			i.values[2] = (addr >> 8) & 0x00FF;
			break;

		case AddressingMode::ABSOLUTE_Y:
			addr = i.address() + registers_.Y;

			if ((addr & 0xFF00) != (i.address() & 0xFF00))
			{
				i.fetch_crossed_page_boundary = true;
			}

			i.values[1] = addr & 0x00FF;
			i.values[2] = (addr >> 8) & 0x00FF;
			break;

		case AddressingMode::ACCUMULATOR:
			// instruction will use the accumulator value
			break;

		case AddressingMode::IMMEDIATE:
			// the 1 byte data is already populated
			break;
		case AddressingMode::IMPLIED:
			// no instruction data
			break;

		case AddressingMode::INDIRECT:
			addr = i.values[1];
			i.values[1] = memory_[addr];
			i.values[2] = memory_[addr + 1];
			break;

		case AddressingMode::INDIRECT_X:
			// Indexed indirect - op data indicates a table of pointers in the zero page. The
			// different pointers in the table are indexed via the X register
			addr = i.values[1] + registers_.X;
			i.values[1] = memory_[addr];
			i.values[2] = memory_[addr + 1];
			break;

		case AddressingMode::INDIRECT_Y:
			// Indirect indexed - op data specifies a pointer to an address which contains a table
			// of pointers. This pointers are indexed via the Y register
			addr = (memory_[i.values[1] + 1]) << 8;
			addr |= memory_[i.values[1]];

			if ((addr & 0xFF00) != ((addr + registers_.Y) & 0xFF00))
			{
				i.fetch_crossed_page_boundary = true;
			}
			addr += registers_.Y;

			i.values[1] = addr & 0x00FF;
			i.values[2] = (addr >> 8) & 0x00FF;
			break;

		case AddressingMode::RELATIVE:
			// the 1 byte data is already populated
			break;

		case AddressingMode::ZERO_PAGE:
			addr = i.values[1];
			i.values[1] = addr & 0x00FF;
			i.values[2] = 0x0;
			break;

		case AddressingMode::ZERO_PAGE_X:
			addr = i.values[1] + registers_.X;
			i.values[1] = addr & 0x00FF;
			i.values[2] = 0x0;
			break;

		case AddressingMode::ZERO_PAGE_Y:
			addr = i.values[1] + registers_.Y;
			i.values[1] = addr & 0x00FF;
			i.values[2] = 0x0;
			break;

		case AddressingMode::INVALID:
			LOG(FATAL) << "Invalid addressing mode";
			break;
	}
}

uint8_t Processor6502::execute_instruction(const Instruction& i)
{
	if (instr_table_[i.opcode()].addr_mode == AddressingMode::INVALID)
	{
		LOG(ERROR) << "Unknown instruction: " << i;
		throw "Unimplemented instruction";
	}
	std::cout << cycle_count_ << "\t"
             << "0x" << std::hex << std::uppercase << registers_.PC << "    "
			  << instr_table_[i.opcode()].assembler << ":" << i << std::dec << std::endl;

    log_ << cycle_count_ << "\t"
         << "0x" << std::hex << std::uppercase << registers_.PC << "    "
         << instr_table_[i.opcode()].assembler << ":" << i << std::dec << std::endl;
    
	bool extra_cycles = instr_table_[i.opcode()].handler(i, registers_, memory_);
	{
		// Add string with current instruction + PC details to the history
//		std::stringstream str;
//		str << "PC: 0x" << std::hex << std::uppercase << registers_.PC
//			<< "      (cycle " << std::setw(6) << std::dec << cycle_count_ << ")"
//			<< "    " << std::hex << instr_table_[i.opcode()].assembler << ":" << i;
//		history_.push_back(str.str());
	}
	instr_count_++;

	return extra_cycles + instr_table_[i.opcode()].cycles - instr_table_[i.opcode()].bytes;
}

void Processor6502::breakpoint(const uint16_t address)
{
	std::cout << "Added breakpoint at 0x" << std::hex << std::setfill('0') << std::setw(4)
			  << std::uppercase << address << std::endl;
	breakpoints_[address] = true;
}

void Processor6502::clear_breakpoint(const uint16_t address)
{
	if (breakpoints_.count(address))
	{
		breakpoints_.erase(address);
		std::cout << "Cleared breakpoint at 0x" << std::hex << std::setfill('0') << std::setw(4)
				  << std::uppercase << address << std::endl;
	}
}

void Processor6502::watchpoint(const uint16_t address)
{
	std::cout << "Added watchpoint at 0x" << std::hex << std::setfill('0') << std::setw(4)
			  << std::uppercase << address << std::endl;
	watchpoints_[address] = true;
}

void Processor6502::clear_watchpoint(const uint16_t address)
{
	if (watchpoints_.count(address))
	{
		watchpoints_.erase(address);
		std::cout << "Cleared watchpoint at 0x" << std::hex << std::setfill('0') << std::setw(4)
				  << std::uppercase << address << std::endl;
	}
}

bool Processor6502::check_watchpoints(const Instruction& i)
{
	bool should_continue = true;

	if (watchpoints_.size())
	{
		switch(i.addr_mode)
		{
			case AddressingMode::ABSOLUTE:
			case AddressingMode::ABSOLUTE_X:
			case AddressingMode::ABSOLUTE_Y:
			case AddressingMode::ACCUMULATOR:
			case AddressingMode::INDIRECT:
			case AddressingMode::INDIRECT_X:
			case AddressingMode::INDIRECT_Y:
			case AddressingMode::ZERO_PAGE:
			case AddressingMode::ZERO_PAGE_X:
			case AddressingMode::ZERO_PAGE_Y:
				if (watchpoints_.count(i.address()))
				{
					std::cout << "Hit watchpoint at 0x" << std::hex << std::setfill('0') << std::setw(4)
							  << std::uppercase << registers_.PC << std::endl;
					should_continue = false;
				}
			case AddressingMode::INVALID:
			case AddressingMode::IMPLIED:
			case AddressingMode::IMMEDIATE:
			case AddressingMode::RELATIVE:
				break;
		}
	}
	return should_continue;
}

bool Processor6502::ready_to_execute(const Instruction& pending_op)
{
	if (pending_op.values.size() == 0)
	{
		return false;
	}

	uint8_t opcode = pending_op.opcode();
	if (instr_table_[opcode].addr_mode == AddressingMode::INVALID)
	{
        LOG(ERROR) << "test valid sig " << std::hex << +memory_[0x6001] << " " << +memory_[0x6002] << " " << +memory_[0x6003];
        LOG(ERROR) << &memory_[0x6004];
        LOG(ERROR) << "status " << +memory_[0x6000];
        LOG(ERROR) << "Unknown instruction: " << pending_op << " PC: " << registers_.PC << " cycle: " << std::dec << cycle_count_;
		log_ << "Unknown instruction: " << pending_op << " PC: " << registers_.PC << " cycle: " << std::dec << cycle_count_ << std::endl;
		print_memory(registers_.PC - 8, 64);
		throw "Unimplemented instruction";
	}

	return instr_table_[pending_op.opcode()].bytes == pending_op.values.size();
}

bool Processor6502::check_nmi()
{
	if (!non_maskable_interrupt_ || pending_operation_.values.size())
	{
		// do not break until NMI and the currently queued operation is complete
		return false;
	}

	assert(pending_operation_.values.size() == 0);
	pending_operation_.values.push_back(0x00); // break
	pending_operation_.nmi = true;

	non_maskable_interrupt_ = false;
	return true;
}

bool Processor6502::check_breakpoints()
{
	if (breakpoints_.count(registers_.PC))
	{
		if (breakpoints_[registers_.PC])
		{
			breakpoints_[registers_.PC] = false;
			
			std::cout << "Hit breakpoint at 0x" << std::hex << std::setfill('0') << std::setw(4)
					  << std::uppercase << registers_.PC << std::endl;
			return false;
		}
		else
		{
			// re-enable so it will hit on next run
			breakpoints_[registers_.PC] = true;
		}
	}
	return true;  // should continue
}

void Processor6502::wait_for_cycle_count(uint8_t cycles)
{
//	static constexpr std::chrono::nanoseconds NS_PER_CYCLE(559); // @ 1.789773 MHz
//	std::this_thread::sleep_for(cycles * NS_PER_CYCLE);

	cycle_count_ += cycles;
	cycles_to_wait_ -= cycles;
}

void Processor6502::print_status()
{
	std::cout << "PC: 0x" << std::hex << std::uppercase << registers_.PC
			  << "    " << std::hex << std::setfill('0') << std::setw(2) << +memory_[registers_.PC]
			  << "      (cycle " << std::dec << cycle_count_ << ")" << std::endl;
}

void Processor6502::print_memory(uint16_t address, uint16_t size) const
{
	std::cout << memory_.view(address, size);
}

void Processor6502::print_stack() const
{
	std::cout << memory_.stack_view(registers_.SP);
}

void Processor6502::print_registers()
{
	std::cout << registers_;
}

void Processor6502::print_breakpoints()
{
	std::cout << std::hex;
	std::cout << "\n-------------\n";
	std::cout << "  Breakpoints\n";
	std::cout << "-------------\n";

	if (breakpoints_.size() == 0)
	{
		std::cout << "none\n";
	}

	for (const auto [b, val] : breakpoints_)
	{
		std::cout << "0x" << std::hex << std::uppercase << std::setw(4) << b << std::endl;
	}
}

void Processor6502::print_watchpoints()
{
	std::cout << std::hex;
	std::cout << "\n-------------\n";
	std::cout << "  Watchpoints\n";
	std::cout << "-------------\n";

	if (watchpoints_.size() == 0)
	{
		std::cout << "none\n";
	}

	for (const auto [b, val] : watchpoints_)
	{
		std::cout << "0x" << std::hex << std::uppercase << std::setw(4) << b << std::endl;
	}
}

void Processor6502::print_history(const uint16_t num_instructions)
{
	std::cout << std::hex;
	std::cout << "\n-------------\n";
	std::cout << "  History\n";
	std::cout << "-------------\n";

	for (int32_t i = 0;i < num_instructions; ++i)
	{
		std::cout << *(history_.end() - num_instructions + i) << std::endl;
	}
}

bool is_hex(const std::string& s)
{
	if (s.starts_with("0x"))
	{
		return true;
	}
	return false;
}

// Return the number of bytes needed to store the hex value
int32_t hex_len(const std::string& s)
{
	static int32_t constexpr PREFIX_LEN = 2;

	if (s.length() > PREFIX_LEN + 4)
	{
		LOG(FATAL) << "Number is too large";
	}
	if (s.length() > PREFIX_LEN + 2)
	{
		return 2;
	}
	return 1;
}

// Return the number of bytes needed to store the hex value
int32_t dec_len(const std::string& s)
{
	if (s.length() == 0)
	{
		return 0;
	}

	uint64_t num = std::stoul(s, 0, 0);

	if (num > 0xFFFF)
	{
		LOG(FATAL) << "Number is too large";
	}
	if (num > 0xFF)
	{
		return 2;
	}
	return 1;
}

Instruction Processor6502::assemble_instruction(std::string inst_string)
{
	std::array<InstructionDetails, 0xFF> oper_details = make_instruction_table();

	for (auto& details : oper_details)
	{
        uint8_t opcode = details.opcode;
		std::string::size_type pos = std::string::npos;
		std::string assembler_regex = details.assembler;

		// Escape any existing parentheses
		pos = assembler_regex.find("(");
		if (pos != std::string::npos)
		{
			assembler_regex.insert(pos, "\\");
		}

		pos = assembler_regex.find(")");
		if (pos != std::string::npos)
		{
			assembler_regex.insert(pos, "\\");
		}

		// Insert the regex for hex/dec numbers
		pos = assembler_regex.find("oper");
		if (pos != std::string::npos)
		{
			assembler_regex.replace(pos, strlen("oper"), "(0x[A-Fa-f0-9]+|[0-9]+)");
		}

		// Match the provided instruction against this current opcode
		const std::regex oper_regex(assembler_regex);
		std::smatch base_match;
	
		bool result = std::regex_match(inst_string, base_match, oper_regex);
		if (!result)
		{
			continue;
		}

		// There can be multiple regex matches for an opcode. To disambiguate, see how many bytes
		// of data was provided
		std::string number_str = base_match[1];
		int32_t oper_data_bytes = 0;

		if (is_hex(number_str))
		{
			oper_data_bytes = hex_len(number_str);
		}
		else
		{
			oper_data_bytes = dec_len(number_str);
		}

		// Check for an opcode match based on the bytes of data supplied. Add 1 to the data supplied
		// to represent the opcode byte
		if (oper_data_bytes + 1 != details.bytes)
		{
			continue;
		}

		// Full match! Package up the information into an Instruction that can be executed
		Instruction i;

		i.values.push_back(opcode);

		if (oper_data_bytes == 1)
		{
			i.values.push_back(static_cast<uint8_t>(stoul(number_str, 0, 0)));
		}
		if (oper_data_bytes == 2)
		{
			i.values.push_back(stoul(number_str, 0, 0) & 0xFF);
			i.values.push_back((stoul(number_str, 0, 0) >> 8) & 0xFF);
		}

		// Handle addressing which adjusts data and populates addr_mode
		calculate_instruction_address(i);
		return i;
	}

	LOG(FATAL) << "Instruction not found";
	return Instruction();
}
