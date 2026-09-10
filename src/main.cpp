#include <map>
#include <optional>
#include <unordered_set>
#define GLM_ENABLE_EXPERIMENTAL 
#include "glm/ext.hpp"
#include <glm/gtx/norm.hpp>

#include <magic_enum/magic_enum.hpp>

#include <raylib.h>



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
                auto mouse_pos = px_to_meters(to_glm(GetMousePosition()));
            for (auto& a: circle_list){
                auto const influenceRadius = 1.0f;
                auto const dist2 = glm::distance2(a.pos, mouse_pos);
                auto const rad_sum2 = glm::pow(a.radius + influenceRadius, 2);
                bool collision = dist2 < rad_sum2;
                if (collision){
                    // impart a force on everything moving from the centre outwards
                    auto const collision_normal = a.pos - mouse_pos;
                    auto const closing_speed =  glm::distance(a.pos,mouse_pos) / influenceRadius;
                    auto const j = 
                        -(1.0f + gElasticity) * closing_speed  
                        /
                        ( (1.0f / a.mass) + (1.0f / 1.0f));
                    a.vel -= (j * collision_normal) / a.mass;
                }
            }
        }
        if (IsKeyDown(KEY_F)){
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
            auto attraction = n_nearby * 0.005 + 0.2f;
            for (auto& a: circle_list){
                auto const dist2 = glm::distance2(a.pos, mouse_pos);
                auto const rad_sum2 = glm::pow(a.radius + 2, 2);
                bool collision = dist2 < rad_sum2;
                if (collision){
                    // impart a force on everything moving from the centre outwards
                    auto const collision_normal = a.pos - mouse_pos;
//                    auto const relative_velocity = a.vel - glm::vec2{0,0};
                    auto const closing_speed = attraction *  glm::distance(a.pos,mouse_pos) / 2.0f;
                    auto const j = 
                        -(1.0f + gElasticity) * closing_speed  
                        /
                        ( (1.0f / a.mass) + (1.0f / 1.0f));
                    a.vel += (j * collision_normal) / a.mass;
                }
            }
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                auto mouse_pos = px_to_meters(to_glm(GetMousePosition()));
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-1,-1},{1,1})
                });
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-1,-1},{1,1})
                });
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-1,-1},{1,1})
                });
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-1,-1},{1,1})
                });
            
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
