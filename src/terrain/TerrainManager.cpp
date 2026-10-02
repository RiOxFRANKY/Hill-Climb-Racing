#include "terrain/TerrainManager.h"
#include <algorithm>
#include <cmath>

namespace Terrain {

TerrainManager::TerrainManager()
    : m_config() {
}

TerrainManager::TerrainManager(const Config& config)
    : m_config(config) {
}

void TerrainManager::generate(const ITerrainGenerator& generator) {
    float totalDist = m_config.trackLength + 1500.0f; // Extra margin at end
    generator.generateNodes(m_nodes, 0.0f, totalDist, m_config.dx);
    buildGrassTiles();
}

void TerrainManager::buildGrassTiles() {
    m_grassTiles.clear();
    if (m_nodes.size() < 2) return;

    const float tileW = 32.0f;
    const float maxDist = m_config.trackLength + 1000.0f;
    constexpr float PI = 3.14159265358979323846f;

    Physics::Vec2 currentP{0.0f, getHeightAt(0.0f)};

    while (currentP.x < maxDist) {
        // Initial estimate using local slope cos(theta)
        float eps = 12.0f;
        float hAhead = getHeightAt(currentP.x + eps);
        float initialAngle = std::atan2(hAhead - currentP.y, eps);
        float guessX = currentP.x + tileW * std::cos(initialAngle);

        // Iteratively refine guessX so that straight-line distance is exactly tileW (32px)
        for (int iter = 0; iter < 4; ++iter) {
            float curY = getHeightAt(guessX);
            float d = std::hypot(guessX - currentP.x, curY - currentP.y);
            if (d > 0.0001f) {
                guessX = currentP.x + (guessX - currentP.x) * (tileW / d);
            }
        }

        Physics::Vec2 nextP{guessX, getHeightAt(guessX)};
        float angleDeg = std::atan2(nextP.y - currentP.y, nextP.x - currentP.x) * (180.0f / PI);

        m_grassTiles.push_back({currentP, nextP, angleDeg});
        currentP = nextP; // Seamless end-to-end chain!
    }
}

float TerrainManager::getHeightAt(float x) const {
    if (m_nodes.empty()) return 350.0f;

    if (x <= 0.0f) {
        return m_nodes.front().height;
    }

    float floatIdx = x / m_config.dx;
    size_t idx = static_cast<size_t>(floatIdx);

    if (idx + 1 >= m_nodes.size()) {
        return m_nodes.back().height;
    }

    float frac = floatIdx - static_cast<float>(idx);
    float h1 = m_nodes[idx].height;
    float h2 = m_nodes[idx + 1].height;

    return h1 + (h2 - h1) * frac;
}

Physics::Vec2 TerrainManager::getNormalAt(float x) const {
    float eps = 2.0f;
    float h1 = getHeightAt(x - eps);
    float h2 = getHeightAt(x + eps);

    Physics::Vec2 tangent(eps * 2.0f, h2 - h1);
    tangent = tangent.normalized();

    // Normal points upward from ground
    Physics::Vec2 normal(-tangent.y, tangent.x);
    if (normal.y > 0.0f) {
        normal = normal * -1.0f;
    }
    return normal;
}

void TerrainManager::feedCollisionSegments(Physics::IPhysicsWorld& physics, float carX) const {
    if (m_nodes.size() < 2) return;

    physics.clearTerrain();

    float minX = std::max(0.0f, carX - m_config.collisionRadius);
    float maxX = carX + m_config.collisionRadius;

    size_t startIdx = static_cast<size_t>(minX / m_config.dx);
    size_t endIdx = static_cast<size_t>(maxX / m_config.dx) + 1;

    startIdx = std::min(startIdx, m_nodes.size() - 2);
    endIdx = std::min(endIdx, m_nodes.size() - 1);

    for (size_t i = startIdx; i < endIdx; ++i) {
        float x1 = static_cast<float>(i) * m_config.dx;
        float y1 = m_nodes[i].height;
        float x2 = static_cast<float>(i + 1) * m_config.dx;
        float y2 = m_nodes[i + 1].height;

        physics.addTerrainSegment({x1, y1}, {x2, y2}, m_nodes[i].friction);
    }
}

TerrainSlice TerrainManager::getVisibleSlice(float viewLeft, float viewRight, float bottomY) const {
    TerrainSlice slice;
    slice.bottomY = bottomY;

    if (m_nodes.size() < 2) return slice;

    float minX = std::max(0.0f, viewLeft - m_config.marginX);
    float maxX = viewRight + m_config.marginX;

    size_t startIdx = static_cast<size_t>(minX / m_config.dx);
    size_t endIdx = static_cast<size_t>(maxX / m_config.dx) + 1;

    startIdx = std::min(startIdx, m_nodes.size() - 2);
    endIdx = std::min(endIdx, m_nodes.size() - 1);

    slice.startX = static_cast<float>(startIdx) * m_config.dx;
    slice.endX = static_cast<float>(endIdx) * m_config.dx;

    slice.surfacePoints.reserve(endIdx - startIdx + 1);
    for (size_t i = startIdx; i <= endIdx; ++i) {
        float x = static_cast<float>(i) * m_config.dx;
        slice.surfacePoints.push_back({x, m_nodes[i].height});
    }

    return slice;
}

} // namespace Terrain
