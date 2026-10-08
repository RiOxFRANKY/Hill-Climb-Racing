#include "terrain.h"
#include "../physics/physics.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVarLengthArray>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>
#include <QObject>

#include <algorithm>
#include <cmath>
#include <limits>

// Standalone function that reads the file on the background thread
QJsonDocument parseTerrainJson(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open terrain file:" << filePath;
        return QJsonDocument(); 
    }
    return QJsonDocument::fromJson(file.readAll());
}

namespace Terrain {

constexpr double DesignWidth = 1920.0;
constexpr double FuelSpacing = 250.0 * Physics::PixelsPerMetre;

quint32 hashValue(qint64 value)
{
    quint64 x = static_cast<quint64>(value) + 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return static_cast<quint32>(x ^ (x >> 31));
}

void TerrainManager::loadCourse(const QString &resourcePath)
{
    // 1. Create a watcher to monitor the background thread
    QFutureWatcher<QJsonDocument>* watcher = new QFutureWatcher<QJsonDocument>();

    // 2. Extract coordinates when the thread finishes loading the JSON
    QObject::connect(watcher, &QFutureWatcher<QJsonDocument>::finished, [this, watcher, resourcePath]() {
        QJsonDocument loadedDoc = watcher->result();
        
        if (loadedDoc.isNull()) {
            qFatal("Missing or invalid course data %s", qPrintable(resourcePath));
        }
        
        const QJsonObject course = loadedDoc.object();

        m_pixelsPerMetre = course.value(QStringLiteral("pixels_per_metre")).toDouble(Physics::PixelsPerMetre);
        const double scale = m_pixelsPerMetre;
        m_spawnX = course.value(QStringLiteral("spawn_x_m")).toDouble() * scale;
        m_finishX = course.value(QStringLiteral("finish_x_m")).toDouble() * scale;

        m_chains.clear();
        const QJsonArray polylines = course.value(QStringLiteral("polylines_m")).toArray();
        for (const QJsonValue &polylineValue : polylines) {
            const QJsonArray polyline = polylineValue.toArray();
            if (polyline.size() < 2)
                continue;
            TerrainChain chain;
            chain.startX = polyline.at(0).toArray().at(0).toDouble() * scale;
            chain.step = (polyline.at(1).toArray().at(0).toDouble()
                          - polyline.at(0).toArray().at(0).toDouble()) * scale;
            chain.heights.reserve(polyline.size());
            for (const QJsonValue &pointValue : polyline) {
                const QJsonArray point = pointValue.toArray();
                const double x = point.at(0).toDouble() * scale;
                if (std::abs(x - (chain.startX + chain.step * chain.heights.size())) > 1e-6)
                    qFatal("Course chain is not uniformly sampled");
                chain.heights.append(point.at(1).toDouble() * scale);
            }
            m_chains.append(chain);
        }

        m_gaps.clear();
        for (const QJsonValue &gapValue : course.value(QStringLiteral("gap_ranges_m")).toArray()) {
            const QJsonArray gap = gapValue.toArray();
            m_gaps.append({gap.at(0).toDouble() * scale, gap.at(1).toDouble() * scale});
        }

        m_sections.clear();
        const QJsonArray bounds = course.value(QStringLiteral("section_bounds_m")).toArray();
        const QJsonArray names = course.value(QStringLiteral("section_names")).toArray();
        for (int i = 0; i < bounds.size(); ++i) {
            const QJsonArray range = bounds.at(i).toArray();
            m_sections.append({range.at(0).toDouble() * scale, range.at(1).toDouble() * scale,
                               names.at(i).toString().toUpper()});
        }
        
        qDebug() << "Terrain successfully loaded and arrays populated in background!";
        watcher->deleteLater(); // Clean up memory
    });

    // 3. Start the heavy parsing function on a separate thread
    qDebug() << "Starting background JSON processing for:" << resourcePath;
    QFuture<QJsonDocument> future = QtConcurrent::run(parseTerrainJson, resourcePath);
    watcher->setFuture(future);
}

void TerrainManager::resetPickups(double spawnX)
{
    m_pickups.clear();
    m_nextPickupX = spawnX + 400.0;
    m_nextFuelX = spawnX + FuelSpacing * 0.6;
    ensurePickupsAhead(spawnX - 560.0);
}

const TerrainChain *TerrainManager::chainAt(double x) const
{
    auto it = std::upper_bound(m_chains.cbegin(), m_chains.cend(), x,
                               [](double value, const TerrainChain &chain) {
                                   return value < chain.startX;
                               });
    if (it == m_chains.cbegin())
        return nullptr;
    --it;
    return x <= it->endX() ? &*it : nullptr;
}

double TerrainManager::chainHeight(const TerrainChain &chain, double x) const
{
    const double position = Physics::clampValue((x - chain.startX) / chain.step, 0.0,
                                                static_cast<double>(chain.heights.size() - 1));
    const int index = std::min(static_cast<int>(position), static_cast<int>(chain.heights.size()) - 2);
    const double t = position - index;
    return chain.heights.at(index) + (chain.heights.at(index + 1) - chain.heights.at(index)) * t;
}

const TerrainGap *TerrainManager::gapAt(double x) const
{
    auto it = std::upper_bound(m_gaps.cbegin(), m_gaps.cend(), x,
                               [](double value, const TerrainGap &gap) {
                                   return value < gap.start;
                               });
    if (it == m_gaps.cbegin())
        return nullptr;
    --it;
    return x > it->start && x < it->end ? &*it : nullptr;
}

int TerrainManager::sectionAt(double x) const
{
    for (int i = 0; i < m_sections.size(); ++i) {
        if (x < m_sections.at(i).end)
            return i;
    }
    return static_cast<int>(m_sections.size()) - 1;
}

double TerrainManager::surfaceHeight(double x) const
{
    if (m_chains.isEmpty())
        return 0.0;
    if (const TerrainChain *chain = chainAt(x))
        return chainHeight(*chain, x);
    if (x < m_chains.constFirst().startX)
        return m_chains.constFirst().heights.constFirst();
    for (int i = 1; i < m_chains.size(); ++i) {
        const TerrainChain &before = m_chains.at(i - 1);
        const TerrainChain &after = m_chains.at(i);
        if (x > before.endX() && x < after.startX) {
            const double t = (x - before.endX()) / (after.startX - before.endX());
            return before.heights.constLast() + (after.heights.constFirst() - before.heights.constLast()) * t;
        }
    }
    return m_chains.constLast().heights.constLast();
}

double TerrainManager::surfaceSlope(double x) const
{
    const TerrainChain *chain = chainAt(x);
    if (!chain)
        return 0.0;
    const double x0 = std::max(chain->startX, x - chain->step);
    const double x1 = std::min(chain->endX(), x + chain->step);
    if (x1 - x0 < 1e-9)
        return 0.0;
    return (chainHeight(*chain, x1) - chainHeight(*chain, x0)) / (x1 - x0);
}

double TerrainManager::terrainHeight(double x) const
{
    return gapAt(x) ? GapFloor : surfaceHeight(x);
}

bool TerrainManager::findTerrainContact(const QPointF &center, double radius,
                                         QPointF *normal, double *depth) const
{
    const double step = m_chains.isEmpty() ? 20.0 : m_chains.constFirst().step;
    const double reach = radius + 6.0;
    const double left = center.x() - reach;
    const double right = center.x() + reach;

    QVarLengthArray<double, 64> xs;
    xs.append(left);
    for (double x = std::floor(left / step) * step + step; x < right; x += step)
        xs.append(x);
    xs.append(right);
    for (const TerrainGap &gap : m_gaps) {
        if (gap.start > right)
            break;
        if (gap.start > left)
            xs.append(gap.start);
        if (gap.end > left && gap.end < right)
            xs.append(gap.end);
    }
    std::sort(xs.begin(), xs.end());

    QVarLengthArray<QPointF, 72> outline;
    bool previousInGap = false;
    for (int i = 0; i < xs.size(); ++i) {
        const double x = xs.at(i);
        const bool inGap = gapAt(x) != nullptr;
        if (i > 0 && inGap != previousInGap)
            outline.append(QPointF(inGap ? xs.at(i - 1) : x, GapFloor));
        outline.append(QPointF(x, inGap ? GapFloor : surfaceHeight(x)));
        previousInGap = inGap;
    }

    double bestDistanceSq = std::numeric_limits<double>::max();
    QPointF bestPoint;
    QPointF bestOutward;
    for (int i = 1; i < outline.size(); ++i) {
        const QPointF a = outline.at(i - 1);
        const QPointF segment = outline.at(i) - a;
        const double segmentLengthSq = QPointF::dotProduct(segment, segment);
        if (segmentLengthSq < 1e-12)
            continue;
        const double t = Physics::clampValue(QPointF::dotProduct(center - a, segment) / segmentLengthSq, 0.0, 1.0);
        const QPointF closest = a + segment * t;
        const QPointF toCenter = center - closest;
        const double distanceSq = QPointF::dotProduct(toCenter, toCenter);
        if (distanceSq < bestDistanceSq) {
            bestDistanceSq = distanceSq;
            bestPoint = closest;
            bestOutward = QPointF(-segment.y(), segment.x()) / std::sqrt(segmentLengthSq);
        }
    }
    if (outline.size() < 2)
        return false;

    const double distance = std::sqrt(bestDistanceSq);
    const bool inside = center.y() < terrainHeight(center.x());
    if (inside) {
        *normal = distance > 1e-9 ? (bestPoint - center) / distance : bestOutward;
        *depth = radius + distance;
        return true;
    }
    if (distance >= radius)
        return false;

    *normal = distance > 1e-9 ? (center - bestPoint) / distance : bestOutward;
    *depth = radius - distance;
    return true;
}

void TerrainManager::updatePickups(const QPointF &chassisPos, const QPointF &headPos, double cameraX, int &coins, double &fuel, int &score)
{
    for (Pickup &pickup : m_pickups) {
        if (pickup.collected)
            continue;

        const QPointF deltaChassis = pickup.position - chassisPos;
        const QPointF deltaHead = pickup.position - headPos;
        if (QPointF::dotProduct(deltaChassis, deltaChassis) < 86.0 * 86.0
            || QPointF::dotProduct(deltaHead, deltaHead) < 64.0 * 64.0) {
            pickup.collected = true;
            if (pickup.type == Pickup::Type::Coin) {
                ++coins;
            } else {
                fuel = 100.0;
                score += 250;
            }
        }
    }

    while (!m_pickups.isEmpty()
           && (m_pickups.first().collected
               || m_pickups.first().position.x() < cameraX - 300.0)) {
        m_pickups.removeFirst();
    }
}

void TerrainManager::ensurePickupsAhead(double cameraX)
{
    const double horizon = std::min(cameraX + DesignWidth + 900.0, m_finishX - 400.0);
    while (m_nextPickupX < horizon) {
        bool nearGap = false;
        for (const TerrainGap &gap : m_gaps) {
            if (m_nextPickupX > gap.start - 500.0 && m_nextPickupX < gap.end + 150.0) {
                m_nextPickupX = gap.end + 150.0;
                nearGap = true;
            }
        }
        if (nearGap)
            continue;

        const int group = static_cast<int>(m_nextPickupX / 420.0);
        const bool placeFuel = m_nextPickupX >= m_nextFuelX;

        if (placeFuel) {
            m_nextFuelX += FuelSpacing;
            m_pickups.push_back({Pickup::Type::Fuel,
                                 QPointF(m_nextPickupX,
                                         surfaceHeight(m_nextPickupX) + 65.0),
                                 false});
            m_nextPickupX += 430.0;
        } else {
            const int count = 3 + (group % 3);
            for (int i = 0; i < count; ++i) {
                const double x = m_nextPickupX + i * 55.0;
                const double arc = std::sin((i + 1.0) / (count + 1.0) * Physics::Pi) * 32.0;
                m_pickups.push_back({Pickup::Type::Coin,
                                     QPointF(x, surfaceHeight(x) + 68.0 + arc),
                                     false});
            }
            m_nextPickupX += count * 55.0 + 285.0;
        }
    }
}

} // namespace Terrain