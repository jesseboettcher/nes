import QtQuick
import QtQuick.Controls
import com.boettcher.jesse 0.1

UIWindow
{
    id: registers_window
    objectName: "registers_window"
    width: 280
    height: 185
    visible: true
    color: "#F8F8FB"
    title: qsTr("Registers")

    property string defaultFontFamily: "Courier"

    property var text_elements: [
        pc_label,
        a_reg_label,
        x_reg_label,
        y_reg_label,
        sr_reg_label,
        sp_reg_label,

        n_flag_label,
        o_flag_label,
        b_flag_label,
        i_flag_label,
        z_flag_label,
        c_flag_label,

        state_label
    ]

    function updateText(key)
    {
        for (let element of text_elements)
        {
            if (element.textKey === key)
            {
                element.text = UIController.get_text(key);
                element.color = UIController.get_color(key);
            }
        }
    }

    function updateColor(key)
    {
        for (let element of text_elements)
        {
            if (element.textKey === key)
            {
                element.color = UIController.get_color(key);
            }
        }
    }

    Rectangle
    {
        x: 0
        y: 0
        width: 68
        height: parent.height

        color: "#E5EEF4"
        anchors.bottom: parent.bottom
    }

    // Register names
    Column
    {
        y: 10
        width: 70
        spacing: 5

        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: "State: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: "PC: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: " A: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: " X: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: " Y: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: "SP: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: "SR: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
        Text
        {
            font.bold: true
            font.family: defaultFontFamily
            text: "Flags: "
            color: "#5F7784"
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
        }
    }

    // Register content
    Column
    {
        y: 10
        x: 73
        spacing: 5

        Text
        {
            id: state_label
            property string textKey: "state_label"

            font.bold: true
            color: "#455760"
            font.family: defaultFontFamily
            text: UIController.get_text(state_label.textKey)

            Component.onCompleted: { UIController.ui_changed.connect(updateText) }
        }

        Text
        {
            id: pc_label
            property string textKey: "pc_label"

            font.bold: true
            color: "#455760"
            font.family: defaultFontFamily
            text: UIController.get_text(pc_label.textKey)

            Component.onCompleted: { UIController.ui_changed.connect(updateText) }
        }
        Text
        {
            id: a_reg_label
            property string textKey: "a_reg_label"

            font.bold: true
            color: "#455760"
            font.family: defaultFontFamily
            text: UIController.get_text(a_reg_label.textKey)

            Component.onCompleted: { UIController.ui_changed.connect(updateText) }
        }

        Text
        {
            id: x_reg_label
            property string textKey: "x_reg_label"

            font.bold: true
            color: "#455760"
            font.family: defaultFontFamily
            text: UIController.get_text(x_reg_label.textKey)

            Component.onCompleted: { UIController.ui_changed.connect(updateText) }
        }

        Text
        {
            id: y_reg_label
            property string textKey: "y_reg_label"

            font.bold: true
            color: "#455760"
            font.family: defaultFontFamily
            text: UIController.get_text(y_reg_label.textKey)

            Component.onCompleted: { UIController.ui_changed.connect(updateText) }
        }

        Text
        {
            id: sp_reg_label
            property string textKey: "sp_reg_label"

            font.bold: true
            color: "#455760"
            font.family: defaultFontFamily
            text: UIController.get_text(sp_reg_label.textKey)

            Component.onCompleted: { UIController.ui_changed.connect(updateText) }
        }

        Text
        {
            id: sr_reg_label
            property string textKey: "sr_reg_label"

            font.bold: true
            color: "#455760"
            font.family: defaultFontFamily
            text: UIController.get_text(sr_reg_label.textKey)

            Component.onCompleted: { UIController.ui_changed.connect(updateText) }
        }

        Row
        {
            spacing: 6

            Text
            {
                id: n_flag_label
                property string textKey: "n_flag_label"

                font.bold: true
                color: "#BBBBBB"
                font.family: defaultFontFamily
                text: "N"

                Component.onCompleted: { UIController.ui_changed.connect(updateColor) }
            }
            Text
            {
                id: o_flag_label
                property string textKey: "o_flag_label"

                font.bold: true
                color: "#BBBBBB"
                font.family: defaultFontFamily
                text: "O"

                Component.onCompleted: { UIController.ui_changed.connect(updateColor) }
            }
            Text
            {
                id: b_flag_label
                property string textKey: "b_flag_label"

                font.bold: true
                color: "#BBBBBB"
                font.family: defaultFontFamily
                text: "B"

                Component.onCompleted: { UIController.ui_changed.connect(updateColor) }
            }
            Text
            {
                id: i_flag_label
                property string textKey: "i_flag_label"

                font.bold: true
                color: "#BBBBBB"
                font.family: defaultFontFamily
                text: "I"

                Component.onCompleted: { UIController.ui_changed.connect(updateColor) }
            }
            Text
            {
                id: z_flag_label
                property string textKey: "z_flag_label"

                font.bold: true
                color: "#BBBBBB"
                font.family: defaultFontFamily
                text: "Z"

                Component.onCompleted: { UIController.ui_changed.connect(updateColor) }
            }
            Text
            {
                id: c_flag_label
                property string textKey: "c_flag_label"

                font.bold: true
                color: "#BBBBBB"
                font.family: defaultFontFamily
                text: "C"

                Component.onCompleted: { UIController.ui_changed.connect(updateColor) }
            }
        }
    }
}
