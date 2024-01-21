#include "io/cartridge_interface.hpp"
#include "io/files.hpp"
#include "processor/processor_6502.hpp"

#include <glog/logging.h>

void test_timings()
{
	LOG(ERROR) << "test_timings";
	std::shared_ptr<MappedFile> maybe_file;

	// Load input file
	maybe_file = MappedFile::open("/Users/jesse/code/nes/test/timing.rom");
	assert(maybe_file);

	Processor6502 processor;
	CartridgeInterface::load(processor, *maybe_file);
	LOG(ERROR) << std::hex << +processor.cmemory()[0xFFFD];

	processor.reset();

	processor.print_registers();

	// reset vector
	processor.print_memory(0xFFFC, 2);
	assert(processor.cmemory()[0xFFFC] == 0x00);
	assert(processor.cmemory()[0xFFFD] == 0xC0);

	processor.breakpoint(0xC26C);
	processor.run();
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	FLAGS_logtostderr = 1;
	FLAGS_minloglevel = google::INFO;

	test_timings();
	LOG(INFO) << "success";
}
