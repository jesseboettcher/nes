#include "nes.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include <glog/logging.h>

Nes::Nes(std::unique_ptr<NesFileParser> cartridge)
: cartridge_(nullptr)
, processor_()
, display_()
, ppu_(processor_, display_)
{
	std::cout << "Launching Nes...\n";

    load_cartridge(std::move(cartridge));
	display_.init();
}

Nes::~Nes()
{

}

void Nes::load_cartridge(std::unique_ptr<NesFileParser> cartridge)
{
    user_interrupt();

    ppu_.reset();

    cartridge_ = std::move(cartridge);
    if (cartridge_)
    {
        CartridgeInterface::load(processor_, ppu_, *cartridge_);
    }
    processor_.reset();
    
//    processor_.breakpoint(0xE261);
}

void Nes::run_continuous()
{
    state_ = State::RUNNING;

    while (!should_exit_)
	{
		if (!step())
		{
			continue;
		}
        check_timer();
	}
    should_exit_ = false;
    state_ = State::IDLE;
}

void Nes::run()
{
    state_ = State::RUNNING;
    
    while (!should_exit_)
	{
		if (!step())
		{
			break;
		}
        check_timer();
//        if (processor_.cycle_count() == 1789774) // DEBUG rendering of game screen
//        {
//            LOG(ERROR) << "BREAK";
//        	break;
//        }
	}
    should_exit_ = false;
    state_ = State::IDLE;
}

bool Nes::step()
{
	bool should_continue = true;

	clock_ticks_ += 4;

	ppu_.step();

	if (clock_ticks_ % 12 == 0)
	{
		should_continue = processor_.step();
	}

	return should_continue;
}

void Nes::step_cpu_instruction()
{
	uint64_t prev_instr_count = processor_.instruction_count();
	bool should_continue = true;

	while (processor_.instruction_count() == prev_instr_count && should_continue)
	{
		should_continue = step();
	}
}

void Nes::user_interrupt()
{
    if (state_ == State::IDLE)
    {
        return;
    }
    
    // TODO FIX RACE
    should_exit_ = true;
    while (state_ == State::RUNNING)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Nes::check_timer()
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    static uint64_t last_cycle_count = 0;
    
    if (processor_.cycle_count() - last_cycle_count > 1789773)
    {
        auto mark = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> delta = mark - start_time;

        std::cout << (100.0 * 1000.0 / delta.count()) << std::dec << "% of realtime" << " cycle " << processor_.cycle_count() << std::endl;
        start_time = std::chrono::high_resolution_clock::now();
        last_cycle_count = processor_.cycle_count();
    }
}
