#include "platform/ui_properties.hpp"

#include "platform/ui_context.hpp"

#include <gainput/gainput.h>
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

void update_ui_memory_view(const AddressBus& m)
{
    static const uint32_t BYTES_PER_LINE = 8;

    std::stringstream memstr;
    std::stringstream addrstr;

    for (int32_t i = 0;i < AddressBus::ADDRESSABLE_MEMORY_SIZE;++i)
    {
        memstr << std::hex << std::setfill('0') << std::setw(2);
        memstr << static_cast<uint32_t>(m.read(i)) << " ";

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

void update_ui_sprites_view(const std::vector<NesPPU::Sprite>& sprite_data)
{
    static auto last_update_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = current_time - last_update_time;

    if (delta.count() > 1000)
    {
        last_update_time = std::chrono::high_resolution_clock::now();

        UIContext& ui = UIContext::instance();
        ui.sprites_model.update(sprite_data);
    }
}

class JoypadInput
{
public:
    JoypadInput()
    {
        input_manager_ = new gainput::InputManager;
        gamepad_id_ = input_manager_->CreateDevice<gainput::InputDevicePad>();
        input_map_ = new gainput::InputMap(*input_manager_);

        input_map_->MapBool(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::A),      gamepad_id_, gainput::PadButtonA);
        input_map_->MapBool(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::B),      gamepad_id_, gainput::PadButtonB);
        input_map_->MapBool(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::Select), gamepad_id_, gainput::PadButtonSelect);
        input_map_->MapBool(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::Start),  gamepad_id_, gainput::PadButtonStart);
        input_map_->MapFloat(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::Up),    gamepad_id_, gainput::PadButtonLeftStickY);
        input_map_->MapFloat(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::Down),  gamepad_id_, gainput::PadButtonLeftStickY);
        input_map_->MapFloat(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::Left),  gamepad_id_, gainput::PadButtonLeftStickX);
        input_map_->MapFloat(magic_enum::enum_integer<Joypads::Button>(Joypads::Button::Right), gamepad_id_, gainput::PadButtonLeftStickX);
    }

    bool is_dir_pad_x(Joypads::Button button)
    {
        return button == Joypads::Button::Right ||
               button == Joypads::Button::Left;
    }

    bool is_dir_pad_y(Joypads::Button button)
    {
        return button == Joypads::Button::Up ||
               button == Joypads::Button::Down;
    }

    bool is_button_pressed(Joypads::Button button)
    {
        input_manager_->Update();

        bool result = false;

        if (is_dir_pad_x(button))
        {
            if (button == Joypads::Button::Left)
            {
                result = input_map_->GetFloat(magic_enum::enum_integer<Joypads::Button>(button)) < 0;
            }
            else
            {
                result = input_map_->GetFloat(magic_enum::enum_integer<Joypads::Button>(button)) > 0;
            }
            return result;
        }
        if (is_dir_pad_y(button))
        {
            if (button == Joypads::Button::Up)
            {
                result = input_map_->GetFloat(magic_enum::enum_integer<Joypads::Button>(button)) > 0;
            }
            else
            {
                result = input_map_->GetFloat(magic_enum::enum_integer<Joypads::Button>(button)) < 0;
            }
            return result;
        }

        return input_map_->GetBool(magic_enum::enum_integer<Joypads::Button>(button));
    }

private:
    gainput::InputManager* input_manager_;
    gainput::DeviceId gamepad_id_;
    gainput::InputMap* input_map_;
};

JoypadInput* joypad_input = nullptr;

void init_joypad_input()
{
    joypad_input = new JoypadInput;
}

bool is_button_pressed(Joypads::Button button)
{
    static const std::unordered_map<Joypads::Button, int> button_to_key_map = {
        { Joypads::Button::A,       Qt::Key::Key_X },
        { Joypads::Button::B,       Qt::Key::Key_Z },
        { Joypads::Button::Select,  Qt::Key::Key_V },
        { Joypads::Button::Start,   Qt::Key::Key_B },
        { Joypads::Button::Up,      Qt::Key::Key_Up },
        { Joypads::Button::Down,    Qt::Key::Key_Down },
        { Joypads::Button::Left,    Qt::Key::Key_Left },
        { Joypads::Button::Right,   Qt::Key::Key_Right }
    };
    UIContext& ui = UIContext::instance();


    return ui.main_window->is_key_pressed(button_to_key_map.at(button)) ||
           joypad_input->is_button_pressed(button) ||
           ui.nes->agent_interface()->is_button_pressed(button);
}
