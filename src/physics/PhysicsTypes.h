#pragma once

#include <cmath>

namespace Physics {

// ============================================================================
// PURE C++ PHYSICS MATHEMATICAL TYPES
// Exposes zero external or Qt dependencies for complete modular decoupling.
// ============================================================================

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    constexpr Vec2() = default;
    constexpr Vec2(float inX, float inY) : x(inX), y(inY) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec2 normalized() const {
        float l = length();
        if (l > 0.00001f) {
            return {x / l, y / l};
        }
        return {0.0f, 0.0f};
    }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float cross(const Vec2& o) const { return x * o.y - y * o.x; }

    Vec2 rotated(float angleRadians) const {
        float cosA = std::cos(angleRadians);
        float sinA = std::sin(angleRadians);
        return {x * cosA - y * sinA, x * sinA + y * cosA};
    }
};

inline Vec2 operator*(float s, const Vec2& v) {
    return {v.x * s, v.y * s};
}

struct Transform2D {
    Vec2 position{0.0f, 0.0f};
    float angleDegrees{0.0f};
};

struct TerrainSegment {
    Vec2 p1;
    Vec2 p2;
    float friction{0.85f};
};

struct VehiclePhysicsState {
    Transform2D chassis;
    Transform2D rearWheel;
    Transform2D frontWheel;
    Transform2D driverHead;
    Vec2 velocity{0.0f, 0.0f};
    float angularVelocity{0.0f};
    bool headCollided{false};
    bool rearWheelGrounded{false};
    bool frontWheelGrounded{false};
};

} // namespace Physics
