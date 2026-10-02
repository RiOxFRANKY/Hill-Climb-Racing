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
#include <QVarLengthArray>

#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

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

// Vehicle physics. Lengths are world pixels (roughly 90 px per metre), time is
// seconds and masses are relative to the chassis.
constexpr double Gravity = 1100.0;
constexpr int PhysicsSubsteps = 16;
constexpr double ChassisMass = 1.0;
constexpr double ChassisInertia = ChassisMass * 70.0 * 70.0;
constexpr double WheelMass = 0.15;
constexpr double WheelInertia = 0.5 * WheelMass * WheelRadius * WheelRadius;

// Suspension travel is measured along the chassis "down" axis from the wheel's
// position in the artwork. The spring rest point sits below it by the static
// sag, so a car at rest settles with its wheels exactly where they are drawn.
constexpr double SuspensionSag = 10.0;
constexpr double SuspensionStiffness = ChassisMass * Gravity * 0.5 / SuspensionSag;
constexpr double SuspensionDamping = 4.5;
constexpr double SuspensionMinExtension = -14.0;
constexpr double SuspensionMaxExtension = 20.0;

constexpr double DriveTorque = 20000.0;
constexpr double BoostDriveTorque = 27000.0;
constexpr double MaxWheelSpin = 22.0;
constexpr double BoostWheelSpin = 28.0;
constexpr double ReverseTorque = 12000.0;
constexpr double MaxReverseSpin = 9.0;
constexpr double BrakeTorque = 32000.0;
constexpr double RollingResistance = 0.35;
constexpr double AirControl = 3.6;
constexpr double MaxChassisSpin = 7.0;

constexpr double TyreStaticFriction = 1.25;
constexpr double TyreDynamicFriction = 1.0;
constexpr double BodyStaticFriction = 0.6;
constexpr double BodyDynamicFriction = 0.45;

// Collision circles approximating the car body artwork (chassis-local, y up).
struct BodyCollider {
    double x;
    double y;
    double radius;
    bool head;
};

constexpr std::array<BodyCollider, 8> ChassisColliders{{
    {-170.0, -5.0, 16.0, false},  // spare tyre
    {-138.0, -30.0, 10.0, false}, // rear bumper
    {150.0, -12.0, 13.0, false},  // front bumper
    {120.0, 26.0, 9.0, false},    // bonnet
    {-5.0, -32.0, 10.0, false},   // floor pan
    {-92.0, 80.0, 9.0, false},    // roll cage
    {24.0, 78.0, 9.0, false},     // windscreen frame
    {-38.0, 68.0, 15.0, true},    // driver's head
}};

constexpr double MinTerrainHeight = 150.0;
constexpr double MaxTerrainHeight = 820.0;
constexpr double GapFloor = -2000.0;
// Between two flat-tangent nodes a Hermite segment peaks at 1.5x its average
// slope. Capping climbs at 0.48 keeps every uphill below ~0.72, which the engine
// can still crawl up from a standstill, so no trough becomes a dead end.
constexpr double MaxClimbSecant = 0.48;
constexpr double MaxDropSecant = 0.85;
// Limits how sharply a descent may turn into a climb. A V narrower than the car
// would wedge its bumper into the far slope, so steep drops get a run-out.
constexpr double MaxTroughBend = 0.9;
constexpr double GrassThickness = 24.0;
constexpr int SoilTileSize = 1024;

enum TerrainFeature { Rolling, Waves, Bumps, Valley, Steps, Gap, FeatureCount };

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

double cross(const QPointF &a, const QPointF &b)
{
    return a.x() * b.y() - a.y() * b.x();
}

double length(const QPointF &v)
{
    return std::sqrt(QPointF::dotProduct(v, v));
}

QPointF pointVelocity(const RigidBody &body, const QPointF &arm)
{
    return body.velocity + QPointF(-body.angularVelocity * arm.y(),
                                   body.angularVelocity * arm.x());
}

// How hard a body resists being pushed along `direction` at offset `arm`.
double generalizedInverseMass(const RigidBody &body, const QPointF &arm, const QPointF &direction)
{
    const double turn = cross(arm, direction);
    return body.inverseMass + body.inverseInertia * turn * turn;
}

// XPBD positional correction: moves the point `armA` of `a` by `delta` relative
// to the point `armB` of `b` (or the static world when `b` is null), sharing the
// motion by inverse mass. Returns the Lagrange multiplier (impulse * h).
double applyCorrection(RigidBody &a, RigidBody *b, const QPointF &armA, const QPointF &armB,
                       const QPointF &delta, double compliance, double h)
{
    const double magnitude = length(delta);
    if (magnitude < 1e-9)
        return 0.0;

    const QPointF direction = delta / magnitude;
    double weight = generalizedInverseMass(a, armA, direction);
    if (b)
        weight += generalizedInverseMass(*b, armB, direction);
    const double lambda = magnitude / (weight + compliance / (h * h));
    const QPointF impulse = direction * lambda;

    a.position += impulse * a.inverseMass;
    a.angle += a.inverseInertia * cross(armA, impulse);
    if (b) {
        b->position -= impulse * b->inverseMass;
        b->angle -= b->inverseInertia * cross(armB, impulse);
    }
    return lambda;
}

void applyImpulse(RigidBody &body, const QPointF &arm, const QPointF &impulse)
{
    body.velocity += impulse * body.inverseMass;
    body.angularVelocity += body.inverseInertia * cross(arm, impulse);
}

// Stable per-position randomness for decorations, so they never flicker.
quint32 hashValue(qint64 value)
{
    quint64 x = static_cast<quint64>(value) + 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return static_cast<quint32>(x ^ (x >> 31));
}

double hashUnit(qint64 value)
{
    return hashValue(value) / 4294967295.0;
}

