#include "vehicle/Car.h"
#include <algorithm>
#include <cmath>

namespace Vehicle {

Car::Car()
    : m_config() {
    reset({100.0f, 250.0f});
}

Car::Car(const Config& config)
    : m_config(config) {
    reset({100.0f, 250.0f});
}

void Car::reset(Physics::Vec2 spawnPos) {
    m_fuel = m_config.maxFuel;
    m_startX = spawnPos.x;
    m_distanceTraveled = 0.0f;
    m_speedKmH = 0.0f;
    m_isAirborne = false;
    m_airTime = 0.0f;
    m_headCollided = false;

    m_physicsState.chassis.position = spawnPos;
    m_physicsState.chassis.angleDegrees = 0.0f;
    m_physicsState.rearWheel.position = spawnPos;
    m_physicsState.frontWheel.position = spawnPos;
    m_physicsState.driverHead.position = spawnPos;
    m_physicsState.velocity = {0.0f, 0.0f};
    m_physicsState.angularVelocity = 0.0f;
    m_physicsState.headCollided = false;
    m_physicsState.rearWheelGrounded = false;
    m_physicsState.frontWheelGrounded = false;
}

void Car::consumeFuel(float dt, bool isThrottling) {
    if (m_fuel <= 0.0f) {
        m_fuel = 0.0f;
        return;
    }

    float burnRate = isThrottling ? m_config.gasFuelBurnRate : m_config.idleFuelBurnRate;
    m_fuel -= burnRate * dt;

    if (m_fuel < 0.0f) {
        m_fuel = 0.0f;
    }
}

void Car::refillFuel(float amount) {
    m_fuel = std::min(m_config.maxFuel, m_fuel + amount);
}

void Car::updateFromPhysics(const Physics::VehiclePhysicsState& state, float dt) {
    m_physicsState = state;

    // Forward speed metric
    float speedPxS = std::sqrt(state.velocity.x * state.velocity.x + state.velocity.y * state.velocity.y);
    m_speedKmH = speedPxS * m_config.speedScaleFactor;

    // Maximum forward progress achieved
    float forwardProgress = state.chassis.position.x - m_startX;
    if (forwardProgress > m_distanceTraveled) {
        m_distanceTraveled = forwardProgress;
    }

    // Airborne calculation
    if (!state.rearWheelGrounded && !state.frontWheelGrounded) {
        m_isAirborne = true;
        m_airTime += dt;
    } else {
        m_isAirborne = false;
        m_airTime = 0.0f;
    }

    if (state.headCollided) {
        m_headCollided = true;
    }
}

CarRenderData Car::getRenderData() const {
    CarRenderData d;
    d.chassis = m_physicsState.chassis;
    d.rearWheel = m_physicsState.rearWheel;
    d.frontWheel = m_physicsState.frontWheel;
    d.driverHead = m_physicsState.driverHead;
    d.velocity = m_physicsState.velocity;
    d.speedKmH = m_speedKmH;
    d.fuel = m_fuel;
    d.headCollided = m_headCollided;
    d.rearWheelGrounded = m_physicsState.rearWheelGrounded;
    d.frontWheelGrounded = m_physicsState.frontWheelGrounded;
    return d;
}

} // namespace Vehicle
