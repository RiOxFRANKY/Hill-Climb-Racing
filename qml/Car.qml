import QtQuick
import HillClimb

Item {
    id: root

    required property GameController game

    component Wheel: Image {
        required property point position
        required property real angle
        readonly property real radius: root.game.wheelSpriteRadius

        x: position.x - radius
        y: position.y - radius
        width: radius * 2
        height: radius * 2
        rotation: angle
        source: "qrc:/assets/wheel.png"
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
        source: "qrc:/assets/car_body.png"
        smooth: false
        transform: Rotation {
            origin.x: -body.offset.x
            origin.y: -body.offset.y
            angle: root.game.chassisRotation
        }
    }
}
