#pragma once

#include "../terrain/terrain.h"

#include <QImage>
#include <QRectF>

namespace Render {

// The visible ground as a low-resolution pixel-art image (one image pixel per
// TerrainPixel world pixels), plus where to draw it in design coordinates.
struct TerrainRaster {
    QImage image;
    QRectF placement;
};

[[nodiscard]] TerrainRaster rasterizeTerrain(const Terrain::TerrainManager &terrain, double cameraX, double cameraY,
                                             int designWidth, int designHeight);

} // namespace Render
