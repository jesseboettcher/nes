#include "io/display.hpp"

#include <glog/logging.h>

// XSetForeground(display_, gc_, color);
// XDrawPoint(display_, window_, gc_, 5, 5);
// XDrawLine(display_, window_, gc_, 511, 0, 511, 200);
// XFillRectangle(display_, window_, gc_, 0, 0, WIDTH * SCALE, HEIGHT * SCALE);

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
    offscreen_[x][y] = color;
}

void NesDisplay::render()
{
    for (uint32_t x = 0; x < WIDTH; ++x)
    {
        for (uint32_t y = 0; y < HEIGHT; ++y)
        {
            XSetForeground(display_, gc_, offscreen_[x][y]);

            int32_t adj_x = x * SCALE;
            int32_t adj_y = y * SCALE;

            XDrawPoint(display_, window_, gc_, adj_x + 0, adj_y + 0);
            XDrawPoint(display_, window_, gc_, adj_x + 1, adj_y + 0);
            XDrawPoint(display_, window_, gc_, adj_x + 0, adj_y + 1);
            XDrawPoint(display_, window_, gc_, adj_x + 1, adj_y + 1);
        }
    }
    XFlush(display_);
    XSync(display_, False);
}

void NesDisplay::init()
{
    display_ = XOpenDisplay(nullptr);

    window_ = XCreateWindow(display_, DefaultRootWindow(display_), 0, 0, 
                            WIDTH * SCALE, HEIGHT * SCALE, 0, 
                            CopyFromParent, CopyFromParent, CopyFromParent,
                            0, 0);

    XMapWindow(display_, window_);

    XGCValues values;    /* initial values for the GC.   */
    gc_ = XCreateGC(display_, window_, 0, &values);
    XSync(display_, False);

    // Wait for an event before drawing or nothing will show up
    XSelectInput(display_, window_, ExposureMask);
    XEvent ev;
    XNextEvent(display_, &ev);

    for (int32_t i = 0; i < WIDTH * HEIGHT; ++i)
    {
        draw_pixel(i % WIDTH, i / WIDTH, rgb(0xFF, 0, 0));
    }

    render();

    XSetForeground(display_, gc_, rgb(0xFF, 0xFF, 0xFF));
    XDrawString(display_, window_, gc_, 10, 10, "Hello! Nice to meet you!",
                strlen("Hello! Nice to meet you!"));

    XFlush(display_);
    XSync(display_, False);
}
