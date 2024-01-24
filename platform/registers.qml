import QtQuick
import QtQuick.Controls

ApplicationWindow
{
    id: registers_window
    objectName: "registers_window"
    width: 350
    height: 165
    visible: true
    title: qsTr("Registers")

    property string defaultFontFamily: "Courier"

    property var text_elements: [
        pc_label,
        a_reg_label,
        x_reg_label,
        y_reg_label,
        sp_reg_label,
        current_instruction_label,
        state_label
    ]

    function updateText(key)
    {
        for (let element of text_elements)
        {
            if (element.textKey === key)
            {
                element.text = TextController.get_text(key);
                element.color = TextController.get_color(key);
            }
        }
    }

    // Status names
    Item
    {
        width: 70

        Text
        {
            y: 15
            font.bold: true
            font.family: defaultFontFamily
            text: "State: "
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            y: 35
            font.bold: true
            font.family: defaultFontFamily
            text: "PC: "
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            y: 55
            font.bold: true
            font.family: defaultFontFamily
            text: " A: "
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            y: 75
            font.bold: true
            font.family: defaultFontFamily
            text: " X: "
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            y: 95
            font.bold: true
            font.family: defaultFontFamily
            text: " Y: "
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            y: 115
            font.bold: true
            font.family: defaultFontFamily
            text: "SP: "
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
    }

    // Status content
    Item
    {
        x: 70
        Text
        {
            id: state_label
            property string textKey: "state_label"

            y: 15
            font.bold: true
            color: "#000000"
            font.family: defaultFontFamily
            text: TextController.get_text(state_label.textKey)

            Component.onCompleted: { TextController.text_changed.connect(updateText) }
        }

        Text
        {
            id: pc_label
            property string textKey: "pc_label"

            y: 35
            font.bold: true
            font.family: defaultFontFamily
            text: TextController.get_text(pc_label.textKey)

            Component.onCompleted: { TextController.text_changed.connect(updateText) }
        }
        Text
        {
            id: a_reg_label
            property string textKey: "a_reg_label"

            y: 55
            font.bold: true
            font.family: defaultFontFamily
            text: TextController.get_text(a_reg_label.textKey)

            Component.onCompleted: { TextController.text_changed.connect(updateText) }
        }

        Text
        {
            id: x_reg_label
            property string textKey: "x_reg_label"

            y: 75
            font.bold: true
            font.family: defaultFontFamily
            text: TextController.get_text(x_reg_label.textKey)

            Component.onCompleted: { TextController.text_changed.connect(updateText) }
        }

        Text
        {
            id: y_reg_label
            property string textKey: "y_reg_label"

            y: 95
            font.bold: true
            font.family: defaultFontFamily
            text: TextController.get_text(y_reg_label.textKey)

            Component.onCompleted: { TextController.text_changed.connect(updateText) }
        }

        Text
        {
            id: sp_reg_label
            property string textKey: "sp_reg_label"

            y: 115
            font.bold: true
            font.family: defaultFontFamily
            text: TextController.get_text(sp_reg_label.textKey)

            Component.onCompleted: { TextController.text_changed.connect(updateText) }
        }
    }
    Text
    {
        id: current_instruction_label
        property string textKey: "current_instruction_label"

        x: 20
        y: 135
        font.bold: true
        font.family: defaultFontFamily
        text: TextController.get_text(current_instruction_label.textKey)

        Component.onCompleted: { TextController.text_changed.connect(updateText) }
    }
}
