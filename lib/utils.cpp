#include "lib/utils.hpp"

void log_function_calls_per_second()
{
    static std::atomic<int> call_count{0};
    static auto last_print_time = std::chrono::high_resolution_clock::now();

    // Increment call count
    ++call_count;

    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = current_time - last_print_time;

    // Check if one second has passed
    if (elapsed.count() >= 1.0)
    {
        LOG(INFO) << "Function was called " << call_count << " times per second.\n";

        // Reset counter and last print time
        call_count = 0;
        last_print_time = std::chrono::high_resolution_clock::now();
    }
}
