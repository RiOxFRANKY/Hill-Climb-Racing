#include "gamewidget.h"

#include <QApplication>
#include <QComboBox>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <algorithm>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Hill Climb Qt — Level Selector"));

    resize(1280, 720);
    setMinimumSize(640, 360);

    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    // Cursor is no longer hidden, so the level dropdown can be clicked.

    m_renderer.loadAssets();

    // --- LEVEL SELECTOR UI ---
    m_levelSelector = new QComboBox(this);
    m_levelSelector->addItem("🌍 Earth - 10km", ":/assets/terrain_10km.json");
    m_levelSelector->addItem("🌕 Moon - 10km",  ":/assets/moon_terrain.json");
    m_levelSelector->setCursor(Qt::ArrowCursor);
    m_levelSelector->setFocusPolicy(Qt::NoFocus);   // keep arrows/space for the car
    m_levelSelector->setStyleSheet(
        "QComboBox { background: rgba(16, 22, 32, 230); color: white; font-weight: bold;"
        "  padding: 4px 8px; border: 2px solid #506580; border-radius: 4px; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right;"
        "  width: 25px; border-left: 1px solid #506580; }"
        "QComboBox QAbstractItemView { background: #101620; color: white;"
        "  selection-background-color: #325078; selection-color: white; border: 1px solid #506580; }"
    );

    connect(m_levelSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString path = m_levelSelector->itemData(index).toString();
        m_gameCore.loadCourse(path, index == 1);   // index 1 = Moon
        resetGame();
        setFocus();   // return keyboard control to the game
    });
    layoutLevelSelector();
    // --- END LEVEL SELECTOR UI ---

    m_gameCore.loadCourse(":/assets/terrain_10km.json", false);

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

void GameWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutLevelSelector();
}

void GameWidget::layoutLevelSelector()
{
    if (!m_levelSelector)
        return;

    const double scale = std::min(width()  / double(DesignWidth),
                                  height() / double(DesignHeight));

    // Sits just under the top-left HUD panel (design coordinates 1920x1080)
    m_levelSelector->setFixedSize(int(300 * scale), int(54 * scale));
    m_levelSelector->move(int(48 * scale), int(212 * scale));

    QFont f = m_levelSelector->font();
    f.setPixelSize(std::max(11, int(22 * scale)));
    m_levelSelector->setFont(f);
}

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Fill the whole window (no black bars): scale by the smaller ratio and
    // let the visible world area grow in the other direction.
    const double scale = std::min(width()  / double(DesignWidth),
                                  height() / double(DesignHeight));
    const int viewW = qRound(width()  / scale);
    const int viewH = qRound(height() / scale);
    painter.scale(scale, scale);

    m_renderer.drawBackground(painter, m_gameCore, m_gameCore.cameraX(), viewW, viewH);
    m_renderer.drawTerrain(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), viewW, viewH);
    m_renderer.drawFinishLine(painter, m_gameCore.terrain(), m_gameCore.cameraX(), m_gameCore.cameraY(), viewW, viewH);
    m_renderer.drawPickups(painter, m_gameCore.terrain(), m_gameCore.survivalTime(), m_gameCore.cameraX(), m_gameCore.cameraY(), viewW, viewH);
    m_renderer.drawCar(painter, m_gameCore.vehicle(), m_gameCore.cameraX(), m_gameCore.cameraY(), viewH);

    // Pedals and speedometer use fixed design coordinates, so anchor them manually
    painter.save();
    painter.translate((viewW - DesignWidth) / 2.0, viewH - DesignHeight);   // bottom-centre
    m_renderer.drawPedals(painter, m_gameCore);
    painter.restore();

    painter.save();
    painter.translate(viewW - DesignWidth, 0);                              // top-right
    m_renderer.drawSpeedometer(painter, m_gameCore);
    painter.restore();

    m_renderer.drawHud(painter, m_gameCore, viewW, viewH);
    m_renderer.drawHangingBanner(painter, m_gameCore, viewW);
    m_renderer.drawOverlay(painter, m_gameCore, QRect(0, 0, viewW, viewH));
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
    } else if (event->key() == Qt::Key_L) {
        // Toggle Earth <-> Moon (the combo's signal reloads the level)
        m_levelSelector->setCurrentIndex(m_levelSelector->currentIndex() == 0 ? 1 : 0);
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