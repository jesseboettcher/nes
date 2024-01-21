#include "processor/processor_6502.hpp"

#include <cassert>

#include <glog/logging.h>

void test_data_bus()
{
	Processor6502 p;
	const Registers& r = p.cregisters();

	// A -> 98
	assert(r.A == 0x00);
	p.execute_instruction(p.assemble_instruction("LDA #0x98"));
	assert(r.A == 0x98);

	assert(p.cmemory()[0xC002] == 0x0);
	p.execute_instruction(p.assemble_instruction("STA 0xC002"));
	assert(p.cmemory()[0xC002] == 0x98);

	assert(p.cmemory()[0x0002] == 0x0);
	p.execute_instruction(p.assemble_instruction("STA 0x02"));
	assert(p.cmemory()[0x0002] == 0x98);

	assert(r.X == 0x00);
	p.execute_instruction(p.assemble_instruction("LDX #0x05"));
	assert(r.X == 0x05);

	assert(p.cmemory()[0x0007] == 0x0);
	p.execute_instruction(p.assemble_instruction("STA 0x02,X"));
	assert(p.cmemory()[0x0007] == 0x98);

	assert(p.cmemory()[0xC007] == 0x0);
	p.execute_instruction(p.assemble_instruction("STA 0xC002,X"));
	assert(p.cmemory()[0xC007] == 0x98);

	// A -> 45
	p.execute_instruction(p.assemble_instruction("LDA #0x45"));
	assert(r.A == 0x45);

	assert(r.Y == 0x00);
	p.execute_instruction(p.assemble_instruction("LDY #0x06"));
	assert(r.Y == 0x06);

	assert(p.cmemory()[0xC008] == 0x0);
	p.execute_instruction(p.assemble_instruction("STA 0xC002,Y"));
	assert(p.cmemory()[0xC008] == 0x45);

	// TODO
	// { "STA (oper,X,)",	0x81,	2, 		6,		AddressingMode::INDIRECT_X, 		STA },
	// { "STA (oper),Y",	0x91,	2, 		6,		AddressingMode::INDIRECT_Y, 		STA },
	// { "LDA #oper",		0xA9,	2,		2,		AddressingMode::IMMEDIATE,			LDA },
	// { "LDA oper",		0xA5,	2,		3,		AddressingMode::ZERO_PAGE, 			LDA },
	// { "LDA oper,X",		0xB5,	2,		4,		AddressingMode::ZERO_PAGE_X,		LDA },
	// { "LDA oper",		0xAD,	3,		4,		AddressingMode::ABSOLUTE, 			LDA },
	// { "LDA oper,X",		0xBD,	3,		4,		AddressingMode::ABSOLUTE_X, 		LDA },
	// { "LDA oper,Y",		0xB9,	3,		4,		AddressingMode::ABSOLUTE_Y, 		LDA },
	// { "LDA (oper,X)",	0xA1,	2,		6,		AddressingMode::INDIRECT_X, 		LDA },
	// { "LDA (oper),Y",	0xB1,	2,		5,		AddressingMode::INDIRECT_Y, 		LDA },
}

