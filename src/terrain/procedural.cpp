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

// sin^2 hump: 0 at both ends with a flat tangent, so it joins the ground
// around it without a kink. Its steepest gradient is pi * height / length.
double hump(double u)
{
    const double s = std::sin(Pi * std::clamp(u, 0.0, 1.0));
    return s * s;
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
    int range(int lo, int hi) { return lo + static_cast<int>(unit() * (hi - lo + 1)); }

private:
    std::uint64_t m_state;
};

// Phase tables integrate 2*pi/wavelength over distance. Evaluating
// sin(phase(x)) instead of sin(2*pi*x/wavelength(x)) keeps hills smooth while
// the wavelength changes between zones; the naive form would squash or
// stretch the waves wildly at large x.
double sampleTable(const std::vector<double> &table, double x)
{
    const double position = std::clamp(x / TrendStep, 0.0, static_cast<double>(table.size() - 1));
    const auto index = std::min(static_cast<std::size_t>(position), table.size() - 2);
    const double t = position - static_cast<double>(index);
    return table[index] + (table[index + 1] - table[index]) * t;
}

// Rebuilds a chain from its own gradients with every gradient passed through a
// soft cap: g / (1 + (|g| / limit)^6)^(1/6). Gentle and moderate slopes are
// left almost exactly as they were (a slope at half the limit changes by about
// 0.3%), while anything steeper bends smoothly towards the limit instead of
// stopping at a sharp corner. Walls the tyres could never climb become steep
// ramps they can. The chain keeps its starting height; later points shift by
// however much height the capped slopes gave up.
void limitSlopes(Chain &chain, double step, double limit)
{
    if (chain.heights.size() < 2 || limit <= 0.0)
        return;
    double previous = chain.heights[0];
    double rebuilt = chain.heights[0];
    for (std::size_t i = 1; i < chain.heights.size(); ++i) {
        const double gradient = (chain.heights[i] - previous) / step;
        previous = chain.heights[i];
        const double ratio = std::abs(gradient) / limit;
        const double capped = gradient / std::pow(1.0 + std::pow(ratio, 6.0), 1.0 / 6.0);
        rebuilt += capped * step;
        chain.heights[i] = rebuilt;
    }
}

// Set pieces grow as the course goes on.
double difficultyAt(double x)
{
    return 0.75 + 0.5 * smoothstep(500.0, 8500.0, x);
}

} // namespace

std::vector<ZoneProfile> Settings::defaultZones()
{
    // Waves:    swell amp/length, hill amp/length, bump amp/length, warp
    // Features: spacing, size, steepness, then weights for
    //           hill, mesa, valley, rollers, steps, kicker
    return {
        {"GREEN VALLEY",    800,  0.00, 0, { 8, 450, 2.0, 70, 0.15, 10.0, 0.5}, {70, 0.6, 0.45, 3, 1, 1, 1, 0, 0}},
        {"CLOVER HILLS",   1000,  0.01, 0, {10, 500, 2.6, 75, 0.25,  9.0, 0.8}, {55, 0.8, 0.55, 4, 1, 2, 2, 0, 1}},
        {"WINDMILL RISE",  1000,  0.05, 1, { 8, 400, 2.3, 80, 0.20,  9.0, 0.6}, {50, 0.9, 0.60, 3, 3, 1, 1, 2, 1}},
        {"PEBBLE RUN",      900,  0.00, 0, { 6, 420, 1.6, 55, 0.30,  7.5, 1.0}, {35, 0.8, 0.55, 1, 0, 1, 5, 3, 2}},
        {"ROLLING SWELLS", 1100, -0.02, 1, {22, 700, 2.0, 90, 0.20, 11.0, 0.4}, {80, 1.1, 0.55, 4, 1, 3, 1, 0, 1}},
        {"GULLY JUMPS",    1000,  0.00, 3, { 7, 350, 2.0, 60, 0.25,  9.0, 0.7}, {45, 0.9, 0.60, 1, 1, 1, 1, 1, 5}},
        {"SAW RIDGE",      1000,  0.03, 0, {10, 420, 2.6, 60, 0.22,  8.5, 1.1}, {40, 1.1, 0.60, 5, 1, 3, 2, 1, 1}},
        {"SKY PLATEAU",    1100,  0.07, 1, { 6, 550, 1.6, 95, 0.32,  7.5, 0.6}, {50, 1.2, 0.58, 1, 5, 1, 1, 3, 1}},
        {"RATTLE ROAD",    1000, -0.03, 2, { 8, 450, 2.2, 60, 0.35,  8.0, 1.0}, {30, 1.0, 0.58, 2, 1, 2, 4, 3, 3}},
        {"HOME STRETCH",   1100, -0.06, 1, {14, 550, 2.5, 70, 0.25,  9.0, 0.6}, {55, 1.3, 0.60, 4, 2, 3, 2, 2, 2}},
    };
}

