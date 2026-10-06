#pragma once

#include "../core/game_core.h"

#include <QPainter>
#include <QPixmap>
#include <QRectF>

namespace Render {

class GameRenderer {
public:
    GameRenderer() = default;

    void loadAssets();

    [[nodiscard]] QPointF worldToScreen(const QPointF &world, double cameraX, double cameraY, int designHeight) const;

    void drawBackground(QPainter &painter, double cameraX, int designWidth, int designHeight) const;
    void drawTerrain(QPainter &painter, const Terrain::TerrainManager &terrain, double cameraX, double cameraY, int designWidth, int designHeight) const;
    void drawFinishLine(QPainter &painter, const Terrain::TerrainManager &terrain, double cameraX, double cameraY, int designWidth, int designHeight) const;
    void drawPickups(QPainter &painter, const Terrain::TerrainManager &terrain, double survivalTime, double cameraX, double cameraY, int designWidth, int designHeight) const;
    void drawCar(QPainter &painter, const Vehicle::VehicleEntity &vehicle, double cameraX, double cameraY, int designHeight) const;
    void drawHud(QPainter &painter, const Core::GameCore &core, int designWidth, int designHeight) const;
    void drawOverlay(QPainter &painter, const Core::GameCore &core, const QRectF &rect) const;
    void drawKeyHint(QPainter &painter, const QRectF &rect, const QString &key, const QString &label, bool active) const;

private:
    QPixmap m_carBody;
    QPixmap m_wheelSprite;
    QPixmap m_backgroundStrip;
};

} // namespace Render
