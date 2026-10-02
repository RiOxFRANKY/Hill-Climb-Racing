#include "gamewidget.h"

#include <QApplication>
#include <QFocusEvent>
#include <QImage>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRandomGenerator>

#include <algorithm>
#include <cmath>

namespace {
constexpr double Pi = 3.14159265358979323846;
constexpr double WheelRadius = 40.0;
constexpr double BodyWheelReferenceRadius = 185.0;
constexpr double CarSpriteScale = WheelRadius / BodyWheelReferenceRadius;
constexpr double WheelSpriteTargetRadius = 46.3;
constexpr double RearWheelSourceX = 419.0;
constexpr double FrontWheelSourceX = 1356.0;
constexpr double WheelSourceY = 730.0;
constexpr double WheelOffset = (FrontWheelSourceX - RearWheelSourceX) * 0.5 * CarSpriteScale;
constexpr double WheelLocalY = -43.0;
constexpr double CarSourceCenterX = (RearWheelSourceX + FrontWheelSourceX) * 0.5;

double clampValue(double value, double minimum, double maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

QPointF rotatePoint(const QPointF &point, double angle)
{
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {point.x() * c - point.y() * s,
            point.x() * s + point.y() * c};
}
}

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Hill Climb Qt — Basic Edition"));
    setFixedSize(DesignWidth, DesignHeight);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setCursor(Qt::BlankCursor);

    m_carBody.load(QStringLiteral(":/assets/car_body.png"));
    m_wheelSprite.load(QStringLiteral(":/assets/wheel.png"));

    // The backdrop is not seamless, so it alternates with a mirrored copy; the
    // touching edges then always match while the strip scrolls.
    const QImage background(QStringLiteral(":/assets/background.png"));
    if (!background.isNull()) {
        const QImage scaled = background.scaled(DesignWidth, DesignHeight,
                                                Qt::IgnoreAspectRatio,
                                                Qt::SmoothTransformation);
        m_backgroundStrip = QPixmap(DesignWidth * 2, DesignHeight);
        QPainter stripPainter(&m_backgroundStrip);
        stripPainter.drawImage(0, 0, scaled);
        stripPainter.drawImage(DesignWidth, 0, scaled.flipped(Qt::Horizontal));
    }

    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.start(16);
    m_clock.start();

    resetGame();
}

void GameWidget::resetGame()
{
    m_keys.clear();
    m_pickups.clear();
    resetTerrain();
    m_position = QPointF(360.0, terrainHeight(360.0) + 118.0);
    m_velocity = QPointF(0.0, 0.0);
    m_angle = std::atan(terrainSlope(m_position.x()));
    m_angularVelocity = 0.0;
    m_cameraX = 0.0;
    m_cameraY = 0.0;
    m_fuel = 100.0;
    m_survivalTime = 0.0;
    m_nextPickupX = 720.0;
    m_score = 0;
    m_coins = 0;
    m_grounded = false;
    m_paused = false;
    m_gameOver = false;
    ensurePickupsAhead();
    m_clock.restart();
    update();
}

void GameWidget::tick()
{
    const qint64 elapsedMs = m_clock.restart();
    const double frameTime = clampValue(elapsedMs / 1000.0, 0.0, 0.034);

    if (!m_paused && !m_gameOver) {
        ensureTerrainAhead(std::max(m_position.x(), m_cameraX + DesignWidth) + 1400.0);

        constexpr int substeps = 3;
        for (int i = 0; i < substeps; ++i)
            updatePhysics(frameTime / substeps);

        updatePickups();
        ensurePickupsAhead();
        m_survivalTime += frameTime;

        const double targetCameraX = std::max(0.0, m_position.x() - 560.0);
        const double terrainFocus = terrainHeight(m_position.x()) - 260.0;
        const double targetCameraY = clampValue(terrainFocus, -120.0, 310.0);
        const double follow = 1.0 - std::exp(-frameTime * 4.0);
        m_cameraX += (targetCameraX - m_cameraX) * follow;
        m_cameraY += (targetCameraY - m_cameraY) * follow;
    }

    update();
}