Settings Settings::forMap(int map, std::uint32_t seed)
{
    static const char *const zoneNames[MapCount][10] = {
        {},  // map 0 keeps the default names
        {"SANDY START", "DUNE DRIFT", "PALM POINT", "SHELL SHALLOWS", "TIDE SWELLS",
         "ROCKPOOL GAPS", "CLIFF COVE", "LIGHTHOUSE HILL", "DRIFTWOOD RUN", "SUNSET STRAND"},
        {"MOSSY MOUTH", "REED BEDS", "FROG HOLLOW", "BOG BUMPS", "MIRE SWELLS",
         "SINKHOLE STRETCH", "GNARLED RIDGE", "HERON HEIGHTS", "MUDSLIDE LANE", "LANTERN BAYOU"},
        {"FROST GATE", "SNOWDRIFT FIELDS", "PINE GLACIER", "ICICLE STEPS", "AURORA SWELLS",
         "CREVASSE JUMPS", "BLIZZARD RIDGE", "SUMMIT SHELF", "AVALANCHE ALLEY", "POLAR FINISH"},
        {"BASE CAMP", "SWITCHBACKS", "BOULDER CLIMB", "GRAVEL CHUTES", "THE BIG SWELL",
         "CANYON LEAPS", "KNIFE EDGE", "CLOUD SHELF", "THE GAUNTLET", "LAST DESCENT"},
    };

    map = std::clamp(map, 0, MapCount - 1);
    const double d = map;  // 0 = easiest, 4 = hardest

    Settings settings;
    settings.seed = seed + static_cast<std::uint32_t>(map) * 7919u;
    // Wider ravines on harder maps.
    settings.ravineMinWidth = 7.0 + 0.8 * d;
    settings.ravineMaxWidth = 11.0 + 1.0 * d;

    for (std::size_t i = 0; i < settings.zones.size(); ++i) {
        ZoneProfile &zone = settings.zones[i];
        if (map > 0)
            zone.name = zoneNames[map][i];

        // Much taller, sharper and bumpier ground: by the last map the
        // rolling hills are about 2.4 times as tall and a quarter shorter,
        // so they are both bigger and steeper.
        WaveProfile &w = zone.waves;
        w.swellAmp *= 1.0 + 0.35 * d;
        w.swellLength *= 1.0 - 0.06 * d;
        w.hillAmp *= 1.0 + 0.35 * d;
        w.hillLength *= 1.0 - 0.06 * d;
        w.bumpAmp *= 1.0 + 0.35 * d;
        w.bumpLength *= 1.0 - 0.04 * d;
        w.warp += 0.15 * d;

        // Long sustained climbs: every zone tilts further uphill, so the
        // course keeps gaining height and downhill zones level off.
        zone.climb = zone.climb * (1.0 + 0.3 * d) + 0.015 * d;

        // Much larger, steeper set pieces packed closer together, with more
        // ledges and kicker ramps in the mix.
        FeatureProfile &f = zone.features;
        f.size *= 1.0 + 0.3 * d;
        f.steepness = std::min(0.95, f.steepness + 0.08 * d);
        f.spacing = std::max(15.0, f.spacing * (1.0 - 0.15 * d));
        f.steps += 0.4 * d;
        f.kicker += 0.6 * d;
        f.valley += 0.3 * d;

        // From the third map on, every zone after the first two gets more jumps.
        if (i >= 2)
            zone.ravines += map / 2;
    }
    return settings;
}

double Feature::heightAt(double x) const
{
    const double xr = x - start;
    if (xr <= 0.0 || xr >= length)
        return 0.0;
    const double u = xr / length;
    switch (kind) {
    case FeatureKind::Hill:
        // a bends the hump so one side is steeper than the other.
        return height * hump(std::pow(u, a));
    case FeatureKind::Valley:
        return -height * hump(std::pow(u, a));
    case FeatureKind::Mesa:
        // a is the length of each ramp; the top is flat.
        return height * smoothstep(0.0, a, xr) * (1.0 - smoothstep(length - a, length, xr));
    case FeatureKind::Rollers:
        return height * hump(u * count - std::floor(u * count));
    case FeatureKind::Steps: {
        // count ledges, each a tread of b metres then a riser of a metres,
        // then one long descent back to the ground.
        const double stepHeight = height / count;
        const double pitch = a + b;
        const double up = pitch * count;
        if (xr >= up)
            return height * (1.0 - smoothstep(up, length, xr));
        double level = 0.0;
        for (int i = 0; i < count; ++i)
            level += stepHeight * smoothstep(i * pitch + b, (i + 1) * pitch, xr);
        return level;
    }
    case FeatureKind::Kicker:
        // a is the ramp length; the rest is the drop behind it.
        return xr < a ? height * smoothstep(0.0, a, xr) : height * (1.0 - smoothstep(a, length, xr));
    }
    return 0.0;
}

