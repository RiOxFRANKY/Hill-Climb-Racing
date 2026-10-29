#include "game_controller.h"

#include <QVariantMap>

#include <algorithm>
#include <cmath>

using namespace Physics;

GameController::GameController(QObject *parent)
    : QObject(parent)
{
    m_core.loadCourse();
    m_core.resetGame();
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