void test_arithmetic()
{
	Processor6502 p;
	const Registers& r = p.cregisters();

	// simple adds
	p.execute_instruction(p.assemble_instruction("LDA #0x1"));
	assert(r.A == 0x01);
	p.execute_instruction(p.assemble_instruction("ADC #0x3"));
	assert(r.A == 0x04);
	p.execute_instruction(p.assemble_instruction("ADC #0x4"));
	assert(r.A == 0x08);
	p.execute_instruction(p.assemble_instruction("ADC #0xF8"));
	assert(r.A == 0x00);
	assert(r.is_status_register_set(Registers::ZERO_FLAG));

	// check carry flag
	p.execute_instruction(p.assemble_instruction("CLC"));
	p.execute_instruction(p.assemble_instruction("LDA #13"));
	assert(r.A == 0x0D);
	p.execute_instruction(p.assemble_instruction("ADC #211"));
	assert(r.A == 224);
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));

	p.execute_instruction(p.assemble_instruction("LDA #254"));
	assert(r.A == 0xFE);
	p.execute_instruction(p.assemble_instruction("ADC #6"));
	assert(r.A == 4);
	assert(r.is_status_register_set(Registers::CARRY_FLAG));

	p.execute_instruction(p.assemble_instruction("CLC"));
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));

	// 16-bit addition, results stored at 0x4000
	// 0x0102 stored in A
	// 0x1010 stored at 0x0030 (lo), 0x0031 (hi)

	p.execute_instruction(p.assemble_instruction("LDA #0x10"));
	p.execute_instruction(p.assemble_instruction("STA 0x30")); // store num2 lo
	p.execute_instruction(p.assemble_instruction("STA 0x31")); // store num2 hi

	p.execute_instruction(p.assemble_instruction("LDA #0x02")); // num1 lo
	p.execute_instruction(p.assemble_instruction("ADC 0x30"));
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));
	p.execute_instruction(p.assemble_instruction("STA 0x4000"));
	p.execute_instruction(p.assemble_instruction("LDA #0x01")); // num1 hi
	p.execute_instruction(p.assemble_instruction("ADC 0x31"));
	p.execute_instruction(p.assemble_instruction("STA 0x4001"));
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));
	assert(p.cmemory()[0x4000] == 0x12); // res lo
	assert(p.cmemory()[0x4001] == 0x11); // res hi

	// 16-bit addition, results stored at 0x4000
	// 0x0180 stored in A
	// 0x0080 stored at 0x0030 (lo), 0x0031 (hi)

	p.execute_instruction(p.assemble_instruction("LDA #0x80"));
	p.execute_instruction(p.assemble_instruction("STA 0x30")); // store num2 lo
	p.execute_instruction(p.assemble_instruction("LDA #0x00"));
	p.execute_instruction(p.assemble_instruction("STA 0x31")); // store num2 hi

	p.execute_instruction(p.assemble_instruction("LDA #0x80")); // num1 lo
	p.execute_instruction(p.assemble_instruction("ADC 0x30"));
	assert(r.is_status_register_set(Registers::CARRY_FLAG));
	p.execute_instruction(p.assemble_instruction("STA 0x4000"));
	p.execute_instruction(p.assemble_instruction("LDA #0x01")); // num1 hi
	p.execute_instruction(p.assemble_instruction("ADC 0x31"));
	p.execute_instruction(p.assemble_instruction("STA 0x4001"));
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));
	assert(p.cmemory()[0x4000] == 0x00); // res lo
	assert(p.cmemory()[0x4001] == 0x02); // res hi

	// check overflow flag
	p.execute_instruction(p.assemble_instruction("LDA #127"));
	p.execute_instruction(p.assemble_instruction("STA 0x30"));
	p.execute_instruction(p.assemble_instruction("LDA #2"));
	p.execute_instruction(p.assemble_instruction("ADC 0x30"));
	assert(static_cast<int8_t>(r.A) == -127);
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));
	assert(r.is_status_register_set(Registers::OVERFLOW_FLAG));

	// decimal mode
	p.execute_instruction(p.assemble_instruction("CLC"));
	p.execute_instruction(p.assemble_instruction("SED"));
	p.execute_instruction(p.assemble_instruction("LDA #0x79"));
	p.execute_instruction(p.assemble_instruction("ADC #0x14"));
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));
	assert(r.A == 0x93);
	p.execute_instruction(p.assemble_instruction("CLD"));
	assert(!r.is_status_register_set(Registers::DECIMAL_FLAG));

	// subtract
	// 5 - 3 = 2
	p.execute_instruction(p.assemble_instruction("SEC"));
	p.execute_instruction(p.assemble_instruction("LDA #3"));
	p.execute_instruction(p.assemble_instruction("STA 0x30"));
	p.execute_instruction(p.assemble_instruction("LDA #5"));
	p.execute_instruction(p.assemble_instruction("SBC 0x30"));
	assert(r.A == 2);
	assert(r.is_status_register_set(Registers::CARRY_FLAG));

	// 5 - 6 = -1
	p.execute_instruction(p.assemble_instruction("SEC"));
	p.execute_instruction(p.assemble_instruction("LDA #6"));
	p.execute_instruction(p.assemble_instruction("STA 0x30"));
	p.execute_instruction(p.assemble_instruction("LDA #5"));
	p.execute_instruction(p.assemble_instruction("SBC 0x30"));
	assert(r.A == 0xFF); // -1
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));

	// 44 - 29 = 15 (decimal mode)
	p.execute_instruction(p.assemble_instruction("SEC"));
	p.execute_instruction(p.assemble_instruction("SED"));
	p.execute_instruction(p.assemble_instruction("LDA #0x29"));
	p.execute_instruction(p.assemble_instruction("STA 0x30"));
	p.execute_instruction(p.assemble_instruction("LDA #0x44"));
	p.execute_instruction(p.assemble_instruction("SBC 0x30"));
	assert(r.A == 0x15); // decimal 15
	assert(r.is_status_register_set(Registers::CARRY_FLAG));

	// 30 - 50 = -20 (decimal mode)
	p.execute_instruction(p.assemble_instruction("SEC"));
	p.execute_instruction(p.assemble_instruction("SED"));
	p.execute_instruction(p.assemble_instruction("LDA #0x50"));
	p.execute_instruction(p.assemble_instruction("STA 0x30"));
	p.execute_instruction(p.assemble_instruction("LDA #0x30"));
	p.execute_instruction(p.assemble_instruction("SBC 0x30"));
	assert(r.A == 0x20); // decimal 20 (negative discarded, but indicated by carry flag)
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));

	// AND
	p.execute_instruction(p.assemble_instruction("LDA #0xF3"));
	p.execute_instruction(p.assemble_instruction("STA 0x30"));
	p.execute_instruction(p.assemble_instruction("LDA #0x1F"));
	p.execute_instruction(p.assemble_instruction("AND 0x30"));
	assert(r.A == 0x13);
	p.execute_instruction(p.assemble_instruction("LDA #0x08"));
	p.execute_instruction(p.assemble_instruction("AND 0x30"));
	assert(r.A == 0x0);
	assert(r.is_status_register_set(Registers::ZERO_FLAG));
	p.execute_instruction(p.assemble_instruction("LDA #0x88"));
	p.execute_instruction(p.assemble_instruction("AND 0x30"));
	assert(r.A == 0x80);
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));
	assert(r.is_status_register_set(Registers::NEGATIVE_FLAG));

	// ORA
	p.execute_instruction(p.assemble_instruction("LDA #0xF3"));
	p.execute_instruction(p.assemble_instruction("ORA #0x0C"));
	assert(r.A == 0xFF);
	assert(r.is_status_register_set(Registers::NEGATIVE_FLAG));
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));

	// EOR
	p.execute_instruction(p.assemble_instruction("LDA #0xF3"));
	p.execute_instruction(p.assemble_instruction("EOR #0x0F"));
	assert(r.A == 0xFC);
	assert(r.is_status_register_set(Registers::NEGATIVE_FLAG));
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));

	// INC
	p.execute_instruction(p.assemble_instruction("INX"));
	assert(r.X == 0x01);

	assert(r.Y == 0x00);
	p.execute_instruction(p.assemble_instruction("INY"));
	assert(r.Y == 0x01);
}

