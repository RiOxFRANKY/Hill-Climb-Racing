#include "game_controller.h"
#include "../terrain/procedural.h"

#include <QVariantMap>

#include <algorithm>
#include <cmath>

using namespace Physics;

namespace {
QString s_courseJson;
quint32 s_courseSeed = 1;
double s_startAtMetres = 0.0;
int s_startMap = 0;
} // namespace

void GameController::setCourse(const QString &jsonPath, quint32 seed, double startAtMetres, int map)
{
    s_courseJson = jsonPath;
    s_courseSeed = seed;
    s_startAtMetres = startAtMetres;
    s_startMap = std::clamp(map, 0, Terrain::Procedural::Settings::MapCount - 1);
}

GameController::GameController(QObject *parent)
    : QObject(parent)
    , m_map(s_startMap)
{
    buildCourse();
    if (s_startAtMetres > 0.0)
        m_core.terrain().setSpawnX(s_startAtMetres * m_core.terrain().pixelsPerMetre());
    m_core.resetGame();
}

void GameController::buildCourse()
{
    if (m_map == 0 && !s_courseJson.isEmpty())
        m_core.loadCourse(s_courseJson);
    else
        m_core.generateCourse(s_courseSeed, m_map);
}

void GameController::selectMap(int map)
{
    map = std::clamp(map, 0, Terrain::Procedural::Settings::MapCount - 1);
    if (map == m_map)
        return;
    m_map = map;
    buildCourse();
    m_core.resetGame();
    emit courseChanged();
    emit frameChanged();
}

QString GameController::mapSummary(int map) const
{
    // Read from the map's settings, so the menu needn't build every course.
    const Terrain::Procedural::Settings settings = Terrain::Procedural::Settings::forMap(map, s_courseSeed);
    double length = 0.0;
    int ravines = 0;
    for (const Terrain::Procedural::ZoneProfile &zone : settings.zones) {
        length += zone.length;
        ravines += zone.ravines;
    }
    return QStringLiteral("%1 KM   •   %2 RAVINES   •   DIFFICULTY %3 / %4")
        .arg(length / 1000.0)
        .arg(ravines)
        .arg(std::clamp(map, 0, Terrain::Procedural::Settings::MapCount - 1) + 1)
        .arg(Terrain::Procedural::Settings::MapCount);
}

void GameController::tick(qreal frameTime)
{
    m_core.tick(clampValue(frameTime, 0.0, 0.034));
    emit frameChanged();
}

void GameController::restart()
{
    m_core.resetGame();
    emit frameChanged();
}

void GameController::togglePause()
{
    m_core.togglePause();
    emit frameChanged();
}

void GameController::pressKey(int key)
{
    if (m_core.isPaused() || m_core.isGameOver())
        return;
    m_core.handleKeyPress(key);
    emit frameChanged();
}

void GameController::releaseKey(int key)
{
    m_core.handleKeyRelease(key);
    emit frameChanged();
}

void GameController::clearKeys()
{
    m_core.clearKeys();
    emit frameChanged();
}

void GameController::setGasPedal(bool down)
{
    m_core.setPedal(Core::GameCore::Pedal::Gas, down);
    emit frameChanged();
}

void GameController::setBrakePedal(bool down)
{
    m_core.setPedal(Core::GameCore::Pedal::Brake, down);
    emit frameChanged();
}

void GameController::setBoostPedal(bool down)
{
    m_core.setPedal(Core::GameCore::Pedal::Boost, down);
    emit frameChanged();
}

QPointF GameController::worldToScreen(const QPointF &world) const
{
    return {world.x() - m_core.cameraX(), DesignHeight - (world.y() - m_core.cameraY())};
}

QVariantList GameController::bridgePlanks() const
{
    QVariantList planks;
    const double left = m_core.cameraX() - 200.0;
    const double right = m_core.cameraX() + DesignWidth + 200.0;
    for (const Terrain::BridgeSet::Bridge &bridge : m_core.bridges().bridges()) {
        if (bridge.endX < left || bridge.startX > right)
            continue;
        for (int i = 0; i + 1 < bridge.joints.size(); ++i) {
            const QPointF a = worldToScreen(bridge.joints[i]);
            const QPointF b = worldToScreen(bridge.joints[i + 1]);
            const QPointF d = b - a;
            planks.append(QVariantMap{
                {QStringLiteral("x"), a.x()},
                {QStringLiteral("y"), a.y()},
                {QStringLiteral("length"), std::hypot(d.x(), d.y())},
                {QStringLiteral("angle"), std::atan2(d.y(), d.x()) * 180.0 / Pi},
            });
        }
    }
    return planks;
}

