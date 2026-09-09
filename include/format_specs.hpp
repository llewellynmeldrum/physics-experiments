#pragma once

#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "types.hpp"
#include <format>
#include <type_traits>


#include <format>
#include <concepts>

#include <glm/glm.hpp>

namespace std {

template<
    glm::length_t L,
    std::floating_point T,
    glm::qualifier Q,
    typename CharT
>
struct formatter<glm::vec<L, T, Q>, CharT>
    : formatter<T, CharT>
{
    using scalar_formatter = formatter<T, CharT>;

    template<typename FormatContext>
    auto format(
        glm::vec<L, T, Q> const& vec,
        FormatContext& ctx
    ) const
    {
        auto out = ctx.out();

        *out++ = static_cast<CharT>('(');
        ctx.advance_to(out);

        for (glm::length_t i = 0; i < L; ++i) {
            if (i != 0) {
                out = ctx.out();
                *out++ = static_cast<CharT>(',');
                *out++ = static_cast<CharT>(' ');
                ctx.advance_to(out);
            }

            ctx.advance_to(
                scalar_formatter::format(vec[i], ctx)
            );
        }

        out = ctx.out();
        *out++ = static_cast<CharT>(')');

        return out;
    }
};

}
// template<size_t t_storage_offset, size_t t_n_bits, typename t_storage_type>
// struct std::formatter<BitFieldMember<t_storage_offset, t_n_bits,t_storage_type>>{
//     using T = BitFieldMember<t_storage_offset, t_n_bits,t_storage_type>;
// 
//     bool binary = false;
// 	constexpr auto parse(std::format_parse_context& ctx){
//         auto it = ctx.begin();
// 
//         if (it != ctx.end() && *it == 'b') {
//             binary = true;
//             ++it;
//         }
// 
//         if (it != ctx.end() && *it != '}')
//             throw format_error{"invalid POD format specifier"};
// 
//         return it;
//     }
// 	auto format(T const& val, auto& ctx)const {
//         std::string res{""};
//         if (binary)
//             return std::format_to(ctx.out(), "[{}]",res);
//         else
//             return std::format_to(ctx.out(), "[{}]",val.get);
//     }
// };

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct std::formatter<glm::mat<C,R,T,Q>>{

	constexpr auto parse(std::format_parse_context& ctx){return ctx.begin();}
	auto format(const glm::mat4& val, auto& ctx)const {
        std::string res{};
        res.append("\n");
        for (i64 row = 0; row < R; row++) {
            res.append("| ");
            for (i64 col = 0; col < C; col++) {
                res.append(std::format("{: 3.1f}", val[col][row]));
                if (col != C - 1) {
                    res.append(" ");
                }
            }
            res.append(" |");
            if (row != R - 1)
                res.append("\n");
        }
        return format_to(ctx.out(), "{}",res);
    }
};