void drawRock(QPainter &painter, const QPointF &center, double radius, quint32 seed)
{
    quint32 state = seed | 1u;
    const auto next = [&state] {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state / 4294967295.0;
    };

    const int corners = 7 + static_cast<int>(next() * 3.0);
    const double turn = next() * 2.0 * Pi;
    QPolygonF shape;
    for (int i = 0; i < corners; ++i) {
        const double angle = turn + (i + (next() - 0.5) * 0.5) * 2.0 * Pi / corners;
        const double distance = radius * (0.78 + 0.22 * next());
        shape << center + QPointF(std::cos(angle) * distance * 1.12,
                                  std::sin(angle) * distance * 0.9);
    }

    const int shade = static_cast<int>(next() * 26.0) - 13;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(112 + shade, 98 + shade, 88 + shade));
    painter.drawPolygon(shape);

    // A lighter upper-left facet gives the chunky, cel-shaded stone look.
    QPolygonF facet;
    for (const QPointF &corner : shape)
        facet << center + (corner - center) * 0.58 + QPointF(-radius * 0.16, -radius * 0.2);
    painter.setBrush(QColor(150 + shade, 136 + shade, 122 + shade, 210));
    painter.drawPolygon(facet);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(52, 38, 28), std::max(1.6, radius * 0.09),
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolygon(shape);
}

// Jagged rock face running from a ledge down past the bottom of the screen.
// `inward` is +1 when the solid ground lies to the right of the wall.
QVector<QPointF> cliffWall(const QPointF &top, double bottomY, int inward, quint32 seed)
{
    QVector<QPointF> points{top};
    int step = 1;
    for (double y = top.y() + 34.0; y < bottomY + 34.0; y += 34.0, ++step) {
        const double depth = y - top.y();
        const double jag = 4.0 + 14.0 * hashUnit(seed + step);
        points << QPointF(top.x() - inward * depth * 0.1 + inward * jag, y);
    }
    return points;
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
    buildSoilTexture();

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
    m_chassis = RigidBody{};
    m_chassis.position = QPointF(360.0, surfaceHeight(360.0) + WheelRadius - WheelLocalY + 4.0);
    m_chassis.inverseMass = 1.0 / ChassisMass;
    m_chassis.inverseInertia = 1.0 / ChassisInertia;
    for (int i = 0; i < 2; ++i) {
        RigidBody &wheel = m_wheels[i];
        wheel = RigidBody{};
        wheel.position = m_chassis.position + QPointF(i == 0 ? -WheelOffset : WheelOffset, WheelLocalY);
        wheel.inverseMass = 1.0 / WheelMass;
        wheel.inverseInertia = 1.0 / WheelInertia;
    }
    m_wheelContacts = 0;
    m_headHit = false;
    m_cameraX = 0.0;
    m_cameraY = 0.0;
    m_fuel = 100.0;
    m_survivalTime = 0.0;
    m_nextPickupX = 720.0;
    m_score = 0;
    m_coins = 0;
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
        ensureTerrainAhead(std::max(m_chassis.position.x(), m_cameraX + DesignWidth) + 1400.0);
        updatePhysics(frameTime);

        updatePickups();
        ensurePickupsAhead();
        m_survivalTime += frameTime;

        const double targetCameraX = std::max(0.0, m_chassis.position.x() - 560.0);

        // Frame the ground under the car, but drop the view far enough that
        // valleys coming up ahead are visible before the car plunges in.
        double lowestAhead = surfaceHeight(m_chassis.position.x());
        for (double x = m_chassis.position.x() - 300.0; x <= m_chassis.position.x() + 1100.0; x += 50.0)
            lowestAhead = std::min(lowestAhead, surfaceHeight(x));
        double targetCameraY = std::min(surfaceHeight(m_chassis.position.x()) - 260.0,
                                        lowestAhead - 70.0);
        targetCameraY = std::max(targetCameraY, m_chassis.position.y() - 820.0);
        const double follow = 1.0 - std::exp(-frameTime * 4.0);
        m_cameraX += (targetCameraX - m_cameraX) * follow;
        m_cameraY += (targetCameraY - m_cameraY) * follow;
    }

    update();
}

void GameWidget::resetTerrain()
{
    m_randomEngine.seed(QRandomGenerator::global()->generate());
    m_terrainNodes.clear();
    m_gaps.clear();
    m_lastFeature = -1;

    // A predictable, nearly flat launch area gives the car time to settle before
    // the generated features begin.
    appendTerrainNode(-900.0, 228.0);
    appendTerrainNode(-400.0, 228.0);
    appendTerrainNode(0.0, 228.0);
    appendTerrainNode(360.0, 230.0);
    appendTerrainNode(620.0, 240.0);

    ensureTerrainAhead(DesignWidth + 1400.0);
}

void GameWidget::ensureTerrainAhead(double worldX)
{
    if (m_terrainNodes.isEmpty())
        appendTerrainNode(-900.0, 228.0);

    while (m_terrainNodes.constLast().x < worldX)
        appendTerrainFeature();
}

double GameWidget::appendTerrainNode(double x, double y, std::optional<double> tangent)
{
    if (!m_terrainNodes.isEmpty()) {
        const TerrainNode &previous = m_terrainNodes.constLast();
        const double run = x - previous.x;
        double maxClimb = MaxClimbSecant;
        if (m_terrainNodes.size() >= 2) {
            const TerrainNode &before = m_terrainNodes.at(m_terrainNodes.size() - 2);
            const double incoming = (previous.y - before.y) / (previous.x - before.x);
            if (incoming < 0.0)
                maxClimb = std::min(maxClimb, std::max(0.0, MaxTroughBend + incoming));
        }
        y = clampValue(y, previous.y - MaxDropSecant * run, previous.y + maxClimb * run);
    }
    m_terrainNodes.push_back({x, y, tangent.value_or(0.0), tangent.has_value()});

    // The previous node now has neighbours on both sides, so its tangent can be
    // settled. Fritsch-Butland weighting keeps the curve monotone between nodes:
    // peaks and troughs stay rounded and steps never overshoot.
    const int count = static_cast<int>(m_terrainNodes.size());
    if (count < 3)
        return y;

    TerrainNode &middle = m_terrainNodes[count - 2];
    if (middle.fixedTangent)
        return y;

    const TerrainNode &before = m_terrainNodes.at(count - 3);
    const TerrainNode &after = m_terrainNodes.at(count - 1);
    const double h0 = middle.x - before.x;
    const double h1 = after.x - middle.x;
    const double d0 = (middle.y - before.y) / h0;
    const double d1 = (after.y - middle.y) / h1;
    if (d0 * d1 <= 0.0) {
        middle.tangent = 0.0;
    } else {
        const double w0 = 2.0 * h1 + h0;
        const double w1 = h1 + 2.0 * h0;
        middle.tangent = (w0 + w1) / (w0 / d0 + w1 / d1);
    }
    return y;
}

