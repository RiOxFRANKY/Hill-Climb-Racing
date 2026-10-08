#include "game_renderer.h"

#include <QFontDatabase>
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
    // Load embedded pixel fonts
    const int id1 = QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/PressStart2P-Regular.ttf"));
    if (id1 != -1) {
        const QStringList families = QFontDatabase::applicationFontFamilies(id1);
        if (!families.isEmpty())
            m_pixelFontFamily = families.first();
    }
    const int id2 = QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/Silkscreen-Bold.ttf"));
    if (id2 != -1) {
        const QStringList families = QFontDatabase::applicationFontFamilies(id2);
        if (!families.isEmpty())
            m_uiFontFamily = families.first();
    }

    // Vehicle and Backgrounds
    m_carBody.load(QStringLiteral(":/assets/car_body.png"));
    m_wheelSprite.load(QStringLiteral(":/assets/wheel.png"));

    m_bgSky.load(QStringLiteral(":/assets/bg_sky.png"));
    m_bgMountains.load(QStringLiteral(":/assets/bg_mountains.png"));
    m_bgHills.load(QStringLiteral(":/assets/bg_hills.png"));

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

    // Load pixel-art UI sprites
    m_pedalBrake.load(QStringLiteral(":/assets/ui/pedal_brake.png"));
    m_pedalBrakePressed.load(QStringLiteral(":/assets/ui/pedal_brake_pressed.png"));
    m_pedalGas.load(QStringLiteral(":/assets/ui/pedal_gas.png"));
    m_pedalGasPressed.load(QStringLiteral(":/assets/ui/pedal_gas_pressed.png"));
    m_pedalBoost.load(QStringLiteral(":/assets/ui/pedal_boost.png"));
    m_pedalBoostPressed.load(QStringLiteral(":/assets/ui/pedal_boost_pressed.png"));
    m_speedoDial.load(QStringLiteral(":/assets/ui/speedometer_dial.png"));
    m_speedoNeedle.load(QStringLiteral(":/assets/ui/speedometer_needle.png"));
    m_woodenBoard.load(QStringLiteral(":/assets/ui/wooden_board.png"));
    m_chainLink.load(QStringLiteral(":/assets/ui/chain_link.png"));
    m_coinPixmap.load(QStringLiteral(":/assets/ui/coin.png"));
    m_hudPanel.load(QStringLiteral(":/assets/ui/hud_panel.png"));
    m_dialogBox.load(QStringLiteral(":/assets/ui/dialog_box.png"));
    m_dialogButton.load(QStringLiteral(":/assets/ui/dialog_button.png"));
    m_dialogButtonPressed.load(QStringLiteral(":/assets/ui/dialog_button_pressed.png"));
    m_coinIcon.load(QStringLiteral(":/assets/ui/coin_icon.png"));
}

QFont GameRenderer::pixelFont(int pointSize, bool bold) const
{
    QFont font(!m_pixelFontFamily.isEmpty() ? m_pixelFontFamily : QStringLiteral("Courier New"), pointSize);
    font.setBold(bold);
    return font;
}

QFont GameRenderer::uiFont(int pointSize, bool bold) const
{
    QFont font(!m_uiFontFamily.isEmpty() ? m_uiFontFamily : (!m_pixelFontFamily.isEmpty() ? m_pixelFontFamily : QStringLiteral("Courier New")), pointSize);
    font.setBold(bold);
    return font;
}

QPointF GameRenderer::worldToScreen(const QPointF &world, double cameraX, double cameraY, int designHeight) const
{
    return {world.x() - cameraX,
            designHeight - (world.y() - cameraY)};
}

