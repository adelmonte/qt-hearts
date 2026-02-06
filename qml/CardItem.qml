import QtQuick

Item {
    id: cardItem

    property int suit: 0
    property int rank: 0
    property string elementId: ""
    property bool faceUp: true
    property bool playable: false
    property bool selected: false
    property bool received: false
    property bool inTrick: false
    property bool keyboardFocused: false
    property real cardWidth: 80
    property real cardHeight: 116

    property bool enableBehaviors: true

    width: cardWidth + 4
    height: cardHeight + 4

    Behavior on x {
        enabled: enableBehaviors
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Behavior on y {
        enabled: enableBehaviors
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Behavior on rotation {
        enabled: enableBehaviors
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutQuad
        }
    }

    Behavior on opacity {
        enabled: enableBehaviors
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    // Card visual container
    Item {
        id: cardVisual
        anchors.centerIn: parent
        width: cardWidth
        height: cardHeight

        transform: Translate {
            id: raiseTranslate
            y: selected ? -15 : (((hoverArea.containsMouse || keyboardFocused) && playable) ? -8 : 0)

            Behavior on y {
                NumberAnimation {
                    duration: 100
                    easing.type: Easing.OutQuad
                }
            }
        }

        Image {
            id: cardImage
            anchors.fill: parent
            source: faceUp ? "image://cards/" + elementId + "?v=" + gameBridge.themeVersion
                           : "image://cards/back?v=" + gameBridge.themeVersion
            sourceSize.width: Math.round(cardWidth)
            sourceSize.height: Math.round(cardHeight)
            smooth: true
            antialiasing: true
            cache: true
            asynchronous: false
        }

        // Highlight border
        Rectangle {
            anchors.centerIn: parent
            width: cardWidth + 2
            height: cardHeight + 2
            radius: 6
            antialiasing: true
            color: "transparent"
            border.width: 2
            border.color: {
                if (received) return "#ffc832"
                if (selected) return "#3296ff"
                if (keyboardFocused && playable) return "#ffdc64"
                if (hoverArea.containsMouse && playable) return "#64c864"
                return "transparent"
            }
            visible: received || selected || ((hoverArea.containsMouse || keyboardFocused) && playable)

            Behavior on border.color {
                ColorAnimation { duration: 100 }
            }
        }

        // Dim non-playable cards
        Rectangle {
            anchors.fill: parent
            radius: 6
            antialiasing: true
            color: "#000000"
            opacity: (!playable && faceUp && !inTrick && !received) ? 0.235 : 0
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation { duration: 100 }
            }
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: playable ? Qt.PointingHandCursor : Qt.ArrowCursor

        onClicked: {
            if (playable) {
                gameBridge.cardClicked(suit, rank)
            }
        }
    }
}
