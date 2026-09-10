#pragma once 
#include "types.hpp"
inline auto randf(f32 min, f32 max)
-> f32 {
    auto r = std::rand() / static_cast<f32>(RAND_MAX);
    auto range = max-min;
    return r*range + min;
}
inline auto rand_vec(glm::vec2 min, glm::vec2 max)
-> glm::vec2 {
    return glm::vec2{
        randf(min.x,max.x),
        randf(min.y,max.y),
    };
}

