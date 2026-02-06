import QtQuick

Rectangle {
    id: messageBanner

    property string message: gameBridge.message

    width: messageText.width + 30
    height: messageText.height + 10
    radius: 6
    color: "#b4000000"
    visible: message.length > 0
    opacity: visible ? 1 : 0

    Behavior on opacity {
        NumberAnimation { duration: 150 }
    }

    Text {
        id: messageText
        anchors.centerIn: parent
        text: message
        color: "white"
        font.pixelSize: 16
        font.bold: true
    }
}
