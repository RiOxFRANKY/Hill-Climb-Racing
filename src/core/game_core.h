#pragma once

#include "../terrain/terrain.h"
#include "../vehicle/vehicle.h"

#include <QSet>
#include <QString>

namespace Core {

class GameCore {
public:
    GameCore() = default;

    void loadCourse(const QString &resourcePath = QStringLiteral(":/assets/terrain_10km.json"));
    void resetGame();
    void tick(double frameTime);
    void updatePhysics(double dt);

    [[nodiscard]] QPointF cameraTarget() const;

    void handleKeyPress(int key);
    void handleKeyRelease(int key);
    void clearKeys();
    [[nodiscard]] bool isKeyPressed(int key) const;
    void togglePause();

    [[nodiscard]] const Terrain::TerrainManager &terrain() const { return m_terrain; }
    [[nodiscard]] Terrain::TerrainManager &terrain() { return m_terrain; }
    [[nodiscard]] const Vehicle::VehicleEntity &vehicle() const { return m_vehicle; }
    [[nodiscard]] Vehicle::VehicleEntity &vehicle() { return m_vehicle; }

    [[nodiscard]] double cameraX() const { return m_cameraX; }
    [[nodiscard]] double cameraY() const { return m_cameraY; }
    [[nodiscard]] double survivalTime() const { return m_survivalTime; }
    [[nodiscard]] int currentSection() const { return m_currentSection; }
    [[nodiscard]] double zoneBannerTime() const { return m_zoneBannerTime; }
    [[nodiscard]] int score() const { return m_score; }
    [[nodiscard]] int coins() const { return m_coins; }
    [[nodiscard]] bool isPaused() const { return m_paused; }
    [[nodiscard]] bool isGameOver() const { return m_gameOver; }
    [[nodiscard]] bool isFinished() const { return m_finished; }
    [[nodiscard]] const QString &gameOverReason() const { return m_gameOverReason; }
    [[nodiscard]] const QSet<int> &keys() const { return m_keys; }

private:
    Terrain::TerrainManager m_terrain;
    Vehicle::VehicleEntity m_vehicle;
    QSet<int> m_keys;

    double m_cameraX = 0.0;
    double m_cameraY = 0.0;
    double m_survivalTime = 0.0;
    int m_currentSection = -1;
    double m_zoneBannerDelay = 0.5;
    double m_zoneBannerTime = 0.0;
    int m_score = 0;
    int m_coins = 0;
    bool m_paused = false;
    bool m_gameOver = false;
    bool m_finished = false;
    QString m_gameOverReason;
};

} // namespace Core
