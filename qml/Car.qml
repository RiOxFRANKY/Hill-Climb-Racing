import QtQuick
import HillClimb

Item {
    id: root

    required property GameController game

    readonly property var themePrefixes: [
        "",         // 0: grassland
        "beach/",   // 1: beach
        "swamp/",   // 2: swamp
        "snow/",    // 3: snow
        "moon/"     // 4: moon
    ]
    readonly property string prefix: (root.game && root.game.map >= 0 && root.game.map < themePrefixes.length)
                                     ? themePrefixes[root.game.map] : ""

    component Wheel: Image {
        required property point position
        required property real angle
        readonly property real radius: root.game.wheelSpriteRadius

        x: position.x - radius
        y: position.y - radius
        width: radius * 2
        height: radius * 2
        rotation: angle
        source: "qrc:/assets/" + root.prefix + "wheel.png"
        smooth: false
    }

    Wheel {
        position: root.game.rearWheelPos
        angle: root.game.rearWheelRotation
    }
    Wheel {
        position: root.game.frontWheelPos
        angle: root.game.frontWheelRotation
    }

    // The body pivots about the chassis centre of mass.
    Image {
        id: body

        readonly property point offset: root.game.carSpriteOffset

        x: root.game.chassisPos.x + offset.x
        y: root.game.chassisPos.y + offset.y
        width: implicitWidth * root.game.carSpriteScale
        height: implicitHeight * root.game.carSpriteScale
        source: "qrc:/assets/" + root.prefix + "car_body.png"
        smooth: false
        transform: Rotation {
            origin.x: -body.offset.x
            origin.y: -body.offset.y
            angle: root.game.chassisRotation
        }
    }
}
