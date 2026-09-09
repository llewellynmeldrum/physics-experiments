#define GLM_ENABLE_EXPERIMENTAL 
#include "glm/ext.hpp"
#include <glm/gtx/norm.hpp>
#include "raylib.h"
#include "conversions.hpp"
#include "types.hpp"
#include "timer.hpp"
#include "draw_helpers.hpp"
#include "format_specs.hpp"
static constexpr auto kElasticity = 0.90f;
static constexpr auto gTimeScale = 1.00f;
static bool paused = false;
static bool labels = false;
static bool paused_rendering = false;
auto rand_vec(glm::vec2 min, glm::vec2 max){
    auto range = max-min;
    return glm::vec2{
        min.x + std::rand() % static_cast<i32>(range.x),
        min.y + std::rand() % static_cast<i32>(range.y)
    };
}
void draw_circle(glm::vec2 pos, f32 radius, glm::vec3 color){
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
    auto handle_motion(f32 msDt) -> void{
        auto const sDt = (msDt / 1000.0f) * gTimeScale;
        auto const clamped_vel = glm::clamp(vel,glm::vec2(0.0f),glm::vec2(10.0f));
        cur_color.g = 0.2 + glm::length(clamped_vel / glm::vec2(10.0f))* 0.8;

        vel += G * sDt;
        if (pos.y + vel.y*sDt + radius >= maxY){
            pos.y = maxY - radius;
            vel.y *= -kElasticity;
        }else if (pos.y + vel.y*sDt - radius < minY){
            pos.y = minY + radius;
            vel.y*=-kElasticity ;
        }
        if (pos.x + vel.x*sDt + radius >= maxX){
            pos.x = maxX - radius;
            vel.x *= -kElasticity;
        }else if (pos.x + vel.x*sDt - radius < minX){
            pos.x = minX + radius;
            vel.x*=-kElasticity ;
        }

        pos += vel * sDt;
    }

    auto draw() -> void {
        auto const pos_px = meters_to_px(pos);
        auto const radius_px = meters_to_px(radius);
        DrawCircleV(
            to_rayvec(pos_px),
            radius_px,
            to_color(cur_color)
        );
        DrawCircleV(
            to_rayvec(pos_px),
            radius_px*0.1f,
            BLACK
        );
    }
    auto draw_label(std::string_view s = ""sv) -> void {
        auto const pos_px = meters_to_px(pos);
        auto const radius_px = meters_to_px(radius);
        draw_label_px(
            pos_px+glm::vec2{0,radius_px},
            "'{}'\np: {:4.2f}\nv:{:4.2f}",s,pos,vel
        );
    }
};
std::vector<Circle> circle_list{};

// units:
// 1m = 100 pixels
// 1 pixel = 1cm 

using TickCount = u32;
TickCount ticks_per_second {200uz};
TickCount tick_count = 0uz;
timer::duration tick_gap_accumulator = timer::duration{0};
timer::time_point t_frame_start = timer::now();
constexpr auto msPerTick(){return timer::milliseconds(1000.0 / ticks_per_second);}

constexpr static auto maxGapContributionPerFrame = timer::milliseconds(250.0); // limit on how many 'lagging' ticks are CREATED
constexpr static auto maxTicksPerFrame = 8uz; // limit on how many 'lagging' ticks are ACCEPTED
    
void perform_tick_updates(timer::duration dt); // called once per frame
void per_tick_update(); // called [1,8] times per frame;


void per_tick_update(){
    for (auto& a: circle_list){
        for (auto& b: circle_list){
            if (a.get_id() == b.get_id()) break;
            auto const dist2 = glm::distance2(a.pos, b.pos);
            auto const rad_sum2 = glm::pow(a.radius + b.radius,2);
            bool collision = dist2 < rad_sum2;
            if (collision){
                std::println("Collision between {} and {}",a.pos, b.pos);
                auto const dist = glm::sqrt(dist2);
                auto const rad_sum = glm::sqrt(rad_sum2);
                // respond:
                // first find the collision vector (the collision vector from a to b = b.p-a.p)
                auto const collision_normal = glm::normalize(b.pos-a.pos);
                auto const overlap_depth = rad_sum - dist;
                // move them both by half the overlap depth

                // Push B away, in the direction of the collision from A's perspective 
                b.pos +=  collision_normal * overlap_depth*0.5f;
                // Push A away, in the direction of the collision from B's perspective 
                a.pos -=  collision_normal * overlap_depth*0.5f;

                auto const relative_velocity = (b.vel-a.vel);

                auto const closing_speed = glm::dot(relative_velocity, collision_normal);
                if (closing_speed < 0){
                    auto const j = 
                        -(1.0f + kElasticity) * closing_speed  
                        /
                        ( (1.0f / a.mass) + (1.0f / b.mass));
                    a.vel -= (j * collision_normal) / a.mass;
                    b.vel += (j * collision_normal) / b.mass;
                }



               // paused = true;
               // paused_rendering = true;
               // EndDrawing();
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
    InitWindow(screenExtentX_px, screenExtentY_px, "raylib-base");
    SetTargetFPS(120);

    // glm is here purely to prove it links; swap for real work.
    circle_list.push_back({
        .pos = glm::vec2{0,0}, 
    });
    circle_list.push_back({
        .pos = glm::vec2{0.2,1}, 
    });
    timer::time_point start_prev_frame = timer::now();
    while (!WindowShouldClose()) {
        auto start_cur_frame = timer::now();
        if (!paused_rendering){
            ClearBackground(BLACK);
        }
        if (IsKeyPressed(KEY_P)){
            paused = !paused;
        }
        if (IsKeyPressed(KEY_L)){
            labels = !labels;
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
                        -(1.0f + kElasticity) * closing_speed  
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
                    auto const relative_velocity = a.vel - glm::vec2{0,0};
                    auto const closing_speed = attraction *  glm::distance(a.pos,mouse_pos) / 2.0f;
                    auto const j = 
                        -(1.0f + kElasticity) * closing_speed  
                        /
                        ( (1.0f / a.mass) + (1.0f / 1.0f));
                    a.vel += (j * collision_normal) / a.mass;
                }
            }
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                auto mouse_pos = px_to_meters(to_glm(GetMousePosition()));
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-5,-5},{5,5})
                });
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-5,-5},{5,5})
                });
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-5,-5},{5,5})
                });
                circle_list.push_back({
                    .pos = mouse_pos + rand_vec({-5,-5},{5,5})
                });
            
        }

        if (!paused){
            perform_tick_updates(start_cur_frame - start_prev_frame);
        }

        if (!paused_rendering){
            for (auto& circle: circle_list){
                circle.draw();
                if (labels){
                    circle.draw_label();
                }
            }


            DrawFPS(10, 10);

            draw_label_px({10,30},"t={: .4f}",timer::get_seconds(timer::since_epoch()));
            draw_label_px({10,60},"n={}",circle_list.size());
        }
        if (!paused_rendering){
            EndDrawing();
        }
        start_prev_frame = start_cur_frame;
    }

    CloseWindow();
    return 0;
}
