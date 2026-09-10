#pragma once 
#include "conversions.hpp"
#include "point_mass.hpp"
#include "draw_helpers.hpp"
#include "debug_toggles.hpp"
#include "point_mass_aabb.hpp"
struct OccupancyGrid{
    static constexpr size_t N_CELLS_PER_METER = 2;
    static constexpr size_t xCellCount = screenExtentX * N_CELLS_PER_METER; // 1 cell per meter
    static constexpr size_t yCellCount = screenExtentY * N_CELLS_PER_METER;
    static constexpr size_t CellCount = xCellCount * yCellCount;
    static constexpr f32 xCellWidth = screenExtentX/xCellCount;
    static constexpr f32 yCellWidth = screenExtentY/yCellCount;
    std::array<std::vector<Circle *>, CellCount> data;
    decltype(auto) cell_at(this auto& self, glm::ivec2 p){
        ASSERT(p.y < yCellCount);
        ASSERT(p.x < xCellCount);
        ASSERT(p.y >= 0);
        ASSERT(p.x >= 0);
        return self.data.at(p.y * xCellCount + p.x);
    }
    static constexpr auto world_to_cell(glm::vec2 w){

        static constexpr auto lo = glm::ivec2{0,0};
        static constexpr auto hi = glm::ivec2{xCellCount-1,yCellCount-1};
        return 
        glm::clamp(
            glm::ivec2{
            static_cast<i32>( (w.x-minX) / xCellWidth ),
            static_cast<i32>( (w.y-minY) / yCellWidth ),
        },lo,hi
        );
    }
    static constexpr auto cell_to_world(glm::ivec2 c){
        return glm::vec2{
            c.x * xCellWidth + minX,
            c.y * yCellWidth + minY,
        };
    }
    auto occupy(Circle * circle, glm::ivec2 cell){
        cell_at(cell).push_back(circle);
    }

    auto draw_dbg() const{
        for (i32 cy = 0; cy < yCellCount; cy++){
            for (i32 cx = 0; cx < xCellCount; cx++){
                auto const& residents = cell_at({cx,cy});
                f32 const xlo = minX + cx*xCellWidth;
                f32 const ylo = minY + cy*yCellWidth;
                auto const cell_center = glm::vec2{xlo+xCellWidth*0.5f, ylo + yCellWidth*0.5f};
                for (auto const& resident: residents){
                    draw_dotted_line(resident->pos, cell_center,__RGB(128,175,128));
                    draw_label_m(cell_center, "n={}",residents.size());
                }
            }
        }

    }
    auto draw() const{
        for (i32 cx = 0; cx < xCellCount; cx++){
            f32 const x = minX + cx*xCellWidth;
            auto const p0_px = meters_to_px(glm::vec2{x,minY});
            auto const p1_px = meters_to_px(glm::vec2{x,maxY});
            static constexpr auto dash_size = 2;
            static constexpr auto space_size= 2;
            DrawLineDashed( to_rayvec(p0_px), to_rayvec(p1_px), dash_size, space_size, to_color(__RGB(128,128,128)));
        }
        for (i32 cy = 0; cy < yCellCount; cy++){
            f32 const y = minY + cy*yCellWidth;
            auto const p0_px = meters_to_px(glm::vec2{minX, y});
            auto const p1_px = meters_to_px(glm::vec2{maxX, y});
            static constexpr auto dash_size = 2;
            static constexpr auto space_size= 2;
            DrawLineDashed( to_rayvec(p0_px), to_rayvec(p1_px), dash_size, space_size, to_color(__RGB(128,128,128)));
        }
    }
};

inline auto build_occupancy_grid(std::span<Circle> circle_list)
-> OccupancyGrid{
    auto grid = OccupancyGrid{};
    for (auto& c: circle_list){
        auto aabb = get_aabb(c);
        auto lo = aabb.bl;
        auto hi = aabb.tr;
        for (i32 wx = std::floor(lo.x); wx<=std::ceil(hi.x); wx++){
            for (i32 wy = std::floor(lo.y); wy<=std::ceil(hi.y); wy++){
                // The circle should occupy each and every cell between the bounds of its AABB
                auto const world_pos = glm::vec2{wx,wy};
                auto const cell = grid.world_to_cell(world_pos);
                grid.occupy(&c, cell);
            }
        }

    }
    return grid;
}
