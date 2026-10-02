#pragma once

#include <QElapsedTimer>
#include <QPointF>
#include <QPixmap>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include <array>
#include <optional>
#include <random>

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

    // Control point of the terrain curve. Heights between nodes come from a
    // cubic Hermite spline; most tangents are derived to keep it overshoot-free.
    struct TerrainNode {
        double x = 0.0;
        double y = 0.0;
        double tangent = 0.0;
        bool fixedTangent = false;
    };

    // A chasm cut into the terrain between two ledges.
    struct TerrainGap {
        double start = 0.0;
        double end = 0.0;
    };

    static constexpr int DesignWidth = 1920;
    static constexpr int DesignHeight = 1080;

    void resetGame();
    void resetTerrain();
    void ensureTerrainAhead(double worldX);
    void appendTerrainFeature();
    double appendTerrainNode(double x, double y, std::optional<double> tangent = std::nullopt);
    void buildSoilTexture();
    void updatePhysics(double dt);
    void stepPhysics(double h, double throttle, bool boost);
    [[nodiscard]] bool findTerrainContact(const QPointF &center, double radius,
                                          QPointF *normal, double *depth) const;
    void updatePickups();
    void ensurePickupsAhead();

    [[nodiscard]] int terrainSegmentFor(double x) const;
    [[nodiscard]] const TerrainGap *gapAt(double x) const;
    [[nodiscard]] double surfaceHeight(double x) const;
    [[nodiscard]] double surfaceSlope(double x) const;
    [[nodiscard]] double terrainHeight(double x) const;
    [[nodiscard]] QPointF worldToScreen(const QPointF &world) const;

    void drawBackground(QPainter &painter) const;
    void drawTerrain(QPainter &painter) const;
    void drawTerrainSection(QPainter &painter, double start, double end,
                            bool leftCliff, bool rightCliff) const;
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
    QVector<TerrainNode> m_terrainNodes;
    QVector<TerrainGap> m_gaps;
    QPixmap m_carBody;
    QPixmap m_wheelSprite;
    QPixmap m_backgroundStrip;
    QPixmap m_soilTexture;
    std::mt19937 m_randomEngine;

    RigidBody m_chassis;
    std::array<RigidBody, 2> m_wheels;
    int m_wheelContacts = 0;
    bool m_headHit = false;
    double m_cameraX = 0.0;
    double m_cameraY = 0.0;
    double m_fuel = 100.0;
    double m_survivalTime = 0.0;
    double m_nextPickupX = 780.0;
    int m_lastFeature = -1;
    int m_score = 0;
    int m_coins = 0;
    bool m_paused = false;
    bool m_gameOver = false;
};
