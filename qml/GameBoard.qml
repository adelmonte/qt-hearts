import QtQuick
import QtQuick.Controls

Item {
    id: gameBoard

    // Card sizing (rounded to integer pixels)
    readonly property real baseCardWidth: 80
    readonly property real baseCardHeight: 116
    readonly property real windowScale: Math.min(width / 1000, height / 750)
    readonly property real effectiveScale: Math.max(0.6, Math.min(windowScale, 1.5)) * gameBridge.cardScale
    readonly property real cardWidth: Math.round(baseCardWidth * effectiveScale)
    readonly property real cardHeight: Math.round(baseCardHeight * effectiveScale)
    readonly property real cardSpacing: Math.round(22 * effectiveScale)

    // Background radial gradient
    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            var cx = width / 2
            var cy = height / 2
            var radius = Math.max(width, height) * 0.7
            var gradient = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius)
            gradient.addColorStop(0.0, "#2d822d")
            gradient.addColorStop(0.5, "#236923")
            gradient.addColorStop(1.0, "#195019")
            ctx.fillStyle = gradient
            ctx.fillRect(0, 0, width, height)
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    // Opponent hands - declared before scoreboards for z-order
    OpponentHand {
        id: westHand
        playerIndex: 1
        cardCount: gameBridge.opponentCardCounts[0] || 0
        x: 20
        y: Math.round((parent.height - height) / 2)
        cardWidth: gameBoard.cardWidth
        cardHeight: gameBoard.cardHeight
        cardSpacing: Math.round(gameBoard.cardSpacing * 0.6)
    }

    OpponentHand {
        id: northHand
        playerIndex: 2
        cardCount: gameBridge.opponentCardCounts[1] || 0
        x: Math.round((parent.width - width) / 2)
        y: 20
        cardWidth: gameBoard.cardWidth
        cardHeight: gameBoard.cardHeight
        cardSpacing: Math.round(gameBoard.cardSpacing * 0.6)
    }

    OpponentHand {
        id: eastHand
        playerIndex: 3
        cardCount: gameBridge.opponentCardCounts[2] || 0
        x: parent.width - width - 20
        y: Math.round((parent.height - height) / 2)
        cardWidth: gameBoard.cardWidth
        cardHeight: gameBoard.cardHeight
        cardSpacing: Math.round(gameBoard.cardSpacing * 0.6)
    }

    // Trick area
    TrickArea {
        id: trickArea
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2) - 30
        cardWidth: gameBoard.cardWidth
        cardHeight: gameBoard.cardHeight
    }

    // Player hand
    PlayerHand {
        id: playerHand
        x: Math.round((parent.width - width) / 2)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        cardWidth: gameBoard.cardWidth
        cardHeight: gameBoard.cardHeight
        cardSpacing: gameBoard.cardSpacing
    }

    // Scoreboards (z: 100 to render above card backs)
    Scoreboard {
        id: southScore
        playerIndex: 0
        z: 100
        x: Math.round((parent.width - width) / 2)
        y: parent.height - 120 - gameBoard.cardHeight
    }

    Scoreboard {
        id: westScore
        playerIndex: 1
        z: 100
        x: 100
        y: Math.round((parent.height - height) / 2)
    }

    Scoreboard {
        id: northScore
        playerIndex: 2
        z: 100
        x: Math.round((parent.width - width) / 2)
        y: 90
    }

    Scoreboard {
        id: eastScore
        playerIndex: 3
        z: 100
        x: parent.width - 200
        y: Math.round((parent.height - height) / 2)
    }

    // Pass direction arrow
    PassArrow {
        id: passArrow
        z: 400
        x: Math.round((parent.width - width) / 2)
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0.3
        visible: gameBridge.passDirection < 3 && gameBridge.gameState === 3 // WaitingForPass
    }

    // Message banner
    MessageBanner {
        id: messageBanner
        z: 500
        x: Math.round((parent.width - width) / 2)
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0.4
    }

    // Game over overlay
    GameOverlay {
        id: gameOverlay
        z: 900
        anchors.fill: parent
        visible: gameBridge.gameOver
    }

    // Card pass animation overlay
    Item {
        id: receivedCardsOverlay
        z: 800
        anchors.fill: parent

        function opponentPosition() {
            var dir = gameBridge.passDirection
            if (dir === 0) {
                return Qt.point(eastHand.x + gameBoard.cardHeight / 2, eastHand.y + eastHand.height / 2)
            }
            if (dir === 1) {
                return Qt.point(westHand.x + westHand.width - gameBoard.cardHeight / 2, westHand.y + westHand.height / 2)
            }
            return Qt.point(northHand.x + northHand.width / 2, northHand.y + northHand.height - gameBoard.cardHeight / 2)
        }

        function opponentRotation() {
            var dir = gameBridge.passDirection
            if (dir === 0) return -90  // East
            if (dir === 1) return 90   // West
            return 180                  // North
        }

        property var animCards: []

        function clearAnimCards() {
            for (var i = 0; i < animCards.length; i++) {
                if (animCards[i]) animCards[i].destroy()
            }
            animCards = []
        }

        Connections {
            target: gameBridge
            function onCardsReceived(cards) {
                if (!gameBridge.animatePassingCards) return

                receivedCardsOverlay.clearAnimCards()
                playerHand.animatingReceived = true

                var startPos = receivedCardsOverlay.opponentPosition()
                var startRot = receivedCardsOverlay.opponentRotation()

                var handCards = gameBridge.playerHand
                var handCount = handCards ? handCards.length : 0

                for (var i = 0; i < cards.length; i++) {
                    var c = cards[i]

                    var component = Qt.createComponent("CardItem.qml")
                    if (component.status === Component.Ready) {
                        var handIndex = Math.floor(handCount / 2)
                        for (var k = 0; k < handCount; k++) {
                            if (handCards[k].suit === c.suit && handCards[k].rank === c.rank) {
                                handIndex = k
                                break
                            }
                        }

                        var targetX = playerHand.x + handIndex * gameBoard.cardSpacing
                        var targetY = playerHand.y

                        var card = component.createObject(receivedCardsOverlay, {
                            "suit": c.suit,
                            "rank": c.rank,
                            "elementId": c.elementId,
                            "faceUp": false,
                            "playable": false,
                            "received": false,
                            "cardWidth": gameBoard.cardWidth,
                            "cardHeight": gameBoard.cardHeight,
                            "enableBehaviors": false,
                            "x": startPos.x - gameBoard.cardWidth / 2,
                            "y": startPos.y - gameBoard.cardHeight / 2,
                            "z": 800 + i,
                            "rotation": startRot,
                            "opacity": 1.0
                        })

                        receivedCardsOverlay.animCards.push(card)

                        receivedCardAnim.createObject(card, {
                            "targetX": targetX,
                            "targetY": targetY
                        })
                    }
                }

                clearAnimTimer.interval = 350
                clearAnimTimer.start()
            }
        }

        Timer {
            id: clearAnimTimer
            onTriggered: {
                playerHand.animatingReceived = false
                receivedCardsOverlay.clearAnimCards()
            }
        }

        Component {
            id: receivedCardAnim
            Item {
                id: animCtrl
                property real targetX
                property real targetY

                // Flight animation
                ParallelAnimation {
                    running: true
                    NumberAnimation {
                        target: animCtrl.parent; property: "x"
                        to: animCtrl.targetX; duration: 280
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        target: animCtrl.parent; property: "y"
                        to: animCtrl.targetY; duration: 280
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        target: animCtrl.parent; property: "rotation"
                        to: 0; duration: 280
                        easing.type: Easing.OutCubic
                    }
                }

                // Crossfade flip
                SequentialAnimation {
                    running: true
                    PauseAnimation { duration: 100 }
                    NumberAnimation {
                        target: animCtrl.parent; property: "opacity"
                        to: 0; duration: 60
                        easing.type: Easing.InQuad
                    }
                    ScriptAction {
                        script: if (animCtrl.parent) animCtrl.parent.faceUp = true
                    }
                    NumberAnimation {
                        target: animCtrl.parent; property: "opacity"
                        to: 1.0; duration: 60
                        easing.type: Easing.OutQuad
                    }
                }
            }
        }
    }

    // Card navigation keyboard shortcuts
    Shortcut {
        sequence: "Left"
        enabled: !gameBridge.inputBlocked
        onActivated: playerHand.moveLeft()
    }
    Shortcut {
        sequence: "Right"
        enabled: !gameBridge.inputBlocked
        onActivated: playerHand.moveRight()
    }
    Shortcut {
        sequence: "Return"
        enabled: !gameBridge.inputBlocked
        onActivated: playerHand.selectFocused()
    }
    Shortcut {
        sequence: "Enter"
        enabled: !gameBridge.inputBlocked
        onActivated: playerHand.selectFocused()
    }
    Shortcut {
        sequence: "Space"
        enabled: !gameBridge.inputBlocked
        onActivated: playerHand.selectFocused()
    }
    Shortcut {
        sequence: "Home"
        enabled: !gameBridge.inputBlocked
        onActivated: playerHand.goToFirst()
    }
    Shortcut {
        sequence: "End"
        enabled: !gameBridge.inputBlocked
        onActivated: playerHand.goToLast()
    }
}
