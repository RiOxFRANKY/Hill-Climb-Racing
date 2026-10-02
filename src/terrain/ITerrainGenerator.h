#pragma once

#include "terrain/TerrainTypes.h"
#include <vector>

namespace Terrain {

// ============================================================================
// ITerrainGenerator Interface
// Pure mathematical procedural terrain generator interface.
// Fills uniform height-step nodes for O(1) random access.
// ============================================================================

class ITerrainGenerator {
public:
    virtual ~ITerrainGenerator() = default;

    // Evaluates pure height at any horizontal coordinate X
    virtual float sampleHeight(float x) const = 0;

    // Generates an array of nodes over a specified range
    virtual void generateNodes(std::vector<TerrainNode>& outNodes, float startX, float endX, float dx) const = 0;
};

} // namespace Terrain
