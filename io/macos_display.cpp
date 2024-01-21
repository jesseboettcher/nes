#include "io/display.hpp"

#include <glog/logging.h>

void NesDisplay::init()
{
}

void NesDisplay::clear_screen(Color color)
{
    for (uint32_t x = 0; x < WIDTH; ++x)
    {
        for (uint32_t y = 0; y < HEIGHT; ++y)
        {
            draw_pixel(x, y, color);
        }
    }
}

void NesDisplay::draw_pixel(int32_t x, int32_t y, Color color)
{
    //assert(x >= 0 && y >= 0);
    if (x > WIDTH || y > HEIGHT)
    {
        return;
    }
    offscreen_[draw_buffer_index()][HEIGHT - 1 - y][x] = color;
}

void NesDisplay::render()
{
    ready_for_display_ = true;
    swap_buffers();
}
