#include "terrain_raster.h"

#include <QColor>
#include <QPainter>
#include <QVarLengthArray>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Render {

using namespace Terrain;

namespace {

// Colours for one ground style. The column is drawn top-down as: outline,
// one highlight row, the top layer (grass, sand, snow...), an edge row, then
// the body (soil, rock...) down to the bottom of the screen.
struct Palette {
    QRgb outline;
    QRgb highlight;
    QRgb top;
    QRgb topSpeck;       // scattered through the upper top layer
    QRgb topShade;       // the lower half of the top layer
    QRgb topEdge;
    QRgb body;
    QRgb bodyDark;
    QRgb bodyLight;
    double topThickness; // in image pixels, before slope and jitter
    int bodySpeckRarity; // one dark and one light speck per this many pixels
};

Palette paletteFor(GroundStyle style)
{
    switch (style) {
    case GroundStyle::Sand:
        return {qRgb(92, 62, 30), qRgb(255, 240, 190), qRgb(240, 208, 140), qRgb(226, 186, 112),
                qRgb(220, 182, 108), qRgb(176, 128, 70), qRgb(206, 158, 92), qRgb(186, 136, 76),
                qRgb(224, 182, 116), 6.0, 21};
    case GroundStyle::Swamp:
        return {qRgb(24, 32, 20), qRgb(132, 156, 66), qRgb(80, 106, 44), qRgb(102, 132, 54),
                qRgb(62, 84, 36), qRgb(40, 36, 24), qRgb(74, 58, 38), qRgb(56, 42, 26),
                qRgb(94, 76, 52), 4.0, 15};
    case GroundStyle::Snow:
        return {qRgb(52, 64, 86), qRgb(255, 255, 255), qRgb(234, 242, 252), qRgb(255, 255, 255),
                qRgb(198, 216, 238), qRgb(146, 172, 206), qRgb(108, 118, 140), qRgb(88, 96, 118),
                qRgb(134, 146, 168), 9.0, 19};
    case GroundStyle::Moon:
        return {qRgb(38, 38, 46), qRgb(226, 226, 230), qRgb(178, 178, 184), qRgb(146, 146, 154),
                qRgb(162, 162, 170), qRgb(118, 118, 128), qRgb(128, 128, 136), qRgb(104, 104, 112),
                qRgb(150, 150, 158), 3.0, 9};
    case GroundStyle::Grass:
        break;
    }
    return {qRgb(31, 58, 20), qRgb(178, 226, 60), qRgb(122, 194, 33), qRgb(96, 165, 27),
            qRgb(84, 150, 26), qRgb(46, 98, 22), qRgb(156, 90, 38), qRgb(132, 74, 30),
            qRgb(172, 104, 48), 7.0, 23};
}

void pixel(QImage &image, int x, int y, QRgb colour)
{
    if (x >= 0 && y >= 0 && x < image.width() && y < image.height())
        reinterpret_cast<QRgb *>(image.scanLine(y))[x] = colour;
}

void block(QImage &image, int x, int y, int w, int h, QRgb colour)
{
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            pixel(image, x + i, y + j, colour);
}

// Grass: dirt clusters and small pebbles in the soil.
void decorateGrass(QImage &image, int c, int y, quint32 h)
{
    QPainter art(&image);
    art.setRenderHint(QPainter::Antialiasing, false);
    if (h % 41 == 0) {
        const QPoint centres[] = {{c, y}, {c + 5, y + 1}, {c + 2, y - 3}};
        art.setPen(QColor(110, 60, 24));
        art.setBrush(QColor(222, 158, 96));
        for (const QPoint &centre : centres)
            art.drawEllipse(centre, 3, 3);
        art.setPen(Qt::NoPen);
        art.setBrush(QColor(246, 204, 150));
        for (const QPoint &centre : centres)
            art.drawRect(centre.x() - 1, centre.y() - 2, 1, 1);
    } else {
        art.fillRect(c, y, 3, 2, QColor(66, 62, 70));
        art.fillRect(c + 1, y - 1, 1, 1, QColor(66, 62, 70));
        art.fillRect(c, y, 1, 1, QColor(128, 122, 130));
    }
}

// Sand: buried shells and sandstone pebbles.
void decorateSand(QImage &image, int c, int y, quint32 h)
{
    if (h % 41 == 0) {
        // A little fan-shaped shell.
        const QRgb rim = qRgb(150, 96, 84);
        const QRgb shell = qRgb(250, 226, 214);
        const QRgb ridge = qRgb(232, 176, 168);
        block(image, c - 2, y - 2, 5, 1, rim);
        block(image, c - 3, y - 1, 7, 2, shell);
        block(image, c - 2, y + 1, 5, 1, shell);
        block(image, c - 1, y + 2, 3, 1, rim);
        pixel(image, c - 2, y - 1, ridge);
        pixel(image, c, y - 1, ridge);
        pixel(image, c + 2, y - 1, ridge);
        pixel(image, c - 1, y + 1, ridge);
        pixel(image, c + 1, y + 1, ridge);
    } else {
        block(image, c, y, 3, 2, qRgb(150, 108, 64));
        pixel(image, c + 1, y - 1, qRgb(150, 108, 64));
        pixel(image, c, y, qRgb(238, 204, 150));
    }
}

// Swamp: air bubbles and roots in the mud.
void decorateSwamp(QImage &image, int c, int y, quint32 h)
{
    if (h % 41 == 0) {
        // A few bubbles of trapped gas.
        const QRgb ring = qRgb(118, 132, 96);
        const QRgb shine = qRgb(170, 186, 140);
        const QPoint centres[] = {{c, y}, {c + 4, y - 3}, {c + 2, y + 3}};
        for (const QPoint &p : centres) {
            pixel(image, p.x(), p.y() - 1, ring);
            pixel(image, p.x() - 1, p.y(), ring);
            pixel(image, p.x() + 1, p.y(), ring);
            pixel(image, p.x(), p.y() + 1, ring);
            pixel(image, p.x() - 1, p.y() - 1, shine);
        }
    } else {
        // A crooked root.
        const QRgb root = qRgb(38, 28, 16);
        const int length = 4 + static_cast<int>((h >> 12) % 5);
        const int lean = (h >> 4) % 2 == 0 ? 1 : -1;
        for (int i = 0; i < length; ++i)
            pixel(image, c + (i / 2) * lean, y + i, root);
        pixel(image, c + lean * 2, y + length / 2, root);
    }
}

// Snow: ice crystals and stones in the frozen rock.
void decorateSnow(QImage &image, int c, int y, quint32 h)
{
    if (h % 41 == 0) {
        const QRgb edge = qRgb(120, 170, 214);
        const QRgb ice = qRgb(188, 226, 248);
        const QRgb glint = qRgb(255, 255, 255);
        // A small diamond of ice.
        pixel(image, c, y - 2, edge);
        block(image, c - 1, y - 1, 3, 1, ice);
        block(image, c - 2, y, 5, 1, ice);
        block(image, c - 1, y + 1, 3, 1, ice);
        pixel(image, c, y + 2, edge);
        pixel(image, c - 2, y, edge);
        pixel(image, c + 2, y, edge);
        pixel(image, c - 1, y - 1, glint);
    } else {
        block(image, c, y, 3, 2, qRgb(70, 74, 88));
        pixel(image, c + 1, y - 1, qRgb(70, 74, 88));
        pixel(image, c, y, qRgb(150, 156, 172));
    }
}

// Moon: craters and loose rocks in the regolith.
void decorateMoon(QImage &image, int c, int y, quint32 h)
{
    if (h % 41 == 0 || h % 41 == 20) {
        // A crater: dark bowl, shadowed upper-left rim, lit lower-right rim.
        const int r = 3 + static_cast<int>((h >> 10) % 3);
        const QRgb bowl = qRgb(100, 100, 108);
        const QRgb shadow = qRgb(78, 78, 86);
        const QRgb lit = qRgb(170, 170, 178);
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r - 1; dx <= r + 1; ++dx) {
                const double e = (dx * dx) / double((r + 1) * (r + 1)) + (dy * dy) / double(r * r);
                if (e > 1.0)
                    continue;
                QRgb colour = bowl;
                if (e > 0.55)
                    colour = dx + dy < 0 ? shadow : lit;
                pixel(image, c + dx, y + dy, colour);
            }
        }
    } else {
        block(image, c, y, 2, 2, qRgb(84, 84, 92));
        pixel(image, c + 2, y + 1, qRgb(84, 84, 92));
        pixel(image, c, y, qRgb(170, 170, 178));
    }
}

