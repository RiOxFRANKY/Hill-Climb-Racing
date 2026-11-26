#pragma once

#include "../core/game_core.h"

#include <QObject>
#include <QPointF>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// Exposes the simulation to QML: input in, per-frame state out. Positions are
// already converted to the 1920x1080 design space the QML scene is laid out in.
class GameController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal cameraX READ cameraX NOTIFY frameChanged)
    Q_PROPERTY(QPointF chassisPos READ chassisPos NOTIFY frameChanged)
    Q_PROPERTY(qreal chassisRotation READ chassisRotation NOTIFY frameChanged)
    Q_PROPERTY(QPointF rearWheelPos READ rearWheelPos NOTIFY frameChanged)
    Q_PROPERTY(qreal rearWheelRotation READ rearWheelRotation NOTIFY frameChanged)
    Q_PROPERTY(QPointF frontWheelPos READ frontWheelPos NOTIFY frameChanged)
    Q_PROPERTY(qreal frontWheelRotation READ frontWheelRotation NOTIFY frameChanged)
    Q_PROPERTY(QPointF carSpriteOffset READ carSpriteOffset CONSTANT)
    Q_PROPERTY(qreal carSpriteScale READ carSpriteScale CONSTANT)
    Q_PROPERTY(qreal wheelSpriteRadius READ wheelSpriteRadius CONSTANT)
    Q_PROPERTY(QPointF finishPos READ finishPos NOTIFY frameChanged)
    Q_PROPERTY(QVariantList pickups READ pickups NOTIFY frameChanged)

    Q_PROPERTY(int distance READ distance NOTIFY frameChanged)
    Q_PROPERTY(int coins READ coins NOTIFY frameChanged)
    Q_PROPERTY(int score READ score NOTIFY frameChanged)
    Q_PROPERTY(qreal fuel READ fuel NOTIFY frameChanged)
    Q_PROPERTY(qreal speedKmh READ speedKmh NOTIFY frameChanged)
    Q_PROPERTY(qreal survivalTime READ survivalTime NOTIFY frameChanged)
    Q_PROPERTY(bool accelerating READ accelerating NOTIFY frameChanged)
    Q_PROPERTY(bool reversing READ reversing NOTIFY frameChanged)
    Q_PROPERTY(bool boosting READ boosting NOTIFY frameChanged)

    Q_PROPERTY(bool paused READ paused NOTIFY frameChanged)
    Q_PROPERTY(bool gameOver READ gameOver NOTIFY frameChanged)
    Q_PROPERTY(bool finished READ finished NOTIFY frameChanged)
    Q_PROPERTY(QString gameOverReason READ gameOverReason NOTIFY frameChanged)

    Q_PROPERTY(QString sectionName READ sectionName NOTIFY frameChanged)
    Q_PROPERTY(QString sectionRange READ sectionRange NOTIFY frameChanged)
    Q_PROPERTY(QString stageText READ stageText NOTIFY frameChanged)
    Q_PROPERTY(qreal zoneBannerTime READ zoneBannerTime NOTIFY frameChanged)
    Q_PROPERTY(QString courseSummary READ courseSummary NOTIFY courseChanged)
    Q_PROPERTY(int map READ map NOTIFY courseChanged)

public:
    static constexpr int DesignWidth = 1920;
    static constexpr int DesignHeight = 1080;

    explicit GameController(QObject *parent = nullptr);

    // Which course the next GameController builds. Call before QML loads.
    // map is 0 (Grazing the Grass) to 4 (The Real Excursion). A non-empty
    // jsonPath replaces map 0 with that course. startAtMetres > 0 spawns the
    // car that far along the first course.
    static void setCourse(const QString &jsonPath, quint32 seed, double startAtMetres = 0.0, int map = 0);

    // Switches to another map and resets the race; does nothing if it is
    // already the current map.
    Q_INVOKABLE void selectMap(int map);
    // One-line description of a map for the menu, e.g. its jumps and difficulty.
    Q_INVOKABLE QString mapSummary(int map) const;

    Q_INVOKABLE void tick(qreal frameTime);
    Q_INVOKABLE void restart();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void pressKey(int key);
    Q_INVOKABLE void releaseKey(int key);
    Q_INVOKABLE void clearKeys();
    Q_INVOKABLE void setGasPedal(bool down);
    Q_INVOKABLE void setBrakePedal(bool down);
    Q_INVOKABLE void setBoostPedal(bool down);

    [[nodiscard]] const Core::GameCore &core() const { return m_core; }

    [[nodiscard]] qreal cameraX() const { return m_core.cameraX(); }
    [[nodiscard]] QPointF chassisPos() const;
    [[nodiscard]] qreal chassisRotation() const;
    [[nodiscard]] QPointF rearWheelPos() const;
    [[nodiscard]] qreal rearWheelRotation() const;
    [[nodiscard]] QPointF frontWheelPos() const;
    [[nodiscard]] qreal frontWheelRotation() const;
    [[nodiscard]] QPointF carSpriteOffset() const;
    [[nodiscard]] qreal carSpriteScale() const;
    [[nodiscard]] qreal wheelSpriteRadius() const;
    [[nodiscard]] QPointF finishPos() const;
    [[nodiscard]] QVariantList pickups() const;

    [[nodiscard]] int distance() const;
    [[nodiscard]] int coins() const { return m_core.coins(); }
    [[nodiscard]] int score() const { return m_core.score(); }
    [[nodiscard]] qreal fuel() const { return m_core.vehicle().fuel(); }
    [[nodiscard]] qreal speedKmh() const;
    [[nodiscard]] qreal survivalTime() const { return m_core.survivalTime(); }
    [[nodiscard]] bool accelerating() const { return m_core.accelerating(); }
    [[nodiscard]] bool reversing() const { return m_core.reversing(); }
    [[nodiscard]] bool boosting() const { return m_core.boosting(); }

    [[nodiscard]] bool paused() const { return m_core.isPaused(); }
    [[nodiscard]] bool gameOver() const { return m_core.isGameOver(); }
    [[nodiscard]] bool finished() const { return m_core.isFinished(); }
    [[nodiscard]] QString gameOverReason() const { return m_core.gameOverReason(); }

    [[nodiscard]] QString sectionName() const;
    [[nodiscard]] QString sectionRange() const;
    [[nodiscard]] QString stageText() const;
    [[nodiscard]] qreal zoneBannerTime() const { return m_core.zoneBannerTime(); }
    [[nodiscard]] QString courseSummary() const;
    [[nodiscard]] int map() const { return m_map; }

signals:
    void frameChanged();
    void courseChanged();

private:
    void buildCourse();

    [[nodiscard]] QPointF worldToScreen(const QPointF &world) const;
    [[nodiscard]] const Terrain::CourseSection *currentSection() const;

    Core::GameCore m_core;
    int m_map = 0;
};
