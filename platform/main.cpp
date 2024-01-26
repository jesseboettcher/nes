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

void close_main_callback()
{
    UIContext::instance().main_window = nullptr;
}

void close_registers_callback()
{
    UIContext::instance().registers_window = nullptr;
}

void close_memory_callback()
{
    UIContext::instance().memory_window = nullptr;
}

UIMainWindow* configure_main_window(UIController& uiController)
{
    UIMainWindow* main_window = new UIMainWindow;

    main_window->setWindowTitle(QObject::tr("Nintendo Entertainment System"));
    main_window->resize(512, 480);
    main_window->show();

    main_window->set_close_callback(close_main_callback);

    QQuickView *view = new QQuickView();
    view->rootContext()->setContextProperty("UIController", &uiController);
    view->setSource(QUrl(QStringLiteral("qrc:/nes_qt/main.qml")));  // Replace with your QML file path

    QWidget *container = QWidget::createWindowContainer(view, main_window);
    container->setMinimumSize(512, 480);

    main_window->setCentralWidget(container);
    return main_window;
}

UIWindow* configure_registers_window(QQmlApplicationEngine& engine, QMainWindow& main_window)
{
    QQmlComponent registers_component(&engine, QUrl(QStringLiteral("qrc:/nes_qt/registers.qml")));
    QObject* registers_object = registers_component.create();

    UIWindow* registers_window = qobject_cast<UIWindow*>(registers_object);
    assert(registers_window);

    registers_window->set_close_callback(close_registers_callback);

    QRect main_rect = main_window.geometry();
    registers_window->setX(main_rect.x() + main_rect.width() + 20);
    registers_window->setY(main_rect.y());

    registers_window->setMinimumSize(registers_window->geometry().size());
    registers_window->setMaximumSize(registers_window->geometry().size());

    return registers_window;
}

UIWindow* configure_memory_window(QQmlApplicationEngine& engine, QWindow& registers_window)
{
    QQmlComponent memory_component(&engine, QUrl(QStringLiteral("qrc:/nes_qt/memory.qml")));
    QObject* memory_object = memory_component.create();

    UIWindow* memory_window = qobject_cast<UIWindow*>(memory_object);
    assert(memory_window);

    memory_window->set_close_callback(close_memory_callback);

    QRect registers_rect = registers_window.geometry();
    memory_window->setX(registers_rect.x() + registers_rect.width() + 20);
    memory_window->setY(registers_rect.y());

    memory_window->setMinimumSize(memory_window->geometry().size());
    memory_window->setMaximumSize(memory_window->geometry().size());

    return memory_window;
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
    run_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));

    QAction *step_action = new QAction("Step", menu_bar);
    step_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));

    QAction *stop_action = new QAction("Stop", menu_bar);
    stop_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));

    QAction *open_action = new QAction("Open ROM...", menu_bar);
    open_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));

    QAction *goto_mem_action = new QAction("Goto Memory Location...", menu_bar);
    goto_mem_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));

    file_menu->addAction(open_action);
    debug_menu->addAction(run_action);
    debug_menu->addAction(step_action);
    debug_menu->addAction(stop_action);
    debug_menu->addSeparator();
    debug_menu->addAction(goto_mem_action);
    debug_menu->addAction("Command..");

    QObject::connect(open_action, &QAction::triggered, &menu_handler, &MenuHandler::load_rom);
    QObject::connect(run_action,  &QAction::triggered, &menu_handler, &MenuHandler::run);
    QObject::connect(step_action,  &QAction::triggered, &menu_handler, &MenuHandler::step);
    QObject::connect(stop_action,  &QAction::triggered, &menu_handler, &MenuHandler::stop);
    QObject::connect(goto_mem_action,  &QAction::triggered, &menu_handler, &MenuHandler::goto_memory);

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
    qmlRegisterType<UIWindow>("com.boettcher.jesse", 0, 1, "UIWindow");

    UIContext& ui_context = UIContext::instance();

    QObject::connect(
        &ui_context.engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    UIController& uiController = UIController::instance();
    ui_context.engine.rootContext()->setContextProperty("UIController", &uiController);

    // Create windows
    ui_context.main_window = configure_main_window(uiController);
    ui_context.registers_window = configure_registers_window(ui_context.engine, *ui_context.main_window);
    ui_context.memory_window = configure_memory_window(ui_context.engine, *ui_context.registers_window);

    // Add menus to the menu bar
    MenuHandler menu_handler(nes_, ui_context.memory_window);
    ui_context.menu_bar = create_menus(*ui_context.main_window, menu_handler);

    ui_context.engine.rootContext()->setContextProperty("menu_handler", &menu_handler);

    return app.exec();
}
