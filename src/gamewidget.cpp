#include "gamewidget.h"

#include <QApplication>
#include <QFile>
#include <QFocusEvent>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QVarLengthArray>

#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>

namespace {
constexpr double Pi = 3.14159265358979323846;
constexpr double WheelRadius = 40.0;
// Car artwork geometry, in car_body.png pixels: the wheel-arch centres and the
// tyre radius that fills them. The body is scaled so that radius matches the
// physical wheel.
constexpr double BodyWheelReferenceRadius = 180.0;
constexpr double CarSpriteScale = WheelRadius / BodyWheelReferenceRadius;
constexpr double RearWheelSourceX = 395.0;
constexpr double FrontWheelSourceX = 1345.0;
constexpr double WheelSourceY = 830.0;
// The tyre in wheel.png spans 1076 of its 1254 pixels.
constexpr double WheelSpriteTargetRadius = WheelRadius * 1254.0 / 1076.0;
constexpr double WheelOffset = (FrontWheelSourceX - RearWheelSourceX) * 0.5 * CarSpriteScale;
constexpr double WheelLocalY = -43.0;
constexpr double CarSourceCenterX = (RearWheelSourceX + FrontWheelSourceX) * 0.5;

// Vehicle physics. Lengths are world pixels at the course scale of 80 px per
// metre (the car is 4.5 m long on 0.5 m wheels), time is seconds and masses are
// relative to the chassis. Gravity is real Earth gravity at that scale.
constexpr double PixelsPerMetre = 80.0;
constexpr double Gravity = 9.81 * PixelsPerMetre;
constexpr int PhysicsSubsteps = 16;
constexpr double ChassisMass = 1.0;
constexpr double ChassisInertia = ChassisMass * 70.0 * 70.0;
constexpr double WheelMass = 0.15;
constexpr double WheelInertia = 0.5 * WheelMass * WheelRadius * WheelRadius;
// The chassis' centre of mass sits this far below the centre of the artwork
// (engine and frame are low). It keeps the car from tipping backwards on
// slopes up to ~60 degrees. All body-local coordinates are artwork-relative.
constexpr double CenterOfMassY = -28.0;
// Suspension travel is measured along the chassis "down" axis from the wheel's
// position in the artwork. The spring rest point sits below it by the static
// sag, so a car at rest settles with its wheels exactly where they are drawn.
constexpr double SuspensionSag = 10.0;
constexpr double SuspensionStiffness = ChassisMass * Gravity * 0.5 / SuspensionSag;
constexpr double SuspensionDamping = 4.5;
constexpr double SuspensionMinExtension = -14.0;
constexpr double SuspensionMaxExtension = 20.0;

constexpr double DriveTorque = 26000.0;
constexpr double BoostDriveTorque = 34000.0;
constexpr double MaxWheelSpin = 30.0;   // 15 m/s
constexpr double BoostWheelSpin = 36.0; // 18 m/s
constexpr double ReverseTorque = 12000.0;
constexpr double MaxReverseSpin = 9.0;
constexpr double BrakeTorque = 32000.0;
constexpr double RollingResistance = 0.35;
constexpr double AirControl = 3.6;
constexpr double MaxChassisSpin = 7.0;
// Resolving an overlap in one substep would otherwise turn the correction into
// a huge velocity; push-out is bled off gently instead.
constexpr double MaxPushOutSpeed = 150.0;

// A car resting on its roof (or stalled on its side) for this long is wrecked.
constexpr double FlippedAngle = 1.75;
constexpr double OnSideAngle = 1.2;
constexpr double FlipGraceTime = 0.4;

// Grippy off-road tyres: the course has climbs close to 60 degrees.
constexpr double TyreStaticFriction = 1.8;
constexpr double TyreDynamicFriction = 1.5;
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
    {-165.0, 8.0, 20.0, false},   // spare tyre
    // Bumper circles sit high and tucked in so the car clears slopes of about
    // 60 degrees (approach and departure angles), as the course demands.
    {-138.0, -20.0, 9.0, false},  // rear bumper
    {142.0, -5.0, 11.0, false},   // front bumper
    {125.0, 30.0, 9.0, false},    // bonnet
    {0.0, -32.0, 10.0, false},    // skid plate
    {-90.0, 90.0, 9.0, false},    // roll cage
    {28.0, 92.0, 9.0, false},     // windscreen frame
    {-38.0, 80.0, 16.0, true},    // driver's head
}};

