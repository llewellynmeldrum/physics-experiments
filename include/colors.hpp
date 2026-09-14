#pragma once 
#include "types.hpp"
namespace colors{
static inline constexpr glm::vec3 make_rgb(u8 r, u8 g, u8 b){
    return glm::vec3{r/255.0f,g/255.0f,b/255.0f};
}
static inline constexpr glm::vec4 make_rgba(u8 r, u8 g, u8 b, u8 a){
    return glm::vec4{r/255.0f,g/255.0f,b/255.0f, a/255.0f};
}

static inline constexpr glm::vec3 make_gray(u8 gray){
    return glm::vec3{gray/255.0f,gray/255.0f,gray/255.0f};
}
}; // namespace colors
