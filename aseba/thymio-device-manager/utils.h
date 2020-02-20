#pragma once

#include <tl/expected.hpp>
#include <limits>
#include <type_traits>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>


#ifdef _MSC_VER
#    define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#else
#    define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

namespace mobsya {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

namespace detail {
    // common_signess evaluates to the signed version of the first parameters, unless both are unsigned
    template <typename T, typename V, typename _ = void>
    struct common_signess;

    template <typename T, typename V>
    struct common_signess<T, V, std::enable_if_t<(std::is_signed_v<T> || std::is_signed_v<V>)>> {
        using type = std::make_signed_t<T>;
    };
    template <typename T, typename V>
    struct common_signess<T, V, std::enable_if_t<!std::is_signed_v<T> && !std::is_signed_v<V>>> {
        using type = T;
    };

    // Given to integral types, evaluates to the one able to hold the largest value, signed if either type is
    template <typename T, typename V, typename _ = void>
    struct largest_common_type;

    template <typename T, typename V>
    struct largest_common_type<T, V, std::enable_if_t<(sizeof(T) > sizeof(V))>> {
        using type = typename common_signess<T, V>::type;
    };
    template <typename T, typename V>
    struct largest_common_type<T, V, std::enable_if_t<(sizeof(T) <= sizeof(V))>> {
        using type = typename common_signess<V, T>::type;
    };

    template <typename T, typename V>
    using largest_common_type_t = typename largest_common_type<T, V>::type;

    static_assert(std::is_same_v<largest_common_type_t<int8_t, uint8_t>, int8_t>);
    static_assert(std::is_same_v<largest_common_type_t<uint8_t, int8_t>, int8_t>);
    static_assert(std::is_same_v<largest_common_type_t<uint8_t, uint8_t>, uint8_t>);
    static_assert(std::is_same_v<largest_common_type_t<int16_t, uint8_t>, int16_t>);
    static_assert(std::is_same_v<largest_common_type_t<uint8_t, int16_t>, int16_t>);
    static_assert(std::is_same_v<largest_common_type_t<int16_t, uint32_t>, int32_t>);
}  // namespace detail

enum class cast_error { bad_cast, narrowing };

template <typename T, typename V>
tl::expected<T, cast_error> numeric_cast(V v) {
    if(!std::is_signed_v<T> && std::is_signed_v<V> && v < 0)
        return {};
    using common_type = typename detail::largest_common_type<T, V>::type;
    if(common_type(std::numeric_limits<T>::min()) > common_type(v))
        return tl::make_unexpected(cast_error::narrowing);
    if(common_type(v) > common_type(std::numeric_limits<T>::max()))
        return tl::make_unexpected(cast_error::narrowing);
    return T(v);
}
template <typename T, typename S>
tl::expected<T, cast_error> lexical_cast(S&& s) {
    T t;
    if(!boost::conversion::try_lexical_convert(std::forward<S>(s), t))
        return tl::make_unexpected(cast_error::bad_cast);
    return t;
}
}  // namespace mobsya