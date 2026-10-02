// ============================================================================
// ASSET SPECIFICATION & DIMENSION STANDARDS: UI & BACKGROUNDS
// ============================================================================
// File Path: assets/backgrounds/sky_bg.png
// Native Resolution: 960 x 540 px | Aspect Ratio: 16:9
// Description: Parallax sky backdrop with distant mountain silhouettes.
//
// File Path: assets/ui/meter_dial.png
// Native Resolution: 64 x 64 px | Aspect Ratio: 1:1
// Description: Speedometer circular dial gauge.
//
// File Path: assets/ui/dial_needle.png
// Native Resolution: 8 x 32 px | Aspect Ratio: 1:4 (Pivot at x=4, y=28)
// Description: Orange-red indicator pointer needle.
//
// File Path: assets/ui/pedal_gas_released.png
// Native Resolution: 36 x 60 px | Aspect Ratio: 3:5
//
// File Path: assets/ui/pedal_gas_pressed.png
// Native Resolution: 36 x 60 px | Aspect Ratio: 3:5
//
// File Path: assets/ui/pedal_brake_released.png
// Native Resolution: 36 x 60 px | Aspect Ratio: 3:5
//
// File Path: assets/ui/pedal_brake_pressed.png
// Native Resolution: 36 x 60 px | Aspect Ratio: 3:5
//
// WARNING: Any replacement PNG MUST strictly match these native dimensions.
// ============================================================================

#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include "core/GameEngine.h"
#include "render/SpriteManager.h"

namespace Render {

class GameWidget : public QWidget {
    Q_OBJECT

public:
    // ========================================================================
    // TWEAKABLE PARAMETERS (Virtual canvas, window sizes & UI layout)
    // ========================================================================
    struct Config {
        // Resolutions
        int virtualCanvasWidth{960};
        int virtualCanvasHeight{540};
        int defaultWindowWidth{1920};
        int defaultWindowHeight{1080};

        // Framerate
        int targetFps{60};

        // UI Interactive pedal layout (In 960x540 coordinate space)
        QRect pedalBrakeRect{30, 440, 54, 80}; // Bottom-left brake pedal
        QRect pedalGasRect{876, 440, 54, 80};  // Bottom-right gas pedal

        // HUD Layout
        int hudMargin{20};
        int fuelBarWidth{180};
        int fuelBarHeight{20};
        int dialSize{64};
    };

    explicit GameWidget(QWidget* parent = nullptr);
    explicit GameWidget(const Config& config, QWidget* parent = nullptr);
    ~GameWidget() override = default;

    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void setupEngine();
    void renderScene(QPainter& painter);

    // Render passes
    void paintBackground(QPainter& painter, float camX);
    void paintTerrain(QPainter& painter, float camX, float camY);
    void paintCollectibles(QPainter& painter);
    void paintVehicle(QPainter& painter);
    void paintHUD(QPainter& painter);
    void paintBanners(QPainter& painter);

    // Procedural Fallbacks (Graceful zero-asset safety net)
    void drawFallbackSky(QPainter& painter);
    void drawFallbackChassis(QPainter& painter);
    void drawFallbackWheel(QPainter& painter);
    void drawFallbackDriverHead(QPainter& painter);
    void drawFallbackCoin(QPainter& painter, float x, float y);
    void drawFallbackFuelCan(QPainter& painter, float x, float y);
    void drawFallbackDial(QPainter& painter, int x, int y, float speedKmH);
    void drawFallbackPedal(QPainter& painter, const QRect& rect, bool isGas, bool isPressed);

    // Coordinate conversion
    QPoint mapWindowToCanvas(const QPoint& windowPos) const;
    void updatePedalTouch(const QPoint& canvasPos, bool pressed);

    Config m_config;
    Core::GameEngine m_engine;
    SpriteManager m_sprites;
    QTimer m_gameTimer;
    QImage m_canvasBuffer;

    QRect m_lastViewportRect;
};

} // namespace Render
