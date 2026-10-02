#include "render/GameWidget.h"
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QLinearGradient>
#include <QtGui/QFont>
#include <QtCore/QTime>
#include <algorithm>
#include <cmath>

namespace Render {

GameWidget::GameWidget(QWidget* parent)
    : GameWidget(Config(), parent) {
}

GameWidget::GameWidget(const Config& config, QWidget* parent)
    : QWidget(parent)
    , m_config(config)
    , m_engine()
    , m_sprites()
    , m_gameTimer(this)
    , m_canvasBuffer(config.virtualCanvasWidth, config.virtualCanvasHeight, QImage::Format_ARGB32_Premultiplied) {

    // Set default Full HD window size and enable keyboard focus
    resize(m_config.defaultWindowWidth, m_config.defaultWindowHeight);
    setFocusPolicy(Qt::StrongFocus);
    setWindowTitle(QStringLiteral("Hill Climb Racing - Qt 6 MinGW 64-bit"));

    setupEngine();
}

void GameWidget::setupEngine() {
    // Load all 14 starter PNG assets
    m_sprites.loadAll();

    // Configure 60 FPS master game loop timer (~16.6ms)
    connect(&m_gameTimer, &QTimer::timeout, this, [this]() {
        float dt = 1.0f / static_cast<float>(m_config.targetFps);
        m_engine.tick(dt);
        update(); // Request repaint
    });

    m_gameTimer.start(1000 / m_config.targetFps);
}

void GameWidget::paintEvent(QPaintEvent* /*event*/) {
    // 1. Render entire game scene into the 960x540 virtual canvas buffer
    {
        QPainter canvasPainter(&m_canvasBuffer);
        canvasPainter.setRenderHint(QPainter::Antialiasing, true);
        canvasPainter.setRenderHint(QPainter::SmoothPixmapTransform, false); // Crisp pixel art

        renderScene(canvasPainter);
    }

    // 2. Render virtual canvas to widget with exact aspect ratio and crisp FastTransformation
    QPainter widgetPainter(this);
    widgetPainter.fillRect(rect(), Qt::black); // Letterbox border background

    float scaleX = static_cast<float>(width()) / static_cast<float>(m_config.virtualCanvasWidth);
    float scaleY = static_cast<float>(height()) / static_cast<float>(m_config.virtualCanvasHeight);
    float scale = std::min(scaleX, scaleY);

    int dstW = static_cast<int>(m_config.virtualCanvasWidth * scale);
    int dstH = static_cast<int>(m_config.virtualCanvasHeight * scale);
    int dstX = (width() - dstW) / 2;
    int dstY = (height() - dstH) / 2;

    m_lastViewportRect = QRect(dstX, dstY, dstW, dstH);

    // Disable smoothing on final blit for crisp, uniform pixel edges
    widgetPainter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    widgetPainter.drawImage(m_lastViewportRect, m_canvasBuffer, m_canvasBuffer.rect());
}

void GameWidget::renderScene(QPainter& painter) {
    Physics::Vec2 cam = m_engine.cameraPosition();

    // 1. Background sky with parallax
    paintBackground(painter, cam.x);

    // 2. World Coordinate Transform: center vehicle at ~35% of canvas width and 60% height
    painter.save();
    float viewOriginX = m_config.virtualCanvasWidth * 0.35f;
    float viewOriginY = m_config.virtualCanvasHeight * 0.60f;
    painter.translate(viewOriginX - cam.x, viewOriginY - cam.y);

    // 3. Terrain polygon
    paintTerrain(painter, cam.x, cam.y);

    // 4. Collectibles (Coins & Fuel Cans)
    paintCollectibles(painter);

    // 5. Vehicle (Suspension, Wheels, Chassis, Driver Head)
    paintVehicle(painter);

    painter.restore();

    // 6. Screen Space HUD Overlay
    paintHUD(painter);

    // 7. Win / Loss / Pause Banners
    paintBanners(painter);
}

void GameWidget::paintBackground(QPainter& painter, float camX) {
    if (m_sprites.hasAsset(AssetId::Background_Sky)) {
        const QPixmap& sky = m_sprites.getPixmap(AssetId::Background_Sky);
        int skyW = sky.width();

        // Parallax scroll factor (sky moves at 15% vehicle speed)
        float parallaxX = std::fmod(camX * 0.15f, static_cast<float>(skyW));
        if (parallaxX < 0.0f) parallaxX += skyW;

        int startX = -static_cast<int>(parallaxX);
        while (startX > 0) startX -= skyW;

        for (int x = startX; x < m_config.virtualCanvasWidth; x += skyW) {
            painter.drawPixmap(x, 0, sky);
        }
    } else {
        drawFallbackSky(painter);
    }
}

void GameWidget::drawFallbackSky(QPainter& painter) {
    // Rich procedural atmospheric sunset gradient
    QLinearGradient grad(0, 0, 0, m_config.virtualCanvasHeight);
    grad.setColorAt(0.0, QColor(30, 58, 138));   // Deep twilight blue
    grad.setColorAt(0.65, QColor(56, 189, 248)); // Bright sky blue
    grad.setColorAt(1.0, QColor(254, 240, 138)); // Warm golden horizon
    painter.fillRect(0, 0, m_config.virtualCanvasWidth, m_config.virtualCanvasHeight, grad);

    // Seamless distant mountain silhouette using exact 960px periodic harmonics
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(71, 85, 105, 180));
    QPainterPath mountainPath;
    mountainPath.moveTo(0, 540);
    mountainPath.lineTo(0, 360);
    float omega0 = 2.0f * 3.14159265f / 960.0f;
    for (int x = 0; x <= 960; x += 10) {
        float my = 350.0f + std::sin(1.0f * omega0 * x + 0.4f) * 45.0f + std::sin(2.0f * omega0 * x + 1.8f) * 25.0f;
        mountainPath.lineTo(x, my);
    }
    mountainPath.lineTo(960, 540);
    mountainPath.closeSubpath();
    painter.drawPath(mountainPath);
}

