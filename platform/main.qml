import QtQuick
import QtQuick.Controls
import com.boettcher.jesse 0.1

Item
{
    width: 512
    height: 480

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
}
