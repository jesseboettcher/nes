#pragma once

#include "platform/ui_properties.hpp"

#include <QObject>
#include <QMap>

class TextController : public QObject
{
    Q_OBJECT

    TextController()
    {
    }

public:
    static TextController& instance()
    {
        static TextController* text_controller = new TextController();

        return *text_controller;
    }


    Q_INVOKABLE QString get_text(const QString &key) const
    {
        return strings_.value(ui_from_string(key.toStdString()), "");
    }

    Q_INVOKABLE QString get_color(const QString &key) const
    {
        return colors_.value(ui_from_string(key.toStdString()), "black");
    }

    Q_INVOKABLE void set_text(const QString &key, const QString &text)
    {
        UI enum_key = ui_from_string(key.toStdString());
        if (strings_[enum_key] != text)
        {
            strings_[enum_key] = text;
            emit text_changed(key);
        }
    }

    Q_INVOKABLE void set_color(const QString &key, const QString &text)
    {
        UI enum_key = ui_from_string(key.toStdString());
        if (colors_[enum_key] != text)
        {
            colors_[enum_key] = text;
            emit text_changed(key);
        }
    }

signals:
    void text_changed(const QString &key);

private:
    QMap<UI, QString> strings_;
    QMap<UI, QString> colors_;
};
