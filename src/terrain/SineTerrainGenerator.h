#pragma once

#include "terrain/ITerrainGenerator.h"

namespace Terrain {

// ============================================================================
// SineTerrainGenerator
// Multi-octave smooth trigonometric terrain generator with dynamic difficulty scaling.
// ============================================================================

class SineTerrainGenerator : public ITerrainGenerator {
public:
    // ========================================================================
    // TWEAKABLE PARAMETERS (Tune hill sizes, wavelengths, and steepness)
    // ========================================================================
    struct Config {
        float baseY{390.0f};           // Baseline ground height in virtual canvas coordinates
        float startFlatLength{250.0f}; // Safe starting runway before hills begin

        // Octave 1: Major mountain ranges & deep rolling valleys
        float amp1{95.0f};
        float freq1{0.0028f};

        // Octave 2: Steep launch ramps, crests & sharp ascents
        float amp2{65.0f};
        float freq2{0.0075f};

        // Octave 3: Dynamic rolling moguls & hillocks
        float amp3{22.0f};
        float freq3{0.0180f};

        // Octave 4: Rough dirt bumps & suspension whoops
        float amp4{12.0f};
        float freq4{0.0360f};

        // Difficulty scaling (hills grow taller and steeper over distance)
        float growthFactor{0.00007f};   // Amplitude increase per pixel traveled
        float maxAmplitudeBonus{120.0f};// Cap on maximum difficulty scaling
    };

    SineTerrainGenerator();
    explicit SineTerrainGenerator(const Config& config);
    ~SineTerrainGenerator() override = default;

    float sampleHeight(float x) const override;
    void generateNodes(std::vector<TerrainNode>& outNodes, float startX, float endX, float dx) const override;

    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

private:
    Config m_config;
};

} // namespace Terrain
