#include "game_renderer.h"

#include <QImage>
#include <QLinearGradient>
#include <QPainterPath>
#include <QVarLengthArray>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Render {

using namespace Physics;
using namespace Terrain;

void GameRenderer::loadAssets()
{
    m_carBody.load(QStringLiteral(":/assets/car_body.png"));
    m_wheelSprite.load(QStringLiteral(":/assets/wheel.png"));

    const QImage background(QStringLiteral(":/assets/background.png"));
    if (!background.isNull()) {
        const QImage scaled = background.scaled(1920, 1080,
                                                Qt::IgnoreAspectRatio,
                                                Qt::SmoothTransformation);
        m_backgroundStrip = QPixmap(1920 * 2, 1080);
        QPainter stripPainter(&m_backgroundStrip);
        stripPainter.drawImage(0, 0, scaled);
        stripPainter.drawImage(1920, 0, scaled.flipped(Qt::Horizontal));
    }
}

QPointF GameRenderer::worldToScreen(const QPointF &world, double cameraX, double cameraY, int designHeight) const
{
    return {world.x() - cameraX,
            designHeight - (world.y() - cameraY)};
}

void GameRenderer::drawBackground(QPainter &painter, double cameraX, int designWidth, int designHeight) const
{
    if (m_backgroundStrip.isNull()) {
        QLinearGradient sky(0.0, 0.0, 0.0, designHeight);
        sky.setColorAt(0.0, QColor(37, 119, 190));
        sky.setColorAt(0.54, QColor(104, 191, 226));
        sky.setColorAt(1.0, QColor(214, 238, 215));
        painter.fillRect(0, 0, designWidth, designHeight, sky);
        return;
    }

    const double stripWidth = m_backgroundStrip.width();
    const double shift = std::fmod(cameraX * 0.06, stripWidth);
    painter.drawPixmap(QPointF(-shift, 0.0), m_backgroundStrip);
    if (stripWidth - shift < designWidth)
        painter.drawPixmap(QPointF(stripWidth - shift, 0.0), m_backgroundStrip);
}

void GameRenderer::drawTerrain(QPainter &painter, const Terrain::TerrainManager &terrain, double cameraX, double cameraY, int designWidth, int designHeight) const
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

    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(QRectF(snappedX - cameraX, (cameraY - snappedY) - Px,
                             columns * Px, rows * Px),
                      image);
    painter.restore();
}

void GameRenderer::drawFinishLine(QPainter &painter, const Terrain::TerrainManager &terrain, double cameraX, double cameraY, int designWidth, int designHeight) const
{
    const double finishX = terrain.finishX();
    const QPointF base = worldToScreen(QPointF(finishX, terrain.surfaceHeight(finishX)), cameraX, cameraY, designHeight);
    if (base.x() < -200.0 || base.x() > designWidth + 200.0)
        return;

    constexpr double Height = 260.0;
    constexpr double Width = 180.0;
    constexpr double Cell = 20.0;
    painter.save();
    painter.setPen(QPen(QColor(40, 40, 40), 8.0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(base + QPointF(-Width * 0.5, 0.0), base + QPointF(-Width * 0.5, -Height));
    painter.drawLine(base + QPointF(Width * 0.5, 0.0), base + QPointF(Width * 0.5, -Height));
    painter.setPen(Qt::NoPen);
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < static_cast<int>(Width / Cell); ++column) {
            painter.setBrush((row + column) % 2 == 0 ? QColor(20, 20, 20) : QColor(245, 245, 245));
            painter.drawRect(QRectF(base.x() - Width * 0.5 + column * Cell,
                                    base.y() - Height + row * Cell, Cell, Cell));
        }
    }
    painter.setPen(QColor(255, 206, 60));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 20, QFont::Black));
    painter.drawText(QRectF(base.x() - 150.0, base.y() - Height - 46.0, 300.0, 40.0),
                     Qt::AlignCenter, QStringLiteral("FINISH"));
    painter.restore();
}

