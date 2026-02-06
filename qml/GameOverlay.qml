import QtQuick
import QtQuick.Controls

Rectangle {
    id: gameOverlay

    property bool isWinner: gameBridge.winner === 0
    property string winnerName: {
        var players = gameBridge.players
        var winner = gameBridge.winner
        if (winner >= 0 && winner < players.length) {
            return players[winner].name
        }
        return ""
    }

    color: "#b4000000"
    opacity: visible ? 1 : 0

    Behavior on opacity {
        NumberAnimation { duration: 300 }
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: isWinner ? "You Win!" : winnerName + " Wins"
            color: isWinner ? "#64ff64" : "#ff6464"
            font.pixelSize: 48
            font.bold: true
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "New Game"
            font.pixelSize: 18
            onClicked: gameBridge.newGame()

            background: Rectangle {
                implicitWidth: 150
                implicitHeight: 50
                radius: 8
                color: parent.down ? "#1a5c1a" : (parent.hovered ? "#2d8c2d" : "#237823")
            }

            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
