import QtQuick
import QtQuick.Controls
import com.boettcher.jesse 0.1

Item
{
    width: 512
    height: 480

    property var elements: [
        dimming_rect
    ]

    function update_ui(key)
    {
        for (let element of elements)
        {
            if (element.textKey === key)
            {
                element.opacity = UIController.get_opacity(key);
            }
        }
    }

    Rectangle
    {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: "cornflowerblue"
        height: parent.height
    }
    NesDisplayView
    {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
    }

    Rectangle
    {
        id: dimming_rect
        property string textKey: "dimming_rect"

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        color: "black"

        opacity: UIController.get_opacity(dimming_rect.textKey)
        Component.onCompleted: { UIController.ui_changed.connect(update_ui) }
}
}
