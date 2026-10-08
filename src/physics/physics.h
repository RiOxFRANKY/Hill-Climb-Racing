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
constexpr int PhysicsSubsteps = 16;
constexpr double ChassisMass = 1.0;
constexpr double ChassisInertia = ChassisMass * 70.0 * 70.0;
constexpr double WheelMass = 0.15;
constexpr double WheelInertia = 0.5 * WheelMass * WheelRadius * WheelRadius;
constexpr double CenterOfMassY = -28.0;
constexpr double SuspensionSag = 10.0;
constexpr double SuspensionDamping = 4.5;
constexpr double SuspensionMinExtension = -14.0;
constexpr double SuspensionMaxExtension = 20.0;

// DYNAMIC PHYSICS (Changed from constexpr to inline for C++17 runtime modification)
inline double Gravity = 9.81 * PixelsPerMetre;
inline double SuspensionStiffness = ChassisMass * Gravity * 0.5 / SuspensionSag;

// Helper to switch gravity based on the planet/level
inline void setPlanetGravity(double gravityMetersPerSecond) {
    Gravity = gravityMetersPerSecond * PixelsPerMetre;
    // Suspension must adjust to the new gravity so the car doesn't bounce violently
    SuspensionStiffness = ChassisMass * Gravity * 0.5 / SuspensionSag;
}

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
void applyImpulse(RigidBody &body, const QPointF &arm, const QPointF &impulse);
QPointF chassisLocal(double x, double y);

} // namespace Physics