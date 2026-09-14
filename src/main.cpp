#include <bit>
#include <map>
#include <optional>
#include <print>
#include <raylib.h>
#include <unordered_set>

#include <cpptrace/basic.hpp>
#include <libassert/assert.hpp>
#include <magic_enum/magic_enum.hpp>

#include "aabb.hpp"
#include "bit_utils.hpp"
#include "collision_context.hpp"
#include "colors.hpp"
#include "conversions.hpp"
#include "debug_toggles.hpp"
#include "draw_helpers.hpp"
#include "format_specs.hpp"
#include "glm/ext.hpp"
#include "glm_types.hpp"
#include "globals.hpp"
#include "logger.hpp"
#include "occupancy_quadtree.hpp"
#include "pair_hashing.hpp"
#include "pod_format.hpp"
#include "point_mass.hpp"
#include "rand.hpp"
#include "timer.hpp"
#include "types.hpp"

struct FrameStats {
    f32 input_time_ms{0.0f};
    f32 draw_time_ms{0.0f};
    f32 update_time_ms{0.0f};
    f32 frame_time_ms{0.0f};
    u64 collision_pairs_evaluated{0};
    u64 collisions_detected{0};
    u64 collisions_responded{0};
    u64 collisions_resolved{0};
    u64 occupancy_group_count{0};
    u64 collision_pairs_evaluated_per_occupancy_group{0};
};

std::vector<Circle> circle_list{};
OccupancyQuadtree occupancy_quadtree{};
FrameStats stats;

void spawn_circles(glm::vec2 mouse_pos_w, size_t num_to_spawn, f32 spawn_rad) {
    for (int i = 0; i < num_to_spawn; i++) {
        auto const jitter =
            rand_vec(glm::vec2(-spawn_rad), glm::vec2(spawn_rad));
        circle_list.push_back({.pos = mouse_pos_w + jitter});
    }
}
void reset_sim() {
    circle_list.clear();
    stats = {};
    occupancy_quadtree = {};
    static constexpr auto START_AMOUNT = 4000;
    spawn_circles({0,0},START_AMOUNT,screenExtentY*0.5f);
}

void draw_hud() {
    DrawFPS(10, 10);
    auto y0 = 30;
    auto x0 = 10;
    auto show_info_line = [&](std::string_view sv) {
        draw_label_px({x0, y0}, sv);
        y0 += 30;
    };
    show_info_line(
        std::format("t={: .4f}", timer::get_seconds(timer::since_epoch())));
    show_info_line(std::format("input %={: .4f}%", 100.0f * stats.input_time_ms / stats.frame_time_ms));
    show_info_line(std::format("draw %={: .4f}%", 100.0f * stats.draw_time_ms / stats.frame_time_ms));
    show_info_line(std::format("update %={: .4f}%", 100.0f * stats.update_time_ms / stats.frame_time_ms));
    show_info_line(std::format("n={}", circle_list.size()));
    show_info_line(std::format("timescale: {} (UP/DOWN to modify)", gTimeScale));

    for (auto &[dbg_toggle, enabled] : debug_toggle_registry) {
        dbg_toggle.draw_label({10, y0 += 30}, enabled);
    }

    std::string stats_str = std::format("{}", stats);
    static constexpr auto font_size = 20.0f;
    auto size_px = to_glm(MeasureTextEx(GetFontDefault(), stats_str.c_str(),
                                        font_size, kTextSpacing));
    auto pos_px = glm::vec2{screenExtentX_px-size_px.x, screenExtentY_px - size_px.y};
    static constexpr auto fill_color = colors::make_rgba(122, 120, 128, 128);
    static constexpr auto out_color = colors::make_rgba(60, 61, 64, 128);
    draw_label_px(pos_px, stats_str);

    auto padding_px = 20.0f;
    pos_px -= glm::vec2(padding_px * 0.5f);
    size_px += glm::vec2(padding_px);
    draw_rect_px(pos_px, size_px, fill_color, out_color);
}

