#include "gamewidget.h"

#include <QApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>

#include <algorithm>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Hill Climb Qt — Basic Edition"));
    // The scene is always rendered at 1920x1080 and scaled to fit the window,
    // so the window can be resized or made fullscreen.
    resize(DesignWidth, DesignHeight);
    setMinimumSize(DesignWidth / 4, DesignHeight / 4);
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

    // Letterbox: scale the fixed design surface to fit the window while keeping
    // its 16:9 shape, centred, with black bars filling any spare space.
    painter.fillRect(rect(), Qt::black);
    const double scale = std::min(width() / double(DesignWidth), height() / double(DesignHeight));
    painter.translate((width() - DesignWidth * scale) * 0.5, (height() - DesignHeight * scale) * 0.5);
    painter.scale(scale, scale);
    const QRectF designRect(0.0, 0.0, DesignWidth, DesignHeight);
    painter.setClipRect(designRect);

    m_renderer.drawBackground(painter, m_gameCore.cameraX(), DesignWidth, DesignHeight);
    m_renderer.drawTerrain(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawFinishLine(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawPickups(painter, m_gameCore.terrain(), m_gameCore.survivalTime(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawCar(painter, m_gameCore.vehicle(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignHeight);
    m_renderer.drawPedals(painter, m_gameCore);
    m_renderer.drawSpeedometer(painter, m_gameCore);
    m_renderer.drawHud(painter, m_gameCore, DesignWidth, DesignHeight);
    m_renderer.drawHangingBanner(painter, m_gameCore, DesignWidth);
    m_renderer.drawOverlay(painter, m_gameCore, designRect);
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    const bool altEnter = (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
                          && (event->modifiers() & Qt::AltModifier);
    if (event->key() == Qt::Key_F11 || altEnter) {
        toggleFullScreen();
    } else if (event->key() == Qt::Key_Escape) {
        // Esc leaves fullscreen first; from a window it quits.
        if (isFullScreen())
            toggleFullScreen();
        else
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

void GameWidget::toggleFullScreen()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
    // Keys held across the switch may never see their release event.
    m_gameCore.clearKeys();
    m_clock.restart();
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
