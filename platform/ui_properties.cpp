#include "platform/ui_properties.hpp"

#include "platform/text_controller.hpp"

#include <glog/logging.h>

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
	if (str)
	{
		TextController::instance().set_text(QString::fromStdString(std::string(magic_enum::enum_name<UI>(property))),
										    QString::fromStdString(std::string(str.value())));
	}

	if (color)
	{
		TextController::instance().set_color(QString::fromStdString(std::string(magic_enum::enum_name<UI>(property))),
										     QString::fromStdString(std::string(color.value())));

	}
}
