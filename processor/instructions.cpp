#include "processor/instructions.hpp"

#include "processor/utils.hpp"

#include <glog/logging.h>

#include <array>

static uint8_t ADC(const Instruction& i, Registers& r, AddressBus& m)
{
    // A + M + C -> A, C
    // N    Z   C   I   D   V
    // +    +   +   -   -   +
    uint8_t extra_cycles_used = i.fetch_crossed_page_boundary ? 1 : 0;
    const AddressBus& cm = m;
    uint8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];
    uint8_t carry = r.is_status_register_flag_set(Registers::CARRY_FLAG);

    int16_t result = 0;

    if (r.is_status_register_flag_set(Registers::DECIMAL_FLAG) &&
        false) // decimal mode from 6502 was removed for cost savings from the nes version
    {
        uint8_t num1 = 0x0F & r.A;
        uint8_t num2 = 0x0F & data;

        int16_t local_result = num1 + num2 + carry;
        bool local_carry = local_result > 9;
        result = local_result > 9 ? local_result - 10 : local_result;

        num1 = 0x0F & (r.A >> 4);
        num2 = 0x0F & (data >> 4);
        local_result = num1 + num2 + local_carry;
        local_carry = local_result > 9;
        result |= local_result > 9 ? (local_result - 10) << 4 : local_result << 4;

        r.set_status_register_flag(Registers::CARRY_FLAG, local_carry);
    }
    else // normal addition
    {
        result = static_cast<uint16_t>(r.A) + static_cast<uint16_t>(data) + carry;

        r.set_status_register_flag(Registers::CARRY_FLAG, result > 0xFF);
        r.set_status_register_flag(Registers::NEGATIVE_FLAG, static_cast<int8_t>(result) < 0);
    }
    r.set_status_register_flag(Registers::ZERO_FLAG, (result & 0x00FF) == 0);

    // Overflow flag should be set if both operands had matching signedness and then the result
    // has a different sign (last bit)
    bool operands_matching_signedness = ((data & 0x80) && (r.A & 0x80)) ||
                                        (!(data & 0x80) && !(r.A & 0x80));

    r.set_status_register_flag(Registers::OVERFLOW_FLAG, operands_matching_signedness &&
                                                    (result & 0x80) != (r.A & 0x80));

    r.A = result & 0x00FF;

    return extra_cycles_used;
}

static uint8_t AND(const Instruction& i, Registers& r, AddressBus& m)
{
    // A AND M -> A
    // N   Z   C   I   D   V
    // +   +   -   -   -   -
    uint8_t extra_cycles_used = i.fetch_crossed_page_boundary ? 1 : 0;
    const AddressBus& cm = m;
    uint8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];

    r.A &= data;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.A == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.A & 0x80);

    return extra_cycles_used;
}

static uint8_t ASL(const Instruction& i, Registers& r, AddressBus& m)
{
    // C <- [76543210] <- 0
    // N   Z   C   I   D   V
    // +   +   +   -   -   -

    uint8_t data = i.addr_mode == AddressingMode::ACCUMULATOR ? r.A : m[i.address()];
    bool set_carry = data & 0x80;

    data <<= 1;
    data &= 0xFE;

    r.set_status_register_flag(Registers::ZERO_FLAG, !data);
    r.set_status_register_flag(Registers::CARRY_FLAG, set_carry);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, data & 0x80);

    if (i.addr_mode == AddressingMode::ACCUMULATOR)
    {
        r.A = data;
    }
    else
    {
        m[i.address()] = data;
    }

    return 0;
}

