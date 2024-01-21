#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <memory>

#include "io/cartridge_interface.hpp"
#include "io/display.hpp"
#include "io/prompt.hpp"
#include "system/callbacks.hpp"
#include "system/nes.hpp"

std::unique_ptr<Nes> nes_;
std::thread* vm_thread_;

void run_6502_tests();

void connect_nes_refresh_callback(std::function<void()> callback)
{
    nes_->display().set_refresh_callback(callback);
}

void start_nes()
{
    std::unique_ptr<NesFileParser> cartridge;

    std::filesystem::path path("/home/jesse/code/nes/roms/donkey_kong.nes");
    cartridge = std::make_unique<NesFileParser>(path);

    nes_->load_cartridge(std::move(cartridge));

    vm_thread_ = new std::thread([]{ CommandPrompt::instance().launch_prompt(*nes_); });
}

int main(int argc, char *argv[])
{
    // Launch NES - nes needs to be created first because the QT NesDisplayView installs its
    // refresh callback in the constructor
    nes_ = std::make_unique<Nes>(nullptr);
    start_nes();

    // Create and start QTApp
    QGuiApplication app(argc, argv);

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


    // CommandPrompt::instance().write_command("run");
    run_6502_tests();
    return 0;

    // return app.exec();
}
