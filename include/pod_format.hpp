#pragma once

#include "../reflect/reflect"

// -- std --
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>


// =====================================================================
// std::formatter for POD-like aggregates that aren't already formattable.
//
//   struct Foo { int member1{42}; glm::vec2 member2{}; };
//   std::println("{}", Foo{});
//
//   Foo{
//       member1: 42
//       member2: (0, 0)
//   }
//
// Format specs:
//   {}    multi-line, 4 space indent
//   {:c}  compact, single line -> Foo{member1: 42, member2: (0, 0)}
//   {:8}  multi-line, 8 space indent
//
// NOTE: a raw C array member (int raw[3]) breaks reflect's member counting --
// it counts the elements, not the array. Use std::array<int, 3> instead.
// =====================================================================

namespace pod_fmt {

// Only claim types std::format cannot already handle:
//  - aggregate:  required for structured bindings / reflect
//  - not array:  arrays are aggregates, but reflect can't bind them
//  - not union:  unions can be aggregates, structured bindings reject them
//  - not range:  C++23 already formats ranges (std::array, ...)
// Non-aggregates (glm::vec, std::string, std::pair, ...) never match, so any
// hand-written formatter for them keeps winning.
template<typename T>
concept ReflectablePod =
       std::is_aggregate_v<T>
    && !std::is_array_v<T>
    && !std::is_union_v<T>
    && !std::ranges::range<T>;

// Append `text`, pushing every line but the first over by `indent` spaces, so
// anything multi-line (the glm::mat formatter, a range of nested PODs) stays
// lined up under its member name.
inline void append_indented(std::string& out, std::string_view text, std::size_t indent)
{
    for (const char c : text) {
        out.push_back(c);
        if (c == '\n')
            out.append(indent, ' ');
    }
}

template<ReflectablePod T>
void append_pod(std::string& out, T const& val, bool compact, std::size_t indent, std::size_t depth);

// One member. The std::formattable case is the whole story for most PODs;
// enums and pointers just keep common members from turning into a wall of
// template errors.
template<typename M>
void append_member(std::string& out, M const& member, bool compact, std::size_t indent, std::size_t depth)
{
    // Recurse directly rather than via std::format, so nested PODs inherit the
    // compact flag and indent width instead of falling back to the defaults.
    if constexpr (ReflectablePod<M>) {
        append_pod(out, member, compact, indent, depth);
    }
    else if constexpr (std::formattable<const M&, char>) {
        append_indented(out, std::format("{}", member), compact ? 0 : indent * depth);
    }
    else if constexpr (std::is_enum_v<M>) {
        // reflect only names values in [REFLECT_ENUM_MIN, REFLECT_ENUM_MAX);
        // fall back to the underlying value for flag/out-of-range enums.
        const std::string_view name = reflect::enum_name(member);
        if (name.empty())
            std::format_to(std::back_inserter(out), "{}", reflect::to_underlying(member));
        else
            out.append(name);
    }
    else if constexpr (std::is_pointer_v<M> && std::is_object_v<std::remove_pointer_t<M>>) {
        std::format_to(std::back_inserter(out), "{}", static_cast<const void*>(member));
    }
    else {
        static_assert(!sizeof(M*), "POD member type has no std::formatter");
    }
}

template<ReflectablePod T>
void append_pod(std::string& out, T const& val, bool compact, std::size_t indent, std::size_t depth)
{
    out.append(reflect::type_name<T>());
    out.push_back('{');

    reflect::for_each([&](auto I) {
        if (compact)
            out.append(I == 0 ? "" : ", ");
        else
            out.append("\n").append(indent * (depth + 1), ' ');

        out.append(reflect::member_name<I, T>()).append(": ");
        append_member(out, reflect::get<I>(val), compact, indent, depth + 1);
    }, val);

    if (!compact && reflect::size<T>() != 0)
        out.append("\n").append(indent * depth, ' ');

    out.push_back('}');
}

}


template<pod_fmt::ReflectablePod T, typename CharT>
struct std::formatter<T, CharT> {

    bool        compact {false};
    std::size_t indent  {4};

    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto       it  = ctx.begin();
        const auto end = ctx.end();

        if (it != end && *it == 'c') {
            compact = true;
            ++it;
        }

        if (it != end && *it >= '0' && *it <= '9') {
            indent = 0;
            while (it != end && *it >= '0' && *it <= '9')
                indent = indent * 10 + static_cast<std::size_t>(*it++ - '0');
        }

        if (it != end && *it != '}')
            throw std::format_error{"invalid POD format spec (expected 'c' and/or an indent width)"};

        return it;
    }

    auto format(T const& val, auto& ctx) const
    {
        std::string res{};
        pod_fmt::append_pod(res, val, compact, indent, 0);
        return std::format_to(ctx.out(), "{}", res);
    }
};
