#pragma once

#include <QObject>

class ViewUpdateRelay : public QObject
{
    Q_OBJECT

public:
    ViewUpdateRelay() {}
    virtual ~ViewUpdateRelay() {}

signals:
    void requestUpdate(); // definition is created by moc preprocessor
};
