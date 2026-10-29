#include "gamewidget.h"

#include <QApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
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
    setMouseTracking(true);

    m_renderer.loadAssets();
    m_gameCore.loadCourse();

    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.start(16);
    m_clock.start();

    openMenu();
}

void GameWidget::resetGame()
{
    m_gameCore.resetGame();
    m_clock.restart();
    update();
}

void GameWidget::openMenu()
{
    // The menu sits over the idle car at the start line.
    m_inMenu = true;
    m_menu.reset();
    setCursor(Qt::ArrowCursor);
    resetGame();
}

void GameWidget::startGame()
{
    m_inMenu = false;
    setCursor(Qt::BlankCursor);
    resetGame();
}

void GameWidget::handleMenuAction(Ui::StartMenu::Action action)
{
    switch (action) {
    case Ui::StartMenu::Action::Play:
        startGame();
        break;
    case Ui::StartMenu::Action::Quit:
        QApplication::quit();
        break;
    case Ui::StartMenu::Action::None:
        break;
    }
}

void GameWidget::tick()
{
    const qint64 elapsedNs = m_clock.nsecsElapsed();
    m_clock.restart();
    const double frameTime = Physics::clampValue(elapsedNs / 1e9, 0.0, 0.034);

    if (m_inMenu)
        m_menu.update(frameTime);
    else
        m_gameCore.tick(frameTime);
    update();
}

QTransform GameWidget::designTransform() const
{
    // Letterbox: scale the fixed design surface to fit the window while keeping
    // its 16:9 shape, centred, with black bars filling any spare space.
    const double scale = std::min(width() / double(DesignWidth), height() / double(DesignHeight));
    QTransform transform;
    transform.translate((width() - DesignWidth * scale) * 0.5, (height() - DesignHeight * scale) * 0.5);
    transform.scale(scale, scale);
    return transform;
}

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    painter.fillRect(rect(), Qt::black);
    painter.setTransform(designTransform());
    const QRectF designRect(0.0, 0.0, DesignWidth, DesignHeight);
    painter.setClipRect(designRect);

    const double animationTime = m_inMenu ? m_menu.time() : m_gameCore.survivalTime();
    m_renderer.drawBackground(painter, m_gameCore.cameraX(), DesignWidth, DesignHeight);
    m_renderer.drawTerrain(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawFinishLine(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawPickups(painter, m_gameCore.terrain(), animationTime, m_gameCore.cameraX(), m_gameCore.cameraY(), DesignWidth, DesignHeight);
    m_renderer.drawCar(painter, m_gameCore.vehicle(), m_gameCore.cameraX(), m_gameCore.cameraY(), DesignHeight);

    if (m_inMenu) {
        m_renderer.drawStartMenu(painter, m_menu, m_gameCore.terrain(), designRect);
        return;
    }
    m_renderer.drawPedals(painter, m_gameCore);
    m_renderer.drawSpeedometer(painter, m_gameCore);
    m_renderer.drawHud(painter, m_gameCore, DesignWidth, DesignHeight);
    m_renderer.drawHangingBanner(painter, m_gameCore, DesignWidth);
    m_renderer.drawOverlay(painter, m_gameCore, designRect);
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        // Holding Up/Down in the menu keeps moving the selection.
        if (m_inMenu && event->key() != Qt::Key_Return && event->key() != Qt::Key_Enter
            && event->key() != Qt::Key_Space && event->key() != Qt::Key_Escape)
            m_menu.handleKey(event->key());
        event->accept();
        return;
    }

    const bool altEnter = (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
                          && (event->modifiers() & Qt::AltModifier);
    if (event->key() == Qt::Key_F11 || altEnter) {
        toggleFullScreen();
    } else if (event->key() == Qt::Key_Escape && isFullScreen() && !(m_inMenu && m_menu.showingControls())) {
        // Esc leaves fullscreen first.
        toggleFullScreen();
    } else if (m_inMenu) {
        if (event->key() == Qt::Key_Escape && !m_menu.showingControls())
            QApplication::quit();
        else
            handleMenuAction(m_menu.handleKey(event->key()));
    } else if (event->key() == Qt::Key_Escape) {
        openMenu();
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

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_inMenu)
        m_menu.handleHover(designTransform().inverted().map(event->position()));
    QWidget::mouseMoveEvent(event);
}

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_inMenu && event->button() == Qt::LeftButton)
        m_menu.handlePress(designTransform().inverted().map(event->position()));
    QWidget::mousePressEvent(event);
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_inMenu && event->button() == Qt::LeftButton)
        handleMenuAction(m_menu.handleRelease(designTransform().inverted().map(event->position())));
    QWidget::mouseReleaseEvent(event);
}

void GameWidget::focusOutEvent(QFocusEvent *event)
{
    m_gameCore.clearKeys();
    QWidget::focusOutEvent(event);
}
