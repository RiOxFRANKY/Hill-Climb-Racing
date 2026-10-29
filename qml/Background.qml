import QtQuick

// Three wrapping parallax strips: sky, distant mountains, closer hills.
Item {
    id: root

    property real cameraX
    readonly property real stripWidth: sky.imageWidth

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
        source: "qrc:/assets/bg_sky.png"
        factor: 0.012
    }
    ParallaxLayer {
        source: "qrc:/assets/bg_mountains.png"
        factor: 0.038
    }
    ParallaxLayer {
        source: "qrc:/assets/bg_hills.png"
        factor: 0.095
    }
}