// Floor of every ravine, far below the lowest point of the course.
constexpr double GapFloor = -20000.0;
// The ground is drawn as pixel art: one art pixel covers this many world
// pixels, and the art-pixel grid is anchored to the world.
constexpr int TerrainPixel = 4;

// Fuel cans appear at this spacing along the course.
constexpr double FuelSpacing = 250.0 * PixelsPerMetre;

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

// Converts a point given relative to the car artwork into the chassis body
// frame, whose origin is the centre of mass.
QPointF chassisLocal(double x, double y)
{
    return QPointF(x, y - CenterOfMassY);
}

// Stable per-position randomness for decorations, so they never flicker.
quint32 hashValue(qint64 value)
{
    quint64 x = static_cast<quint64>(value) + 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return static_cast<quint32>(x ^ (x >> 31));
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
    loadCourse();

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
    m_chassis = RigidBody{};
    m_chassis.position = QPointF(m_spawnX, surfaceHeight(m_spawnX) + WheelRadius
                                               - chassisLocal(0.0, WheelLocalY).y() + 4.0);
    m_chassis.inverseMass = 1.0 / ChassisMass;
    m_chassis.inverseInertia = 1.0 / ChassisInertia;
    for (int i = 0; i < 2; ++i) {
        RigidBody &wheel = m_wheels[i];
        wheel = RigidBody{};
        wheel.position = m_chassis.position
                         + chassisLocal(i == 0 ? -WheelOffset : WheelOffset, WheelLocalY);
        wheel.inverseMass = 1.0 / WheelMass;
        wheel.inverseInertia = 1.0 / WheelInertia;
    }
    m_wheelContacts = 0;
    m_headHit = false;
    m_bodyContact = false;
    m_flipTimer = 0.0;
    m_gameOverReason.clear();
    m_currentSection = -1;
    m_zoneBannerTime = 0.0;
    m_cameraX = cameraTarget().x();
    m_cameraY = cameraTarget().y();
    m_fuel = 100.0;
    m_survivalTime = 0.0;
    m_nextPickupX = m_spawnX + 400.0;
    m_nextFuelX = m_spawnX + FuelSpacing * 0.6;
    m_score = 0;
    m_coins = 0;
    m_paused = false;
    m_gameOver = false;
    m_finished = false;
    ensurePickupsAhead();
    m_clock.restart();
    update();
}

void GameWidget::tick()
{
    // Nanosecond timing: two ticks can land in the same millisecond, and a zero
    // time step would divide by zero inside the physics.
    const qint64 elapsedNs = m_clock.nsecsElapsed();
    m_clock.restart();
    const double frameTime = clampValue(elapsedNs / 1e9, 0.0, 0.034);

    if (!m_paused && !m_gameOver) {
        updatePhysics(frameTime);

        updatePickups();
        ensurePickupsAhead();
        m_survivalTime += frameTime;

        const int section = sectionAt(m_chassis.position.x());
        if (section != m_currentSection) {
            m_currentSection = section;
            m_zoneBannerTime = 4.0;
        }
        m_zoneBannerTime = std::max(0.0, m_zoneBannerTime - frameTime);

        const QPointF target = cameraTarget();
        const double follow = 1.0 - std::exp(-frameTime * 4.0);
        m_cameraX += (target.x() - m_cameraX) * follow;
        m_cameraY += (target.y() - m_cameraY) * follow;
    }

    update();
}

