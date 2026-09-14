#pragma once 
#include "glm/vec2.hpp"
#include <type_traits>
struct AABB{
    AABB(
        glm::vec2 a_centre,
        glm::vec2 a_hextents
    )
        :centre(a_centre)
        ,hextents(a_hextents)
        ,points(
            {
                centre+glm::vec2{-hextents.x,-hextents.y},
                centre+glm::vec2{+hextents.x,-hextents.y},
                centre+glm::vec2{-hextents.x,+hextents.y},
                centre+glm::vec2{+hextents.x,+hextents.y},
            }
        ){}
//    static constexpr auto from_centre() const{
//    }
//    static constexpr auto from_top_left() const{
//    }
    glm::vec2 centre;
    glm::vec2 hextents;
    union{
        std::array<glm::vec2, 4> points;
        struct {
            glm::vec2 bl;
            glm::vec2 br;
            glm::vec2 tl;
            glm::vec2 tr;
        };
    };
};
static_assert(std::is_constructible_v<AABB, glm::vec2, glm::vec2>);