void GameRenderer::drawPickups(QPainter &painter, const Terrain::TerrainManager &terrain, double survivalTime, double cameraX, double cameraY, int designWidth, int designHeight) const
{
    for (const Terrain::Pickup &pickup : terrain.pickups()) {
        if (pickup.collected)
            continue;
        const QPointF center = worldToScreen(pickup.position, cameraX, cameraY, designHeight);
        if (center.x() < -100.0 || center.x() > designWidth + 100.0)
            continue;

        if (pickup.type == Terrain::Pickup::Type::Coin) {
            const double pulse = 1.0 + 0.06 * std::sin(survivalTime * 5.0 + pickup.position.x());
            painter.save();
            painter.translate(center);
            painter.scale(pulse, pulse);
            painter.setPen(QPen(QColor(178, 112, 14), 6.0));
            painter.setBrush(QColor(255, 203, 40));
            painter.drawEllipse(QPointF(0.0, 0.0), 25.0, 25.0);
            painter.setPen(QPen(QColor(255, 239, 128), 5.0, Qt::SolidLine, Qt::RoundCap));
            painter.drawArc(QRectF(-14.0, -15.0, 28.0, 30.0), 65 * 16, 125 * 16);
            painter.setPen(QPen(QColor(194, 128, 15), 3.0));
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 18, QFont::Black));
            painter.drawText(QRectF(-20.0, -21.0, 40.0, 42.0), Qt::AlignCenter, QStringLiteral("C"));
            painter.restore();
        } else {
            painter.save();
            painter.translate(center);
            painter.rotate(-5.0);
            painter.setPen(QPen(QColor(80, 27, 25), 5.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(QColor(214, 49, 43));
            painter.drawRoundedRect(QRectF(-31.0, -38.0, 62.0, 76.0), 10.0, 10.0);
            painter.setBrush(QColor(71, 75, 74));
            painter.drawRoundedRect(QRectF(4.0, -51.0, 23.0, 17.0), 4.0, 4.0);
            painter.setPen(QPen(QColor(255, 235, 215), 6.0));
            painter.drawLine(QPointF(-16.0, -18.0), QPointF(16.0, 18.0));
            painter.drawLine(QPointF(16.0, -18.0), QPointF(-16.0, 18.0));
            painter.restore();
        }
    }
}

void GameRenderer::drawCar(QPainter &painter, const Vehicle::VehicleEntity &vehicle, double cameraX, double cameraY, int designHeight) const
{
    if (m_carBody.isNull() || m_wheelSprite.isNull())
        return;

    for (const RigidBody &wheel : vehicle.wheels()) {
        painter.save();
        painter.translate(worldToScreen(wheel.position, cameraX, cameraY, designHeight));
        painter.rotate(-wheel.angle * 180.0 / Pi);
        painter.drawPixmap(
            QRectF(-WheelSpriteTargetRadius, -WheelSpriteTargetRadius,
                   WheelSpriteTargetRadius * 2.0, WheelSpriteTargetRadius * 2.0),
            m_wheelSprite, QRectF(m_wheelSprite.rect()));
        painter.restore();
    }

    painter.save();
    painter.translate(worldToScreen(vehicle.chassis().position, cameraX, cameraY, designHeight));
    painter.rotate(-vehicle.chassis().angle * 180.0 / Pi);
    painter.translate(0.0, CenterOfMassY);
    const double spriteLeft = -CarSourceCenterX * CarSpriteScale;
    const double spriteTop = -WheelLocalY - WheelSourceY * CarSpriteScale;
    painter.drawPixmap(
        QRectF(spriteLeft, spriteTop,
               m_carBody.width() * CarSpriteScale,
               m_carBody.height() * CarSpriteScale),
        m_carBody, QRectF(m_carBody.rect()));
    painter.restore();
}

