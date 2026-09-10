#pragma once 
#include "draw_helpers.hpp"
#include "magic_enum/magic_enum.hpp"
#include "types.hpp"
#include <libassert/assert.hpp>
#include <map>
#include <algorithm>
#include <compare>

struct DebugToggle{
    std::string_view name;
    bool default_status;
    KeyboardKey keybind = KEY_N;
    std::string_view keybind_name;
    constexpr DebugToggle(const char* a_name, bool a_enabled, KeyboardKey a_keybind)
        : name(a_name)
        , default_status(a_enabled)
        , keybind(a_keybind)
        , keybind_name(magic_enum::enum_name(a_keybind).substr(4))
    {}
    auto draw_label(glm::vec2 pos, bool enabled)const -> void {
        draw_label_px(
            pos,
            std::format(
                "[{:^3}] -> {} : {}", 
                keybind_name,
                name,
                (enabled ? "on" : "off")
            ),
            (enabled ? __RGB(32,128,12) : __RGB(16,64,6))
        );
    }
    auto toggle() -> void {default_status = !default_status;}
    auto operator<=>(DebugToggle const& b)const noexcept = default;
//        return this->name <=> b.name;
//    }
};

#define LIST_DEBUG_TOGGLES                      \
    X(use_occupancy_grid,      true, KEY_N)        \
    X(sim_paused,              false, KEY_P)                  \
    X(render_paused,           false, KEY_R)               \
    X(show_labels,             false, KEY_L)                 \
    X(draw_occupancy_grid_dbg, false, KEY_O)                 \
    X(draw_cursor_pos,         false, KEY_C)                 \
    X(show_aabb,               false, KEY_A)                   \

#define X(name, ...) inline constexpr DebugToggle name{#name, ##__VA_ARGS__};
    LIST_DEBUG_TOGGLES
#undef X

//namespace std{
//    template<>
//    struct hash<DebugToggle>{
//        constexpr std::size_t operator()(std::pair<A,B> const& e) const noexcept{
//            return std::hash<A>{}(e.first) ^ (std::hash<B>{}(e.second) << 3U);
//        }
//    };
//};

static_assert(show_aabb.keybind==KEY_A);
inline std::map<DebugToggle, bool> debug_toggle_registry{
#define X(name, ...) std::pair<DebugToggle,bool>{name, name.default_status},
    LIST_DEBUG_TOGGLES
#undef X
};
inline constexpr bool is_enabled(DebugToggle const& opt){
    ASSERT(debug_toggle_registry.contains(opt));
    return debug_toggle_registry.at(opt);
}
inline constexpr bool not_enabled(DebugToggle const& opt){
    return !is_enabled(opt);
}