void GameRenderer::drawBackground(QPainter &painter, const Core::GameCore &core, double cameraX, int designWidth, int designHeight) const
{
    // Check if we are currently on the Moon level
    if (core.isMoonLevel()) {
        // Deep space black-to-dark-purple gradient background
        QLinearGradient spaceSky(0.0, 0.0, 0.0, designHeight);
        spaceSky.setColorAt(0.0, QColor(5, 5, 12));
        spaceSky.setColorAt(0.7, QColor(20, 20, 35));
        spaceSky.setColorAt(1.0, QColor(45, 45, 65));
        painter.fillRect(0, 0, designWidth, designHeight, spaceSky);

        // Parallax Crater Horizon Layer (Slower movement)
        const double craterShift = std::fmod(cameraX * 0.02, designWidth);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(30, 30, 45));
        // Draw distant lunar hills/craters procedurally
        painter.drawRect(QRectF(-craterShift, designHeight - 250, designWidth * 2, 250));
        return;
    }

    // Standard Earth Background Rendering
    if (!m_bgSky.isNull() && !m_bgMountains.isNull() && !m_bgHills.isNull()) {
        const double stripW = m_bgSky.width();

        const double skyShift = std::fmod(cameraX * 0.012, stripW);
        painter.drawPixmap(QPointF(-skyShift, 0.0), m_bgSky);
        if (stripW - skyShift < designWidth)
            painter.drawPixmap(QPointF(stripW - skyShift, 0.0), m_bgSky);

        const double mtnShift = std::fmod(cameraX * 0.038, stripW);
        painter.drawPixmap(QPointF(-mtnShift, 0.0), m_bgMountains);
        if (stripW - mtnShift < designWidth)
            painter.drawPixmap(QPointF(stripW - mtnShift, 0.0), m_bgMountains);

        const double hillsShift = std::fmod(cameraX * 0.095, stripW);
        painter.drawPixmap(QPointF(-hillsShift, 0.0), m_bgHills);
        if (stripW - hillsShift < designWidth)
            painter.drawPixmap(QPointF(stripW - hillsShift, 0.0), m_bgHills);
        return;
    }

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
    painter.setFont(pixelFont(16, true));
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
            const double pulse = 1.0 + 0.05 * std::sin(survivalTime * 5.0 + pickup.position.x());
            const double coinSize = 46.0 * pulse;
            painter.save();
            painter.translate(center);
            if (!m_coinPixmap.isNull()) {
                painter.drawPixmap(QRectF(-coinSize * 0.5, -coinSize * 0.5, coinSize, coinSize),
                                   m_coinPixmap, m_coinPixmap.rect());
            } else {
                painter.scale(pulse, pulse);
                painter.setPen(QPen(QColor(178, 112, 14), 6.0));
                painter.setBrush(QColor(255, 203, 40));
                painter.drawEllipse(QPointF(0.0, 0.0), 23.0, 23.0);
            }
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

    const QRectF panelRect(42.0, 36.0, 520.0, 164.0);
    if (!m_hudPanel.isNull()) {
        painter.drawPixmap(panelRect, m_hudPanel, m_hudPanel.rect());
    }

    const auto drawPixelTextWithShadow = [&](const QPointF &pt, const QString &text, const QColor &fg, const QFont &font) {
        painter.setFont(font);
        painter.setPen(QColor(8, 12, 16, 240));
        painter.drawText(pt + QPointF(2.0, 2.0), text);
        painter.setPen(fg);
        painter.drawText(pt, text);
    };

    const QFont labelFont = uiFont(11, true);
    const QFont numFont = pixelFont(16, true);

    drawPixelTextWithShadow(QPointF(64.0, 72.0), QStringLiteral("DISTANCE"), QColor(175, 195, 215), labelFont);
    drawPixelTextWithShadow(QPointF(290.0, 72.0), QStringLiteral("COINS"), QColor(175, 195, 215), labelFont);
    drawPixelTextWithShadow(QPointF(428.0, 72.0), QStringLiteral("SCORE"), QColor(175, 195, 215), labelFont);

    const int distance = static_cast<int>(std::max(0.0, core.vehicle().chassis().position.x()) / core.terrain().pixelsPerMetre());
    drawPixelTextWithShadow(QPointF(64.0, 116.0), QStringLiteral("%1 m").arg(distance), Qt::white, numFont);

    if (!m_coinIcon.isNull()) {
        painter.drawPixmap(QRectF(280.0, 92.0, 26.0, 26.0), m_coinIcon, m_coinIcon.rect());
    }
    drawPixelTextWithShadow(QPointF(314.0, 116.0), QString::number(core.coins()), QColor(255, 215, 45), numFont);
    drawPixelTextWithShadow(QPointF(428.0, 116.0), QString::number(core.score()), Qt::white, numFont);

    drawPixelTextWithShadow(QPointF(64.0, 166.0), QStringLiteral("FUEL"), QColor(175, 195, 215), labelFont);

    const double fuel = core.vehicle().fuel();
    constexpr int totalSegments = 20;
    constexpr double segStartX = 145.0;
    constexpr double segY = 148.0;
    constexpr double segWidth = 15.5;
    constexpr double segHeight = 22.0;
    constexpr double segGap = 3.0;

    const int activeSegments = static_cast<int>(std::round((fuel / 100.0) * totalSegments));
    const bool isLowFuel = fuel <= 28.0;

    for (int i = 0; i < totalSegments; ++i) {
        const double sx = segStartX + i * (segWidth + segGap);
        const QRectF segRect(sx, segY, segWidth, segHeight);

        if (i < activeSegments) {
            const QColor mainColor = isLowFuel ? QColor(245, 55, 45) : QColor(60, 225, 95);
            const QColor hiColor = isLowFuel ? QColor(255, 160, 150) : QColor(160, 255, 180);
            const QColor shadowColor = isLowFuel ? QColor(140, 20, 15) : QColor(25, 120, 45);

            painter.setPen(QColor(8, 12, 16));
            painter.setBrush(mainColor);
            painter.drawRect(segRect);

            painter.setPen(hiColor);
            painter.drawLine(QPointF(segRect.left() + 1, segRect.top() + 1), QPointF(segRect.right() - 1, segRect.top() + 1));
            painter.setPen(shadowColor);
            painter.drawLine(QPointF(segRect.left() + 1, segRect.bottom() - 1), QPointF(segRect.right() - 1, segRect.bottom() - 1));
        } else {
            painter.setPen(QColor(8, 12, 16));
            painter.setBrush(QColor(16, 22, 30, 230));
            painter.drawRect(segRect);
        }
    }

    painter.setFont(uiFont(11, true));
    painter.setPen(QColor(8, 12, 16, 240));
    painter.drawText(QRectF(762.0, 32.0, 400.0, 40.0), Qt::AlignCenter,
                     QStringLiteral("P  PAUSE   •   R  RESTART   •   ESC  QUIT"));
    painter.setPen(QColor(240, 245, 255, 220));
    painter.drawText(QRectF(760.0, 30.0, 400.0, 40.0), Qt::AlignCenter,
                     QStringLiteral("P  PAUSE   •   R  RESTART   •   ESC  QUIT"));

    const auto &sections = core.terrain().sections();
    if (!sections.isEmpty()) {
        const Terrain::CourseSection &section = sections.at(core.terrain().sectionAt(core.vehicle().chassis().position.x()));
        const QString range = QStringLiteral("%1–%2 KM")
                                  .arg(section.start / core.terrain().pixelsPerMetre() / 1000.0)
                                  .arg(section.end / core.terrain().pixelsPerMetre() / 1000.0);

        painter.setFont(uiFont(12, true));
        painter.setPen(QColor(10, 14, 20, 240));
        painter.drawText(QRectF(562.0, 66.0, 800.0, 32.0), Qt::AlignCenter,
                         QStringLiteral("%1  ·  %2   (%3 / %4 m)")
                             .arg(range, section.name)
                             .arg(distance)
                             .arg(static_cast<int>(core.terrain().finishX() / core.terrain().pixelsPerMetre())));
        painter.setPen(QColor(255, 226, 120));
        painter.drawText(QRectF(560.0, 64.0, 800.0, 32.0), Qt::AlignCenter,
                         QStringLiteral("%1  ·  %2   (%3 / %4 m)")
                             .arg(range, section.name)
                             .arg(distance)
                             .arg(static_cast<int>(core.terrain().finishX() / core.terrain().pixelsPerMetre())));
    }

    painter.restore();
}

