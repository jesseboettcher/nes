#pragma once

#include "platform/ui_properties.hpp"

#include <QObject>
#include <QMap>

class UIController : public QObject
{
    Q_OBJECT

public:
    UIController()
    {
    }

    Q_INVOKABLE QString get_text(const QString &key) const
    {
        return strings_.value(ui_from_string(key.toStdString()), "");
    }

    Q_INVOKABLE QString get_color(const QString &key) const
    {
        return colors_.value(ui_from_string(key.toStdString()), "black");
    }

    Q_INVOKABLE double get_opacity(const QString &key) const
    {
        return opacities_.value(ui_from_string(key.toStdString()), 1.0);
    }

    Q_INVOKABLE void set_text(const QString &key, const QString &text)
    {
        UI enum_key = ui_from_string(key.toStdString());
        if (strings_[enum_key] != text)
        {
            strings_[enum_key] = text;
            emit ui_changed(key);
        }
    }

    Q_INVOKABLE void set_color(const QString &key, const QString &text)
    {
        UI enum_key = ui_from_string(key.toStdString());
        if (colors_[enum_key] != text)
        {
            colors_[enum_key] = text;
            emit ui_changed(key);
        }
    }

    Q_INVOKABLE void set_opacity(const QString &key, const double opacity)
    {
        UI enum_key = ui_from_string(key.toStdString());
        if (opacities_[enum_key] != opacity)
        {
            opacities_[enum_key] = opacity;
            emit ui_changed(key);
        }
    }

signals:
    void ui_changed(const QString &key);

private:
    QMap<UI, QString> strings_;
    QMap<UI, QString> colors_;
    QMap<UI, double> opacities_;
};
