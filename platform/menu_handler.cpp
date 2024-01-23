#include "platform/menu_handler.hpp"

#include "io/cartridge_interface.hpp"
#include "io/prompt.hpp"

#include <QDebug>
#include <QFileDialog>

#include <string>

std::thread* vm_thread_;

void MenuHandler::start_nes(std::filesystem::path path)
{
    std::unique_ptr<NesFileParser> cartridge;

    // std::filesystem::path path("/home/jesse/code/nes/roms/donkey_kong.nes");
    cartridge = std::make_unique<NesFileParser>(path);

    nes_->load_cartridge(std::move(cartridge));

    vm_thread_ = new std::thread([this]{ CommandPrompt::instance().launch_prompt(*nes_); });
}

void MenuHandler::load_rom()
{
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        tr("Select a Nes ROM"),
        "~/",
        tr("Nes ROMs (*.nes)")
    );

    if (!fileName.isEmpty())
    {
        start_nes(fileName.toStdString());
    }

}

void MenuHandler::run_command()
{
    qDebug() << "run_command";
    CommandPrompt::instance().write_command("run");
    // Your action code here
}