TickCount ticks_per_second{200uz};
TickCount tick_count = 0uz;
timer::duration tick_gap_accumulator = timer::duration{0};
timer::time_point t_frame_start = timer::now();
constexpr auto msPerTick() {
    return timer::milliseconds(1000.0 / ticks_per_second);
}

constexpr static auto maxGapContributionPerFrame =
    timer::milliseconds(250.0); // limit on how many 'lagging' ticks are CREATED
constexpr static auto maxTicksPerFrame =
    8uz; // limit on how many 'lagging' ticks are ACCEPTED

void perform_tick_updates(timer::duration dt); // called once per frame
void per_tick_update();                        // called [1,8] times per frame;

// Make the bodies no longer intersect.
// apply the resultant forces of the collision
void per_tick_update() {
    // TODO: implement broad phase:
    // Create a uniform grid based AABB residency map.
    // For each body, add each cell which its AABB intersects with. AABB v AABB
    // intersection test
    occupancy_quadtree = build_occupancy_grid_quadtree(circle_list);
    stats = {};
    // iterate over the grid, and check for collisions only between circles in
    // the same grid

    stats.occupancy_group_count = occupancy_quadtree.node_count();
    if (is_enabled(use_occupancy_grid)) {
        // anyways idk how to iterate this atm
        occupancy_quadtree.for_each_pair([&](Circle &a, Circle &b) {
            stats.collision_pairs_evaluated++;

            if (auto collision = collision_check_narrow(a, b)) {
                stats.collisions_detected++;
                (*collision).respond();

                bool resolution_took_place = (*collision).resolve();
                stats.collisions_resolved += resolution_took_place;
            }
        });
    } else {
        for (auto &a : circle_list) {
            for (auto &b : circle_list) {
                if (a == b)
                    continue;
                stats.collision_pairs_evaluated++;
                if (auto collision = collision_check_narrow(a, b)) {
                    stats.collisions_detected++;
                    (*collision).respond();
                    bool resolution_took_place = (*collision).resolve();
                    stats.collisions_resolved += resolution_took_place;
                }
            }
        }
    }
    stats.collision_pairs_evaluated_per_occupancy_group =
        stats.collision_pairs_evaluated / stats.occupancy_group_count;
    for (auto &circle : circle_list) {
        circle.handle_motion(timer::get_milliseconds(msPerTick()));
    }
}
void perform_tick_updates(timer::duration dt) {
    tick_gap_accumulator += std::min(dt, maxGapContributionPerFrame);
    //    std::println("tick_gap_accum: {}",tick_gap_accumulator);
    //    std::println("dt: {}",dt);

    auto ticks_this_frame{0uz};
    while (tick_gap_accumulator > msPerTick() &&
           ticks_this_frame < maxTicksPerFrame) {
        per_tick_update();
        tick_gap_accumulator -= msPerTick();
        ticks_this_frame++;
    }
}

void apply_forcefield(bool attractiveForce, f32 influenceRadius = 1.0f) {
    auto mouse_pos = px_to_meters(to_glm(GetMousePosition()));
    int n_nearby = 0;
    for (auto &a : circle_list) {
        auto const dist2 = glm::distance2(a.pos, mouse_pos);
        auto const rad_sum2 = glm::pow(a.radius + 2, 2);
        bool collision = dist2 < rad_sum2;
        if (collision) {
            n_nearby++;
        }
    }

    i32 sign = bool_to_signed(attractiveForce);
    auto attraction = n_nearby * 0.005 + 0.2f;
    for (auto &a : circle_list) {
        auto const dist2 = glm::distance2(a.pos, mouse_pos);
        auto const rad_sum2 = glm::pow(a.radius + 2, influenceRadius);
        bool collision = dist2 < rad_sum2;
        if (collision) {
            // impart a force on everything moving from the centre outwards
            auto const collision_normal = a.pos - mouse_pos;
            //                    auto const relative_velocity = a.vel -
            //                    glm::vec2{0,0};
            auto const closing_speed =
                attraction * glm::distance(a.pos, mouse_pos) / influenceRadius;
            auto const j = -(1.0f + gElasticity) * closing_speed /
                           ((1.0f / a.mass) + (1.0f / 1.0f));
            a.vel += sign * ((j * collision_normal) / a.mass);
        }
    }
    auto const color = attractiveForce ? colors::make_rgb(0, 128, 0)
                                       : colors::make_rgb(128, 0, 0);
    draw_circle_outline(mouse_pos, influenceRadius, color);
}

