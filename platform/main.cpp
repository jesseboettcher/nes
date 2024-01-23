#include "io/display.hpp"
#include "platform/menu_handler.hpp"
#include "system/callbacks.hpp"
#include "system/nes.hpp"

#include <QApplication>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenuBar>
#include <QQmlApplicationEngine>

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

void configure_main_window(QMainWindow& main_window)
{
    main_window.setWindowTitle(QObject::tr("Nintendo Entertainment System"));
    main_window.resize(512, 480);
    main_window.show();

    QQuickView *view = new QQuickView();
    view->setSource(QUrl(QStringLiteral("qrc:/nes_qt/main.qml")));  // Replace with your QML file path

    QWidget *container = QWidget::createWindowContainer(view, &main_window);
    container->setMinimumSize(512, 480);

    main_window.setCentralWidget(container);
}

QMenuBar* create_menus(QMainWindow& main_window, MenuHandler& menu_handler)
{
    QMenuBar* menu_bar = nullptr;

    if constexpr (CURRENT_PLATFORM == Platform::MAC)
    {
        menu_bar = new QMenuBar();
    }
    else
    {
        menu_bar = main_window.menuBar();
    }

    QMenu *file_menu = menu_bar->addMenu("File");
    QMenu *debug_menu = menu_bar->addMenu("Debug");
    
    QAction *run_action = new QAction("Run", menu_bar);
    run_action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));

    QAction *open_action = new QAction("Open ROM...", menu_bar);
    open_action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));

    file_menu->addAction(open_action);
    debug_menu->addAction(run_action);
    debug_menu->addSeparator();
    debug_menu->addAction("Command..");

    QObject::connect(open_action, &QAction::triggered, &menu_handler, &MenuHandler::load_rom);
    QObject::connect(run_action,  &QAction::triggered, &menu_handler, &MenuHandler::run);

    return menu_bar;
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
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // const QUrl ctrl_window_qml(u"qrc:/nes_qt/controls.qml"_qs);
    // engine.load(ctrl_window_qml);

    // Create the main window
    QMainWindow main_window;
    configure_main_window(main_window);

    // Add menus to the menu bar
    MenuHandler menu_handler(nes_);
    QMenuBar* menu_bar = create_menus(main_window, menu_handler);

    engine.rootContext()->setContextProperty("menu_handler", &menu_handler);

    return app.exec();
}