// Swamp only: reeds and cattails standing on the ground.
void drawReed(QImage &image, int c, int top, quint32 h)
{
    const QRgb stalk = qRgb(64, 112, 38);
    const QRgb stalkDark = qRgb(40, 74, 26);
    const QRgb head = qRgb(108, 64, 28);
    const QRgb headLight = qRgb(146, 92, 44);
    const int height = 6 + static_cast<int>((h >> 6) % 6);
    const int lean = (h >> 3) % 3 == 0 ? 1 : 0;
    for (int i = 1; i <= height; ++i)
        pixel(image, c + (i > height / 2 ? lean : 0), top - i, i % 3 == 0 ? stalkDark : stalk);
    if ((h >> 9) % 2 == 0) {
        // A cattail head on top.
        const int x = c + lean;
        block(image, x, top - height - 3, 1, 3, head);
        pixel(image, x, top - height - 3, headLight);
        pixel(image, x, top - height - 4, stalk);
    } else {
        // A leaf curling off to the side.
        pixel(image, c - 1, top - height / 2, stalk);
        pixel(image, c - 2, top - height / 2 - 1, stalk);
    }
}

} // namespace

GroundStyle groundStyleForMap(int map)
{
    switch (map) {
    case 1: return GroundStyle::Sand;
    case 2: return GroundStyle::Swamp;
    case 3: return GroundStyle::Snow;
    case 4: return GroundStyle::Moon;
    default: return GroundStyle::Grass;
    }
}