void GameRenderer::drawHangingBanner(QPainter &painter, const Core::GameCore &core, int designWidth) const
{
    const double timer = core.zoneBannerTime();
    if (timer <= 0.0)
        return;

    const auto &sections = core.terrain().sections();
    if (sections.isEmpty())
        return;

    const Terrain::CourseSection &section = sections.at(core.terrain().sectionAt(core.vehicle().chassis().position.x()));
    const QString range = QStringLiteral("%1–%2 KM")
                              .arg(section.start / core.terrain().pixelsPerMetre() / 1000.0)
                              .arg(section.end / core.terrain().pixelsPerMetre() / 1000.0);

    const double elapsed = 5.0 - timer;
    constexpr double targetCenterY = 112.0;
    constexpr double startCenterY = -180.0;
    constexpr double boardW = 720.0;
    constexpr double boardH = 124.0;
    const double centerX = designWidth * 0.5;

    double currentCenterY = targetCenterY;
    double swingDeg = 0.0;

    if (elapsed < 1.4) {
        const double dropT = clampValue(elapsed / 0.6, 0.0, 1.0);
        const double easeDrop = 1.0 - std::pow(1.0 - dropT, 3);
        const double baseCenterY = startCenterY + (targetCenterY - startCenterY) * easeDrop;
        const double bounce = 26.0 * std::exp(-3.2 * elapsed) * std::sin(elapsed * 15.0);
        currentCenterY = baseCenterY + bounce;
        swingDeg = 4.0 * std::exp(-2.4 * elapsed) * std::sin(elapsed * 9.5);
    } else if (elapsed < 4.2) {
        currentCenterY = targetCenterY;
        swingDeg = 0.3 * std::sin(elapsed * 2.2);
    } else {
        const double retractT = clampValue((elapsed - 4.2) / 0.8, 0.0, 1.0);
        currentCenterY = targetCenterY - (targetCenterY - startCenterY) * (retractT * retractT);
        swingDeg = -2.5 * std::sin(retractT * Pi);
    }

    if (currentCenterY < startCenterY - 20.0)
        return;

    painter.save();

    const double rad = swingDeg * Pi / 180.0;
    const double eyeletRelX = 252.0;
    const double eyeletRelY = -boardH * 0.5 + 4.0;

    const QPointF leftEyelet(centerX + (-eyeletRelX * std::cos(rad) - eyeletRelY * std::sin(rad)),
                             currentCenterY + (-eyeletRelX * std::sin(rad) + eyeletRelY * std::cos(rad)));
    const QPointF rightEyelet(centerX + (eyeletRelX * std::cos(rad) - eyeletRelY * std::sin(rad)),
                              currentCenterY + (eyeletRelX * std::sin(rad) + eyeletRelY * std::cos(rad)));

    const auto drawChain = [&](const QPointF &target) {
        const double linkH = 26.0;
        const double topY = -15.0;
        const double bottomY = target.y();
        const double linkX = target.x();
        for (double y = topY; y < bottomY; y += linkH) {
            if (!m_chainLink.isNull()) {
                painter.drawPixmap(QRectF(linkX - 8.0, y, 16.0, 32.0), m_chainLink, m_chainLink.rect());
            } else {
                painter.setPen(QPen(QColor(20, 24, 32), 6.0));
                painter.drawLine(QPointF(linkX, topY), target);
            }
        }
    };

    drawChain(leftEyelet);
    drawChain(rightEyelet);

    painter.translate(centerX, currentCenterY);
    painter.rotate(swingDeg);

    if (!m_woodenBoard.isNull()) {
        painter.drawPixmap(QRectF(-boardW * 0.5, -boardH * 0.5, boardW, boardH), m_woodenBoard, m_woodenBoard.rect());
    } else {
        painter.setBrush(QColor(115, 68, 38));
        painter.drawRect(QRectF(-boardW * 0.5, -boardH * 0.5, boardW, boardH));
    }

    painter.setFont(uiFont(12, true));
    painter.setPen(QColor(25, 12, 6, 230));
    painter.drawText(QRectF(-boardW * 0.5, -boardH * 0.5 + 18.0, boardW, 26.0), Qt::AlignCenter, range);
    painter.setPen(QColor(245, 220, 160));
    painter.drawText(QRectF(-boardW * 0.5, -boardH * 0.5 + 16.0, boardW, 26.0), Qt::AlignCenter, range);

    painter.setFont(pixelFont(17, true));
    painter.setPen(QColor(25, 12, 6, 240));
    painter.drawText(QRectF(-boardW * 0.5, -boardH * 0.5 + 48.0, boardW, 46.0), Qt::AlignCenter, section.name);
    painter.setPen(QColor(255, 210, 50));
    painter.drawText(QRectF(-boardW * 0.5, -boardH * 0.5 + 46.0, boardW, 46.0), Qt::AlignCenter, section.name);

    painter.restore();
}

