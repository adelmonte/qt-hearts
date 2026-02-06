import QtQuick

Item {
    id: cardBack

    property real cardWidth: 80
    property real cardHeight: 116

    width: cardWidth + 4
    height: cardHeight + 4

    Image {
        anchors.centerIn: parent
        width: cardWidth
        height: cardHeight
        source: "image://cards/back?v=" + gameBridge.themeVersion
        sourceSize.width: Math.round(cardWidth)
        sourceSize.height: Math.round(cardHeight)
        smooth: true
        antialiasing: true
        cache: true
    }
}