void GameWidget::paintTerrain(QPainter& painter, float camX, float /*camY*/) {
    float viewLeft = camX - (m_config.virtualCanvasWidth * 0.45f);
    float viewRight = camX + (m_config.virtualCanvasWidth * 0.70f);
    float bottomY = 1200.0f; // Deep subterranean floor

    Terrain::TerrainSlice slice = m_engine.terrain().getVisibleSlice(viewLeft, viewRight, bottomY);
    if (slice.surfacePoints.size() < 2) return;

    // Build subterranean ground polygon
    QPolygonF poly;
    poly.reserve(static_cast<int>(slice.surfacePoints.size()) + 2);

    for (const auto& pt : slice.surfacePoints) {
        poly.append(QPointF(pt.x, pt.y));
    }
    poly.append(QPointF(slice.endX, bottomY));
    poly.append(QPointF(slice.startX, bottomY));

    // Fill subterranean dirt
    painter.save();
    if (m_sprites.hasAsset(AssetId::Terrain_DirtFill)) {
        const QPixmap& dirt = m_sprites.getPixmap(AssetId::Terrain_DirtFill);
        painter.setBrushOrigin(0, 0);
        painter.setBrush(QBrush(dirt));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(poly);
    } else {
        // Fallback procedural earth fill
        QLinearGradient dirtGrad(0, 200, 0, 900);
        dirtGrad.setColorAt(0.0, QColor(120, 53, 15));
        dirtGrad.setColorAt(0.7, QColor(69, 26, 3));
        painter.setBrush(dirtGrad);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(poly);
    }

    // Paint grass top trim ribbon connected end-to-end along the landscape
    if (m_sprites.hasAsset(AssetId::Terrain_GrassTop)) {
        const QPixmap& grass = m_sprites.getPixmap(AssetId::Terrain_GrassTop);
        const auto& tiles = m_engine.terrain().grassTiles();

        for (const auto& tile : tiles) {
            // Strict viewport culling: only render tiles within visible camera window + safe margin
            if (tile.p2.x < viewLeft - 60.0f) continue;
            if (tile.p1.x > viewRight + 60.0f) break; // Sorted by X, so break early!

            painter.save();
            painter.translate(tile.p1.x, tile.p1.y);
            painter.rotate(tile.angleDeg);
            // Draw tile aligned with exact secant slope, top edge hugging terrain surface
            painter.drawPixmap(0, -6, grass);
            painter.restore();
        }
    } else {
        // Procedural lush grass ribbon
        QPen grassPen(QColor(34, 197, 94), 7.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(grassPen);
        painter.setBrush(Qt::NoBrush);

        QPainterPath path;
        path.moveTo(slice.surfacePoints.front().x, slice.surfacePoints.front().y);
        for (size_t i = 1; i < slice.surfacePoints.size(); ++i) {
            path.lineTo(slice.surfacePoints[i].x, slice.surfacePoints[i].y);
        }
        painter.drawPath(path);

        // Highlight line on top
        QPen highlightPen(QColor(74, 222, 128), 2.5f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(highlightPen);
        painter.drawPath(path);
    }

    // Draw Finish Line Banner at track finish
    float finishX = m_engine.finishLineX();
    float finishY = m_engine.terrain().getHeightAt(finishX);
    painter.setPen(QPen(Qt::white, 4.0f));
    painter.drawLine(QPointF(finishX, finishY), QPointF(finishX, finishY - 90.0f));

    // Checkered finish flag
    int flagW = 36;
    int flagH = 24;
    for (int fy = 0; fy < 3; ++fy) {
        for (int fx = 0; fx < 3; ++fx) {
            QColor blockColor = ((fx + fy) % 2 == 0) ? Qt::white : Qt::black;
            painter.fillRect(static_cast<int>(finishX) + fx * 12, static_cast<int>(finishY - 90.0f) + fy * 8, 12, 8, blockColor);
        }
    }

    painter.restore();
}

void GameWidget::paintCollectibles(QPainter& painter) {
    const auto& items = m_engine.collectibles();

    for (const auto& item : items) {
        if (item.collected) continue;

        float bobOffset = std::sin(item.bobTimer) * 4.0f;
        float drawX = item.position.x;
        float drawY = item.position.y + bobOffset;

        if (item.type == Core::CollectibleType::Coin) {
            if (m_sprites.hasAsset(AssetId::Collectible_Coin)) {
                const QPixmap& coin = m_sprites.getPixmap(AssetId::Collectible_Coin);
                painter.drawPixmap(static_cast<int>(drawX - coin.width() / 2),
                                   static_cast<int>(drawY - coin.height() / 2),
                                   coin);
            } else {
                drawFallbackCoin(painter, drawX, drawY);
            }
        } else if (item.type == Core::CollectibleType::FuelCan) {
            if (m_sprites.hasAsset(AssetId::Collectible_FuelCan)) {
                const QPixmap& fuel = m_sprites.getPixmap(AssetId::Collectible_FuelCan);
                painter.drawPixmap(static_cast<int>(drawX - fuel.width() / 2),
                                   static_cast<int>(drawY - fuel.height() / 2),
                                   fuel);
            } else {
                drawFallbackFuelCan(painter, drawX, drawY);
            }
        }
    }
}

void GameWidget::paintVehicle(QPainter& painter) {
    Vehicle::CarRenderData data = m_engine.car().getRenderData();

    painter.save();

    // 1. Suspension Arms / Struts (drawn behind wheels and chassis)
    painter.setPen(QPen(QColor(51, 65, 85), 4.0f, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(data.chassis.position.x, data.chassis.position.y),
                     QPointF(data.rearWheel.position.x, data.rearWheel.position.y));
    painter.drawLine(QPointF(data.chassis.position.x, data.chassis.position.y),
                     QPointF(data.frontWheel.position.x, data.frontWheel.position.y));

    // Spring coil accents
    painter.setPen(QPen(QColor(220, 38, 38), 2.5f));
    QPointF rearMid = (QPointF(data.chassis.position.x, data.chassis.position.y) +
                       QPointF(data.rearWheel.position.x, data.rearWheel.position.y)) * 0.5;
    painter.drawEllipse(rearMid, 4.0, 4.0);

    QPointF frontMid = (QPointF(data.chassis.position.x, data.chassis.position.y) +
                        QPointF(data.frontWheel.position.x, data.frontWheel.position.y)) * 0.5;
    painter.drawEllipse(frontMid, 4.0, 4.0);

    // 2. Rear Wheel
    painter.save();
    painter.translate(data.rearWheel.position.x, data.rearWheel.position.y);
    painter.rotate(data.rearWheel.angleDegrees);
    if (m_sprites.hasAsset(AssetId::Vehicle_Wheel)) {
        const QPixmap& wheel = m_sprites.getPixmap(AssetId::Vehicle_Wheel);
        painter.drawPixmap(-wheel.width() / 2, -wheel.height() / 2, wheel);
    } else {
        drawFallbackWheel(painter);
    }
    painter.restore();

    // 3. Front Wheel
    painter.save();
    painter.translate(data.frontWheel.position.x, data.frontWheel.position.y);
    painter.rotate(data.frontWheel.angleDegrees);
    if (m_sprites.hasAsset(AssetId::Vehicle_Wheel)) {
        const QPixmap& wheel = m_sprites.getPixmap(AssetId::Vehicle_Wheel);
        painter.drawPixmap(-wheel.width() / 2, -wheel.height() / 2, wheel);
    } else {
        drawFallbackWheel(painter);
    }
    painter.restore();

    // 4. Chassis Body
    painter.save();
    painter.translate(data.chassis.position.x, data.chassis.position.y);
    painter.rotate(data.chassis.angleDegrees);
    if (m_sprites.hasAsset(AssetId::Vehicle_Chassis)) {
        const QPixmap& chassis = m_sprites.getPixmap(AssetId::Vehicle_Chassis);
        painter.drawPixmap(-chassis.width() / 2, -chassis.height() / 2, chassis);
    } else {
        drawFallbackChassis(painter);
    }
    painter.restore();

    // 5. Driver Head with Helmet & Goggles
    painter.save();
    painter.translate(data.driverHead.position.x, data.driverHead.position.y);
    // Add slight bobbing / neck inertia rotation
    painter.rotate(data.driverHead.angleDegrees);
    if (m_sprites.hasAsset(AssetId::Vehicle_DriverHead)) {
        const QPixmap& head = m_sprites.getPixmap(AssetId::Vehicle_DriverHead);
        painter.drawPixmap(-head.width() / 2, -head.height() / 2, head);
    } else {
        drawFallbackDriverHead(painter);
    }
    painter.restore();

    painter.restore();
}

void GameWidget::paintHUD(QPainter& painter) {
    painter.save();

    QFont hudFont("Segoe UI", 11, QFont::Bold);
    painter.setFont(hudFont);

    // ========================================================================
    // 1. FUEL GAUGE (Top-Left)
    // ========================================================================
    int fuelX = m_config.hudMargin;
    int fuelY = m_config.hudMargin;
    int barW = m_config.fuelBarWidth;
    int barH = m_config.fuelBarHeight;

    // Fuel icon / label
    painter.setPen(Qt::white);
    painter.drawText(fuelX, fuelY + 15, QStringLiteral("FUEL"));

    // Gauge frame
    int gaugeStartX = fuelX + 50;
    painter.setPen(QPen(QColor(15, 23, 42), 2));
    painter.setBrush(QColor(30, 41, 59, 210));
    painter.drawRoundedRect(gaugeStartX, fuelY, barW, barH, 4, 4);

    // Filled bar
    float fuelRatio = m_engine.car().fuelRatio();
    int fillW = static_cast<int>((barW - 4) * fuelRatio);

    if (fillW > 0) {
        QColor barColor(34, 197, 94); // Green
        if (fuelRatio < 0.25f) {
            // Flashing warning red when critical
            bool flash = (QTime::currentTime().msec() / 250) % 2 == 0;
            barColor = flash ? QColor(239, 68, 68) : QColor(185, 28, 28);
        } else if (fuelRatio < 0.50f) {
            barColor = QColor(234, 179, 8); // Yellow
        }
        painter.fillRect(gaugeStartX + 2, fuelY + 2, fillW, barH - 4, barColor);
    }

    // ========================================================================
    // 2. DISTANCE TRACKER & PROGRESS BAR (Top-Center)
    // ========================================================================
    int centerBarW = 240;
    int centerBarX = (m_config.virtualCanvasWidth - centerBarW) / 2;
    int centerBarY = m_config.hudMargin;

    float currentMeters = m_engine.car().distanceMeters();
    float targetMeters = m_engine.targetDistanceMeters();
    float progressRatio = std::min(1.0f, currentMeters / targetMeters);

    painter.setPen(Qt::white);
    painter.drawText(centerBarX, centerBarY + 15,
                     QStringLiteral("%1m / %2m").arg(static_cast<int>(currentMeters)).arg(static_cast<int>(targetMeters)));

    int distTrackX = centerBarX + 105;
    painter.setPen(QPen(QColor(15, 23, 42), 2));
    painter.setBrush(QColor(30, 41, 59, 210));
    painter.drawRoundedRect(distTrackX, centerBarY, 130, barH, 4, 4);

    int progressW = static_cast<int>(126 * progressRatio);
    if (progressW > 0) {
        painter.fillRect(distTrackX + 2, centerBarY + 2, progressW, barH - 4, QColor(56, 189, 248));
    }

    // ========================================================================
    // 3. COINS & SCORE COUNTER (Top-Right)
    // ========================================================================
    int coinX = m_config.virtualCanvasWidth - 210;
    int coinY = m_config.hudMargin;

    // Small coin icon
    painter.setBrush(QColor(234, 179, 8));
    painter.setPen(QPen(QColor(161, 98, 7), 2));
    painter.drawEllipse(coinX, coinY + 1, 16, 16);

    painter.setPen(Qt::white);
    painter.drawText(coinX + 24, coinY + 15,
                     QStringLiteral("COINS: %1  SCORE: %2").arg(m_engine.coinsCollected()).arg(m_engine.score()));

    // ========================================================================
    // 4. SPEEDOMETER GAUGE DIAL (Bottom-Right corner)
    // ========================================================================
    int dialX = m_config.virtualCanvasWidth - 90;
    int dialY = m_config.virtualCanvasHeight - 160;
    float speedKmH = m_engine.car().speedKmH();

    if (m_sprites.hasAsset(AssetId::UI_MeterDial)) {
        const QPixmap& dial = m_sprites.getPixmap(AssetId::UI_MeterDial);
        painter.drawPixmap(dialX, dialY, dial);

        if (m_sprites.hasAsset(AssetId::UI_DialNeedle)) {
            const QPixmap& needle = m_sprites.getPixmap(AssetId::UI_DialNeedle);
            painter.save();
            // Dial center at (dialX + 32, dialY + 32)
            painter.translate(dialX + 32, dialY + 32);
            // Map 0..120 km/h to -120 deg .. +120 deg
            float needleAngle = -120.0f + std::min(120.0f, speedKmH) * (240.0f / 120.0f);
            painter.rotate(needleAngle);
            // Needle sprite pivot is at x=4, y=28
            painter.drawPixmap(-4, -28, needle);
            painter.restore();
        }
    } else {
        drawFallbackDial(painter, dialX, dialY, speedKmH);
    }

    // Speed digital readout
    painter.setPen(Qt::white);
    painter.drawText(dialX - 5, dialY + 78, QStringLiteral("%1 KM/H").arg(static_cast<int>(speedKmH)));

    // ========================================================================
    // 5. INTERACTIVE PEDALS (Bottom-Left & Bottom-Right)
    // ========================================================================
    bool gasActive = m_engine.input().isGasActive();
    bool brakeActive = m_engine.input().isBrakeActive();

    // Brake Pedal (Bottom-Left)
    if (brakeActive && m_sprites.hasAsset(AssetId::UI_PedalBrakePressed)) {
        painter.drawPixmap(m_config.pedalBrakeRect.topLeft(), m_sprites.getPixmap(AssetId::UI_PedalBrakePressed));
    } else if (!brakeActive && m_sprites.hasAsset(AssetId::UI_PedalBrakeReleased)) {
        painter.drawPixmap(m_config.pedalBrakeRect.topLeft(), m_sprites.getPixmap(AssetId::UI_PedalBrakeReleased));
    } else {
        drawFallbackPedal(painter, m_config.pedalBrakeRect, false, brakeActive);
    }

    // Gas Pedal (Bottom-Right)
    if (gasActive && m_sprites.hasAsset(AssetId::UI_PedalGasPressed)) {
        painter.drawPixmap(m_config.pedalGasRect.topLeft(), m_sprites.getPixmap(AssetId::UI_PedalGasPressed));
    } else if (!gasActive && m_sprites.hasAsset(AssetId::UI_PedalGasReleased)) {
        painter.drawPixmap(m_config.pedalGasRect.topLeft(), m_sprites.getPixmap(AssetId::UI_PedalGasReleased));
    } else {
        drawFallbackPedal(painter, m_config.pedalGasRect, true, gasActive);
    }

    painter.restore();
}

void GameWidget::paintBanners(QPainter& painter) {
    Core::GameState state = m_engine.state();
    if (state == Core::GameState::Playing) return;

    painter.save();

    // Semi-transparent overlay backdrop
    painter.fillRect(0, 0, m_config.virtualCanvasWidth, m_config.virtualCanvasHeight, QColor(0, 0, 0, 160));

    QRect bannerRect(m_config.virtualCanvasWidth / 2 - 250, m_config.virtualCanvasHeight / 2 - 110, 500, 220);
    painter.setPen(QPen(QColor(255, 255, 255, 80), 3));
    painter.setBrush(QColor(15, 23, 42, 240));
    painter.drawRoundedRect(bannerRect, 12, 12);

    painter.setPen(Qt::white);
    QFont titleFont("Segoe UI", 22, QFont::Bold);
    painter.setFont(titleFont);

    QString title;
    QString subtitle;
    QColor titleColor = Qt::white;

    switch (state) {
        case Core::GameState::GameOver_HeadCrash:
            title = QStringLiteral("DRIVER DOWN!");
            subtitle = QStringLiteral("Neck snap! Drive carefully over steep crests.");
            titleColor = QColor(239, 68, 68);
            break;
        case Core::GameState::GameOver_OutOfFuel:
            title = QStringLiteral("OUT OF FUEL!");
            subtitle = QStringLiteral("Collect red Jerrycans along the hills to keep driving.");
            titleColor = QColor(234, 179, 8);
            break;
        case Core::GameState::LevelWon:
            title = QStringLiteral("STAGE COMPLETED!");
            subtitle = QStringLiteral("Incredible driving! You reached the 1000m finish line!");
            titleColor = QColor(34, 197, 94);
            break;
        case Core::GameState::Paused:
            title = QStringLiteral("GAME PAUSED");
            subtitle = QStringLiteral("Press [ESC] or [P] to Resume.");
            titleColor = QColor(56, 189, 248);
            break;
        default:
            break;
    }

    painter.setPen(titleColor);
    painter.drawText(bannerRect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop, title);

    QFont subFont("Segoe UI", 11);
    painter.setFont(subFont);
    painter.setPen(QColor(203, 213, 225));
    painter.drawText(bannerRect.adjusted(20, 85, -20, 0), Qt::AlignHCenter | Qt::AlignTop, subtitle);

    // Final score / stats
    QString stats = QStringLiteral("Final Distance: %1 m  |  Coins: %2  |  Score: %3")
                        .arg(static_cast<int>(m_engine.car().distanceMeters()))
                        .arg(m_engine.coinsCollected())
                        .arg(m_engine.score());
    painter.setPen(QColor(254, 240, 138));
    painter.drawText(bannerRect.adjusted(0, 125, 0, 0), Qt::AlignHCenter | Qt::AlignTop, stats);

    // Restart instruction button
    QFont promptFont("Segoe UI", 12, QFont::Bold);
    painter.setFont(promptFont);
    painter.setPen(Qt::white);
    painter.drawText(bannerRect.adjusted(0, 165, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                     QStringLiteral("Press [R] to Restart Course"));

    painter.restore();
}

// ============================================================================
// PROCEDURAL VECTOR FALLBACKS
// Ensures 100% playable game even if PNG assets are unavailable.
// ============================================================================

void GameWidget::drawFallbackChassis(QPainter& painter) {
    painter.setPen(Qt::NoPen);

    // Undercarriage dark frame
    painter.setBrush(QColor(30, 35, 42));
    painter.drawRect(-38, 8, 76, 8);

    // Main red body
    painter.setBrush(QColor(220, 38, 38));
    painter.drawRoundedRect(-44, -4, 88, 14, 3, 3);

    // Cabin / roll cage
    painter.setBrush(QColor(185, 28, 28));
    painter.drawRect(-24, -18, 42, 14);

    // Tinted window
    painter.setBrush(QColor(96, 165, 250));
    painter.drawRect(-18, -15, 30, 10);

    // Roll cage bars
    painter.setPen(QPen(QColor(203, 213, 225), 3));
    painter.drawLine(-22, -18, -22, -4);
    painter.drawLine(18, -18, 18, -4);

    // Headlight
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(254, 240, 138));
    painter.drawRect(40, -1, 4, 6);
}

void GameWidget::drawFallbackWheel(QPainter& painter) {
    painter.setPen(Qt::NoPen);

    // Tire rubber
    painter.setBrush(QColor(30, 33, 37));
    painter.drawEllipse(-16, -16, 32, 32);

    // Outer rim
    painter.setBrush(QColor(148, 163, 184));
    painter.drawEllipse(-9, -9, 18, 18);

    // Inner rim
    painter.setBrush(QColor(203, 213, 225));
    painter.drawEllipse(-6, -6, 12, 12);

    // Center hub cap
    painter.setBrush(QColor(30, 41, 59));
    painter.drawEllipse(-3, -3, 6, 6);
}

void GameWidget::drawFallbackDriverHead(QPainter& painter) {
    painter.setPen(Qt::NoPen);

    // Orange helmet
    painter.setBrush(QColor(234, 88, 12));
    painter.drawEllipse(-10, -10, 20, 20);

    // White stripe
    painter.setBrush(Qt::white);
    painter.drawRect(-2, -10, 4, 8);

    // Blue goggles visor
    painter.setBrush(QColor(56, 189, 248));
    painter.drawRoundedRect(-1, -3, 11, 6, 2, 2);

    // Face / skin
    painter.setBrush(QColor(254, 205, 165));
    painter.drawRect(0, 3, 8, 4);
}

void GameWidget::drawFallbackCoin(QPainter& painter, float x, float y) {
    painter.save();
    painter.translate(x, y);

    painter.setPen(QPen(QColor(161, 98, 7), 2));
    painter.setBrush(QColor(250, 204, 21));
    painter.drawEllipse(-10, -10, 20, 20);

    painter.setBrush(QColor(234, 179, 8));
    painter.drawEllipse(-7, -7, 14, 14);

    // Sparkle
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(-5, -5, 3, 3);

    painter.restore();
}

void GameWidget::drawFallbackFuelCan(QPainter& painter, float x, float y) {
    painter.save();
    painter.translate(x, y);

    painter.setPen(QPen(QColor(153, 27, 27), 2));
    painter.setBrush(QColor(220, 38, 38));
    painter.drawRoundedRect(-8, -6, 16, 20, 2, 2);

    // Handle
    painter.setBrush(QColor(153, 27, 27));
    painter.drawRect(-5, -12, 10, 6);
    painter.setBrush(Qt::transparent);
    painter.drawRect(-3, -10, 6, 4);

    // Cap
    painter.setBrush(QColor(71, 85, 105));
    painter.drawRect(-8, -11, 4, 5);

    // White fuel label
    painter.setBrush(Qt::white);
    painter.drawRect(-4, 5, 8, 3);

    painter.restore();
}

void GameWidget::drawFallbackDial(QPainter& painter, int x, int y, float speedKmH) {
    painter.save();
    painter.translate(x + 32, y + 32);

    painter.setPen(QPen(QColor(51, 65, 85), 3));
    painter.setBrush(QColor(15, 23, 42));
    painter.drawEllipse(-30, -30, 60, 60);

    // Needle
    float angle = -120.0f + std::min(120.0f, speedKmH) * (240.0f / 120.0f);
    painter.rotate(angle);
    painter.setPen(QPen(QColor(239, 68, 68), 2.5f));
    painter.drawLine(0, 0, 0, -24);

    painter.restore();
}

void GameWidget::drawFallbackPedal(QPainter& painter, const QRect& rect, bool isGas, bool isPressed) {
    painter.save();

    QColor baseColor = isGas ? QColor(34, 197, 94) : QColor(239, 68, 68);
    if (isPressed) {
        baseColor = isGas ? QColor(74, 222, 128) : QColor(248, 113, 113);
    }

    painter.setPen(QPen(QColor(15, 23, 42), 2));
    painter.setBrush(baseColor);
    painter.drawRoundedRect(rect, 6, 6);

    // Rib lines
    painter.setPen(QPen(QColor(0, 0, 0, 70), 2));
    for (int y = rect.top() + 15; y < rect.bottom() - 10; y += 10) {
        painter.drawLine(rect.left() + 8, y, rect.right() - 8, y);
    }

    // Label
    painter.setFont(QFont("Segoe UI", 8, QFont::Bold));
    painter.setPen(Qt::white);
    painter.drawText(rect.adjusted(0, 0, 0, -5), Qt::AlignHCenter | Qt::AlignBottom, isGas ? "GAS" : "BRAKE");

    painter.restore();
}

// ============================================================================
// INPUT EVENT DISPATCHERS
// ============================================================================

void GameWidget::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }
    m_engine.input().onKeyPressed(event->key());
}

void GameWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }
    m_engine.input().onKeyReleased(event->key());
}