void GameWidget::resetTerrain()
{
    m_randomEngine.seed(QRandomGenerator::global()->generate());
    m_terrainDirection = 0.06;

    // A predictable, nearly flat launch area gives the car time to settle before
    // the randomly generated hills begin.
    m_terrainPoints = {
        QPointF(-900.0, 228.0),
        QPointF(-400.0, 228.0),
        QPointF(0.0, 228.0),
        QPointF(360.0, 230.0),
        QPointF(620.0, 240.0)
    };

    ensureTerrainAhead(DesignWidth + 1400.0);
}

void GameWidget::ensureTerrainAhead(double worldX)
{
    if (m_terrainPoints.isEmpty())
        m_terrainPoints.push_back(QPointF(-900.0, 228.0));

    std::uniform_real_distribution<double> segmentLength(155.0, 245.0);
    std::uniform_real_distribution<double> jitter(-0.065, 0.065);
    std::uniform_real_distribution<double> turnAmount(-0.16, 0.16);
    std::uniform_real_distribution<double> boundaryPush(0.07, 0.14);
    std::bernoulli_distribution changeTrend(0.24);

    while (m_terrainPoints.constLast().x() < worldX) {
        const QPointF previous = m_terrainPoints.constLast();
        const double length = segmentLength(m_randomEngine);

        // This is a constrained random walk rather than unrelated random values.
        // Direction has momentum, which produces recognizable climbs and descents.
        m_terrainDirection += jitter(m_randomEngine);
        if (changeTrend(m_randomEngine))
            m_terrainDirection += turnAmount(m_randomEngine);

        // Gently steer the generator back toward the playable height band.
        if (previous.y() > 390.0)
            m_terrainDirection -= boundaryPush(m_randomEngine);
        else if (previous.y() < 165.0)
            m_terrainDirection += boundaryPush(m_randomEngine);

        m_terrainDirection = clampValue(m_terrainDirection, -0.30, 0.30);
        if (std::abs(m_terrainDirection) < 0.035)
            m_terrainDirection = m_terrainDirection < 0.0 ? -0.035 : 0.035;

        double nextHeight = previous.y() + m_terrainDirection * length;
        const double constrainedHeight = clampValue(nextHeight, 135.0, 430.0);
        if (constrainedHeight != nextHeight)
            m_terrainDirection *= -0.55;
        nextHeight = constrainedHeight;

        m_terrainPoints.push_back(QPointF(previous.x() + length, nextHeight));
    }
}

