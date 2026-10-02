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

        // Chassis properties
        float chassisMass{90.0f};        // Scaled chassis mass (kg)
        float chassisInertia{65000.0f};  // High moment of inertia to resist toppling
        float chassisHalfWidth{44.0f};   // Half width (px)
        float chassisHalfHeight{20.0f};  // Half height (px)
        float airTorque{18000.0f};       // Controllable pitch torque when airborne
        float angularDamping{3.2f};      // Angular drag eliminating wild tumbling
        float groundStabilizer{18.0f};   // Restoring alignment to wheel plane when grounded

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
        Vec2  position{0.0f, 0.0f};
        Vec2  velocity{0.0f, 0.0f};
        float angle{0.0f};           // Radians
        float angularVelocity{0.0f}; // rad/s
    };

    void subStep(float subDt);
    void solveSuspension(WheelBody& wheel, const Vec2& mountLocal, float subDt);
    void solveWheelTerrainCollision(WheelBody& wheel, float subDt);
    void solveChassisTerrainCollision(float subDt);
    bool checkHeadTerrainCollision();

    Config m_config;
    ChassisBody m_chassis;
    WheelBody   m_rearWheel;
    WheelBody   m_frontWheel;

    Vec2  m_rearMountLocal{-30.0f, 8.0f};
    Vec2  m_frontMountLocal{30.0f, 8.0f};
    Vec2  m_headMountLocal{-4.0f, -14.0f};

    float m_throttle{0.0f};
    bool  m_headCollided{false};

    std::vector<TerrainSegment> m_terrainSegments;
};

} // namespace Physics
