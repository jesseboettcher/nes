#pragma once

#include "config/flags.hpp"

#include <cstdint>
#include <functional>

#if DISPLAY_TYPE == DISPLAY_LINUX

#include "platform/view_update_relay.hpp"

#include <QtQuick>
#include <QQuickPaintedItem>

class NesDisplay
{
public:
    static constexpr int32_t WIDTH = 256;
    static constexpr int32_t HEIGHT = 240;
    
    struct Color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    
    NesDisplay() {}
    
    void set_refresh_callback(std::function<void()> callback) { refresh_callback_ = callback; }

    void init();
    void clear_screen(Color color = {0,0,0,0});
    void draw_pixel(int32_t x, int32_t y, Color color);
    void render();
    
    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return {.r = r, .g = g, .b = b, .a = 0xFF}; }

    Color* display_buffer() { return (Color*)offscreen_[display_buffer_index()]; }
    
    uint32_t draw_buffer_index() { return draw_buffer_index_ == 1 ? 1 : 0; }
    uint32_t display_buffer_index() { return draw_buffer_index_ == 1 ? 0 : 1; }
    
    void swap_buffers()
    {
        draw_buffer_index_ = draw_buffer_index_ == 1 ? 0 : 1;
    }
    
    bool ready_for_display_{true};
private:
    uint32_t draw_buffer_index_{0};

    Color offscreen_[2][HEIGHT][WIDTH];

    std::function<void()> refresh_callback_;
};

class NesDisplayView : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    NesDisplayView(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;

    void refresh();

private:
    ViewUpdateRelay refresh_relay_;
};

#elif DISPLAY_TYPE == DISPLAY_HEADLESS

class NesDisplay
{
public:
	static constexpr int32_t WIDTH = 256;
	static constexpr int32_t HEIGHT = 240;

    struct Color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

	NesDisplay() {}

	void init();
	void clear_screen(Color color = {0,0,0,0xFF}) {}
	void draw_pixel(int32_t x, int32_t y, Color color) {}
	void render() {}

	static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return {.r = r, .g = g, .b = b, .a = 0xFF}; }
};

#elif DISPLAY_TYPE == DISPLAY_MACOS

class NesDisplay
{
public:
    static constexpr int32_t WIDTH = 256;
    static constexpr int32_t HEIGHT = 240;
    
    struct Color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    
    NesDisplay() {}
    
    void init();
    void clear_screen(Color color = {0,0,0,0});
    void draw_pixel(int32_t x, int32_t y, Color color);
    void render();
    
    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return {.r = r, .g = g, .b = b, .a = 0xFF}; }

    Color* display_buffer() { return (Color*)offscreen_[display_buffer_index()]; }
    
    uint32_t draw_buffer_index() { return draw_buffer_index_ == 1 ? 1 : 0; }
    uint32_t display_buffer_index() { return draw_buffer_index_ == 1 ? 0 : 1; }
    
    void swap_buffers()
    {
        draw_buffer_index_ = draw_buffer_index_ == 1 ? 0 : 1;
    }
    
    bool ready_for_display_{true};
private:
    uint32_t draw_buffer_index_{0};

    Color offscreen_[2][HEIGHT][WIDTH];
};

#endif  // DISPLAY_TYPE
