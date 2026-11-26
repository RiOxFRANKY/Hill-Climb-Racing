#pragma once

// Procedural course generator.
//
// The ground is built from two parts, both pure functions of distance:
//
//  * Waves: a few layers of domain-warped sine waves (long swells, rolling
//    hills, short bumps) plus a slow climb or descent per zone. They give the
//    ground its constant, uneven texture.
//  * Set pieces: separate shapes placed one after another along the course,
//    such as big hills, mesas, valleys, rollers, ledges and kicker ramps. They
//    give the course landmarks and moments that need throttle control.
//
// A seed shifts every wave and chooses every set piece, so each seed gives a
// different but fully reproducible course.
//
// This file has no Qt dependency so the generator can be tested on its own.
// Units are metres, x to the right and height up.

#include <cstdint>
#include <string>
#include <vector>

namespace Terrain::Procedural {

// The always-on wave layers of a zone.
struct WaveProfile {
    double swellAmp = 0.0;       // long, gentle waves
    double swellLength = 300.0;  // their wavelength in metres
    double hillAmp = 0.0;        // ordinary rolling hills
    double hillLength = 60.0;
    double bumpAmp = 0.0;        // short bumps on top
    double bumpLength = 9.0;
    double warp = 0.0;           // how much the hills are bent out of a pure sine
};

// Which set pieces a zone uses, how big they are and how often they come.
// The six weights are relative chances; zero leaves that kind out.
struct FeatureProfile {
    double spacing = 80.0;    // average plain ground between set pieces (m)
    double size = 1.0;        // scales every set piece's height
    double steepness = 0.6;   // steepest gradient a set piece aims for
    double hill = 0.0;        // big hill, often steeper on one side
    double mesa = 0.0;        // climb to a flat top, then back down
    double valley = 0.0;      // a deep dip to drive through
    double rollers = 0.0;     // a run of small humps
    double steps = 0.0;       // a staircase of ledges, then a long descent
    double kicker = 0.0;      // short ramp with a drop behind it
};

// One zone of the course. Neighbouring zones blend their waves into each other
// over ZoneBlend metres, so the ground never jumps at a boundary.
struct ZoneProfile {
    std::string name;
    double length = 1000.0;  // metres of course
    double climb = 0.0;      // average gradient (metres up per metre forward)
    int ravines = 0;         // jumps placed in this zone
    WaveProfile waves;
    FeatureProfile features;
    int bridges = 0;         // gorges with a plank bridge across, placed in this zone
};

enum class FeatureKind { Hill, Mesa, Valley, Rollers, Steps, Kicker };

struct Feature {
    FeatureKind kind = FeatureKind::Hill;
    double start = 0.0;
    double length = 0.0;
    double height = 0.0;  // peak height, or depth for a valley
    double a = 0.0;       // shape settings; their meaning depends on kind
    double b = 0.0;
    int count = 0;

    [[nodiscard]] double end() const { return start + length; }
    [[nodiscard]] double heightAt(double x) const;
};

struct Ravine {
    double takeoff = 0.0;  // last solid x before the hole
    double landing = 0.0;  // first solid x after it
    bool bridge = false;   // spanned by a plank bridge instead of jumped
};

struct Chain {
    double startX = 0.0;
    std::vector<double> heights;  // one sample every Course::step metres
};

struct Zone {
    double start = 0.0;
    double end = 0.0;
    std::string name;
};

struct Course {
    double step = 0.25;
    double length = 10000.0;
    double spawnX = 10.0;
    double finishX = 10000.0;
    std::vector<Chain> chains;  // solid ground, split at every ravine
    std::vector<Ravine> ravines;
    std::vector<Feature> features;
    std::vector<Zone> zones;
};

struct Settings {
    std::uint32_t seed = 1;
    double step = 0.25;
    double startFlat = 25.0;   // flat run-up at the start
    double finishFlat = 50.0;  // flat run-out before the finish
    double ravineMinWidth = 7.0;
    double ravineMaxWidth = 11.0;
    // Steepest gradient the ground may have (1.48 is about 56 degrees, below
    // the ~61 degrees the tyres' grip can hold). Steeper spots are eased
    // towards this rather than cut off; see limitSlopes in procedural.cpp.
    double maxGradient = 1.48;
    std::vector<ZoneProfile> zones = defaultZones();

    static std::vector<ZoneProfile> defaultZones();

    // The five maps, from 0 (Grazing the Grass, the default zones above) to 4
    // (The Real Excursion). Each map renames the zones and makes the ground
    // harder than the map before it. The seed also differs per map, so the
    // same --seed gives five different courses.
    static constexpr int MapCount = 5;
    static Settings forMap(int map, std::uint32_t seed);
};

// The height of the ground before ravines are cut, at distance x.
class HeightField {
public:
    // Set pieces are kept clear of the given ravines.
    HeightField(const Settings &settings, const std::vector<Ravine> &ravines);

    [[nodiscard]] double height(double x) const;
    // The same ground with only the climb and the long swells. Used to calm
    // the ground into a runway before and after a jump.
    [[nodiscard]] double heightWithoutDetail(double x) const;
    [[nodiscard]] double length() const { return m_length; }
    [[nodiscard]] const std::vector<Feature> &features() const { return m_features; }

private:
    struct Blend {
        double swellAmp, swellLength, hillAmp, hillLength, bumpAmp, bumpLength, warp;
    };
    [[nodiscard]] double zoneWeight(std::size_t zone, double x) const;
    [[nodiscard]] Blend blendAt(double x) const;
    [[nodiscard]] double climbAt(double x) const;
    [[nodiscard]] double envelope(double x) const;
    [[nodiscard]] double swellWave(double x, const Blend &b) const;
    [[nodiscard]] double detailWave(double x, const Blend &b) const;
    [[nodiscard]] double featureHeight(double x) const;
    void placeFeatures(const std::vector<Ravine> &ravines);

    Settings m_settings;
    std::vector<double> m_zoneStart;
    std::vector<double> m_trend;  // integrated climb, one value per metre
    std::vector<double> m_swellPhase, m_hillPhase, m_bumpPhase;
    std::vector<Feature> m_features;  // sorted by start, never overlapping
    double m_length = 0.0;
    double m_phase[6] = {};
};

Course generate(const Settings &settings = Settings{});

// Largest |slope| in degrees over the solid ground, for sanity checks.
double steepestSlopeDegrees(const Course &course);

} // namespace Terrain::Procedural
