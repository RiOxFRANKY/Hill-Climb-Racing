#include "physics.h"

#include <algorithm>
#include <cmath>

namespace Physics {

const std::array<BodyCollider, 8> ChassisColliders{{
    {-165.0, 8.0, 20.0, false},   // spare tyre
    {-138.0, -20.0, 9.0, false},  // rear bumper
    {142.0, -5.0, 11.0, false},   // front bumper
    {125.0, 30.0, 9.0, false},    // bonnet
    {0.0, -32.0, 10.0, false},    // skid plate
    {-90.0, 90.0, 9.0, false},    // roll cage
    {28.0, 92.0, 9.0, false},     // windscreen frame
    {-38.0, 80.0, 16.0, true},    // driver's head
}};

double clampValue(double value, double minimum, double maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

QPointF rotatePoint(const QPointF &point, double angle)
{
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {point.x() * c - point.y() * s,
            point.x() * s + point.y() * c};
}

double cross(const QPointF &a, const QPointF &b)
{
    return a.x() * b.y() - a.y() * b.x();
}

double length(const QPointF &v)
{
    return std::sqrt(QPointF::dotProduct(v, v));
}

QPointF pointVelocity(const RigidBody &body, const QPointF &arm)
{
    return body.velocity + QPointF(-body.angularVelocity * arm.y(),
                                   body.angularVelocity * arm.x());
}

double generalizedInverseMass(const RigidBody &body, const QPointF &arm, const QPointF &direction)
{
    const double turn = cross(arm, direction);
    return body.inverseMass + body.inverseInertia * turn * turn;
}

double applyCorrection(RigidBody &a, RigidBody *b, const QPointF &armA, const QPointF &armB,
                       const QPointF &delta, double compliance, double h)
{
    const double magnitude = length(delta);
    if (magnitude < 1e-9)
        return 0.0;

    const QPointF direction = delta / magnitude;
    double weight = generalizedInverseMass(a, armA, direction);
    if (b)
        weight += generalizedInverseMass(*b, armB, direction);
    const double lambda = magnitude / (weight + compliance / (h * h));
    const QPointF impulse = direction * lambda;

    a.position += impulse * a.inverseMass;
    a.angle += a.inverseInertia * cross(armA, impulse);
    if (b) {
        b->position -= impulse * b->inverseMass;
        b->angle -= b->inverseInertia * cross(armB, impulse);
    }
    return lambda;
}

void applyImpulse(RigidBody &body, const QPointF &arm, const QPointF &impulse)
{
    body.velocity += impulse * body.inverseMass;
    body.angularVelocity += body.inverseInertia * cross(arm, impulse);
}

QPointF chassisLocal(double x, double y)
{
    return QPointF(x, y - CenterOfMassY);
}

} // namespace Physics
