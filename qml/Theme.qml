pragma Singleton

import QtQuick

// Fonts and colour helpers shared by the whole scene.
QtObject {
    readonly property FontLoader pixelFontLoader: FontLoader {
        source: "qrc:/assets/fonts/PressStart2P-Regular.ttf"
    }
    readonly property FontLoader uiFontLoader: FontLoader {
        source: "qrc:/assets/fonts/Silkscreen-Bold.ttf"
    }

    readonly property string pixelFamily: pixelFontLoader.status === FontLoader.Ready
                                          ? pixelFontLoader.font.family : "Courier New"
    readonly property string uiFamily: uiFontLoader.status === FontLoader.Ready
                                       ? uiFontLoader.font.family : pixelFamily

    // Touch platforms get touch wording in the menu instead of key names.
    readonly property bool touchUi: Qt.platform.os === "android" || Qt.platform.os === "ios"

    // Sizes are the original point sizes at 96 DPI, given in pixels so they
    // stay the same on every screen density.
    function pixelFont(points) {
        return Qt.font({ family: pixelFamily, pixelSize: Math.round(points * 96 / 72), bold: true })
    }

    function uiFont(points) {
        return Qt.font({ family: uiFamily, pixelSize: Math.round(points * 96 / 72), bold: true })
    }

    function rgb(r, g, b, a) {
        return Qt.rgba(r / 255, g / 255, b / 255, (a === undefined ? 255 : a) / 255)
    }
}
