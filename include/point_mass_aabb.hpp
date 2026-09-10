#pragma once 
#include "point_mass.hpp"
#include "aabb.hpp"
inline auto get_aabb(Circle const& c) {
    return AABB(c.pos,{c.radius,c.radius});
}
