#ifndef __UI_PROPERTIES_H__
#define __UI_PROPERTIES_H__
#pragma once

#include "lib/magic_enum.hpp"

#include <optional>
#include <string_view>

enum class UI
{
	none,
	pc_label,
	a_reg_label,
	x_reg_label,
	y_reg_label,
	sp_reg_label,
	current_instruction_label,
	state_label,
};

static constexpr std::string_view UI_BLACK = "#000000";
static constexpr std::string_view UI_LIGHT_GREY = "#BBBBBB";

std::string_view ui_to_string(UI property);
UI ui_from_string(std::string s);

void update_ui(UI property, std::optional<std::string_view> str,
			                std::optional<std::string_view> color = std::nullopt);

#endif // __UI_PROPERTIES_H__