#pragma once

#include "../physics/physics.h"
#include <array>

namespace Terrain {
class TerrainManager;
}

namespace Vehicle {

class VehicleEntity {
public:
    VehicleEntity() = default;

    void reset(double spawnX, double surfaceHeightAtSpawn);
    void stepPhysics(double h, double throttle, bool boost, const Terrain::TerrainManager &terrain);

    [[nodiscard]] const Physics::RigidBody &chassis() const { return m_chassis; }
    [[nodiscard]] Physics::RigidBody &chassis() { return m_chassis; }
    [[nodiscard]] const std::array<Physics::RigidBody, 2> &wheels() const { return m_wheels; }
    [[nodiscard]] std::array<Physics::RigidBody, 2> &wheels() { return m_wheels; }

    [[nodiscard]] int wheelContacts() const { return m_wheelContacts; }
    [[nodiscard]] bool headHit() const { return m_headHit; }
    [[nodiscard]] bool bodyContact() const { return m_bodyContact; }
    [[nodiscard]] double flipTimer() const { return m_flipTimer; }
    [[nodiscard]] double fuel() const { return m_fuel; }

    void setHeadHit(bool hit) { m_headHit = hit; }
    void setBodyContact(bool contact) { m_bodyContact = contact; }
    void setFlipTimer(double timer) { m_flipTimer = timer; }
    void setFuel(double fuel) { m_fuel = fuel; }
    void consumeFuel(double amount) { m_fuel = std::max(0.0, m_fuel - amount); }

private:
    Physics::RigidBody m_chassis;
    std::array<Physics::RigidBody, 2> m_wheels;
    int m_wheelContacts = 0;
    bool m_headHit = false;
    bool m_bodyContact = false;
    double m_flipTimer = 0.0;
    double m_fuel = 100.0;
};

} // namespace Vehicle
