#include "io/files.hpp"
#include "io/prompt.hpp"
#include "system/nes.hpp"

#include <glog/logging.h>

void run_6502_tests();


int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	FLAGS_logtostderr = 1;
	FLAGS_minloglevel = google::INFO;

	std::unique_ptr<NesFileParser> cartridge;

    run_6502_tests();
    return 0;

	if (argc == 2)
	{
		cartridge = std::make_unique<NesFileParser>(argv[1]);
	}

	Nes nes(std::move(cartridge));

	while (true)
	{
		try
		{
			CommandPrompt::instance().launch_prompt(nes);
			// nes.run_continuous();
			break;
		}
		catch(...)
		{
			LOG(ERROR) << "Exception";
		}
	}

	return 0;
}
