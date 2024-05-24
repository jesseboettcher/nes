#include "nes.hpp"

#include "platform/ui_properties.hpp"
#include "minitrace.h"

#include <chrono>
#include <iostream>
#include <thread>

#include <glog/logging.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Nes::Nes(std::shared_ptr<Cartridge> cartridge)
: cartridge_(nullptr)
, display_()
, snapshot_interval_ticks_(0)
, last_snapshot_ticks_(0)
, last_button_triggered_snapshot_ticks_(0)
{
    std::cout << "Launching Nes...\n";

    processor_ = std::make_shared<Processor6502>(address_bus_, nmi_signal_);
    joypads_ = std::make_shared<Joypads>();
    apu_ = std::make_shared<NesAPU>();
    ppu_ = std::make_shared<NesPPU>(address_bus_, ppu_address_bus_, display_, nmi_signal_);

    address_bus_.attach_cpu(processor_);
    address_bus_.attach_joypads(joypads_);
    address_bus_.attach_ppu(ppu_);
    address_bus_.attach_apu(apu_);

    ppu_address_bus_.attach_ppu(ppu_);

    load_cartridge(cartridge);
    display_.init();

    update_state(State::OFF);

    // start the audio right away, it will play empty samples until something is pushed
    // this needs to be called from the UI thread
    apu_->start();

    agent_interface_ = std::make_shared<AgentInterface>();
}

Nes::~Nes()
{
}

bool Nes::load_cartridge(std::shared_ptr<Cartridge> cartridge)
{
    bool success = true;

    user_interrupt();

    cartridge_ = cartridge;
    if (cartridge_ && cartridge_->valid())
    {
        address_bus_.attach_cartridge(cartridge_);
        ppu_address_bus_.attach_cartridge(cartridge_);

        cartridge_->reset();
        processor_->reset();
        ppu_->reset();
        apu_->reset();

        update_state(State::IDLE);
    }

    return success;
}

void Nes::run()
{
    mtr_init("/tmp/nes_trace.json");
    MTR_META_PROCESS_NAME("nes_emulator");
    MTR_META_THREAD_NAME("main_thread");

    update_state(State::RUNNING);
    
    while (!should_exit_)
    {
        if (!step())
        {
            break;
        }
        adjust_emulation_speed();
        print_emulation_speed();
    }
    update_state(State::IDLE);

    mtr_shutdown();
    mtr_flush();
    processor_->write_history();
}

bool Nes::step()
{
    bool should_continue = true;

    clock_ticks_ += 4;

    ppu_->step();

    if (clock_ticks_ % 12 == 0)
    {
        joypads_->step(clock_ticks_);
        should_continue = processor_->step();
    }
    if (clock_ticks_ % 24 == 0)
    {
        apu_->step(clock_ticks_);
    }
    check_capture_snapshot();
    check_send_screenshot_to_agent();

    return should_continue;
}