void GameWidget::updatePhysics(double dt)
{
    const bool accelerate = m_keys.contains(Qt::Key_Right) || m_keys.contains(Qt::Key_D);
    const bool reverse = m_keys.contains(Qt::Key_Left) || m_keys.contains(Qt::Key_A);
    const bool boost = m_keys.contains(Qt::Key_Space);
    const double throttle = (accelerate ? 1.0 : 0.0) - (reverse ? 1.0 : 0.0);

    // Gravity is expressed in world pixels per second squared.
    m_velocity.ry() -= 1320.0 * dt;
    m_grounded = false;

    double accumulatedAngularAcceleration = 0.0;
    for (const double localX : {-WheelOffset, WheelOffset}) {
        const QPointF wheel = wheelPosition(localX);
        const QPointF offset = wheel - m_position;
        const double slope = terrainSlope(wheel.x());
        const double invLength = 1.0 / std::sqrt(1.0 + slope * slope);
        const QPointF normal(-slope * invLength, invLength);
        const QPointF tangent(invLength, slope * invLength);
        const double ground = terrainHeight(wheel.x());
        const double penetration = ground + WheelRadius - wheel.y();

        if (penetration > 0.0) {
            m_grounded = true;
            const QPointF rotationalVelocity(-m_angularVelocity * offset.y(),
                                             m_angularVelocity * offset.x());
            const QPointF pointVelocity = m_velocity + rotationalVelocity;
            const double normalSpeed = QPointF::dotProduct(pointVelocity, normal);
            double suspensionAcceleration = penetration * 72.0 - normalSpeed * 12.0;
            suspensionAcceleration = clampValue(suspensionAcceleration, 0.0, 4700.0);

            const QPointF suspension = normal * (suspensionAcceleration * 0.5);
            m_velocity += suspension * dt;
            accumulatedAngularAcceleration +=
                (offset.x() * suspension.y() - offset.y() * suspension.x()) / 7600.0;

            if (throttle != 0.0 && m_fuel > 0.0) {
                const double enginePower = boost ? 1120.0 : 810.0;
                m_velocity += tangent * (throttle * enginePower * dt * 0.5);
            }
        }
    }

    if (m_grounded) {
        // Rolling drag keeps the car controllable without feeling sticky on jumps.
        m_velocity.rx() *= std::pow(0.994, dt * 60.0);
        m_angularVelocity *= std::pow(0.965, dt * 60.0);
    } else {
        // Pedals also rotate the car in the air, a defining hill-climb mechanic.
        accumulatedAngularAcceleration += throttle * 3.1;
        m_angularVelocity *= std::pow(0.997, dt * 60.0);
    }

    accumulatedAngularAcceleration += throttle * (m_grounded ? 0.8 : 0.0);
    m_angularVelocity += accumulatedAngularAcceleration * dt;
    m_angularVelocity = clampValue(m_angularVelocity, -3.4, 3.4);

    m_velocity.setX(clampValue(m_velocity.x(), -520.0, boost ? 1060.0 : 820.0));
    m_velocity.setY(clampValue(m_velocity.y(), -1150.0, 900.0));
    m_position += m_velocity * dt;
    m_angle += m_angularVelocity * dt;

    // Resolve rare deep chassis contacts so the vehicle cannot tunnel through a hill.
    const QPointF chassisBottom = m_position + rotatePoint(QPointF(0.0, -65.0), m_angle);
    const double bodyGround = terrainHeight(chassisBottom.x());
    if (chassisBottom.y() < bodyGround) {
        const double correction = bodyGround - chassisBottom.y();
        m_position.ry() += correction;
        if (m_velocity.y() < 0.0)
            m_velocity.ry() *= -0.16;
        m_angularVelocity *= 0.72;
    }

    if (throttle != 0.0 && m_fuel > 0.0) {
        m_fuel -= dt * (boost ? 2.25 : 0.82);
        m_fuel = std::max(0.0, m_fuel);
    }

    const double wrappedAngle = std::remainder(m_angle, 2.0 * Pi);
    const bool upsideDown = std::abs(wrappedAngle) > 2.22;
    const bool roofTouching =
        m_position.y() + rotatePoint(QPointF(0.0, 45.0), m_angle).y()
        < terrainHeight(m_position.x()) + 12.0;

    const bool fuelFinished =
        m_fuel <= 0.0 && std::abs(m_velocity.x()) < 16.0 && m_grounded;
    const bool crashDetected =
        (upsideDown && roofTouching) || m_position.y() < -250.0;
    if (fuelFinished || crashDetected) {
        m_gameOver = true;
        m_keys.clear();
    }

    // Do not allow reversing beyond the starting area.
    if (m_position.x() < 250.0) {
        m_position.setX(250.0);
        m_velocity.setX(std::max(0.0, m_velocity.x()));
    }

    m_score = std::max(m_score,
                       static_cast<int>(std::max(0.0, m_position.x() - 360.0) / 4.0)
                           + m_coins * 100);
}

void GameWidget::updatePickups()
{
    for (Pickup &pickup : m_pickups) {
        if (pickup.collected)
            continue;

        const QPointF delta = pickup.position - m_position;
        if (QPointF::dotProduct(delta, delta) < 78.0 * 78.0) {
            pickup.collected = true;
            if (pickup.type == Pickup::Type::Coin) {
                ++m_coins;
            } else {
                m_fuel = std::min(100.0, m_fuel + 38.0);
                m_score += 250;
            }
        }
    }

    while (!m_pickups.isEmpty()
           && (m_pickups.first().collected
               || m_pickups.first().position.x() < m_cameraX - 300.0)) {
        m_pickups.removeFirst();
    }
}