void GameWidget::appendTerrainFeature()
{
    const TerrainNode last = m_terrainNodes.constLast();
    const double difficulty = clampValue(last.x / 30000.0, 0.0, 1.0);
    const double scale = 0.9 + 0.4 * difficulty;
    const double middleHeight = (MinTerrainHeight + MaxTerrainHeight) * 0.5;

    const auto uniform = [this](double minimum, double maximum) {
        return std::uniform_real_distribution<double>(minimum, maximum)(m_randomEngine);
    };
    const auto keepInBand = [](double y) {
        return clampValue(y, MinTerrainHeight, MaxTerrainHeight);
    };

    std::array<double, FeatureCount> weights{};
    weights[Rolling] = 2.0;
    weights[Waves] = 3.0;
    weights[Bumps] = 2.0;
    weights[Valley] = last.y - MinTerrainHeight > 250.0 ? 2.5 : 0.0;
    weights[Steps] = 2.0;
    weights[Gap] = last.x > 2600.0 ? 1.2 + difficulty : 0.0;
    if (m_lastFeature >= 0)
        weights[m_lastFeature] *= 0.25;
    if (m_lastFeature == Gap)
        weights[Gap] = 0.0;

    std::discrete_distribution<int> pick(weights.begin(), weights.end());
    const int feature = pick(m_randomEngine);
    m_lastFeature = feature;

    double x = last.x;
    double y = last.y;

    switch (feature) {
    case Rolling: {
        const int count = 2 + static_cast<int>(uniform(0.0, 3.0));
        for (int i = 0; i < count; ++i) {
            x += uniform(280.0, 420.0);
            y = appendTerrainNode(
                x, keepInBand(y + uniform(-160.0, 160.0) * scale + (middleHeight - y) * 0.2));
        }
        break;
    }
    case Waves: {
        // A chain of rounded hills, optionally growing taller as they go.
        const int hills = 3 + static_cast<int>(uniform(0.0, 3.0));
        const double trend = (y < middleHeight ? 1.0 : -1.0) * uniform(20.0, 60.0);
        const bool growing = uniform(0.0, 1.0) < 0.5;
        for (int i = 0; i < hills; ++i) {
            const double halfWave = uniform(270.0, 360.0);
            const double amplitude =
                std::min(260.0, (growing ? 70.0 + i * 45.0 : uniform(110.0, 200.0)) * scale);
            y = clampValue(y + trend, MinTerrainHeight, MaxTerrainHeight - amplitude);
            x += halfWave;
            appendTerrainNode(x, y + amplitude);
            x += halfWave;
            y = appendTerrainNode(x, y);
        }
        break;
    }
    case Bumps: {
        // Short, low ripples that rattle the suspension.
        const int count = 5 + static_cast<int>(uniform(0.0, 4.0));
        const double trend = uniform(-6.0, 6.0);
        for (int i = 0; i < count; ++i) {
            const double halfWave = uniform(85.0, 115.0);
            y = keepInBand(y + trend);
            x += halfWave;
            appendTerrainNode(x, y + std::min(30.0, uniform(18.0, 28.0) * scale));
            x += halfWave;
            y = appendTerrainNode(x, y);
        }
        break;
    }
    case Valley: {
        // Plunge into a basin, roll over a couple of bumps, then climb out.
        const double depth = std::min(uniform(280.0, 460.0) * scale, y - MinTerrainHeight);
        x += uniform(480.0, 600.0);
        const double floor = appendTerrainNode(x, y - depth);
        for (int i = 0; i < 2; ++i) {
            x += uniform(110.0, 130.0);
            appendTerrainNode(x, floor + uniform(18.0, 32.0));
            x += uniform(110.0, 130.0);
            appendTerrainNode(x, floor);
        }
        const double exitHeight = keepInBand(last.y + uniform(-80.0, 60.0));
        x += uniform(340.0, 420.0);
        appendTerrainNode(x, floor + (exitHeight - floor) * 0.55);
        x += uniform(320.0, 400.0);
        appendTerrainNode(x, exitHeight);
        x += 220.0;
        appendTerrainNode(x, keepInBand(exitHeight + uniform(-10.0, 10.0)));
        break;
    }
    case Steps: {
        // Terraces: a short steep riser followed by a flat tread.
        const double direction = y < MaxTerrainHeight - 380.0 ? 1.0 : -1.0;
        const int count = 3 + static_cast<int>(uniform(0.0, 2.0));
        for (int i = 0; i < count; ++i) {
            x += uniform(170.0, 220.0);
            y = appendTerrainNode(x, keepInBand(y + direction * uniform(70.0, 105.0) * scale));
            x += uniform(230.0, 320.0);
            y = appendTerrainNode(x, keepInBand(y + direction * uniform(0.0, 8.0)));
        }
        break;
    }
    case Gap: {
        // Run-up, a kicker whose lip points upward, the chasm, then a slightly
        // lower landing that slopes away to soften the touchdown.
        y = std::min(y, MaxTerrainHeight - 60.0);
        x += uniform(420.0, 520.0);
        y = appendTerrainNode(x, keepInBand(y - uniform(0.0, 25.0)));
        x += uniform(260.0, 320.0);
        y = appendTerrainNode(x, y + uniform(35.0, 55.0), uniform(0.32, 0.4));
        const double gapStart = x;
        x += uniform(200.0, 240.0) + 60.0 * difficulty;
        y = appendTerrainNode(x, y - uniform(30.0, 70.0), -0.12);
        m_gaps.push_back({gapStart, x});
        x += uniform(300.0, 380.0);
        y = keepInBand(y - uniform(10.0, 40.0));
        appendTerrainNode(x, y);
        break;
    }
    default:
        break;
    }
}

