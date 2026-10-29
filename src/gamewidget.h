#pragma once

#include "core/game_core.h"
#include "render/game_renderer.h"
#include "ui/start_menu.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QTransform>
#include <QWidget>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QFocusEvent;

class GameWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);

    void startGame();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void tick();

private:
    static constexpr int DesignWidth = 1920;
    static constexpr int DesignHeight = 1080;

    void resetGame();
    void openMenu();
    void handleMenuAction(Ui::StartMenu::Action action);
    void toggleFullScreen();
    [[nodiscard]] QTransform designTransform() const;

    QTimer m_timer;
    QElapsedTimer m_clock;

    Core::GameCore m_gameCore;
    Render::GameRenderer m_renderer;
    Ui::StartMenu m_menu;
    bool m_inMenu = true;
};
