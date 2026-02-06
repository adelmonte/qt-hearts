import QtQuick

Item {
    id: trickArea

    property real cardWidth: 80
    property real cardHeight: 116

    property var cardItems: []
    property var cardPlayers: []
    property int expectedCardCount: 0
    property var exitingCards: []

    width: cardWidth * 3
    height: cardHeight * 3

    onCardWidthChanged: relayoutTrickCards()
    onCardHeightChanged: relayoutTrickCards()

    function relayoutTrickCards() {
        for (var i = 0; i < cardItems.length; i++) {
            var card = cardItems[i]
            var player = cardPlayers[i]
            if (card && player !== undefined) {
                card.cardWidth = trickArea.cardWidth
                card.cardHeight = trickArea.cardHeight
                card.x = cardX(player)
                card.y = cardY(player)
            }
        }
    }

    // Card positions per player
    function cardX(player) {
        switch (player) {
            case 0: return Math.round((width - cardWidth) / 2)
            case 1: return Math.round((width - cardWidth) / 2 - cardHeight * 0.7)
            case 2: return Math.round((width - cardWidth) / 2)
            case 3: return Math.round((width - cardWidth) / 2 + cardHeight * 0.7)
        }
        return Math.round((width - cardWidth) / 2)
    }

    function cardY(player) {
        switch (player) {
            case 0: return Math.round(height / 2 + cardHeight * 0.35)
            case 1: return Math.round((height - cardHeight) / 2)
            case 2: return Math.round((height - cardHeight) / 2 - cardHeight * 0.7)
            case 3: return Math.round((height - cardHeight) / 2)
        }
        return Math.round((height - cardHeight) / 2)
    }

    // Start positions for fly-in animation
    function startX(player) {
        switch (player) {
            case 0: return (width - cardWidth) / 2           // South: same x
            case 1: return -cardWidth * 2                    // West: from left
            case 2: return (width - cardWidth) / 2           // North: same x
            case 3: return width + cardWidth                 // East: from right
        }
        return (width - cardWidth) / 2
    }

    function startY(player) {
        switch (player) {
            case 0: return height + cardHeight               // South: from bottom
            case 1: return (height - cardHeight) / 2         // West: same y
            case 2: return -cardHeight * 2                   // North: from top
            case 3: return (height - cardHeight) / 2         // East: same y
        }
        return (height - cardHeight) / 2
    }

    // Exit positions for trick-won animation
    function exitX(winner) {
        switch (winner) {
            case 0: return (width - cardWidth) / 2
            case 1: return -cardWidth * 2
            case 2: return (width - cardWidth) / 2
            case 3: return width + cardWidth
        }
        return (width - cardWidth) / 2
    }

    function exitY(winner) {
        switch (winner) {
            case 0: return height + cardHeight
            case 1: return (height - cardHeight) / 2
            case 2: return -cardHeight * 2
            case 3: return (height - cardHeight) / 2
        }
        return (height - cardHeight) / 2
    }

    function clearTrickCards() {
        for (var i = 0; i < cardItems.length; i++) {
            if (cardItems[i]) {
                cardItems[i].destroy()
            }
        }
        cardItems = []
        cardPlayers = []
        expectedCardCount = 0
    }

    function destroyExitingCards() {
        for (var i = 0; i < exitingCards.length; i++) {
            if (exitingCards[i]) {
                exitingCards[i].destroy()
            }
        }
        exitingCards = []
    }

    function addCard(player, suit, rank, elementId) {
        var component = Qt.createComponent("CardItem.qml")
        if (component.status === Component.Ready) {
            var card = component.createObject(trickArea, {
                "suit": suit,
                "rank": rank,
                "elementId": elementId,
                "faceUp": player === 0,  // AI cards start face-down
                "playable": false,
                "inTrick": true,
                "cardWidth": trickArea.cardWidth,
                "cardHeight": trickArea.cardHeight,
                "enableBehaviors": false,
                "x": startX(player),
                "y": startY(player),
                "z": cardItems.length,
                "rotation": gameBridge.animateCardRotation ? (Math.floor(Math.random() * 11) - 5) : 0
            })

            cardItems.push(card)
            cardPlayers.push(player)

            var targetX = cardX(player)
            var targetY = cardY(player)

            entryAnim.createObject(card, {
                "targetX": targetX,
                "targetY": targetY,
                "isAI": player !== 0
            })
        }
    }

    Component {
        id: entryAnim
        Item {
            id: animController
            property real targetX
            property real targetY
            property bool isAI: false

            ParallelAnimation {
                id: moveAnim
                running: true
                NumberAnimation {
                    target: animController.parent
                    property: "x"
                    to: animController.targetX
                    duration: 180
                    easing.type: Easing.OutQuad
                }
                NumberAnimation {
                    target: animController.parent
                    property: "y"
                    to: animController.targetY
                    duration: 180
                    easing.type: Easing.OutQuad
                }
            }

            // Flip AI cards face-up partway through
            Timer {
                running: animController.isAI
                interval: 60
                onTriggered: {
                    if (animController.parent) {
                        animController.parent.faceUp = true
                    }
                }
            }

            // Clean up after animation
            Timer {
                running: true
                interval: 200
                onTriggered: animController.destroy()
            }
        }
    }

    Connections {
        target: gameBridge
        function onCardPlayedToTrick(player, suit, rank, fromX, fromY) {
            trickArea.expectedCardCount++
            var tricks = gameBridge.trickCards
            if (tricks.length > 0) {
                var lastCard = tricks[tricks.length - 1]
                trickArea.addCard(player, suit, rank, lastCard.elementId)
            }
        }
    }

    Connections {
        target: gameBridge
        function onTrickWonByPlayer(winner, points) {
            var cardsToExit = cardItems
            trickArea.exitingCards = trickArea.exitingCards.concat(cardsToExit)
            trickArea.cardItems = []
            trickArea.cardPlayers = []
            trickArea.expectedCardCount = 0

            var ex = trickArea.exitX(winner)
            var ey = trickArea.exitY(winner)

            for (var i = 0; i < cardsToExit.length; i++) {
                var item = cardsToExit[i]
                if (item) {
                    exitAnim.createObject(item, {
                        "exitX": ex,
                        "exitY": ey
                    })
                }
            }

            exitCleanupTimer.start()
        }
    }

    Timer {
        id: exitCleanupTimer
        interval: 250
        onTriggered: trickArea.destroyExitingCards()
    }

    Component {
        id: exitAnim
        Item {
            id: exitController
            property real exitX
            property real exitY

            ParallelAnimation {
                running: true
                NumberAnimation {
                    target: exitController.parent
                    property: "x"
                    to: exitController.exitX
                    duration: 200
                    easing.type: Easing.InQuad
                }
                NumberAnimation {
                    target: exitController.parent
                    property: "y"
                    to: exitController.exitY
                    duration: 200
                    easing.type: Easing.InQuad
                }
                NumberAnimation {
                    target: exitController.parent
                    property: "opacity"
                    to: 0
                    duration: 200
                }
            }

            Timer {
                running: true
                interval: 220
                onTriggered: exitController.destroy()
            }
        }
    }

    Connections {
        target: gameBridge
        function onTrickCardsChanged() {
            var modelCards = gameBridge.trickCards

            if (modelCards.length === 0 && cardItems.length > 0) {
                clearTrickCards()
                return
            }

            if (modelCards.length > 0 && modelCards.length < cardItems.length) {
                clearTrickCards()
            }

            if (modelCards.length <= expectedCardCount) {
                return
            }

            // Rebuild from model (game reload / undo)
            if (cardItems.length === 0 && expectedCardCount === 0 && modelCards.length > 0) {
                expectedCardCount = modelCards.length
                for (var i = 0; i < modelCards.length; i++) {
                    var c = modelCards[i]
                    var component = Qt.createComponent("CardItem.qml")
                    if (component.status === Component.Ready) {
                        var card = component.createObject(trickArea, {
                            "suit": c.suit,
                            "rank": c.rank,
                            "elementId": c.elementId,
                            "faceUp": true,
                            "playable": false,
                            "inTrick": true,
                            "cardWidth": trickArea.cardWidth,
                            "cardHeight": trickArea.cardHeight,
                            "enableBehaviors": false,
                            "x": cardX(c.player),
                            "y": cardY(c.player),
                            "z": i,
                            "rotation": gameBridge.animateCardRotation ? (Math.floor(Math.random() * 11) - 5) : 0
                        })
                        cardItems.push(card)
                        cardPlayers.push(c.player)
                    }
                }
            }
        }
    }
}
