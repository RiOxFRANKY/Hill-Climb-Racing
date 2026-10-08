#pragma once

#include "core/game_core.h"
#include "render/game_renderer.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

class QComboBox;
class QKeyEvent;
class QPaintEvent;
class QFocusEvent;
class QResizeEvent;

class GameWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void tick();

private:
    static constexpr int DesignWidth = 1920;
    static constexpr int DesignHeight = 1080;

    void resetGame();
    void layoutLevelSelector();

    QTimer m_timer;
    QElapsedTimer m_clock;

    Core::GameCore m_gameCore;
    Render::GameRenderer m_renderer;

    QComboBox *m_levelSelector = nullptr;
};