#include <bit>
#include <map>
#include <optional>
#include <print>
#include <unordered_set>
#define GLM_ENABLE_EXPERIMENTAL 
#include "glm/ext.hpp"
#include <glm/gtx/norm.hpp>

#include <magic_enum/magic_enum.hpp>

#include <raylib.h>
#include <bit>



#include "debug_toggles.hpp"
#include "conversions.hpp"
#include "types.hpp"
#include "timer.hpp"
#include "draw_helpers.hpp"
#include "format_specs.hpp"
#include "globals.hpp"
#include "aabb.hpp"
#include "occupancy_grid.hpp"
#include "point_mass.hpp"
#include "rand.hpp"
#include "pair_hashing.hpp"
#include "logger.hpp"



struct FrameStats{
    u64 collision_pairs_evaluated;
    u64 collisions_detected;
    u64 collisions_responded;
    u64 collisions_resolved;
};

using TickCount = u32;
std::vector<Circle> circle_list{};
OccupancyGrid occupancy_grid{};
FrameStats stats;



TickCount ticks_per_second {200uz};
TickCount tick_count = 0uz;
timer::duration tick_gap_accumulator = timer::duration{0};
timer::time_point t_frame_start = timer::now();
constexpr auto msPerTick(){return timer::milliseconds(1000.0 / ticks_per_second);}

constexpr static auto maxGapContributionPerFrame = timer::milliseconds(250.0); // limit on how many 'lagging' ticks are CREATED
constexpr static auto maxTicksPerFrame = 8uz; // limit on how many 'lagging' ticks are ACCEPTED
    
void perform_tick_updates(timer::duration dt); // called once per frame
void per_tick_update(); // called [1,8] times per frame;




struct CollisionContext{
    Circle & a;
    Circle & b;
    f32 dist = {};
    f32 overlap_depth{};
    glm::vec2 collision_normal; // from a to b
    auto respond(){
        // first find the collision vector (the collision vector from a to b = b.p-a.p)
        collision_normal = glm::normalize(b.pos-a.pos);
        // move them both by half the overlap depth

        // Push B away, in the direction of the collision from A's perspective 
        b.pos +=  collision_normal * overlap_depth*0.5f;

        // Push A away, in the direction of the collision from B's perspective 
        a.pos -=  collision_normal * overlap_depth*0.5f;
    }
    auto resolve(){
        auto const relative_velocity = (b.vel-a.vel);
        auto const closing_speed = glm::dot(relative_velocity, collision_normal);
        if (closing_speed < 0){
            auto const j = 
                -(1.0f + gElasticity) * closing_speed  
                /
                ( (1.0f / a.mass) + (1.0f / b.mass));
            a.vel -= (j * collision_normal) / a.mass;
            b.vel += (j * collision_normal) / b.mass;
        }
    }
};
auto collision_check_narrow(
    Circle & a,
    Circle & b
) -> std::optional<CollisionContext>{
    if (a.get_id() == b.get_id()) return std::nullopt;
    auto const dist2 = glm::distance2(a.pos, b.pos);
    auto const radius_sum = a.radius + b.radius;
    auto const radius_sum2 = glm::pow(radius_sum , 2);
    bool collision_occured = dist2 < radius_sum2;
    if (!collision_occured){
        return std::nullopt;
    }
    auto const dist = glm::sqrt(dist2);
    auto const overlap_depth = radius_sum - dist;
    return CollisionContext{a,b,dist,overlap_depth};
}



// Make the bodies no longer intersect.
// apply the resultant forces of the collision
void per_tick_update(){
    // TODO: implement broad phase:
    // Create a uniform grid based AABB residency map.
    // For each body, add each cell which its AABB intersects with. AABB v AABB intersection test
    occupancy_grid = build_occupancy_grid(circle_list);
    // iterate over the grid, and check for collisions only between circles in the same grid


    std::unordered_set<std::pair<Circle, Circle>> evaluated_pairs;
    if (is_enabled(use_occupancy_grid)){
        for (auto& occupancy_group: occupancy_grid.data){
            for (auto * a_ptr: occupancy_group){
                for (auto * b_ptr: occupancy_group){
                    if (a_ptr == b_ptr) break;
                    auto& a = *a_ptr;
                    auto& b = *b_ptr;
//                    auto it = evaluated_pairs.find({a,b});
//                    bool pair_already_evaluated = it != evaluated_pairs.end();
//                    if (pair_already_evaluated) continue;

                    if (auto collision = collision_check_narrow(a,b)){
                        (*collision).respond();
                        (*collision).resolve();
                    }

//                    evaluated_pairs.emplace_hint(it,a,b);
//                    evaluated_pairs.emplace(b,a);
                }
            }
        }
    } else{
        // seems strictly slower. i must be doing something wrong
        for (auto & a: circle_list){
            for (auto & b: circle_list){
                if (a==b) continue;

                auto it = evaluated_pairs.find({a,b});
                bool pair_already_evaluated = it != evaluated_pairs.end();
                if (pair_already_evaluated) continue;

                if (auto collision = collision_check_narrow(a,b)){
                    (*collision).respond();
                    (*collision).resolve();
                }

                evaluated_pairs.emplace_hint(it,a,b);
                evaluated_pairs.emplace(b,a);
            }
        }
    }
    for (auto& circle: circle_list){
        circle.handle_motion(timer::get_milliseconds(msPerTick()));
    }
}
void perform_tick_updates(timer::duration dt){
    tick_gap_accumulator += std::min(dt, maxGapContributionPerFrame);
    //    std::println("tick_gap_accum: {}",tick_gap_accumulator);
    //    std::println("dt: {}",dt);

    auto ticks_this_frame {0uz};
    while (tick_gap_accumulator > msPerTick() && ticks_this_frame < maxTicksPerFrame){
        per_tick_update();
        tick_gap_accumulator -= msPerTick();
        ticks_this_frame++;
    }

}