void Nes::step_cpu_instruction()
{
    uint64_t prev_instr_count = processor_->instruction_count();
    bool should_continue = true;

    while (processor_->instruction_count() == prev_instr_count && should_continue)
    {
        should_continue = step();
    }
    processor_->print_status();
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

void Nes::check_capture_snapshot()
{
    if (snapshot_interval_ticks_ == 0)
    {
        return;
    }

    bool button_change = false;

    if (joypads_->last_press_clock_ticks() - last_button_triggered_snapshot_ticks_ >
        int64_t(21477272 * (100.0 / 1000.0))) // 250ms interval
    {
        button_change = true;
        last_button_triggered_snapshot_ticks_ = joypads_->last_press_clock_ticks();
    }

    if (button_change ||
        (clock_ticks_ - last_snapshot_ticks_ > snapshot_interval_ticks_))
    {
        last_snapshot_ticks_ = clock_ticks_;

        // write snapshot
        std::scoped_lock lock(display_.display_buffer_lock());

        QImage image((const uchar*)display_.display_buffer(),
                     NesDisplay::WIDTH, NesDisplay::HEIGHT,
                     QImage::Format_RGBA8888);

        std::stringstream ts;
        {
            auto now = std::chrono::system_clock::now();
            auto now_as_time_t = std::chrono::system_clock::to_time_t(now);

            auto duration = now.time_since_epoch();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            ts << std::put_time(std::localtime(&now_as_time_t), "%Y-%m-%d_%H.%M.%S.");
            ts << std::setw(3) << std::setfill('0') << milliseconds % 1000;  // extract milliseconds part
        }

        std::stringstream file_basename;;
        file_basename << "nes_screenshot_" << ts.str();

        std::stringstream img_path;
        img_path << snapshots_directory_ << file_basename.str() << ".png";

        bool success = image.save(img_path.str().c_str(), "PNG");

        LOG_IF(ERROR, !success) << "failed to write snapshot";

        // write metadata
        std::stringstream metadata_path;
        metadata_path << snapshots_directory_ << file_basename.str() << ".json";

        json j;
        j["timestamp"] = ts.str();
        j["image"] = std::string(file_basename.str() + ".png");
        j["buttons"] = json::object();

        j["buttons"]["up"]      = is_button_pressed(Joypads::Button::Up);
        j["buttons"]["down"]    = is_button_pressed(Joypads::Button::Down);
        j["buttons"]["left"]    = is_button_pressed(Joypads::Button::Left);
        j["buttons"]["right"]   = is_button_pressed(Joypads::Button::Right);
        j["buttons"]["a"]       = is_button_pressed(Joypads::Button::A);
        j["buttons"]["b"]       = is_button_pressed(Joypads::Button::B);
        j["buttons"]["select"]  = is_button_pressed(Joypads::Button::Select);
        j["buttons"]["start"]   = is_button_pressed(Joypads::Button::Start);

        std::ofstream metadata_output(metadata_path.str());
        metadata_output << j.dump(4);
        LOG(INFO) << "snapshot " << file_basename.str();
    }
}

void Nes::configure_capture_snapshots(std::string_view path, std::chrono::milliseconds interval)
{
    if (interval.count() == 0)
    {
        snapshot_interval_ticks_ = 0;
        return; // disable
    }

    if (interval.count() <= 30)
    {
        LOG(WARNING) << "configure_capture_snapshots: interval is to short. Must be > 30";
        return; // too fast
    }

    snapshots_directory_ = path;

    // ticks per second / fraction of a second of interval
    snapshot_interval_ticks_ = static_cast<uint64_t>(21477272 * (interval.count() / 1000.0));
    LOG(INFO) << "interval " << snapshot_interval_ticks_;
}

void Nes::check_send_screenshot_to_agent()
{
    if (clock_ticks_ - last_agent_screenshot_ticks_ <
        int64_t(21477272 * (500.0 / 1000.0))) // 500ms interval
    {
        return;
    }

    if (!agent_interface_->any_connections())
    {
        return;
    }
    last_agent_screenshot_ticks_ = clock_ticks_;

    std::scoped_lock lock(display_.display_buffer_lock());

    std::vector<int8_t> image_data;
    {
        QImage image((const uchar*)display_.display_buffer(),
                     NesDisplay::WIDTH, NesDisplay::HEIGHT,
                     QImage::Format_RGBA8888);

        QByteArray byte_array;
        QBuffer buffer(&byte_array);
        buffer.open(QIODevice::WriteOnly);

        image.save(&buffer, "PNG");

        image_data = std::vector<int8_t>(byte_array.begin(), byte_array.end());
        LOG(INFO) << "qbytearray size " << byte_array.size() << " " << image_data.size();
    }

    agent_interface_->send_screenshot(std::move(image_data));
}

void Nes::adjust_emulation_speed()
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    static auto last_update_time = std::chrono::high_resolution_clock::now();
    static uint64_t last_cycle_count = 0;
    static int32_t adjustment_padding = 0;
    static constexpr int32_t ADJUSTMENT_PADDING_DELTA = 5;
    
    if (processor_->cycle_count() - last_cycle_count > 17897)  // 1789773 = 1 second
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> delta = current_time - last_update_time;

        if (10000 > delta.count())
        {
            std::chrono::duration<double, std::micro> wait_for(10000 - delta.count() - adjustment_padding);
            std::this_thread::sleep_for(wait_for);
        }

        current_time = std::chrono::high_resolution_clock::now();
        delta = current_time - last_update_time;

        if (10000 > delta.count())
        {
            adjustment_padding -= ADJUSTMENT_PADDING_DELTA;
        }
        else
        {
            adjustment_padding += ADJUSTMENT_PADDING_DELTA * 2;
        }

        last_update_time = std::chrono::high_resolution_clock::now();
        last_cycle_count = processor_->cycle_count();
    }
    print_emulation_speed();
}

void Nes::print_emulation_speed()
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    static auto last_update_time = std::chrono::high_resolution_clock::now();
    static uint64_t last_cycle_count = 0;

    if (processor_->cycle_count() - last_cycle_count > 17897730)  // 1789773 = 1 second
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> delta = current_time - last_update_time;
        std::chrono::duration<double> uptime_seconds = current_time - start_time;

        std::cout << (100.0 * 10000000.0 / delta.count()) << std::dec << "% of realtime" << " cycle "
                  << "cycle " << processor_->cycle_count() << ", "
                  << "uptime: " << uptime_seconds.count() << " seconds" << std::endl;

        last_update_time = std::chrono::high_resolution_clock::now();
        last_cycle_count = processor_->cycle_count();
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

    processor_->set_verbose(state_ == State::IDLE);

    if (state_ == State::IDLE || state_ == State::OFF)
    {
        should_exit_ = false;

        processor_->print_status();
        update_ui_opacity(UI::dimming_rect, 0.3);

        update_ui_memory_view(processor_->cmemory());
    }
    else if (state_ == State::RUNNING)
    {
        processor_->dim_status();
        update_ui_opacity(UI::dimming_rect, 0.0);
    }
}
