import QtQuick
import HillClimb

// Title screen over the idle car at the start line.
Item {
    id: root

    property string courseSummary
    property int selected: 0
    property bool showingControls: false
    property real time: 0

    readonly property var labels: ["PLAY", "CONTROLS", "QUIT"]

    signal playRequested()
    signal quitRequested()

    function reset() {
        selected = 0
        showingControls = false
        time = 0
    }

    function advance(frameTime) {
        time += Math.min(Math.max(frameTime, 0), 0.034)
    }

    function itemRect(index) {
        // dialog_button.png (300x74) at a uniform 1.2x.
        return Qt.rect(960 - 180, 384 + index * 112, 360, 88.8)
    }

    function activate(index) {
        if (index === 0)
            playRequested()
        else if (index === 1)
            showingControls = true
        else if (index === 2)
            quitRequested()
    }

    function handleKey(key) {
        if (showingControls) {
            if (key === Qt.Key_Escape || key === Qt.Key_Return || key === Qt.Key_Enter
                    || key === Qt.Key_Space || key === Qt.Key_Backspace)
                showingControls = false
            return
        }
        if (key === Qt.Key_Up || key === Qt.Key_W)
            selected = (selected + labels.length - 1) % labels.length
        else if (key === Qt.Key_Down || key === Qt.Key_S)
            selected = (selected + 1) % labels.length
        else if (key === Qt.Key_Return || key === Qt.Key_Enter || key === Qt.Key_Space)
            activate(selected)
    }

    // Shade the top and bottom of the idle scene so the title and buttons read clearly.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.rgb(6, 10, 16, 150) }
            GradientStop { position: 0.45; color: Theme.rgb(6, 10, 16, 40) }
            GradientStop { position: 1.0; color: Theme.rgb(6, 10, 16, 120) }
        }
    }

    Item {
        anchors.fill: parent
        visible: !root.showingControls

        // 1. Title board: drops in on its chains like the zone banner, then sways gently.
        Item {
            id: title

            readonly property real boardWidth: 900 // wooden_board.png at a uniform 1.25x
            readonly property real boardHeight: 160
            readonly property real targetCenterY: 150
            readonly property real startCenterY: -200
            readonly property real t: root.time
            readonly property real dropT: Math.min(Math.max(t / 0.6, 0), 1)
            readonly property real centerY: t < 1.4
                ? startCenterY + (targetCenterY - startCenterY) * (1 - Math.pow(1 - dropT, 3))
                  + 26 * Math.exp(-3.2 * t) * Math.sin(t * 15)
                : targetCenterY
            readonly property real swing: t < 1.4 ? 4 * Math.exp(-2.4 * t) * Math.sin(t * 9.5)
                                                  : 0.4 * Math.sin(t * 2.2)
            readonly property real swingRadians: swing * Math.PI / 180

            function eyelet(side) {
                const ex = side * boardWidth * 0.35
                const ey = -boardHeight / 2 + 5
                const c = Math.cos(swingRadians)
                const s = Math.sin(swingRadians)
                return Qt.point(960 + ex * c - ey * s, centerY + ex * s + ey * c)
            }

            anchors.fill: parent

            Chain { target: title.eyelet(-1) }
            Chain { target: title.eyelet(1) }

            Item {
                x: 960
                y: title.centerY
                rotation: title.swing
                transformOrigin: Item.TopLeft

                Image {
                    x: -title.boardWidth / 2
                    y: -title.boardHeight / 2
                    width: title.boardWidth
                    height: title.boardHeight
                    source: "qrc:/assets/ui/wooden_board.png"
                    smooth: false
                }

                ShadowText {
                    x: -title.boardWidth / 2
                    y: -title.boardHeight / 2 + 2
                    width: title.boardWidth
                    height: title.boardHeight
                    text: "HILL CLIMB RACING"
                    font: Theme.pixelFont(32)
                    color: Theme.rgb(255, 210, 50)
                    shadowColor: Theme.rgb(25, 12, 6, 240)
                    shadowX: 4
                    shadowY: 4
                }
            }

            // 2. Course summary under the board.
            ShadowText {
                y: title.centerY + title.boardHeight / 2 + 28
                width: parent.width
                height: 40
                text: root.courseSummary
                font: Theme.uiFont(16)
                shadowX: 3
                shadowY: 3
            }
        }

        // 3. Buttons. The highlighted one gets a pixel frame and arrow; the others are dimmed.
        Repeater {
            model: root.labels.length

            Item {
                id: button

                required property int index
                readonly property rect area: root.itemRect(index)
                readonly property bool isSelected: root.selected === index
                readonly property bool isPressed: buttonArea.pressed

                x: area.x
                y: area.y
                width: area.width
                height: area.height

                Rectangle {
                    visible: button.isSelected
                    x: -10
                    y: -10
                    width: button.width + 20
                    height: button.height + 20
                    color: Theme.rgb(8, 12, 16)
                }
                Rectangle {
                    visible: button.isSelected
                    x: -6
                    y: -6
                    width: button.width + 12
                    height: button.height + 12
                    color: Theme.rgb(255, 210, 50)
                }
                Image {
                    anchors.fill: parent
                    source: button.isPressed ? "qrc:/assets/ui/dialog_button_pressed.png" : "qrc:/assets/ui/dialog_button.png"
                    smooth: false
                }
                Rectangle {
                    visible: !button.isSelected
                    anchors.fill: parent
                    color: Theme.rgb(8, 12, 16, 80)
                }

                ShadowText {
                    y: button.isPressed ? 2 : 0
                    width: button.width
                    height: button.height
                    text: root.labels[button.index]
                    font: Theme.pixelFont(18)
                    color: button.isSelected ? Theme.rgb(255, 220, 60) : Theme.rgb(225, 228, 235)
                    shadowX: 3
                    shadowY: 3
                }

                // A click only counts when released over the button it started on.
                MouseArea {
                    id: buttonArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: root.selected = button.index
                    onPositionChanged: root.selected = button.index
                    onPressed: root.selected = button.index
                    onClicked: root.activate(button.index)
                }
            }
        }

        // Pixel arrow beside the highlighted button, nudging in 4 px steps.
        Item {
            id: arrow

            readonly property rect target: root.itemRect(root.selected)
            readonly property real nudge: Math.floor((Math.sin(root.time * 6) + 1) * 1.5) * 4
            readonly property real tipX: target.x - 22 + nudge
            readonly property real tipY: Math.round((target.y + target.height / 2) / 4) * 4

            Repeater {
                model: 2 * 6

                Rectangle {
                    required property int index
                    readonly property bool outline: index < 6
                    readonly property int column: index % 6
                    readonly property real grow: outline ? 4 : 0
                    readonly property real halfHeight: (6 - column) * 4 + grow

                    x: arrow.tipX - 24 + column * 4 - grow
                    y: arrow.tipY - halfHeight
                    width: 4 + grow * 2
                    height: halfHeight * 2
                    color: outline ? Theme.rgb(8, 12, 16) : Theme.rgb(255, 210, 50)
                }
            }
        }

        // 4. Footer hint (keyboard only).
        ShadowText {
            visible: !Theme.touchUi
            y: parent.height - 70
            width: parent.width
            height: 40
            text: "UP / DOWN  SELECT   •   ENTER  CONFIRM   •   F11  FULLSCREEN"
            font: Theme.uiFont(12)
            color: Theme.rgb(240, 245, 255)
        }
    }

    ControlsPanel {
        anchors.fill: parent
        visible: root.showingControls
        onBackRequested: root.showingControls = false
    }
}
