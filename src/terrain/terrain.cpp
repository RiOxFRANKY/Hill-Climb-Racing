#include "terrain.h"
#include "procedural.h"
#include "../physics/physics.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVarLengthArray>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Terrain {

constexpr double DesignWidth = 1920.0;
constexpr double Metre = Physics::PixelsPerMetre;

// Fuel can spacing. Fuel burns by time (see GameCore::updatePhysics), so a
// metre of climbing costs more than a metre of flat ground, a metre of
// downhill less, and bumpy ground (rollers, ledges) more because it slows the
// car. The next can goes where a driver on the gas would have used about the
// same fuel since the last one, measured in "flat metres": the aim is to
// reach every can with roughly 10-20% of the tank left. These numbers were
// tuned with a headless drive over several seeds.
constexpr double FuelBudget = 650.0;     // flat metres between cans
constexpr double ClimbCost = 2.0;        // extra cost per metre, per unit of uphill gradient
constexpr double DescentSaving = 1.2;    // saving per metre, per unit of downhill gradient
constexpr double BumpCost = 6.0;         // extra cost per metre, per unit of gradient change
constexpr double MinCostPerMetre = 0.6;

// Coin groups are worth more further along the course. roll is in [0, 1).
int coinValueAt(double x, double roll)
{
    const double km = std::max(0.0, x / Metre) / 1000.0;
    const double chance500 = 0.15 * Physics::clampValue((km - 5.0) / 4.0, 0.0, 1.0);
    const double chance100 = 0.35 * Physics::clampValue((km - 2.5) / 4.0, 0.0, 1.0);
    const double chance25 = 0.45 * Physics::clampValue(km / 2.0, 0.0, 1.0);
    if (roll < chance500)
        return 500;
    if (roll < chance500 + chance100)
        return 100;
    if (roll < chance500 + chance100 + chance25)
        return 25;
    return 5;
}

quint32 hashValue(qint64 value)
{
    quint64 x = static_cast<quint64>(value) + 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return static_cast<quint32>(x ^ (x >> 31));
}

void TerrainManager::loadCourse(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly))
        qFatal("Missing course data %s", qPrintable(resourcePath));
    const QJsonObject course = QJsonDocument::fromJson(file.readAll()).object();

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
}

void TerrainManager::generateCourse(quint32 seed, int map)
{
    Procedural::Settings settings = Procedural::Settings::forMap(map, seed);
    settings.step = 0.25;  // 20 px, the same spacing as the JSON course
    const Procedural::Course course = Procedural::generate(settings);

    m_pixelsPerMetre = Physics::PixelsPerMetre;
    const double scale = m_pixelsPerMetre;
    m_spawnX = course.spawnX * scale;
    m_finishX = course.finishX * scale;

    m_chains.clear();
    for (const Procedural::Chain &source : course.chains) {
        if (source.heights.size() < 2)
            continue;
        TerrainChain chain;
        chain.startX = source.startX * scale;
        chain.step = course.step * scale;
        chain.heights.reserve(static_cast<qsizetype>(source.heights.size()));
        for (double height : source.heights)
            chain.heights.append(height * scale);
        m_chains.append(chain);
    }

    m_gaps.clear();
    for (const Procedural::Ravine &ravine : course.ravines)
        m_gaps.append({ravine.takeoff * scale, ravine.landing * scale});

    m_sections.clear();
    for (const Procedural::Zone &zone : course.zones)
        m_sections.append({zone.start * scale, zone.end * scale, QString::fromStdString(zone.name)});
}

void TerrainManager::setSpawnX(double x)
{
    x = Physics::clampValue(x, 0.0, m_finishX - 400.0);
    if (const TerrainGap *gap = gapAt(x))
        x = gap->end + 200.0;
    m_spawnX = x;
}

