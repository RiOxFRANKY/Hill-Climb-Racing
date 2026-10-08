#pragma once

#include "../core/game_core.h"

#include <QFont>
#include <QPainter>
#include <QPixmap>
#include <QRectF>

namespace Render {

class GameRenderer {
public:
    GameRenderer() = default;

    void loadAssets();

    [[nodiscard]] QPointF worldToScreen(const QPointF &world, double cameraX, double cameraY, int designHeight) const;

    // UPDATED: Now takes the GameCore reference so it can check core.isMoonLevel()
    void drawBackground(QPainter &painter, const Core::GameCore &core, double cameraX, int designWidth, int designHeight) const;
    void drawTerrain(QPainter &painter, const Terrain::TerrainManager &terrain, double cameraX, double cameraY, int designWidth, int designHeight) const;
    void drawFinishLine(QPainter &painter, const Terrain::TerrainManager &terrain, double cameraX, double cameraY, int designWidth, int designHeight) const;
    void drawPickups(QPainter &painter, const Terrain::TerrainManager &terrain, double survivalTime, double cameraX, double cameraY, int designWidth, int designHeight) const;
    void drawCar(QPainter &painter, const Vehicle::VehicleEntity &vehicle, double cameraX, double cameraY, int designHeight) const;
    void drawHud(QPainter &painter, const Core::GameCore &core, int designWidth, int designHeight) const;
    void drawHangingBanner(QPainter &painter, const Core::GameCore &core, int designWidth) const;
    void drawOverlay(QPainter &painter, const Core::GameCore &core, const QRectF &rect) const;
    void drawPedals(QPainter &painter, const Core::GameCore &core) const;
    void drawSpeedometer(QPainter &painter, const Core::GameCore &core) const;

    [[nodiscard]] QFont pixelFont(int pointSize, bool bold = false) const;
    [[nodiscard]] QFont uiFont(int pointSize, bool bold = false) const;

private:
    QPixmap m_carBody;
    QPixmap m_wheelSprite;
    QPixmap m_backgroundStrip;
    QPixmap m_bgSky;
    QPixmap m_bgMountains;
    QPixmap m_bgHills;
    QPixmap m_coinPixmap;

    // Pixel Art UI Assets
    QPixmap m_pedalBrake;
    QPixmap m_pedalBrakePressed;
    QPixmap m_pedalGas;
    QPixmap m_pedalGasPressed;
    QPixmap m_pedalBoost;
    QPixmap m_pedalBoostPressed;
    QPixmap m_speedoDial;
    QPixmap m_speedoNeedle;
    QPixmap m_woodenBoard;
    QPixmap m_chainLink;
    QPixmap m_hudPanel;
    QPixmap m_dialogBox;
    QPixmap m_dialogButton;
    QPixmap m_dialogButtonPressed;
    QPixmap m_coinIcon;

    QString m_pixelFontFamily;
    QString m_uiFontFamily;
};

} // namespace Render