HeightField::HeightField(const Settings &settings, const std::vector<Ravine> &ravines)
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
    m_swellPhase.assign(samples, m_phase[0]);
    m_hillPhase.assign(samples, m_phase[1]);
    m_bumpPhase.assign(samples, m_phase[2]);
    for (std::size_t i = 1; i < samples; ++i) {
        const double x = (static_cast<double>(i) - 0.5) * TrendStep;
        const Blend b = blendAt(x);
        m_trend[i] = m_trend[i - 1] + climbAt(x) * TrendStep;
        m_swellPhase[i] = m_swellPhase[i - 1] + TwoPi / b.swellLength * TrendStep;
        m_hillPhase[i] = m_hillPhase[i - 1] + TwoPi / b.hillLength * TrendStep;
        m_bumpPhase[i] = m_bumpPhase[i - 1] + TwoPi / b.bumpLength * TrendStep;
    }

    placeFeatures(ravines);
}

void HeightField::placeFeatures(const std::vector<Ravine> &ravines)
{
    SplitMix rng(0x5E7F1ECEull + m_settings.seed * 0x9E3779B1ull);
    const double firstX = m_settings.startFlat + 70.0;
    const double lastX = m_length - m_settings.finishFlat - 90.0;

    // Returns where a piece starting at x and running for length metres would
    // hit a ravine's runway, or a negative number if it is clear.
    auto blockedUntil = [&ravines](double x, double length) {
        for (const Ravine &r : ravines) {
            const double keepOutStart = r.takeoff - 70.0;
            const double keepOutEnd = r.landing + 80.0;
            if (x < keepOutEnd && x + length > keepOutStart)
                return keepOutEnd;
        }
        return -1.0;
    };

    for (std::size_t z = 0; z < m_settings.zones.size(); ++z) {
        const FeatureProfile &p = m_settings.zones[z].features;
        const double weights[6] = {p.hill, p.mesa, p.valley, p.rollers, p.steps, p.kicker};
        double totalWeight = 0.0;
        for (double w : weights)
            totalWeight += w;
        if (totalWeight <= 0.0)
            continue;

        const double zoneEnd = m_zoneStart[z] + m_settings.zones[z].length;
        double x = std::max(firstX, m_zoneStart[z] + rng.range(0.2, 0.8) * p.spacing);
        if (!m_features.empty())
            x = std::max(x, m_features.back().end() + 8.0);

        while (x < zoneEnd) {
            // Pick a kind by weight.
            double pick = rng.unit() * totalWeight;
            int kindIndex = 0;
            while (kindIndex < 5 && pick >= weights[kindIndex]) {
                pick -= weights[kindIndex];
                ++kindIndex;
            }

            const double scale = p.size * difficultyAt(x);
            const double grade = p.steepness * rng.range(0.75, 1.0);
            Feature f;
            f.kind = static_cast<FeatureKind>(kindIndex);
            f.start = x;
            switch (f.kind) {
            case FeatureKind::Hill:
            case FeatureKind::Valley: {
                f.height = scale * (f.kind == FeatureKind::Hill ? rng.range(6.0, 16.0) : rng.range(4.0, 10.0));
                f.a = rng.range(0.7, 1.4);
                // Bending the hump makes one side steeper by about 1/a or a.
                const double bend = std::max(f.a, 1.0 / f.a);
                f.length = Pi * f.height / grade * bend;
                break;
            }
            case FeatureKind::Mesa:
                f.height = scale * rng.range(5.0, 12.0);
                f.a = 1.5 * f.height / grade;  // smoothstep's steepest gradient is 1.5 * rise / run
                f.length = 2.0 * f.a + rng.range(15.0, 40.0);
                break;
            case FeatureKind::Rollers: {
                f.height = std::min(1.8, scale * rng.range(0.8, 1.4));
                f.count = rng.range(3, 6);
                const double humpLength = std::max(5.0, Pi * f.height / (grade * 1.2));
                f.length = humpLength * f.count;
                break;
            }
            case FeatureKind::Steps: {
                f.count = rng.range(3, 5);
                const double stepHeight = std::min(1.2, scale * rng.range(0.6, 1.0));
                f.height = stepHeight * f.count;
                f.a = std::max(1.8, 1.5 * stepHeight / grade);  // riser
                f.b = rng.range(5.0, 9.0);                     // tread
                const double descent = 1.5 * f.height / (grade * 0.7);
                f.length = (f.a + f.b) * f.count + descent;
                break;
            }
            case FeatureKind::Kicker:
                f.height = std::min(3.5, scale * rng.range(1.5, 3.0));
                f.a = 1.5 * f.height / (grade * 0.8);                  // ramp up
                f.length = f.a + std::max(3.0, 1.5 * f.height / (grade * 1.3));  // and the drop
                break;
            }

            if (f.end() > lastX)
                break;
            const double blocked = blockedUntil(f.start, f.length);
            if (blocked >= 0.0) {
                x = blocked;
                continue;
            }
            m_features.push_back(f);
            x = f.end() + p.spacing * rng.range(0.4, 1.6);
        }
    }
}

