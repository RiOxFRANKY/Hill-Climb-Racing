#include "procedural.h"

#include <algorithm>
#include <cmath>

namespace Terrain::Procedural {

namespace {

constexpr double Pi = 3.14159265358979323846;
constexpr double TwoPi = 2.0 * Pi;
constexpr double ZoneBlend = 120.0;  // metres over which two zones cross-fade
constexpr double TrendStep = 1.0;    // resolution of the precomputed phase tables

double smoothstep(double edge0, double edge1, double x)
{
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

// Small deterministic generator so a seed always gives the same course.
class SplitMix {
public:
    explicit SplitMix(std::uint64_t seed) : m_state(seed) {}

    std::uint64_t next()
    {
        std::uint64_t z = (m_state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    // Uniform in [0, 1).
    double unit() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }
    double range(double lo, double hi) { return lo + (hi - lo) * unit(); }

private:
    std::uint64_t m_state;
};

// Phase tables integrate 2*pi/wavelength over distance. Evaluating
// sin(phase(x)) instead of sin(2*pi*x/wavelength(x)) keeps hills smooth while
// the wavelength changes between zones; the naive form would squash or
// stretch the waves wildly at large x.
struct PhaseTables {
    std::vector<double> swell, hill, bump;
};

double sampleTable(const std::vector<double> &table, double x)
{
    const double position = std::clamp(x / TrendStep, 0.0, static_cast<double>(table.size() - 1));
    const auto index = std::min(static_cast<std::size_t>(position), table.size() - 2);
    const double t = position - static_cast<double>(index);
    return table[index] + (table[index + 1] - table[index]) * t;
}

} // namespace

std::vector<ZoneProfile> Settings::defaultZones()
{
    //        name              len   swell        hill         bump        warp  climb  ravines
    return {
        {"GREEN VALLEY",    800,  4.0, 400,  1.6, 55,  0.15, 10.0, 0.5,  0.00, 0},
        {"CLOVER HILLS",   1000,  6.0, 450,  2.4, 60,  0.25,  9.0, 0.8,  0.01, 0},
        {"WINDMILL RISE",  1000,  5.0, 350,  2.0, 70,  0.20,  9.0, 0.6,  0.05, 1},
        {"PEBBLE RUN",      900,  3.0, 380,  1.3, 45,  0.35,  7.5, 1.0,  0.00, 0},
        {"ROLLING SWELLS", 1100, 13.0, 650,  1.6, 80,  0.20, 11.0, 0.4, -0.02, 1},
        {"GULLY JUMPS",    1000,  4.0, 300,  1.8, 55,  0.25,  9.0, 0.7,  0.00, 3},
        {"SAW RIDGE",      1000,  6.0, 380,  2.3, 52,  0.22,  8.5, 1.1,  0.03, 0},
        {"SKY PLATEAU",    1100,  3.0, 500,  1.2, 90,  0.38,  7.5, 0.6,  0.07, 1},
        {"RATTLE ROAD",    1000,  5.0, 420,  2.0, 55,  0.38,  8.0, 1.0, -0.03, 2},
        {"HOME STRETCH",   1100,  8.0, 500,  2.0, 65,  0.25,  9.0, 0.6, -0.06, 1},
    };
}

HeightField::HeightField(const Settings &settings)
    : m_settings(settings)
{
    double start = 0.0;
    for (const ZoneProfile &zone : m_settings.zones) {
        m_zoneStart.push_back(start);
        start += zone.length;
    }
    m_length = start;

    SplitMix rng(0xC0FFEEull ^ (static_cast<std::uint64_t>(m_settings.seed) * 0x100000001B3ull));
    for (double &phase : m_phase)
        phase = rng.range(0.0, TwoPi);

    // Integrate the climb and the three wave phases along the course. The
    // climb fades out near both ends so the start and finish stay level.
    const auto samples = static_cast<std::size_t>(m_length / TrendStep) + 2;
    m_trend.assign(samples, 0.0);
    PhaseTables tables;
    tables.swell.assign(samples, m_phase[0]);
    tables.hill.assign(samples, m_phase[1]);
    tables.bump.assign(samples, m_phase[2]);
    for (std::size_t i = 1; i < samples; ++i) {
        const double x = (static_cast<double>(i) - 0.5) * TrendStep;
        const Blend b = blendAt(x);
        m_trend[i] = m_trend[i - 1] + climbAt(x) * TrendStep;
        tables.swell[i] = tables.swell[i - 1] + TwoPi / b.swellLength * TrendStep;
        tables.hill[i] = tables.hill[i - 1] + TwoPi / b.hillLength * TrendStep;
        tables.bump[i] = tables.bump[i - 1] + TwoPi / b.bumpLength * TrendStep;
    }
    m_swellPhase = std::move(tables.swell);
    m_hillPhase = std::move(tables.hill);
    m_bumpPhase = std::move(tables.bump);
}

HeightField::Blend HeightField::blendAt(double x) const
{
    // Weight each zone by how far x is inside it, cross-fading over
    // ZoneBlend metres around every boundary.
    Blend sum{0, 0, 0, 0, 0, 0, 0};
    double total = 0.0;
    for (std::size_t i = 0; i < m_settings.zones.size(); ++i) {
        const ZoneProfile &z = m_settings.zones[i];
        const double a = m_zoneStart[i];
        const double b = a + z.length;
        const double in = i == 0 ? 1.0 : smoothstep(a - ZoneBlend * 0.5, a + ZoneBlend * 0.5, x);
        const double out = i + 1 == m_settings.zones.size()
                               ? 1.0
                               : 1.0 - smoothstep(b - ZoneBlend * 0.5, b + ZoneBlend * 0.5, x);
        const double w = in * out;
        if (w <= 0.0)
            continue;
        sum.swellAmp += w * z.swellAmp;
        sum.swellLength += w * z.swellLength;
        sum.hillAmp += w * z.hillAmp;
        sum.hillLength += w * z.hillLength;
        sum.bumpAmp += w * z.bumpAmp;
        sum.bumpLength += w * z.bumpLength;
        sum.warp += w * z.warp;
        total += w;
    }
    if (total > 0.0) {
        sum.swellAmp /= total;
        sum.swellLength /= total;
        sum.hillAmp /= total;
        sum.hillLength /= total;
        sum.bumpAmp /= total;
        sum.bumpLength /= total;
        sum.warp /= total;
    }
    return sum;
}

double HeightField::climbAt(double x) const
{
    double sum = 0.0;
    double total = 0.0;
    for (std::size_t i = 0; i < m_settings.zones.size(); ++i) {
        const double a = m_zoneStart[i];
        const double b = a + m_settings.zones[i].length;
        const double in = i == 0 ? 1.0 : smoothstep(a - ZoneBlend * 0.5, a + ZoneBlend * 0.5, x);
        const double out = i + 1 == m_settings.zones.size()
                               ? 1.0
                               : 1.0 - smoothstep(b - ZoneBlend * 0.5, b + ZoneBlend * 0.5, x);
        sum += in * out * m_settings.zones[i].climb;
        total += in * out;
    }
    return total > 0.0 ? sum / total * envelope(x) : 0.0;
}

double HeightField::envelope(double x) const
{
    const double flatEnd = m_length - m_settings.finishFlat;
    return smoothstep(m_settings.startFlat, m_settings.startFlat + 40.0, x)
           * (1.0 - smoothstep(flatEnd - 60.0, flatEnd, x));
}

// Domain warping: each layer's phase is pushed around by a slower sine, so
// hills lean, bunch up and spread out instead of repeating exactly.
double HeightField::swellWave(double x, const Blend &b) const
{
    const double swell = sampleTable(m_swellPhase, x);
    return b.swellAmp * std::sin(swell + 0.6 * std::sin(0.41 * swell + m_phase[3]));
}

double HeightField::detailWave(double x, const Blend &b) const
{
    const double swell = sampleTable(m_swellPhase, x);
    const double hill = sampleTable(m_hillPhase, x);
    const double bump = sampleTable(m_bumpPhase, x);

    // The course gets a little rougher the further you drive.
    const double difficulty = 0.8 + 0.45 * smoothstep(0.0, 7000.0, x);

    const double hillWave =
        b.hillAmp * (std::sin(hill + b.warp * std::sin(0.37 * hill + m_phase[4]))
                     + 0.3 * std::sin(1.71 * hill + 0.9 * std::sin(2.3 * swell) + m_phase[5]));
    // Bumps come in patches: louder on some hills, almost gone on others.
    const double patch = 0.55 + 0.45 * std::sin(0.43 * hill + m_phase[3]);
    const double bumpWave = b.bumpAmp * patch * std::sin(bump + 0.8 * std::sin(0.53 * bump + m_phase[5]));

    return difficulty * (hillWave + bumpWave);
}

double HeightField::height(double x) const
{
    const Blend b = blendAt(x);
    return sampleTable(m_trend, x) + envelope(x) * (swellWave(x, b) + detailWave(x, b));
}

double HeightField::heightWithoutDetail(double x) const
{
    return sampleTable(m_trend, x) + envelope(x) * swellWave(x, blendAt(x));
}

Course generate(const Settings &settings)
{
    HeightField field(settings);
    Course course;
    course.step = settings.step;
    course.length = field.length();
    course.finishX = course.length;
    course.spawnX = std::min(10.0, settings.startFlat * 0.4);

    // Zones, and where each zone's ravines go.
    SplitMix rng(0xBADC0DEull + settings.seed);
    double start = 0.0;
    for (const ZoneProfile &zone : settings.zones) {
        course.zones.push_back({start, start + zone.length, zone.name});
        const double usable = zone.length - 240.0;  // keep jumps clear of zone edges
        for (int k = 0; k < zone.ravines && usable > 0.0; ++k) {
            const double slot = usable / zone.ravines;
            const double centre = start + 120.0 + slot * (k + 0.5) + rng.range(-0.25, 0.25) * slot;
            const double width = rng.range(7.0, 11.0);
            const double takeoff = std::round(centre / course.step) * course.step;
            const double landing = takeoff + std::round(width / course.step) * course.step;
            course.ravines.push_back({takeoff, landing});
        }
        start += zone.length;
    }

    // Around each ravine the hills calm down into a short runway, the
    // takeoff lip kicks up, and the landing side slopes away so the car
    // comes down onto a downhill rather than a wall.
    auto ravineShape = [&course](double x, double &calm, double &offset) {
        calm = 0.0;
        offset = 0.0;
        for (const Ravine &r : course.ravines) {
            if (x < r.takeoff - 60.0 || x > r.landing + 70.0)
                continue;
            calm = std::max(calm, smoothstep(r.takeoff - 50.0, r.takeoff - 32.0, x)
                                      * (1.0 - smoothstep(r.landing + 22.0, r.landing + 40.0, x)));
            if (x <= r.takeoff) {
                offset += 1.4 * smoothstep(r.takeoff - 12.0, r.takeoff, x);
            } else if (x >= r.landing) {
                const double rise = 0.15 + 0.85 * smoothstep(r.landing, r.landing + 18.0, x);
                const double fade = 1.0 - smoothstep(r.landing + 18.0, r.landing + 55.0, x);
                offset -= 1.6 * (x < r.landing + 18.0 ? rise : fade);
            }
        }
    };

    const auto samples = static_cast<std::size_t>(std::llround(course.length / course.step)) + 1;
    std::size_t nextRavine = 0;
    Chain chain;
    for (std::size_t i = 0; i < samples; ++i) {
        const double x = static_cast<double>(i) * course.step;
        if (nextRavine < course.ravines.size()) {
            const Ravine &r = course.ravines[nextRavine];
            if (x > r.takeoff + 1e-9 && x < r.landing - 1e-9)
                continue;  // inside the hole
            if (x >= r.landing - 1e-9) {
                course.chains.push_back(std::move(chain));
                chain = Chain{};
                ++nextRavine;
            }
        }
        if (chain.heights.empty())
            chain.startX = x;

        double calm = 0.0;
        double offset = 0.0;
        ravineShape(x, calm, offset);
        const double base = field.height(x);
        // Calm ground follows the slow swell only, not the hills and bumps.
        const double calmBase = base + calm * (field.heightWithoutDetail(x) - base);
        chain.heights.push_back(calmBase + offset);
    }
    course.chains.push_back(std::move(chain));
    return course;
}

double steepestSlopeDegrees(const Course &course)
{
    double steepest = 0.0;
    for (const Chain &chain : course.chains) {
        for (std::size_t i = 1; i < chain.heights.size(); ++i) {
            const double slope = (chain.heights[i] - chain.heights[i - 1]) / course.step;
            steepest = std::max(steepest, std::abs(slope));
        }
    }
    return std::atan(steepest) * 180.0 / Pi;
}

} // namespace Terrain::Procedural
