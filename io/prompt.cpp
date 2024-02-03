#include "io/prompt.hpp"

#include "processor/utils.hpp"
#include "system/nes.hpp"

#include <cassert>
#include <iostream>
#include <regex>

[[maybe_unused]] static std::string read_input()
{
    std::string result;
    bool arrow = false;

    while (true)
    {
        int ch = getchar();
        
        // TODO hide special charactes from stdout
        if (arrow)
        {
            if (ch == 65) //up
            {

            }
            if (ch == 66) // down
            {

            }
            arrow = false;
            continue;
        }
        if (ch == 27)
        {
            arrow = true;
            continue;
        }
        if (ch == '\n')
        {
            break;
        }
        result += ch;
    }
    return result;
}

CommandPrompt::CommandPrompt()
: command_sema_(0)
{
    
}

CommandPrompt& CommandPrompt::instance()
{
    static CommandPrompt prompt;
    return prompt;
}

void CommandPrompt::launch_prompt(Nes& nes)
{
    bool should_continue = true;
    while (should_continue)
    {
        std::string cmd;

        command_sema_.acquire();

        std::lock_guard<std::mutex> guard(mutex_);
        if (do_shutdown_)
        {
            break;
        }
        cmd = queued_commands_.front();
        queued_commands_.pop();
        
        should_continue = execute_command(nes, cmd);
    }
}

void CommandPrompt::write_command(const std::string& cmd)
{
    std::lock_guard<std::mutex> guard(mutex_);
    
    queued_commands_.push(cmd);
    command_sema_.release();
}

void CommandPrompt::shutdown()
{
    std::lock_guard<std::mutex> guard(mutex_);
    do_shutdown_ = true;
    command_sema_.release();
}

