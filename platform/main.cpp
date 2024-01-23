#include <QApplication>
#include <QQmlApplicationEngine>

#include <memory>

#include "io/display.hpp"
#include "platform/menu_handler.hpp"
#include "system/callbacks.hpp"
#include "system/nes.hpp"

std::shared_ptr<Nes> nes_;

void connect_nes_refresh_callback(std::function<void()> callback)
{
    nes_->display().set_refresh_callback(callback);
}

int main(int argc, char *argv[])
{
    // Launch NES - nes needs to be created first because the QT NesDisplayView installs its
    // refresh callback in the constructor
    nes_ = std::make_shared<Nes>(nullptr);

    // Create and start QTApp
    QApplication app(argc, argv);
    MenuHandler menu_handler(nes_);

    qmlRegisterType<NesDisplayView>("com.boettcher.jesse", 0, 1, "NesDisplayView");

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/nes_qt/Main.qml"_qs);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    engine.rootContext()->setContextProperty("menu_handler", &menu_handler);

    return app.exec();
}