void GameWidget::updatePhysics(double dt)
{
    const bool accelerate = m_keys.contains(Qt::Key_Right) || m_keys.contains(Qt::Key_D);
    const bool reverse = m_keys.contains(Qt::Key_Left) || m_keys.contains(Qt::Key_A);
    const bool boost = m_keys.contains(Qt::Key_Space);
    const double throttle = (accelerate ? 1.0 : 0.0) - (reverse ? 1.0 : 0.0);

    m_headHit = false;
    const double h = dt / PhysicsSubsteps;
    for (int i = 0; i < PhysicsSubsteps; ++i)
        stepPhysics(h, throttle, boost);

    if (throttle != 0.0 && m_fuel > 0.0) {
        m_fuel -= dt * (boost ? 2.25 : 0.82);
        m_fuel = std::max(0.0, m_fuel);
    }

    // Do not allow reversing beyond the starting area.
    if (m_chassis.position.x() < 250.0) {
        const double push = 250.0 - m_chassis.position.x();
        for (RigidBody *body : {&m_chassis, &m_wheels[0], &m_wheels[1]}) {
            body->position.rx() += push;
            body->velocity.setX(std::max(0.0, body->velocity.x()));
        }
    }

    const bool fuelFinished = m_fuel <= 0.0 && m_wheelContacts > 0
                              && std::abs(m_chassis.velocity.x()) < 16.0;
    const bool fellIntoGap =
        m_chassis.position.y() < surfaceHeight(m_chassis.position.x()) - 220.0;
    if (fuelFinished || fellIntoGap || m_headHit) {
        m_gameOver = true;
        m_keys.clear();
    }

    m_score = std::max(m_score,
                       static_cast<int>(std::max(0.0, m_chassis.position.x() - 360.0) / 4.0)
                           + m_coins * 100);
}

void GameWidget::stepPhysics(double h, double throttle, bool boost)
{
    struct Contact {
        RigidBody *body;
        QPointF arm;
        QPointF normal;
        double normalLambda;
        double dynamicFriction;
        bool stuck;
    };

    RigidBody *const bodies[] = {&m_chassis, &m_wheels[0], &m_wheels[1]};
    const bool hasFuel = m_fuel > 0.0;

    // 1. Integrate external forces, engine and brakes.
    for (RigidBody *body : bodies) {
        body->previousPosition = body->position;
        body->previousAngle = body->angle;
        body->velocity.ry() -= Gravity * h;
    }

    const double driveTorque = boost ? BoostDriveTorque : DriveTorque;
    const double maxSpin = boost ? BoostWheelSpin : MaxWheelSpin;
    for (RigidBody &wheel : m_wheels) {
        // Positive spin rolls the car forward (clockwise in a y-up world).
        const double spin = -wheel.angularVelocity;
        double torque = 0.0;
        if (throttle > 0.0 && hasFuel) {
            torque = -driveTorque * clampValue(1.0 - spin / maxSpin, 0.0, 1.0);
        } else if (throttle < 0.0) {
            if (spin > 0.5)
                torque = std::min(BrakeTorque, spin / (wheel.inverseInertia * h));
            else if (hasFuel)
                torque = ReverseTorque * clampValue(1.0 + spin / MaxReverseSpin, 0.0, 1.0);
        }

        // The engine twists the wheel one way and the chassis the other, which
        // is what lifts the nose under hard acceleration.
        wheel.angularVelocity += torque * wheel.inverseInertia * h;
        m_chassis.angularVelocity -= torque * m_chassis.inverseInertia * h;
        wheel.angularVelocity *= 1.0 - RollingResistance * h;
    }

    if (m_wheelContacts == 0) {
        // Pedals pitch the car in the air, a defining hill-climb mechanic.
        m_chassis.angularVelocity += throttle * AirControl * h;
        m_chassis.angularVelocity *= 1.0 - 0.3 * h;
    }
    m_chassis.angularVelocity = clampValue(m_chassis.angularVelocity, -MaxChassisSpin, MaxChassisSpin);

    for (RigidBody *body : bodies) {
        body->position += body->velocity * h;
        body->angle += body->angularVelocity * h;
    }

    // 2. Suspension: each wheel slides on a rigid axis through its mounting
    //    point, held by a spring and bounded by bump stops.
    for (int i = 0; i < 2; ++i) {
        RigidBody &wheel = m_wheels[i];
        const QPointF mount(i == 0 ? -WheelOffset : WheelOffset, WheelLocalY);

        const auto extension = [&](QPointF *up, QPointF *side) {
            *up = rotatePoint(QPointF(0.0, 1.0), m_chassis.angle);
            *side = rotatePoint(QPointF(1.0, 0.0), m_chassis.angle);
            const QPointF anchor = m_chassis.position + rotatePoint(mount, m_chassis.angle);
            return wheel.position - anchor;
        };

        QPointF up;
        QPointF side;
        QPointF offset = extension(&up, &side);
        applyCorrection(wheel, &m_chassis, QPointF(), wheel.position - m_chassis.position,
                        -side * QPointF::dotProduct(offset, side), 0.0, h);

        offset = extension(&up, &side);
        const double travel = -QPointF::dotProduct(offset, up);
        if (travel > SuspensionMaxExtension || travel < SuspensionMinExtension) {
            const double limit = clampValue(travel, SuspensionMinExtension, SuspensionMaxExtension);
            applyCorrection(wheel, &m_chassis, QPointF(), wheel.position - m_chassis.position,
                            up * (travel - limit), 0.0, h);
        }

        offset = extension(&up, &side);
        const double stretch = -QPointF::dotProduct(offset, up) - SuspensionSag;
        applyCorrection(wheel, &m_chassis, QPointF(), wheel.position - m_chassis.position,
                        up * stretch, 1.0 / SuspensionStiffness, h);
    }

    // 3. Ground contacts with static friction. Tyres are true circles tested
    //    against the terrain outline, so they cannot sink into slopes or crests.
    QVarLengthArray<Contact, 12> contacts;
    const auto collide = [&](RigidBody &body, const QPointF &center, double radius,
                             double staticFriction, double dynamicFriction) {
        QPointF normal;
        double depth = 0.0;
        if (!findTerrainContact(center, radius, &normal, &depth))
            return false;

        const QPointF arm = center - body.position - normal * radius;
        const double normalLambda =
            applyCorrection(body, nullptr, arm, QPointF(), normal * depth, 0.0, h);

        // Cancel sliding of the touching material point while the normal force
        // can hold it; this is what lets a spinning tyre push the car along.
        const QPointF localArm = rotatePoint(arm, -body.angle);
        const QPointF before = body.previousPosition + rotatePoint(localArm, body.previousAngle);
        const QPointF now = body.position + arm;
        const QPointF slip = now - before;
        const QPointF tangentialSlip = slip - normal * QPointF::dotProduct(slip, normal);
        const double slipLength = length(tangentialSlip);
        bool stuck = false;
        if (slipLength > 1e-9) {
            const double tangentialLambda =
                slipLength / generalizedInverseMass(body, arm, tangentialSlip / slipLength);
            if (tangentialLambda < staticFriction * normalLambda) {
                applyCorrection(body, nullptr, arm, QPointF(), -tangentialSlip, 0.0, h);
                stuck = true;
            }
        }

        contacts.append({&body, arm, normal, normalLambda, dynamicFriction, stuck});
        return true;
    };

    m_wheelContacts = 0;
    for (RigidBody &wheel : m_wheels) {
        if (collide(wheel, wheel.position, WheelRadius, TyreStaticFriction, TyreDynamicFriction))
            ++m_wheelContacts;
    }
    for (const BodyCollider &collider : ChassisColliders) {
        const QPointF center =
            m_chassis.position + rotatePoint(QPointF(collider.x, collider.y), m_chassis.angle);
        if (collide(m_chassis, center, collider.radius, BodyStaticFriction, BodyDynamicFriction)
            && collider.head) {
            m_headHit = true;
        }
    }

    // 4. Derive velocities from the corrected positions.
    for (RigidBody *body : bodies) {
        body->velocity = (body->position - body->previousPosition) / h;
        body->angularVelocity = (body->angle - body->previousAngle) / h;
    }

    // 5. Velocity-level effects: suspension damping and kinetic friction.
    for (int i = 0; i < 2; ++i) {
        RigidBody &wheel = m_wheels[i];
        const QPointF up = rotatePoint(QPointF(0.0, 1.0), m_chassis.angle);
        const QPointF arm = wheel.position - m_chassis.position;
        const double relative =
            QPointF::dotProduct(wheel.velocity - pointVelocity(m_chassis, arm), up);
        const double weight =
            generalizedInverseMass(wheel, QPointF(), up) + generalizedInverseMass(m_chassis, arm, up);
        const double impulse = -relative * std::min(SuspensionDamping * h, 1.0 / weight);
        applyImpulse(wheel, QPointF(), up * impulse);
        applyImpulse(m_chassis, arm, -up * impulse);
    }

    for (const Contact &contact : contacts) {
        if (contact.stuck)
            continue;
        const QPointF velocity = pointVelocity(*contact.body, contact.arm);
        const QPointF tangential =
            velocity - contact.normal * QPointF::dotProduct(velocity, contact.normal);
        const double speed = length(tangential);
        if (speed < 1e-9)
            continue;
        const QPointF direction = tangential / speed;
        // Coulomb friction: the impulse is bounded by mu * N * h, and never more
        // than what would bring the sliding point to rest.
        const double normalForce = contact.normalLambda / (h * h);
        const double weight = generalizedInverseMass(*contact.body, contact.arm, direction);
        const double impulse = std::min(h * contact.dynamicFriction * normalForce, speed / weight);
        applyImpulse(*contact.body, contact.arm, -direction * impulse);
    }
}

