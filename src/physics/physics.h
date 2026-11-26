#pragma once

#include <QPointF>
#include <array>

namespace Physics {

// Physics constants
constexpr double Pi = 3.14159265358979323846;
constexpr double WheelRadius = 40.0;

// Car artwork geometry
constexpr double BodyWheelReferenceRadius = 180.0;
constexpr double CarSpriteScale = WheelRadius / BodyWheelReferenceRadius;
constexpr double RearWheelSourceX = 395.0;
constexpr double FrontWheelSourceX = 1345.0;
constexpr double WheelSourceY = 830.0;
constexpr double WheelSpriteTargetRadius = WheelRadius * 1254.0 / 1076.0;
constexpr double WheelOffset = (FrontWheelSourceX - RearWheelSourceX) * 0.5 * CarSpriteScale;
constexpr double WheelLocalY = -43.0;
constexpr double CarSourceCenterX = (RearWheelSourceX + FrontWheelSourceX) * 0.5;

// Vehicle physics parameters
constexpr double PixelsPerMetre = 80.0;
constexpr double Gravity = 9.81 * PixelsPerMetre;
constexpr int PhysicsSubsteps = 16;
constexpr double ChassisMass = 1.0;
constexpr double ChassisInertia = ChassisMass * 70.0 * 70.0;
constexpr double WheelMass = 0.15;
constexpr double WheelInertia = 0.5 * WheelMass * WheelRadius * WheelRadius;
constexpr double CenterOfMassY = -28.0;
constexpr double SuspensionSag = 10.0;
constexpr double SuspensionStiffness = ChassisMass * Gravity * 0.5 / SuspensionSag;
constexpr double SuspensionDamping = 4.5;
constexpr double SuspensionMinExtension = -14.0;
constexpr double SuspensionMaxExtension = 20.0;

constexpr double DriveTorque = 26000.0;
constexpr double BoostDriveTorque = 34000.0;
constexpr double MaxWheelSpin = 30.0;   // 15 m/s
constexpr double BoostWheelSpin = 36.0; // 18 m/s
constexpr double ReverseTorque = 12000.0;
constexpr double MaxReverseSpin = 9.0;
constexpr double BrakeTorque = 32000.0;
constexpr double RollingResistance = 0.35;
constexpr double AirControl = 3.6;
constexpr double MaxChassisSpin = 7.0;
constexpr double MaxPushOutSpeed = 150.0;

constexpr double FlippedAngle = 1.75;
constexpr double OnSideAngle = 1.2;
constexpr double FlipGraceTime = 0.4;

constexpr double TyreStaticFriction = 1.8;
constexpr double TyreDynamicFriction = 1.5;
constexpr double BodyStaticFriction = 0.6;
constexpr double BodyDynamicFriction = 0.45;

// How a map's ground behaves under the tyres. The default is grassland.
struct SurfaceModel {
    double gravityScale = 1.0;  // moon: well under 1
    double driveScale = 1.0;    // multiplies engine torque; eased off in low gravity, where
                                // full torque would lift the nose and flip the car
    double tyreGrip = 1.0;      // multiplies the tyres' static and dynamic friction

    // Sand: a slow tyre sinks in and drags; a fast one rides on top.
    bool sand = false;
    double sinkMax = 0.0;       // deepest a tyre can sink (px)
    double sinkSpeed = 0.0;     // at or above this speed (px/s) a tyre does not sink
    double sinkRate = 0.0;      // how fast a slow tyre sinks (px/s)
    double riseRate = 0.0;      // how fast a fast or airborne tyre comes back up (px/s)
    double sinkDrag = 0.0;      // drag on a fully sunk tyre, per second

    // Mud: patches along the course (TerrainManager::mudAt) that drag the tyres.
    double mudDrag = 0.0;       // per second, at full patch strength
};

// Rigid body state
struct RigidBody {
    QPointF position;
    QPointF velocity;
    double angle = 0.0;
    double angularVelocity = 0.0;
    double inverseMass = 0.0;
    double inverseInertia = 0.0;
    QPointF previousPosition;
    double previousAngle = 0.0;
};

// Body collider definition for vehicle chassis artwork
struct BodyCollider {
    double x;
    double y;
    double radius;
    bool head;
};

extern const std::array<BodyCollider, 8> ChassisColliders;

// Vector and rigid body math helpers
double clampValue(double value, double minimum, double maximum);
QPointF rotatePoint(const QPointF &point, double angle);
double cross(const QPointF &a, const QPointF &b);
double length(const QPointF &v);
QPointF pointVelocity(const RigidBody &body, const QPointF &arm);
double generalizedInverseMass(const RigidBody &body, const QPointF &arm, const QPointF &direction);
double applyCorrection(RigidBody &a, RigidBody *b, const QPointF &armA, const QPointF &armB,
                       const QPointF &delta, double compliance, double h);
// Like applyCorrection against a second object that is not a RigidBody (such
// as a point on a bridge deck), given how easily that object moves along
// delta. Moves only body; returns lambda so the caller can move the other
// object by -delta direction * lambda times its own inverse mass.
double applyCorrectionAgainst(RigidBody &body, const QPointF &arm, const QPointF &delta,
                              double otherInverseMass);
void applyImpulse(RigidBody &body, const QPointF &arm, const QPointF &impulse);
QPointF chassisLocal(double x, double y);

} // namespace Physics
