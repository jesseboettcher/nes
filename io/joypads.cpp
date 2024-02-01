#include "io/joypads.hpp"

#include "lib/magic_enum.hpp"
#include "platform/ui_properties.hpp"
#include "processor/memory.hpp"

#include <glog/logging.h>

Joypads::Joypads(Processor6502& processor)
 : processor_(processor)
{
    AddressBus &m = processor_.memory();
    m.add_peripheral_connection(JOYPAD1, std::bind(&Joypads::read_joypad_1_callback, this));
    m.add_peripheral_connection(JOYPAD2, std::bind(&Joypads::read_joypad_2_callback, this));
}

void Joypads::step()
{
    // memory view bypasses the peripheral connection, so we can see
    // what the program has written to memory instead of getting what
    // is provided by this peripheral
    const AddressBus::View memory = processor_.memory().view(JOYPAD1, 2);

    bool strobe_last = strobe_bit_;
    strobe_bit_ = memory[JOYPAD1] & 0x01;

    if (strobe_bit_)
    {
        while (joypad_1_snapshot_.size()) { joypad_1_snapshot_.pop(); }
        while (joypad_2_snapshot_.size()) { joypad_2_snapshot_.pop(); }

        // grab state of all the buttons
        for (Button b : magic_enum::enum_values<Button>())
        {
            bool last = map_[b];
            map_[b] = is_button_pressed(b);

            // if (last != map_[b])
            // {
            //     if (map_[b])
            //     {
            //         LOG(INFO) << magic_enum::enum_name<Button>(b) << " pressed ";
            //     }
            //     else
            //     {
            //         LOG(INFO) << magic_enum::enum_name<Button>(b) << " released ";
            //     }
            // }
            // add the buttons in the order they must be returned
            // (element 0 will need to be returned first)
            joypad_1_snapshot_.push(is_button_pressed(b));
            joypad_1_snapshot_label_.push(b);
            joypad_2_snapshot_.push(false); // TODO joypad 2 unimplemented
        }
    }
}

uint8_t Joypads::read(std::queue<bool>& snapshot)
{
    if (strobe_bit_)
    {
        // return status of A button
        return is_button_pressed(Button::A);
    }

    static uint8_t bits = 0;

    if (snapshot.size() == 0)
    {
        bits = 0;
        return 0x01; // nothing left to read, std nes controllers continue to return 1
    }

    bool result = snapshot.front();
    Button b = joypad_1_snapshot_label_.front();
    snapshot.pop();
    joypad_1_snapshot_label_.pop();

    bits = (bits << 1) | result;

    // LOG(INFO) << "reading joypad button " << magic_enum::enum_name<Button>(b) << " value " << result;

    // if (snapshot.size() == 0)
    // {
    //     LOG(INFO) << "returned last joypad bit: " << std::hex << static_cast<int32_t>(bits);
    // }


    return result;
}

uint8_t Joypads::read_joypad_1_callback()
{
    return read(joypad_1_snapshot_);
}

uint8_t Joypads::read_joypad_2_callback()
{
    return 0;
    // return read(joypad_2_snapshot_);
}
