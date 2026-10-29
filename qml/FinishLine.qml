import QtQuick
import HillClimb

// Chequered banner on two posts, anchored at the ground under the finish.
Item {
    id: root

    property point base

    x: base.x
    y: base.y
    visible: base.x >= -200 && base.x <= 1920 + 200

    // 8 px round-capped posts.
    Repeater {
        model: [-1, 1]

        Rectangle {
            required property int modelData
            x: modelData * 90 - 4
            y: -260 - 4
            width: 8
            height: 260 + 8
            radius: 4
            color: Theme.rgb(40, 40, 40)
        }
    }

    Repeater {
        model: 3 * 9

        Rectangle {
            required property int index
            readonly property int row: Math.floor(index / 9)
            readonly property int column: index % 9
            x: -90 + column * 20
            y: -260 + row * 20
            width: 20
            height: 20
            color: (row + column) % 2 === 0 ? Theme.rgb(20, 20, 20) : Theme.rgb(245, 245, 245)
        }
    }

    Text {
        x: -150
        y: -260 - 46
        width: 300
        height: 40
        text: "FINISH"
        font: Theme.pixelFont(16)
        color: Theme.rgb(255, 206, 60)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
