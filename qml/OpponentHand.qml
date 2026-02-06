import QtQuick

Item {
    id: opponentHand

    property int playerIndex: 1
    property int cardCount: 0
    property real cardWidth: 80
    property real cardHeight: 116
    property real cardSpacing: 13

    // West (1): vertical, cards rotated 90°
    // North (2): horizontal, cards rotated 180°
    // East (3): vertical, cards rotated -90°
    width: {
        if (playerIndex === 2)
            return cardCount > 0 ? (cardCount - 1) * cardSpacing + cardWidth + 4 : 0
        else
            return cardHeight + 4
    }
    height: {
        if (playerIndex === 2)
            return cardHeight + 4
        else
            return cardCount > 0 ? (cardCount - 1) * cardSpacing + cardWidth + 4 : 0
    }

    Repeater {
        model: cardCount

        CardBack {
            cardWidth: opponentHand.cardWidth
            cardHeight: opponentHand.cardHeight
            transformOrigin: Item.TopLeft
            z: index

            rotation: {
                if (opponentHand.playerIndex === 1) return 90
                if (opponentHand.playerIndex === 2) return 180
                if (opponentHand.playerIndex === 3) return -90
                return 0
            }

            x: {
                if (opponentHand.playerIndex === 1)
                    return opponentHand.cardHeight + 4
                if (opponentHand.playerIndex === 2)
                    return index * opponentHand.cardSpacing + opponentHand.cardWidth + 4
                if (opponentHand.playerIndex === 3)
                    return 0
                return index * opponentHand.cardSpacing
            }

            y: {
                if (opponentHand.playerIndex === 1)
                    return index * opponentHand.cardSpacing
                if (opponentHand.playerIndex === 2)
                    return opponentHand.cardHeight + 4
                if (opponentHand.playerIndex === 3)
                    return index * opponentHand.cardSpacing + opponentHand.cardWidth + 4
                return 0
            }

            Behavior on x {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutQuad
                }
            }

            Behavior on y {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutQuad
                }
            }
        }
    }
}
