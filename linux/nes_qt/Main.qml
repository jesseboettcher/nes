import QtQuick
import QtQuick.Controls
import com.boettcher.jesse 0.1

Window {
    width: 512
    height: 480
    visible: true
    title: qsTr("Hello World")

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

    MenuBar
    {
        Menu
        {
            title: qsTr("File")

            MenuItem
            {
                text: qsTr("Load ROM...")
            }
        }
        Menu
        {
            title: qsTr("Control")

            MenuItem
            {
                text: qsTr("Run Command...")
            }
        }
    }
}
