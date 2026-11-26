#pragma once

#include "../terrain/terrain.h"

#include <QImage>
#include <QRectF>

namespace Render {

// The look of the ground. Each map uses one.
enum class GroundStyle {
    Grass,  // Grazing the Grass: grass over brown soil
    Sand,   // Beachday: sand over layered sandstone
    Swamp,  // Swampy Vibe: moss and wet mud, with reeds
    Snow,   // Arctic Elegance: a snow cap over frozen rock
    Moon,   // The Real Excursion: grey dust over cratered rock
};

// The style for a map index (0 to 4).
[[nodiscard]] GroundStyle groundStyleForMap(int map);

// The visible ground as a low-resolution pixel-art image (one image pixel per
// TerrainPixel world pixels), plus where to draw it in design coordinates.
struct TerrainRaster {
    QImage image;
    QRectF placement;
};

[[nodiscard]] TerrainRaster rasterizeTerrain(const Terrain::TerrainManager &terrain, double cameraX, double cameraY,
                                             int designWidth, int designHeight,
                                             GroundStyle style = GroundStyle::Grass);

} // namespace Render
