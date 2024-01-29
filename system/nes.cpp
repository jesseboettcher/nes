#include "nes.hpp"

#include "platform/ui_properties.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include <glog/logging.h>

Nes::Nes(std::unique_ptr<NesFileParser> cartridge)
: cartridge_(nullptr)
, processor_()
, display_()
, ppu_(processor_, display_)
, joypads_(processor_)
{
    std::cout << "Launching Nes...\n";

    load_cartridge(std::move(cartridge));
    display_.init();

    update_state(State::OFF);
}

Nes::~Nes()
{

}

bool Nes::load_cartridge(std::unique_ptr<NesFileParser> cartridge)
{
    bool success = true;

    user_interrupt();

    ppu_.reset();

    cartridge_ = std::move(cartridge);
    if (cartridge_)
    {
        success = CartridgeInterface::load(processor_, ppu_, *cartridge_);
    }
    processor_.reset();
    
    update_state(State::IDLE);

    return success;
}

void Nes::run_continuous()
{
    update_state(State::RUNNING);

    while (!should_exit_)
    {
        if (!step())
        {
            continue;
        }
        check_timer();
    }
    update_state(State::IDLE);
}

void Nes::run()
{
    update_state(State::RUNNING);
    
    while (!should_exit_)
    {
        if (!step())
        {
            break;
        }
        check_timer();
    }
    update_state(State::IDLE);
}

bool Nes::step()
{
    bool should_continue = true;

    clock_ticks_ += 4;

    ppu_.step();

    if (clock_ticks_ % 12 == 0)
    {
        joypads_.step();
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
    processor_.print_status();
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

void Nes::update_state(State state)
{
    if (state == state_)
    {
        return;
    }

    State old_state = state_;

    state_ = state;
    const char * color = state_ == State::RUNNING ? "#4F8F00" : "#455760";
    update_ui(UI::state_label, magic_enum::enum_name<State>(state), color);

    processor_.set_verbose(state_ == State::IDLE);

    if (state_ == State::IDLE || state_ == State::OFF)
    {
        should_exit_ = false;

        processor_.print_status();
        update_ui(UI::current_instruction_label, "");
        update_ui_opacity(UI::dimming_rect, 0.3);

        update_ui_memory_view(processor_.cmemory());
    }
    else if (state_ == State::RUNNING)
    {
        processor_.dim_status();
        update_ui(UI::current_instruction_label, "");
        update_ui_opacity(UI::dimming_rect, 0.0);
    }
}
