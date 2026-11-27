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

    readonly property var themeStyles: [
        // 0: Grassland (default rustic wood & iron chains)
        {
            boardSource: "qrc:/assets/ui/wooden_board.png",
            linkSource: "qrc:/assets/ui/chain_link.png",
            rangeColor: Theme.rgb(245, 220, 160),
            rangeShadowColor: Theme.rgb(25, 12, 6, 230),
            titleColor: Theme.rgb(255, 210, 50),
            titleShadowColor: Theme.rgb(25, 12, 6, 240),
            shadowY: 2
        },
        // 1: Beach (driftwood with algae, coral, and braided ropes)
        {
            boardSource: "qrc:/assets/ui/wooden_board_beach.png",
            linkSource: "qrc:/assets/ui/chain_link_beach.png",
            rangeColor: Theme.rgb(255, 248, 224),
            rangeShadowColor: Theme.rgb(44, 22, 8, 245),
            titleColor: Theme.rgb(44, 24, 10),
            titleShadowColor: Theme.rgb(255, 236, 196, 170),
            shadowY: 1
        },
        // 2: Swamp (dark rusted iron plate & rusty chains)
        {
            boardSource: "qrc:/assets/ui/wooden_board_swamp.png",
            linkSource: "qrc:/assets/ui/chain_link_swamp.png",
            rangeColor: Theme.rgb(186, 218, 164),
            rangeShadowColor: Theme.rgb(8, 12, 8, 245),
            titleColor: Theme.rgb(220, 240, 120),
            titleShadowColor: Theme.rgb(8, 12, 8, 250),
            shadowY: 2
        },
        // 3: Snow / Arctic (dark timber with snow cap & frosted chains, white text)
        {
            boardSource: "qrc:/assets/ui/wooden_board_snow.png",
            linkSource: "qrc:/assets/ui/chain_link_snow.png",
            rangeColor: Theme.rgb(180, 228, 255),
            rangeShadowColor: Theme.rgb(10, 16, 28, 245),
            titleColor: Theme.rgb(255, 255, 255),
            titleShadowColor: Theme.rgb(10, 16, 28, 250),
            shadowY: 2
        },
        // 4: Moon (titanium spacecraft plate & sturdy modern chains)
        {
            boardSource: "qrc:/assets/ui/wooden_board_moon.png",
            linkSource: "qrc:/assets/ui/chain_link_moon.png",
            rangeColor: Theme.rgb(46, 86, 140),
            rangeShadowColor: Theme.rgb(255, 255, 255, 180),
            titleColor: Theme.rgb(20, 34, 62),
            titleShadowColor: Theme.rgb(255, 255, 255, 200),
            shadowY: 1
        }
    ]
    readonly property var themeStyle: (game.map >= 0 && game.map < themeStyles.length)
                                      ? themeStyles[game.map] : themeStyles[0]

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

    Chain { target: root.eyelet(-1); linkSource: root.themeStyle.linkSource }
    Chain { target: root.eyelet(1); linkSource: root.themeStyle.linkSource }

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
            source: root.themeStyle.boardSource
            smooth: false
        }

        ShadowText {
            x: -root.boardWidth / 2
            y: -root.boardHeight / 2 + 16
            width: root.boardWidth
            height: 26
            text: root.game.sectionRange
            font: Theme.uiFont(12)
            color: root.themeStyle.rangeColor
            shadowColor: root.themeStyle.rangeShadowColor
            shadowX: 0
            shadowY: root.themeStyle.shadowY
        }

        ShadowText {
            x: -root.boardWidth / 2
            y: -root.boardHeight / 2 + 46
            width: root.boardWidth
            height: 46
            text: root.game.sectionName
            font: Theme.pixelFont(17)
            color: root.themeStyle.titleColor
            shadowColor: root.themeStyle.titleShadowColor
            shadowX: 0
            shadowY: root.themeStyle.shadowY
        }
    }
}
