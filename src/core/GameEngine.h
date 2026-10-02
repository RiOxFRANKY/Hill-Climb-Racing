#pragma once

#include "physics/PhysicsWorld.h"
#include "terrain/TerrainManager.h"
#include "terrain/SineTerrainGenerator.h"
#include "vehicle/Car.h"
#include "core/GameTypes.h"
#include "core/InputHandler.h"
#include <memory>
#include <vector>

namespace Core {

class GameEngine {
public:
    // ========================================================================
    // TWEAKABLE PARAMETERS (Gameplay, pacing, collectibles & camera)
    // ========================================================================
    struct Config {
        float fixedTimeStep{1.0f / 60.0f};  // 60 Hz simulation rate (~0.01667s)
        float targetDistanceMeters{1000.0f};// Finish line distance in meters

        // Collectibles generation
        float coinSpacing{40.0f};          // Horizontal distance between coins in clusters
        int   coinsPerCluster{5};          // Number of coins in a cluster
        float clusterInterval{140.0f};     // Distance between coin clusters
        float fuelCanInterval{600.0f};     // Distance between fuel cans (in px, ~250m)
        float collectibleFloatHeight{28.0f};// Height above terrain surface

        // Camera dynamics
        float cameraLeadX{180.0f};         // How far ahead of car center the camera looks
        float cameraLeadY{-40.0f};         // Vertical camera offset (slightly above car)
        float cameraSmoothFactor{0.085f};  // Exponential smoothing speed (0 to 1)

        // Game Over thresholds
        float outOfFuelStopThreshold{0.8f}; // Speed below which car is declared stopped (km/h)
        float outOfFuelGracePeriod{2.5f};   // Seconds after fuel runs out before declaring Game Over
    };

    GameEngine();
    explicit GameEngine(const Config& config);

    void reset();
    void tick(float dt);

    // Collectibles & terrain initialization
    void populateCollectibles();

    // Input access
    InputHandler& input() { return m_input; }
    const InputHandler& input() const { return m_input; }

    // World & entity access
    const Vehicle::Car& car() const { return m_car; }
    const Terrain::TerrainManager& terrain() const { return m_terrain; }
    const std::vector<Collectible>& collectibles() const { return m_collectibles; }

    // Camera state
    Physics::Vec2 cameraPosition() const { return m_cameraPos; }

    // Game state & metrics
    GameState state() const { return m_state; }
    int score() const { return m_score; }
    int coinsCollected() const { return m_coinsCollected; }
    float targetDistanceMeters() const { return m_config.targetDistanceMeters; }
    float finishLineX() const { return m_config.targetDistanceMeters * m_car.config().pixelsPerMeter; }

    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

private:
    void updatePlaying(float dt);
    void checkLossWinConditions(float dt);
    void updateCamera(float dt);

    Config m_config;

    std::unique_ptr<Physics::IPhysicsWorld> m_physics;
    Terrain::TerrainManager m_terrain;
    Terrain::SineTerrainGenerator m_generator;
    Vehicle::Car m_car;
    InputHandler m_input;

    std::vector<Collectible> m_collectibles;

    GameState m_state{GameState::Playing};
    Physics::Vec2 m_cameraPos{100.0f, 250.0f};

    int   m_score{0};
    int   m_coinsCollected{0};
    float m_outOfFuelTimer{0.0f};
    float m_spawnX{100.0f};
    float m_spawnY{250.0f};
};

} // namespace Core
