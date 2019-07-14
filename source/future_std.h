#pragma once

namespace future_std
{
namespace std17
{
namespace detail
{
template <typename...> struct make_void
{
    using type = void;
};
} // namespace detail

template <typename... T> using void_t = typename detail::make_void<T...>::type;
} // namespace std17
} // namespace future_std
