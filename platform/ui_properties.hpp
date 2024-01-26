#pragma once

#include "lib/magic_enum.hpp"
#include "processor/memory.hpp"

#include <optional>
#include <string_view>

enum class UI
{
	none,

	// registers window
	pc_label,
	a_reg_label,
	x_reg_label,
	y_reg_label,
	sp_reg_label,
	current_instruction_label,
	state_label,
	dimming_rect,

	// memory window
	address_view,
	memory_view
};

static constexpr std::string_view UI_BLACK = "#000000";
static constexpr std::string_view UI_NEAR_BLACK = "#455760";
static constexpr std::string_view UI_LIGHT_BLACK = "#5F7784";
static constexpr std::string_view UI_LIGHT_GREY = "#BBBBBB";

std::string_view ui_to_string(UI property);
UI ui_from_string(std::string s);

void update_ui(UI property, std::optional<std::string_view> str,
			                std::optional<std::string_view> color = std::nullopt);

void update_ui_opacity(UI property, double opacity);

void update_ui_memory_view(const Memory& memory);