void test_shift_memory_modify()
{
	Processor6502 p;
	const Registers& r = p.cregisters();

	// LEFT shift ops on accumulator
	p.execute_instruction(p.assemble_instruction("LDA #0x81"));
	p.execute_instruction(p.assemble_instruction("LSR A"));
	assert(r.A == 0x40);
	assert(r.is_status_register_set(Registers::CARRY_FLAG));
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));
	p.execute_instruction(p.assemble_instruction("LSR A"));
	assert(r.A == 0x20);
	assert(!r.is_status_register_set(Registers::CARRY_FLAG));
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));
	p.execute_instruction(p.assemble_instruction("LDA #0x01"));
	p.execute_instruction(p.assemble_instruction("LSR A"));
	assert(r.is_status_register_set(Registers::CARRY_FLAG));
	assert(r.is_status_register_set(Registers::ZERO_FLAG));

	// shift ops in memory
	p.execute_instruction(p.assemble_instruction("LDA #0x02"));
	p.execute_instruction(p.assemble_instruction("STA 0x30"));
	assert(p.cmemory()[0x30] == 0x02);
	p.execute_instruction(p.assemble_instruction("LSR 0x30"));
	assert(p.cmemory()[0x30] == 0x01);

	// RIGHT shift ops on accumulator
	p.execute_instruction(p.assemble_instruction("LDA #0x81"));
	p.execute_instruction(p.assemble_instruction("ASL A"));
	assert(r.A == 0x02);
	assert(r.is_status_register_set(Registers::CARRY_FLAG));
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));

	// LEFT rotate ops on accumulator
	p.execute_instruction(p.assemble_instruction("LDA #0x81"));
	p.execute_instruction(p.assemble_instruction("ROL A"));
	assert(r.A == 0x03);
	assert(r.is_status_register_set(Registers::CARRY_FLAG));
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));

	// RIGHT rotate ops on accumulator
	p.execute_instruction(p.assemble_instruction("LDA #0x81"));
	p.execute_instruction(p.assemble_instruction("ROR A"));
	assert(r.A == 0xC0);
	assert(r.is_status_register_set(Registers::CARRY_FLAG));
	assert(!r.is_status_register_set(Registers::ZERO_FLAG));
}

// BIT
// Branch instructions
// Compare instructions

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	FLAGS_logtostderr = 1;
	FLAGS_minloglevel = google::INFO;

	test_data_bus();
	test_arithmetic();
	test_shift_memory_modify();
	LOG(INFO) << "success";
}
