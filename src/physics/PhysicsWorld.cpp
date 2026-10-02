#include "physics/PhysicsWorld.h"
#include <algorithm>
#include <cmath>

namespace Physics {

namespace {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    // Helper: Clamps value between min and max
    inline float clamp(float v, float minV, float maxV) {
        return std::max(minV, std::min(maxV, v));
    }

    // Helper: Distance from point P to line segment AB, returning closest point C and normal
    bool testCircleSegment(const Vec2& circlePos, float radius, const Vec2& a, const Vec2& b,
                           Vec2& outClosest, Vec2& outNormal, float& outPenetration) {
        Vec2 ab = b - a;
        float abLenSq = ab.lengthSq();
        if (abLenSq < 0.0001f) {
            return false;
        }

        float t = clamp((circlePos - a).dot(ab) / abLenSq, 0.0f, 1.0f);
        outClosest = a + ab * t;
        Vec2 delta = circlePos - outClosest;
        float dist = delta.length();

        if (dist < radius) {
            outPenetration = radius - dist;
            if (dist > 0.0001f) {
                outNormal = delta / dist;
            } else {
                // If directly on segment, choose upward perpendicular normal
                outNormal = Vec2(-ab.y, ab.x).normalized();
                if (outNormal.y > 0.0f) {
                    outNormal = outNormal * -1.0f; // Ensure pointing upwards
                }
            }
            return true;
        }
        return false;
    }
}

PhysicsWorld::PhysicsWorld()
    : m_config() {
    reset(Vec2(100.0f, 250.0f));
}

PhysicsWorld::PhysicsWorld(const Config& config)
    : m_config(config) {
    reset(Vec2(100.0f, 250.0f));
}

void PhysicsWorld::reset(Vec2 spawnPos) {
    // spawnPos represents visual car spawn position.
    // Center of Mass is positioned at spawnPos + centerOfMassOffset
    m_chassis.position = spawnPos + m_config.centerOfMassOffset;
    m_chassis.velocity = Vec2(0.0f, 0.0f);
    m_chassis.angle = 0.0f;
    m_chassis.angularVelocity = 0.0f;

    m_rearWheel.position = m_chassis.position + rearMountCoM() + Vec2(0.0f, m_config.restLength);
    m_rearWheel.velocity = Vec2(0.0f, 0.0f);
    m_rearWheel.angle = 0.0f;
    m_rearWheel.angularVelocity = 0.0f;
    m_rearWheel.isGrounded = false;

    m_frontWheel.position = m_chassis.position + frontMountCoM() + Vec2(0.0f, m_config.restLength);
    m_frontWheel.velocity = Vec2(0.0f, 0.0f);
    m_frontWheel.angle = 0.0f;
    m_frontWheel.angularVelocity = 0.0f;
    m_frontWheel.isGrounded = false;

    m_throttle = 0.0f;
    m_headCollided = false;
}

void PhysicsWorld::setThrottle(float amount) {
    m_throttle = clamp(amount, -1.0f, 1.0f);
}

void PhysicsWorld::clearTerrain() {
    m_terrainSegments.clear();
}

void PhysicsWorld::addTerrainSegment(Vec2 p1, Vec2 p2, float friction) {
    m_terrainSegments.push_back({p1, p2, friction});
}

VehiclePhysicsState PhysicsWorld::getVehicleState() const {
    VehiclePhysicsState s;
    // Visual chassis position = Center of Mass minus rotated CoM offset
    Vec2 visualPos = m_chassis.position - m_config.centerOfMassOffset.rotated(m_chassis.angle);
    s.chassis.position = visualPos;
    s.chassis.angleDegrees = m_chassis.angle * RAD_TO_DEG;

    s.rearWheel.position = m_rearWheel.position;
    s.rearWheel.angleDegrees = m_rearWheel.angle * RAD_TO_DEG;
    s.rearWheelGrounded = m_rearWheel.isGrounded;

    s.frontWheel.position = m_frontWheel.position;
    s.frontWheel.angleDegrees = m_frontWheel.angle * RAD_TO_DEG;
    s.frontWheelGrounded = m_frontWheel.isGrounded;

    // Driver head world position anchored relative to visual chassis
    Vec2 headWorld = visualPos + visualHeadMount().rotated(m_chassis.angle);
    s.driverHead.position = headWorld;
    s.driverHead.angleDegrees = s.chassis.angleDegrees;

    s.velocity = m_chassis.velocity;
    s.angularVelocity = m_chassis.angularVelocity;
    s.headCollided = m_headCollided;

    return s;
}

void PhysicsWorld::step(float dt) {
    if (dt <= 0.0f) return;

    int steps = std::max(1, m_config.subSteps);
    float subDt = dt / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        subStep(subDt);
    }
}