void handle_input() {
    for (auto &[dbg_toggle, enabled] : debug_toggle_registry) {
        if (IsKeyPressed(dbg_toggle.keybind)) {
            enabled = !enabled;
        }
    }
    if (IsKeyDown(KEY_G)) {
        apply_forcefield(true);
    }
    if (IsKeyDown(KEY_E)) {
        //            auto const mouse_pos =
        //            px_to_meters(to_glm(GetMousePosition()));
        // erase_circles(mouse_pos, 10000, 0.5);
    }
    if (IsKeyDown(KEY_R)) {
        reset_sim();
    }
    if (IsKeyDown(KEY_F)) {
        apply_forcefield(false);
    }
    if (IsKeyPressed(KEY_UP)) {
        gTimeScale = std::clamp(gTimeScale + gTimeScaleInterval, gTimeScaleMin,
                                gTimeScaleMax);
    }
    if (IsKeyPressed(KEY_DOWN)) {
        gTimeScale = std::clamp(gTimeScale - gTimeScaleInterval, gTimeScaleMin,
                                gTimeScaleMax);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        auto const mouse_pos = px_to_meters(to_glm(GetMousePosition()));
        spawn_circles(mouse_pos, 1, 0.05);
    }
}
void draw_scene() {
    ClearBackground(BLACK);

    if (is_enabled(show_labels)) {
        for (auto &circle : circle_list) {
            circle.draw_label();
        }
    }
    for (auto &circle : circle_list) {
        circle.draw();
    }
    if (is_enabled(show_aabb)) {
        for (auto &circle : circle_list) {
            circle.draw_aabb();
        }
    }
    if (is_enabled(show_velocity_vector)) {
        for (auto &circle : circle_list) {
            circle.draw_velocity_vector();
        }
    }

    if (is_enabled(draw_occupancy_grid_dbg)) {
        occupancy_quadtree.draw(is_enabled(draw_quadtree_residency));
    }

    if (is_enabled(draw_cursor_pos)) {
        auto cursor_m = px_to_meters(to_glm(GetMousePosition()));
        draw_label_m(cursor_m, "pos_m: {}", cursor_m);
    }

}

void intHandler(int dummy) {
    cpptrace::generate_trace().print();
    std::exit(EXIT_FAILURE);
}
int main() {
    cpptrace::register_terminate_handler();
    signal(SIGINT, intHandler);

    InitWindow(screenExtentX_px, screenExtentY_px, "raylib-base");
    SetTargetFPS(120);

    // glm is here purely to prove it links; swap for real work.
    reset_sim();
    timer::time_point start_prev_frame = timer::now();

    while (!WindowShouldClose()) {
        auto start_cur_frame = timer::now();

        {
            auto t0 = timer::now();
            handle_input();
            stats.input_time_ms = timer::to_milliseconds(timer::since(t0));
        }

        {
            auto t0 = timer::now();
            if (not_enabled(sim_paused)) {
                perform_tick_updates(start_cur_frame - start_prev_frame);
            }
            stats.update_time_ms= timer::to_milliseconds(timer::since(t0));
        }

        {
            auto t0 = timer::now();
            BeginDrawing();
            draw_scene();
            stats.draw_time_ms = timer::to_milliseconds(timer::since(t0));
        }

        stats.frame_time_ms =
            timer::to_milliseconds(timer::since(start_cur_frame));
        if (is_enabled(show_hud)){
            draw_hud();
        }
        EndDrawing();

        start_prev_frame = start_cur_frame;
    }

    CloseWindow();
    return 0;
}
