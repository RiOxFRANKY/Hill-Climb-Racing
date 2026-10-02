// ============================================================================
// ASSET SPECIFICATION & DIMENSION STANDARDS
// ============================================================================
// File Path: assets/terrain/grass_top.png
// Native Resolution: 32 x 16 px | Aspect Ratio: 2:1 (Horizontal Repeating)
// Purpose: Repeating top-edge grass trim texture.
//
// File Path: assets/terrain/dirt_fill.png
// Native Resolution: 32 x 32 px | Aspect Ratio: 1:1 (2D Tiling)
// Purpose: Seamless repeating subterranean dirt fill.
//
// WARNING: Any replacement PNG MUST strictly match these dimensions (32x16 and 32x32).
// Failure to maintain dimensions will break texture tiling alignment.
// ============================================================================

#pragma once

#include "terrain/TerrainTypes.h"
#include "terrain/ITerrainGenerator.h"
#include "physics/IPhysicsWorld.h"
#include <memory>
#include <vector>

namespace Terrain {

class TerrainManager {
public:
    // ========================================================================
    // TWEAKABLE PARAMETERS
    // ========================================================================
    struct Config {
        float dx{5.0f};                 // Horizontal spacing between terrain sample nodes (px)
        float trackLength{25000.0f};    // Total course length in virtual pixels (~1000m)
        float collisionRadius{350.0f};  // Local window around vehicle to feed collision segments
        float marginX{250.0f};          // Generous margin outside screen for seamless visual culling
    };

    TerrainManager();
    explicit TerrainManager(const Config& config);

    // Initializes or regenerates the entire track using a generator
    void generate(const ITerrainGenerator& generator);

    // O(1) instantaneous height lookup with linear sub-node interpolation
    float getHeightAt(float x) const;

    // Returns surface normal vector at coordinate X
    Physics::Vec2 getNormalAt(float x) const;

    // Feeds local line segments around the car to the physics engine
    void feedCollisionSegments(Physics::IPhysicsWorld& physics, float carX) const;

    // Extracts visible surface nodes for rendering ground polygon
    TerrainSlice getVisibleSlice(float viewLeft, float viewRight, float bottomY) const;

    float trackLength() const { return m_config.trackLength; }
    float dx() const { return m_config.dx; }

    const std::vector<TerrainNode>& nodes() const { return m_nodes; }
    const std::vector<GrassTileSegment>& grassTiles() const { return m_grassTiles; }
    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

private:
    void buildGrassTiles();

    Config m_config;
    std::vector<TerrainNode> m_nodes;
    std::vector<GrassTileSegment> m_grassTiles;
};

} // namespace Terrain
