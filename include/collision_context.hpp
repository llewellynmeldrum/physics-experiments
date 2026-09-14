#pragma once
#include "glm_types.hpp"
#include "point_mass.hpp"
#include "types.hpp"

struct CollisionContext {
    Circle &a;
    Circle &b;
    f32 dist = {};
    f32 overlap_depth{};
    glm::vec2 collision_normal; // from a to b
    auto respond() -> void {
        // first find the collision vector (the collision vector from a to b =
        // b.p-a.p)
        collision_normal = glm::normalize(b.pos - a.pos);
        // move them both by half the overlap depth

        // Push B away, in the direction of the collision from A's perspective
        b.pos += collision_normal * overlap_depth * 0.5f;

        // Push A away, in the direction of the collision from B's perspective
        a.pos -= collision_normal * overlap_depth * 0.5f;
    }

    // * Returns: whether or not a resolution took place (closing speed >=0)
    auto resolve() -> bool {
        auto const relative_velocity = (b.vel - a.vel);
        auto const closing_speed =
            glm::dot(relative_velocity, collision_normal);

        a.msSinceLastCollision = 0.0f;
        b.msSinceLastCollision = 0.0f;

        if (closing_speed <= 0) {
            auto const j = -(1.0f + gElasticity) * closing_speed /
                           ((1.0f / a.mass) + (1.0f / b.mass));
            a.vel -= (j * collision_normal) / a.mass;
            b.vel += (j * collision_normal) / b.mass;
            return true;
        }
        return false;
    }
};

inline auto collision_check_narrow(Circle &a, Circle &b)
    -> std::optional<CollisionContext> {
    if (a.get_id() == b.get_id())
        return std::nullopt;
    auto const dist2 = glm::distance2(a.pos, b.pos);
    auto const radius_sum = a.radius + b.radius;
    auto const radius_sum2 = glm::pow(radius_sum, 2);
    bool collision_occured = dist2 < radius_sum2;
    if (!collision_occured) {
        return std::nullopt;
    }
    auto const dist = glm::sqrt(dist2);
    auto const overlap_depth = radius_sum - dist;
    return CollisionContext{a, b, dist, overlap_depth};
}