bool GameWidget::findTerrainContact(const QPointF &center, double radius,
                                    QPointF *normal, double *depth) const
{
    // Build the ground outline around the circle, including the vertical rock
    // faces and floor of any chasm, then find the closest point on it.
    constexpr double Step = 3.0;
    const double reach = radius + 6.0;
    const double left = center.x() - reach;
    const double right = center.x() + reach;

    QVarLengthArray<double, 64> xs;
    for (double x = left; x < right; x += Step)
        xs.append(x);
    xs.append(right);
    for (const TerrainGap &gap : m_gaps) {
        if (gap.start > right)
            break;
        if (gap.start > left)
            xs.append(gap.start);
        if (gap.end > left && gap.end < right)
            xs.append(gap.end);
    }
    std::sort(xs.begin(), xs.end());

    QVarLengthArray<QPointF, 72> outline;
    bool previousInGap = false;
    for (int i = 0; i < xs.size(); ++i) {
        const double x = xs.at(i);
        const bool inGap = gapAt(x) != nullptr;
        if (i > 0 && inGap != previousInGap)
            outline.append(QPointF(inGap ? xs.at(i - 1) : x, GapFloor));
        outline.append(QPointF(x, inGap ? GapFloor : surfaceHeight(x)));
        previousInGap = inGap;
    }

    double bestDistanceSq = std::numeric_limits<double>::max();
    QPointF bestPoint;
    QPointF bestOutward;
    for (int i = 1; i < outline.size(); ++i) {
        const QPointF a = outline.at(i - 1);
        const QPointF segment = outline.at(i) - a;
        const double segmentLengthSq = QPointF::dotProduct(segment, segment);
        if (segmentLengthSq < 1e-12)
            continue;
        const double t = clampValue(QPointF::dotProduct(center - a, segment) / segmentLengthSq, 0.0, 1.0);
        const QPointF closest = a + segment * t;
        const QPointF toCenter = center - closest;
        const double distanceSq = QPointF::dotProduct(toCenter, toCenter);
        if (distanceSq < bestDistanceSq) {
            bestDistanceSq = distanceSq;
            bestPoint = closest;
            // The outline runs left to right, so solid ground is on its right.
            bestOutward = QPointF(-segment.y(), segment.x()) / std::sqrt(segmentLengthSq);
        }
    }
    if (outline.size() < 2)
        return false;

    // The ground is a height field with vertical chasm walls, so the inside
    // test is exact against the height; segment normals are ambiguous at corners.
    const double distance = std::sqrt(bestDistanceSq);
    const bool inside = center.y() < terrainHeight(center.x());
    if (inside) {
        *normal = distance > 1e-9 ? (bestPoint - center) / distance : bestOutward;
        *depth = radius + distance;
        return true;
    }
    if (distance >= radius)
        return false;

    *normal = distance > 1e-9 ? (center - bestPoint) / distance : bestOutward;
    *depth = radius - distance;
    return true;
}

