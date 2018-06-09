#pragma once

#include <tuple>

template <class... Args>
struct TypeTuple {
    using TupleType = std::tuple<Args...>;
    template <std::size_t N>
    using get = typename std::tuple_element<N, TupleType>::type;
    using size = std::tuple_size<TupleType>;
    static constexpr auto size_v = size::value;
};
