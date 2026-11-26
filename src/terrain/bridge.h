#pragma once

// Plank bridges over gorges.
//
// Each bridge is a chain of joints, one plank between each pair, pinned to the
// two cliff edges. It is a little longer than the gap, so it hangs in a gentle
// sag under its own weight. The joints are simulated with the same
// position-based method as the car (see vehicle.cpp): every substep they fall
// under gravity, then each plank is pulled back to its length. When a wheel or
// the car body presses on a plank, the push is shared between the car and the
// two joints at that plank's ends, by their masses. So the deck dips and bends
// under the car, then springs back and settles once it has passed.
//
// World pixels, y up, like the rest of the simulation.

#include <QPointF>
#include <QVector>

namespace Terrain {

class TerrainManager;

class BridgeSet {
public:
    struct Bridge {
        double startX = 0.0;            // the two cliff edges it is pinned to
        double endX = 0.0;
        double plankLength = 0.0;       // rest length of every plank
        QVector<QPointF> joints;        // first and last are pinned
        QVector<QPointF> previous;      // joint positions at the start of the substep
        QVector<QPointF> velocity;
        QVector<double> lambdas;        // per plank: force applied so far this substep
        QVector<QPointF> restJoints;    // the settled shape, used on reset
    };

    // Where a circle touches a bridge deck.
    struct Contact {
        int bridge = -1;
        int plank = -1;      // joints plank and plank + 1
        double t = 0.0;      // position along the plank, 0 to 1
        QPointF normal;      // pointing out of the deck, towards the circle
        double depth = 0.0;
    };

    // Builds a bridge over every gap marked as bridged, then lets each one
    // settle into its resting sag.
    void build(const TerrainManager &terrain);
    // Puts every bridge back to its resting shape, at rest.
    void reset();

    // One substep is begin, then the car's contacts (which call push), then end.
    // Only bridges near focusX are simulated; the others are at rest anyway.
    void beginStep(double h, double focusX);
    void endStep(double h);

    // The deepest contact between a circle and any bridge deck, if there is one.
    bool findContact(const QPointF &center, double radius, Contact *contact) const;
    // How easily the touched point of the deck moves, for sharing a push.
    [[nodiscard]] double inverseMassAt(const Contact &contact) const;
    // Moves the touched point of the deck by a position-level impulse,
    // spread over the plank's two joints.
    void push(const Contact &contact, const QPointF &impulse);

    [[nodiscard]] const QVector<Bridge> &bridges() const { return m_bridges; }

private:
    void solvePlanks(Bridge &bridge, int iterations, double h);

    QVector<Bridge> m_bridges;
    QVector<bool> m_active;
};

} // namespace Terrain