void GameRenderer::drawPedals(QPainter &painter, const Core::GameCore &core) const
{
    painter.save();

    const bool leftActive = core.isKeyPressed(Qt::Key_Left) || core.isKeyPressed(Qt::Key_A);
    const bool rightActive = core.isKeyPressed(Qt::Key_Right) || core.isKeyPressed(Qt::Key_D);
    const bool boostActive = core.isKeyPressed(Qt::Key_Space);

    const QRectF brakeRect(48.0, 900.0, 250.0, 120.0);
    const QPixmap &brakePix = leftActive ? m_pedalBrakePressed : m_pedalBrake;
    if (!brakePix.isNull()) {
        painter.drawPixmap(brakeRect, brakePix, brakePix.rect());
    }

    painter.setFont(pixelFont(15, true));
    painter.setPen(QColor(8, 12, 16));
    painter.drawText(QRectF(brakeRect.left() + 2, brakeRect.top() + 38.0, brakeRect.width(), 32.0), Qt::AlignCenter, QStringLiteral("BRAKE"));
    painter.setPen(leftActive ? QColor(255, 240, 160) : Qt::white);
    painter.drawText(QRectF(brakeRect.left(), brakeRect.top() + 36.0, brakeRect.width(), 32.0), Qt::AlignCenter, QStringLiteral("BRAKE"));

    painter.setFont(uiFont(10, true));
    painter.setPen(QColor(8, 12, 16));
    painter.drawText(QRectF(brakeRect.left() + 2, brakeRect.top() + 72.0, brakeRect.width(), 24.0), Qt::AlignCenter, QStringLiteral("REVERSE"));
    painter.setPen(leftActive ? QColor(255, 180, 170) : QColor(200, 215, 230));
    painter.drawText(QRectF(brakeRect.left(), brakeRect.top() + 70.0, brakeRect.width(), 24.0), Qt::AlignCenter, QStringLiteral("REVERSE"));

    const QRectF boostRect(817.0, 900.0, 286.0, 120.0);
    const QPixmap &boostPix = boostActive ? m_pedalBoostPressed : m_pedalBoost;
    if (!boostPix.isNull()) {
        painter.drawPixmap(boostRect, boostPix, boostPix.rect());
    }

    painter.setFont(pixelFont(16, true));
    painter.setPen(QColor(8, 12, 16));
    painter.drawText(QRectF(boostRect.left() + 2, boostRect.top() + 46.0, boostRect.width(), 36.0), Qt::AlignCenter, QStringLiteral("BOOST"));
    painter.setPen(boostActive ? QColor(100, 240, 255) : QColor(255, 210, 50));
    painter.drawText(QRectF(boostRect.left(), boostRect.top() + 44.0, boostRect.width(), 36.0), Qt::AlignCenter, QStringLiteral("BOOST"));

    const QRectF gasRect(1622.0, 900.0, 250.0, 120.0);
    const QPixmap &gasPix = rightActive ? m_pedalGasPressed : m_pedalGas;
    if (!gasPix.isNull()) {
        painter.drawPixmap(gasRect, gasPix, gasPix.rect());
    }

    painter.setFont(pixelFont(16, true));
    painter.setPen(QColor(8, 12, 16));
    painter.drawText(QRectF(gasRect.left() + 2, gasRect.top() + 46.0, gasRect.width(), 36.0), Qt::AlignCenter, QStringLiteral("GAS"));
    painter.setPen(rightActive ? QColor(160, 255, 180) : Qt::white);
    painter.drawText(QRectF(gasRect.left(), gasRect.top() + 44.0, gasRect.width(), 36.0), Qt::AlignCenter, QStringLiteral("GAS"));

    painter.restore();
}

