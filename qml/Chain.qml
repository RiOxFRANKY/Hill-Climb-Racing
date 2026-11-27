import QtQuick

// A chain hanging from just above the screen ceiling down to target.
Item {
    id: root

    property point target
    property url linkSource: "qrc:/assets/ui/chain_link.png"

    Repeater {
        model: Math.max(0, Math.ceil((root.target.y + 15) / 26))

        Image {
            required property int index
            x: root.target.x - 8
            y: -15 + index * 26
            width: 16
            height: 32
            source: root.linkSource
            smooth: false
        }
    }
}
