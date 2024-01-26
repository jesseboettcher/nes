import QtQuick
import QtQuick.Controls

Window
{
    id: memory_window
    objectName: "memory_window"
    width: 275
    height: 165
    visible: true
    color: "#F8F8FB"
    title: qsTr("Memory")

    property string defaultFontFamily: "Courier"

    property var text_elements: [
        address_view,
        memory_view
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

    Rectangle
    {
        x: 0
        y: 0
        width: 65
        height: parent.height

        color: "#E5EEF4"
        anchors.bottom: parent.bottom
    }

    ScrollView
    {
        objectName: "memory_scroll_view"
        width: parent.width
        height: parent.height
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        clip: true

        Row
        {
            width: parent.width
            spacing: 20

            Text
            {
                id: address_view
                property string textKey: "address_view"

                y: 5
                x: 0
                width: 55

                font.bold: true
                font.family: defaultFontFamily
                text: UIController.get_text(address_view.textKey)
                color: "#5F7784"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight

                Component.onCompleted: { UIController.ui_changed.connect(updateText) }
            }

            Text
            {
                id: memory_view
                property string textKey: "memory_view"

                y: 5
                width: 180
                height: contentHeight

                font.bold: true
                color: "#455760"
                wrapMode: Text.WordWrap
                font.family: defaultFontFamily
                text: UIController.get_text(memory_view.textKey)

                Component.onCompleted: { UIController.ui_changed.connect(updateText) }
            }
        }
    }
}