void GameWidget::updatePickups()
{
    for (Pickup &pickup : m_pickups) {
        if (pickup.collected)
            continue;

        const QPointF delta = pickup.position - m_chassis.position;
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
    // Generate past the horizon so the curve under new pickups is final.
    ensureTerrainAhead(horizon + 1200.0);
    while (m_nextPickupX < horizon) {
        const int group = static_cast<int>(m_nextPickupX / 420.0);
        const bool placeFuel = group > 0 && group % 7 == 0;

        if (placeFuel) {
            m_pickups.push_back({Pickup::Type::Fuel,
                                 QPointF(m_nextPickupX,
                                         surfaceHeight(m_nextPickupX) + 104.0),
                                 false});
            m_nextPickupX += 430.0;
        } else {
            const int count = 3 + (group % 3);
            for (int i = 0; i < count; ++i) {
                const double x = m_nextPickupX + i * 55.0;
                const double arc = std::sin((i + 1.0) / (count + 1.0) * Pi) * 48.0;
                m_pickups.push_back({Pickup::Type::Coin,
                                     QPointF(x, surfaceHeight(x) + 112.0 + arc),
                                     false});
            }
            m_nextPickupX += count * 55.0 + 285.0;
        }
    }
}

int GameWidget::terrainSegmentFor(double x) const
{
    if (m_terrainNodes.size() < 2)
        return 0;

    int low = 0;
    int high = static_cast<int>(m_terrainNodes.size()) - 1;
    while (low + 1 < high) {
        const int middle = low + (high - low) / 2;
        if (m_terrainNodes.at(middle).x <= x)
            low = middle;
        else
            high = middle;
    }

    return std::clamp(low, 0, static_cast<int>(m_terrainNodes.size()) - 2);
}

const GameWidget::TerrainGap *GameWidget::gapAt(double x) const
{
    auto it = std::upper_bound(m_gaps.cbegin(), m_gaps.cend(), x,
                               [](double value, const TerrainGap &gap) {
                                   return value < gap.start;
                               });
    if (it == m_gaps.cbegin())
        return nullptr;
    --it;
    return x > it->start && x < it->end ? &*it : nullptr;
}

double GameWidget::surfaceHeight(double x) const
{
    if (m_terrainNodes.isEmpty())
        return 228.0;
    if (m_terrainNodes.size() == 1 || x <= m_terrainNodes.constFirst().x)
        return m_terrainNodes.constFirst().y;
    if (x >= m_terrainNodes.constLast().x)
        return m_terrainNodes.constLast().y;

    const int index = terrainSegmentFor(x);
    const TerrainNode &a = m_terrainNodes.at(index);
    const TerrainNode &b = m_terrainNodes.at(index + 1);
    const double h = b.x - a.x;
    const double t = clampValue((x - a.x) / h, 0.0, 1.0);
    const double t2 = t * t;
    const double t3 = t2 * t;

    return (2.0 * t3 - 3.0 * t2 + 1.0) * a.y
           + (t3 - 2.0 * t2 + t) * h * a.tangent
           + (-2.0 * t3 + 3.0 * t2) * b.y
           + (t3 - t2) * h * b.tangent;
}

double GameWidget::surfaceSlope(double x) const
{
    if (m_terrainNodes.size() < 2
        || x <= m_terrainNodes.constFirst().x
        || x >= m_terrainNodes.constLast().x) {
        return 0.0;
    }

    const int index = terrainSegmentFor(x);
    const TerrainNode &a = m_terrainNodes.at(index);
    const TerrainNode &b = m_terrainNodes.at(index + 1);
    const double h = b.x - a.x;
    const double t = clampValue((x - a.x) / h, 0.0, 1.0);
    const double t2 = t * t;

    return ((6.0 * t2 - 6.0 * t) * a.y
            + (3.0 * t2 - 4.0 * t + 1.0) * h * a.tangent
            + (-6.0 * t2 + 6.0 * t) * b.y
            + (3.0 * t2 - 2.0 * t) * h * b.tangent) / h;
}

double GameWidget::terrainHeight(double x) const
{
    return gapAt(x) ? GapFloor : surfaceHeight(x);
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

void GameWidget::buildSoilTexture()
{
    // One seamless soil tile, painted once: mottled dirt, grit, pebbles and
    // chunky outlined rocks. Everything is also drawn shifted by a tile in each
    // direction so features crossing an edge continue on the other side.
    QImage tile(SoilTileSize, SoilTileSize, QImage::Format_ARGB32_Premultiplied);
    tile.fill(QColor(146, 92, 48));
    QPainter painter(&tile);
    painter.setRenderHint(QPainter::Antialiasing, true);

    std::mt19937 rng(20261003u);
    const auto uniform = [&rng](double minimum, double maximum) {
        return std::uniform_real_distribution<double>(minimum, maximum)(rng);
    };
    const auto wrapped = [](const QPointF &center, const auto &draw) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy)
                draw(center + QPointF(dx * SoilTileSize, dy * SoilTileSize));
        }
    };

    const std::array<QColor, 4> mottles{QColor(126, 76, 36, 70), QColor(164, 108, 58, 60),
                                        QColor(112, 66, 30, 55), QColor(152, 98, 50, 70)};
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < 260; ++i) {
        const QPointF center(uniform(0.0, SoilTileSize), uniform(0.0, SoilTileSize));
        const double rx = uniform(18.0, 95.0);
        const double ry = rx * uniform(0.45, 0.8);
        painter.setBrush(mottles[static_cast<size_t>(i) % mottles.size()]);
        wrapped(center, [&](const QPointF &c) { painter.drawEllipse(c, rx, ry); });
    }

    for (int i = 0; i < 2600; ++i) {
        const QPointF center(uniform(0.0, SoilTileSize), uniform(0.0, SoilTileSize));
        const double radius = uniform(0.9, 2.3);
        painter.setBrush(i % 3 == 0 ? QColor(184, 128, 74, 110) : QColor(92, 54, 26, 130));
        wrapped(center, [&](const QPointF &c) { painter.drawEllipse(c, radius, radius); });
    }

    for (int i = 0; i < 170; ++i) {
        const QPointF center(uniform(0.0, SoilTileSize), uniform(0.0, SoilTileSize));
        const double radius = uniform(3.5, 9.0);
        const quint32 seed = static_cast<quint32>(rng());
        wrapped(center, [&](const QPointF &c) { drawRock(painter, c, radius, seed); });
    }

    QVector<QPointF> placed;
    QVector<double> radii;
    for (int attempt = 0; attempt < 600 && placed.size() < 34; ++attempt) {
        const QPointF center(uniform(0.0, SoilTileSize), uniform(0.0, SoilTileSize));
        const double radius = uniform(16.0, 46.0);
        bool overlaps = false;
        for (int j = 0; j < placed.size() && !overlaps; ++j) {
            QPointF delta = center - placed.at(j);
            delta.rx() = std::remainder(delta.x(), double(SoilTileSize));
            delta.ry() = std::remainder(delta.y(), double(SoilTileSize));
            overlaps = std::hypot(delta.x(), delta.y()) < radius + radii.at(j) + 26.0;
        }
        if (overlaps)
            continue;
        placed << center;
        radii << radius;
        const quint32 seed = static_cast<quint32>(rng());
        wrapped(center, [&](const QPointF &c) { drawRock(painter, c, radius, seed); });
    }

    painter.end();
    m_soilTexture = QPixmap::fromImage(tile);
}

