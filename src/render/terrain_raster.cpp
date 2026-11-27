#include "terrain_raster.h"

#include <QColor>
#include <QPainter>
#include <QVarLengthArray>

#include <cmath>
#include <limits>

namespace Render {

using namespace Terrain;

namespace {

struct ThemePalette {
    QRgb outline;
    QRgb highlight;
    QRgb surface;
    QRgb surfaceSpeck;
    QRgb surfaceShade;
    QRgb surfaceEdge;
    QRgb soil;
    QRgb soilDark;
    QRgb soilLight;
    int decorType; // 0 = grass/pebbles, 1 = beach shells, 2 = swamp roots, 3 = snow crystals, 4 = moon crater
};

ThemePalette paletteForMap(int map)
{
    switch (map) {
    case 1: // Beachday
        return {
            qRgb(140, 110, 50),
            qRgb(255, 245, 180),
            qRgb(235, 195, 115),
            qRgb(245, 215, 140),
            qRgb(215, 175, 95),
            qRgb(185, 145, 80),
            qRgb(195, 150, 95),
            qRgb(165, 120, 75),
            qRgb(215, 170, 115),
            1
        };
    case 2: // Swampy Vibe
        return {
            qRgb(25, 40, 20),
            qRgb(135, 170, 60),
            qRgb(85, 115, 45),
            qRgb(105, 135, 55),
            qRgb(65, 90, 35),
            qRgb(45, 60, 25),
            qRgb(60, 48, 36),
            qRgb(42, 32, 24),
            qRgb(78, 62, 46),
            2
        };
    case 3: // Arctic Elegance / Snow
        return {
            qRgb(110, 150, 185),
            qRgb(255, 255, 255),
            qRgb(230, 242, 255),
            qRgb(245, 250, 255),
            qRgb(200, 222, 245),
            qRgb(150, 185, 220),
            qRgb(95, 120, 150),
            qRgb(70, 90, 120),
            qRgb(120, 145, 175),
            3
        };
    case 4: // The Real Excursion / Moon
        return {
            qRgb(55, 60, 70),
            qRgb(225, 230, 240),
            qRgb(175, 180, 190),
            qRgb(200, 205, 215),
            qRgb(140, 145, 155),
            qRgb(110, 115, 125),
            qRgb(80, 85, 95),
            qRgb(55, 60, 70),
            qRgb(105, 110, 120),
            4
        };
    case 0: // Grazing the Grass
    default:
        return {
            qRgb(31, 58, 20),
            qRgb(178, 226, 60),
            qRgb(122, 194, 33),
            qRgb(96, 165, 27),
            qRgb(84, 150, 26),
            qRgb(46, 98, 22),
            qRgb(156, 90, 38),
            qRgb(132, 74, 30),
            qRgb(172, 104, 48),
            0
        };
    }
}

} // namespace

TerrainRaster rasterizeTerrain(const Terrain::TerrainManager &terrain, double cameraX, double cameraY,
                               int designWidth, int designHeight, int map)
{
    constexpr int Px = TerrainPixel;
    const double snappedX = std::floor(cameraX / Px) * Px;
    const double snappedY = std::floor(cameraY / Px) * Px;
    const int columns = designWidth / Px + 2;
    const int rows = designHeight / Px + 3;
    const qint64 firstColumn = static_cast<qint64>(snappedX / Px);

    const ThemePalette pal = paletteForMap(map);
    const QRgb outline = pal.outline;
    const QRgb highlight = pal.highlight;
    const QRgb grass = pal.surface;
    const QRgb grassSpeck = pal.surfaceSpeck;
    const QRgb grassShade = pal.surfaceShade;
    const QRgb grassEdge = pal.surfaceEdge;
    const QRgb soil = pal.soil;
    const QRgb soilDark = pal.soilDark;
    const QRgb soilLight = pal.soilLight;

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

        if (pal.decorType == 1) { // Beach: seashells and pebbles
            if (pebble) {
                art.fillRect(c, y, 3, 2, QColor(210, 205, 195));
                art.fillRect(c + 1, y - 1, 1, 1, QColor(240, 235, 225));
            } else {
                art.setPen(QColor(180, 120, 90));
                art.setBrush(QColor(255, 230, 220));
                art.drawChord(QRect(c - 2, y - 2, 6, 6), 30 * 16, 120 * 16);
            }
        } else if (pal.decorType == 2) { // Swamp: bog roots and dark logs
            if (pebble) {
                art.fillRect(c, y, 4, 2, QColor(30, 22, 16));
                art.fillRect(c + 1, y + 1, 2, 2, QColor(45, 34, 25));
            } else {
                art.setPen(QPen(QColor(38, 28, 20), 2));
                art.drawLine(c - 3, y - 2, c + 3, y + 2);
                art.drawLine(c, y, c + 4, y - 3);
            }
        } else if (pal.decorType == 3) { // Snow: ice crystals and glacial stones
            if (pebble) {
                art.fillRect(c, y, 3, 2, QColor(70, 95, 125));
                art.fillRect(c + 1, y - 1, 1, 1, QColor(190, 230, 255));
            } else {
                const QPoint diamond[] = {{c, y - 3}, {c + 3, y}, {c, y + 3}, {c - 3, y}};
                art.setPen(QColor(140, 195, 235));
                art.setBrush(QColor(220, 245, 255));
                art.drawPolygon(diamond, 4);
            }
        } else if (pal.decorType == 4) { // Moon: impact craters and basalt chunks
            if (pebble) {
                art.fillRect(c, y, 3, 2, QColor(40, 42, 48));
                art.fillRect(c + 1, y - 1, 1, 1, QColor(160, 165, 175));
            } else {
                art.setPen(QColor(140, 145, 155));
                art.setBrush(QColor(45, 48, 55));
                art.drawEllipse(c, y, 5, 4);
                art.setPen(Qt::NoPen);
                art.setBrush(QColor(210, 215, 225));
                art.drawRect(c + 1, y - 1, 2, 1);
            }
        } else { // Grassland
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
    }
    art.end();

    return {image, QRectF(snappedX - cameraX, (cameraY - snappedY) - Px, columns * Px, rows * Px)};
}

} // namespace Render
