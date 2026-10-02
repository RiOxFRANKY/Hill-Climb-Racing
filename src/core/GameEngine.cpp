#include "core/GameEngine.h"
#include <algorithm>
#include <cmath>

namespace Core {

GameEngine::GameEngine()
    : m_config()
    , m_physics(std::make_unique<Physics::PhysicsWorld>())
    , m_terrain()
    , m_generator()
    , m_car()
    , m_input() {
    reset();
}

GameEngine::GameEngine(const Config& config)
    : m_config(config)
    , m_physics(std::make_unique<Physics::PhysicsWorld>())
    , m_terrain()
    , m_generator()
    , m_car()
    , m_input() {
    reset();
}

void GameEngine::reset() {
    m_state = GameState::Playing;
    m_score = 0;
    m_coinsCollected = 0;
    m_outOfFuelTimer = 0.0f;
    m_input.reset();

    // 1. Generate procedural hills
    m_terrain.generate(m_generator);

    // 2. Compute safe spawn coordinates on flat start with clean wheel clearance
    m_spawnX = 120.0f;
    m_spawnY = m_terrain.getHeightAt(m_spawnX) - 50.0f;

    // 3. Reset physics and car
    Physics::Vec2 spawnVec(m_spawnX, m_spawnY);
    m_physics->reset(spawnVec);
    m_car.reset(spawnVec);

    // 4. Populate collectibles across entire track length
    populateCollectibles();

    // 5. Initial terrain collision feed
    m_terrain.feedCollisionSegments(*m_physics, m_spawnX);

    // 6. Center camera
    m_cameraPos = spawnVec + Physics::Vec2(m_config.cameraLeadX, m_config.cameraLeadY);
}

void GameEngine::populateCollectibles() {
    m_collectibles.clear();

    float finishX = finishLineX();
    float currentX = 380.0f;
    float nextFuelCanX = 750.0f;

    while (currentX < finishX - 100.0f) {
        // Spawn Fuel Can at regular intervals
        if (currentX >= nextFuelCanX) {
            Collectible fuel;
            fuel.type = CollectibleType::FuelCan;
            fuel.position.x = currentX;
            fuel.position.y = m_terrain.getHeightAt(currentX) - 22.0f;
            fuel.value = 100;
            fuel.collected = false;
            fuel.bobTimer = static_cast<float>(m_collectibles.size());
            fuel.pickupRadius = 32.0f;
            m_collectibles.push_back(fuel);

            nextFuelCanX += m_config.fuelCanInterval;
            currentX += 80.0f;
            continue;
        }

        // Spawn Coin Cluster following the terrain crest
        for (int i = 0; i < m_config.coinsPerCluster; ++i) {
            float coinX = currentX + static_cast<float>(i) * m_config.coinSpacing;
            if (coinX >= finishX) break;

            Collectible coin;
            coin.type = CollectibleType::Coin;
            coin.position.x = coinX;
            // Place coin along terrain curve with upward arc
            float groundY = m_terrain.getHeightAt(coinX);
            float arcOffset = std::sin(static_cast<float>(i) / (m_config.coinsPerCluster - 1) * 3.14159f) * 16.0f;
            coin.position.y = groundY - (m_config.collectibleFloatHeight + arcOffset);
            coin.value = 5;
            coin.collected = false;
            coin.bobTimer = static_cast<float>(i) * 0.4f;
            coin.pickupRadius = 26.0f;

            m_collectibles.push_back(coin);
        }

        currentX += m_config.clusterInterval + (m_config.coinsPerCluster * m_config.coinSpacing);
    }
}

void GameEngine::tick(float dt) {
    // Check restart trigger from keyboard
    if (m_input.consumeRestartTrigger()) {
        reset();
        return;
    }

    // Check pause trigger
    if (m_input.consumePauseTrigger()) {
        if (m_state == GameState::Playing) {
            m_state = GameState::Paused;
        } else if (m_state == GameState::Paused) {
            m_state = GameState::Playing;
        }
    }

    if (m_state == GameState::Paused) {
        return;
    }

    if (m_state == GameState::Playing) {
        updatePlaying(dt);
    } else {
        // When game over or won, vehicle continues rolling to rest
        m_physics->setThrottle(0.0f);
        m_terrain.feedCollisionSegments(*m_physics, m_car.getRenderData().chassis.position.x);
        m_physics->step(dt);
        m_car.updateFromPhysics(m_physics->getVehicleState(), dt);
        updateCamera(dt);
    }
}

void GameEngine::updatePlaying(float dt) {
    // 1. Process player input intent
    float throttle = m_input.getThrottle();

    // 2. Consume fuel (disables throttle when tank runs completely dry)
    bool isThrottling = (m_input.isGasActive() && m_car.hasFuel());
    m_car.consumeFuel(dt, isThrottling);

    if (!m_car.hasFuel()) {
        throttle = 0.0f; // Engine stalls without fuel
    }

    // 3. Feed terrain collision segments around the car to physics
    float carX = m_car.getRenderData().chassis.position.x;
    m_terrain.feedCollisionSegments(*m_physics, carX);

    // 4. Step physics simulation
    m_physics->setThrottle(throttle);
    m_physics->step(dt);

    // 5. Synchronize physics state to car entity
    m_car.updateFromPhysics(m_physics->getVehicleState(), dt);

    // 6. Update collectibles and collision check with vehicle
    Physics::Vec2 carPos = m_car.getRenderData().chassis.position;
    for (auto& item : m_collectibles) {
        if (item.collected) continue;

        item.bobTimer += dt * 3.5f;

        // Bounding distance check
        float dx = item.position.x - carPos.x;
        float dy = item.position.y - carPos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= item.pickupRadius * item.pickupRadius) {
            item.collected = true;
            if (item.type == CollectibleType::Coin) {
                m_score += item.value;
                m_coinsCollected++;
            } else if (item.type == CollectibleType::FuelCan) {
                m_car.refillFuel(100.0f);
                m_score += 50;
                m_outOfFuelTimer = 0.0f; // Reset stall timer immediately
            }
        }
    }