void GameRenderer::drawHud(QPainter &painter, const Core::GameCore &core, int, int) const
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(7, 21, 32, 178));
    painter.drawRoundedRect(QRectF(42.0, 36.0, 520.0, 164.0), 24.0, 24.0);

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 19, QFont::DemiBold));
    painter.drawText(QPointF(76.0, 79.0), QStringLiteral("DISTANCE"));
    painter.drawText(QPointF(282.0, 79.0), QStringLiteral("COINS"));
    painter.drawText(QPointF(425.0, 79.0), QStringLiteral("SCORE"));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 30, QFont::Black));
    const int distance = static_cast<int>(std::max(0.0, core.vehicle().chassis().position.x()) / core.terrain().pixelsPerMetre());
    painter.drawText(QPointF(76.0, 125.0), QStringLiteral("%1 m").arg(distance));
    painter.setPen(QColor(255, 211, 55));
    painter.drawText(QPointF(300.0, 125.0), QString::number(core.coins()));
    painter.setPen(Qt::white);
    painter.drawText(QPointF(425.0, 125.0), QString::number(core.score()));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 15, QFont::DemiBold));
    painter.drawText(QPointF(76.0, 170.0), QStringLiteral("FUEL"));
    painter.setBrush(QColor(255, 255, 255, 45));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(145.0, 147.0, 374.0, 25.0), 12.0, 12.0);

    const double fuel = core.vehicle().fuel();
    const QColor fuelColor = fuel > 28.0 ? QColor(79, 211, 105) : QColor(245, 74, 65);
    QLinearGradient fuelGradient(145.0, 0.0, 519.0, 0.0);
    fuelGradient.setColorAt(0.0, fuelColor.darker(115));
    fuelGradient.setColorAt(1.0, fuelColor.lighter(120));
    painter.setBrush(fuelGradient);
    painter.drawRoundedRect(QRectF(145.0, 147.0, 374.0 * (fuel / 100.0), 25.0), 12.0, 12.0);

    const bool leftActive = core.isKeyPressed(Qt::Key_Left) || core.isKeyPressed(Qt::Key_A);
    const bool rightActive = core.isKeyPressed(Qt::Key_Right) || core.isKeyPressed(Qt::Key_D);
    const bool boostActive = core.isKeyPressed(Qt::Key_Space);
    drawKeyHint(painter, QRectF(48.0, 900.0, 250.0, 118.0),
                QStringLiteral("A / ←"), QStringLiteral("BRAKE / REVERSE"), leftActive);
    drawKeyHint(painter, QRectF(1622.0, 900.0, 250.0, 118.0),
                QStringLiteral("D / →"), QStringLiteral("GAS"), rightActive);
    drawKeyHint(painter, QRectF(817.0, 900.0, 286.0, 118.0),
                QStringLiteral("SPACE"), QStringLiteral("BOOST"), boostActive);

    painter.setPen(QColor(255, 255, 255, 170));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 14, QFont::Medium));
    painter.drawText(QRectF(760.0, 30.0, 400.0, 40.0), Qt::AlignCenter,
                     QStringLiteral("P  PAUSE     •     R  RESTART     •     ESC  QUIT"));

    const auto &sections = core.terrain().sections();
    if (!sections.isEmpty()) {
        const Terrain::CourseSection &section = sections.at(core.terrain().sectionAt(core.vehicle().chassis().position.x()));
        const QString range = QStringLiteral("%1–%2 KM")
                                  .arg(section.start / core.terrain().pixelsPerMetre() / 1000.0)
                                  .arg(section.end / core.terrain().pixelsPerMetre() / 1000.0);

        painter.setPen(QColor(255, 226, 120, 220));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 15, QFont::Bold));
        painter.drawText(QRectF(560.0, 64.0, 800.0, 32.0), Qt::AlignCenter,
                         QStringLiteral("%1  ·  %2   (%3 / %4 m)")
                             .arg(range, section.name)
                             .arg(distance)
                             .arg(static_cast<int>(core.terrain().finishX() / core.terrain().pixelsPerMetre())));

        if (core.zoneBannerTime() > 0.0) {
            const double alpha = clampValue(core.zoneBannerTime(), 0.0, 1.0);
            const QRectF banner(610.0, 120.0, 700.0, 110.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(7, 21, 32, static_cast<int>(185 * alpha)));
            painter.drawRoundedRect(banner, 26.0, 26.0);
            painter.setPen(QColor(255, 255, 255, static_cast<int>(200 * alpha)));
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 16, QFont::DemiBold));
            painter.drawText(QRectF(banner.left(), banner.top() + 12.0, banner.width(), 30.0),
                             Qt::AlignCenter, range);
            painter.setPen(QColor(255, 206, 60, static_cast<int>(255 * alpha)));
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 34, QFont::Black));
            painter.drawText(QRectF(banner.left(), banner.top() + 42.0, banner.width(), 56.0),
                             Qt::AlignCenter, section.name);
        }
    }
    painter.restore();
}

