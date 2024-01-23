import QtQuick
import QtQuick.Controls
import com.boettcher.jesse 0.1

Window {
    width: 512
    height: 480
    visible: true
    title: qsTr("Nintendo Entertainment System")

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
                onTriggered: menu_handler.load_rom();
            }
        }
        Menu
        {
            title: qsTr("Control")

            MenuItem
            {
                text: qsTr("Run Command...")
                onTriggered: menu_handler.run_command();
            }
        }
    }
}
