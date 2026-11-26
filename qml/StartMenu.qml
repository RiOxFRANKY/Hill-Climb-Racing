import QtQuick
import HillClimb

// Title screen over the idle car at the start line. It has three pages:
// main (Play, Garage, Controls, Quit), mode (Normal, Rescue, Race) and map.
// Items marked locked are not built yet; choosing one only shows a notice.
Item {
    id: root

    property string courseSummary
    property var mapSummaries: []  // one line per map, shown under the buttons
    property string page: "main"
    property int selected: 0
    property bool showingControls: false
    property real time: 0
    property real noticeTime: -10  // when a locked item was last chosen

    // True on the first page, where Esc quits instead of going back.
    readonly property bool atTop: page === "main" && !showingControls

    readonly property var pages: ({
        "main": {
            heading: root.courseSummary,
            items: [
                { label: "PLAY", action: "mode" },
                { label: "GARAGE", locked: true },
                { label: "CONTROLS", action: "controls" },
                { label: "QUIT", action: "quit" },
            ]
        },
        "mode": {
            heading: "CHOOSE A MODE",
            back: "main",
            items: [
                { label: "NORMAL", action: "map" },
                { label: "RESCUE", locked: true },
                { label: "RACE", locked: true },
                { label: "BACK", action: "back" },
            ]
        },
        "map": {
            heading: "CHOOSE A MAP",
            back: "mode",
            items: [
                // Each map is harder than the one above it.
                { label: "GRAZING THE GRASS", action: "play", map: 0, hint: root.mapSummaries[0] },
                { label: "BEACHDAY", action: "play", map: 1, hint: root.mapSummaries[1] },
                { label: "SWAMPY VIBE", action: "play", map: 2, hint: root.mapSummaries[2] },
                { label: "ARCTIC ELEGANCE", action: "play", map: 3, hint: root.mapSummaries[3] },
                { label: "THE REAL EXCURSION", action: "play", map: 4, hint: root.mapSummaries[4] },
                { label: "BACK", action: "back" },
            ]
        },
    })
    readonly property var current: pages[page]
    readonly property var items: current.items
    readonly property var selectedItem: items[Math.min(selected, items.length - 1)]
    // Longer lists get slimmer, wider buttons so six fit above the footer.
    readonly property bool compact: items.length > 4

    signal playRequested(int map)
    signal quitRequested()

    function reset() {
        page = "main"
        selected = 0
        showingControls = false
        time = 0
        noticeTime = -10
    }

    function advance(frameTime) {
        time += Math.min(Math.max(frameTime, 0), 0.034)
    }

    function itemRect(index) {
        // dialog_button.png (300x74): 1.2x normally; wider and slimmer when compact.
        return compact ? Qt.rect(960 - 240, 384 + index * 92, 480, 76)
                       : Qt.rect(960 - 180, 384 + index * 112, 360, 88.8)
    }

    function openPage(name) {
        page = name
        selected = 0
        noticeTime = -10
    }

    function goBack() {
        if (current.back !== undefined)
            openPage(current.back)
    }

    function activate(index) {
        const item = items[index]
        if (item.locked) {
            noticeTime = time
            return
        }
        switch (item.action) {
        case "mode": openPage("mode"); break
        case "map": openPage("map"); break
        case "back": goBack(); break
        case "controls": showingControls = true; break
        case "play": playRequested(item.map); break
        case "quit": quitRequested(); break
        }
    }

    function handleKey(key) {
        if (showingControls) {
            if (key === Qt.Key_Escape || key === Qt.Key_Return || key === Qt.Key_Enter
                    || key === Qt.Key_Space || key === Qt.Key_Backspace)
                showingControls = false
            return
        }
        if (key === Qt.Key_Escape || key === Qt.Key_Backspace)
            goBack()
        else if (key === Qt.Key_Up || key === Qt.Key_W)
            selected = (selected + items.length - 1) % items.length
        else if (key === Qt.Key_Down || key === Qt.Key_S)
            selected = (selected + 1) % items.length
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

            // 2. Page heading under the board.
            ShadowText {
                y: title.centerY + title.boardHeight / 2 + 28
                width: parent.width
                height: 40
                text: root.current.heading
                font: Theme.uiFont(16)
                shadowX: 3
                shadowY: 3
            }
        }

        // 3. Buttons. The highlighted one gets a pixel frame and arrow; the others are dimmed.
        Repeater {
            model: root.items

            Item {
                id: button

                required property int index
                required property var modelData
                readonly property bool locked: modelData.locked === true
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
                    text: button.modelData.label
                    font: Theme.pixelFont(root.compact ? 15 : 18)
                    color: button.locked ? Theme.rgb(150, 154, 162)
                         : button.isSelected ? Theme.rgb(255, 220, 60) : Theme.rgb(225, 228, 235)
                    shadowX: 3
                    shadowY: 3
                }

                // Tag beside items that are not built yet.
                ShadowText {
                    visible: button.locked
                    x: button.width + 18
                    width: 120
                    height: button.height
                    horizontalAlignment: Text.AlignLeft
                    text: "SOON"
                    font: Theme.uiFont(12)
                    color: Theme.rgb(255, 210, 50)
                    shadowX: 2
                    shadowY: 2
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

        // 4. What the highlighted item is. Choosing a locked item makes the
        // "coming soon" notice flash.
        ShadowText {
            readonly property real since: root.time - root.noticeTime
            readonly property bool flashing: since < 1.2

            y: parent.height - 128
            width: parent.width
            height: 40
            text: root.selectedItem.locked ? root.selectedItem.label + "  •  COMING SOON"
                                           : (root.selectedItem.hint || "")
            font: Theme.uiFont(14)
            color: flashing && Math.floor(since * 8) % 2 === 0 ? Theme.rgb(255, 120, 60)
                 : root.selectedItem.locked ? Theme.rgb(255, 210, 50) : Theme.rgb(240, 245, 255)
            shadowX: 3
            shadowY: 3
        }

        // 5. Footer hint (keyboard only).
        ShadowText {
            visible: !Theme.touchUi
            y: parent.height - 70
            width: parent.width
            height: 40
            text: root.atTop ? "UP / DOWN  SELECT   •   ENTER  CONFIRM   •   F11  FULLSCREEN"
                             : "UP / DOWN  SELECT   •   ENTER  CONFIRM   •   ESC  BACK"
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
