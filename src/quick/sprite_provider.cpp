#include "sprite_provider.h"

#include <QPainter>

namespace {

// Rendered at twice its on-screen size. The can's centre sits at (35, 55) in
// the 70x100 area QML shows it in.
constexpr int SpriteScale = 2;

QImage fuelCan()
{
    QImage image(70 * SpriteScale, 100 * SpriteScale, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.scale(SpriteScale, SpriteScale);
    painter.translate(35.0, 55.0);
    painter.setPen(QPen(QColor(80, 27, 25), 5.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QColor(214, 49, 43));
    painter.drawRoundedRect(QRectF(-31.0, -38.0, 62.0, 76.0), 10.0, 10.0);
    painter.setBrush(QColor(71, 75, 74));
    painter.drawRoundedRect(QRectF(4.0, -51.0, 23.0, 17.0), 4.0, 4.0);
    painter.setPen(QPen(QColor(255, 235, 215), 6.0));
    painter.drawLine(QPointF(-16.0, -18.0), QPointF(16.0, 18.0));
    painter.drawLine(QPointF(16.0, -18.0), QPointF(-16.0, 18.0));
    return image;
}

} // namespace

SpriteProvider::SpriteProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage SpriteProvider::requestImage(const QString &id, QSize *size, const QSize &)
{
    const QImage image = id == QLatin1String("fuel_can") ? fuelCan() : QImage();
    if (size)
        *size = image.size();
    return image;
}
