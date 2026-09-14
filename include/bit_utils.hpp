#pragma once 
#include <climits>
#include "types.hpp"

static constexpr u64 kBitsPerByte = CHAR_BIT;
static_assert(kBitsPerByte == 8, 
    "If this fails wtf architecture are you on and in what universe does it compile c++23 anyways");

template<typename T>
static constexpr auto size_bytes(T const& v) noexcept -> u32{ return sizeof(T); }
template<typename T>
static constexpr auto size_bits(T const& v={}) noexcept -> u32{ return size_bytes(v) * CHAR_BIT; }

template<typename T> requires std::signed_integral<T>
static constexpr auto get_sign_bit(T const& v) noexcept -> i8{ return v << (size_bits(v)-1); }

// converts true to +1, false to -1
static constexpr i32 bool_to_signed(bool b){
    return (b << 1) - 1;
}
static_assert(bool_to_signed(true)==+1);
static_assert(bool_to_signed(false)==-1);
