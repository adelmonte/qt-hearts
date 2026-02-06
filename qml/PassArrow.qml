import QtQuick

Text {
    id: passArrow

    property int direction: gameBridge.passDirection

    text: {
        switch (direction) {
            case 0: return "\u2190"  // Left arrow
            case 1: return "\u2192"  // Right arrow
            case 2: return "\u2191"  // Up arrow (across)
            default: return ""
        }
    }

    color: "#ccffffff"
    font.pixelSize: 48
    font.bold: true

    opacity: visible ? 1 : 0

    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }
}
