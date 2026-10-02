#pragma once

#include "physics/PhysicsTypes.h"
#include <vector>

namespace Physics {

// ============================================================================
// IPhysicsWorld Interface
// Pure abstract interface decoupling game logic and rendering from physics solver.
// Exposes exclusively pure C++ types.
// ============================================================================

class IPhysicsWorld {
public:
    virtual ~IPhysicsWorld() = default;

    // Simulation stepping
    virtual void step(float dt) = 0;

    // Driving inputs: throttle in range [-1.0 (full brake/reverse), +1.0 (full gas)]
    virtual void setThrottle(float amount) = 0;

    // Terrain management for local collision feeding
    virtual void clearTerrain() = 0;
    virtual void addTerrainSegment(Vec2 p1, Vec2 p2, float friction = 0.85f) = 0;

    // Vehicle state inspection
    virtual VehiclePhysicsState getVehicleState() const = 0;

    // Reset vehicle to spawn position
    virtual void reset(Vec2 spawnPos) = 0;
};

} // namespace Physics
