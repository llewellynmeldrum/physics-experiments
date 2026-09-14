#pragma once
#include "raylib.h"
#include "types.hpp"
constexpr inline auto pixelsPerMeter = 100.0f;
constexpr inline auto metersPerPixel = 1.0f / pixelsPerMeter;
constexpr inline auto px_to_meters(auto in) { return in * metersPerPixel; }
constexpr inline auto meters_to_px(auto in) { return in * pixelsPerMeter; }

constexpr auto screenExtentX_px = 1280;
constexpr auto screenExtentY_px = 720;
constexpr auto screenExtent_px = glm::vec2{screenExtentX_px, screenExtentY_px};

constexpr f32 screenExtentX = px_to_meters(screenExtentX_px);
constexpr f32 screenExtentY = px_to_meters(screenExtentY_px);
constexpr auto screenExtent = glm::vec2{screenExtentX, screenExtentY};

constexpr f32 minX = -screenExtentX * 0.5f;
constexpr f32 maxX = screenExtentX * 0.5f;
constexpr f32 minY = -screenExtentY * 0.5f;
constexpr f32 maxY = screenExtentY * 0.5f;

template <> constexpr inline auto px_to_meters(glm::vec2 in) {
    return in * metersPerPixel * glm::vec2{1.0f, -1.0f} // flip y axis
           - glm::vec2{screenExtentX * 0.5f,
                       -screenExtentY * 0.5f}; // move to center
}

template <> constexpr inline auto meters_to_px(glm::vec2 in) {
    return in * pixelsPerMeter * glm::vec2{1.0f, -1.0f} // flip y axis
           + glm::vec2{screenExtentX_px * 0.5f,
                       +screenExtentY_px * 0.5f}; // move to center
}

inline auto to_glm(Vector3 v) { return glm::vec3{v.x, v.y, v.z}; }
inline auto to_glm(Vector2 v) { return glm::vec2{v.x, v.y}; }

inline auto to_rayvec(glm::vec3 v) { return Vector3{v.x, v.y, v.z}; }
inline auto to_rayvec(glm::vec2 v) { return Vector2{v.x, v.y}; }

inline auto to_color(glm::vec4 c) {
    return ColorFromNormalized(Vector4(c.r, c.g, c.b, c.a));
}
inline auto to_color(glm::vec3 c) { return to_color(glm::vec4(c, 1.0f)); }
