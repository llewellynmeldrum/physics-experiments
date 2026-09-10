#pragma once 
#include "glm/vec2.hpp"
struct AABB{
    AABB(
        glm::vec2 centre,
        glm::vec2 extents
    )
    :points(
        {
            centre-glm::vec2{-extents.x,-extents.y},
            centre-glm::vec2{+extents.x,-extents.y},
            centre-glm::vec2{-extents.x,+extents.y},
            centre-glm::vec2{+extents.x,+extents.y},
        }
    ){}
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
