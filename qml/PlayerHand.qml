import QtQuick

Item {
    id: playerHand

    property real cardWidth: 80
    property real cardHeight: 116
    property real cardSpacing: 22

    property var cards: gameBridge.playerHand
    property int cardCount: cards ? cards.length : 0

    // Track keyboard focus
    property int keyboardFocusIndex: -1

    // True while the fly-in animation is playing for received cards
    property bool animatingReceived: false

    width: cardCount > 0 ? (cardCount - 1) * cardSpacing + cardWidth + 4 : 0
    height: cardHeight + 12

    // Reset keyboard focus when cards change
    onCardCountChanged: {
        if (keyboardFocusIndex >= cardCount) {
            keyboardFocusIndex = cardCount > 0 ? cardCount - 1 : -1
        }
    }

    // Clear keyboard focus when game state changes (e.g., after passing)
    Connections {
        target: gameBridge
        function onGameStateChanged() {
            playerHand.keyboardFocusIndex = -1
        }
        function onCardsReceived(cards) {
            // Clear keyboard focus when receiving cards
            playerHand.keyboardFocusIndex = -1
        }
    }

    // Keyboard navigation functions
    function moveLeft() {
        if (cardCount === 0) return
        if (keyboardFocusIndex <= 0) {
            keyboardFocusIndex = cardCount - 1
        } else {
            keyboardFocusIndex--
        }
    }

    function moveRight() {
        if (cardCount === 0) return
        if (keyboardFocusIndex >= cardCount - 1) {
            keyboardFocusIndex = 0
        } else {
            keyboardFocusIndex++
        }
    }

    function selectFocused() {
        if (keyboardFocusIndex >= 0 && keyboardFocusIndex < cardCount) {
            var cardData = cards[keyboardFocusIndex]
            if (cardData.playable) {
                gameBridge.cardClicked(cardData.suit, cardData.rank)
            }
        }
    }

    function goToFirst() {
        if (cardCount > 0) keyboardFocusIndex = 0
    }

    function goToLast() {
        if (cardCount > 0) keyboardFocusIndex = cardCount - 1
    }

    Repeater {
        model: cards

        CardItem {
            property var cardData: modelData

            suit: cardData.suit
            rank: cardData.rank
            elementId: cardData.elementId
            faceUp: true
            playable: cardData.playable
            selected: cardData.selected
            received: cardData.received
            keyboardFocused: index === playerHand.keyboardFocusIndex
            cardWidth: playerHand.cardWidth
            cardHeight: playerHand.cardHeight

            // Hidden while pass animation overlay is active
            visible: !(received && playerHand.animatingReceived)

            // Cards slide together when one is removed
            x: index * playerHand.cardSpacing
            y: 0
            z: index

            // Received cards push up with wobble after fly-in
            transform: [
                Translate {
                    y: (received && !playerHand.animatingReceived) ? -20 : 0

                    Behavior on y {
                        NumberAnimation {
                            duration: 80
                            easing.type: Easing.OutQuad
                        }
                    }
                },
                Rotation {
                    angle: (received && !playerHand.animatingReceived) ? ((index % 2 === 0) ? 5 : -5) : 0
                    origin.x: (playerHand.cardWidth + 4) / 2
                    origin.y: (playerHand.cardHeight + 4) / 2

                    Behavior on angle {
                        NumberAnimation {
                            duration: 80
                            easing.type: Easing.OutQuad
                        }
                    }
                }
            ]
        }
    }
}
