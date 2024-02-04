#pragma once

#include "lib/magic_enum.hpp"
#include "io/joypads.hpp"
#include "processor/address_bus.hpp"
#include "processor/nes_ppu.hpp"

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
	sr_reg_label,

    n_flag_label,
    o_flag_label,
    b_flag_label,
    i_flag_label,
    z_flag_label,
    c_flag_label,

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

void update_ui_memory_view(const AddressBus& memory);
void update_ui_sprites_view(const std::vector<NesPPU::Sprite>& sprite_data);

bool is_button_pressed(Joypads::Button button);