bool CommandPrompt::execute_command(Nes& nes, std::string cmd)
{
    static std::string last_cmd;

    const std::regex txt_regex("[a-z]+\\.txt");
    const std::regex break_regex("(break|b) ?(0x[A-Fa-f0-9]+|[0-9]+)?");
    const std::regex watch_regex("(watch|w) ?(0x[A-Fa-f0-9]+|[0-9]+)?");
    const std::regex clear_regex("(clear|c) (0x[A-Fa-f0-9]+|[0-9]+)");
    const std::regex history_regex("(history|h) (0x[A-Fa-f0-9]+|[0-9]+)");
    const std::regex run_regex("run|r");
    const std::regex step_regex("(step|s)\\s*(\\d*)");
    const std::regex exit_regex("exit|e|quit|q");
    const std::regex test_regex("test|t");
    const std::regex print_regex("(print|p) (r|registers|m|memory|s|stack|vram|v|n|nametable|tile|oam|sprite|attr|palette) ?(0x[A-Fa-f0-9]+|[0-9]+)? ?(0x[A-Fa-f0-9]+|[0-9]+)?");
    std::smatch base_match;

    if (cmd.size() == 0)
    {
        cmd = last_cmd;
    }
    last_cmd = cmd;

    if (cmd.size() == 0)
    {

    }
    else if (std::regex_match(cmd, base_match, exit_regex))
    {
        return false;
    }
    else if (std::regex_match(cmd, base_match, test_regex))
    {
        nes.processor().execute_instruction({{0xA9, 0x42}});
        nes.processor().execute_instruction({{0x8D, 142, 2}});
        nes.processor().execute_instruction({{0xA9, 0x0}});
        nes.processor().execute_instruction({{0xAD, 142, 2}});
    }
    else if (std::regex_match(cmd, base_match, break_regex))
    {
        if (base_match.size() == 3 && base_match[2].str().size())
        {
            nes.processor().breakpoint(std::stoi(base_match[2], 0, 0));
        }
        else
        {
            nes.processor().print_breakpoints();
        }
    }
    else if (std::regex_match(cmd, base_match, watch_regex))
    {
        if (base_match.size() == 3 && base_match[2].str().size())
        {
            nes.processor().watchpoint(std::stoi(base_match[2], 0, 0));
        }
        else
        {
            nes.processor().print_watchpoints();
        }
    }
    else if (std::regex_match(cmd, base_match, clear_regex))
    {
        nes.processor().clear_breakpoint(std::stoi(base_match[2], 0, 0));
    }
    else if (std::regex_match(cmd, base_match, history_regex))
    {
        nes.processor().print_history(std::stoi(base_match[2], 0, 0));
    }
    else if (std::regex_match(cmd, base_match, run_regex))
    {
        nes.run();
    }
    else if (std::regex_match(cmd, base_match, step_regex))
    {
        int32_t i_count = base_match.size() == 3 && base_match[2].str().size() ? std::stoi(base_match[2], 0, 0) : 1;

        for (int32_t i = 0;i < i_count;i++)
        {
            nes.step_cpu_instruction();
        }
    }
    else if (std::regex_match(cmd, base_match, print_regex) && base_match.size() > 1)
    {
        std::smatch detail_match;
        std::string sub_cmd = base_match[2];

        if (std::regex_match(sub_cmd, detail_match, std::regex("r|regsiters")))
        {
            nes.processor().print_registers();
        }
        else if (std::regex_match(sub_cmd, detail_match, std::regex("m|memory")))
        {
            assert(base_match.size() == 5);
            nes.processor().print_memory(std::stoi(base_match[3], 0, 0),
                                         std::stoi(base_match[4], 0, 0));
        }
        else if (std::regex_match(sub_cmd, detail_match, std::regex("v|vram")))
        {
            assert(base_match.size() == 5);
            std::cout << nes.ppu().cmemory().view(std::stoi(base_match[3], 0, 0),
                                                  std::stoi(base_match[4], 0, 0));
        }
        else if (std::regex_match(sub_cmd, detail_match, std::regex("s|stack")))
        {
            nes.processor().print_stack();
        }
        else if (std::regex_match(sub_cmd, detail_match, std::regex("n|nametable")))
        {
            PPUAddressBus::NametableView nv =
                nes.ppu().cmemory().nametable_view(nes.ppu().nametable_base_address());

            std::cout << nv;
        }
        else if (std::regex_match(sub_cmd, detail_match, std::regex("oam")))
        {
            for (uint16_t i = 0;i < 64;i++)
            {
                NesPPU::Sprite sprite = nes.ppu().sprite(i);
                std::cout << "sprite " << i << ": " << sprite << std::endl;
            }
        }
        if (std::regex_match(sub_cmd, detail_match, std::regex("sprite")))
        {
//            for (int16_t i = 63;i >= 0;i--) // sprite with lower address wins with overlapping sprites
            int16_t i = 0;
            {
                NesPPU::Sprite s = nes.ppu().sprite(i);

                for (uint8_t p = 0;p < 8 * 8;p++)
                {
                    uint8_t tile_pixel_x = p % 8;
                    uint8_t tile_pixel_y = p / 8;
//                    uint8_t pixel_x = s.x_pos + tile_pixel_x;
//                    uint8_t pixel_y = s.y_pos + tile_pixel_y + 1; // sprite data is delayed by one scanline https://www.nesdev.org/wiki/PPU_OAM
                    
                    // Get the index into the color table from the pattern table tile
                    const uint8_t colortable_index = nes.ppu().get_colortable_index_for_tile_and_pixel(nes.ppu().pattern_table_base_address(),
                                                                                                       tile_pixel_x,
                                                                                                       tile_pixel_y,
                                                                                                       s.tile_index);
                    // Determine which palette to use
//                    const uint8_t palette_index = 0 + (s.attributes & 0x3); // + 4?

                    // Retrieve the RGB color for this pixel
//                    const NesDisplay::Color pixel_color = nes.ppu().fetch_color_from_palette(palette_index, colortable_index);

                    std::cout << +colortable_index << " ";
//                    std::cout << +palette_index << " ";
                    
                    if (p % 8 == 7)
                    {
                        std::cout << std::endl;
                    }
                }
            }

        }
        else if (std::regex_match(sub_cmd, detail_match, std::regex("tile")))
        {
            std::cout << "tile " << base_match[3] << std::endl;
            int32_t pattern_tile_index = std::stoi(base_match[3], 0, 0);


            for (int32_t y = 0;y < 8;++y)
            {
                for(int32_t x = 0;x < 8;++x)
                {
                    uint8_t color_for_pixel =
                        nes.ppu().get_colortable_index_for_tile_and_pixel(
                                nes.ppu().pattern_table_base_address(),
                                x,
                                y,
                                pattern_tile_index);

                    std::cout << std::hex << std::setfill('0') << std::setw(2) << + color_for_pixel << " ";
                }
                std::cout << std::endl;
            }
        }
        else if (std::regex_match(sub_cmd, detail_match, std::regex("attr")))
        {
            uint16_t attr_addr = nes.ppu().nametable_base_address() + 32 * 30;
            PPUAddressBus::View v =
                nes.ppu().cmemory().view(attr_addr, 64);


            std::cout << std::hex << std::setfill('0');
            std::cout << "\n-------------\n";
            std::cout << "  Attribute Table\n";
            std::cout << "-------------\n";

            static constexpr uint8_t BYTES_PER_BLOCK = 1;
            static constexpr uint8_t BYTES_PER_LINE = 8;

            for (uint16_t i = 0; i < v.size; ++i)
            {
                if (i % BYTES_PER_LINE == 0)
                {
                    if (i > 0)
                    {
                        std::cout << std::endl;
                    }

                    // address prefix
                    std::cout << "0x" << std::setw(4) << v.address + i << "    ";
                }
                else if (i % BYTES_PER_BLOCK == 0)
                {
                    std::cout << " ";
                }

                std::cout << std::setw(2) << +v.memory[v.address + i];
            }
            std::cout << std::dec << std::endl << std::endl;
        }
    }
    else
    {
        last_cmd = "";
        std::cout << "Unrecognized command: " << cmd << "\n";
    }
    return true;
}
