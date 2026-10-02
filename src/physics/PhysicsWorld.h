#pragma once

#include "physics/IPhysicsWorld.h"
#include <vector>

namespace Physics {

// ============================================================================
// PhysicsWorld
// Self-contained 2D Spring-Damper Vehicle Simulation & Collision Solver.
// ============================================================================

class PhysicsWorld : public IPhysicsWorld {
public:
    // ========================================================================
    // TWEAKABLE PARAMETERS (Easily configurable physical constants)
    // ========================================================================
    struct Config {
        // Environment
        float gravity{980.0f};           // Downward gravity in px/s^2
        int   subSteps{16};              // Simulation sub-steps per frame for stability

        // Chassis mass distribution & Center of Mass (CoM)
        // Front-heavy body mass (60% front, 40% rear) and elevated CoM for natural airborne nose-dive
        float chassisMass{90.0f};        // Total chassis mass (kg)
        float frontChassisMass{54.0f};   // Front body mass (kg) - heavier front
        float rearChassisMass{36.0f};    // Rear body mass (kg)
        Vec2  centerOfMassOffset{6.0f, -2.0f}; // CoM offset relative to visual center: +X (front), -Y (elevated above bottom)
        float chassisInertia{62000.0f};  // Moment of inertia for realistic rotation
        float chassisHalfWidth{44.0f};   // Half width (px)
        float chassisHalfHeight{20.0f};  // Half height (px)

        // Airborne & pitch dynamics
        float airNoseDiveTorque{42000.0f}; // Natural gradual forward nose-dive torque when airborne
        float airTorque{28000.0f};         // Player aerial pitch control (Gas = pitch up, Brake = pitch down)
        float angularDamping{2.2f};        // Angular damping (keeps motion smooth without freezing pitch)
        float groundStabilizer{18.0f};     // Restoring alignment to wheel plane when BOTH wheels grounded

        // Suspension properties (Taut, stiff, strictly bounded travel)
        float springK{18000.0f};         // Stiff spring preventing excessive stretching
        float dampingC{1400.0f};         // High damping absorbing oscillation
        float restLength{14.0f};         // Compact, realistic neutral suspension length (px)
        float minLength{8.0f};           // Full compression bump-stop limit (px)
        float maxLength{18.0f};          // Strict max extension limit (no over-stretching)

        // Wheel properties
        float wheelMass{12.0f};          // Wheel mass
        float wheelRadius{14.0f};        // Collision circle radius (px)
        float driveTorque{950.0f};       // Smooth, progressive forward acceleration
        float reverseTorque{550.0f};     // Slower, gentle reverse acceleration
        float brakeTorque{1600.0f};      // Responsive brake stopping torque when moving forward
        float maxWheelSpeed{50.0f};      // Max forward wheel angular velocity (~105 km/h)
        float maxReverseSpeed{22.0f};    // Lower max reverse speed (~45 km/h)
        float tireFriction{1.75f};       // High tire-to-ground grip eliminating slipping

        // Driver head
        float headRadius{8.0f};          // Head collision radius for neck-snap detection
    };

    PhysicsWorld();
    explicit PhysicsWorld(const Config& config);
    ~PhysicsWorld() override = default;

    // IPhysicsWorld implementation
    void step(float dt) override;
    void setThrottle(float amount) override; // [-1.0f .. +1.0f]
    void clearTerrain() override;
    void addTerrainSegment(Vec2 p1, Vec2 p2, float friction = 0.85f) override;
    VehiclePhysicsState getVehicleState() const override;
    void reset(Vec2 spawnPos) override;

    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

private:
    struct WheelBody {
        Vec2  position{0.0f, 0.0f};
        Vec2  velocity{0.0f, 0.0f};
        float angle{0.0f};           // Current rotation in radians
        float angularVelocity{0.0f}; // rad/s
        bool  isGrounded{false};
        Vec2  groundNormal{0.0f, -1.0f};
        float groundFriction{0.85f};
    };

    struct ChassisBody {
        Vec2  position{0.0f, 0.0f}; // Center of Mass (CoM) position in world space
        Vec2  velocity{0.0f, 0.0f};
        float angle{0.0f};           // Radians
        float angularVelocity{0.0f}; // rad/s
    };

    // Mount points relative to visual chassis center
    Vec2 visualRearMount() const { return Vec2(-30.0f, 8.0f); }
    Vec2 visualFrontMount() const { return Vec2(30.0f, 8.0f); }
    Vec2 visualHeadMount() const { return Vec2(-4.0f, -14.0f); }

    // Mount points relative to Center of Mass (CoM)
    Vec2 rearMountCoM() const { return visualRearMount() - m_config.centerOfMassOffset; }
    Vec2 frontMountCoM() const { return visualFrontMount() - m_config.centerOfMassOffset; }
    Vec2 headMountCoM() const { return visualHeadMount() - m_config.centerOfMassOffset; }

    void subStep(float subDt);
    void solveSuspension(WheelBody& wheel, const Vec2& mountLocal, float subDt);
    void solveWheelTerrainCollision(WheelBody& wheel, float subDt);
    void solveChassisTerrainCollision(float subDt);
    bool checkHeadTerrainCollision();

    Config m_config;
    ChassisBody m_chassis;
    WheelBody   m_rearWheel;
    WheelBody   m_frontWheel;

    float m_throttle{0.0f};
    bool  m_headCollided{false};

    std::vector<TerrainSegment> m_terrainSegments;
};

} // namespace Physics
