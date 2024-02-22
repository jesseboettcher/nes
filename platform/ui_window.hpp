#pragma once

#include "io/display.hpp"

#include <QMainWindow>
#include <QQuickWindow>

#include <optional>
#include <unordered_map>

#include <glog/logging.h>

class UIWindow : public QQuickWindow
{
    Q_OBJECT
    QML_ELEMENT

public:
    UIWindow()
     : QQuickWindow()
    {

    }

    void set_close_callback(std::function<void()> close_callback)
    {
        close_callback_ = close_callback;
    }

protected:
    virtual void closeEvent(QCloseEvent *ev)
    {
        if (close_callback_.has_value())
        {
            close_callback_.value();
        }
        ev->ignore();
        hide();
    }

private:
    std::optional<std::function<void()>> close_callback_;
};

class AspectRatioEventFilter : public QObject
{
public:
    AspectRatioEventFilter(qreal ratio, QObject* parent = nullptr)
     : QObject(parent)
     , aspect_ratio(ratio) {}

protected:
    qreal aspect_ratio; // Desired aspect ratio

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Resize && watched->isWidgetType())
        {
            QWidget* widget = static_cast<QWidget*>(watched);
            QSize size = widget->size();
            int new_width = size.width();
            int new_height = static_cast<int>(new_width / aspect_ratio);

            // Adjust height to maintain aspect ratio without exceeding the window's maximum height
            if (new_height > widget->maximumHeight())
            {
                new_height = widget->maximumHeight();
                new_width = static_cast<int>(new_height * aspect_ratio);
            }

            widget->resize(new_width, new_height);
            return true;
        }
        return QObject::eventFilter(watched, event);
    }
};


class UIMainWindow : public QMainWindow
{
    Q_OBJECT
    QML_ELEMENT

public:
    UIMainWindow()
     : QMainWindow()
    {
        AspectRatioEventFilter* filter = new AspectRatioEventFilter((qreal)NesDisplay::WIDTH / (qreal)NesDisplay::HEIGHT, this);
        installEventFilter(filter);
    }

    void set_close_callback(std::function<void()> close_callback)
    {
        close_callback_ = close_callback;
    }

    bool is_key_pressed(int key) { return pressed_keys_[key]; }

protected:
    virtual void keyPressEvent(QKeyEvent *event) override
    {
        pressed_keys_[event->key()] = true;
    }

    virtual void keyReleaseEvent(QKeyEvent *event) override
    {
        pressed_keys_[event->key()] = false;
    }

    virtual void closeEvent(QCloseEvent *ev) override
    {
        close_callback_();
    }

private:
    std::function<void()> close_callback_;

    std::unordered_map<int, bool> pressed_keys_;
};

