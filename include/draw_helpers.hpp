#pragma once
#include "conversions.hpp"
#include "raylib.h"
static constexpr auto kTextSpacing = 1.0f;
template <class... Args>
auto draw_label_px(glm::vec2 pos_px, std::string_view sv,
                   glm::vec3 color01 = {1.0f, 1.0f, 1.0f},
                   f32 font_size = 20.0f) -> void {
    auto const msg = std::string(sv);
    DrawTextEx(GetFontDefault(), msg.c_str(), to_rayvec(pos_px), font_size,
               kTextSpacing, to_color(color01));
}

template <class... Args>
void draw_label_m(glm::vec2 pos_m, std::format_string<Args...> fmt,
                  Args &&...args) {
    glm::vec2 pos_px = meters_to_px(pos_m);
    static constexpr auto LabelFontSize = 20;
    static constexpr auto LabelColor = LIME;
    auto const msg =
        std::string(std::vformat(fmt.get(), std::make_format_args(args...)));
    DrawTextEx(GetFontDefault(), msg.c_str(), to_rayvec(pos_px), LabelFontSize,
               kTextSpacing, LabelColor);
}
inline auto draw_line(glm::vec2 p0, glm::vec2 p1, f32 thick,
                      glm::vec3 color = {1.0f, 1.0f, 1.0f}) -> void {
    auto const p0_px = meters_to_px(p0);
    auto const p1_px = meters_to_px(p1);
    DrawLineEx(to_rayvec(p0_px), to_rayvec(p1_px), thick, to_color(color));
}
inline auto
draw_rect_px(glm::vec2 top_left_px, glm::vec2 extents_px,
             glm::vec4 fill_color = {1.0f, 1.0f, 1.0f, 1.0f},
             std::optional<glm::vec4> outline_color_opt = std::nullopt)
    -> void {
    glm::vec4 outline_color = outline_color_opt.value_or(fill_color);
    DrawRectangleV(to_rayvec(top_left_px), to_rayvec(extents_px),
                   to_color(fill_color));
    DrawRectangleLines(top_left_px.x, top_left_px.y, extents_px.x, extents_px.y,
                       to_color(outline_color));
}
inline auto
draw_rect_outline(glm::vec2 top_left, glm::vec2 extents,
                  glm::vec4 fill_color = {1.0f, 1.0f, 1.0f, 1.0f},
                  std::optional<glm::vec4> outline_color_opt = std::nullopt)
    -> void {
    glm::vec4 outline_color = outline_color_opt.value_or(fill_color);
    auto const top_left_px = meters_to_px(top_left);
    auto const extents_px = glm::vec2{meters_to_px(extents.x),meters_to_px(extents.y)};
    DrawRectangleLines(top_left_px.x, top_left_px.y, extents_px.x, extents_px.y,
                       to_color(outline_color));
}
inline auto draw_rect(glm::vec2 top_left, glm::vec2 extents,
                      glm::vec4 fill_color = {1.0f, 1.0f, 1.0f, 1.0f},
                      std::optional<glm::vec4> outline_color_opt = std::nullopt)
    -> void {
    glm::vec4 outline_color = outline_color_opt.value_or(fill_color);
    auto const top_left_px = meters_to_px(top_left);
    auto const extents_px = meters_to_px(extents);
    draw_rect_px(top_left_px, extents_px, fill_color, outline_color);
}
