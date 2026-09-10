#pragma once 
#include "conversions.hpp"
template <class... Args>
auto draw_label_px(
    glm::vec2 pos_px,
    std::string_view sv,
    glm::vec3 color01={1.0f,1.0f,1.0f},
    f32 font_size=20.0f
) -> void{
    auto const msg = std::string(sv);
    DrawText(msg.c_str(),pos_px.x,pos_px.y,font_size,to_color(color01));
}



template <class... Args>
void draw_label_m(glm::vec2 pos_m, std::format_string<Args...> fmt, Args&&... args){
    glm::vec2 pos_px = meters_to_px(pos_m);
    static constexpr auto LabelFontSize = 20;
    static constexpr auto LabelColor = LIME;
    auto const msg = std::string(std::vformat(fmt.get(), std::make_format_args(args...)));
    DrawText(msg.c_str(),pos_px.x,pos_px.y,LabelFontSize,LabelColor);
}
inline auto draw_dotted_line(
    glm::vec2 p0,
    glm::vec2 p1,
    glm::vec3 color = {1.0f,1.0f,1.0f}
) -> void{
    auto const p0_px = meters_to_px(p0);
    auto const p1_px = meters_to_px(p1);
    static constexpr auto dash_size = 2;
    static constexpr auto space_size= 2;
    DrawLineDashed( to_rayvec(p0_px), to_rayvec(p1_px), dash_size, space_size, to_color(color));

}
