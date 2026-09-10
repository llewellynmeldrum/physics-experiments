#pragma once 
#include "conversions.hpp"
#include "draw_helpers.hpp"
#include "globals.hpp"
#include "format_specs.hpp"
inline void draw_circle_aabb(glm::vec2 pos, f32 radius, glm::vec3 color){
    auto const radius_px = meters_to_px(radius);
    auto draw_corner = [radius_px](glm::vec2 pos){
        auto const pos_px = meters_to_px(pos);
        DrawCircleV( to_rayvec(pos_px), radius_px*0.2f, GREEN);
    };
    draw_corner(pos-glm::vec2{-radius,-radius});
    draw_corner(pos-glm::vec2{+radius,-radius});
    draw_corner(pos-glm::vec2{-radius,+radius});
    draw_corner(pos-glm::vec2{+radius,+radius});
}
inline void draw_circle(glm::vec2 pos, f32 radius, glm::vec3 color){
    auto const pos_px = meters_to_px(pos);
    auto const radius_px = meters_to_px(radius);
    DrawCircleV(
        to_rayvec(pos_px),
        radius_px,
        to_color(color)
    );
    DrawCircleV(
        to_rayvec(pos_px),
        radius_px*0.1f,
        BLACK
    );
}

struct Circle{
    static constexpr auto G = glm::vec2{0.0f, -9.81f};
    glm::vec2 pos {};
    glm::vec3 color =  __RGB(0,255,0);
    glm::vec3 cur_color =  color;
    f32 radius {0.05f};
    f32 density {1.00f};
    f32 mass = density * PI * pow(radius,2);
    glm::vec2 vel {0.0f,1.0f}; // meters per second
    auto get_id() const {
        return reinterpret_cast<u64>(this);
    }
    auto resolve_world_boundary_intersection(f32 sDt)-> void{
        if (pos.y + vel.y*sDt + radius >= maxY){
            pos.y = maxY - radius;
            vel.y *= -gElasticity;
        }else if (pos.y + vel.y*sDt - radius < minY){
            pos.y = minY + radius;
            vel.y*=-gElasticity ;
        }
        if (pos.x + vel.x*sDt + radius >= maxX){
            pos.x = maxX - radius;
            vel.x *= -gElasticity;
        }else if (pos.x + vel.x*sDt - radius < minX){
            pos.x = minX + radius;
            vel.x*=-gElasticity ;
        }
    }

    auto handle_motion(f32 msDt) -> void{
        auto const sDt = (msDt / 1000.0f) * gTimeScale;
        auto const clamped_vel = glm::clamp(vel,glm::vec2(0.0f),glm::vec2(10.0f));
        // make color reflect velocity
        cur_color.g = 0.5 + glm::length(clamped_vel / glm::vec2(10.0f))* 0.5;

        vel += G * sDt;
        resolve_world_boundary_intersection(sDt);
        pos += vel * sDt;
    }

    auto draw() -> void {draw_circle(pos,radius,cur_color);}
    auto draw_aabb() -> void {draw_circle_aabb(pos,radius,cur_color);}

    auto draw_label(std::string_view s = ""sv) -> void {
        auto const pos_px = meters_to_px(pos);
        auto const radius_px = meters_to_px(radius);
        draw_label_px(
            pos_px+glm::vec2{0,radius_px},
            std::format("'{}'\np: {:4.2f}\nv:{:4.2f}",s,pos,vel)
        );
    }
};

namespace std{
    template<>
    struct hash<Circle>{
        constexpr std::size_t operator()(Circle const& c){
            return std::hash<u64>{}(c.get_id());
        }
    };
};
inline auto operator==(Circle const& a, Circle const& b){
    return a.get_id()==b.get_id();
}
