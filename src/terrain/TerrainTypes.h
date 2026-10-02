// ============================================================================
// ASSET SPECIFICATION & DIMENSION STANDARDS
// ============================================================================
// File Path: assets/terrain/grass_top.png
// Native Resolution: 32 x 16 px | Aspect Ratio: 2:1 (Horizontal Repeating)
// Purpose: Seamless top-edge lush grass surface trim.
//
// File Path: assets/terrain/dirt_fill.png
// Native Resolution: 32 x 32 px | Aspect Ratio: 1:1 (Tiling Texture)
// Purpose: Seamless repeating subterranean earthy dirt fill.
//
// WARNING: Any replacement PNG for these terrain textures MUST strictly match
// the exact pixel dimensions and aspect ratios specified above to prevent visual
// seams, warping, or distortion.
// ============================================================================

#pragma once

#include "physics/PhysicsTypes.h"
#include <vector>

namespace Terrain {

enum class SurfaceType : int {
    Grass = 0,
    Dirt  = 1,
    Rock  = 2
};

struct TerrainNode {
    float height{350.0f}; // Height in canvas Y coordinates (Y points down, so higher Y is lower ground)
    float friction{0.85f};
    SurfaceType surfaceType{SurfaceType::Grass};
};

struct TerrainSlice {
    std::vector<Physics::Vec2> surfacePoints;
    float startX{0.0f};
    float endX{0.0f};
    float bottomY{540.0f};
};

struct GrassTileSegment {
    Physics::Vec2 p1;
    Physics::Vec2 p2;
    float angleDeg{0.0f};
};

} // namespace Terrain