void PhysicsWorld::subStep(float subDt) {
    Vec2 gravityVec(0.0f, m_config.gravity);

    // 1. Reset ground contact flags
    m_rearWheel.isGrounded = false;
    m_frontWheel.isGrounded = false;

    // 2. Solve wheel terrain collisions first to establish ground normals, traction and contacts
    solveWheelTerrainCollision(m_rearWheel, subDt);
    solveWheelTerrainCollision(m_frontWheel, subDt);

    // 3. Solve suspensions (Spring-damper forces connecting chassis and wheels relative to CoM)
    solveSuspension(m_rearWheel, rearMountCoM(), subDt);
    solveSuspension(m_frontWheel, frontMountCoM(), subDt);

    // 4. Airborne dynamics & Pitch Behavior vs Ground Alignment
    bool rearGrounded = m_rearWheel.isGrounded;
    bool frontGrounded = m_frontWheel.isGrounded;
    bool inAir = (!rearGrounded && !frontGrounded);
    bool bothGrounded = (rearGrounded && frontGrounded);

    if (inAir) {
        // Natural airborne nose-dive (Hill Climb Racing signature mechanic):
        // Front-heavy mass distribution and aerodynamic forward airflow naturally tilt
        // the nose down slowly and progressively when launched off a hill or edge.
        float forwardSpeed = std::max(0.0f, m_chassis.velocity.x);
        float speedFactor = clamp(forwardSpeed / 300.0f, 0.45f, 1.4f);
        float naturalNoseDive = (m_config.airNoseDiveTorque * speedFactor / m_config.chassisInertia) * subDt;
        m_chassis.angularVelocity += naturalNoseDive;

        // Player aerial pitch control:
        // Gas (Right Arrow / D): smoothly pulls nose up to level out or counter dive
        // Brake (Left Arrow / A): actively dips nose down faster for steep landing slopes
        if (m_throttle > 0.05f) {
            m_chassis.angularVelocity -= (m_config.airTorque / m_config.chassisInertia) * m_throttle * subDt;
        } else if (m_throttle < -0.05f) {
            m_chassis.angularVelocity -= (m_config.airTorque / m_config.chassisInertia) * m_throttle * subDt;
        }
    } else if (bothGrounded) {
        // When BOTH wheels are on the ground, align chassis with the wheel baseline slope
        float wheelBaseAngle = std::atan2(m_frontWheel.position.y - m_rearWheel.position.y,
                                          m_frontWheel.position.x - m_rearWheel.position.x);
        float angleDiff = wheelBaseAngle - m_chassis.angle;
        while (angleDiff > PI) angleDiff -= 2.0f * PI;
        while (angleDiff < -PI) angleDiff += 2.0f * PI;

        // Apply restoring torque to keep chassis aligned with terrain slope
        m_chassis.angularVelocity += angleDiff * (m_config.groundStabilizer * subDt);
    } else {
        // One wheel airborne (e.g., front wheel driving off a cliff or ledge)
        // With no front ground support, gravity on the front-biased CoM and upward rear suspension
        // pivot naturally rotate the chassis nose-down immediately off the edge!
    }

    // 5. Integrate chassis linear velocity & position
    m_chassis.velocity += gravityVec * subDt;
    m_chassis.position += m_chassis.velocity * subDt;

    // Strong angular damping to prevent wild spinning
    m_chassis.angularVelocity *= (1.0f - m_config.angularDamping * subDt);
    m_chassis.angle += m_chassis.angularVelocity * subDt;

    // 6. Integrate wheels
    m_rearWheel.velocity += gravityVec * subDt;
    m_rearWheel.position += m_rearWheel.velocity * subDt;
    m_rearWheel.angle += m_rearWheel.angularVelocity * subDt;

    m_frontWheel.velocity += gravityVec * subDt;
    m_frontWheel.position += m_frontWheel.velocity * subDt;
    m_frontWheel.angle += m_frontWheel.angularVelocity * subDt;

    // 7. Solve chassis bottom bumper collision against terrain
    solveChassisTerrainCollision(subDt);

    // 8. Test driver head collision
    if (!m_headCollided) {
        m_headCollided = checkHeadTerrainCollision();
    }
}

