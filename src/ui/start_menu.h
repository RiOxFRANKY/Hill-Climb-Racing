#pragma once

#include <QPointF>
#include <QRectF>
#include <QString>

namespace Ui {

// Title-screen state: which button is highlighted, whether the controls panel
// is open, and the button layout shared by the renderer and mouse hit tests.
// All rectangles are in the 1920x1080 design space.
class StartMenu {
public:
    enum class Action { None, Play, Quit };

    StartMenu() = default;

    void reset();
    void update(double dt);

    Action handleKey(int key);
    void handleHover(const QPointF &designPos);
    void handlePress(const QPointF &designPos);
    Action handleRelease(const QPointF &designPos);

    [[nodiscard]] int selected() const { return m_selected; }
    [[nodiscard]] int pressed() const { return m_pressed; }
    [[nodiscard]] bool backPressed() const { return m_backPressed; }
    [[nodiscard]] bool showingControls() const { return m_showControls; }
    [[nodiscard]] double time() const { return m_time; }

    [[nodiscard]] static int itemCount();
    [[nodiscard]] static QString itemLabel(int index);
    [[nodiscard]] static QRectF itemRect(int index);
    [[nodiscard]] static QRectF controlsPanelRect();
    [[nodiscard]] static QRectF backButtonRect();

private:
    [[nodiscard]] static int itemAt(const QPointF &designPos);
    Action activate(int index);

    int m_selected = 0;
    int m_pressed = -1;
    bool m_backPressed = false;
    bool m_showControls = false;
    double m_time = 0.0;
};

} // namespace Ui
