#pragma once 
#include <unordered_set>
namespace std{
    template<typename A, typename B>
    struct hash<std::pair<A,B>>{
        constexpr std::size_t operator()(std::pair<A,B> const& e) const noexcept{
            return std::hash<A>{}(e.first) ^ (std::hash<B>{}(e.second) << 3U);
        }
    };
};
