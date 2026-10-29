#include "start_menu.h"

#include <QtGlobal>

namespace Ui {

namespace {

enum Item { Play, Controls, Quit, ItemCount };

// dialog_button.png (300x74) drawn at a uniform 1.2x so its pixel border stays even.
constexpr double ButtonWidth = 360.0;
constexpr double ButtonHeight = 88.8;
constexpr double ButtonTop = 384.0;
constexpr double ButtonSpacing = 112.0;
constexpr double CentreX = 960.0;

// dialog_box.png (700x390) drawn at a uniform 1.3x.
constexpr double PanelWidth = 910.0;
constexpr double PanelHeight = 507.0;

} // namespace

void StartMenu::reset()
{
    m_selected = Play;
    m_pressed = -1;
    m_backPressed = false;
    m_showControls = false;
    m_time = 0.0;
}

void StartMenu::update(double dt)
{
    m_time += dt;
}

StartMenu::Action StartMenu::handleKey(int key)
{
    if (m_showControls) {
        if (key == Qt::Key_Escape || key == Qt::Key_Return || key == Qt::Key_Enter
            || key == Qt::Key_Space || key == Qt::Key_Backspace) {
            m_showControls = false;
        }
        return Action::None;
    }

    if (key == Qt::Key_Up || key == Qt::Key_W) {
        m_selected = (m_selected + ItemCount - 1) % ItemCount;
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        m_selected = (m_selected + 1) % ItemCount;
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        return activate(m_selected);
    }
    return Action::None;
}

void StartMenu::handleHover(const QPointF &designPos)
{
    if (m_showControls)
        return;
    const int index = itemAt(designPos);
    if (index >= 0)
        m_selected = index;
}

void StartMenu::handlePress(const QPointF &designPos)
{
    if (m_showControls) {
        m_backPressed = backButtonRect().contains(designPos);
        return;
    }
    m_pressed = itemAt(designPos);
    if (m_pressed >= 0)
        m_selected = m_pressed;
}

StartMenu::Action StartMenu::handleRelease(const QPointF &designPos)
{
    if (m_showControls) {
        if (m_backPressed && backButtonRect().contains(designPos))
            m_showControls = false;
        m_backPressed = false;
        return Action::None;
    }

    // A click only counts when released over the same button it started on.
    const int pressed = m_pressed;
    m_pressed = -1;
    if (pressed >= 0 && itemAt(designPos) == pressed)
        return activate(pressed);
    return Action::None;
}

int StartMenu::itemCount()
{
    return ItemCount;
}

QString StartMenu::itemLabel(int index)
{
    switch (index) {
    case Play:
        return QStringLiteral("PLAY");
    case Controls:
        return QStringLiteral("CONTROLS");
    case Quit:
        return QStringLiteral("QUIT");
    default:
        return {};
    }
}

QRectF StartMenu::itemRect(int index)
{
    return {CentreX - ButtonWidth * 0.5, ButtonTop + index * ButtonSpacing, ButtonWidth, ButtonHeight};
}

QRectF StartMenu::controlsPanelRect()
{
    return {CentreX - PanelWidth * 0.5, 540.0 - PanelHeight * 0.5, PanelWidth, PanelHeight};
}

QRectF StartMenu::backButtonRect()
{
    const QRectF panel = controlsPanelRect();
    return {CentreX - ButtonWidth * 0.5, panel.bottom() - ButtonHeight - 34.0, ButtonWidth, ButtonHeight};
}

int StartMenu::itemAt(const QPointF &designPos)
{
    for (int i = 0; i < ItemCount; ++i) {
        if (itemRect(i).contains(designPos))
            return i;
    }
    return -1;
}

StartMenu::Action StartMenu::activate(int index)
{
    switch (index) {
    case Play:
        return Action::Play;
    case Controls:
        m_showControls = true;
        return Action::None;
    case Quit:
        return Action::Quit;
    default:
        return Action::None;
    }
}

} // namespace Ui
