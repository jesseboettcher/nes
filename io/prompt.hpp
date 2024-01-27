#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <semaphore>
#include <string>

class Nes;

class CommandPrompt
{
public:
    CommandPrompt();
    static CommandPrompt& instance();
	
    void launch_prompt(Nes& nes);

    void write_command(const std::string& cmd);
    
    void shutdown();
    
private:
    bool execute_command(Nes& nes, std::string cmd);

    bool do_shutdown_{false};
    
    std::mutex mutex_;
    std::binary_semaphore command_sema_;
    std::queue<std::string> queued_commands_;
    std::optional<std::string> maybe_command_;
};
