#pragma once

#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace Terrain {

constexpr double GapFloor = -20000.0;
constexpr int TerrainPixel = 4;

struct Pickup {
    enum class Type { Coin, Fuel };
    Type type = Type::Coin;
    QPointF position;
    bool collected = false;
    int value = 0;  // coins only: 5, 25, 100 or 500
};

struct TerrainChain {
    double startX = 0.0;
    double step = 0.0;
    QVector<double> heights;

    [[nodiscard]] double endX() const { return startX + step * (heights.size() - 1); }
};

struct TerrainGap {
    double start = 0.0;
    double end = 0.0;
};

struct CourseSection {
    double start = 0.0;
    double end = 0.0;
    QString name;
};

quint32 hashValue(qint64 value);

class TerrainManager {
public:
    TerrainManager() = default;

    void loadCourse(const QString &resourcePath = QStringLiteral(":/assets/terrain_10km.json"));
    // Build a 10 km course from the procedural generator for one of the five
    // maps (0 = Grazing the Grass ... 4 = The Real Excursion, each harder than
    // the last). The same seed and map always give the same course.
    void generateCourse(quint32 seed, int map = 0);
    // Start somewhere other than the course's own spawn point (world pixels).
    // A point inside a ravine moves to its landing side.
    void setSpawnX(double x);

    [[nodiscard]] const TerrainChain *chainAt(double x) const;
    [[nodiscard]] double chainHeight(const TerrainChain &chain, double x) const;
    [[nodiscard]] const TerrainGap *gapAt(double x) const;
    [[nodiscard]] int sectionAt(double x) const;
    [[nodiscard]] double surfaceHeight(double x) const;
    [[nodiscard]] double surfaceSlope(double x) const;
    [[nodiscard]] double terrainHeight(double x) const;

    [[nodiscard]] bool findTerrainContact(const QPointF &center, double radius,
                                          QPointF *normal, double *depth) const;

    void updatePickups(const QPointF &chassisPos, const QPointF &headPos, double cameraX, int &coins, double &fuel, int &score);
    void ensurePickupsAhead(double cameraX);
    void resetPickups(double spawnX);

    [[nodiscard]] double pixelsPerMetre() const { return m_pixelsPerMetre; }
    [[nodiscard]] double spawnX() const { return m_spawnX; }
    [[nodiscard]] double finishX() const { return m_finishX; }
    [[nodiscard]] const QVector<TerrainChain> &chains() const { return m_chains; }
    [[nodiscard]] const QVector<TerrainGap> &gaps() const { return m_gaps; }
    [[nodiscard]] const QVector<CourseSection> &sections() const { return m_sections; }
    [[nodiscard]] const QVector<Pickup> &pickups() const { return m_pickups; }

private:
    double m_pixelsPerMetre = 80.0;
    double m_spawnX = 800.0;
    double m_finishX = 800000.0;

    QVector<TerrainChain> m_chains;
    QVector<TerrainGap> m_gaps;
    QVector<CourseSection> m_sections;
    QVector<Pickup> m_pickups;

    // Where the fuel can after one at x goes, once budget "flat metres" of
    // fuel are used up; see FuelBudget in terrain.cpp.
    [[nodiscard]] double nextFuelX(double from, double budget) const;

    double m_nextPickupX = 0.0;  // where the next coin group may start
    double m_nextFuelX = 0.0;    // where the next fuel can goes
    qint64 m_coinGroup = 0;      // counts coin groups, to vary them
    double m_fuelBudgetScale = 1.0;  // below 1 on the hardest maps: cans come sooner
};

} // namespace Terrain
