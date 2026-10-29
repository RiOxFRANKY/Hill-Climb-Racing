import QtQuick
import HillClimb

// Dial in the top-right corner: 0 to 140 km/h sweeps -135 to +135 degrees.
Item {
    id: root

    required property GameController game
    readonly property rect dial: Qt.rect(1720, 36, 152, 152)
    readonly property real needleAngle: -135 + Math.min(Math.max(game.speedKmh / 140, 0), 1) * 270

    Image {
        x: root.dial.x
        y: root.dial.y
        width: root.dial.width
        height: root.dial.height
        source: "qrc:/assets/ui/speedometer_dial.png"
        smooth: false
    }

    // Needle pivots at (10, 44) of its 20x56 sprite.
    Image {
        x: root.dial.x + root.dial.width / 2 - 10
        y: root.dial.y + root.dial.height / 2 - 44
        width: 20
        height: 56
        source: "qrc:/assets/ui/speedometer_needle.png"
        smooth: false
        transform: Rotation { origin.x: 10; origin.y: 44; angle: root.needleAngle }
    }

    ShadowText {
        x: root.dial.x
        y: root.dial.y + 90
        width: root.dial.width
        height: 28
        text: Math.round(root.game.speedKmh)
        font: Theme.pixelFont(15)
        color: Theme.rgb(255, 220, 60)
        shadowColor: Theme.rgb(8, 12, 16)
    }

    ShadowText {
        x: root.dial.x
        y: root.dial.y + 119
        width: root.dial.width
        height: 16
        text: "KM/H"
        font: Theme.uiFont(9)
        color: Theme.rgb(160, 185, 210)
        shadowColor: Theme.rgb(8, 12, 16)
        shadowX: 1
        shadowY: 1
    }
}
