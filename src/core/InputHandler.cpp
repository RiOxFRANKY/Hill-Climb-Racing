#include "core/InputHandler.h"
#include <QtCore/Qt>

namespace Core {

InputHandler::InputHandler()
    : m_config() {
}

InputHandler::InputHandler(const Config& config)
    : m_config(config) {
}

void InputHandler::reset() {
    m_keyGas = false;
    m_keyBrake = false;
    m_pedalGas = false;
    m_pedalBrake = false;
    m_restartTrigger = false;
    m_pauseTrigger = false;
}

void InputHandler::onKeyPressed(int key) {
    switch (key) {
        case Qt::Key_D:
        case Qt::Key_Right:
            m_keyGas = true;
            break;
        case Qt::Key_A:
        case Qt::Key_Left:
            m_keyBrake = true;
            break;
        case Qt::Key_R:
            m_restartTrigger = true;
            break;
        case Qt::Key_Escape:
        case Qt::Key_P:
            m_pauseTrigger = true;
            break;
        default:
            break;
    }
}

void InputHandler::onKeyReleased(int key) {
    switch (key) {
        case Qt::Key_D:
        case Qt::Key_Right:
            m_keyGas = false;
            break;
        case Qt::Key_A:
        case Qt::Key_Left:
            m_keyBrake = false;
            break;
        default:
            break;
    }
}

void InputHandler::setPedalGas(bool pressed) {
    m_pedalGas = pressed;
}

void InputHandler::setPedalBrake(bool pressed) {
    m_pedalBrake = pressed;
}

bool InputHandler::isGasActive() const {
    return m_keyGas || m_pedalGas;
}

bool InputHandler::isBrakeActive() const {
    return m_keyBrake || m_pedalBrake;
}

float InputHandler::getThrottle() const {
    float throttle = 0.0f;
    if (isGasActive()) {
        throttle += 1.0f;
    }
    if (isBrakeActive()) {
        throttle -= 1.0f;
    }
    return throttle * m_config.keyboardThrottleSensitivity;
}

bool InputHandler::consumeRestartTrigger() {
    bool trigger = m_restartTrigger;
    m_restartTrigger = false;
    return trigger;
}

bool InputHandler::consumePauseTrigger() {
    bool trigger = m_pauseTrigger;
    m_pauseTrigger = false;
    return trigger;
}

} // namespace Core