void PhysicsWorld::solveSuspension(WheelBody& wheel, const Vec2& mountLocal, float subDt) {
    // 1. Suspension strut anchor in world space
    Vec2 mountWorld = m_chassis.position + mountLocal.rotated(m_chassis.angle);

    // Strut direction points downwards from chassis bottom
    Vec2 strutDir = Vec2(0.0f, 1.0f).rotated(m_chassis.angle);
    Vec2 lateralDir = Vec2(-strutDir.y, strutDir.x);

    // 2. Relative velocity at mount point on chassis
    Vec2 mountVelocity = m_chassis.velocity + Vec2(-mountLocal.y, mountLocal.x).rotated(m_chassis.angle) * m_chassis.angularVelocity;
    Vec2 relVel = wheel.velocity - mountVelocity;

    // 3. Current suspension length along strut axis
    Vec2 toWheel = wheel.position - mountWorld;
    float currentLength = toWheel.dot(strutDir);

    // Clamp strictly within travel limits (no over-stretching!)
    currentLength = clamp(currentLength, m_config.minLength, m_config.maxLength);

    // 4. Spring displacement and spring force along strut
    float displacement = currentLength - m_config.restLength;
    float relVelAlongStrut = relVel.dot(strutDir);

    // Hooke's Law + viscous damping along strut axis
    float springForce = -m_config.springK * displacement - m_config.dampingC * relVelAlongStrut;

    Vec2 forceOnWheel = strutDir * springForce;
    Vec2 forceOnChassis = forceOnWheel * -1.0f;

    wheel.velocity += (forceOnWheel / m_config.wheelMass) * subDt;
    m_chassis.velocity += (forceOnChassis / m_config.chassisMass) * subDt;

    // Chassis torque produced by suspension force around Center of Mass
    Vec2 rChassis = mountWorld - m_chassis.position;
    float torque = rChassis.cross(forceOnChassis) * 0.40f;
    m_chassis.angularVelocity += (torque / m_config.chassisInertia) * subDt;

    // 5. RIGID STRUT CONSTRAINT:
    // Wheel is locked to the strut line, completely eliminating horizontal drift,
    // rubber-banding, or wheels crossing over each other!
    wheel.position = mountWorld + strutDir * currentLength;

    // Cancel relative lateral velocity so wheel tracks rigidly with the chassis stance
    float lateralRelVel = relVel.dot(lateralDir);
    wheel.velocity -= lateralDir * lateralRelVel;
}

