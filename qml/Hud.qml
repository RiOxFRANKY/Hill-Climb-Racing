import QtQuick
import HillClimb

// Dashboard panel, fuel gauge, hotkey bar and the current stage line.
Item {
    id: root

    required property GameController game

    signal pauseRequested()
    signal restartRequested()
    signal menuRequested()

    Image {
        x: 42
        y: 36
        width: 520
        height: 164
        source: "qrc:/assets/ui/hud_panel.png"
        smooth: false
    }

    HudLabel { x: 64; y: 72; text: "DISTANCE"; font: Theme.uiFont(11); color: Theme.rgb(175, 195, 215) }
    HudLabel { x: 290; y: 72; text: "COINS"; font: Theme.uiFont(11); color: Theme.rgb(175, 195, 215) }
    HudLabel { x: 428; y: 72; text: "SCORE"; font: Theme.uiFont(11); color: Theme.rgb(175, 195, 215) }

    HudLabel { x: 64; y: 116; text: root.game.distance + " m"; font: Theme.pixelFont(16) }
    Image {
        x: 280
        y: 92
        width: 26
        height: 26
        source: "qrc:/assets/ui/coin_icon.png"
        smooth: false
    }
    HudLabel { x: 314; y: 116; text: root.game.coins; font: Theme.pixelFont(16); color: Theme.rgb(255, 215, 45) }
    HudLabel { x: 428; y: 116; text: root.game.score; font: Theme.pixelFont(16) }

    // Fuel meter with retro pixel segments.
    HudLabel { x: 64; y: 166; text: "FUEL"; font: Theme.uiFont(11); color: Theme.rgb(175, 195, 215) }

    Repeater {
        model: 20

        Item {
            id: segment

            required property int index
            readonly property real leftEdge: 145 + index * (15.5 + 3)
            readonly property bool lit: index < Math.round(root.game.fuel / 100 * 20)
            readonly property bool low: root.game.fuel <= 28

            // QPainter strokes the 1 px outline centred on the rectangle edge.
            Rectangle {
                x: segment.leftEdge - 0.5
                y: 148 - 0.5
                width: 15.5 + 1
                height: 22 + 1
                color: !segment.lit ? Theme.rgb(16, 22, 30, 230)
                                    : segment.low ? Theme.rgb(245, 55, 45) : Theme.rgb(60, 225, 95)
                border.width: 1
                border.color: Theme.rgb(8, 12, 16)
            }
            Rectangle {
                visible: segment.lit
                x: segment.leftEdge + 0.5
                y: 148 + 0.5
                width: 15.5 - 1
                height: 1
                color: segment.low ? Theme.rgb(255, 160, 150) : Theme.rgb(160, 255, 180)
            }
            Rectangle {
                visible: segment.lit
                x: segment.leftEdge + 0.5
                y: 148 + 22 - 1.5
                width: 15.5 - 1
                height: 1
                color: segment.low ? Theme.rgb(140, 20, 15) : Theme.rgb(25, 120, 45)
            }
        }
    }

    // Hotkey bar. Each part is also a tap target for touch screens.
    ShadowText {
        id: hotkeys
        x: 560
        y: 30
        width: 800
        height: 40
        text: "P  PAUSE   •   R  RESTART   •   ESC  MENU"
        font: Theme.uiFont(11)
        color: Theme.rgb(240, 245, 255, 220)
    }

    TextMetrics { id: fullHint; font: hotkeys.font; text: hotkeys.text }
    TextMetrics { id: beforeFirstDot; font: hotkeys.font; text: "P  PAUSE   " }
    TextMetrics { id: beforeSecondDot; font: hotkeys.font; text: "P  PAUSE   •   R  RESTART   " }
    TextMetrics { id: dot; font: hotkeys.font; text: "•" }

    Item {
        id: hotkeyTaps

        readonly property real textLeft: hotkeys.x + (hotkeys.width - fullHint.advanceWidth) / 2
        readonly property real firstSplit: textLeft + beforeFirstDot.advanceWidth + dot.advanceWidth / 2
        readonly property real secondSplit: textLeft + beforeSecondDot.advanceWidth + dot.advanceWidth / 2

        MouseArea {
            x: hotkeyTaps.textLeft - 40
            y: 10
            width: hotkeyTaps.firstSplit - x
            height: 90
            onClicked: root.pauseRequested()
        }
        MouseArea {
            x: hotkeyTaps.firstSplit
            y: 10
            width: hotkeyTaps.secondSplit - x
            height: 90
            onClicked: root.restartRequested()
        }
        MouseArea {
            x: hotkeyTaps.secondSplit
            y: 10
            width: hotkeyTaps.textLeft + fullHint.advanceWidth + 40 - x
            height: 90
            onClicked: root.menuRequested()
        }
    }

    ShadowText {
        visible: root.game.stageText !== ""
        x: 560
        y: 64
        width: 800
        height: 32
        text: root.game.stageText
        font: Theme.uiFont(12)
        color: Theme.rgb(255, 226, 120)
        shadowColor: Theme.rgb(10, 14, 20, 240)
    }
}
