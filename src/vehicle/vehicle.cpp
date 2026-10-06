#include "vehicle.h"
#include "../terrain/terrain.h"

#include <QVarLengthArray>
#include <algorithm>
#include <cmath>

namespace Vehicle {

using namespace Physics;

void VehicleEntity::reset(double spawnX, double surfaceHeightAtSpawn)
{
    m_chassis = RigidBody{};
    m_chassis.position = QPointF(spawnX, surfaceHeightAtSpawn + WheelRadius
                                              - chassisLocal(0.0, WheelLocalY).y() + 4.0);
    m_chassis.inverseMass = 1.0 / ChassisMass;
    m_chassis.inverseInertia = 1.0 / ChassisInertia;
    for (int i = 0; i < 2; ++i) {
        RigidBody &wheel = m_wheels[i];
        wheel = RigidBody{};
        wheel.position = m_chassis.position
                         + chassisLocal(i == 0 ? -WheelOffset : WheelOffset, WheelLocalY);
        wheel.inverseMass = 1.0 / WheelMass;
        wheel.inverseInertia = 1.0 / WheelInertia;
    }
    m_wheelContacts = 0;
    m_headHit = false;
    m_bodyContact = false;
    m_flipTimer = 0.0;
    m_fuel = 100.0;
}

void VehicleEntity::stepPhysics(double h, double throttle, bool boost, const Terrain::TerrainManager &terrain)
{
    struct Contact {
        RigidBody *body;
        QPointF arm;
        QPointF normal;
        double normalLambda;
        double dynamicFriction;
        bool stuck;
    };

    RigidBody *const bodies[] = {&m_chassis, &m_wheels[0], &m_wheels[1]};
    const bool hasFuel = m_fuel > 0.0;

    // 1. Integrate external forces, engine and brakes.
    for (RigidBody *body : bodies) {
        body->previousPosition = body->position;
        body->previousAngle = body->angle;
        body->velocity.ry() -= Gravity * h;
    }

    const double driveTorque = boost ? BoostDriveTorque : DriveTorque;
    const double maxSpin = boost ? BoostWheelSpin : MaxWheelSpin;
    for (RigidBody &wheel : m_wheels) {
        const double spin = -wheel.angularVelocity;
        double torque = 0.0;
        if (throttle > 0.0 && hasFuel) {
            torque = -driveTorque * clampValue((maxSpin - spin) / (maxSpin * 0.15), 0.0, 1.0);
        } else if (throttle < 0.0) {
            if (spin > 0.5)
                torque = std::min(BrakeTorque, spin / (wheel.inverseInertia * h));
            else if (hasFuel)
                torque = ReverseTorque * clampValue(1.0 + spin / MaxReverseSpin, 0.0, 1.0);
        }

        wheel.angularVelocity += torque * wheel.inverseInertia * h;
        m_chassis.angularVelocity -= torque * m_chassis.inverseInertia * h;
        wheel.angularVelocity *= 1.0 - RollingResistance * h;
    }

    if (m_wheelContacts == 0) {
        m_chassis.angularVelocity += throttle * AirControl * h;
        m_chassis.angularVelocity *= 1.0 - 0.3 * h;
    }
    m_chassis.angularVelocity = clampValue(m_chassis.angularVelocity, -MaxChassisSpin, MaxChassisSpin);

    for (RigidBody *body : bodies) {
        body->position += body->velocity * h;
        body->angle += body->angularVelocity * h;
    }

    // 2. Suspension
    for (int i = 0; i < 2; ++i) {
        RigidBody &wheel = m_wheels[i];
        const QPointF mount = chassisLocal(i == 0 ? -WheelOffset : WheelOffset, WheelLocalY);

        const auto extension = [&](QPointF *up, QPointF *side) {
            *up = rotatePoint(QPointF(0.0, 1.0), m_chassis.angle);
            *side = rotatePoint(QPointF(1.0, 0.0), m_chassis.angle);
            const QPointF anchor = m_chassis.position + rotatePoint(mount, m_chassis.angle);
            return wheel.position - anchor;
        };

        QPointF up;
        QPointF side;
        QPointF offset = extension(&up, &side);
        applyCorrection(wheel, &m_chassis, QPointF(), wheel.position - m_chassis.position,
                        -side * QPointF::dotProduct(offset, side), 0.0, h);

        offset = extension(&up, &side);
        const double travel = -QPointF::dotProduct(offset, up);
        if (travel > SuspensionMaxExtension || travel < SuspensionMinExtension) {
            const double limit = clampValue(travel, SuspensionMinExtension, SuspensionMaxExtension);
            applyCorrection(wheel, &m_chassis, QPointF(), wheel.position - m_chassis.position,
                            up * (travel - limit), 0.0, h);
        }

        offset = extension(&up, &side);
        const double stretch = -QPointF::dotProduct(offset, up) - SuspensionSag;
        applyCorrection(wheel, &m_chassis, QPointF(), wheel.position - m_chassis.position,
                        up * stretch, 1.0 / SuspensionStiffness, h);
    }

    // 3. Ground contacts with static friction
    QVarLengthArray<Contact, 12> contacts;
    const auto collide = [&](RigidBody &body, const QPointF &center, double radius,
                             double staticFriction, double dynamicFriction) {
        QPointF normal;
        double depth = 0.0;
        if (!terrain.findTerrainContact(center, radius, &normal, &depth))
            return false;

        const QPointF arm = center - body.position - normal * radius;
        const double normalSpeed = QPointF::dotProduct(pointVelocity(body, arm), normal);
        const double correction =
            std::min(depth, std::max(0.0, (MaxPushOutSpeed - normalSpeed) * h));
        const double normalLambda =
            applyCorrection(body, nullptr, arm, QPointF(), normal * correction, 0.0, h);

        const QPointF localArm = rotatePoint(arm, -body.angle);
        const QPointF before = body.previousPosition + rotatePoint(localArm, body.previousAngle);
        const QPointF now = body.position + arm;
        const QPointF slip = now - before;
        const QPointF tangentialSlip = slip - normal * QPointF::dotProduct(slip, normal);
        const double slipLength = length(tangentialSlip);
        bool stuck = false;
        if (slipLength > 1e-9) {
            const double tangentialLambda =
                slipLength / generalizedInverseMass(body, arm, tangentialSlip / slipLength);
            if (tangentialLambda < staticFriction * normalLambda) {
                applyCorrection(body, nullptr, arm, QPointF(), -tangentialSlip, 0.0, h);
                stuck = true;
            }
        }

        contacts.append({&body, arm, normal, normalLambda, dynamicFriction, stuck});
        return true;
    };

    m_wheelContacts = 0;
    for (RigidBody &wheel : m_wheels) {
        if (collide(wheel, wheel.position, WheelRadius, TyreStaticFriction, TyreDynamicFriction))
            ++m_wheelContacts;
    }
    for (const BodyCollider &collider : ChassisColliders) {
        const QPointF center =
            m_chassis.position + rotatePoint(chassisLocal(collider.x, collider.y), m_chassis.angle);
        if (collide(m_chassis, center, collider.radius, BodyStaticFriction, BodyDynamicFriction)) {
            m_bodyContact = true;
            if (collider.head)
                m_headHit = true;
        }
    }

    // 4. Derive velocities from the corrected positions.
    for (RigidBody *body : bodies) {
        body->velocity = (body->position - body->previousPosition) / h;
        body->angularVelocity = (body->angle - body->previousAngle) / h;
    }

    // 5. Velocity-level effects
    for (int i = 0; i < 2; ++i) {
        RigidBody &wheel = m_wheels[i];
        const QPointF up = rotatePoint(QPointF(0.0, 1.0), m_chassis.angle);
        const QPointF arm = wheel.position - m_chassis.position;
        const double relative =
            QPointF::dotProduct(wheel.velocity - pointVelocity(m_chassis, arm), up);
        const double weight =
            generalizedInverseMass(wheel, QPointF(), up) + generalizedInverseMass(m_chassis, arm, up);
        const double impulse = -relative * std::min(SuspensionDamping * h, 1.0 / weight);
        applyImpulse(wheel, QPointF(), up * impulse);
        applyImpulse(m_chassis, arm, -up * impulse);
    }

    for (const Contact &contact : contacts) {
        if (contact.stuck)
            continue;
        const QPointF velocity = pointVelocity(*contact.body, contact.arm);
        const QPointF tangential =
            velocity - contact.normal * QPointF::dotProduct(velocity, contact.normal);
        const double speed = length(tangential);
        if (speed < 1e-9)
            continue;
        const QPointF direction = tangential / speed;
        const double normalForce = contact.normalLambda / (h * h);
        const double weight = generalizedInverseMass(*contact.body, contact.arm, direction);
        const double impulse = std::min(h * contact.dynamicFriction * normalForce, speed / weight);
        applyImpulse(*contact.body, contact.arm, -direction * impulse);
    }
}

} // namespace Vehicle
