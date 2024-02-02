#include "processor_6502.hpp"

#include "processor/utils.hpp"
#include "platform/ui_properties.hpp"

#include <iomanip>
#include <iostream>
#include <format>
#include <regex>
#include <sstream>
#include <thread>

void write_pc_to_file(unsigned int pc)
{
    std::ofstream outFile("/tmp/nes_pc", std::ios::out | std::ios::trunc);
    if (!outFile)
    {
        std::cerr << "Failed to open file: " << "/tmp/nes_pc" << std::endl;
        return;
    }

    // matching the output of disassembler for PC formatting
    outFile << std::setfill('0') << std::uppercase << std::setw(4) << std::hex << pc;
    outFile.close();
}

Processor6502::Processor6502()
{
    std::cout << "Launching Processor6502...\n";

    instr_table_ = make_instruction_table();

    if constexpr (ENABLE_CPU_LOGGING)
    {
        std::filesystem::remove("/tmp/vnes.log");
        log_.open("/tmp/vnes.log");
        if (!log_.is_open())
        {
            LOG(WARNING) << "Could not write /tmp/vnes.log";
        }
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
        uint8_t next_instruction = memory_.read(registers_.PC++);

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

    return should_continue;
}

void Processor6502::calculate_instruction_address(Instruction& i)
{
    // Look at the instruction data supplied along with the addressing mode for the opcode. If
    // the data or address needed by the instruction is not already fully populated, calculate and
    // update it.
    i.addr_mode = instr_table_[i.opcode()].addr_mode;

    uint16_t addr = 0;
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
        {
            addr = (static_cast<uint16_t>(i.values[2]) << 8) | static_cast<uint16_t>(i.values[1]);

            // In the 6502, the when low byte is incremented for a JMP, the carry is not handled
            // and the low byte wraps to 0x00 without affecting the high byte
            uint16_t inc_addr = 0;
            inc_addr |= (static_cast<uint16_t>(i.values[2]) << 8) & 0xFF00;
            inc_addr |= (static_cast<uint8_t>(i.values[1]) + 1) & 0x00FF;

            i.values[1] = memory_[addr];
            i.values[2] = memory_[inc_addr];

            break;
        }

        case AddressingMode::INDIRECT_X:
            // Indexed indirect - op data indicates a table of pointers in the zero page. The
            // different pointers in the table are indexed via the X register

            addr = static_cast<uint8_t>(i.values[1] + registers_.X); // truncate addr to 1 byte
            i.values[1] = memory_[addr];
            i.values[2] = memory_[static_cast<uint8_t>(addr + 1)]; // truncate addr to 1 byte
            break;

        case AddressingMode::INDIRECT_Y:
            // Indirect indexed - op data specifies a pointer to an address which contains a table
            // of pointers. This pointers are indexed via the Y register
            addr = (memory_[static_cast<uint8_t>(i.values[1] + 1)]) << 8; // truncate addr to 1 byte
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

    if (verbose())
    {
        // std::cout << cycle_count_ << "\t"
        //          << "0x0" << std::hex << std::uppercase << registers_.PC << "    "
        //        << instr_table_[i.opcode()].assembler << ":" << i << std::dec << std::endl;

        std::stringstream str;
        str << instr_table_[i.opcode()].assembler << ":" << i;
        update_ui(UI::current_instruction_label, str.str(), UI_NEAR_BLACK);
    }

    if constexpr (ENABLE_CPU_LOGGING)
    {
        log_ << cycle_count_ << "\t"
             << "0x" << std::hex << std::uppercase << registers_.PC << "    "
             << instr_table_[i.opcode()].assembler << ":" << i << std::dec << std::endl;
    }
    
    bool extra_cycles = instr_table_[i.opcode()].handler(i, registers_, memory_);
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
    // static std::chrono::time_point<std::chrono::high_resolution_clock> last_call_time_;

    // auto now = std::chrono::high_resolution_clock::now();
    // auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_call_time_);
    // last_call_time_ = now;

    // static constexpr std::chrono::nanoseconds NS_PER_CYCLE(55);//559); // @ 1.789773 MHz
    // auto desired_duration = NS_PER_CYCLE * cycles;
    // if (elapsed < desired_duration)
    // {
    //     auto wait_duration = desired_duration - elapsed;
    //     std::this_thread::sleep_for(wait_duration);
    // }

    cycle_count_ += cycles;
    cycles_to_wait_ -= cycles;
}

void Processor6502::print_status()
{
    // std::cout << "PC: 0x" << std::hex << std::uppercase << registers_.PC
    //        << "    " << std::hex << std::setfill('0') << std::setw(2) << +memory_[registers_.PC]
    //        << "      (cycle " << std::dec << cycle_count_ << ")" << std::endl;

    update_ui(UI::pc_label,    strformat("0x%04X", registers_.PC), UI_NEAR_BLACK);
    update_ui(UI::a_reg_label, strformat("0x%04X", registers_.A), UI_NEAR_BLACK);
    update_ui(UI::x_reg_label, strformat("0x%04X", registers_.X), UI_NEAR_BLACK);
    update_ui(UI::y_reg_label, strformat("0x%04X", registers_.Y), UI_NEAR_BLACK);
    update_ui(UI::sp_reg_label, strformat("0x%04X", registers_.SP), UI_NEAR_BLACK);

    write_pc_to_file(registers_.PC);
}

void Processor6502::dim_status()
{
    update_ui(UI::pc_label,    std::nullopt, UI_LIGHT_GREY);
    update_ui(UI::a_reg_label, std::nullopt, UI_LIGHT_GREY);
    update_ui(UI::x_reg_label, std::nullopt, UI_LIGHT_GREY);
    update_ui(UI::y_reg_label, std::nullopt, UI_LIGHT_GREY);
    update_ui(UI::sp_reg_label, std::nullopt, UI_LIGHT_GREY);
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

uint8_t Processor6502::read(uint16_t a) const
{
    return internal_memory_[a];
}

uint8_t& Processor6502::write(uint16_t a)
{
    return internal_memory_[a];
}