static uint8_t BCC(const Instruction& i, Registers& r, AddressBus& m)
{
    uint8_t extra_cycles_used = 0;

    if (!r.is_status_register_flag_set(Registers::CARRY_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != ((r.PC + i.data()) & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t BCS(const Instruction& i, Registers& r, AddressBus& m)
{
    uint8_t extra_cycles_used = 0;

    if (r.is_status_register_flag_set(Registers::CARRY_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != ((r.PC + i.data()) & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t BEQ(const Instruction& i, Registers& r, AddressBus& m)
{
    uint8_t extra_cycles_used = 0;

    if (r.is_status_register_flag_set(Registers::ZERO_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != ((r.PC + i.data()) & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t BIT(const Instruction& i, Registers& r, AddressBus& m)
{
    // bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
    // the zero-flag is set to the result of operand AND accumulator.

    // A AND M, M7 -> N, M6 -> V
    // N   Z   C   I   D   V
    // M7  +   -   -   -   M6
    const AddressBus& cm = m;
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, cm[i.address()] & 0x80);
    r.set_status_register_flag(Registers::OVERFLOW_FLAG, cm[i.address()] & 0x40);
    r.set_status_register_flag(Registers::ZERO_FLAG, (cm[i.address()] & r.A) == 0);

    return 0;
}

static uint8_t BMI(const Instruction& i, Registers& r, AddressBus& m)
{
    uint8_t extra_cycles_used = 0;

    if (r.is_status_register_flag_set(Registers::NEGATIVE_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != ((r.PC + i.data()) & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t BNE(const Instruction& i, Registers& r, AddressBus& m)
{
    // branch on Z = 0
    // N    Z   C   I   D   V
    // -    -   -   -   -   -

    uint8_t extra_cycles_used = 0;

    if (!r.is_status_register_flag_set(Registers::ZERO_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != (i.data() & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t BPL(const Instruction& i, Registers& r, AddressBus& m)
{
    uint8_t extra_cycles_used = 0;

    if (!r.is_status_register_flag_set(Registers::NEGATIVE_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != ((r.PC + i.data()) & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t BRK(const Instruction& i, Registers& r, AddressBus& m)
{
    // BRK
    // Force Break

    // BRK initiates a software interrupt similar to a hardware
    // interrupt (IRQ). The return address pushed to the stack is
    // PC+2, providing an extra byte of spacing for a break mark
    // (identifying a reason for the break.)
    // The status register will be pushed to the stack with the break
    // flag set to 1. However, when retrieved during RTI or by a PLP
    // instruction, the break flag will be ignored.
    // The interrupt disable flag is not set automatically.

    // interrupt,
    // push PC+2, push SR
    // N   Z   C   I   D   V
    // -   -   -   1   -   -
    uint16_t save_pc = i.nmi ? r.PC : r.PC + 1 + 0 /* was already incremented by 1 to get here */;

    m.stack_push(r.SP, static_cast<uint8_t>((save_pc >> 8) & 0x00FF));
    m.stack_push(r.SP, static_cast<uint8_t>(save_pc & 0x00FF));

    m.stack_push(r.SP, r.SR());

    r.set_status_register_flag(Registers::BREAK_FLAG, true);
    r.set_status_register_flag(Registers::INTERRUPT_DISABLE_FLAG, true);

    const AddressBus& cm = m;
    r.PC = i.nmi ? (cm[NMI_low_addr] | (cm[NMI_hi_addr] << 8)) :
                   (cm[BRK_low_addr] | (cm[BRK_hi_addr] << 8));


    return 0;
}

static uint8_t BVC(const Instruction& i, Registers& r, AddressBus& m)
{
    uint8_t extra_cycles_used = 0;

    if (!r.is_status_register_flag_set(Registers::OVERFLOW_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != ((r.PC + i.data()) & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t BVS(const Instruction& i, Registers& r, AddressBus& m)
{
    uint8_t extra_cycles_used = 0;

    if (r.is_status_register_flag_set(Registers::OVERFLOW_FLAG))
    {
        extra_cycles_used++;

        if ((r.PC & 0xFF00) != ((r.PC + i.data()) & 0xFF00))
        {
            // Page boundary crossed
            extra_cycles_used++;
        }

        r.PC += static_cast<int8_t>(i.data());
        
    }

    return extra_cycles_used;
}

static uint8_t CLC(const Instruction& i, Registers& r, AddressBus& m)
{
    // 0 -> C
    // N    Z   C   I   D   V
    // -    -   0   -   -   -

    r.clear_status_register_flag(Registers::CARRY_FLAG);
    return 0;
}

static uint8_t CLD(const Instruction& i, Registers& r, AddressBus& m)
{
    // 0 -> D
    // N    Z   C   I   D   V
    // -    -   -   -   0   -

    r.clear_status_register_flag(Registers::DECIMAL_FLAG);
    return 0;
}

static uint8_t CLI(const Instruction& i, Registers& r, AddressBus& m)
{
    // 0 -> D
    // N    Z   C   I   D   V
    // -    -   -   0   -   -

    r.clear_status_register_flag(Registers::INTERRUPT_DISABLE_FLAG);
    return 0;
}

static uint8_t CLV(const Instruction& i, Registers& r, AddressBus& m)
{
    // 0 -> D
    // N    Z   C   I   D   V
    // -    -   -   -   -   0

    r.clear_status_register_flag(Registers::OVERFLOW_FLAG);
    return 0;
}

static uint8_t CMP(const Instruction& i, Registers& r, AddressBus& m)
{
    // Y - M
    // N    Z   C   I   D   V
    // +    +   +   -   -   -
    uint8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : m.read(i.address());
    uint8_t extra_cycles_used = i.fetch_crossed_page_boundary ? 1 : 0;

    uint8_t difference = r.A - data;

    r.set_status_register_flag(Registers::CARRY_FLAG, r.A >= data);
    r.set_status_register_flag(Registers::ZERO_FLAG, difference == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, difference & 0x80);

    return extra_cycles_used;
}

static uint8_t CPX(const Instruction& i, Registers& r, AddressBus& m)
{
    // Y - M
    // N    Z   C   I   D   V
    // +    +   +   -   -   -
    uint8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : m.read(i.address());

    uint8_t difference = r.X - data;

    r.set_status_register_flag(Registers::CARRY_FLAG, r.X >= data);
    r.set_status_register_flag(Registers::ZERO_FLAG, difference == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, difference & 0x80);

    return 0;
}

static uint8_t CPY(const Instruction& i, Registers& r, AddressBus& m)
{
    // Y - M
    // N    Z   C   I   D   V
    // +    +   +   -   -   -
    uint8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : m.read(i.address());

    uint8_t difference = r.Y - data;

    r.set_status_register_flag(Registers::CARRY_FLAG, r.Y >= data);
    r.set_status_register_flag(Registers::ZERO_FLAG, difference == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, difference & 0x80);

    return 0;
}

static uint8_t DEC(const Instruction& i, Registers& r, AddressBus& m)
{
    // M - 1 -> M
    // N   Z   C   I   D   V
    // +   +   -   -   -   -

    m[i.address()]--;

    r.update_zero_flag(r, m[i.address()]);
    r.update_negative_flag(r, m[i.address()]);

    return 0;
}

static uint8_t DEX(const Instruction& i, Registers& r, AddressBus& m)
{
    // X - 1 -> X
    // N   Z   C   I   D   V
    // +   +   -   -   -   -

    r.X--;

    r.update_zero_flag(r, r.X);
    r.update_negative_flag(r, r.X);

    return 0;
}

static uint8_t DEY(const Instruction& i, Registers& r, AddressBus& m)
{
    // X - 1 -> X
    // N   Z   C   I   D   V
    // +   +   -   -   -   -

    r.Y--;

    r.update_zero_flag(r, r.Y);
    r.update_negative_flag(r, r.Y);

    return 0;
}

static uint8_t EOR(const Instruction& i, Registers& r, AddressBus& m)
{
    // A EOR M -> A
    // N   Z   C   I   D   V
    // +   +   -   -   -   -
    const AddressBus& cm = m;
    uint8_t extra_cycles_used = i.fetch_crossed_page_boundary ? 1 : 0;
    uint8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];

    r.A ^= data;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.A == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.A & 0x80);

    return extra_cycles_used;
}

static uint8_t INC(const Instruction& i, Registers& r, AddressBus& m)
{
    // M + 1 -> M
    // N    Z   C   I   D   V
    // +    +   -   -   -   -

    m[i.address()]++;

    r.update_zero_flag(r, m[i.address()]);
    r.update_negative_flag(r, m[i.address()]);

    return 0;
}

static uint8_t INX(const Instruction& i, Registers& r, AddressBus& m)
{
    // Y + 1 -> Y
    // N    Z   C   I   D   V
    // +    +   -   -   -   -

    r.X++;

    r.update_zero_flag(r, r.X);
    r.update_negative_flag(r, r.X);

    return 0;
}

static uint8_t INY(const Instruction& i, Registers& r, AddressBus& m)
{
    // Y + 1 -> Y
    // N    Z   C   I   D   V
    // +    +   -   -   -   -

    r.Y++;

    r.update_zero_flag(r, r.Y);
    r.update_negative_flag(r, r.Y);

    return 0;
}

static uint8_t JMP(const Instruction& i, Registers& r, AddressBus& m)
{
    // (PC+1) -> PCL
    // (PC+2) -> PCH
    // N    Z   C   I   D   V
    // -    -   -   -   -   -

    r.PC = i.address();

    return 0;
}

static uint8_t JSR(const Instruction& i, Registers& r, AddressBus& m)
{
    // push (PC+2),
    // (PC+1) -> PCL
    // (PC+2) -> PCH
    // N    Z   C   I   D   V
    // -    -   -   -   -   -

    // PC is decremented and pointing to the last addess of this instruction. It will be incremented
    // on return from subroutine (RTS) so the PC will then be pointing at the next opcode
    r.PC--;

    // Save PC to stack and update PC to address indicated by JSR params
    m.stack_push(r.SP, r.PC >> 8);
    m.stack_push(r.SP, static_cast<uint8_t>(r.PC & 0x00FF));
    r.PC = i.address();

    return 0;
}

static uint8_t LDA(const Instruction& i, Registers& r, AddressBus& m)
{
    // M -> A
    // N    Z   C   I   D   V
    // +    +   -   -   -   -

    const AddressBus& cm = m;
    int8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];

    r.A = data;

    r.update_zero_flag(r, r.A);
    r.update_negative_flag(r, r.A);

    return 0;
}

static uint8_t LDX(const Instruction& i, Registers& r, AddressBus& m)
{
    // M -> X
    // N    Z   C   I   D   V
    // +    +   -   -   -   -

    const AddressBus& cm = m;
    int8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];

    r.X = data;

    r.update_zero_flag(r, r.X);
    r.update_negative_flag(r, r.X);

    return 0;
}

static uint8_t LDY(const Instruction& i, Registers& r, AddressBus& m)
{
    // M -> Y
    // N    Z   C   I   D   V
    // +    +   -   -   -   -

    const AddressBus& cm = m;
    int8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];

    r.Y = data;

    r.update_zero_flag(r, r.Y);
    r.update_negative_flag(r, r.Y);

    return 0;
}

static uint8_t LSR(const Instruction& i, Registers& r, AddressBus& m)
{
    // 0 -> [76543210] -> C
    // N   Z   C   I   D   V
    // 0   +   +   -   -   -

    uint8_t data = i.addr_mode == AddressingMode::ACCUMULATOR ? r.A : m[i.address()];
    bool set_carry = data & 0x01;

    data >>= 1;
    data &= 0x7F;

    r.set_status_register_flag(Registers::ZERO_FLAG, !data);
    r.set_status_register_flag(Registers::CARRY_FLAG, set_carry);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, false);

    if (i.addr_mode == AddressingMode::ACCUMULATOR)
    {
        r.A = data;
    }
    else
    {
        m[i.address()] = data;
    }

    return 0;
}

static uint8_t NOP(const Instruction& i, Registers& r, AddressBus& m)
{
    return 0;
}

static uint8_t ORA(const Instruction& i, Registers& r, AddressBus& m)
{
    // A OR M -> A
    // N   Z   C   I   D   V
    // +   +   -   -   -   -
    uint8_t extra_cycles_used = i.fetch_crossed_page_boundary ? 1 : 0;

    const AddressBus& cm = m;
    int8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];

    r.A |= data;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.A == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.A & 0x80);

    return extra_cycles_used;
}

static uint8_t PHA(const Instruction& i, Registers& r, AddressBus& m)
{
    m.stack_push(r.SP, r.A);
    return 0;
}

static uint8_t PHP(const Instruction& i, Registers& r, AddressBus& m)
{
    // TODO resolve difference in masswerk.at and 6502 dev guide pdf on whether this affects R
    m.stack_push(r.SP, r.SR());
    return 0;
}

static uint8_t PLA(const Instruction& i, Registers& r, AddressBus& m)
{
    // pull A
    // N   Z   C   I   D   V
    // +   +   -   -   -   -
    r.A = m.stack_pop(r.SP);
    r.set_status_register_flag(Registers::ZERO_FLAG, r.A == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.A & 0x80);

    return 0;
}

static uint8_t PLP(const Instruction& i, Registers& r, AddressBus& m)
{
    r.set_SR(m.stack_pop(r.SP));
    r.set_status_register_flag(Registers::BREAK_FLAG, false);

    return 0;
}

static uint8_t ROL(const Instruction& i, Registers& r, AddressBus& m)
{
    // C <- [76543210] <- C
    // N   Z   C   I   D   V
    // +   +   +   -   -   -

    uint8_t data = i.addr_mode == AddressingMode::ACCUMULATOR ? r.A : m[i.address()];
    bool set_carry = data & 0x80;

    data <<= 1;
    data |= r.is_status_register_flag_set(Registers::CARRY_FLAG) ? 0x01 : 0x00;

    r.set_status_register_flag(Registers::ZERO_FLAG, !data);
    r.set_status_register_flag(Registers::CARRY_FLAG, set_carry);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, data & 0x80);

    if (i.addr_mode == AddressingMode::ACCUMULATOR)
    {
        r.A = data;
    }
    else
    {
        m[i.address()] = data;
    }

    return 0;
}

static uint8_t ROR(const Instruction& i, Registers& r, AddressBus& m)
{
    // C -> [76543210] -> C
    // N   Z   C   I   D   V
    // +   +   +   -   -   -

    uint8_t data = i.addr_mode == AddressingMode::ACCUMULATOR ? r.A : m[i.address()];
    bool set_carry = data & 0x01;

    data >>= 1;
    data |= r.is_status_register_flag_set(Registers::CARRY_FLAG) ? 0x80 : 0x00;

    r.set_status_register_flag(Registers::ZERO_FLAG, !data);
    r.set_status_register_flag(Registers::CARRY_FLAG, set_carry);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, data & 0x80);

    if (i.addr_mode == AddressingMode::ACCUMULATOR)
    {
        r.A = data;
    }
    else
    {
        m[i.address()] = data;
    }

    return 0;
}

static uint8_t RTI(const Instruction& i, Registers& r, AddressBus& m)
{
    // RTI
    // Return from Interrupt

    // The status register is pulled with the break flag
    // and bit 5 ignored. Then PC is pulled from the stack.

    // pull SR, pull PC
    // N   Z   C   I   D   V
    // from stack

    r.set_SR(m.stack_pop(r.SP));

    r.PC = 0;
    r.PC |= m.stack_pop(r.SP);
    r.PC |= m.stack_pop(r.SP) << 8;
 
    return 0;
}

static uint8_t RTS(const Instruction& i, Registers& r, AddressBus& m)
{
    // pull PC, PC+1 -> PC
    // N    Z   C   I   D   V
    // -    -   -   -   -   -

    // PC stored on the stack was pointing to the last instruction of the JSR call
    // After restoring the PC, increment it so it is pointing to the next opcode

    r.PC = 0;
    r.PC |= m.stack_pop(r.SP);
    r.PC |= m.stack_pop(r.SP) << 8;
    r.PC++;

    return 0;
}

static uint8_t SBC(const Instruction& i, Registers& r, AddressBus& m)
{
    // Subtract with carry subtracts M from A and an implied carry and the contents of the carry flag
    // There is no subtract without a carry instruction. Must set the carry flag explicitly (SEC)
    // before an SBC to do a subtraction without a carry. 

    // A - M - C -> A
    // N   Z   C   I   D   V
    // +   +   +   -   -   +
    uint8_t extra_cycles_used = i.fetch_crossed_page_boundary ? 1 : 0;

    const AddressBus& cm = m;
    int8_t data = (i.addr_mode == AddressingMode::IMMEDIATE) ? i.data() : cm[i.address()];
    int16_t result = 0;

    if (r.is_status_register_flag_set(Registers::DECIMAL_FLAG) &&
        false) // decimal mode was removed for cost savings from the nes 6502
    {
        uint8_t num1 = ((r.A >> 4) & 0x0F)  * 10 + (0x0F & r.A);
        uint8_t num2 = ((data >> 4) & 0x0F) * 10 + (0x0F & data);
        int16_t local_result = num1 - num2;

        if (local_result < 0)
        {
            local_result *= -1;
        }

        result |= local_result / 10 << 4;
        result |= local_result % 10;

        r.set_status_register_flag(Registers::CARRY_FLAG, num1 >= num2);
        r.A = result & 0x00FF;
    }
    else
    {
        int8_t carry = r.is_status_register_flag_set(Registers::CARRY_FLAG) ? 1 : 0;
        int8_t result = static_cast<uint8_t>(r.A) - static_cast<uint8_t>(data) - (1 - carry);
        int16_t overflow_check = static_cast<int16_t>(static_cast<int8_t>(r.A)) - static_cast<int16_t>(static_cast<int8_t>(data)) - (1 - carry);

        r.set_status_register_flag(Registers::CARRY_FLAG, ((static_cast<uint16_t>(data) & 0x00FF) + (1 - carry)) <= r.A);
        r.A = result & 0x00FF;

        r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.A & 0x80);
        r.set_status_register_flag(Registers::ZERO_FLAG, r.A == 0);
        r.set_status_register_flag(Registers::OVERFLOW_FLAG, result != overflow_check);
    }

    return extra_cycles_used;
}

static uint8_t SEC(const Instruction& i, Registers& r, AddressBus& m)
{
    // 1 -> C
    // N   Z   C   I   D   V
    // -   -   1   -   -   -

    r.set_status_register_flag(Registers::CARRY_FLAG, true);

    return 0;
}

static uint8_t SED(const Instruction& i, Registers& r, AddressBus& m)
{
    // 1 -> D
    // N   Z   C   I   D   V
    // -   -   -   -   1   -

    r.set_status_register_flag(Registers::DECIMAL_FLAG, true);

    return 0;
}

static uint8_t SEI(const Instruction& i, Registers& r, AddressBus& m)
{
    // 1 -> I
    // N   Z   C   I   D   V
    // -   -   -   1   -   -
    r.set_status_register_flag(Registers::INTERRUPT_DISABLE_FLAG, true);

    return 0;
}

static uint8_t STA(const Instruction& i, Registers& r, AddressBus& m)
{
    // A -> M
    // N    Z   C   I   D   V
    // -    -   -   -   -   -

    m[i.address()] = r.A;

    return 0;
}

static uint8_t STX(const Instruction& i, Registers& r, AddressBus& m)
{
    // X -> M
    // N   Z   C   I   D   V
    // -   -   -   -   -   -

    m[i.address()] = r.X;

    return 0;
}

static uint8_t STY(const Instruction& i, Registers& r, AddressBus& m)
{
    // X -> M
    // N   Z   C   I   D   V
    // -   -   -   -   -   -

    m[i.address()] = r.Y;

    return 0;
}

static uint8_t TAX(const Instruction& i, Registers& r, AddressBus& m)
{
    // A -> X
    // N   Z   C   I   D   V
    // +   +   -   -   -   -

    r.X = r.A;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.X == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.X & 0x80);

    return 0;
}

static uint8_t TAY(const Instruction& i, Registers& r, AddressBus& m)
{
    // A -> Y
    // N   Z   C   I   D   V
    // +   +   -   -   -   -

    r.Y = r.A;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.Y == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.Y & 0x80);

    return 0;
}

static uint8_t TXA(const Instruction& i, Registers& r, AddressBus& m)
{
    // X -> A
    // N   Z   C   I   D   V
    // +   +   -   -   -   -

    r.A = r.X;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.A == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.A & 0x80);

    return 0;
}

static uint8_t TSX(const Instruction& i, Registers& r, AddressBus& m)
{
    // N   Z   C   I   D   V
    // +   +   -   -   -   -
    r.X = r.SP;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.X == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.X & 0x80);

    return 0;
}

static uint8_t TXS(const Instruction& i, Registers& r, AddressBus& m)
{
    r.SP = r.X;
    return 0;
}

static uint8_t TYA(const Instruction& i, Registers& r, AddressBus& m)
{
    // Y -> A
    // N   Z   C   I   D   V
    // +   +   -   -   -   -

    r.A = r.Y;
    r.set_status_register_flag(Registers::ZERO_FLAG, r.Y == 0);
    r.set_status_register_flag(Registers::NEGATIVE_FLAG, r.Y & 0x80);

    return 0;
}

static std::array INSTRUCTION_DETAILS = std::to_array<InstructionDetails>(
{
    // source: https://www.masswerk.at/6502/6502_instruction_set.html#TSX
    //
    // cycles - Indicates the minimum number of cycles used by this instructions.
    //          Some instructions in some addressing modes can take additional cycles if memory
    //          access cross a page boundary.

    // assembler        opcode  bytes   cycles  addressing                          handler
    { "ADC #oper",      0x69,   2,      2,      AddressingMode::IMMEDIATE,          ADC },
    { "ADC oper",       0x65,   2,      3,      AddressingMode::ZERO_PAGE,          ADC },
    { "ADC oper,X",     0x75,   2,      4,      AddressingMode::ZERO_PAGE_X,        ADC },
    { "ADC oper",       0x6D,   3,      4,      AddressingMode::ABSOLUTE,           ADC },
    { "ADC oper,X",     0x7D,   3,      4,      AddressingMode::ABSOLUTE_X,         ADC },
    { "ADC oper,Y",     0x79,   3,      4,      AddressingMode::ABSOLUTE_Y,         ADC }, 
    { "ADC (oper,X)",   0x61,   2,      6,      AddressingMode::INDIRECT_X,         ADC },
    { "ADC (oper),Y",   0x71,   2,      5,      AddressingMode::INDIRECT_Y,         ADC },
    { "AND #oper",      0x29,   2,      2,      AddressingMode::IMMEDIATE,          AND },
    { "AND oper",       0x25,   2,      3,      AddressingMode::ZERO_PAGE,          AND },
    { "AND oper,X",     0x35,   2,      4,      AddressingMode::ZERO_PAGE_X,        AND },
    { "AND oper",       0x2D,   3,      4,      AddressingMode::ABSOLUTE,           AND },
    { "AND oper,X",     0x3D,   3,      4,      AddressingMode::ABSOLUTE_X,         AND },
    { "AND oper,Y",     0x39,   3,      4,      AddressingMode::ABSOLUTE_Y,         AND },
    { "AND (oper,X)",   0x21,   2,      6,      AddressingMode::INDIRECT_X,         AND },
    { "AND (oper),Y",   0x31,   2,      5,      AddressingMode::INDIRECT_Y,         AND },
    { "ASL A",          0x0A,   1,      2,      AddressingMode::ACCUMULATOR,        ASL },
    { "ASL oper",       0x06,   2,      5,      AddressingMode::ZERO_PAGE,          ASL },
    { "ASL oper,X",     0x16,   2,      6,      AddressingMode::ZERO_PAGE_X,        ASL },
    { "ASL oper",       0x0E,   3,      6,      AddressingMode::ABSOLUTE,           ASL },
    { "ASL oper,X",     0x1E,   3,      7,      AddressingMode::ABSOLUTE_X,         ASL },
    { "BCC oper",       0x90,   2,      2,      AddressingMode::RELATIVE,           BCC },
    { "BCS oper",       0xB0,   2,      2,      AddressingMode::RELATIVE,           BCS },
    { "BEQ oper",       0xF0,   2,      2,      AddressingMode::RELATIVE,           BEQ },
    { "BIT oper",       0x24,   2,      3,      AddressingMode::ZERO_PAGE,          BIT },
    { "BIT oper",       0x2C,   3,      4,      AddressingMode::ABSOLUTE,           BIT },
    { "BMI oper",       0x30,   2,      2,      AddressingMode::RELATIVE,           BMI },
    { "BNE oper",       0xD0,   2,      2,      AddressingMode::RELATIVE,           BNE },
    { "BPL oper",       0x10,   2,      2,      AddressingMode::RELATIVE,           BPL },
    { "BRK",            0x00,   1,      7,      AddressingMode::IMPLIED,            BRK },
    { "BVC oper",       0x50,   2,      2,      AddressingMode::RELATIVE,           BVC },
    { "BVS oper",       0x70,   2,      2,      AddressingMode::RELATIVE,           BVS },
    { "CLC",            0x18,   1,      2,      AddressingMode::IMPLIED,            CLC },
    { "CLD",            0xD8,   1,      2,      AddressingMode::IMPLIED,            CLD },
    { "CLI",            0x58,   1,      2,      AddressingMode::IMPLIED,            CLI },
    { "CLV",            0xB8,   1,      2,      AddressingMode::IMPLIED,            CLV },
    { "CMP #oper",      0xC9,   2,      2,      AddressingMode::IMMEDIATE,          CMP },
    { "CMP oper",       0xC5,   2,      3,      AddressingMode::ZERO_PAGE,          CMP },
    { "CMP oper,X",     0xD5,   2,      4,      AddressingMode::ZERO_PAGE_X,        CMP },
    { "CMP oper",       0xCD,   3,      4,      AddressingMode::ABSOLUTE,           CMP },
    { "CMP oper,X",     0xDD,   3,      4,      AddressingMode::ABSOLUTE_X,         CMP },
    { "CMP oper,Y",     0xD9,   3,      4,      AddressingMode::ABSOLUTE_Y,         CMP },
    { "CMP (oper,X)",   0xC1,   2,      6,      AddressingMode::INDIRECT_X,         CMP },
    { "CMP (oper),Y",   0xD1,   2,      5,      AddressingMode::INDIRECT_Y,         CMP },
    { "CPX #oper",      0xE0,   2,      2,      AddressingMode::IMMEDIATE,          CPX },
    { "CPX oper",       0xE4,   2,      3,      AddressingMode::ZERO_PAGE,          CPX },
    { "CPX oper",       0xEC,   3,      4,      AddressingMode::ABSOLUTE,           CPX },
    { "CPY #oper",      0xC0,   2,      2,      AddressingMode::IMMEDIATE,          CPY },
    { "CPY oper",       0xC4,   2,      3,      AddressingMode::ZERO_PAGE,          CPY }, 
    { "CPY oper",       0xCC,   3,      4,      AddressingMode::ABSOLUTE,           CPY },
    { "DEC oper",       0xC6,   2,      5,      AddressingMode::ZERO_PAGE,          DEC },
    { "DEC oper,X",     0xD6,   2,      6,      AddressingMode::ZERO_PAGE_X,        DEC },
    { "DEC oper",       0xCE,   3,      6,      AddressingMode::ABSOLUTE,           DEC },
    { "DEC oper,X",     0xDE,   3,      7,      AddressingMode::ABSOLUTE_X,         DEC },
    { "DEX",            0xCA,   1,      2,      AddressingMode::IMPLIED,            DEX },
    { "DEY",            0x88,   1,      2,      AddressingMode::IMPLIED,            DEY },
    { "EOR #oper",      0x49,   2,      2,      AddressingMode::IMMEDIATE,          EOR },
    { "EOR oper",       0x45,   2,      3,      AddressingMode::ZERO_PAGE,          EOR },
    { "EOR oper,X",     0x55,   2,      4,      AddressingMode::ZERO_PAGE_X,        EOR },
    { "EOR oper",       0x4D,   3,      4,      AddressingMode::ABSOLUTE,           EOR },
    { "EOR oper,X",     0x5D,   3,      4,      AddressingMode::ABSOLUTE_X,         EOR },
    { "EOR oper,Y",     0x59,   3,      4,      AddressingMode::ABSOLUTE_Y,         EOR },
    { "EOR (oper,X)",   0x41,   2,      6,      AddressingMode::INDIRECT_X,         EOR },
    { "EOR (oper),Y",   0x51,   2,      5,      AddressingMode::INDIRECT_Y,         EOR },
    { "INC oper",       0xE6,   2,      5,      AddressingMode::ZERO_PAGE,          INC },
    { "INC oper,X",     0xF6,   2,      6,      AddressingMode::ZERO_PAGE_X,        INC },
    { "INC oper",       0xEE,   3,      6,      AddressingMode::ABSOLUTE,           INC },
    { "INC oper,X",     0xFE,   3,      7,      AddressingMode::ABSOLUTE_X,         INC },
    { "INX",            0xE8,   1,      2,      AddressingMode::IMPLIED,            INX },
    { "INY",            0xC8,   1,      2,      AddressingMode::IMPLIED,            INY },
    { "JMP oper",       0x4C,   3,      3,      AddressingMode::ABSOLUTE,           JMP },
    { "JMP (oper)",     0x6C,   3,      5,      AddressingMode::INDIRECT,           JMP },
    { "JSR oper",       0x20,   3,      6,      AddressingMode::ABSOLUTE,           JSR },
    { "LDA #oper",      0xA9,   2,      2,      AddressingMode::IMMEDIATE,          LDA },
    { "LDA oper",       0xA5,   2,      3,      AddressingMode::ZERO_PAGE,          LDA },
    { "LDA oper,X",     0xB5,   2,      4,      AddressingMode::ZERO_PAGE_X,        LDA },
    { "LDA oper",       0xAD,   3,      4,      AddressingMode::ABSOLUTE,           LDA },
    { "LDA oper,X",     0xBD,   3,      4,      AddressingMode::ABSOLUTE_X,         LDA },
    { "LDA oper,Y",     0xB9,   3,      4,      AddressingMode::ABSOLUTE_Y,         LDA },
    { "LDA (oper,X)",   0xA1,   2,      6,      AddressingMode::INDIRECT_X,         LDA },
    { "LDA (oper),Y",   0xB1,   2,      5,      AddressingMode::INDIRECT_Y,         LDA },
    { "LDX #oper",      0xA2,   2,      2,      AddressingMode::IMMEDIATE,          LDX },
    { "LDX oper",       0xA6,   2,      3,      AddressingMode::ZERO_PAGE,          LDX },
    { "LDX oper,Y",     0xB6,   2,      4,      AddressingMode::ZERO_PAGE_Y,        LDX },
    { "LDX oper",       0xAE,   3,      4,      AddressingMode::ABSOLUTE,           LDX },
    { "LDX oper,Y",     0xBE,   3,      4,      AddressingMode::ABSOLUTE_Y,         LDX },
    { "LDY #oper",      0xA0,   2,      2,      AddressingMode::IMMEDIATE,          LDY },
    { "LDY oper",       0xA4,   2,      3,      AddressingMode::ZERO_PAGE,          LDY },
    { "LDY oper,X",     0xB4,   2,      4,      AddressingMode::ZERO_PAGE_X,        LDY },
    { "LDY oper",       0xAC,   3,      4,      AddressingMode::ABSOLUTE,           LDY },
    { "LDY oper,X",     0xBC,   3,      4,      AddressingMode::ABSOLUTE_X,         LDY },
    { "LSR A",          0x4A,   1,      2,      AddressingMode::ACCUMULATOR,        LSR },
    { "LSR oper",       0x46,   2,      5,      AddressingMode::ZERO_PAGE,          LSR },
    { "LSR oper,X",     0x56,   2,      6,      AddressingMode::ZERO_PAGE_X,        LSR },
    { "LSR oper",       0x4E,   3,      6,      AddressingMode::ABSOLUTE,           LSR },
    { "LSR oper,X",     0x5E,   3,      7,      AddressingMode::ABSOLUTE_X,         LSR },
    { "NOP",            0xEA,   1,      2,      AddressingMode::IMPLIED,            NOP },
    { "ORA #oper",      0x09,   2,      2,      AddressingMode::IMMEDIATE,          ORA },
    { "ORA oper",       0x05,   2,      3,      AddressingMode::ZERO_PAGE,          ORA },
    { "ORA oper,X",     0x15,   2,      4,      AddressingMode::ZERO_PAGE_X,        ORA },
    { "ORA oper",       0x0D,   3,      4,      AddressingMode::ABSOLUTE,           ORA },
    { "ORA oper,X",     0x1D,   3,      4,      AddressingMode::ABSOLUTE_X,         ORA },
    { "ORA oper,Y",     0x19,   3,      4,      AddressingMode::ABSOLUTE_Y,         ORA },
    { "ORA (oper,X)",   0x01,   2,      6,      AddressingMode::INDIRECT_X,         ORA },
    { "ORA (oper),Y",   0x11,   2,      5,      AddressingMode::INDIRECT_Y,         ORA },
    { "PHA",            0x48,   1,      3,      AddressingMode::IMPLIED,            PHA },
    { "PHP",            0x08,   1,      3,      AddressingMode::IMPLIED,            PHP },
    { "PLA",            0x68,   1,      4,      AddressingMode::IMPLIED,            PLA },
    { "PLP",            0x28,   1,      4,      AddressingMode::IMPLIED,            PLP },
    { "ROL A",          0x2A,   1,      2,      AddressingMode::ACCUMULATOR,        ROL },
    { "ROL oper",       0x26,   2,      5,      AddressingMode::ZERO_PAGE,          ROL },
    { "ROL oper,X",     0x36,   2,      6,      AddressingMode::ZERO_PAGE_X,        ROL },
    { "ROL oper",       0x2E,   3,      6,      AddressingMode::ABSOLUTE,           ROL },
    { "ROL oper,X",     0x3E,   3,      7,      AddressingMode::ABSOLUTE_X,         ROL },
    { "ROR A",          0x6A,   1,      2,      AddressingMode::ACCUMULATOR,        ROR },
    { "ROR oper",       0x66,   2,      5,      AddressingMode::ZERO_PAGE,          ROR },
    { "ROR oper,X",     0x76,   2,      6,      AddressingMode::ZERO_PAGE_X,        ROR },
    { "ROR oper",       0x6E,   3,      6,      AddressingMode::ABSOLUTE,           ROR },
    { "ROR oper,X",     0x7E,   3,      7,      AddressingMode::ABSOLUTE_X,         ROR },
    { "RTI",            0x40,   1,      6,      AddressingMode::IMPLIED,            RTI },
    { "RTS oper",       0x60,   1,      6,      AddressingMode::IMPLIED,            RTS },
    { "SBC #oper",      0xE9,   2,      2,      AddressingMode::IMMEDIATE,          SBC },
    { "SBC oper",       0xE5,   2,      3,      AddressingMode::ZERO_PAGE,          SBC },
    { "SBC oper,X",     0xF5,   2,      4,      AddressingMode::ZERO_PAGE_X,        SBC },
    { "SBC oper",       0xED,   3,      4,      AddressingMode::ABSOLUTE,           SBC },
    { "SBC oper,X",     0xFD,   3,      4,      AddressingMode::ABSOLUTE_X,         SBC },
    { "SBC oper,Y",     0xF9,   3,      4,      AddressingMode::ABSOLUTE_Y,         SBC },
    { "SBC (oper,X)",   0xE1,   2,      6,      AddressingMode::INDIRECT_X,         SBC },
    { "SBC (oper),Y",   0xF1,   2,      5,      AddressingMode::INDIRECT_Y,         SBC },
    { "SEC",            0x38,   1,      2,      AddressingMode::IMPLIED,            SEC },
    { "SED",            0xF8,   1,      2,      AddressingMode::IMPLIED,            SED },
    { "SEI",            0x78,   1,      2,      AddressingMode::IMPLIED,            SEI },
    { "STA oper",       0x85,   2,      3,      AddressingMode::ZERO_PAGE,          STA },
    { "STA oper,X",     0x95,   2,      4,      AddressingMode::ZERO_PAGE_X,        STA },
    { "STA oper",       0x8D,   3,      4,      AddressingMode::ABSOLUTE,           STA },
    { "STA oper,X",     0x9D,   3,      5,      AddressingMode::ABSOLUTE_X,         STA },
    { "STA oper,Y",     0x99,   3,      5,      AddressingMode::ABSOLUTE_Y,         STA },
    { "STA (oper,X,)",  0x81,   2,      6,      AddressingMode::INDIRECT_X,         STA },
    { "STA (oper),Y",   0x91,   2,      6,      AddressingMode::INDIRECT_Y,         STA },
    { "STX oper",       0x86,   2,      3,      AddressingMode::ZERO_PAGE,          STX },
    { "STX oper,Y",     0x96,   2,      4,      AddressingMode::ZERO_PAGE_Y,        STX },
    { "STX oper",       0x8E,   3,      4,      AddressingMode::ABSOLUTE,           STX },
    { "STY oper",       0x84,   2,      3,      AddressingMode::ZERO_PAGE,          STY },
    { "STY oper,X",     0x94,   2,      4,      AddressingMode::ZERO_PAGE_X,        STY },
    { "STY oper",       0x8C,   3,      4,      AddressingMode::ABSOLUTE,           STY },
    { "TAX",            0xAA,   1,      2,      AddressingMode::IMPLIED,            TAX },
    { "TAY",            0xA8,   1,      2,      AddressingMode::IMPLIED,            TAY },
    { "TSX",            0xBA,   1,      2,      AddressingMode::IMPLIED,            TSX },
    { "TXA",            0x8A,   1,      2,      AddressingMode::IMPLIED,            TXA },
    { "TXS",            0x9A,   1,      2,      AddressingMode::IMPLIED,            TXS },
    { "TYA",            0x98,   1,      2,      AddressingMode::IMPLIED,            TYA },
});

std::array<InstructionDetails, 0xFF> make_instruction_table()
{
    std::array<InstructionDetails, 0xFF> instr_table;

    instr_table.fill({"", 0x00, 0, 0, AddressingMode::INVALID, BRK});
    
    for (auto& i : INSTRUCTION_DETAILS)
    {
        instr_table[i.opcode] = i;
    }
    return instr_table;
}