void PhysicsWorld::solveWheelTerrainCollision(WheelBody& wheel, float subDt) {
    for (const auto& seg : m_terrainSegments) {
        Vec2 closest, normal;
        float penetration = 0.0f;

        if (testCircleSegment(wheel.position, m_config.wheelRadius, seg.p1, seg.p2, closest, normal, penetration)) {
            wheel.isGrounded = true;
            wheel.groundNormal = normal;
            wheel.groundFriction = seg.friction * m_config.tireFriction;

            // Push wheel out of ground along collision normal
            wheel.position += normal * penetration;

            // Normal impulse cancels penetration velocity
            float vn = wheel.velocity.dot(normal);
            if (vn < 0.0f) {
                wheel.velocity -= normal * vn;
            }

            // Tangent direction along the terrain slope pointing forward (+X)
            Vec2 tangent(-normal.y, normal.x);
            if (tangent.x < 0.0f) {
                tangent = tangent * -1.0f;
            }

            float groundLinearSpeed = wheel.velocity.dot(tangent);

            // Engine driving torque delivered to wheel
            if (m_throttle > 0.01f) {
                // Forward gas: smooth progressive acceleration
                wheel.angularVelocity += (m_config.driveTorque / m_config.wheelMass) * m_throttle * subDt;
            } else if (m_throttle < -0.01f) {
                // Reverse or Brake input:
                if (groundLinearSpeed > 4.0f) {
                    // Car is rolling forward: apply stopping brake torque!
                    wheel.angularVelocity -= (m_config.brakeTorque / m_config.wheelMass) * subDt;
                } else {
                    // Car is stationary or reversing: apply gentle reverse torque (slower than forward)
                    wheel.angularVelocity += (m_config.reverseTorque / m_config.wheelMass) * m_throttle * subDt;
                }
            } else {
                // Rolling resistance when coasting
                wheel.angularVelocity *= (1.0f - 1.5f * subDt);
            }

            // Clamp wheel speed: forward up to maxWheelSpeed, reverse capped to maxReverseSpeed
            wheel.angularVelocity = clamp(wheel.angularVelocity, -m_config.maxReverseSpeed, m_config.maxWheelSpeed);

            // Recompute peripheral speed and slip
            float wheelPeripheralSpeed = wheel.angularVelocity * m_config.wheelRadius;
            float slipSpeed = groundLinearSpeed - wheelPeripheralSpeed;

            // Normal load on wheel (wheel mass + half chassis weight)
            float normalLoad = (m_config.wheelMass + m_config.chassisMass * 0.5f) * m_config.gravity;
            float maxTractionForce = wheel.groundFriction * normalLoad;

            // Standstill static friction deadband: locks wheels at zero when coasted to halt
            if (std::abs(m_throttle) < 0.01f && std::abs(groundLinearSpeed) < 5.0f && std::abs(wheel.angularVelocity) < 0.6f) {
                wheel.velocity -= tangent * groundLinearSpeed;
                m_chassis.velocity -= tangent * (m_chassis.velocity.dot(tangent) * 0.5f);
                wheel.angularVelocity = 0.0f;
            } else {
                // High-traction grip: tightly couples wheel rotation to ground motion with no slip
                float tractionForce = -clamp(slipSpeed * 150.0f, -maxTractionForce, maxTractionForce);

                float totalMass = m_config.chassisMass + 2.0f * m_config.wheelMass;
                Vec2 accel = tangent * (tractionForce / totalMass * subDt);
                wheel.velocity += accel;
                m_chassis.velocity += accel;

                // Wheel angular reaction from ground traction
                float wheelInertia = 0.5f * m_config.wheelMass * m_config.wheelRadius * m_config.wheelRadius;
                float torqueReaction = -tractionForce * m_config.wheelRadius;
                wheel.angularVelocity += (torqueReaction / wheelInertia) * (0.08f * subDt);
            }
        }
    }
}

void PhysicsWorld::solveChassisTerrainCollision(float subDt) {
    // 3 bottom bumper probes relative to visual center
    const Vec2 visualProbes[3] = {
        Vec2(-34.0f, 15.0f),
        Vec2(0.0f, 16.0f),
        Vec2(34.0f, 15.0f)
    };
    float probeRadius = 4.0f;
    Vec2 visualPos = m_chassis.position - m_config.centerOfMassOffset.rotated(m_chassis.angle);

    for (const auto& probeLocal : visualProbes) {
        Vec2 probeWorld = visualPos + probeLocal.rotated(m_chassis.angle);

        for (const auto& seg : m_terrainSegments) {
            Vec2 closest, normal;
            float penetration = 0.0f;
            if (testCircleSegment(probeWorld, probeRadius, seg.p1, seg.p2, closest, normal, penetration)) {
                m_chassis.position += normal * penetration;

                float vn = m_chassis.velocity.dot(normal);
                if (vn < 0.0f) {
                    m_chassis.velocity -= normal * vn;
                }

                Vec2 tangent(-normal.y, normal.x);
                float vt = m_chassis.velocity.dot(tangent);
                m_chassis.velocity -= tangent * (vt * 1.5f * subDt);

                Vec2 rChassis = probeWorld - m_chassis.position;
                Vec2 impactForce = normal * (penetration * 200.0f);
                float torque = rChassis.cross(impactForce);
                m_chassis.angularVelocity += (torque / m_config.chassisInertia) * subDt;
            }
        }
    }
}

bool PhysicsWorld::checkHeadTerrainCollision() {
    Vec2 visualPos = m_chassis.position - m_config.centerOfMassOffset.rotated(m_chassis.angle);
    Vec2 headWorld = visualPos + visualHeadMount().rotated(m_chassis.angle);

    for (const auto& seg : m_terrainSegments) {
        Vec2 closest, normal;
        float penetration = 0.0f;
        if (testCircleSegment(headWorld, m_config.headRadius, seg.p1, seg.p2, closest, normal, penetration)) {
            return true; // Neck snap collision detected!
        }
    }
    return false;
}

} // namespace Physics