void GameWidget::ensurePickupsAhead()
{
    const double horizon = m_cameraX + DesignWidth + 900.0;
    while (m_nextPickupX < horizon) {
        const int group = static_cast<int>(m_nextPickupX / 420.0);
        const bool placeFuel = group > 0 && group % 7 == 0;

        if (placeFuel) {
            m_pickups.push_back({Pickup::Type::Fuel,
                                 QPointF(m_nextPickupX,
                                         terrainHeight(m_nextPickupX) + 104.0),
                                 false});
            m_nextPickupX += 430.0;
        } else {
            const int count = 3 + (group % 3);
            for (int i = 0; i < count; ++i) {
                const double x = m_nextPickupX + i * 55.0;
                const double arc = std::sin((i + 1.0) / (count + 1.0) * Pi) * 48.0;
                m_pickups.push_back({Pickup::Type::Coin,
                                     QPointF(x, terrainHeight(x) + 112.0 + arc),
                                     false});
            }
            m_nextPickupX += count * 55.0 + 285.0;
        }
    }
}

int GameWidget::terrainSegmentFor(double x) const
{
    if (m_terrainPoints.size() < 2)
        return 0;

    int low = 0;
    int high = static_cast<int>(m_terrainPoints.size()) - 1;
    while (low + 1 < high) {
        const int middle = low + (high - low) / 2;
        if (m_terrainPoints.at(middle).x() <= x)
            low = middle;
        else
            high = middle;
    }

    return std::clamp(low, 0, static_cast<int>(m_terrainPoints.size()) - 2);
}

double GameWidget::terrainHeight(double x) const
{
    if (m_terrainPoints.isEmpty())
        return 228.0;
    if (m_terrainPoints.size() == 1 || x <= m_terrainPoints.constFirst().x())
        return m_terrainPoints.constFirst().y();
    if (x >= m_terrainPoints.constLast().x())
        return m_terrainPoints.constLast().y();

    const int index = terrainSegmentFor(x);
    const QPointF &start = m_terrainPoints.at(index);
    const QPointF &end = m_terrainPoints.at(index + 1);
    const double t = clampValue((x - start.x()) / (end.x() - start.x()), 0.0, 1.0);
    const double smoothT = t * t * (3.0 - 2.0 * t);

    return start.y() + (end.y() - start.y()) * smoothT;
}

double GameWidget::terrainSlope(double x) const
{
    if (m_terrainPoints.size() < 2
        || x <= m_terrainPoints.constFirst().x()
        || x >= m_terrainPoints.constLast().x()) {
        return 0.0;
    }

    const int index = terrainSegmentFor(x);
    const QPointF &start = m_terrainPoints.at(index);
    const QPointF &end = m_terrainPoints.at(index + 1);
    const double length = end.x() - start.x();
    const double t = clampValue((x - start.x()) / length, 0.0, 1.0);
    const double smoothDerivative = 6.0 * t * (1.0 - t);

    return (end.y() - start.y()) / length * smoothDerivative;
}

QPointF GameWidget::wheelPosition(double localX) const
{
    return m_position + rotatePoint(QPointF(localX, WheelLocalY), m_angle);
}

QPointF GameWidget::worldToScreen(const QPointF &world) const
{
    return {world.x() - m_cameraX,
            DesignHeight - (world.y() - m_cameraY)};
}

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    drawBackground(painter);
    drawTerrain(painter);
    drawPickups(painter);
    drawCar(painter);
    drawHud(painter);
    drawOverlay(painter);
}

void GameWidget::drawBackground(QPainter &painter) const
{
    if (m_backgroundStrip.isNull()) {
        QLinearGradient sky(0.0, 0.0, 0.0, DesignHeight);
        sky.setColorAt(0.0, QColor(37, 119, 190));
        sky.setColorAt(0.54, QColor(104, 191, 226));
        sky.setColorAt(1.0, QColor(214, 238, 215));
        painter.fillRect(rect(), sky);
        return;
    }

    const double stripWidth = m_backgroundStrip.width();
    const double shift = std::fmod(m_cameraX * 0.06, stripWidth);
    painter.drawPixmap(QPointF(-shift, 0.0), m_backgroundStrip);
    if (stripWidth - shift < DesignWidth)
        painter.drawPixmap(QPointF(stripWidth - shift, 0.0), m_backgroundStrip);
}

