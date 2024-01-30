import QtQuick
import QtQuick.Controls
import com.boettcher.jesse 0.1

UIWindow
{
    id: sprites_window
    objectName: "sprites_window"
    width: 300
    height: 480
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

    Item
    {
        x: 10
        y: 10
        height: parent.height - 10
        width: parent.width - 10

        ListView
        {
            height: parent.height
            anchors.fill: parent

            model: SpritesModel

            delegate: Item
            {
                height: 25
                width: 200

                Rectangle
                {
                    width: parent.width
                    height: parent.height
                    color: index % 2 === 0 ? "#E5EEF4" : "#F8F8FB"
                }
                Row
                {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 20

                    Text
                    {
                        width: 20
                        font.bold: true
                        color: "#455760"
                        horizontalAlignment: Text.AlignRight
                        text: model.SpriteIndex
                    }
                    Text
                    {
                        width: 60
                        font.bold: true
                        color: "#455760"
                        text: model.SpritePosition
                    }
                    Text
                    {
                        width: 25
                        font.bold: true
                        color: "#455760"
                        text: model.SpriteAttributes
                    }
                    Text
                    {
                        width: 25
                        font.bold: true
                        color: "#455760"
                        text: model.SpriteTileIndex
                    }
                    Image
                    {
                        width: 16
                        height: 16
                        fillMode: Image.PreserveAspectFit
                        source: model.SpriteTileImage

                    }
                }
            }
        }
    }
}
