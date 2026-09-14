#pragma once
#include <queue>
#include <sys/syslimits.h>

#include <libassert/assert.hpp>

#include "aabb.hpp"
#include "colors.hpp"
#include "conversions.hpp"
#include "logger.hpp"
#include "point_mass.hpp"
#include "point_mass_aabb.hpp"

struct OccupancyQuadtree {
    static constexpr size_t kMaxLeafCapcity = 8;
    static constexpr size_t kChildCount = 4;
    using NodeIndex = i32;
    static constexpr NodeIndex NULL_NODE = -1;
    static constexpr NodeIndex ROOT_NODE = 0;

    struct Resident {
        Circle *circle;
        glm::vec2 lo;
        glm::vec2 hi;
    };
    static constexpr auto min_hextent =
        glm::vec2(Circle::radius) * 1.25f;
    // bl, br, tl, tr
    static auto make_child_bounds(AABB const &parent) -> std::array<AABB, 4> {
        auto const hextents = parent.hextents * .5f;
        auto const &hwidth = hextents.x;
        auto const &hheight = hextents.y;

        auto const bot_left_center =
            parent.tl + glm::vec2{hwidth * 1.0f, -hheight * 3.0f};
        auto const bot_right_center =
            parent.tl + glm::vec2{hwidth * 3.0f, -hheight * 3.0f};

        auto const top_left_center =
            parent.tl + glm::vec2{hwidth * 1.0f, -hheight * 1.0f};
        auto const top_right_center =
            parent.tl + glm::vec2{hwidth * 3.0f, -hheight * 1.0f};

        return {
            AABB{bot_left_center, hextents},
            AABB{bot_right_center, hextents},
            AABB{top_left_center, hextents},
            AABB{top_right_center, hextents},
        };
    }

    static constexpr auto BL_IDX = 0;
    static constexpr auto BR_IDX = 1;
    static constexpr auto TL_IDX = 2;
    static constexpr auto TR_IDX = 3;
    struct Node {
        NodeIndex first_child_idx = {
            NULL_NODE}; // children are stored contiguously. -1 if leaf
        std::vector<Resident> residents{}; // only filled on for leaves
        AABB bounds;
        constexpr auto is_leaf() const -> bool {
            return first_child_idx == NULL_NODE;
        }
        constexpr auto is_empty() const -> bool { return residents.empty(); }
        constexpr auto leaf_occupancy() const -> size_t {
            return residents.size();
        }
        constexpr auto split_predicate() const -> bool {
            return bounds.hextents.x * 0.5f > min_hextent.x &&
                   bounds.hextents.y * 0.5f > min_hextent.y &&
                   (leaf_occupancy() > kMaxLeafCapcity);
        }
        constexpr auto midpoint() const -> glm::vec2 { return bounds.centre; }

        constexpr auto bl_child_idx() const -> NodeIndex {
            ASSERT(!is_leaf());
            return first_child_idx + BL_IDX;
        }
        constexpr auto br_child_idx() const -> NodeIndex {
            ASSERT(!is_leaf());
            return first_child_idx + BR_IDX;
        }
        constexpr auto tl_child_idx() const -> NodeIndex {
            ASSERT(!is_leaf());
            return first_child_idx + TL_IDX;
        }
        constexpr auto tr_child_idx() const -> NodeIndex {
            ASSERT(!is_leaf());
            return first_child_idx + TR_IDX;
        }
    };
    auto node_count() const -> size_t { return nodes.size(); }
    // i.e how many objects can a leaf hold before it splits.
    // start with a single leaf, covering the entire area.
    std::vector<Node> nodes;

