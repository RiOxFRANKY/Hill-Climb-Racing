#pragma once

#include <QElapsedTimer>
#include <QPointF>
#include <QPixmap>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include <array>

class QKeyEvent;
class QPaintEvent;
class QPainter;
class QResizeEvent;

// A 2D rigid body advanced with substepped position-based dynamics.
struct RigidBody {
    QPointF position;
    QPointF velocity;
    double angle = 0.0;
    double angularVelocity = 0.0;
    double inverseMass = 0.0;
    double inverseInertia = 0.0;
    QPointF previousPosition;
    double previousAngle = 0.0;
};

class GameWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void tick();

private:
    struct Pickup {
        enum class Type { Coin, Fuel };
        Type type = Type::Coin;
        QPointF position;
        bool collected = false;
    };

    // One continuous run of ground from the course data: heights (world
    // pixels, y up) sampled at a fixed horizontal step. Chains are never joined;
    // the space between two chains is a ravine.
    struct TerrainChain {
        double startX = 0.0;
        double step = 0.0;
        QVector<double> heights;

        [[nodiscard]] double endX() const { return startX + step * (heights.size() - 1); }
    };

    // A ravine between a takeoff lip (start) and a landing lip (end).
    struct TerrainGap {
        double start = 0.0;
        double end = 0.0;
    };

    struct CourseSection {
        double start = 0.0;
        double end = 0.0;
        QString name;
    };

    static constexpr int DesignWidth = 1920;
    static constexpr int DesignHeight = 1080;

    void resetGame();
    void loadCourse();
    void updatePhysics(double dt);
    void stepPhysics(double h, double throttle, bool boost);
    [[nodiscard]] bool findTerrainContact(const QPointF &center, double radius,
                                          QPointF *normal, double *depth) const;
    void updatePickups();
    void ensurePickupsAhead();

    [[nodiscard]] const TerrainChain *chainAt(double x) const;
    [[nodiscard]] double chainHeight(const TerrainChain &chain, double x) const;
    [[nodiscard]] const TerrainGap *gapAt(double x) const;
    [[nodiscard]] int sectionAt(double x) const;
    [[nodiscard]] double surfaceHeight(double x) const;
    [[nodiscard]] double surfaceSlope(double x) const;
    [[nodiscard]] double terrainHeight(double x) const;
    [[nodiscard]] QPointF worldToScreen(const QPointF &world) const;
    [[nodiscard]] QPointF cameraTarget() const;

    void drawBackground(QPainter &painter) const;
    void drawTerrain(QPainter &painter) const;
    void drawFinishLine(QPainter &painter) const;
    void drawPickups(QPainter &painter) const;
    void drawCar(QPainter &painter) const;
    void drawHud(QPainter &painter) const;
    void drawOverlay(QPainter &painter) const;
    void drawKeyHint(QPainter &painter, const QRectF &rect,
                     const QString &key, const QString &label, bool active) const;

    QTimer m_timer;
    QElapsedTimer m_clock;
    QSet<int> m_keys;
    QVector<Pickup> m_pickups;
    QVector<TerrainChain> m_chains;
    QVector<TerrainGap> m_gaps;
    QVector<CourseSection> m_sections;
    double m_pixelsPerMetre = 80.0;
    double m_spawnX = 800.0;
    double m_finishX = 800000.0;
    QPixmap m_carBody;
    QPixmap m_wheelSprite;
    QPixmap m_backgroundStrip;

    RigidBody m_chassis;
    std::array<RigidBody, 2> m_wheels;
    int m_wheelContacts = 0;
    bool m_headHit = false;
    bool m_bodyContact = false;
    double m_flipTimer = 0.0;
    QString m_gameOverReason;
    double m_cameraX = 0.0;
    double m_cameraY = 0.0;
    double m_fuel = 100.0;
    double m_survivalTime = 0.0;
    double m_nextPickupX = 0.0;
    double m_nextFuelX = 0.0;
    int m_currentSection = -1;
    double m_zoneBannerTime = 0.0;
    int m_score = 0;
    int m_coins = 0;
    bool m_paused = false;
    bool m_gameOver = false;
    bool m_finished = false;
};
