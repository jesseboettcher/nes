#include "io/joypads.hpp"

#include "lib/magic_enum.hpp"
#include "platform/ui_properties.hpp"
#include "processor/address_bus.hpp"

#include <glog/logging.h>

static constexpr bool JOYPAD_DEBUG = false;

Joypads::Joypads()
{
    data(JOYPAD1) = 0x00;
    data(JOYPAD2) = 0x00;
}

void Joypads::step()
{
    // memory view bypasses the peripheral connection, so we can see
    // what the program has written to memory instead of getting what
    // is provided by this peripheral
    strobe_bit_ = data(JOYPAD1);

    if (strobe_bit_)
    {
        while (joypad_1_snapshot_.size()) { joypad_1_snapshot_.pop(); }
        while (joypad_2_snapshot_.size()) { joypad_2_snapshot_.pop(); }

        // grab state of all the buttons
        for (Button b : magic_enum::enum_values<Button>())
        {
            if constexpr (JOYPAD_DEBUG)
            {
                static std::unordered_map<Button, bool> map_;
                bool last = map_[b];

                map_[b] = is_button_pressed(b);

                if (last != map_[b])
                {
                    if (map_[b])
                    {
                        LOG(INFO) << magic_enum::enum_name<Button>(b) << " pressed ";
                    }
                    else
                    {
                        LOG(INFO) << magic_enum::enum_name<Button>(b) << " released ";
                    }
                }
            }

            // add the buttons in the order they must be returned
            // (element 0 will need to be returned first)
            joypad_1_snapshot_.push(is_button_pressed(b));
            joypad_2_snapshot_.push(false); // TODO joypad 2 unimplemented
        }
    }
}

uint8_t Joypads::read(uint16_t a) const
{
    if (a == 0x4016)
    {
        return read_joypad_1_callback();
    }
    else if (a == 0x4017)
    {
        return read_joypad_2_callback();
    }

    assert(false);
    return 0;
}

uint8_t& Joypads::write(uint16_t a)
{
    assert(a == JOYPAD1 || a == JOYPAD2);

    return data(a);
}

uint8_t Joypads::read(std::queue<bool>& snapshot) const
{
    if (strobe_bit_)
    {
        // return status of A button
        return is_button_pressed(Button::A);
    }

    if (snapshot.size() == 0)
    {
        return 0x01; // nothing left to read, std nes controllers continue to return 1
    }

    bool result = snapshot.front();
    snapshot.pop();

    return result;
}

uint8_t Joypads::read_joypad_1_callback() const
{
    return read(joypad_1_snapshot_);
}

uint8_t Joypads::read_joypad_2_callback() const
{
    return 0;
    return read(joypad_2_snapshot_);
}