void GameRenderer::drawSpeedometer(QPainter &painter, const Core::GameCore &core) const
{
    painter.save();

    const QRectF dialRect(1720.0, 36.0, 152.0, 152.0);
    if (!m_speedoDial.isNull()) {
        painter.drawPixmap(dialRect, m_speedoDial, m_speedoDial.rect());
    }

    const double speedPx = std::abs(core.vehicle().chassis().velocity.x());
    const double speedKmh = (speedPx / core.terrain().pixelsPerMetre()) * 3.6;
    const double speedAngle = -135.0 + clampValue(speedKmh / 140.0, 0.0, 1.0) * 270.0;

    const double dialCenterX = dialRect.center().x();
    const double dialCenterY = dialRect.center().y();

    if (!m_speedoNeedle.isNull()) {
        painter.save();
        painter.translate(dialCenterX, dialCenterY);
        painter.rotate(speedAngle);
        painter.drawPixmap(QRectF(-10.0, -44.0, 20.0, 56.0), m_speedoNeedle, m_speedoNeedle.rect());
        painter.restore();
    }

    const int speedVal = static_cast<int>(std::round(speedKmh));
    painter.setFont(pixelFont(15, true));
    painter.setPen(QColor(8, 12, 16));
    painter.drawText(QRectF(dialRect.left() + 2, dialRect.top() + 92.0, dialRect.width(), 28.0),
                     Qt::AlignCenter, QString::number(speedVal));
    painter.setPen(QColor(255, 220, 60));
    painter.drawText(QRectF(dialRect.left(), dialRect.top() + 90.0, dialRect.width(), 28.0),
                     Qt::AlignCenter, QString::number(speedVal));

    painter.setFont(uiFont(9, true));
    painter.setPen(QColor(8, 12, 16));
    painter.drawText(QRectF(dialRect.left() + 1, dialRect.top() + 120.0, dialRect.width(), 16.0),
                     Qt::AlignCenter, QStringLiteral("KM/H"));
    painter.setPen(QColor(160, 185, 210));
    painter.drawText(QRectF(dialRect.left(), dialRect.top() + 119.0, dialRect.width(), 16.0),
                     Qt::AlignCenter, QStringLiteral("KM/H"));

    painter.restore();
}

