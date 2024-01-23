#include "io/display.hpp"
#include "platform/menu_handler.hpp"
#include "system/callbacks.hpp"
#include "system/nes.hpp"

#include <QApplication>
#include <QMenuBar>
#include <QQmlApplicationEngine>
#include <QMainWindow>

#include <memory>

std::shared_ptr<Nes> nes_;


enum class Platform
{
    MAC,
    LINUX
};

#ifdef Q_OS_MAC
static constexpr Platform CURRENT_PLATFORM = Platform::MAC;
#else
static constexpr Platform CURRENT_PLATFORM = Platform::LINUX;
#endif

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

    qmlRegisterType<NesDisplayView>("com.boettcher.jesse", 0, 1, "NesDisplayView");

    QQmlApplicationEngine engine;
    // const QUrl ctrl_window_qml(u"qrc:/nes_qt/controls.qml"_qs);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    // engine.load(ctrl_window_qml);

    // Create the main window
    QMainWindow main_window;
    main_window.setWindowTitle(QObject::tr("Nintendo Entertainment System"));
    main_window.resize(512, 480);
    main_window.show();

    QQuickView *view = new QQuickView();
    view->setSource(QUrl(QStringLiteral("qrc:/nes_qt/main.qml")));  // Replace with your QML file path

    QWidget *container = QWidget::createWindowContainer(view, &main_window);
    container->setMinimumSize(512, 480);

    main_window.setCentralWidget(container);

    // Add menus to the menu bar
    MenuHandler menu_handler(nes_);
    QMenuBar* menu_bar = nullptr;

    if constexpr (CURRENT_PLATFORM == Platform::MAC)
    {
        menu_bar = new QMenuBar();
    }
    else
    {
        menu_bar = main_window.menuBar();
    }

    QMenu *fileMenu = menu_bar->addMenu("File");
    QMenu *debugMenu = menu_bar->addMenu("Debug");
    fileMenu->addAction("Open ROM...");
    debugMenu->addAction("Run");
    debugMenu->addSeparator();
    debugMenu->addAction("Command..");

    // Other menus can be added similarly
    // ...


    engine.rootContext()->setContextProperty("menu_handler", &menu_handler);

    return app.exec();
}