    // 7. Check win / loss conditions
    checkLossWinConditions(dt);

    // 8. Update smooth camera
    updateCamera(dt);
}

void GameEngine::checkLossWinConditions(float dt) {
    // Loss 1: Driver head touched the terrain (Neck snap)
    if (m_car.isHeadCrashed()) {
        m_state = GameState::GameOver_HeadCrash;
        return;
    }

    // Loss 2: Fuel reaches 0 and vehicle velocity drops below threshold
    if (!m_car.hasFuel()) {
        if (m_car.speedKmH() < m_config.outOfFuelStopThreshold) {
            m_outOfFuelTimer += dt;
            if (m_outOfFuelTimer >= m_config.outOfFuelGracePeriod) {
                m_state = GameState::GameOver_OutOfFuel;
                return;
            }
        } else {
            m_outOfFuelTimer = 0.0f;
        }
    }

    // Win: Reached finish line distance
    if (m_car.distanceMeters() >= m_config.targetDistanceMeters) {
        m_state = GameState::LevelWon;
    }
}

void GameEngine::updateCamera(float dt) {
    Physics::Vec2 carPos = m_car.getRenderData().chassis.position;

    // Lead ahead of the car based on forward velocity
    float leadX = m_config.cameraLeadX;
    float leadY = m_config.cameraLeadY;

    Physics::Vec2 targetCamPos = carPos + Physics::Vec2(leadX, leadY);

    // Exponential moving average smoothing
    float alpha = std::min(1.0f, m_config.cameraSmoothFactor * (dt * 60.0f));
    m_cameraPos.x += (targetCamPos.x - m_cameraPos.x) * alpha;
    m_cameraPos.y += (targetCamPos.y - m_cameraPos.y) * alpha;
}

} // namespace Core
