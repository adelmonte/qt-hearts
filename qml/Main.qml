import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    GameBoard {
        anchors.fill: parent
    }

    // ===== Scores Dialog =====
    Dialog {
        id: scoresDialog
        title: qsTr("Scores")
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        width: 300
        modal: true

        ColumnLayout {
            spacing: 10
            anchors.fill: parent

            Repeater {
                model: gameBridge.players
                delegate: RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: modelData.name
                        Layout.fillWidth: true
                        font.bold: true
                    }
                    Label {
                        text: modelData.score
                        horizontalAlignment: Text.AlignRight
                        font.bold: true
                        color: "#ffdc50"
                    }
                }
            }
        }
    }

    // ===== Statistics Dialog =====
    Dialog {
        id: statisticsDialog
        title: qsTr("Statistics")
        anchors.centerIn: parent
        width: 380
        modal: true

        footer: DialogButtonBox {
            Button {
                text: qsTr("Reset")
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                onClicked: resetConfirmDialog.open()
            }
            Button {
                text: qsTr("OK")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            onAccepted: statisticsDialog.close()
        }

        ColumnLayout {
            spacing: 8
            anchors.fill: parent

            Label {
                text: qsTr("Lifetime Statistics")
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 10
            }

            GridLayout {
                columns: 2
                Layout.fillWidth: true
                columnSpacing: 20
                rowSpacing: 6

                Label { text: qsTr("Games Played:") }
                Label { text: gameBridge.gamesPlayed; font.bold: true; Layout.alignment: Qt.AlignRight }

                Label { text: qsTr("Games Won:") }
                Label { text: gameBridge.gamesWon; font.bold: true; Layout.alignment: Qt.AlignRight }

                Label { text: qsTr("Win Rate:") }
                Label { text: gameBridge.winRate.toFixed(1) + "%"; font.bold: true; Layout.alignment: Qt.AlignRight }

                Label { text: qsTr("Average Score:") }
                Label { text: gameBridge.avgScore.toFixed(1); font.bold: true; Layout.alignment: Qt.AlignRight }

                Label { text: qsTr("Best Score:") }
                Label { text: gameBridge.bestScore >= 0 ? gameBridge.bestScore : "-"; font.bold: true; Layout.alignment: Qt.AlignRight }

                Label { text: qsTr("Shot the Moon:") }
                Label { text: gameBridge.shootTheMoonCount; font.bold: true; Layout.alignment: Qt.AlignRight }
            }
        }
    }

    // Reset confirmation
    Dialog {
        id: resetConfirmDialog
        title: qsTr("Reset Statistics")
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent
        modal: true

        Label {
            text: qsTr("Are you sure you want to reset all statistics?")
        }

        onAccepted: {
            gameBridge.resetStatistics()
            statisticsDialog.close()
        }
    }

    // ===== Settings Dialog =====
    Dialog {
        id: settingsDialog
        title: qsTr("Settings")
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        width: 520
        modal: true

        background: Rectangle {
            color: "#353535"
            radius: 8
            border.color: "#555555"
            border.width: 1
        }

        // Store original values for cancel
        property real origScale
        property int origDifficulty
        property string origThemePath
        property bool origSound
        property bool origAnimRotation
        property bool origAnimAI
        property bool origAnimPassing
        property int origEndScore
        property bool origExactReset
        property bool origQueenBreaks
        property bool origMoonProtect
        property bool origFullPolish
        property bool origShowMenuBar

        // Guard to prevent theme preview resets during control sync
        property bool syncing: false

        onAboutToShow: {
            syncing = true

            origScale = gameBridge.cardScale
            origDifficulty = gameBridge.aiDifficulty
            origThemePath = gameBridge.themePath
            origSound = gameBridge.soundEnabled
            origAnimRotation = gameBridge.animateCardRotation
            origAnimAI = gameBridge.animateAICards
            origAnimPassing = gameBridge.animatePassingCards
            origEndScore = gameBridge.endScore
            origExactReset = gameBridge.exactResetTo50
            origQueenBreaks = gameBridge.queenBreaksHearts
            origMoonProtect = gameBridge.moonProtection
            origFullPolish = gameBridge.fullPolish
            origShowMenuBar = gameBridge.showMenuBar

            // Sync controls
            menuBarCheck.checked = gameBridge.showMenuBar
            cardScaleSlider.value = gameBridge.cardScale
            difficultyCombo.currentIndex = gameBridge.aiDifficulty
            soundCheck.checked = gameBridge.soundEnabled
            cardRotationCheck.checked = gameBridge.animateCardRotation
            aiCardsCheck.checked = gameBridge.animateAICards
            passingCardsCheck.checked = gameBridge.animatePassingCards
            exactResetCheck.checked = gameBridge.exactResetTo50
            queenBreaksCheck.checked = gameBridge.queenBreaksHearts
            moonChoiceCheck.checked = gameBridge.moonProtection
            fullPolishCheck.checked = gameBridge.fullPolish

            // Sync theme combo (themePath returns SVG file path,
            // combo entries use directory paths, so use startsWith)
            var currentPath = gameBridge.themePath
            for (var i = 0; i < themeCombo.model.length; i++) {
                var entryPath = themeCombo.model[i].path
                if (entryPath === "" && currentPath === "") {
                    themeCombo.currentIndex = i
                    break
                }
                if (entryPath !== "" && currentPath.indexOf(entryPath) === 0) {
                    themeCombo.currentIndex = i
                    break
                }
            }

            // Sync end score combo
            var es = gameBridge.endScore
            for (var j = 0; j < endScoreCombo.count; j++) {
                if (endScoreModel.get(j).value === es) {
                    endScoreCombo.currentIndex = j
                    break
                }
            }

            syncing = false

            // Load preview after sync is complete to use the correct theme
            gameBridge.loadPreviewTheme(themeCombo.model[themeCombo.currentIndex].path)
        }

        onAccepted: {
            gameBridge.cardScale = cardScaleSlider.value
            gameBridge.aiDifficulty = difficultyCombo.currentIndex
            gameBridge.soundEnabled = soundCheck.checked
            gameBridge.animateCardRotation = cardRotationCheck.checked
            gameBridge.animateAICards = aiCardsCheck.checked
            gameBridge.animatePassingCards = passingCardsCheck.checked
            gameBridge.endScore = endScoreModel.get(endScoreCombo.currentIndex).value
            gameBridge.exactResetTo50 = exactResetCheck.checked
            gameBridge.queenBreaksHearts = queenBreaksCheck.checked
            gameBridge.moonProtection = moonChoiceCheck.checked
            gameBridge.fullPolish = fullPolishCheck.checked

            gameBridge.themePath = themeCombo.model[themeCombo.currentIndex].path

            gameBridge.showMenuBar = menuBarCheck.checked
        }

        onRejected: {
            // Revert preview changes
            if (gameBridge.cardScale !== origScale) gameBridge.cardScale = origScale
            if (gameBridge.showMenuBar !== origShowMenuBar) gameBridge.showMenuBar = origShowMenuBar
        }

        ColumnLayout {
            id: settingsColumn
            spacing: 12
            anchors.fill: parent

            // Card Theme
            Label { text: qsTr("Card Theme:"); font.bold: true }
            ComboBox {
                id: themeCombo
                Layout.fillWidth: true
                textRole: "name"
                model: {
                    var themes = gameBridge.availableThemes
                    var result = [{name: qsTr("Built-in"), path: ""}]
                    for (var i = 0; i < themes.length; i++) {
                        result.push(themes[i])
                    }
                    return result
                }
                onCurrentIndexChanged: {
                    if (!settingsDialog.syncing && currentIndex >= 0 && model && model.length > currentIndex) {
                        gameBridge.loadPreviewTheme(model[currentIndex].path)
                    }
                }
            }

            // Theme preview
            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12

                Image {
                    source: "image://cardpreview/2_12?v=" + gameBridge.previewVersion
                    sourceSize.width: 70
                    sourceSize.height: 101
                    smooth: true
                }
                Image {
                    source: "image://cardpreview/3_14?v=" + gameBridge.previewVersion
                    sourceSize.width: 70
                    sourceSize.height: 101
                    smooth: true
                }
                Image {
                    source: "image://cardpreview/back?v=" + gameBridge.previewVersion
                    sourceSize.width: 70
                    sourceSize.height: 101
                    smooth: true
                }
            }

            // Card size slider
            Label { text: qsTr("Card Size:"); font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                Slider {
                    id: cardScaleSlider
                    Layout.fillWidth: true
                    from: 0.5
                    to: 2.0
                    stepSize: 0.05
                    value: gameBridge.cardScale
                    onMoved: gameBridge.cardScale = value
                }
                Label {
                    text: Math.round(cardScaleSlider.value * 100) + "%"
                    Layout.minimumWidth: 45
                }
            }

            // Sound
            CheckBox {
                id: soundCheck
                text: qsTr("Enable sound effects")
                checked: gameBridge.soundEnabled
            }

            // Animations header
            Label { text: qsTr("Animations"); font.bold: true; Layout.topMargin: 6 }

            CheckBox {
                id: cardRotationCheck
                text: qsTr("Card rotation on trick pile (slight random tilt)")
                checked: gameBridge.animateCardRotation
            }
            CheckBox {
                id: aiCardsCheck
                text: qsTr("Animate AI card plays")
                checked: gameBridge.animateAICards
            }
            CheckBox {
                id: passingCardsCheck
                text: qsTr("Animate card passing")
                checked: gameBridge.animatePassingCards
            }

            // AI Difficulty
            Label { text: qsTr("AI Difficulty:"); font.bold: true; Layout.topMargin: 6 }
            ComboBox {
                id: difficultyCombo
                Layout.fillWidth: true
                model: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")]
                currentIndex: gameBridge.aiDifficulty
            }

            // Game Rules header
            Label { text: qsTr("Game Rules"); font.bold: true; Layout.topMargin: 6 }

            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Game ends at score:") }
                ComboBox {
                    id: endScoreCombo
                    Layout.fillWidth: true
                    textRole: "text"
                    model: ListModel {
                        id: endScoreModel
                        ListElement { text: "50"; value: 50 }
                        ListElement { text: "75"; value: 75 }
                        ListElement { text: "100 (Standard)"; value: 100 }
                        ListElement { text: "150"; value: 150 }
                    }
                    Component.onCompleted: {
                        var es = gameBridge.endScore
                        for (var i = 0; i < endScoreModel.count; i++) {
                            if (endScoreModel.get(i).value === es) {
                                currentIndex = i
                                break
                            }
                        }
                    }
                }
            }

            CheckBox {
                id: exactResetCheck
                text: qsTr("Exactly %1 = reset to 50 (\"Save and take half\")").arg(
                    endScoreModel.count > 0 ? endScoreModel.get(endScoreCombo.currentIndex).value : 100)
            }
            CheckBox {
                id: queenBreaksCheck
                text: qsTr("Queen of Spades breaks hearts")
            }
            CheckBox {
                id: moonChoiceCheck
                text: qsTr("Shoot the Moon protection: if +26 to others would cause shooter to lose, take -26 instead")
                Layout.fillWidth: true
            }
            CheckBox {
                id: fullPolishCheck
                text: qsTr("Full Polish: 99 points + takes 25 = reset to 98")
            }

            // UI
            Label { text: qsTr("Interface"); font.bold: true; Layout.topMargin: 6 }
            CheckBox {
                id: menuBarCheck
                text: qsTr("Show menu bar (Ctrl+M to toggle)")
                checked: gameBridge.showMenuBar
            }

            // Theme info
            Label {
                text: qsTr("Card themes are loaded from:\n~/.local/share/carddecks/\n/usr/share/carddecks/\nInstall KDE card decks for more themes.")
                font.pixelSize: 11
                color: "#888888"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.topMargin: 10
            }
        }
    }

    // ===== About Dialog =====
    Dialog {
        id: aboutDialog
        title: qsTr("About Hearts")
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        width: 400
        modal: true

        ColumnLayout {
            spacing: 10
            anchors.fill: parent

            Label {
                text: qsTr("Hearts")
                font.pixelSize: 24
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: qsTr("A classic card game for Qt.")
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: qsTr("Try to avoid taking hearts and especially the Queen of Spades!")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            Label {
                text: qsTr("<b>Rules:</b><ul>" +
                    "<li>Each heart is worth 1 point</li>" +
                    "<li>Queen of Spades is worth 13 points</li>" +
                    "<li>Lowest score wins</li>" +
                    "<li>\"Shoot the Moon\" - take all hearts and QoS to give 26 points to others</li>" +
                    "</ul>")
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Version 1.0")
                Layout.alignment: Qt.AlignHCenter
                color: "#888888"
            }
        }
    }

    // Connect menu bar signals from C++ to QML dialogs
    Connections {
        target: gameBridge
        function onOpenScoresRequested() { scoresDialog.open() }
        function onOpenStatisticsRequested() { statisticsDialog.open() }
        function onOpenSettingsRequested() { settingsDialog.open() }
        function onOpenAboutRequested() { aboutDialog.open() }
    }

    Component.onCompleted: {
        gameBridge.newGame()
    }
}