void GameWidget::drawTerrain(QPainter &painter) const
{
    const double viewLeft = m_cameraX - 60.0;
    const double viewRight = m_cameraX + DesignWidth + 60.0;

    // Split the visible range into solid stretches separated by chasms.
    double cursor = viewLeft;
    for (const TerrainGap &gap : m_gaps) {
        if (gap.end <= viewLeft)
            continue;
        if (gap.start >= viewRight)
            break;
        if (gap.start > cursor)
            drawTerrainSection(painter, cursor, gap.start, cursor > viewLeft, true);
        cursor = std::max(cursor, gap.end);
    }
    if (cursor < viewRight)
        drawTerrainSection(painter, cursor, viewRight, cursor > viewLeft, false);
}

void GameWidget::drawTerrainSection(QPainter &painter, double start, double end,
                                    bool leftCliff, bool rightCliff) const
{
    // Sample on a fixed world grid so the outline does not shimmer as it scrolls.
    constexpr double Step = 8.0;
    QVector<double> xs{start};
    for (double x = std::floor(start / Step) * Step + Step; x < end; x += Step)
        xs << x;
    xs << end;

    QVector<QPointF> surface;
    QVector<QPointF> downNormals;
    surface.reserve(xs.size());
    downNormals.reserve(xs.size());
    double highestY = DesignHeight;
    for (const double x : xs) {
        const QPointF point = worldToScreen(QPointF(x, surfaceHeight(x)));
        const double slope = surfaceSlope(x);
        const double length = std::sqrt(1.0 + slope * slope);
        surface << point;
        downNormals << QPointF(slope / length, 1.0 / length);
        highestY = std::min(highestY, point.y());
    }

    const double bottom = DesignHeight + 40.0;
    QVector<QPointF> leftWall;
    QVector<QPointF> rightWall;
    if (leftCliff)
        leftWall = cliffWall(surface.constFirst(), bottom, 1, hashValue(qRound64(start)));
    if (rightCliff)
        rightWall = cliffWall(surface.constLast(), bottom, -1, hashValue(qRound64(end) + 7));

    QPainterPath ground;
    if (leftCliff) {
        ground.moveTo(leftWall.constLast());
        for (int i = static_cast<int>(leftWall.size()) - 2; i >= 0; --i)
            ground.lineTo(leftWall.at(i));
    } else {
        ground.moveTo(surface.constFirst().x(), bottom);
        ground.lineTo(surface.constFirst());
    }
    for (const QPointF &point : surface)
        ground.lineTo(point);
    if (rightCliff) {
        for (const QPointF &point : rightWall)
            ground.lineTo(point);
    } else {
        ground.lineTo(surface.constLast().x(), bottom);
    }
    ground.closeSubpath();

    // Soil texture is anchored to the world so it travels with the hills.
    QBrush soil(m_soilTexture);
    soil.setTransform(QTransform::fromTranslate(-std::fmod(m_cameraX, double(SoilTileSize)),
                                                std::fmod(m_cameraY, double(SoilTileSize))));
    painter.fillPath(ground, soil);

    QPainterPath surfaceLine;
    surfaceLine.moveTo(surface.constFirst());
    for (int i = 1; i < surface.size(); ++i)
        surfaceLine.lineTo(surface.at(i));

    painter.save();
    painter.setClipPath(ground);
    // Darker topsoil under the turf, then a general darkening with depth.
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(90, 50, 22, 55), 150.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(surfaceLine);
    painter.setPen(QPen(QColor(84, 46, 20, 80), 76.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(surfaceLine);

    QLinearGradient depthShade(0.0, highestY + 140.0, 0.0, DesignHeight);
    depthShade.setColorAt(0.0, QColor(40, 20, 8, 0));
    depthShade.setColorAt(1.0, QColor(40, 20, 8, 120));
    painter.fillRect(QRectF(surface.constFirst().x() - 200.0, highestY,
                            surface.constLast().x() - surface.constFirst().x() + 400.0,
                            DesignHeight - highestY + 50.0),
                     depthShade);

    // Shadowed rims along cliff faces.
    for (const QVector<QPointF> *wall : {&leftWall, &rightWall}) {
        if (wall->isEmpty())
            continue;
        QPainterPath face;
        face.moveTo(wall->constFirst());
        for (int i = 1; i < wall->size(); ++i)
            face.lineTo(wall->at(i));
        painter.setPen(QPen(QColor(48, 26, 10, 110), 46.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(face);
    }
    painter.restore();

    // Boulders jutting out of the cliff faces, then a crisp outline.
    const auto finishWall = [&](const QVector<QPointF> &wall, int inward, quint32 seed) {
        if (wall.isEmpty())
            return;
        for (int i = 2; i < wall.size(); i += 3) {
            const double radius = 18.0 + 16.0 * hashUnit(seed + i * 31);
            drawRock(painter, wall.at(i) + QPointF(inward * radius * 0.55, 0.0),
                     radius, hashValue(seed + i));
        }
        QPainterPath face;
        face.moveTo(wall.constFirst());
        for (int i = 1; i < wall.size(); ++i)
            face.lineTo(wall.at(i));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(46, 28, 14), 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(face);
    };
    finishWall(leftWall, 1, hashValue(qRound64(start) + 3));
    finishWall(rightWall, -1, hashValue(qRound64(end) + 11));

    // Thick turf band with a ragged underside, like a cartoon grass cap.
    QPolygonF turf;
    for (const QPointF &point : surface)
        turf << point;
    for (int i = static_cast<int>(surface.size()) - 1; i >= 0; --i) {
        const qint64 cell = qRound64(xs.at(i) / Step);
        const double fringe = (cell % 2 == 0 ? 0.0 : 8.0) + 4.0 * hashUnit(cell);
        const bool edge = (leftCliff && i == 0) || (rightCliff && i == surface.size() - 1);
        turf << surface.at(i) + downNormals.at(i) * (edge ? GrassThickness : GrassThickness + fringe);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(132, 204, 36));
    painter.drawPolygon(turf);

    QPainterPath shadeLine;
    QPainterPath highlightLine;
    for (int i = 0; i < surface.size(); ++i) {
        const QPointF shade = surface.at(i) + downNormals.at(i) * (GrassThickness * 0.72);
        const QPointF highlight = surface.at(i) + downNormals.at(i) * 3.0;
        if (i == 0) {
            shadeLine.moveTo(shade);
            highlightLine.moveTo(highlight);
        } else {
            shadeLine.lineTo(shade);
            highlightLine.lineTo(highlight);
        }
    }
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(92, 162, 26), 9.0, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
    painter.drawPath(shadeLine);
    painter.setPen(QPen(QColor(192, 240, 84), 5.0, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
    painter.drawPath(highlightLine);
    painter.setPen(QPen(QColor(36, 64, 12), 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolygon(turf);

    // Grass tufts and the odd pebble sitting on top of the turf.
    constexpr double TuftCell = 130.0;
    for (double cellX = std::floor(start / TuftCell) * TuftCell; cellX < end; cellX += TuftCell) {
        const qint64 cell = qRound64(cellX / TuftCell);
        const quint32 h = hashValue(cell * 977);
        const double x = cellX + 15.0 + (h >> 8) % 100;
        if (x < start + 30.0 || x > end - 30.0)
            continue;

        const QPointF base = worldToScreen(QPointF(x, surfaceHeight(x)));
        if (h % 11 == 7) {
            drawRock(painter, base + QPointF(0.0, 1.0), 8.0 + (h >> 4) % 6, h);
            continue;
        }
        if (h % 5 >= 2)
            continue;

        painter.save();
        painter.translate(base + QPointF(0.0, -1.0));
        painter.rotate(-std::atan(surfaceSlope(x)) * 180.0 / Pi);
        const int blades = 3 + static_cast<int>((h >> 16) % 3);
        QPainterPath tuft;
        for (int b = 0; b < blades; ++b) {
            const double lean = (b - (blades - 1) * 0.5) * 4.5;
            const double height = 11.0 + 9.0 * hashUnit(h + b);
            tuft.moveTo(lean - 3.2, 0.0);
            tuft.quadTo(lean * 1.2, -height * 0.55, lean * 1.9, -height);
            tuft.quadTo(lean * 1.1 + 1.0, -height * 0.45, lean + 3.2, 0.0);
            tuft.closeSubpath();
        }
        painter.setPen(QPen(QColor(36, 64, 12), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(QColor(120, 196, 38));
        painter.drawPath(tuft);
        painter.restore();
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
    if (m_carBody.isNull() || m_wheelSprite.isNull())
        return;

    // Tyres are drawn first at their simulated positions, then the wheel-free
    // body covers the portions that belong behind the fenders and suspension.
    for (const RigidBody &wheel : m_wheels) {
        painter.save();
        painter.translate(worldToScreen(wheel.position));
        painter.rotate(-wheel.angle * 180.0 / Pi);
        painter.drawPixmap(
            QRectF(-WheelSpriteTargetRadius, -WheelSpriteTargetRadius,
                   WheelSpriteTargetRadius * 2.0, WheelSpriteTargetRadius * 2.0),
            m_wheelSprite, QRectF(m_wheelSprite.rect()));
        painter.restore();
    }

    painter.save();
    painter.translate(worldToScreen(m_chassis.position));
    painter.rotate(-m_chassis.angle * 180.0 / Pi);
    const double spriteLeft = -CarSourceCenterX * CarSpriteScale;
    const double spriteTop = -WheelLocalY - WheelSourceY * CarSpriteScale;
    painter.drawPixmap(
        QRectF(spriteLeft, spriteTop,
               m_carBody.width() * CarSpriteScale,
               m_carBody.height() * CarSpriteScale),
        m_carBody, QRectF(m_carBody.rect()));
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
    const int distance = static_cast<int>(std::max(0.0, m_chassis.position.x() - 360.0) / 10.0);
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

    const int distance = static_cast<int>(std::max(0.0, m_chassis.position.x() - 360.0) / 10.0);
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
