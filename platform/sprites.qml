import QtQuick
import QtQuick.Controls
import com.boettcher.jesse 0.1

UIWindow
{
    id: sprites_window
    objectName: "sprites_window"
    width: 300
    height: 365
    visible: true
    color: "#F8F8FB"
    title: qsTr("Sprites")

    property string defaultFontFamily: "Courier"

    property var text_elements: [
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
}
