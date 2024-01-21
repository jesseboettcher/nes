#include "io/display.hpp"

#include "system/callbacks.hpp"

#include <glog/logging.h>

NesDisplay * global_display_ptr = nullptr;

void NesDisplay::init()
{
    global_display_ptr = this;
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
    assert(x >= 0 && y >= 0);
    if (x > WIDTH || y > HEIGHT)
    {
        return;
    }
    offscreen_[draw_buffer_index()][y][x] = color;
}

void NesDisplay::render()
{
    ready_for_display_ = true;
    swap_buffers();

    refresh_callback_();
}

NesDisplayView::NesDisplayView(QQuickItem *parent)
{
    // bind the nes display refresh callback to the QT signal in ViewUpdateRelay which makes sure
    // the handler is called on the main event thread
    connect_nes_refresh_callback(std::bind(&ViewUpdateRelay::requestUpdate, &refresh_relay_));

    // connect the QT signal ViewUpdateRelay to call the NesDisplayView::refresh which will
    // call the view update() to trigger a re-paint
    connect(&refresh_relay_, &ViewUpdateRelay::requestUpdate, this, &NesDisplayView::refresh,
            Qt::QueuedConnection);
}


void NesDisplayView::refresh()
{
    update(boundingRect().toAlignedRect());
}

void NesDisplayView::paint(QPainter *painter)
{
    // TODO get instance of NesDisplay and retrieve buffer from it
    // TODO time this and look at more efficient options

    QImage image((const uchar*)global_display_ptr->display_buffer(), NesDisplay::WIDTH, NesDisplay::HEIGHT,
                 QImage::Format_RGBA8888);

    QPixmap pixmap;
    pixmap.convertFromImage(image);
    painter->drawPixmap(0, 0, NesDisplay::WIDTH * 2, NesDisplay::HEIGHT * 2, // dest rect
                        pixmap,
                        0, 0,NesDisplay::WIDTH, NesDisplay::HEIGHT); // source rect
}