void TerrainManager::resetPickups(double spawnX)
{
    // Pickups are always laid out from the start of the course, so every can
    // and coin group sits at the same place on a given course however far
    // along the car starts. Anything behind the car is then dropped.
    m_pickups.clear();
    m_nextPickupX = 22.0 * Metre;
    // The first can comes a little sooner: the car starts from a standstill.
    m_nextFuelX = nextFuelX(0.0, 0.85 * FuelBudget);
    m_coinGroup = 0;
    ensurePickupsAhead(spawnX - 560.0);
    const double keepFrom = spawnX - 300.0;
    m_pickups.erase(std::remove_if(m_pickups.begin(), m_pickups.end(),
                                   [keepFrom](const Pickup &p) { return p.position.x() < keepFrom; }),
                    m_pickups.end());
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
                coins += pickup.value;
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

double TerrainManager::nextFuelX(double from, double budget) const
{
    // Walk forward a metre at a time, charging each metre by the gradient
    // averaged over the car's length and by how fast that gradient changes,
    // until the fuel budget is spent.
    constexpr double Step = Metre;
    constexpr double HalfCar = 2.25 * Metre;
    auto gradientAt = [this](double x) {
        return (surfaceHeight(x + HalfCar) - surfaceHeight(x - HalfCar)) / (2.0 * HalfCar);
    };
    double cost = 0.0;
    double x = from;
    double gradient = gradientAt(x);
    while (cost < budget && x < m_finishX) {
        const double nextGradient = gradientAt(x + Step);
        cost += std::max(MinCostPerMetre, 1.0 + ClimbCost * std::max(0.0, gradient)
                                              - DescentSaving * std::max(0.0, -gradient)
                                              + BumpCost * std::abs(nextGradient - gradient));
        gradient = nextGradient;
        x += Step;
    }
    return x;
}

void TerrainManager::ensurePickupsAhead(double cameraX)
{
    constexpr double CoinSpacing = 55.0;
    const double horizon = std::min(cameraX + DesignWidth + 900.0, m_finishX - 400.0);

    // Moves [x, x + span] past any ravine, keeping clear of its run-up and landing.
    auto clearOfGaps = [this](double x, double span) {
        for (const TerrainGap &gap : m_gaps) {
            if (x + span > gap.start - 500.0 && x < gap.end + 150.0)
                x = gap.end + 150.0;
        }
        return x;
    };

    // Coin groups and fuel cans are placed in order along the course, so the
    // pickup list stays sorted by x.
    while (std::min(m_nextPickupX, m_nextFuelX) < horizon) {
        if (m_nextFuelX <= m_nextPickupX) {
            const double x = clearOfGaps(m_nextFuelX, 0.0);
            if (x != m_nextFuelX) {
                m_nextFuelX = x;
                continue;
            }
            m_pickups.push_back({Pickup::Type::Fuel, QPointF(x, surfaceHeight(x) + 65.0), false, 0});
            m_nextFuelX = nextFuelX(x, FuelBudget);
            // Leave the can on its own rather than inside a row of coins.
            m_nextPickupX = std::max(m_nextPickupX, x + 4.0 * Metre);
            continue;
        }

        const quint32 hash = hashValue(m_coinGroup);
        const int count = 5 + static_cast<int>(hash % 4);  // 5 to 8 coins
        const double span = (count - 1) * CoinSpacing;
        const double start = clearOfGaps(m_nextPickupX, span);
        if (start != m_nextPickupX) {
            m_nextPickupX = start;
            continue;
        }
        if (start + span > m_nextFuelX - 3.0 * Metre && start < m_nextFuelX + 4.0 * Metre) {
            // The group would crowd the next fuel can; start after it instead.
            m_nextPickupX = m_nextFuelX + 4.0 * Metre;
            continue;
        }

        // The row follows the ground: each coin sits a fixed distance out
        // along the ground's normal, and neighbours are evenly spaced along
        // the slope rather than horizontally.
        constexpr double CoinLift = 70.0;
        const int value = coinValueAt(start, ((hash >> 8) & 0xFFFF) / 65536.0);
        double x = start;
        for (int i = 0; i < count; ++i) {
            const double slope = surfaceSlope(x);
            const double stretch = std::sqrt(1.0 + slope * slope);
            const QPointF normal(-slope / stretch, 1.0 / stretch);  // y is up
            m_pickups.push_back({Pickup::Type::Coin, QPointF(x, surfaceHeight(x)) + normal * CoinLift, false, value});
            x += CoinSpacing / stretch;
        }
        ++m_coinGroup;
        // 45 to 110 m of open road before the next group.
        const double gap = 45.0 + 65.0 * (((hash >> 24) & 0xFF) / 255.0);
        m_nextPickupX = start + span + gap * Metre;
    }
}

} // namespace Terrain
