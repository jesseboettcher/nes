#include "platform/menu_handler.hpp"

#include "io/cartridge.hpp"
#include "io/prompt.hpp"
#include "platform/ui_context.hpp"
#include "test/6502_tests.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>

#include <string>

std::thread* vm_thread_;

MenuHandler::MenuHandler()
    : QObject(nullptr)
{
}

void MenuHandler::start_nes(std::filesystem::path path)
{
    std::shared_ptr<Cartridge> cartridge;

    cartridge = Cartridge::create(path);

    if (!UIContext::instance().nes->load_cartridge(std::move(cartridge)))
    {
        // failed to load cartridge
        return;
    }

    vm_thread_ = new std::thread([this]{ CommandPrompt::instance().launch_prompt(*UIContext::instance().nes); });
}

void MenuHandler::load_rom()
{
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        tr("Select a Nes ROM"),
        QDir::homePath(),
        tr("Nes ROMs (*.nes)")
    );

    if (!fileName.isEmpty())
    {
        start_nes(fileName.toStdString());
    }
}

void MenuHandler::run()
{
    // load_rom(); // DEBUG
    // CommandPrompt::instance().write_command("b 0xE031"); // DEBUG
    CommandPrompt::instance().write_command("run");
}

void MenuHandler::step()
{
    CommandPrompt::instance().write_command("step");
}

void MenuHandler::stop()
{
    UIContext::instance().nes->user_interrupt();
}

void MenuHandler::goto_memory()
{
    if (not UIContext::instance().memory_window->isVisible())
    {
        show_memory();
    }

    QQuickItem* scroll_view_ = UIContext::instance().memory_window->findChild<QQuickItem*>("memory_scroll_view");

    QString text = QInputDialog::getText(
        nullptr,                 // Parent widget
        "Goto Memory Location",    // Title of the dialog
        "Jump to memory at address...",      // Label text
        QLineEdit::Normal,       // Text line edit mode
        QString(),               // Default text
        nullptr,                 // OK button pressed
        Qt::WindowFlags()        // Window flags
    );

    try
    {
        int32_t address = std::stoi(text.toStdString(), nullptr, 0);
        int32_t line_number = address / 8; /* number of bytes shown per line */

        static const double ALIGNMENT_OFFSET = 0.00001; // needed to avoid a partial line shown
        double scroll_position = line_number / (AddressBus::ADDRESSABLE_MEMORY_SIZE / 8.0) + ALIGNMENT_OFFSET;

        QMetaObject::invokeMethod(scroll_view_, "set_scroll_position", Q_ARG(QVariant, scroll_position));
    }
    catch (...)
    {}
}

void MenuHandler::command()
{
    QString text = QInputDialog::getText(
        nullptr,                 // Parent widget
        "Command",    // Title of the dialog
        "Enter command to run...",      // Label text
        QLineEdit::Normal,       // Text line edit mode
        QString(),               // Default text
        nullptr,                 // OK button pressed
        Qt::WindowFlags()        // Window flags
    );
    CommandPrompt::instance().write_command(text.toStdString());
}

void MenuHandler::run_processor_tests()
{
    Test6502 test_6502;

    test_6502.run();
}

void MenuHandler::show_registers()
{
    UIContext::instance().registers_window->show();
    UIContext::instance().configure_registers_window();
}

void MenuHandler::show_memory()
{
    UIContext::instance().memory_window->show();
    UIContext::instance().configure_memory_window();
}

void MenuHandler::show_sprites()
{
    UIContext::instance().sprites_window->show();
    UIContext::instance().configure_sprites_window();
}

void MenuHandler::close()
{
    if (UIContext::instance().registers_window && UIContext::instance().registers_window->isActive())
    {
        UIContext::instance().registers_window->close();
    }
    else if (UIContext::instance().memory_window && UIContext::instance().memory_window->isActive())
    {
        UIContext::instance().memory_window->close();
    }
    else if (UIContext::instance().memory_window && UIContext::instance().sprites_window->isActive())
    {
        UIContext::instance().sprites_window->close();
    }
    else if (UIContext::instance().main_window)
    {
        // there no isActive call for main window, but if it exists and the other windows are not
        // active, it must be
        UIContext::instance().main_window->close();
    }
}
