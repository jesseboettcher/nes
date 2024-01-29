#include "io/display.hpp"
#include "platform/menu_handler.hpp"
#include "platform/ui_controller.hpp"
#include "platform/ui_context.hpp"
#include "platform/ui_window.hpp"
#include "system/callbacks.hpp"
#include "system/nes.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QKeySequence>
#include <QMainWindow>
#include <QLabel>
#include <QMenuBar>
#include <QQmlApplicationEngine>

#include <memory>

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
    UIContext::instance().nes->display().set_refresh_callback(callback);
}

void close_main_callback()
{
    UIContext::instance().main_window = nullptr;
    QApplication::exit(0);
}

void close_registers_callback()
{
    // UIContext::instance().registers_window = nullptr;
}

void close_memory_callback()
{
    // UIContext::instance().memory_window = nullptr;
}

void create_windows()
{
    UIContext& ui = UIContext::instance();

    // main window
    ui.main_window = new UIMainWindow;

    ui.main_window->setWindowTitle(QObject::tr("Nintendo Entertainment System"));
    ui.main_window->resize(512, 480);
    ui.main_window->show();

    ui.main_window->set_close_callback(close_main_callback);

    QQuickView *view = new QQuickView();
    view->rootContext()->setContextProperty("UIController", &ui.controller);
    view->setSource(QUrl(QStringLiteral("qrc:/nes_qt/main.qml")));

    QWidget *container = QWidget::createWindowContainer(view, ui.main_window);
    container->setMinimumSize(512, 480);

    ui.main_window->setCentralWidget(container);

    // memory window
    QQmlComponent memory_component(&ui.engine, QUrl(QStringLiteral("qrc:/nes_qt/memory.qml")));
    QObject* memory_object = memory_component.create();

    ui.memory_window = qobject_cast<UIWindow*>(memory_object);

    ui.memory_window->set_close_callback(close_memory_callback);

    ui.memory_window->setMinimumSize(ui.memory_window->geometry().size());
    ui.memory_window->setMaximumSize(ui.memory_window->geometry().size());

    QRect main_rect = ui.main_window->geometry();
    ui.memory_window->setX(main_rect.x() + main_rect.width() + 20);
    ui.memory_window->setY(main_rect.y() + main_rect.height() - ui.memory_window->geometry().height());

    ui.configure_memory_window();

    // registers window
    QQmlComponent registers_component(&ui.engine, QUrl(QStringLiteral("qrc:/nes_qt/registers.qml")));
    QObject* registers_object = registers_component.create();

    ui.registers_window = qobject_cast<UIWindow*>(registers_object);

    ui.registers_window->set_close_callback(close_registers_callback);

    ui.registers_window->setMinimumSize(ui.registers_window->geometry().size());
    ui.registers_window->setMaximumSize(ui.registers_window->geometry().size());

    ui.configure_registers_window();

    // registers window
    QQmlComponent sprites_component(&ui.engine, QUrl(QStringLiteral("qrc:/nes_qt/sprites.qml")));
    QObject* sprites_object = sprites_component.create();

    ui.sprites_window = qobject_cast<UIWindow*>(sprites_object);

    ui.sprites_window->setMinimumSize(ui.sprites_window->geometry().size());
    ui.sprites_window->setMaximumSize(ui.sprites_window->geometry().size());

    ui.configure_sprites_window();

    // make sure main is activated, to get all the keyboard input
    ui.main_window->activateWindow();
}

void create_menus()
{
    UIContext& ui = UIContext::instance();

    if constexpr (CURRENT_PLATFORM == Platform::MAC)
    {
        ui.menu_bar = new QMenuBar();
    }
    else
    {
        ui.menu_bar = ui.main_window->menuBar();
    }
    
    // File menu
    QMenu *file_menu = ui.menu_bar->addMenu("File");

    QAction *open_action = new QAction("Open ROM...", ui.menu_bar);
    open_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));

    file_menu->addAction(open_action);

    QObject::connect(open_action, &QAction::triggered, &ui.menu_handler, &MenuHandler::load_rom);

    // Control menu
    QMenu *debug_menu = ui.menu_bar->addMenu("Control");

    QAction *run_action = new QAction("Run", ui.menu_bar);
    run_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));

    QAction *step_action = new QAction("Step", ui.menu_bar);
    step_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));

    QAction *stop_action = new QAction("Stop", ui.menu_bar);
    stop_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));

    QAction *goto_mem_action = new QAction("Goto Memory Location...", ui.menu_bar);
    goto_mem_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));

    QAction *cmd_action = new QAction("Command...", ui.menu_bar);
    cmd_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));

    debug_menu->addAction(run_action);
    debug_menu->addAction(step_action);
    debug_menu->addAction(stop_action);
    debug_menu->addSeparator();
    debug_menu->addAction(goto_mem_action);
    debug_menu->addAction(cmd_action);

    QObject::connect(run_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::run);
    QObject::connect(step_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::step);
    QObject::connect(stop_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::stop);
    QObject::connect(goto_mem_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::goto_memory);
    QObject::connect(cmd_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::command);

    // Window menu
    QMenu *window_menu = ui.menu_bar->addMenu("Window");

    QAction *close_action = new QAction("Close", ui.menu_bar);
    close_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));

    QAction *show_memory_action = new QAction("Memory", ui.menu_bar);
    QAction *show_registers_action = new QAction("Registers", ui.menu_bar);
    QAction *show_sprites_action = new QAction("Sprites", ui.menu_bar);

    window_menu->addAction(close_action);
    window_menu->addSeparator();
    window_menu->addAction(show_memory_action);
    window_menu->addAction(show_registers_action);
    window_menu->addAction(show_sprites_action);

    QObject::connect(close_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::close);
    QObject::connect(show_memory_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::show_memory);
    QObject::connect(show_registers_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::show_registers);
    QObject::connect(show_sprites_action,  &QAction::triggered, &ui.menu_handler, &MenuHandler::show_sprites);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Launch NES - nes needs to be created first because the QT NesDisplayView installs its
    // refresh callback in the constructor
    UIContext& ui = UIContext::instance();
    ui.nes = std::make_shared<Nes>();

    qmlRegisterType<NesDisplayView>("com.boettcher.jesse", 0, 1, "NesDisplayView");
    qmlRegisterType<UIWindow>("com.boettcher.jesse", 0, 1, "UIWindow");

    QObject::connect(
        &ui.engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    ui.engine.rootContext()->setContextProperty("UIController", &ui.controller);
    ui.engine.rootContext()->setContextProperty("menu_handler", &ui.menu_handler);
    ui.engine.rootContext()->setContextProperty("SpritesModel", &ui.sprites_model);

    create_windows();
    create_menus();

    return app.exec();
}
