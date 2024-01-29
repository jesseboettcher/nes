#pragma once

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

class UIMainWindow : public QMainWindow
{
    Q_OBJECT
    QML_ELEMENT

public:
    UIMainWindow()
     : QMainWindow()
    {

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

