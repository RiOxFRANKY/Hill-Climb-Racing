#include "game_core.h"
#include <QtGlobal>
#include <cmath>
#include <algorithm>

namespace Core {

void GameCore::loadCourse(const QString &resourcePath)
{
    m_terrain.loadCourse(resourcePath);
}

void GameCore::resetGame()
{
    m_keys.clear();
    const double spawnX = m_terrain.spawnX();
    m_vehicle.reset(spawnX, m_terrain.surfaceHeight(spawnX));
    m_gameOverReason.clear();
    m_currentSection = -1;
    m_zoneBannerDelay = 0.5;
    m_zoneBannerTime = 0.0;
    m_cameraX = cameraTarget().x();
    m_cameraY = cameraTarget().y();
    m_survivalTime = 0.0;
    m_score = 0;
    m_coins = 0;
    m_paused = false;
    m_gameOver = false;
    m_finished = false;
    m_terrain.resetPickups(spawnX);
}

void GameCore::tick(double frameTime)
{
    if (!m_paused && !m_gameOver) {
        updatePhysics(frameTime);

        double fuel = m_vehicle.fuel();
        const QPointF headArm = Physics::rotatePoint(Physics::chassisLocal(-38.0, 80.0), m_vehicle.chassis().angle);
        const QPointF headPos = m_vehicle.chassis().position + headArm;
        m_terrain.updatePickups(m_vehicle.chassis().position, headPos, m_cameraX, m_coins, fuel, m_score);
        m_vehicle.setFuel(fuel);
        m_terrain.ensurePickupsAhead(m_cameraX);
        m_survivalTime += frameTime;

        const int section = m_terrain.sectionAt(m_vehicle.chassis().position.x());
        if (m_zoneBannerDelay > 0.0) {
            m_zoneBannerDelay -= frameTime;
            if (m_zoneBannerDelay <= 0.0) {
                m_currentSection = section;
                m_zoneBannerTime = 5.0;
            }
        } else if (section != m_currentSection) {
            m_currentSection = section;
            m_zoneBannerTime = 5.0;
        }
        m_zoneBannerTime = std::max(0.0, m_zoneBannerTime - frameTime);

        const QPointF target = cameraTarget();
        const double follow = 1.0 - std::exp(-frameTime * 4.0);
        m_cameraX += (target.x() - m_cameraX) * follow;
        m_cameraY += (target.y() - m_cameraY) * follow;
    }
}

QPointF GameCore::cameraTarget() const
{
    const double chassisX = m_vehicle.chassis().position.x();
    const double chassisY = m_vehicle.chassis().position.y();
    const double targetCameraX = std::max(0.0, chassisX - 560.0);

    double lowestAhead = m_terrain.surfaceHeight(chassisX);
    double highestAhead = lowestAhead;
    for (double x = chassisX - 300.0; x <= chassisX + 1000.0; x += 40.0) {
        const double height = m_terrain.surfaceHeight(x);
        lowestAhead = std::min(lowestAhead, height);
        highestAhead = std::max(highestAhead, height);
    }
    double targetCameraY = m_terrain.surfaceHeight(chassisX) - 260.0;
    const double lowLimit = lowestAhead - 70.0;
    const double highLimit = highestAhead - 940.0;
    targetCameraY = highLimit <= lowLimit ? Physics::clampValue(targetCameraY, highLimit, lowLimit)
                                          : (highLimit + lowLimit) * 0.5;
    targetCameraY = Physics::clampValue(targetCameraY, chassisY - 880.0, chassisY - 240.0);
    return {targetCameraX, targetCameraY};
}

void GameCore::updatePhysics(double dt)
{
    if (dt <= 0.0)
        return;

    const bool accelerate = isKeyPressed(Qt::Key_Right) || isKeyPressed(Qt::Key_D);
    const bool reverse = isKeyPressed(Qt::Key_Left) || isKeyPressed(Qt::Key_A);
    const bool boost = isKeyPressed(Qt::Key_Space);
    const double throttle = (accelerate ? 1.0 : 0.0) - (reverse ? 1.0 : 0.0);

    m_vehicle.setHeadHit(false);
    m_vehicle.setBodyContact(false);
    const double h = dt / Physics::PhysicsSubsteps;
    for (int i = 0; i < Physics::PhysicsSubsteps; ++i)
        m_vehicle.stepPhysics(h, throttle, boost, m_terrain);

    if (throttle != 0.0 && m_vehicle.fuel() > 0.0) {
        m_vehicle.consumeFuel(dt * (boost ? 2.25 : 0.82));
    }

    if (m_vehicle.chassis().position.x() < 250.0) {
        const double push = 250.0 - m_vehicle.chassis().position.x();
        for (Physics::RigidBody *body : {&m_vehicle.chassis(), &m_vehicle.wheels()[0], &m_vehicle.wheels()[1]}) {
            body->position.rx() += push;
            body->velocity.setX(std::max(0.0, body->velocity.x()));
        }
    }

    const double groundAngle = std::atan(m_terrain.surfaceSlope(m_vehicle.chassis().position.x()));
    const double tilt = std::abs(std::remainder(m_vehicle.chassis().angle - groundAngle, 2.0 * Physics::Pi));
    const bool upsideDown = tilt > Physics::FlippedAngle && m_vehicle.bodyContact();
    const bool stalledOnSide =
        tilt > Physics::OnSideAngle && m_vehicle.bodyContact() && Physics::length(m_vehicle.chassis().velocity) < 60.0;
    m_vehicle.setFlipTimer(upsideDown || stalledOnSide ? m_vehicle.flipTimer() + dt : 0.0);

    const bool fuelFinished = m_vehicle.fuel() <= 0.0 && m_vehicle.wheelContacts() > 0
                              && std::abs(m_vehicle.chassis().velocity.x()) < 16.0;
    const bool fellIntoGap =
        m_vehicle.chassis().position.y() < m_terrain.surfaceHeight(m_vehicle.chassis().position.x()) - 220.0;

    if (m_vehicle.chassis().position.x() >= m_terrain.finishX()) {
        m_finished = true;
        m_gameOverReason = QStringLiteral("FINISHED!");
    } else if (m_vehicle.headHit())
        m_gameOverReason = QStringLiteral("HEAD CRASH");
    else if (m_vehicle.flipTimer() > Physics::FlipGraceTime)
        m_gameOverReason = QStringLiteral("FLIPPED OVER");
    else if (fellIntoGap)
        m_gameOverReason = QStringLiteral("FELL IN");
    else if (fuelFinished)
        m_gameOverReason = QStringLiteral("OUT OF FUEL");

    if (!m_gameOverReason.isEmpty()) {
        m_gameOver = true;
        m_keys.clear();
    }

    const double metres = std::max(0.0, m_vehicle.chassis().position.x()) / m_terrain.pixelsPerMetre();
    m_score = std::max(m_score, static_cast<int>(metres * 2.5) + m_coins * 100);
}

void GameCore::handleKeyPress(int key)
{
    m_keys.insert(key);
}

void GameCore::handleKeyRelease(int key)
{
    m_keys.remove(key);
}

void GameCore::clearKeys()
{
    m_keys.clear();
}

bool GameCore::isKeyPressed(int key) const
{
    return m_keys.contains(key);
}

void GameCore::togglePause()
{
    if (!m_gameOver) {
        m_paused = !m_paused;
        m_keys.clear();
    }
}

} // namespace Core
