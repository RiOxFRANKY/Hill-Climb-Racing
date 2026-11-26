#pragma once

// Procedural course generator.
//
// The ground is a pure function of distance: a few layers of domain-warped
// sine waves whose amplitudes and wavelengths change from zone to zone, plus a
// slow climb or descent per zone. A seed shifts the phase of every layer, so
// each seed gives a different but fully reproducible course.
//
// This file has no Qt dependency so the generator can be tested on its own.
// Units are metres, x to the right and height up.

#include <cstdint>
#include <string>
#include <vector>

namespace Terrain::Procedural {

// How one zone of the course is shaped. Neighbouring zones blend into each
// other over ZoneBlend metres, so the ground never jumps at a boundary.
struct ZoneProfile {
    std::string name;
    double length = 1000.0;      // metres of course
    double swellAmp = 0.0;       // long, gentle waves
    double swellLength = 300.0;  // their wavelength in metres
    double hillAmp = 0.0;        // ordinary rolling hills
    double hillLength = 60.0;
    double bumpAmp = 0.0;        // short bumps on top
    double bumpLength = 9.0;
    double warp = 0.0;           // how much the hills are bent out of a pure sine
    double climb = 0.0;          // average gradient (metres up per metre forward)
    int ravines = 0;             // jumps placed in this zone
};

struct Ravine {
    double takeoff = 0.0;  // last solid x before the hole
    double landing = 0.0;  // first solid x after it
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
    std::vector<Zone> zones;
};

struct Settings {
    std::uint32_t seed = 1;
    double step = 0.25;
    double startFlat = 25.0;   // flat run-up at the start
    double finishFlat = 50.0;  // flat run-out before the finish
    std::vector<ZoneProfile> zones = defaultZones();

    static std::vector<ZoneProfile> defaultZones();
};

// The height of the ground before ravines are cut, at distance x.
class HeightField {
public:
    explicit HeightField(const Settings &settings);

    [[nodiscard]] double height(double x) const;
    // The same ground with only the long swells, no hills or bumps. Used to
    // calm the ground into a runway before and after a jump.
    [[nodiscard]] double heightWithoutDetail(double x) const;
    [[nodiscard]] double length() const { return m_length; }

private:
    struct Blend {
        double swellAmp, swellLength, hillAmp, hillLength, bumpAmp, bumpLength, warp;
    };
    [[nodiscard]] Blend blendAt(double x) const;
    [[nodiscard]] double climbAt(double x) const;
    [[nodiscard]] double envelope(double x) const;
    [[nodiscard]] double swellWave(double x, const Blend &b) const;
    [[nodiscard]] double detailWave(double x, const Blend &b) const;

    Settings m_settings;
    std::vector<double> m_zoneStart;
    std::vector<double> m_trend;  // integrated climb, one value per metre
    std::vector<double> m_swellPhase, m_hillPhase, m_bumpPhase;
    double m_length = 0.0;
    double m_phase[6] = {};
};

Course generate(const Settings &settings = Settings{});

// Largest |slope| in degrees over the solid ground, for sanity checks.
double steepestSlopeDegrees(const Course &course);

} // namespace Terrain::Procedural
