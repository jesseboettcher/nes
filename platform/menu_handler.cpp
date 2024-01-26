#include "platform/menu_handler.hpp"

#include "io/cartridge_interface.hpp"
#include "io/prompt.hpp"
#include "platform/ui_context.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>

#include <string>

std::thread* vm_thread_;

MenuHandler::MenuHandler(std::shared_ptr<Nes>& nes, QWindow* memory_window)
    : QObject(nullptr)
    , nes_(nes)
{
}

void MenuHandler::start_nes(std::filesystem::path path)
{
    std::unique_ptr<NesFileParser> cartridge;

    cartridge = std::make_unique<NesFileParser>(path);

    nes_->load_cartridge(std::move(cartridge));

    vm_thread_ = new std::thread([this]{ CommandPrompt::instance().launch_prompt(*nes_); });
}

void MenuHandler::load_rom()
{
    // DEBUG ITERATION SPEED
    std::filesystem::path path("/Users/jesse/code/nes/roms/donkey_kong.nes");
    start_nes(path);
    return;
    ///////

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
    CommandPrompt::instance().write_command("run");
}

void MenuHandler::step()
{
    CommandPrompt::instance().write_command("step");
}

void MenuHandler::stop()
{
    nes_->user_interrupt();
}

void MenuHandler::goto_memory()
{
    if (UIContext::instance().memory_window == nullptr)
    {
        return;
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
    int32_t address = std::stoi(text.toStdString(), nullptr, 0);
    int32_t line_number = address / 8; /* number of bytes shown per line */

    static const double ALIGNMENT_OFFSET = 0.00001; // needed to avoid a partial line shown
    double scroll_position = line_number / (Memory::ADDRESSABLE_MEMORY_SIZE / 8.0) + ALIGNMENT_OFFSET;

    QMetaObject::invokeMethod(scroll_view_, "set_scroll_position", Q_ARG(QVariant, scroll_position));
}

void MenuHandler::command()
{
    qDebug() << "command";
}
