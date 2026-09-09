#pragma once 
#include "conversions.hpp"
template <class... _Args>
void draw_label_px(glm::vec2 pos_px, std::format_string<_Args...> fmt, _Args&&... args){
    static constexpr auto LabelFontSize = 20;
    static constexpr auto LabelColor = LIME;
    auto const msg = std::string(std::vformat(fmt.get(), std::make_format_args(args...)));
    DrawText(msg.c_str(),pos_px.x,pos_px.y,LabelFontSize,LabelColor);
}
template <class... _Args>
void draw_label_m(glm::vec2 pos_m, std::format_string<_Args...> fmt, _Args&&... args){
    glm::vec2 pos_px = meters_to_px(pos_m);
    static constexpr auto LabelFontSize = 20;
    static constexpr auto LabelColor = LIME;
    auto const msg = std::string(std::vformat(fmt.get(), std::make_format_args(args...)));
    DrawText(msg.c_str(),pos_px.x,pos_px.y,LabelFontSize,LabelColor);
}
