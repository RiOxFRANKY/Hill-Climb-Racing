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
        float baseY{380.0f};           // Baseline ground height in virtual canvas coordinates
        float startFlatLength{300.0f}; // Safe flat starting distance before hills begin

        // Octave 1: Long rolling hills
        float amp1{55.0f};
        float freq1{0.0035f};

        // Octave 2: Medium steep hills & crests
        float amp2{35.0f};
        float freq2{0.0085f};

        // Octave 3: Short bumps and ripples
        float amp3{12.0f};
        float freq3{0.0220f};

        // Difficulty scaling (hills grow taller over distance)
        float growthFactor{0.00004f};  // Amplitude increase per pixel traveled
        float maxAmplitudeBonus{80.0f};// Cap on maximum difficulty scaling
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
