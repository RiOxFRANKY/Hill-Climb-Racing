#include "terrain_raster.h"

#include <QColor>
#include <QPainter>
#include <QVarLengthArray>

#include <cmath>
#include <limits>

namespace Render {

using namespace Terrain;

TerrainRaster rasterizeTerrain(const Terrain::TerrainManager &terrain, double cameraX, double cameraY,
                               int designWidth, int designHeight)
{
    constexpr int Px = TerrainPixel;
    const double snappedX = std::floor(cameraX / Px) * Px;
    const double snappedY = std::floor(cameraY / Px) * Px;
    const int columns = designWidth / Px + 2;
    const int rows = designHeight / Px + 3;
    const qint64 firstColumn = static_cast<qint64>(snappedX / Px);

    const QRgb outline = qRgb(31, 58, 20);
    const QRgb highlight = qRgb(178, 226, 60);
    const QRgb grass = qRgb(122, 194, 33);
    const QRgb grassSpeck = qRgb(96, 165, 27);
    const QRgb grassShade = qRgb(84, 150, 26);
    const QRgb grassEdge = qRgb(46, 98, 22);
    const QRgb soil = qRgb(156, 90, 38);
    const QRgb soilDark = qRgb(132, 74, 30);
    const QRgb soilLight = qRgb(172, 104, 48);

    constexpr int NoGround = std::numeric_limits<int>::max();
    QVarLengthArray<int, 520> tops(columns + 2);
    QVarLengthArray<double, 520> slopes(columns + 2);
    for (int c = -1; c <= columns; ++c) {
        const double x = (firstColumn + c + 0.5) * Px;
        if (terrain.gapAt(x)) {
            tops[c + 1] = NoGround;
            slopes[c + 1] = 0.0;
            continue;
        }
        const double screenY = designHeight - (terrain.surfaceHeight(x) - snappedY);
        tops[c + 1] = static_cast<int>(std::floor(screenY / Px)) + 1;
        slopes[c + 1] = terrain.surfaceSlope(x);
    }

    QImage image(columns, rows, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    for (int c = 0; c < columns; ++c) {
        const int top = tops[c + 1];
        if (top == NoGround || top >= rows)
            continue;
        const qint64 worldColumn = firstColumn + c;
        const int left = tops[c];
        const int right = tops[c + 2];
        const bool cliff = left == NoGround || right == NoGround;

        int outlineBottom = top;
        if (left != NoGround)
            outlineBottom = std::max(outlineBottom, left - 1);
        if (right != NoGround)
            outlineBottom = std::max(outlineBottom, right - 1);
        const double slope = slopes[c + 1];
        const int grassDepth = static_cast<int>(std::lround(7.0 * std::sqrt(1.0 + slope * slope)))
                               + (worldColumn % 2 == 0 ? 0 : 2)
                               + static_cast<int>(hashValue(worldColumn * 31) % 2);

        for (int r = std::max(top, 0); r < rows; ++r) {
            const int depth = r - top;
            QRgb colour;
            if (cliff || r <= outlineBottom) {
                colour = outline;
            } else if (r == outlineBottom + 1) {
                colour = highlight;
            } else if (depth <= grassDepth) {
                const bool lower = depth > (outlineBottom - top) + grassDepth / 2;
                const quint32 n = hashValue(worldColumn * 7919 + depth * 104729) % 7;
                colour = lower ? (n < 2 ? grass : grassShade) : (n == 0 ? grassSpeck : grass);
            } else if (depth <= grassDepth + 1) {
                colour = grassEdge;
            } else {
                const quint32 h = hashValue(worldColumn * 15485863 + depth * 2038074743LL) % 23;
                colour = h == 0 ? soilDark : (h == 1 ? soilLight : soil);
            }
            reinterpret_cast<QRgb *>(image.scanLine(r))[c] = colour;
        }
    }

    QPainter art(&image);
    art.setRenderHint(QPainter::Antialiasing, false);
    for (int c = 2; c < columns - 2; ++c) {
        const qint64 worldColumn = firstColumn + c;
        const quint32 h = hashValue(worldColumn * 2654435761LL);
        const bool cluster = h % 41 == 0;
        const bool pebble = !cluster && h % 13 == 0;
        if (!cluster && !pebble)
            continue;

        int shallowest = std::numeric_limits<int>::min();
        bool solid = true;
        for (int k = -2; k <= 2; ++k) {
            const int t = tops[c + 1 + k];
            solid = solid && t != NoGround;
            shallowest = std::max(shallowest, t);
        }
        if (!solid)
            continue;
        const int depth = 12 + static_cast<int>((h >> 8) % 26);
        const int y = tops[c + 1] + depth;
        if (y - 4 < shallowest + 12 || y + 4 >= rows)
            continue;

        if (pebble) {
            art.fillRect(c, y, 3, 2, QColor(66, 62, 70));
            art.fillRect(c + 1, y - 1, 1, 1, QColor(66, 62, 70));
            art.fillRect(c, y - 1 + 1, 1, 1, QColor(128, 122, 130));
        } else {
            const QPoint centres[] = {{c, y}, {c + 5, y + 1}, {c + 2, y - 3}};
            art.setPen(QColor(110, 60, 24));
            art.setBrush(QColor(222, 158, 96));
            for (const QPoint &centre : centres)
                art.drawEllipse(centre, 3, 3);
            art.setPen(Qt::NoPen);
            art.setBrush(QColor(246, 204, 150));
            for (const QPoint &centre : centres)
                art.drawRect(centre.x() - 1, centre.y() - 2, 1, 1);
        }
    }
    art.end();

    return {image, QRectF(snappedX - cameraX, (cameraY - snappedY) - Px, columns * Px, rows * Px)};
}

} // namespace Render
