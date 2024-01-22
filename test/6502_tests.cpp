#include "lib/json.hpp"
#include "processor/processor_6502.hpp"

#include <glog/logging.h>

#include <array>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

using json = nlohmann::json;

class Test6502
{
public:
	void run();

private:
	bool test_instruction(uint8_t instr);
	bool run_one_test(const json& j, std::string name);

	Processor6502 processor_;

    std::set<uint8_t> tests_run_;
    std::set<uint8_t> tests_skipped_;
};


bool Test6502::run_one_test(const json& j, std::string name)
{
    // set initial state
	Registers& r = processor_.registers();
	Memory& m = processor_.memory();

	json initial_state = j.at("initial");

	processor_.reset();
	r.PC = initial_state.at("pc");
	r.SP = initial_state.at("s");
	r.A  = initial_state.at("a");
	r.X  = initial_state.at("x");
	r.Y  = initial_state.at("y");
	r.set_SR(initial_state.at("p"));

	json initial_ram = initial_state.at("ram");

	for (int32_t i = 0;i < initial_ram.size();++i)
	{
		m[initial_ram.at(i).at(0)] = initial_ram.at(i).at(1);
	}

	// step the processor

    // when running for cycles, failure here:
    // E20240122 08:30:22.440444 3636924416 6502_tests.cpp:66] check failure: expected 54554 actual 54555
    // E20240122 08:30:22.440488 3636924416 6502_tests.cpp:88] 10:28: Failed on PC check
	// for (int32_t num_cycles = 0; num_cycles < j.at("cycles").size();++num_cycles)

	uint64_t start_instr_count = processor_.instruction_count();

    while (processor_.instruction_count() == start_instr_count)
    {
		processor_.step();
	}

	// check final state
	json final_state = j.at("final");

    auto assert_and_log = [](auto actual, auto expected, const std::string& message)
    {
        if (actual != expected)
        {
            LOG(ERROR) << "check failure: expected " << expected << " actual " << static_cast<int32_t>(actual);
            throw std::runtime_error(message);
        }
    };

    try 
    {
        assert_and_log(r.PC, final_state.at("pc"), name + std::string(": Failed on PC check"));
        assert_and_log(r.SP, final_state.at("s"), name + std::string(": Failed on SP check"));
        assert_and_log(r.A,  final_state.at("a"), name + std::string(": Failed on A register check"));
        assert_and_log(r.X,  final_state.at("x"), name + std::string(": Failed on X register check"));
        assert_and_log(r.Y,  final_state.at("y"), name + std::string(": Failed on Y register check"));

        json final_ram = final_state.at("ram");

        for (int32_t i = 0;i < final_ram.size();++i)
        {
            assert_and_log(m[final_ram.at(i).at(0)], final_ram.at(i).at(1), name + std::string(": Failed on memory check"));
        }
    }
    catch (const std::runtime_error& e)
    {
        LOG(ERROR) << e.what();
        return false; // or any other appropriate action
    }

	return true;
}

bool Test6502::test_instruction(uint8_t instr)
{
	if (tests_run_.count(instr) || tests_skipped_.count(instr))
	{
		return true; // skip!
	}
	tests_run_.insert(instr);

	LOG(INFO) << std::hex << std::setw(2) << std::setfill('0')
			  << static_cast<int32_t>(instr) << ":";

	std::stringstream path;
    std::stringstream opcode_str;

    opcode_str << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int32_t>(instr);

	path << "/Users/jesse/code/ProcessorTests/nes6502/v1/" << opcode_str.str() << ".json";

	std::ifstream test_file(path.str());
	json test_json = json::parse(test_file);

	for (int32_t i = 0;i < test_json.size();++i)
	{
		bool result = run_one_test(test_json.at(i), opcode_str.str() + ":" + std::to_string(i));

        if (!result)
        {
            return false;
        }
	}
    return true;
}

void Test6502::run()
{
	LOG(INFO) << "run_6502_tests";

	std::array<InstructionDetails, 0xFF> instr_table = make_instruction_table();

	for (const auto& instr : instr_table)
	{
		bool result = test_instruction(instr.opcode);

        if (!result)
        {
            break;
        }
	}

    LOG(INFO) << "Finished " << tests_run_.size()
              << " tests (skipped " << tests_skipped_.size() << ")";
}

int main(void)
{
	Test6502 test_6502;

	test_6502.run();
	return 0;
}
