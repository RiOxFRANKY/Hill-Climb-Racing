#pragma once

#include <QElapsedTimer>
#include <QPointF>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QKeyEvent;
class QPaintEvent;
class QPainter;
class QResizeEvent;

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

    static constexpr int DesignWidth = 1920;
    static constexpr int DesignHeight = 1080;

    void resetGame();
    void updatePhysics(double dt);
    void updatePickups();
    void ensurePickupsAhead();

    [[nodiscard]] double terrainHeight(double x) const;
    [[nodiscard]] double terrainSlope(double x) const;
    [[nodiscard]] QPointF wheelPosition(double localX) const;
    [[nodiscard]] QPointF worldToScreen(const QPointF &world) const;

    void drawBackground(QPainter &painter) const;
    void drawTerrain(QPainter &painter) const;
    void drawPickups(QPainter &painter) const;
    void drawCar(QPainter &painter) const;
    void drawHud(QPainter &painter) const;
    void drawOverlay(QPainter &painter) const;
    void drawCloud(QPainter &painter, QPointF position, double scale) const;
    void drawWheel(QPainter &painter, const QPointF &center, double angle) const;
    void drawKeyHint(QPainter &painter, const QRectF &rect,
                     const QString &key, const QString &label, bool active) const;

    QTimer m_timer;
    QElapsedTimer m_clock;
    QSet<int> m_keys;
    QVector<Pickup> m_pickups;

    QPointF m_position;
    QPointF m_velocity;
    double m_angle = 0.0;
    double m_angularVelocity = 0.0;
    double m_cameraX = 0.0;
    double m_cameraY = 0.0;
    double m_fuel = 100.0;
    double m_survivalTime = 0.0;
    double m_nextPickupX = 780.0;
    int m_score = 0;
    int m_coins = 0;
    bool m_grounded = false;
    bool m_paused = false;
    bool m_gameOver = false;
};

