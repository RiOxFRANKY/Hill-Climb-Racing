import QtQuick

// Three wrapping parallax strips: sky, distant mountains, closer hills.
// Seamlessly adapts to the selected map (Grassland, Beach, Swamp, Arctic, Moon).
Item {
    id: root

    property real cameraX
    property int map: 0
    readonly property real stripWidth: sky.imageWidth

    readonly property var themePrefixes: [
        "",         // 0: grassland (root assets)
        "beach/",   // 1: beach
        "swamp/",   // 2: swamp
        "snow/",    // 3: arctic / snow
        "moon/"     // 4: moon / excursion
    ]
    readonly property string prefix: (map >= 0 && map < themePrefixes.length) ? themePrefixes[map] : ""

    component ParallaxLayer: Item {
        id: layer

        required property url source
        required property real factor
        readonly property real imageWidth: first.implicitWidth
        readonly property real shift: root.stripWidth > 0 ? (root.cameraX * factor) % root.stripWidth : 0

        anchors.fill: parent

        Image {
            id: first
            x: -layer.shift
            source: layer.source
            smooth: false
        }
        Image {
            x: root.stripWidth - layer.shift
            visible: root.stripWidth - layer.shift < root.width
            source: layer.source
            smooth: false
        }
    }

    ParallaxLayer {
        id: sky
        source: "qrc:/assets/" + root.prefix + "bg_sky.png"
        factor: 0.012
    }
    ParallaxLayer {
        source: "qrc:/assets/" + root.prefix + "bg_mountains.png"
        factor: 0.038
    }
    ParallaxLayer {
        source: "qrc:/assets/" + root.prefix + "bg_hills.png"
        factor: 0.095
    }
}
