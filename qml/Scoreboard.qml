import QtQuick

Rectangle {
    id: scoreboard

    property int playerIndex: 0
    property var playerData: gameBridge.players[playerIndex]
    property string playerName: playerData ? playerData.name : ""
    property int score: playerData ? playerData.score : 0
    property bool isCurrentPlayer: playerData ? playerData.isCurrentPlayer : false

    width: 100
    height: 50
    radius: 4
    color: isCurrentPlayer ? "#dc283c28" : "#c81e1e1e"
    border.color: isCurrentPlayer ? "#64ff64" : "#505050"
    border.width: isCurrentPlayer ? 2 : 1

    Behavior on color {
        ColorAnimation { duration: 200 }
    }

    Behavior on border.color {
        ColorAnimation { duration: 200 }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 2

        Text {
            text: playerName
            color: "white"
            font.pixelSize: 11
            font.bold: true
        }

        Text {
            text: score.toString()
            color: "#ffdc50"
            font.pixelSize: 14
            font.bold: true
        }
    }
}