QPointF GameWidget::cameraTarget() const
{
    const double targetCameraX = std::max(0.0, m_chassis.position.x() - 560.0);

    // Frame the ground under the car, then shift so the lowest and highest
    // ground just ahead stay in view, and finally keep the car itself on
    // screen. Screen y of world height h is DesignHeight - (h - cameraY).
    double lowestAhead = surfaceHeight(m_chassis.position.x());
    double highestAhead = lowestAhead;
    for (double x = m_chassis.position.x() - 300.0; x <= m_chassis.position.x() + 1000.0; x += 40.0) {
        const double height = surfaceHeight(x);
        lowestAhead = std::min(lowestAhead, height);
        highestAhead = std::max(highestAhead, height);
    }
    double targetCameraY = surfaceHeight(m_chassis.position.x()) - 260.0;
    const double lowLimit = lowestAhead - 70.0;   // lowest ground no lower than 1010
    const double highLimit = highestAhead - 940.0; // highest ground no higher than 140
    targetCameraY = highLimit <= lowLimit ? clampValue(targetCameraY, highLimit, lowLimit)
                                          : (highLimit + lowLimit) * 0.5;
    targetCameraY = clampValue(targetCameraY, m_chassis.position.y() - 880.0,
                               m_chassis.position.y() - 240.0);
    return {targetCameraX, targetCameraY};
}

void GameWidget::loadCourse()
{
    // The course comes from data/terrain_10km.npz, exported to JSON by
    // tools/export_terrain.py. Coordinates are metres with height up; world
    // pixels are metres * pixels_per_metre with y up as well.
    QFile file(QStringLiteral(":/assets/terrain_10km.json"));
    if (!file.open(QIODevice::ReadOnly))
        qFatal("Missing course data :/assets/terrain_10km.json");
    const QJsonObject course = QJsonDocument::fromJson(file.readAll()).object();

    m_pixelsPerMetre = course.value(QStringLiteral("pixels_per_metre")).toDouble(PixelsPerMetre);
    const double scale = m_pixelsPerMetre;
    m_spawnX = course.value(QStringLiteral("spawn_x_m")).toDouble() * scale;
    m_finishX = course.value(QStringLiteral("finish_x_m")).toDouble() * scale;

    m_chains.clear();
    const QJsonArray polylines = course.value(QStringLiteral("polylines_m")).toArray();
    for (const QJsonValue &polylineValue : polylines) {
        const QJsonArray polyline = polylineValue.toArray();
        if (polyline.size() < 2)
            continue;
        TerrainChain chain;
        chain.startX = polyline.at(0).toArray().at(0).toDouble() * scale;
        chain.step = (polyline.at(1).toArray().at(0).toDouble()
                      - polyline.at(0).toArray().at(0).toDouble()) * scale;
        chain.heights.reserve(polyline.size());
        for (const QJsonValue &pointValue : polyline) {
            const QJsonArray point = pointValue.toArray();
            const double x = point.at(0).toDouble() * scale;
            // Heights are looked up by index, which requires uniform sampling.
            if (std::abs(x - (chain.startX + chain.step * chain.heights.size())) > 1e-6)
                qFatal("Course chain is not uniformly sampled");
            chain.heights.append(point.at(1).toDouble() * scale);
        }
        m_chains.append(chain);
    }

    m_gaps.clear();
    for (const QJsonValue &gapValue : course.value(QStringLiteral("gap_ranges_m")).toArray()) {
        const QJsonArray gap = gapValue.toArray();
        m_gaps.append({gap.at(0).toDouble() * scale, gap.at(1).toDouble() * scale});
    }

    m_sections.clear();
    const QJsonArray bounds = course.value(QStringLiteral("section_bounds_m")).toArray();
    const QJsonArray names = course.value(QStringLiteral("section_names")).toArray();
    for (int i = 0; i < bounds.size(); ++i) {
        const QJsonArray range = bounds.at(i).toArray();
        m_sections.append({range.at(0).toDouble() * scale, range.at(1).toDouble() * scale,
                           names.at(i).toString().toUpper()});
    }
}

