#pragma once

#include <QQuickImageProvider>

// Sprites drawn with QPainter rather than loaded from a file, served to QML as
// image://sprites/<name>.
class SpriteProvider : public QQuickImageProvider
{
public:
    SpriteProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};
