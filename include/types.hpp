#pragma once
#include <cstdint>
#include <numeric>
#include <string_view>

// NOTE: I know this is considered bad practice, and I probably wont do it for the ""s operators, but i think ""sv is unique enough that it shouldnt matter.
// I just really dislike c strings.
using namespace std::string_view_literals;

#define st_cast static_cast



// =================================================
//   zig/rust-like typedefs for fixed width types
// =================================================
using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using u64   = uint64_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

using f32 = float;
using f64 = double;

// Parity with glsl
using uint = u32;




// Absolutely the worst name in the entire STL. 
// FYI, ::min() doesnt return the min on floating point types, it returns 0.
template<typename T>
constexpr inline T numeric_min = std::numeric_limits<T>::lowest();

template<typename T>
constexpr inline T numeric_max = std::numeric_limits<T>::max();


template<typename T> requires std::integral<T>
constexpr inline u64 numeric_range = static_cast<u64>(numeric_max<T>) - static_cast<u64>(numeric_min<T>);

constexpr static inline std::size_t N_CARDINAL_DIRECTIONS {4};


using TickCount = u32;

#define NOP ((void)0)
#define CONCAT(a, b) a##b


#ifdef TYPES_H_CLANG_ARBITRARY_WIDTH_INTEGERS
// TODO: Add this if i need these in the future.
using u2 = _BitInt(2); using u3 = _BitInt(3); using u4 = _BitInt(4);
using u5 = _BitInt(5); using u6 = _BitInt(6); using u7 = _BitInt(7);
#endif 
