#include "bridge.h"
#include "terrain.h"
#include "../physics/physics.h"

#include <algorithm>
#include <cmath>

namespace Terrain {

namespace {

constexpr double TargetPlankLength = 72.0;  // 0.9 m
constexpr double Slack = 1.015;             // deck is 1.5% longer than the gap
constexpr double JointInverseMass = 1.0 / 0.06;  // each plank weighs 6% of the chassis
// How much the ropes between planks stretch under load (XPBD compliance, in
// pixels of stretch per unit of force): enough for the deck to visibly give
// under the car, then spring back.
constexpr double Compliance = 3.0e-4;
constexpr double Damping = 1.8;             // per second; lets the deck bounce a few times
constexpr double SettleDamping = 8.0;       // only while settling after build
constexpr int SolverIterations = 4;         // per half substep
constexpr double ActiveRange = 60.0 * Physics::PixelsPerMetre;

double weightOf(const BridgeSet::Bridge &bridge, int joint)
{
    // The two end joints are pinned to the cliffs.
    return joint == 0 || joint == bridge.joints.size() - 1 ? 0.0 : JointInverseMass;
}

void integrate(BridgeSet::Bridge &bridge, double h)
{
    bridge.previous = bridge.joints;
    bridge.lambdas.fill(0.0);  // each substep accumulates its plank forces afresh
    for (int i = 1; i + 1 < bridge.joints.size(); ++i) {
        bridge.velocity[i].ry() -= Physics::Gravity * h;
        bridge.joints[i] += bridge.velocity[i] * h;
    }
}

void updateVelocities(BridgeSet::Bridge &bridge, double h, double damping)
{
    const double keep = std::max(0.0, 1.0 - damping * h);
    for (int i = 0; i < bridge.joints.size(); ++i)
        bridge.velocity[i] = weightOf(bridge, i) > 0.0 ? (bridge.joints[i] - bridge.previous[i]) / h * keep : QPointF();
}

} // namespace

void BridgeSet::build(const TerrainManager &terrain)
{
    m_bridges.clear();
    for (const TerrainGap &gap : terrain.gaps()) {
        if (!gap.bridge)
            continue;
        Bridge bridge;
        bridge.startX = gap.start;
        bridge.endX = gap.end;
        const QPointF a(gap.start, terrain.surfaceHeight(gap.start));
        const QPointF b(gap.end, terrain.surfaceHeight(gap.end));
        const double span = Physics::length(b - a);
        const int planks = std::max(4, static_cast<int>(std::lround(span / TargetPlankLength)));
        bridge.plankLength = span * Slack / planks;

        // Start from a parabola of about the right length (a parabola sagging
        // s * span is about 1 + 8/3 s^2 times the span long), then let the
        // simulation settle it properly.
        const double sag = std::sqrt((Slack - 1.0) * 3.0 / 8.0) * span;
        for (int i = 0; i <= planks; ++i) {
            const double u = static_cast<double>(i) / planks;
            bridge.joints.append(a + (b - a) * u - QPointF(0.0, 4.0 * sag * u * (1.0 - u)));
        }
        bridge.previous = bridge.joints;
        bridge.velocity = QVector<QPointF>(bridge.joints.size());
        bridge.lambdas = QVector<double>(planks, 0.0);
        m_bridges.append(bridge);
    }
    m_active = QVector<bool>(m_bridges.size(), true);

    constexpr double SettleStep = 1.0 / 240.0;
    for (Bridge &bridge : m_bridges) {
        for (int step = 0; step < 4 * 240; ++step) {
            integrate(bridge, SettleStep);
            solvePlanks(bridge, 2 * SolverIterations, SettleStep);
            updateVelocities(bridge, SettleStep, SettleDamping);
        }
        bridge.velocity.fill(QPointF());
        bridge.previous = bridge.joints;
        bridge.restJoints = bridge.joints;
    }
}

void BridgeSet::reset()
{
    for (Bridge &bridge : m_bridges) {
        bridge.joints = bridge.restJoints;
        bridge.previous = bridge.restJoints;
        bridge.velocity.fill(QPointF());
    }
}

void BridgeSet::beginStep(double h, double focusX)
{
    for (int b = 0; b < m_bridges.size(); ++b) {
        Bridge &bridge = m_bridges[b];
        m_active[b] = focusX > bridge.startX - ActiveRange && focusX < bridge.endX + ActiveRange;
        if (!m_active[b])
            continue;
        integrate(bridge, h);
        solvePlanks(bridge, SolverIterations, h);
    }
}

void BridgeSet::endStep(double h)
{
    for (int b = 0; b < m_bridges.size(); ++b) {
        if (!m_active[b])
            continue;
        solvePlanks(m_bridges[b], SolverIterations, h);
        updateVelocities(m_bridges[b], h, Damping);
    }
}

void BridgeSet::solvePlanks(Bridge &bridge, int iterations, double h)
{
    // Pull each plank back towards its length, alternating direction so the
    // correction spreads evenly along the chain. This is the compliant (XPBD)
    // form: each plank keeps a running total of the force it has applied this
    // substep (its lambda), so the links settle at a stretch proportional to
    // the load on them, like rope, however many iterations are run.
    const double softness = Compliance / (h * h);
    const int planks = bridge.joints.size() - 1;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        const bool forward = iteration % 2 == 0;
        for (int k = 0; k < planks; ++k) {
            const int i = forward ? k : planks - 1 - k;
            const double wa = weightOf(bridge, i);
            const double wb = weightOf(bridge, i + 1);
            if (wa + wb <= 0.0)
                continue;
            const QPointF delta = bridge.joints[i + 1] - bridge.joints[i];
            const double distance = Physics::length(delta);
            if (distance < 1e-9)
                continue;
            const QPointF direction = delta / distance;
            const double stretch = distance - bridge.plankLength;
            const double dLambda = (-stretch - softness * bridge.lambdas[i]) / (wa + wb + softness);
            bridge.lambdas[i] += dLambda;
            bridge.joints[i] -= direction * (wa * dLambda);
            bridge.joints[i + 1] += direction * (wb * dLambda);
        }
    }
}

