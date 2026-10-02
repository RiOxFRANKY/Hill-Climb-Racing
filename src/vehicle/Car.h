// ============================================================================
// ASSET SPECIFICATION & DIMENSION STANDARDS
// ============================================================================
// File Path: assets/vehicle/chassis.png
// Native Resolution: 96 x 48 px | Aspect Ratio: 2:1
// Description: Classic retro off-roader chassis body (red jeep/buggy styling).
//
// File Path: assets/vehicle/wheel.png
// Native Resolution: 32 x 32 px | Aspect Ratio: 1:1
// Description: Deep-tread off-road tire with silver alloy rim and bolts.
//
// File Path: assets/vehicle/driver_head.png
// Native Resolution: 24 x 24 px | Aspect Ratio: 1:1
// Description: Driver head with retro racing helmet and tinted goggles.
//
// WARNING: Any replacement PNG for the vehicle sprites MUST strictly match
// the exact pixel dimensions and aspect ratios specified above. The physics
// collision bounds, wheel mounts, and driver head positions are mathematically
// aligned to these native resolutions.
// ============================================================================

#pragma once

#include "physics/PhysicsTypes.h"

namespace Vehicle {

struct CarRenderData {
    Physics::Transform2D chassis;
    Physics::Transform2D rearWheel;
    Physics::Transform2D frontWheel;
    Physics::Transform2D driverHead;
    Physics::Vec2 velocity{0.0f, 0.0f};
    float speedKmH{0.0f};
    float fuel{100.0f};
    bool  headCollided{false};
    bool  rearWheelGrounded{false};
    bool  frontWheelGrounded{false};
};

class Car {
public:
    // ========================================================================
    // TWEAKABLE PARAMETERS (Gameplay, fuel, and vehicle dimensions)
    // ========================================================================
    struct Config {
        // Sprite Dimensions (Must match native PNG resolutions)
        float chassisWidth{96.0f};
        float chassisHeight{48.0f};
        float wheelDiameter{32.0f};
        float driverHeadSize{24.0f};

        // Fuel parameters
        float maxFuel{100.0f};
        float idleFuelBurnRate{0.45f};  // Fuel burned per second while coasting (%/s)
        float gasFuelBurnRate{2.20f};   // Fuel burned per second under full throttle (%/s)

        // Gameplay metrics
        float pixelsPerMeter{25.0f};    // Distance metric conversion (25 px = 1 meter)
        float speedScaleFactor{0.144f}; // Converts px/s to km/h for HUD speedometer
    };

    Car();
    explicit Car(const Config& config);

    void reset(Physics::Vec2 spawnPos);

    // Fuel management
    void consumeFuel(float dt, bool isThrottling);
    void refillFuel(float amount = 100.0f);
    bool hasFuel() const { return m_fuel > 0.0f; }

    // Synchronize physics transforms and calculate speed / distance
    void updateFromPhysics(const Physics::VehiclePhysicsState& state, float dt);

    // Get render descriptor
    CarRenderData getRenderData() const;

    // Getters
    float fuel() const { return m_fuel; }
    float fuelRatio() const { return m_fuel / m_config.maxFuel; }
    float distanceTraveled() const { return m_distanceTraveled; }
    float distanceMeters() const { return m_distanceTraveled / m_config.pixelsPerMeter; }
    float speedKmH() const { return m_speedKmH; }
    bool  isAirborne() const { return m_isAirborne; }
    float airTime() const { return m_airTime; }
    bool  isHeadCrashed() const { return m_headCollided; }

    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

private:
    Config m_config;

    Physics::VehiclePhysicsState m_physicsState;
    float m_fuel{100.0f};
    float m_distanceTraveled{0.0f};
    float m_startX{0.0f};
    float m_speedKmH{0.0f};
    bool  m_isAirborne{false};
    float m_airTime{0.0f};
    bool  m_headCollided{false};
};

} // namespace Vehicle
