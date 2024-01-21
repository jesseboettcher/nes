#include "external/json.hpp"
#include "processor/processor_6502.hpp"

#include <glog/logging.h>

#include <array>
#include <fstream>
#include <unordered_map>

using json = nlohmann::json;

bool run_one_test(const json& j)
{
	LOG(INFO) << j.at("name");
	// set initial state
	// run x# of steps
	// check final state
}

void run_6502_tests()
{
	Processor6502 processor;
	std::array<InstructionDetails, 0xFF> instr_table = make_instruction_table();
	std::set<uint8_t> tests_run;

	LOG(INFO) << "run_6502_tests";

	std::ifstream test_file("/home/jesse/code/ProcessorTests/nes6502/v1/a9.json");
	json test_json = json::parse(test_file);

	run_one_test(test_json.at(1));
}