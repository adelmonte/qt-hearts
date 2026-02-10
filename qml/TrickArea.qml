import QtQuick

Item {
    id: trickArea

    property real cardWidth: 80
    property real cardHeight: 116

    property int expectedCardCount: 0
    property var exitingCards: []

    width: cardWidth * 3
    height: cardHeight * 3

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

    // --- Trick cards: ListModel + Repeater for reactive positioning ---

    ListModel {
        id: trickModel
    }

    Repeater {
        id: trickRepeater
        model: trickModel

        delegate: Item {
            id: del

            // Fly-in offset: starts at (startPos - targetPos), animates to 0.
            // The x/y bindings below always point at the final target, so after
            // the offset reaches 0, the card tracks resize changes natively.
            property real flyInOffsetX: model.noAnim ? 0
                : (trickArea.startX(model.player) - trickArea.cardX(model.player))
            property real flyInOffsetY: model.noAnim ? 0
                : (trickArea.startY(model.player) - trickArea.cardY(model.player))
            property bool flipDone: model.noAnim

            // Position — inlined to guarantee dependency tracking on
            // trickArea.width, trickArea.cardWidth, trickArea.cardHeight
            x: trickArea.cardX(model.player)
            y: trickArea.cardY(model.player)
            width: trickArea.cardWidth + 4
            height: trickArea.cardHeight + 4
            z: model.index
            rotation: model.rot

            // Visual offset for fly-in (does not break x/y bindings)
            transform: Translate {
                x: del.flyInOffsetX
                y: del.flyInOffsetY
            }

            Behavior on flyInOffsetX {
                NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
            }
            Behavior on flyInOffsetY {
                NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
            }

            CardItem {
                suit: model.suit
                rank: model.rank
                elementId: model.elId
                faceUp: model.player === 0 || del.flipDone
                playable: false
                inTrick: true
                cardWidth: trickArea.cardWidth
                cardHeight: trickArea.cardHeight
                enableBehaviors: false
            }

            Component.onCompleted: {
                if (!model.noAnim) {
                    // Kick off fly-in: animate offset from (start-target) to 0
                    flyInOffsetX = 0
                    flyInOffsetY = 0
                }
            }

            // Flip AI cards face-up partway through fly-in
            Timer {
                running: model.player !== 0 && !del.flipDone
                interval: 60
                onTriggered: del.flipDone = true
            }
        }
    }

    function addCard(player, suit, rank, elementId) {
        trickModel.append({
            "player": player,
            "suit": suit,
            "rank": rank,
            "elId": elementId,
            "rot": gameBridge.animateCardRotation ? (Math.floor(Math.random() * 11) - 5) : 0,
            "noAnim": false
        })
    }

    function clearTrickCards() {
        trickModel.clear()
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
            // Snapshot current cards as standalone items for exit animation
            var newExiting = []
            for (var i = 0; i < trickModel.count; i++) {
                var m = trickModel.get(i)
                var item = trickRepeater.itemAt(i)
                var posX = cardX(m.player)
                var posY = cardY(m.player)
                if (item) {
                    var pos = item.mapToItem(trickArea, 0, 0)
                    posX = pos.x
                    posY = pos.y
                }

                var component = Qt.createComponent("CardItem.qml")
                if (component.status === Component.Ready) {
                    var card = component.createObject(trickArea, {
                        "suit": m.suit,
                        "rank": m.rank,
                        "elementId": m.elId,
                        "faceUp": true,
                        "playable": false,
                        "inTrick": true,
                        "cardWidth": trickArea.cardWidth,
                        "cardHeight": trickArea.cardHeight,
                        "enableBehaviors": false,
                        "x": posX,
                        "y": posY,
                        "z": 10 + i,
                        "rotation": m.rot
                    })
                    newExiting.push(card)
                }
            }

            // Clear model — Repeater delegates vanish instantly
            trickModel.clear()
            trickArea.expectedCardCount = 0

            // Animate exit cards toward winner
            trickArea.exitingCards = trickArea.exitingCards.concat(newExiting)
            var ex = trickArea.exitX(winner)
            var ey = trickArea.exitY(winner)

            for (var j = 0; j < newExiting.length; j++) {
                if (newExiting[j]) {
                    exitAnim.createObject(newExiting[j], {
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

            if (modelCards.length === 0 && trickModel.count > 0) {
                clearTrickCards()
                return
            }

            if (modelCards.length > 0 && modelCards.length < trickModel.count) {
                clearTrickCards()
            }

            if (modelCards.length <= expectedCardCount) {
                return
            }

            // Rebuild from model (game reload / undo)
            if (trickModel.count === 0 && expectedCardCount === 0 && modelCards.length > 0) {
                expectedCardCount = modelCards.length
                for (var i = 0; i < modelCards.length; i++) {
                    var c = modelCards[i]
                    trickModel.append({
                        "player": c.player,
                        "suit": c.suit,
                        "rank": c.rank,
                        "elId": c.elementId,
                        "rot": gameBridge.animateCardRotation ? (Math.floor(Math.random() * 11) - 5) : 0,
                        "noAnim": true
                    })
                }
            }
        }
    }
}
