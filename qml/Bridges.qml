import QtQuick
import HillClimb

// Plank bridges on screen. Each plank hangs from the joint line the physics
// uses (its top face is the surface the car drives on), with a rope rail
// above the deck and a wooden post at each cliff edge. A fixed pool of slots
// is reused every frame, like Pickups.qml.
Item {
    id: root

    required property GameController game
    readonly property var planks: game.bridgePlanks
    readonly property var posts: game.bridgePosts

    readonly property color wood: Theme.rgb(170, 108, 52)
    readonly property color woodLight: Theme.rgb(204, 140, 74)
    readonly property color woodDark: Theme.rgb(96, 56, 24)
    readonly property color outline: Theme.rgb(46, 26, 10)
    readonly property color rope: Theme.rgb(214, 186, 128)

    // Rope rail: a line along each plank, raised above the deck.
    Repeater {
        model: 64

        Rectangle {
            required property int index
            readonly property var plank: index < root.planks.length ? root.planks[index] : null

            visible: plank !== null
            x: plank ? plank.x : 0
            y: plank ? plank.y - 46 : 0
            width: plank ? plank.length + 1 : 0
            height: 4
            color: root.rope
            border.color: root.outline
            border.width: 1
            transformOrigin: Item.TopLeft
            rotation: plank ? plank.angle : 0
        }
    }

    // Planks: drawn below the joint line, with a light top edge and a dark
    // bottom edge, and a small gap between neighbours.
    Repeater {
        model: 64

        Item {
            id: plankSlot

            required property int index
            readonly property var plank: index < root.planks.length ? root.planks[index] : null

            visible: plank !== null
            x: plank ? plank.x : 0
            y: plank ? plank.y : 0
            transformOrigin: Item.TopLeft
            rotation: plank ? plank.angle : 0

            // Hanger rope from the rail down to the plank.
            Rectangle {
                x: 2
                y: -44
                width: 3
                height: 46
                color: root.rope
            }

            Rectangle {
                x: 1
                y: 0
                width: plankSlot.plank ? Math.max(4, plankSlot.plank.length - 2) : 0
                height: 16
                color: root.wood
                border.color: root.outline
                border.width: 2

                Rectangle {
                    x: 2; y: 2
                    width: parent.width - 4
                    height: 3
                    color: root.woodLight
                }
                Rectangle {
                    x: 2; y: parent.height - 5
                    width: parent.width - 4
                    height: 3
                    color: root.woodDark
                }
            }
        }
    }

    // Posts at the cliff edges, standing on the ground with a rope knot on top.
    Repeater {
        model: 8

        Item {
            id: postSlot

            required property int index
            readonly property var post: index < root.posts.length ? root.posts[index] : null

            visible: post !== null
            x: post ? post.x : 0
            y: post ? post.y : 0

            Rectangle {
                x: -9
                y: -70
                width: 18
                height: 90
                color: root.woodDark
                border.color: root.outline
                border.width: 2

                Rectangle {
                    x: 3; y: 2
                    width: 4
                    height: parent.height - 4
                    color: root.wood
                }
            }
            Rectangle {
                x: -12
                y: -54
                width: 24
                height: 8
                color: root.rope
                border.color: root.outline
                border.width: 1
            }
        }
    }
}
