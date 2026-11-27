import QtQuick
import HillClimb

// The start menu's list of controls, with a Back button.
Item {
    id: root

    signal backRequested()

    // dialog_box.png (700x390) at a uniform 1.3x.
    readonly property rect panel: Qt.rect(960 - 455, 540 - 253.5, 910, 507)
    readonly property real keyRight: panel.x + 330

    readonly property var keyboardBindings: [
        ["D / RIGHT", "GAS  •  TILT FORWARD IN THE AIR"],
        ["A / LEFT", "BRAKE  •  REVERSE  •  TILT BACK"],
        ["SPACE", "BOOST  (BURNS MORE FUEL)"],
        ["P", "PAUSE / RESUME"],
        ["R", "RESTART"],
        ["F11", "TOGGLE FULLSCREEN"],
        ["ESC", "BACK TO THIS MENU"],
    ]
    readonly property var touchBindings: [
        ["GAS", "DRIVE  •  TILT FORWARD IN THE AIR"],
        ["BRAKE", "BRAKE  •  REVERSE  •  TILT BACK"],
        ["BOOST", "BOOST  (BURNS MORE FUEL)"],
        ["PAUSE", "TAP  P PAUSE  AT THE TOP"],
        ["RESTART", "TAP  R RESTART  AT THE TOP"],
        ["BACK", "BACK TO THIS MENU"],
    ]

    Rectangle {
        anchors.fill: parent
        color: Theme.rgb(6, 10, 16, 120)
    }

    // Keeps clicks from reaching anything underneath.
    MouseArea {
        anchors.fill: parent
    }

    Image {
        x: root.panel.x
        y: root.panel.y
        width: root.panel.width
        height: root.panel.height
        source: "qrc:/assets/ui/dialog_box.png"
        smooth: false
    }

    ShadowText {
        x: root.panel.x
        y: root.panel.y + 36
        width: root.panel.width
        height: 60
        text: "CONTROLS"
        font: Theme.pixelFont(22)
        color: Theme.rgb(255, 220, 100)
        shadowColor: Theme.rgb(6, 8, 12)
        shadowX: 3
        shadowY: 3
    }

    Repeater {
        model: Theme.touchUi ? root.touchBindings : root.keyboardBindings

        Item {
            id: row

            required property int index
            required property var modelData
            readonly property real rowY: root.panel.y + 116 + index * 34

            ShadowText {
                x: root.panel.x
                y: row.rowY
                width: root.keyRight - root.panel.x
                height: 32
                text: row.modelData[0]
                font: Theme.pixelFont(12)
                color: Theme.rgb(255, 210, 50)
                shadowColor: Theme.rgb(6, 8, 12)
                horizontalAlignment: Text.AlignRight
            }

            ShadowText {
                x: root.keyRight + 40
                y: row.rowY
                width: root.panel.x + root.panel.width - root.keyRight - 60
                height: 32
                text: row.modelData[1]
                font: Theme.uiFont(12)
                color: Theme.rgb(210, 225, 240)
                shadowColor: Theme.rgb(6, 8, 12)
                horizontalAlignment: Text.AlignLeft
            }
        }
    }

    Image {
        id: back
        x: 960 - 180
        y: root.panel.y + root.panel.height - 88.8 - 34
        width: 360
        height: 88.8
        source: backArea.pressed ? "qrc:/assets/ui/dialog_button_pressed.png" : "qrc:/assets/ui/dialog_button.png"
        smooth: false

        ShadowText {
            y: backArea.pressed ? 2 : 0
            width: back.width
            height: back.height
            text: "BACK"
            font: Theme.pixelFont(16)
            shadowColor: Theme.rgb(6, 8, 12)
            shadowX: 3
            shadowY: 3
        }

        MouseArea {
            id: backArea
            anchors.fill: parent
            onClicked: root.backRequested()
        }
    }
}
