#pragma once

#include "platform/view_update_relay.hpp"

#include <cstdint>
#include <functional>
#include <mutex>

#include <QtQuick>
#include <QQuickPaintedItem>

class NesDisplay
{
public:
    struct Color
    {

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    static constexpr int32_t WIDTH = 256;
    static constexpr int32_t HEIGHT = 240;
    static constexpr int32_t OVERSCAN = 16;
    static constexpr Color BLACK = Color({0, 0, 0, 255});
    
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
        std::scoped_lock lock(display_buffer_lock_);
        draw_buffer_index_ = draw_buffer_index_ == 1 ? 0 : 1;
    }
    
    bool is_pixel_transparent(int32_t x, int32_t y);

    std::mutex& display_buffer_lock() { return display_buffer_lock_; }

private:
    std::mutex display_buffer_lock_;

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