void GameRenderer::drawOverlay(QPainter &painter, const Core::GameCore &core, const QRectF &rect) const
{
    if (!core.isPaused() && !core.isGameOver())
        return;

    painter.save();
    painter.fillRect(rect, QColor(6, 10, 16, 175));

    const QRectF card(610.0, 320.0, 700.0, 390.0);
    if (!m_dialogBox.isNull()) {
        painter.drawPixmap(card, m_dialogBox, m_dialogBox.rect());
    } else {
        painter.setPen(QColor(60, 75, 95));
        painter.setBrush(QColor(16, 22, 32, 245));
        painter.drawRect(card);
    }

    const QString title = core.isGameOver() ? core.gameOverReason() : QStringLiteral("PAUSED");
    painter.setFont(pixelFont(24, true));
    painter.setPen(QColor(6, 8, 12));
    painter.drawText(QRectF(card.left() + 3, card.top() + 53.0, card.width(), 80.0), Qt::AlignCenter, title);
    painter.setPen(core.isGameOver() ? QColor(255, 80, 60) : QColor(255, 220, 100));
    painter.drawText(QRectF(card.left(), card.top() + 50.0, card.width(), 80.0), Qt::AlignCenter, title);

    const int distance = static_cast<int>(std::max(0.0, core.vehicle().chassis().position.x()) / core.terrain().pixelsPerMetre());
    const QString stats = QStringLiteral("%1 m   •   %2 coins   •   %3 points")
                              .arg(distance).arg(core.coins()).arg(core.score());
    painter.setFont(uiFont(15, true));
    painter.setPen(QColor(6, 8, 12));
    painter.drawText(QRectF(card.left() + 2, card.top() + 152.0, card.width(), 55.0), Qt::AlignCenter, stats);
    painter.setPen(QColor(210, 225, 240));
    painter.drawText(QRectF(card.left(), card.top() + 150.0, card.width(), 55.0), Qt::AlignCenter, stats);

    const QRectF btnRect(810.0, 565.0, 300.0, 74.0);
    if (!m_dialogButton.isNull()) {
        painter.drawPixmap(btnRect, m_dialogButton, m_dialogButton.rect());
    }

    const QString btnText = !core.isGameOver() ? QStringLiteral("P  RESUME")
                                               : (core.isFinished() ? QStringLiteral("R  RACE AGAIN")
                                                                    : QStringLiteral("R  TRY AGAIN"));
    painter.setFont(pixelFont(13, true));
    painter.setPen(QColor(6, 8, 12));
    painter.drawText(QRectF(btnRect.left() + 2, btnRect.top() + 3.0, btnRect.width(), btnRect.height()), Qt::AlignCenter, btnText);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(btnRect.left(), btnRect.top() + 1.0, btnRect.width(), btnRect.height()), Qt::AlignCenter, btnText);

    painter.restore();
}

} // namespace Render