#include "platform/ui_properties.hpp"

#include "platform/ui_context.hpp"

#include <glog/logging.h>
#include <iomanip>

std::string_view ui_to_string(UI property)
{
	return magic_enum::enum_name<UI>(property);
}

UI ui_from_string(std::string s)
{
	return magic_enum::enum_cast<UI>(s).value_or(UI::none);
}


void update_ui(UI property, std::optional<std::string_view> str,
			                std::optional<std::string_view> color)
{
    UIContext& ui = UIContext::instance();

	if (str)
	{
		ui.controller.set_text(QString::fromStdString(std::string(magic_enum::enum_name<UI>(property))),
							   QString::fromStdString(std::string(str.value())));
	}

	if (color)
	{
		ui.controller.set_color(QString::fromStdString(std::string(magic_enum::enum_name<UI>(property))),
					            QString::fromStdString(std::string(color.value())));

	}
}

void update_ui_opacity(UI property, double opacity)
{
    UIContext& ui = UIContext::instance();
    ui.controller.set_opacity(QString::fromStdString(std::string(magic_enum::enum_name<UI>(property))),
                              opacity);
}

void update_ui_memory_view(const Memory& memory)
{
    static const uint32_t BYTES_PER_LINE = 8;

    std::stringstream memstr;
    std::stringstream addrstr;

    for (int32_t i = 0;i < Memory::ADDRESSABLE_MEMORY_SIZE;++i)
    {
        memstr << std::hex << std::setfill('0') << std::setw(2);
        memstr << static_cast<uint32_t>(memory[i]) << " ";

        if (i % BYTES_PER_LINE == 0)
        {
            if (i != 0)
            {
                addrstr << " ";
            }
            addrstr << std::setw(0);
            addrstr << "0x";
            addrstr << std::hex << std::uppercase << std::setfill('0') << std::setw(4);
            addrstr << i;
        }
    }

    UIContext& ui = UIContext::instance();

    ui.controller.set_text(QString::fromStdString(std::string(magic_enum::enum_name<UI>(UI::memory_view))),
                           QString::fromStdString(memstr.str()));
    ui.controller.set_color(QString::fromStdString(std::string(magic_enum::enum_name<UI>(UI::memory_view))),
                           QString::fromStdString(std::string(UI_NEAR_BLACK)));


    ui.controller.set_text(QString::fromStdString(std::string(magic_enum::enum_name<UI>(UI::address_view))),
                           QString::fromStdString(addrstr.str()));
    ui.controller.set_color(QString::fromStdString(std::string(magic_enum::enum_name<UI>(UI::address_view))),
                           QString::fromStdString(std::string(UI_LIGHT_BLACK)));
}