    // Take some leaf `u`, and spawn 4 child nodes, which inherit all of its
    // residents
    void split(NodeIndex idx) {
        auto const bounds = make_child_bounds(nodes.at(idx).bounds);
        auto residents = std::move(nodes.at(idx).residents);
        nodes.at(idx).residents.clear();
        nodes.at(idx).first_child_idx = node_count();
        for (auto const &b : bounds) {
            nodes.push_back(Node{.bounds = b});
        }
        for (auto const &resident : residents) {
            insert_each_overlap(idx, resident);
        }
    }
    void insert(Circle *circle) {
        if (nodes.empty()) {
            nodes.push_back(Node{.bounds = AABB({0, 0}, screenExtent * 0.5f)});
        }
        auto resident = OccupancyQuadtree::Resident{
            .circle = circle,
            .lo = circle->pos - glm::vec2(circle->radius),
            .hi = circle->pos + glm::vec2(circle->radius),
        };
        insert_each_overlap(ROOT_NODE, resident);
    }
    void insert_each_overlap(NodeIndex idx, Resident const &resident) {
        if (nodes.at(idx).is_leaf()) {
            nodes.at(idx).residents.push_back(resident);
            if (nodes.at(idx).split_predicate()) {
                split(idx);
            }
            return;
        }
        auto const first = nodes.at(idx).first_child_idx;
        auto const mid = nodes.at(idx).midpoint();
        auto const left = resident.lo.x < mid.x;
        auto const right = resident.hi.x >= mid.x;
        auto const bot = resident.lo.y < mid.y;
        auto const top = resident.hi.y >= mid.y;

        if (bot && left) {
            insert_each_overlap(first + BL_IDX, resident);
        }
        if (bot && right) {
            insert_each_overlap(first + BR_IDX, resident);
        }
        if (top && right) {
            insert_each_overlap(first + TR_IDX, resident);
        }
        if (top && left) {
            insert_each_overlap(first + TL_IDX, resident);
        }
    }

    auto find_containing_leaf(glm::vec2 wpos) const -> NodeIndex {
        auto idx = ROOT_NODE;
        while (!nodes.at(idx).is_leaf()) {
            auto const mid = nodes.at(idx).midpoint();
            auto const hoz_idx_offset = NodeIndex{wpos.x >= mid.x};
            auto const vert_idx_offset = NodeIndex{wpos.y >= mid.y} * 2;
            idx = nodes.at(idx).first_child_idx + hoz_idx_offset +
                  vert_idx_offset;
        }
        return idx;
    }

    auto draw(bool draw_resident_lines) const -> void {
        // bfs?
        if (nodes.empty()) {
            log_WARN("Cannot draw an empty quadtree.");
            return;
        }

        std::queue<NodeIndex> q;
        q.push(ROOT_NODE);
        while (!q.empty()) {
            auto u_idx = q.front();
            q.pop();
            auto u = nodes.at(u_idx);
            if (u.is_leaf()) {
                // draw the node
                // implment
                draw_rect_outline(u.bounds.tl, u.bounds.hextents * 2.0f,
                                  colors::make_rgba(255, 0, 0, 255));
                if (draw_resident_lines){
                    for (auto const resident : u.residents) {
                        if (resident.circle) {
                            draw_line(u.bounds.centre, resident.circle->pos, 3.0f,
                            colors::make_rgba(255, 255, 255, 255));
                        }
                    }
                }
            } else {
                // add its children to the queue
                if (u.first_child_idx != NULL_NODE) {
                    for (i32 i = 0; i < kChildCount; i++) {
                        auto const child_idx = i + u.first_child_idx;
                        q.push(child_idx);
                    }
                }
            }
        }
    }
    template <typename Fn> auto for_each_pair(Fn &&fn) const -> void {
        for (NodeIndex idx = ROOT_NODE; idx < node_count(); idx++) {
            auto const &residents = nodes.at(idx).residents;
            for (auto i = 0uz; i < residents.size(); i++) {
                for (auto j = 0uz; j < i; j++) {
                    // in order to avoid duplicate cases, we only process a pair
                    // if the current leaf is the same as the leaf which
                    // occupies the minimum corner of the overlap between each
                    // circles AABBs.
                    auto const &a = residents.at(i);
                    auto const &b = residents.at(j);
                    auto const min_corner_of_overlap = glm::max(a.lo, b.lo);
                    if (find_containing_leaf(min_corner_of_overlap) == idx) {
                        std::invoke(fn, *residents[i].circle,
                                    *residents[j].circle);
                    }
                }
            }
        }
    }
};

inline auto build_occupancy_grid_quadtree(std::span<Circle> circle_list)
    -> OccupancyQuadtree {
    auto tree = OccupancyQuadtree{};
    for (auto &c : circle_list) {
        tree.insert(&c);
    }
    return tree;
}