double HeightField::zoneWeight(std::size_t zone, double x) const
{
    const double a = m_zoneStart[zone];
    const double b = a + m_settings.zones[zone].length;
    const double in = zone == 0 ? 1.0 : smoothstep(a - ZoneBlend * 0.5, a + ZoneBlend * 0.5, x);
    const double out = zone + 1 == m_settings.zones.size()
                           ? 1.0
                           : 1.0 - smoothstep(b - ZoneBlend * 0.5, b + ZoneBlend * 0.5, x);
    return in * out;
}

HeightField::Blend HeightField::blendAt(double x) const
{
    // Weight each zone by how far x is inside it, cross-fading over
    // ZoneBlend metres around every boundary.
    Blend sum{0, 0, 0, 0, 0, 0, 0};
    double total = 0.0;
    for (std::size_t i = 0; i < m_settings.zones.size(); ++i) {
        const double w = zoneWeight(i, x);
        if (w <= 0.0)
            continue;
        const WaveProfile &z = m_settings.zones[i].waves;
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
        const double w = zoneWeight(i, x);
        sum += w * m_settings.zones[i].climb;
        total += w;
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

    const double hillWave =
        b.hillAmp * (std::sin(hill + b.warp * std::sin(0.37 * hill + m_phase[4]))
                     + 0.3 * std::sin(1.71 * hill + 0.9 * std::sin(2.3 * swell) + m_phase[5]));
    // Bumps come in patches: louder on some hills, almost gone on others.
    const double patch = 0.55 + 0.45 * std::sin(0.43 * hill + m_phase[3]);
    const double bumpWave = b.bumpAmp * patch * std::sin(bump + 0.8 * std::sin(0.53 * bump + m_phase[5]));

    return difficultyAt(x) * (hillWave + bumpWave);
}

double HeightField::featureHeight(double x) const
{
    // Set pieces never overlap, so only the last one starting before x matters.
    auto it = std::upper_bound(m_features.cbegin(), m_features.cend(), x,
                               [](double value, const Feature &f) { return value < f.start; });
    if (it == m_features.cbegin())
        return 0.0;
    return std::prev(it)->heightAt(x);
}

double HeightField::height(double x) const
{
    const Blend b = blendAt(x);
    return sampleTable(m_trend, x) + envelope(x) * (swellWave(x, b) + detailWave(x, b)) + featureHeight(x);
}

double HeightField::heightWithoutDetail(double x) const
{
    return sampleTable(m_trend, x) + envelope(x) * swellWave(x, blendAt(x));
}

Course generate(const Settings &settings)
{
    Course course;
    course.step = settings.step;
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
            const double width = rng.range(settings.ravineMinWidth, settings.ravineMaxWidth);
            const double takeoff = std::round(centre / course.step) * course.step;
            const double landing = takeoff + std::round(width / course.step) * course.step;
            course.ravines.push_back({takeoff, landing});
        }
        start += zone.length;
    }

    const HeightField field(settings, course.ravines);
    course.length = field.length();
    course.finishX = course.length;
    course.features = field.features();

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
        // Calm ground follows the climb and slow swell only.
        const double calmBase = base + calm * (field.heightWithoutDetail(x) - base);
        chain.heights.push_back(calmBase + offset);
    }
    course.chains.push_back(std::move(chain));

    for (Chain &solid : course.chains)
        limitSlopes(solid, course.step, settings.maxGradient);
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
