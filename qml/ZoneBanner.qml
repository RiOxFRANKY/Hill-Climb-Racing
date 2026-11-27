import QtQuick
import HillClimb

// Wooden stage sign that drops in on chains when a new stage starts.
//   0.0 - 1.4 s: drop from the ceiling with a spring bounce and sway
//   1.4 - 4.2 s: steady, with a gentle ambient sway
//   4.2 - 5.0 s: pulled back up out of the screen
Item {
    id: root

    required property GameController game

    readonly property real elapsed: 5 - game.zoneBannerTime
    readonly property real targetCenterY: 112
    readonly property real startCenterY: -180
    readonly property real boardWidth: 720
    readonly property real boardHeight: 124
    readonly property real centerX: 960
    readonly property var pose: poseAt(elapsed)
    readonly property real swingRadians: pose.swing * Math.PI / 180

    visible: game.zoneBannerTime > 0 && game.sectionName !== "" && pose.y >= startCenterY - 20

    function clamp(value, minimum, maximum) {
        return Math.max(minimum, Math.min(value, maximum))
    }

    function poseAt(t) {
        if (t < 1.4) {
            const easeDrop = 1 - Math.pow(1 - clamp(t / 0.6, 0, 1), 3)
            const bounce = 26 * Math.exp(-3.2 * t) * Math.sin(t * 15)
            return { y: startCenterY + (targetCenterY - startCenterY) * easeDrop + bounce,
                     swing: 4 * Math.exp(-2.4 * t) * Math.sin(t * 9.5) }
        }
        if (t < 4.2)
            return { y: targetCenterY, swing: 0.3 * Math.sin(t * 2.2) }
        const retract = clamp((t - 4.2) / 0.8, 0, 1)
        return { y: targetCenterY - (targetCenterY - startCenterY) * retract * retract,
                 swing: -2.5 * Math.sin(retract * Math.PI) }
    }

    function eyelet(side) {
        const ex = side * 252
        const ey = -boardHeight / 2 + 4
        const c = Math.cos(swingRadians)
        const s = Math.sin(swingRadians)
        return Qt.point(centerX + ex * c - ey * s, pose.y + ex * s + ey * c)
    }

    Chain { target: root.eyelet(-1) }
    Chain { target: root.eyelet(1) }

    Item {
        x: root.centerX
        y: root.pose.y
        rotation: root.pose.swing
        transformOrigin: Item.TopLeft

        Image {
            x: -root.boardWidth / 2
            y: -root.boardHeight / 2
            width: root.boardWidth
            height: root.boardHeight
            source: "qrc:/assets/ui/wooden_board.png"
            smooth: false
        }

        ShadowText {
            x: -root.boardWidth / 2
            y: -root.boardHeight / 2 + 16
            width: root.boardWidth
            height: 26
            text: root.game.sectionRange
            font: Theme.uiFont(12)
            color: Theme.rgb(245, 220, 160)
            shadowColor: Theme.rgb(25, 12, 6, 230)
            shadowX: 0
            shadowY: 2
        }

        ShadowText {
            x: -root.boardWidth / 2
            y: -root.boardHeight / 2 + 46
            width: root.boardWidth
            height: 46
            text: root.game.sectionName
            font: Theme.pixelFont(17)
            color: Theme.rgb(255, 210, 50)
            shadowColor: Theme.rgb(25, 12, 6, 240)
            shadowX: 0
            shadowY: 2
        }
    }
}