void GameWidget::drawTerrain(QPainter &painter) const
{
    QPainterPath ground;
    const double startX = std::floor(m_cameraX / 12.0) * 12.0 - 24.0;
    ground.moveTo(worldToScreen(QPointF(startX, terrainHeight(startX))));
    for (double x = startX + 12.0; x <= m_cameraX + DesignWidth + 36.0; x += 12.0)
        ground.lineTo(worldToScreen(QPointF(x, terrainHeight(x))));
    ground.lineTo(DesignWidth + 40.0, DesignHeight + 10.0);
    ground.lineTo(-40.0, DesignHeight + 10.0);
    ground.closeSubpath();

    QLinearGradient soil(0.0, 560.0, 0.0, DesignHeight);
    soil.setColorAt(0.0, QColor(125, 90, 52));
    soil.setColorAt(0.3, QColor(101, 70, 44));
    soil.setColorAt(1.0, QColor(52, 45, 38));
    painter.setPen(Qt::NoPen);
    painter.setBrush(soil);
    painter.drawPath(ground);

    QPainterPath grass;
    grass.moveTo(worldToScreen(QPointF(startX, terrainHeight(startX))));
    for (double x = startX + 8.0; x <= m_cameraX + DesignWidth + 32.0; x += 8.0)
        grass.lineTo(worldToScreen(QPointF(x, terrainHeight(x))));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(48, 101, 52), 24.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(grass);
    painter.setPen(QPen(QColor(92, 168, 67), 12.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(grass);
    painter.setPen(QPen(QColor(147, 202, 79), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(grass);

    // Small embedded stones break up the large soil area without image assets.
    painter.setPen(Qt::NoPen);
    for (double x = std::floor(startX / 150.0) * 150.0;
         x < m_cameraX + DesignWidth + 150.0; x += 150.0) {
        const QPointF surface = worldToScreen(QPointF(x, terrainHeight(x)));
        const double wobble = 34.0 + std::fmod(std::abs(x * 0.37), 75.0);
        painter.setBrush(QColor(73, 63, 50, 125));
        painter.drawEllipse(surface + QPointF(25.0, wobble), 8.0, 5.0);
    }
}

void GameWidget::drawPickups(QPainter &painter) const
{
    for (const Pickup &pickup : m_pickups) {
        if (pickup.collected)
            continue;
        const QPointF center = worldToScreen(pickup.position);
        if (center.x() < -100.0 || center.x() > DesignWidth + 100.0)
            continue;

        if (pickup.type == Pickup::Type::Coin) {
            const double pulse = 1.0 + 0.06 * std::sin(m_survivalTime * 5.0 + pickup.position.x());
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

void GameWidget::drawCar(QPainter &painter) const
{
    const QPointF bodyCenter = worldToScreen(m_position);
    painter.save();
    painter.translate(bodyCenter);
    painter.rotate(-m_angle * 180.0 / Pi);

    if (!m_carBody.isNull() && !m_wheelSprite.isNull()) {
        const double wheelSpinDegrees =
            std::remainder(m_position.x() / WheelRadius * 180.0 / Pi, 360.0);

        const auto drawAnimatedTyre = [&](double localX) {
            painter.save();
            painter.translate(localX, -WheelLocalY);
            painter.rotate(wheelSpinDegrees);
            painter.drawPixmap(
                QRectF(-WheelSpriteTargetRadius, -WheelSpriteTargetRadius,
                       WheelSpriteTargetRadius * 2.0,
                       WheelSpriteTargetRadius * 2.0),
                m_wheelSprite,
                QRectF(m_wheelSprite.rect()));
            painter.restore();
        };

        // Tyres are rendered first, then the wheel-free body covers the portions
        // that belong behind the fenders and suspension.
        drawAnimatedTyre(-WheelOffset);
        drawAnimatedTyre(WheelOffset);

        const double spriteLeft = -CarSourceCenterX * CarSpriteScale;
        const double spriteTop = -WheelLocalY - WheelSourceY * CarSpriteScale;
        painter.drawPixmap(
            QRectF(spriteLeft, spriteTop,
                   m_carBody.width() * CarSpriteScale,
                   m_carBody.height() * CarSpriteScale),
            m_carBody,
            QRectF(m_carBody.rect()));
    }

    painter.restore();
}

void GameWidget::drawHud(QPainter &painter) const
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
    const int distance = static_cast<int>(std::max(0.0, m_position.x() - 360.0) / 10.0);
    painter.drawText(QPointF(76.0, 125.0), QStringLiteral("%1 m").arg(distance));
    painter.setPen(QColor(255, 211, 55));
    painter.drawText(QPointF(300.0, 125.0), QString::number(m_coins));
    painter.setPen(Qt::white);
    painter.drawText(QPointF(425.0, 125.0), QString::number(m_score));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 15, QFont::DemiBold));
    painter.drawText(QPointF(76.0, 170.0), QStringLiteral("FUEL"));
    painter.setBrush(QColor(255, 255, 255, 45));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(145.0, 147.0, 374.0, 25.0), 12.0, 12.0);

    const QColor fuelColor = m_fuel > 28.0 ? QColor(79, 211, 105) : QColor(245, 74, 65);
    QLinearGradient fuelGradient(145.0, 0.0, 519.0, 0.0);
    fuelGradient.setColorAt(0.0, fuelColor.darker(115));
    fuelGradient.setColorAt(1.0, fuelColor.lighter(120));
    painter.setBrush(fuelGradient);
    painter.drawRoundedRect(QRectF(145.0, 147.0, 374.0 * (m_fuel / 100.0), 25.0), 12.0, 12.0);

    const bool leftActive = m_keys.contains(Qt::Key_Left) || m_keys.contains(Qt::Key_A);
    const bool rightActive = m_keys.contains(Qt::Key_Right) || m_keys.contains(Qt::Key_D);
    const bool boostActive = m_keys.contains(Qt::Key_Space);
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
    painter.restore();
}

void GameWidget::drawKeyHint(QPainter &painter, const QRectF &rect,
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

void GameWidget::drawOverlay(QPainter &painter) const
{
    if (!m_paused && !m_gameOver)
        return;

    painter.fillRect(rect(), QColor(5, 13, 22, 150));
    const QRectF card(610.0, 320.0, 700.0, 390.0);
    painter.setPen(QPen(QColor(255, 255, 255, 55), 3.0));
    painter.setBrush(QColor(14, 31, 45, 235));
    painter.drawRoundedRect(card, 35.0, 35.0);

    painter.setPen(Qt::white);
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 54, QFont::Black));
    painter.drawText(QRectF(650.0, 365.0, 620.0, 90.0), Qt::AlignCenter,
                     m_gameOver ? QStringLiteral("RUN OVER") : QStringLiteral("PAUSED"));

    const int distance = static_cast<int>(std::max(0.0, m_position.x() - 360.0) / 10.0);
    painter.setPen(QColor(200, 222, 232));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 24, QFont::DemiBold));
    painter.drawText(QRectF(650.0, 475.0, 620.0, 55.0), Qt::AlignCenter,
                     QStringLiteral("%1 m     •     %2 coins     •     %3 points")
                         .arg(distance).arg(m_coins).arg(m_score));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(225, 70, 47));
    painter.drawRoundedRect(QRectF(810.0, 565.0, 300.0, 74.0), 18.0, 18.0);
    painter.setPen(Qt::white);
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Black));
    painter.drawText(QRectF(810.0, 565.0, 300.0, 74.0), Qt::AlignCenter,
                     m_gameOver ? QStringLiteral("R  TRY AGAIN") : QStringLiteral("P  RESUME"));
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (event->key() == Qt::Key_R) {
        resetGame();
    } else if (event->key() == Qt::Key_P && !m_gameOver) {
        m_paused = !m_paused;
        m_keys.clear();
        m_clock.restart();
    } else if (!m_paused && !m_gameOver) {
        m_keys.insert(event->key());
    }
    event->accept();
}

void GameWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
        m_keys.remove(event->key());
    event->accept();
}

void GameWidget::focusOutEvent(QFocusEvent *event)
{
    m_keys.clear();
    QWidget::focusOutEvent(event);
}