TerrainRaster rasterizeTerrain(const Terrain::TerrainManager &terrain, double cameraX, double cameraY,
                               int designWidth, int designHeight, GroundStyle style)
{
    constexpr int Px = TerrainPixel;
    const double snappedX = std::floor(cameraX / Px) * Px;
    const double snappedY = std::floor(cameraY / Px) * Px;
    const int columns = designWidth / Px + 2;
    const int rows = designHeight / Px + 3;
    const qint64 firstColumn = static_cast<qint64>(snappedX / Px);
    const Palette p = paletteFor(style);

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
        const int topDepth = static_cast<int>(std::lround(p.topThickness * std::sqrt(1.0 + slope * slope)))
                             + (worldColumn % 2 == 0 ? 0 : 2)
                             + static_cast<int>(hashValue(worldColumn * 31) % 2);
        // Sandstone strata wander gently up and down across the screen.
        const int strataShift = static_cast<int>(std::lround(3.0 * std::sin(worldColumn * 0.045)));
        // Swamp: deep mud patches (the ones that slow the car) replace the
        // moss with a thicker layer of glossy, dark wet mud.
        const bool mud = style == GroundStyle::Swamp && terrain.mudAt((worldColumn + 0.5) * Px) > 0.25;
        const int mudDepth = topDepth + 3;

        for (int r = std::max(top, 0); r < rows; ++r) {
            const int depth = r - top;
            QRgb colour;
            if (cliff || r <= outlineBottom) {
                colour = p.outline;
            } else if (mud && r == outlineBottom + 1) {
                colour = qRgb(112, 96, 70);  // wet shine along the top
            } else if (mud && depth <= mudDepth) {
                const quint32 n = hashValue(worldColumn * 7919 + depth * 104729) % 11;
                colour = n == 0 ? qRgb(126, 108, 78)                       // glint
                       : n < 3 ? qRgb(40, 30, 18)                          // darker swirl
                       : depth > mudDepth / 2 ? qRgb(48, 36, 22) : qRgb(58, 44, 28);
            } else if (r == outlineBottom + 1) {
                colour = p.highlight;
            } else if (depth <= topDepth) {
                const bool lower = depth > (outlineBottom - top) + topDepth / 2;
                const quint32 n = hashValue(worldColumn * 7919 + depth * 104729) % 7;
                colour = lower ? (n < 2 ? p.top : p.topShade) : (n == 0 ? p.topSpeck : p.top);
                // Swamp: the odd glint of standing water in the moss.
                if (style == GroundStyle::Swamp && !lower && n == 1 && hashValue(worldColumn * 13) % 5 == 0)
                    colour = qRgb(120, 150, 150);
            } else if (depth <= topDepth + 1) {
                colour = p.topEdge;
            } else {
                const quint32 h = hashValue(worldColumn * 15485863 + depth * 2038074743LL) % p.bodySpeckRarity;
                colour = h == 0 ? p.bodyDark : (h == 1 ? p.bodyLight : p.body);
                if (style == GroundStyle::Sand) {
                    // Layered sandstone: a darker band every 14 rows.
                    const int band = (depth + strataShift) % 14;
                    if (band == 0 || band == 1)
                        colour = h == 1 ? p.body : p.bodyDark;
                } else if (style == GroundStyle::Snow && h == 2 && depth < topDepth + 10) {
                    colour = qRgb(176, 208, 232);  // frost just under the snow
                }
            }
            reinterpret_cast<QRgb *>(image.scanLine(r))[c] = colour;
        }
    }

    // Decorations buried in the body, and reeds on top for the swamp.
    for (int c = 2; c < columns - 2; ++c) {
        const qint64 worldColumn = firstColumn + c;
        const quint32 h = hashValue(worldColumn * 2654435761LL);

        int shallowest = std::numeric_limits<int>::min();
        bool solid = true;
        for (int k = -2; k <= 2; ++k) {
            const int t = tops[c + 1 + k];
            solid = solid && t != NoGround;
            shallowest = std::max(shallowest, t);
        }
        if (!solid)
            continue;

        if (style == GroundStyle::Swamp && h % 9 == 4 && tops[c + 1] > 12 && tops[c + 1] < rows)
            drawReed(image, c, tops[c + 1], h);

        const bool feature = h % 41 == 0 || (style == GroundStyle::Moon && h % 41 == 20);
        if (!feature && h % 13 != 0)
            continue;
        const int depth = 12 + static_cast<int>((h >> 8) % 26);
        const int y = tops[c + 1] + depth;
        if (y - 5 < shallowest + 12 || y + 5 >= rows)
            continue;

        switch (style) {
        case GroundStyle::Grass: decorateGrass(image, c, y, h); break;
        case GroundStyle::Sand: decorateSand(image, c, y, h); break;
        case GroundStyle::Swamp: decorateSwamp(image, c, y, h); break;
        case GroundStyle::Snow: decorateSnow(image, c, y, h); break;
        case GroundStyle::Moon: decorateMoon(image, c, y, h); break;
        }
    }

    return {image, QRectF(snappedX - cameraX, (cameraY - snappedY) - Px, columns * Px, rows * Px)};
}

} // namespace Render
