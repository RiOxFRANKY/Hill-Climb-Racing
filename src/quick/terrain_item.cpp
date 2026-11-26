#include "terrain_item.h"

#include "../render/terrain_raster.h"

#include <QQuickWindow>
#include <QSGImageNode>

TerrainItem::TerrainItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void TerrainItem::setGame(GameController *game)
{
    if (m_game == game)
        return;
    disconnect(m_frameConnection);
    m_game = game;
    if (m_game)
        m_frameConnection = connect(m_game, &GameController::frameChanged, this, &TerrainItem::refresh);
    // A new map may have a different ground style; frameChanged also fires
    // then, but redraw at once so the menu never shows the old look.
    if (m_game)
        connect(m_game, &GameController::courseChanged, this, &TerrainItem::refresh);
    refresh();
    emit gameChanged();
}

void TerrainItem::refresh()
{
    if (!m_game)
        return;
    const Core::GameCore &core = m_game->core();
    Render::TerrainRaster raster = Render::rasterizeTerrain(core.terrain(), core.cameraX(), core.cameraY(),
                                                            GameController::DesignWidth, GameController::DesignHeight,
                                                            Render::groundStyleForMap(m_game->map()));
    setPosition(raster.placement.topLeft());
    setSize(raster.placement.size());
    m_image = std::move(raster.image);
    m_imageDirty = true;
    update();
}

QSGNode *TerrainItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto *node = static_cast<QSGImageNode *>(oldNode);
    if (m_imageDirty && !m_image.isNull()) {
        // A fresh node owns the fresh texture, so the previous frame's texture
        // is released with the old node.
        delete node;
        node = window()->createImageNode();
        node->setTexture(window()->createTextureFromImage(m_image));
        node->setOwnsTexture(true);
        node->setFiltering(QSGTexture::Nearest);
        m_imageDirty = false;
    }
    if (node)
        node->setRect(boundingRect());
    return node;
}