QPoint GameWidget::mapWindowToCanvas(const QPoint& windowPos) const {
    if (m_lastViewportRect.isEmpty()) return QPoint(-1, -1);

    float relX = static_cast<float>(windowPos.x() - m_lastViewportRect.left()) / static_cast<float>(m_lastViewportRect.width());
    float relY = static_cast<float>(windowPos.y() - m_lastViewportRect.top()) / static_cast<float>(m_lastViewportRect.height());

    int cx = static_cast<int>(relX * m_config.virtualCanvasWidth);
    int cy = static_cast<int>(relY * m_config.virtualCanvasHeight);
    return QPoint(cx, cy);
}

void GameWidget::updatePedalTouch(const QPoint& canvasPos, bool pressed) {
    if (m_config.pedalGasRect.contains(canvasPos)) {
        m_engine.input().setPedalGas(pressed);
    } else {
        m_engine.input().setPedalGas(false);
    }

    if (m_config.pedalBrakeRect.contains(canvasPos)) {
        m_engine.input().setPedalBrake(pressed);
    } else {
        m_engine.input().setPedalBrake(false);
    }
}

void GameWidget::mousePressEvent(QMouseEvent* event) {
    QPoint canvasPt = mapWindowToCanvas(event->pos());
    updatePedalTouch(canvasPt, true);
}

void GameWidget::mouseReleaseEvent(QMouseEvent* /*event*/) {
    m_engine.input().setPedalGas(false);
    m_engine.input().setPedalBrake(false);
}

void GameWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        QPoint canvasPt = mapWindowToCanvas(event->pos());
        updatePedalTouch(canvasPt, true);
    }
}

} // namespace Render
