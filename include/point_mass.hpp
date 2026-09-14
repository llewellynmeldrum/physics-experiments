#pragma once
#include "aabb.hpp"
#include "colors.hpp"
#include "conversions.hpp"
#include "draw_helpers.hpp"
#include "format_specs.hpp"
#include "globals.hpp"
#include "timer.hpp"
inline void draw_circle_aabb(glm::vec2 pos, f32 radius, glm::vec3 color) {
    auto const radius_px = meters_to_px(radius);
    auto draw_point = [radius_px](glm::vec2 pos, std::string_view label) {
        auto const pos_px = meters_to_px(pos);
        DrawCircleV(to_rayvec(pos_px), radius_px * 0.2f, GREEN);
        draw_label_px(pos_px, label);
    };
    auto aabb = AABB{pos, {radius, radius}};
    draw_point(aabb.bl, "bl");
    draw_point(aabb.br, "br");
    draw_point(aabb.tl, "tl");
    draw_point(aabb.tr, "tr");
}
inline void draw_circle_outline(glm::vec2 pos, f32 radius, glm::vec3 color) {
    auto const pos_px = meters_to_px(pos);
    auto const radius_px = meters_to_px(radius);
    DrawCircleLinesV(to_rayvec(pos_px), radius_px, to_color(color));
}
inline void draw_circle(glm::vec2 pos, f32 radius, glm::vec3 color) {
    auto const pos_px = meters_to_px(pos);
    auto const radius_px = meters_to_px(radius);
    DrawCircleV(to_rayvec(pos_px), radius_px, to_color(color));
}

struct Circle {
    static constexpr f32 radius{0.05f};
    static constexpr auto G = glm::vec2{0.0f, -9.81f};

    glm::vec2 pos{};
    glm::vec3 color = colors::make_rgb(0, 255, 0);
    glm::vec3 cur_color = color;
    f32 density{1.00f};
    f32 mass = density * PI * pow(radius, 2);
    f32 msCollisionFlashDuration = 250.0f;
    f32 msSinceLastCollision{msCollisionFlashDuration + 1.0f};
    static constexpr auto collision_color = colors::make_rgb(255, 0, 0);

    glm::vec2 vel{0.0f, 1.0f}; // meters per second
    auto get_id() const { return reinterpret_cast<u64>(this); }
    auto resolve_world_boundary_intersection(f32 sDt) -> void {
        if (pos.y + vel.y * sDt + radius >= maxY) {
            pos.y = maxY - radius;
            vel.y *= -gElasticity;
        } else if (pos.y + vel.y * sDt - radius < minY) {
            pos.y = minY + radius;
            vel.y *= -gElasticity;
        }
        if (pos.x + vel.x * sDt + radius >= maxX) {
            pos.x = maxX - radius;
            vel.x *= -gElasticity;
        } else if (pos.x + vel.x * sDt - radius < minX) {
            pos.x = minX + radius;
            vel.x *= -gElasticity;
        }
    }

    auto handle_motion(f32 msDt) -> void {
        auto const sDt = (msDt / 1000.0f) * gTimeScale;
        //       auto const clamped_vel =
        //      glm::clamp(vel, glm::vec2(0.0f), glm::vec2(10.0f));
        // make color reflect velocity
        //        cur_color.g = 0.5 + glm::length(clamped_vel /
        //        glm::vec2(10.0f)) * 0.5;

        vel += G * sDt;
        resolve_world_boundary_intersection(sDt);
        update_color(sDt);

        pos += vel * sDt;
    }
    auto update_color(f32 sDt) -> void {
        if (msSinceLastCollision < msCollisionFlashDuration) {
            f32 t = 1.0f - (msSinceLastCollision / msCollisionFlashDuration);
            cur_color = glm::mix(color, collision_color, t);
            msSinceLastCollision += sDt * 1000.0f;
        } else {
            cur_color = color;
        }
    }

    auto draw() -> void { draw_circle(pos, radius, cur_color); }
    auto draw_aabb() -> void { draw_circle_aabb(pos, radius, cur_color); }
    auto draw_velocity_vector() -> void {
        draw_line(pos, pos + glm::normalize(vel) * radius, 3.0f,
                  colors::make_rgb(12, 24, 12));
    }

    auto draw_label(std::string_view s = ""sv) -> void {
        auto const pos_px = meters_to_px(pos);
        auto const radius_px = meters_to_px(radius);
        draw_label_px(pos_px + glm::vec2{0, radius_px},
                      std::format("'{}'\np: {:4.2f}\nv:{:4.2f}", s, pos, vel));
    }
};

namespace std {
template <> struct hash<Circle> {
    constexpr std::size_t operator()(Circle const &c) {
        return std::hash<u64>{}(c.get_id());
    }
};
}; // namespace std
inline auto operator==(Circle const &a, Circle const &b) {
    return a.get_id() == b.get_id();
}