QVariantList GameController::bridgePosts() const
{
    QVariantList posts;
    const double left = m_core.cameraX() - 200.0;
    const double right = m_core.cameraX() + DesignWidth + 200.0;
    for (const Terrain::BridgeSet::Bridge &bridge : m_core.bridges().bridges()) {
        if (bridge.endX < left || bridge.startX > right)
            continue;
        for (const QPointF &anchor : {bridge.joints.constFirst(), bridge.joints.constLast()}) {
            const QPointF p = worldToScreen(anchor);
            posts.append(QVariantMap{{QStringLiteral("x"), p.x()}, {QStringLiteral("y"), p.y()}});
        }
    }
    return posts;
}

QPointF GameController::chassisPos() const
{
    return worldToScreen(m_core.vehicle().chassis().position);
}

qreal GameController::chassisRotation() const
{
    return -m_core.vehicle().chassis().angle * 180.0 / Pi;
}

QPointF GameController::rearWheelPos() const
{
    return worldToScreen(m_core.vehicle().wheels()[0].position);
}

qreal GameController::rearWheelRotation() const
{
    return -m_core.vehicle().wheels()[0].angle * 180.0 / Pi;
}

QPointF GameController::frontWheelPos() const
{
    return worldToScreen(m_core.vehicle().wheels()[1].position);
}

qreal GameController::frontWheelRotation() const
{
    return -m_core.vehicle().wheels()[1].angle * 180.0 / Pi;
}

QPointF GameController::carSpriteOffset() const
{
    // Top-left of the body sprite relative to the chassis centre of mass, before rotation.
    return {-CarSourceCenterX * CarSpriteScale, CenterOfMassY - WheelLocalY - WheelSourceY * CarSpriteScale};
}

qreal GameController::carSpriteScale() const
{
    return CarSpriteScale;
}

qreal GameController::wheelSpriteRadius() const
{
    return WheelSpriteTargetRadius;
}

QPointF GameController::finishPos() const
{
    const double finishX = m_core.terrain().finishX();
    return worldToScreen(QPointF(finishX, m_core.terrain().surfaceHeight(finishX)));
}

QVariantList GameController::pickups() const
{
    QVariantList visible;
    for (const Terrain::Pickup &pickup : m_core.terrain().pickups()) {
        if (pickup.collected)
            continue;
        const QPointF centre = worldToScreen(pickup.position);
        if (centre.x() < -100.0 || centre.x() > DesignWidth + 100.0)
            continue;
        visible.append(QVariantMap{
            {QStringLiteral("x"), centre.x()},
            {QStringLiteral("y"), centre.y()},
            {QStringLiteral("fuel"), pickup.type == Terrain::Pickup::Type::Fuel},
            {QStringLiteral("value"), pickup.value},
            {QStringLiteral("phase"), pickup.position.x()},
        });
    }
    return visible;
}

int GameController::distance() const
{
    return static_cast<int>(std::max(0.0, m_core.vehicle().chassis().position.x()) / m_core.terrain().pixelsPerMetre());
}

qreal GameController::speedKmh() const
{
    return std::abs(m_core.vehicle().chassis().velocity.x()) / m_core.terrain().pixelsPerMetre() * 3.6;
}

const Terrain::CourseSection *GameController::currentSection() const
{
    const auto &sections = m_core.terrain().sections();
    if (sections.isEmpty())
        return nullptr;
    return &sections.at(m_core.terrain().sectionAt(m_core.vehicle().chassis().position.x()));
}

QString GameController::sectionName() const
{
    const Terrain::CourseSection *section = currentSection();
    return section ? section->name : QString();
}

QString GameController::sectionRange() const
{
    const Terrain::CourseSection *section = currentSection();
    if (!section)
        return {};
    return QStringLiteral("%1–%2 KM")
        .arg(section->start / m_core.terrain().pixelsPerMetre() / 1000.0)
        .arg(section->end / m_core.terrain().pixelsPerMetre() / 1000.0);
}

QString GameController::stageText() const
{
    const Terrain::CourseSection *section = currentSection();
    if (!section)
        return {};
    return QStringLiteral("%1  ·  %2   (%3 / %4 m)")
        .arg(sectionRange(), section->name)
        .arg(distance())
        .arg(static_cast<int>(m_core.terrain().finishX() / m_core.terrain().pixelsPerMetre()));
}

QString GameController::courseSummary() const
{
    const Terrain::TerrainManager &terrain = m_core.terrain();
    return QStringLiteral("%1 KM   •   %2 STAGES   •   %3 RAVINES")
        .arg(terrain.finishX() / terrain.pixelsPerMetre() / 1000.0)
        .arg(terrain.sections().size())
        .arg(terrain.gaps().size());
}
