import QtQuick
import HillClimb

// Brake, boost and gas pedals. They light up for keyboard input too, and each
// can be held by its own finger on a touch screen.
Item {
    id: root

    required property GameController game

    component Pedal: Image {
        id: pedal

        required property string name
        property bool active

        signal held(bool down)

        source: "qrc:/assets/ui/pedal_" + name + (active ? "_pressed" : "") + ".png"
        smooth: false

        MultiPointTouchArea {
            anchors.fill: parent
            maximumTouchPoints: 1
            onPressed: pedal.held(true)
            onReleased: pedal.held(false)
            onCanceled: pedal.held(false)
        }
    }

    Pedal {
        id: brake
        x: 48
        y: 900
        width: 250
        height: 120
        name: "brake"
        active: root.game.reversing
        onHeld: down => root.game.setBrakePedal(down)
    }
    ShadowText {
        x: brake.x
        y: brake.y + 36
        width: brake.width
        height: 32
        text: "BRAKE"
        font: Theme.pixelFont(15)
        color: brake.active ? Theme.rgb(255, 240, 160) : "white"
        shadowColor: Theme.rgb(8, 12, 16)
    }
    ShadowText {
        x: brake.x
        y: brake.y + 70
        width: brake.width
        height: 24
        text: "REVERSE"
        font: Theme.uiFont(10)
        color: brake.active ? Theme.rgb(255, 180, 170) : Theme.rgb(200, 215, 230)
        shadowColor: Theme.rgb(8, 12, 16)
    }

    Pedal {
        id: boost
        x: 817
        y: 900
        width: 286
        height: 120
        name: "boost"
        active: root.game.boosting
        onHeld: down => root.game.setBoostPedal(down)
    }
    ShadowText {
        x: boost.x
        y: boost.y + 44
        width: boost.width
        height: 36
        text: "BOOST"
        font: Theme.pixelFont(16)
        color: boost.active ? Theme.rgb(100, 240, 255) : Theme.rgb(255, 210, 50)
        shadowColor: Theme.rgb(8, 12, 16)
    }

    Pedal {
        id: gas
        x: 1622
        y: 900
        width: 250
        height: 120
        name: "gas"
        active: root.game.accelerating
        onHeld: down => root.game.setGasPedal(down)
    }
    ShadowText {
        x: gas.x
        y: gas.y + 44
        width: gas.width
        height: 36
        text: "GAS"
        font: Theme.pixelFont(16)
        color: gas.active ? Theme.rgb(160, 255, 180) : "white"
        shadowColor: Theme.rgb(8, 12, 16)
    }
}
