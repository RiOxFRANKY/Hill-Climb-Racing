import QtQuick
import HillClimb

// Coins and fuel cans near the screen. A fixed pool of slots is reused every
// frame instead of recreating items as pickups come and go.
Item {
    id: root

    required property GameController game
    property real animationTime
    readonly property var visiblePickups: game.pickups

    Repeater {
        model: 64

        Item {
            id: slot

            required property int index
            readonly property var pickup: index < root.visiblePickups.length ? root.visiblePickups[index] : null

            visible: pickup !== null
            x: pickup ? pickup.x : 0
            y: pickup ? pickup.y : 0

            Image {
                readonly property real size: 46 * (1 + 0.05 * Math.sin(root.animationTime * 5 + (slot.pickup ? slot.pickup.phase : 0)))
                visible: slot.pickup !== null && !slot.pickup.fuel
                x: -size / 2
                y: -size / 2
                width: size
                height: size
                source: "qrc:/assets/ui/coin.png"
                smooth: false
            }

            Image {
                visible: slot.pickup !== null && slot.pickup.fuel
                x: -35
                y: -55
                width: 70
                height: 100
                source: "image://sprites/fuel_can"
                transform: Rotation { origin.x: 35; origin.y: 55; angle: -5 }
            }
        }
    }
}
