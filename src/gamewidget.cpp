#include "gamewidget.h"

#include <QApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Hill Climb Qt — Basic Edition"));
    setFixedSize(DesignWidth, DesignHeight);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setCursor(Qt::BlankCursor);

    m_renderer.loadAssets();
    m_gameCore.loadCourse();

    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.start(16);
    m_clock.start();

    resetGame();
}

void GameWidget::resetGame()
{
    m_gameCore.resetGame();
    m_clock.restart();
    update();
}

void GameWidget::tick()
{
    const qint64 elapsedNs = m_clock.nsecsElapsed();
    m_clock.restart();
    const double frameTime = Physics::clampValue(elapsedNs / 1e9, 0.0, 0.034);

    m_gameCore.tick(frameTime);
    update();
}

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    m_renderer.drawBackground(painter, m_gameCore.cameraX(), DesignWidth, DesignHeight);
    m_renderer.drawTerrain(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawFinishLine(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawPickups(painter, m_gameCore.terrain(), m_gameCore.survivalTime(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawCar(painter, m_gameCore.vehicle(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignHeight);
    m_renderer.drawPedals(painter, m_gameCore);
    m_renderer.drawSpeedometer(painter, m_gameCore);
    m_renderer.drawHud(painter, m_gameCore, DesignWidth, DesignHeight);
    m_renderer.drawHangingBanner(painter, m_gameCore, DesignWidth);
    m_renderer.drawOverlay(painter, m_gameCore, rect());
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
    } else if (event->key() == Qt::Key_P) {
        m_gameCore.togglePause();
        m_clock.restart();
    } else if (!m_gameCore.isPaused() && !m_gameCore.isGameOver()) {
        m_gameCore.handleKeyPress(event->key());
    }
    event->accept();
}

void GameWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
        m_gameCore.handleKeyRelease(event->key());
    event->accept();
}

void GameWidget::focusOutEvent(QFocusEvent *event)
{
    m_gameCore.clearKeys();
    QWidget::focusOutEvent(event);
}