void GameRenderer::drawKeyHint(QPainter &painter, const QRectF &rect,
                             const QString &key, const QString &label, bool active) const
{
    painter.save();
    painter.setPen(QPen(active ? QColor(255, 236, 131) : QColor(255, 255, 255, 90), 3.0));
    painter.setBrush(active ? QColor(224, 81, 50, 225) : QColor(7, 21, 32, 165));
    painter.drawRoundedRect(rect, 22.0, 22.0);
    painter.setPen(Qt::white);
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Black));
    painter.drawText(QRectF(rect.left(), rect.top() + 11.0, rect.width(), 38.0),
                     Qt::AlignCenter, key);
    painter.setPen(QColor(255, 255, 255, 185));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 12, QFont::DemiBold));
    painter.drawText(QRectF(rect.left(), rect.top() + 61.0, rect.width(), 30.0),
                     Qt::AlignCenter, label);
    painter.restore();
}

void GameRenderer::drawOverlay(QPainter &painter, const Core::GameCore &core, const QRectF &rect) const
{
    if (!core.isPaused() && !core.isGameOver())
        return;

    painter.fillRect(rect, QColor(5, 13, 22, 150));
    const QRectF card(610.0, 320.0, 700.0, 390.0);
    painter.setPen(QPen(QColor(255, 255, 255, 55), 3.0));
    painter.setBrush(QColor(14, 31, 45, 235));
    painter.drawRoundedRect(card, 35.0, 35.0);

    painter.setPen(Qt::white);
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 54, QFont::Black));
    painter.drawText(QRectF(650.0, 365.0, 620.0, 90.0), Qt::AlignCenter,
                     core.isGameOver() ? core.gameOverReason() : QStringLiteral("PAUSED"));

    const int distance = static_cast<int>(std::max(0.0, core.vehicle().chassis().position.x()) / core.terrain().pixelsPerMetre());
    painter.setPen(QColor(200, 222, 232));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 24, QFont::DemiBold));
    painter.drawText(QRectF(650.0, 475.0, 620.0, 55.0), Qt::AlignCenter,
                     QStringLiteral("%1 m     •     %2 coins     •     %3 points")
                         .arg(distance).arg(core.coins()).arg(core.score()));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(225, 70, 47));
    painter.drawRoundedRect(QRectF(810.0, 565.0, 300.0, 74.0), 18.0, 18.0);
    painter.setPen(Qt::white);
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Black));
    painter.drawText(QRectF(810.0, 565.0, 300.0, 74.0), Qt::AlignCenter,
                     !core.isGameOver() ? QStringLiteral("P  RESUME")
                                        : (core.isFinished() ? QStringLiteral("R  RACE AGAIN")
                                                             : QStringLiteral("R  TRY AGAIN")));
}

} // namespace Render
