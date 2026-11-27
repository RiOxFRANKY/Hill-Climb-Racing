#pragma once

#include "game_controller.h"

#include <QImage>
#include <QPointer>
#include <QQuickItem>

// Draws the visible ground. The pixel-art image is built on the CPU each frame
// and shown as a nearest-filtered texture, positioned by the item itself.
class TerrainItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GameController *game READ game WRITE setGame NOTIFY gameChanged)

public:
    explicit TerrainItem(QQuickItem *parent = nullptr);

    [[nodiscard]] GameController *game() const { return m_game; }
    void setGame(GameController *game);

signals:
    void gameChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    void refresh();

    QPointer<GameController> m_game;
    QMetaObject::Connection m_frameConnection;
    QMetaObject::Connection m_courseConnection;
    QImage m_image;
    bool m_imageDirty = false;
};