void GameWidget::updatePhysics(double dt)
{
    if (dt <= 0.0)
        return;

    const bool accelerate = m_keys.contains(Qt::Key_Right) || m_keys.contains(Qt::Key_D);
    const bool reverse = m_keys.contains(Qt::Key_Left) || m_keys.contains(Qt::Key_A);
    const bool boost = m_keys.contains(Qt::Key_Space);
    const double throttle = (accelerate ? 1.0 : 0.0) - (reverse ? 1.0 : 0.0);

    m_headHit = false;
    m_bodyContact = false;
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

    // Tilt relative to the ground underneath, so steep hills do not count.
    const double groundAngle = std::atan(surfaceSlope(m_chassis.position.x()));
    const double tilt = std::abs(std::remainder(m_chassis.angle - groundAngle, 2.0 * Pi));
    const bool upsideDown = tilt > FlippedAngle && m_bodyContact;
    const bool stalledOnSide =
        tilt > OnSideAngle && m_bodyContact && length(m_chassis.velocity) < 60.0;
    m_flipTimer = upsideDown || stalledOnSide ? m_flipTimer + dt : 0.0;

    const bool fuelFinished = m_fuel <= 0.0 && m_wheelContacts > 0
                              && std::abs(m_chassis.velocity.x()) < 16.0;
    const bool fellIntoGap =
        m_chassis.position.y() < surfaceHeight(m_chassis.position.x()) - 220.0;
    if (m_chassis.position.x() >= m_finishX) {
        m_finished = true;
        m_gameOverReason = QStringLiteral("FINISHED!");
    } else if (m_headHit)
        m_gameOverReason = QStringLiteral("HEAD CRASH");
    else if (m_flipTimer > FlipGraceTime)
        m_gameOverReason = QStringLiteral("FLIPPED OVER");
    else if (fellIntoGap)
        m_gameOverReason = QStringLiteral("FELL IN");
    else if (fuelFinished)
        m_gameOverReason = QStringLiteral("OUT OF FUEL");

    if (!m_gameOverReason.isEmpty()) {
        m_gameOver = true;
        m_keys.clear();
    }

    const double metres = std::max(0.0, m_chassis.position.x()) / m_pixelsPerMetre;
    m_score = std::max(m_score, static_cast<int>(metres * 2.5) + m_coins * 100);
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
            // Flat torque through the low and mid range, fading near top speed.
            torque = -driveTorque * clampValue((maxSpin - spin) / (maxSpin * 0.15), 0.0, 1.0);
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
        const QPointF mount = chassisLocal(i == 0 ? -WheelOffset : WheelOffset, WheelLocalY);

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
        // Undo this substep's motion into the ground fully, but let any older
        // overlap separate no faster than MaxPushOutSpeed so it cannot launch.
        const double normalSpeed = QPointF::dotProduct(pointVelocity(body, arm), normal);
        const double correction =
            std::min(depth, std::max(0.0, (MaxPushOutSpeed - normalSpeed) * h));
        const double normalLambda =
            applyCorrection(body, nullptr, arm, QPointF(), normal * correction, 0.0, h);

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
            m_chassis.position + rotatePoint(chassisLocal(collider.x, collider.y), m_chassis.angle);
        if (collide(m_chassis, center, collider.radius, BodyStaticFriction, BodyDynamicFriction)) {
            m_bodyContact = true;
            if (collider.head)
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
    // Build the ground outline around the circle from the course vertices
    // (ground is linear between them), plus the vertical rock faces and floor
    // of any ravine, then find the closest point on it.
    const double step = m_chains.isEmpty() ? 20.0 : m_chains.constFirst().step;
    const double reach = radius + 6.0;
    const double left = center.x() - reach;
    const double right = center.x() + reach;

    QVarLengthArray<double, 64> xs;
    xs.append(left);
    for (double x = std::floor(left / step) * step + step; x < right; x += step)
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
                m_fuel = 100.0;
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
    const double horizon = std::min(m_cameraX + DesignWidth + 900.0, m_finishX - 400.0);
    while (m_nextPickupX < horizon) {
        // Keep pickups away from ravines and their kickers.
        bool nearGap = false;
        for (const TerrainGap &gap : m_gaps) {
            if (m_nextPickupX > gap.start - 500.0 && m_nextPickupX < gap.end + 150.0) {
                m_nextPickupX = gap.end + 150.0;
                nearGap = true;
            }
        }
        if (nearGap)
            continue;

        const int group = static_cast<int>(m_nextPickupX / 420.0);
        const bool placeFuel = m_nextPickupX >= m_nextFuelX;

        if (placeFuel) {
            m_nextFuelX += FuelSpacing;
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

const GameWidget::TerrainChain *GameWidget::chainAt(double x) const
{
    auto it = std::upper_bound(m_chains.cbegin(), m_chains.cend(), x,
                               [](double value, const TerrainChain &chain) {
                                   return value < chain.startX;
                               });
    if (it == m_chains.cbegin())
        return nullptr;
    --it;
    return x <= it->endX() ? &*it : nullptr;
}

double GameWidget::chainHeight(const TerrainChain &chain, double x) const
{
    // Linear interpolation between the chain's own samples only.
    const double position = clampValue((x - chain.startX) / chain.step, 0.0,
                                       static_cast<double>(chain.heights.size() - 1));
    const int index = std::min(static_cast<int>(position), static_cast<int>(chain.heights.size()) - 2);
    const double t = position - index;
    return chain.heights.at(index) + (chain.heights.at(index + 1) - chain.heights.at(index)) * t;
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

int GameWidget::sectionAt(double x) const
{
    for (int i = 0; i < m_sections.size(); ++i) {
        if (x < m_sections.at(i).end)
            return i;
    }
    return static_cast<int>(m_sections.size()) - 1;
}

// Ground height for solid ground. Across a ravine this returns a straight line
// between the two lips; that is only for framing the camera, placing pickups and
// detecting a fall. Collision uses terrainHeight() and the outline, which never
// bridge a ravine.
double GameWidget::surfaceHeight(double x) const
{
    if (m_chains.isEmpty())
        return 0.0;
    if (const TerrainChain *chain = chainAt(x))
        return chainHeight(*chain, x);
    if (x < m_chains.constFirst().startX)
        return m_chains.constFirst().heights.constFirst();
    for (int i = 1; i < m_chains.size(); ++i) {
        const TerrainChain &before = m_chains.at(i - 1);
        const TerrainChain &after = m_chains.at(i);
        if (x > before.endX() && x < after.startX) {
            const double t = (x - before.endX()) / (after.startX - before.endX());
            return before.heights.constLast() + (after.heights.constFirst() - before.heights.constLast()) * t;
        }
    }
    return m_chains.constLast().heights.constLast();
}

// Slope from a central difference over one sample on each side, kept inside a
// single chain, so normals and tilt vary smoothly along the polyline.
double GameWidget::surfaceSlope(double x) const
{
    const TerrainChain *chain = chainAt(x);
    if (!chain)
        return 0.0;
    const double x0 = std::max(chain->startX, x - chain->step);
    const double x1 = std::min(chain->endX(), x + chain->step);
    if (x1 - x0 < 1e-9)
        return 0.0;
    return (chainHeight(*chain, x1) - chainHeight(*chain, x0)) / (x1 - x0);
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
    drawFinishLine(painter);
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
    // Pixel-art ground. Each column of art pixels is coloured by its depth below
    // the surface: dark outline, highlight, grass band with a ragged bottom edge,
    // then speckled soil with pebbles and rock clusters. The buffer is drawn
    // scaled up without smoothing; snapping it to the world grid keeps pixels
    // from shimmering while the camera moves smoothly.
    constexpr int Px = TerrainPixel;
    const double snappedX = std::floor(m_cameraX / Px) * Px;
    const double snappedY = std::floor(m_cameraY / Px) * Px;
    const int columns = DesignWidth / Px + 2;
    const int rows = DesignHeight / Px + 3;
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

    // Buffer row 0 starts one art pixel above the screen, so the sub-pixel
    // offset applied when drawing never leaves an uncovered strip.
    constexpr int NoGround = std::numeric_limits<int>::max();
    QVarLengthArray<int, 520> tops(columns + 2);
    QVarLengthArray<double, 520> slopes(columns + 2);
    for (int c = -1; c <= columns; ++c) {
        const double x = (firstColumn + c + 0.5) * Px;
        if (gapAt(x)) {
            tops[c + 1] = NoGround;
            slopes[c + 1] = 0.0;
            continue;
        }
        const double screenY = DesignHeight - (surfaceHeight(x) - snappedY);
        tops[c + 1] = static_cast<int>(std::floor(screenY / Px)) + 1;
        slopes[c + 1] = surfaceSlope(x);
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

        // On steep ground the column's side is exposed; outline it down to the
        // lower neighbour so slopes get a continuous dark edge.
        int outlineBottom = top;
        if (left != NoGround)
            outlineBottom = std::max(outlineBottom, left - 1);
        if (right != NoGround)
            outlineBottom = std::max(outlineBottom, right - 1);
        const double slope = slopes[c + 1];
        // Thick turf whose underside forms ragged teeth, as in the art.
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
                // Brighter upper turf, shaded lower half with blade-like streaks.
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

    // Pebbles and rock clusters, placed per world column at a fixed depth below
    // the surface, only where the soil around them is deep enough.
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
            // Rounded dark stone with a lighter top-left pixel.
            art.fillRect(c, y, 3, 2, QColor(66, 62, 70));
            art.fillRect(c + 1, y - 1, 1, 1, QColor(66, 62, 70));
            art.fillRect(c, y - 1 + 1, 1, 1, QColor(128, 122, 130));
        } else {
            // Three overlapping stones, outlined, with a light top pixel each.
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
    painter.drawImage(QRectF(snappedX - m_cameraX, (m_cameraY - snappedY) - Px,
                             columns * Px, rows * Px),
                      image);
    painter.restore();
}

void GameWidget::drawFinishLine(QPainter &painter) const
{
    const QPointF base = worldToScreen(QPointF(m_finishX, surfaceHeight(m_finishX)));
    if (base.x() < -200.0 || base.x() > DesignWidth + 200.0)
        return;

    // A chequered banner on two posts spanning the road.
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
    // Move from the centre of mass to the artwork origin (screen y is down).
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
    const int distance = static_cast<int>(std::max(0.0, m_chassis.position.x()) / m_pixelsPerMetre);
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

    if (!m_sections.isEmpty()) {
        const CourseSection &section = m_sections.at(sectionAt(m_chassis.position.x()));
        const QString range = QStringLiteral("%1–%2 KM")
                                  .arg(section.start / m_pixelsPerMetre / 1000.0)
                                  .arg(section.end / m_pixelsPerMetre / 1000.0);

        // Persistent section label under the controls hint.
        painter.setPen(QColor(255, 226, 120, 220));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 15, QFont::Bold));
        painter.drawText(QRectF(560.0, 64.0, 800.0, 32.0), Qt::AlignCenter,
                         QStringLiteral("%1  ·  %2   (%3 / %4 m)")
                             .arg(range, section.name)
                             .arg(distance)
                             .arg(static_cast<int>(m_finishX / m_pixelsPerMetre)));

        // Large banner that fades out after entering a new section.
        if (m_zoneBannerTime > 0.0) {
            const double alpha = clampValue(m_zoneBannerTime, 0.0, 1.0);
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
                     m_gameOver ? m_gameOverReason : QStringLiteral("PAUSED"));

    const int distance = static_cast<int>(std::max(0.0, m_chassis.position.x()) / m_pixelsPerMetre);
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
                     !m_gameOver ? QStringLiteral("P  RESUME")
                                 : (m_finished ? QStringLiteral("R  RACE AGAIN")
                                               : QStringLiteral("R  TRY AGAIN")));
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
