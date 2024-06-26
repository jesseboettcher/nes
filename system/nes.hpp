#pragma once

#include "agent/agent_interface.hpp"
#include "io/cartridge.hpp"
#include "io/display.hpp"
#include "io/files.hpp"
#include "io/joypads.hpp"
#include "processor/nes_apu.hpp"
#include "processor/nes_ppu.hpp"
#include "processor/ppu_address_bus.hpp"
#include "processor/processor_6502.hpp"

#include <atomic>
#include <chrono>

class Nes
{
public:
    enum class State
    {
    	OFF,
        IDLE,
        RUNNING,
    };
    
    Nes(std::shared_ptr<Cartridge> cartridge = nullptr);
	virtual ~Nes();

    bool load_cartridge(std::shared_ptr<Cartridge> cartridge);
    
	// Run and execute instructions from memory
	void run();

	// Step the system 1 cycle. Returns true if the system should continue running
	bool step();

	// Step the system until the cpu has executed another instruction
	void step_cpu_instruction();

    // Interrupt the run sequence, blocks until running has exited
    void user_interrupt();
    
    NesDisplay& display() { return display_; }

    void shutdown() { user_interrupt(); }

    void sound_test() { apu_->test(); }

    // Snapshots consist of screenshots and a corresponding json file that includes the
    // state of joypad buttons.
    void configure_capture_snapshots(std::string_view path, std::chrono::milliseconds interval);
    void check_capture_snapshot();
    
	void check_send_screenshot_to_agent();

    std::shared_ptr<AgentInterface> agent_interface() { return agent_interface_; }

protected:
	friend class CommandPrompt;
	Processor6502& processor() { return *processor_; }
	NesPPU& ppu() { return *ppu_; }

private:
    void adjust_emulation_speed();
    void print_emulation_speed();

    void update_state(State state);
    
    std::shared_ptr<Cartridge> cartridge_;

	// clocks:
	// master: 21.477272Mhz
	// ppu: master / 4
	// cpu: master / 12
	uint64_t clock_ticks_{0};

	AddressBus address_bus_;
	PPUAddressBus ppu_address_bus_;

	std::shared_ptr<Processor6502> processor_;
	NesDisplay display_;
	std::shared_ptr<NesAPU> apu_;
	std::shared_ptr<NesPPU> ppu_;
	std::shared_ptr<Joypads> joypads_;
	bool nmi_signal_{false};

	std::shared_ptr<AgentInterface> agent_interface_;

    std::atomic<State> state_{State::IDLE};
    std::atomic<bool> should_exit_{false};

    uint64_t snapshot_interval_ticks_;
    uint64_t last_snapshot_ticks_;
    uint64_t last_button_triggered_snapshot_ticks_;
    std::string snapshots_directory_;

    uint64_t last_agent_screenshot_ticks_;
};