bool BridgeSet::findContact(const QPointF &center, double radius, Contact *contact) const
{
    bool found = false;
    for (int b = 0; b < m_bridges.size(); ++b) {
        const Bridge &bridge = m_bridges[b];
        if (center.x() < bridge.startX - radius || center.x() > bridge.endX + radius)
            continue;
        for (int i = 0; i + 1 < bridge.joints.size(); ++i) {
            const QPointF a = bridge.joints[i];
            const QPointF segment = bridge.joints[i + 1] - a;
            const double lengthSq = QPointF::dotProduct(segment, segment);
            if (lengthSq < 1e-9)
                continue;
            const double t = Physics::clampValue(QPointF::dotProduct(center - a, segment) / lengthSq, 0.0, 1.0);
            const QPointF closest = a + segment * t;
            const QPointF up = QPointF(-segment.y(), segment.x()) / std::sqrt(lengthSq);  // deck's upper side
            QPointF normal;
            double depth = 0.0;
            if (t > 0.0 && t < 1.0) {
                // Over the plank: measure from its top face. Only the top
                // pushes back, so something well below the deck passes under.
                const double above = QPointF::dotProduct(center - a, up);
                if (above >= radius || above < -0.75 * radius)
                    continue;
                normal = up;
                depth = radius - above;
            } else {
                // Past the end of a plank: round off the joint.
                const QPointF away = center - closest;
                const double distance = Physics::length(away);
                if (distance >= radius || distance < 1e-9 || QPointF::dotProduct(away, up) <= 0.0)
                    continue;
                normal = away / distance;
                depth = radius - distance;
            }
            if (!found || depth > contact->depth) {
                *contact = {b, i, t, normal, depth};
                found = true;
            }
        }
    }
    return found;
}

double BridgeSet::inverseMassAt(const Contact &contact) const
{
    const Bridge &bridge = m_bridges.at(contact.bridge);
    const double s = 1.0 - contact.t;
    return s * s * weightOf(bridge, contact.plank) + contact.t * contact.t * weightOf(bridge, contact.plank + 1);
}

void BridgeSet::push(const Contact &contact, const QPointF &impulse)
{
    Bridge &bridge = m_bridges[contact.bridge];
    bridge.joints[contact.plank] += impulse * (weightOf(bridge, contact.plank) * (1.0 - contact.t));
    bridge.joints[contact.plank + 1] += impulse * (weightOf(bridge, contact.plank + 1) * contact.t);
}

} // namespace Terrain
