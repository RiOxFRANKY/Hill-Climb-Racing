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

            // Coins: worth more means bigger, with the value printed on the
            // larger ones.
            Item {
                id: coin

                readonly property int value: slot.pickup && !slot.pickup.fuel ? slot.pickup.value : 0
                readonly property real baseSize: value >= 500 ? 64 : value >= 100 ? 56 : value >= 25 ? 50 : 44
                readonly property real size: baseSize * (1 + 0.05 * Math.sin(root.animationTime * 5 + (slot.pickup ? slot.pickup.phase : 0)))

                visible: value > 0

                Image {
                    x: -coin.size / 2
                    y: -coin.size / 2
                    width: coin.size
                    height: coin.size
                    source: "qrc:/assets/ui/coin.png"
                    smooth: false
                }

                ShadowText {
                    visible: coin.value >= 25
                    x: -coin.baseSize / 2
                    y: -coin.baseSize / 2
                    width: coin.baseSize
                    height: coin.baseSize
                    text: coin.value
                    font: Theme.pixelFont(coin.value >= 100 ? 8 : 9)
                    color: Theme.rgb(255, 250, 225)
                    shadowColor: Theme.rgb(90, 50, 5, 255)
                    shadowX: 2
                    shadowY: 2
                }
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