#include <climits>
static constexpr u64 kBitsPerByte = CHAR_BIT;
template<typename T>
static constexpr auto size_bytes(T const& v) noexcept
-> u32{
    return sizeof(T);
}
template<typename T>
static constexpr auto size_bits(T const& v={}) noexcept
-> u32{
    return size_bytes(v) * CHAR_BIT;
}

template<typename T>
    requires std::integral<T>
static constexpr auto get_sign_bit(T const& v) noexcept
-> i8{
    return v << (size_bits(v)-1);
}

// converts true to +1, false to -1
static constexpr i32 bool_to_signed(bool b){
    return (b << 1) - 1;
}
static_assert(bool_to_signed(true)==+1);
static_assert(bool_to_signed(false)==-1);
void apply_forcefield(bool attractiveForce, f32 influenceRadius=1.0f){
    auto mouse_pos = px_to_meters(to_glm(GetMousePosition()));
    int n_nearby = 0;
    for (auto& a: circle_list){
        auto const dist2 = glm::distance2(a.pos, mouse_pos);
        auto const rad_sum2 = glm::pow(a.radius + 2, 2);
        bool collision = dist2 < rad_sum2;
        if (collision){
            n_nearby++;
        }
    }

    i32 sign = bool_to_signed(attractiveForce);
    auto attraction = n_nearby * 0.005 + 0.2f;
    for (auto& a: circle_list){
        auto const dist2 = glm::distance2(a.pos, mouse_pos);
        auto const rad_sum2 = glm::pow(a.radius + 2, influenceRadius);
        bool collision = dist2 < rad_sum2;
        if (collision){
            // impart a force on everything moving from the centre outwards
            auto const collision_normal = a.pos - mouse_pos;
//                    auto const relative_velocity = a.vel - glm::vec2{0,0};
            auto const closing_speed = attraction *  glm::distance(a.pos,mouse_pos) / influenceRadius;
            auto const j = 
                -(1.0f + gElasticity) * closing_speed  
                /
                ( (1.0f / a.mass) + (1.0f / 1.0f));
            a.vel += sign * ((j * collision_normal) / a.mass);
        }
    }
    auto const color = attractiveForce ? __RGB(0,128,0) : __RGB(128,0,0);
    draw_circle_outline(mouse_pos,influenceRadius,color);
}

int main() {
    cpptrace::register_terminate_handler();

    InitWindow(screenExtentX_px, screenExtentY_px, "raylib-base");
    SetTargetFPS(120);

    // glm is here purely to prove it links; swap for real work.
    circle_list.push_back({
        .pos = glm::vec2{0,0}, 
    });
    circle_list.push_back({
        .pos = glm::vec2{0.2,1}, 
        .vel = glm::vec2{-0.2,0},
    });
    timer::time_point start_prev_frame = timer::now();
    while (!WindowShouldClose()) {
        auto start_cur_frame = timer::now();
        if (not_enabled(render_paused)){
            ClearBackground(BLACK);
        }
        for (auto & [dbg_toggle, enabled]: debug_toggle_registry){
            if(IsKeyPressed(dbg_toggle.keybind)){
                enabled = !enabled;
            }
        }
        if (IsKeyDown(KEY_G)){
            apply_forcefield(true);
        }
        if (IsKeyDown(KEY_F)){
            apply_forcefield(false);
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                auto mouse_pos = px_to_meters(to_glm(GetMousePosition()));
            for (int i = 0; i<16; i++){
                static constexpr auto spawn_rad = 0.05;
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec(glm::vec2(-spawn_rad),glm::vec2(spawn_rad)),
                });
            }
        }

        if (not_enabled(sim_paused)){
            perform_tick_updates(start_cur_frame - start_prev_frame);
        }

        if (not_enabled(render_paused)){
            if (is_enabled(draw_occupancy_grid_dbg)){
                occupancy_grid.draw();
                occupancy_grid.draw_dbg();

            }
            if (is_enabled(show_labels)){
                for (auto& circle: circle_list){
                    circle.draw_label();
                }
            }
            for (auto& circle: circle_list){
                circle.draw();
            }
            if (is_enabled(show_aabb)){
                for (auto& circle: circle_list){
                    circle.draw_aabb();
                }
            }

            if (is_enabled(draw_cursor_pos)){
                auto cursor_m = px_to_meters(to_glm(GetMousePosition()));
                draw_label_m(cursor_m, "pos_m: {}",cursor_m);
            }

            DrawFPS(10, 10);
            f32 y0 = 60;
            for (auto & [dbg_toggle, enabled]: debug_toggle_registry){
                dbg_toggle.draw_label({10,y0+=30},enabled);
            }
            draw_label_px({10,30},std::format("t={: .4f}",timer::get_seconds(timer::since_epoch())) );
            draw_label_px({10,60}, std::format("n={}",circle_list.size()) );
        }
        if (not_enabled(render_paused)){
            EndDrawing();
        }
        start_prev_frame = start_cur_frame;
    }

    CloseWindow();
    return 0;
}
