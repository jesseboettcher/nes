#pragma once

#include "lib/json.hpp"
#include "processor/address_bus.hpp"
#include "processor/processor_6502.hpp"

#include <set>

class Test6502
{
public:
	// Runs TomHarte's ProcessorTests against Processor6502. They contain 10k randomly generated
	// tests for each instruction. Each instruction's test suite is contained in a json file that
	// specifies, for each test case, the state of registers and memory before and afterwards.
	// The test case is run for one instruction execution and then the contents of memory is compared
	// against the json.
	//
	// Clone the repo locally and update TESTS_PATH: https://github.com/TomHarte/ProcessorTests
	static constexpr std::string_view TESTS_PATH = "/Users/jesse/code/ProcessorTests/nes6502/v1/";

	Test6502()
	 {
	 	processor_ = std::make_shared<Processor6502>(address_bus_, nmi_signal_,
	 												 AddressBus::ADDRESSABLE_MEMORY_SIZE);
	 	address_bus_.attach_cpu(processor_);
	 }

	void run();

private:
	bool test_instruction(uint8_t instr);
	bool run_one_test(const nlohmann::json& j, std::string name);

	AddressBus address_bus_;
	std::shared_ptr<Processor6502> processor_;
	std::shared_ptr<NesPPU> ppu_;

	bool nmi_signal_{false};

    std::set<uint8_t> tests_run_;
    std::set<uint8_t> tests_skipped_;
};
