import QtQuick
import HillClimb

// Pause and game-over dialog.
Item {
    id: root

    required property GameController game

    signal resumeRequested()
    signal restartRequested()
    signal menuRequested()

    visible: game.paused || game.gameOver

    Rectangle {
        anchors.fill: parent
        color: Theme.rgb(6, 10, 16, 175)
    }

    // Keeps taps from reaching the pedals and hotkey bar underneath.
    MouseArea {
        anchors.fill: parent
    }

    Image {
        id: card
        x: 610
        y: 320
        width: 700
        height: 390
        source: "qrc:/assets/ui/dialog_box.png"
        smooth: false
    }

    ShadowText {
        x: card.x
        y: card.y + 50
        width: card.width
        height: 80
        text: root.game.gameOver ? root.game.gameOverReason : "PAUSED"
        font: Theme.pixelFont(24)
        color: root.game.gameOver ? Theme.rgb(255, 80, 60) : Theme.rgb(255, 220, 100)
        shadowColor: Theme.rgb(6, 8, 12)
        shadowX: 3
        shadowY: 3
    }

    ShadowText {
        x: card.x
        y: card.y + 150
        width: card.width
        height: 55
        text: root.game.distance + " m   •   " + root.game.coins + " coins   •   " + root.game.score + " points"
        font: Theme.uiFont(15)
        color: Theme.rgb(210, 225, 240)
        shadowColor: Theme.rgb(6, 8, 12)
    }

    Image {
        id: button
        x: 810
        y: 565
        width: 300
        height: 74
        source: buttonArea.pressed ? "qrc:/assets/ui/dialog_button_pressed.png" : "qrc:/assets/ui/dialog_button.png"
        smooth: false

        ShadowText {
            y: 1 + (buttonArea.pressed ? 2 : 0)
            width: button.width
            height: button.height
            text: !root.game.gameOver ? "P  RESUME" : (root.game.finished ? "R  RACE AGAIN" : "R  TRY AGAIN")
            font: Theme.pixelFont(13)
            shadowColor: Theme.rgb(6, 8, 12)
        }

        MouseArea {
            id: buttonArea
            anchors.fill: parent
            onClicked: root.game.gameOver ? root.restartRequested() : root.resumeRequested()
        }
    }

    ShadowText {
        id: menuHint
        x: card.x
        y: button.y + button.height + 14
        width: card.width
        height: 28
        text: "ESC  MENU"
        font: Theme.uiFont(11)
        color: Theme.rgb(160, 185, 210)
        shadowColor: Theme.rgb(6, 8, 12)

        MouseArea {
            x: menuHint.width / 2 - 110
            y: -8
            width: 220
            height: menuHint.height + 16
            onClicked: root.menuRequested()
        }
    }
}
