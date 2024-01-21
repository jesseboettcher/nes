#include "io/cartridge_interface.hpp"
#include "io/files.hpp"
#include "processor/processor_6502.hpp"

#include <glog/logging.h>

void test_nes_file_parser()
{
	std::shared_ptr<MappedFile> maybe_file;

	// Load input file
	maybe_file = MappedFile::open("/Users/jesse/code/nes/test/nes-tutorial.nes");
	assert(maybe_file);

	NesFileParser parsed_file(maybe_file->buffer());
	assert(parsed_file.valid());

	assert(parsed_file.num_prg_rom_units() == 1);
	assert(parsed_file.sizeof_prg_rom() == 16 * 1024);
	assert(parsed_file.num_chr_rom_units() == 1);
	assert(parsed_file.sizeof_chr_rom() == 8 * 1024);
}

void test_cartidge_interface()
{
	std::shared_ptr<MappedFile> maybe_file;

	// Load input file
	maybe_file = MappedFile::open("/Users/jesse/code/nes/test/nes-tutorial.nes");
	assert(maybe_file);

	NesFileParser parsed_file(maybe_file->buffer());
	assert(parsed_file.valid());

	Processor6502 processor;
	CartridgeInterface::load(processor, parsed_file);
	LOG(ERROR) << std::hex << +processor.cmemory()[0xFFFD];

	// reset vector
	processor.print_memory(0xFFFC, 2);
	assert(processor.cmemory()[0xFFFC] == 0x00);
	assert(processor.cmemory()[0xFFFD] == 0xC0);

	// target instructions from reset vector
	processor.print_memory(0xC000, 3);
	assert(processor.cmemory()[0xC000] == 0x20);	// JSR
	assert(processor.cmemory()[0xC001] == 0x27);	// PCL
	assert(processor.cmemory()[0xC002] == 0xC0);	// PCH
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	FLAGS_logtostderr = 1;
	FLAGS_minloglevel = google::INFO;

	test_nes_file_parser();
	test_cartidge_interface();
	LOG(INFO) << "success";
}
