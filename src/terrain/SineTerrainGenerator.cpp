#include "terrain/SineTerrainGenerator.h"
#include <cmath>
#include <algorithm>

namespace Terrain {

SineTerrainGenerator::SineTerrainGenerator()
    : m_config() {
}

SineTerrainGenerator::SineTerrainGenerator(const Config& config)
    : m_config(config) {
}

float SineTerrainGenerator::sampleHeight(float x) const {
    if (x <= m_config.startFlatLength) {
        // Safe starting flat runway
        return m_config.baseY;
    }

    float relX = x - m_config.startFlatLength;

    // Smooth blend-in transition from flat to hills over 150px
    float blend = std::min(1.0f, relX / 150.0f);
    blend = blend * blend * (3.0f - 2.0f * blend); // Smoothstep

    // Gradual difficulty scaling
    float bonus = std::min(m_config.maxAmplitudeBonus, relX * m_config.growthFactor);
    float scale = 1.0f + bonus / 50.0f;

    // Multi-octave harmonic synthesis
    float wave1 = std::sin(relX * m_config.freq1) * (m_config.amp1 * scale);
    float wave2 = std::sin(relX * m_config.freq2 + 1.2f) * (m_config.amp2 * scale);
    float wave3 = std::cos(relX * m_config.freq3 + 2.5f) * m_config.amp3;

    // Invert because screen Y points downwards (subtracting wave makes hills go up!)
    float height = m_config.baseY - (wave1 + wave2 + wave3) * blend;
    return height;
}

void SineTerrainGenerator::generateNodes(std::vector<TerrainNode>& outNodes, float startX, float endX, float dx) const {
    if (dx <= 0.0f) return;

    size_t count = static_cast<size_t>(std::max(0.0f, (endX - startX) / dx)) + 1;
    outNodes.resize(count);

    for (size_t i = 0; i < count; ++i) {
        float x = startX + static_cast<float>(i) * dx;
        outNodes[i].height = sampleHeight(x);
        outNodes[i].friction = 0.88f;
        outNodes[i].surfaceType = SurfaceType::Grass;
    }
}

} // namespace Terrain
