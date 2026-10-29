import QtQuick
import QtQuick.Window
import HillClimb

Window {
    id: window

    property bool startInGame: false
    property bool inMenu: !startInGame
    readonly property bool isAndroid: Qt.platform.os === "android"

    width: 1920
    height: 1080
    minimumWidth: 1920 / 4
    minimumHeight: 1080 / 4
    visible: true
    visibility: isAndroid ? Window.FullScreen : Window.Windowed
    color: "black"
    title: "Hill Climb Qt — Basic Edition"

    onActiveChanged: {
        if (!active)
            controller.clearKeys()
    }

    function openMenu() {
        // The menu sits over the idle car at the start line.
        inMenu = true
        menu.reset()
        controller.restart()
    }

    function startGame() {
        inMenu = false
        controller.restart()
    }

    function toggleFullScreen() {
        if (isAndroid)
            return
        visibility = visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
        // Keys held across the switch may never see their release event.
        controller.clearKeys()
    }

    function handleKeyPress(event) {
        const key = event.key
        // Android's Back button does what Esc does on a keyboard.
        const escape = key === Qt.Key_Escape || key === Qt.Key_Back
        event.accepted = true

        if (event.isAutoRepeat) {
            // Holding Up/Down in the menu keeps moving the selection.
            if (inMenu && !escape && key !== Qt.Key_Return && key !== Qt.Key_Enter && key !== Qt.Key_Space)
                menu.handleKey(key)
            return
        }

        const altEnter = (key === Qt.Key_Return || key === Qt.Key_Enter) && (event.modifiers & Qt.AltModifier)
        if (key === Qt.Key_F11 || altEnter) {
            toggleFullScreen()
        } else if (escape && !isAndroid && visibility === Window.FullScreen && !(inMenu && menu.showingControls)) {
            // Esc leaves fullscreen first.
            toggleFullScreen()
        } else if (inMenu) {
            if (escape && !menu.showingControls)
                Qt.quit()
            else
                menu.handleKey(escape ? Qt.Key_Escape : key)
        } else if (escape) {
            openMenu()
        } else if (key === Qt.Key_R) {
            controller.restart()
        } else if (key === Qt.Key_P) {
            controller.togglePause()
        } else {
            controller.pressKey(key)
        }
    }

    GameController {
        id: controller
    }

    FrameAnimation {
        running: true
        onTriggered: window.inMenu ? menu.advance(frameTime) : controller.tick(frameTime)
    }

    // The scene is always laid out at 1920x1080 and scaled to fit the window,
    // keeping its 16:9 shape, centred, with black bars filling any spare space.
    Item {
        id: stage
        objectName: "stage"

        readonly property real fit: Math.min(window.width / width, window.height / height)

        width: 1920
        height: 1080
        x: (window.width - width * fit) / 2
        y: (window.height - height * fit) / 2
        scale: fit
        transformOrigin: Item.TopLeft
        clip: true
        focus: true

        Keys.onPressed: event => window.handleKeyPress(event)
        Keys.onReleased: event => {
            if (!event.isAutoRepeat)
                controller.releaseKey(event.key)
            event.accepted = true
        }

        Background {
            anchors.fill: parent
            cameraX: controller.cameraX
        }
        TerrainItem {
            game: controller
        }
        FinishLine {
            base: controller.finishPos
        }
        Pickups {
            anchors.fill: parent
            game: controller
            animationTime: window.inMenu ? menu.time : controller.survivalTime
        }
        Car {
            anchors.fill: parent
            game: controller
        }

        Item {
            anchors.fill: parent
            visible: !window.inMenu
            enabled: !window.inMenu

            Pedals {
                anchors.fill: parent
                game: controller
            }
            Speedometer {
                anchors.fill: parent
                game: controller
            }
            Hud {
                anchors.fill: parent
                game: controller
                onPauseRequested: controller.togglePause()
                onRestartRequested: controller.restart()
                onMenuRequested: window.openMenu()
            }
            ZoneBanner {
                anchors.fill: parent
                game: controller
            }
            GameOverlay {
                anchors.fill: parent
                game: controller
                onResumeRequested: controller.togglePause()
                onRestartRequested: controller.restart()
                onMenuRequested: window.openMenu()
            }
        }

        StartMenu {
            id: menu
            anchors.fill: parent
            visible: window.inMenu
            enabled: window.inMenu
            courseSummary: controller.courseSummary
            onPlayRequested: window.startGame()
            onQuitRequested: Qt.quit()
        }

        // Hide the mouse pointer while racing.
        MouseArea {
            anchors.fill: parent
            enabled: !window.inMenu
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            cursorShape: Qt.BlankCursor
        }
    }
}
