#pragma once

namespace Core {

class InputHandler {
public:
    // ========================================================================
    // TWEAKABLE PARAMETERS
    // ========================================================================
    struct Config {
        float keyboardThrottleSensitivity{1.0f}; // Responsive binary throttle
    };

    InputHandler();
    explicit InputHandler(const Config& config);

    void reset();

    // Key event dispatchers (Qt key codes passed in)
    void onKeyPressed(int key);
    void onKeyReleased(int key);

    // On-screen / mouse pedal touch dispatchers
    void setPedalGas(bool pressed);
    void setPedalBrake(bool pressed);

    // State queries
    float getThrottle() const;
    bool isGasActive() const;
    bool isBrakeActive() const;

    // Single-frame action triggers (consumed upon query)
    bool consumeRestartTrigger();
    bool consumePauseTrigger();

    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

private:
    Config m_config;

    bool m_keyGas{false};
    bool m_keyBrake{false};
    bool m_pedalGas{false};
    bool m_pedalBrake{false};

    bool m_restartTrigger{false};
    bool m_pauseTrigger{false};
};

} // namespace Core
