#pragma once
#include <vector>
#include <string>
#include <map>
#ifndef WIN32
#    include "variant.hpp"
#else
#    include <variant>
#endif
#include <iostream>

namespace mobsya {

namespace detail {

    template <typename char_type>
    struct types {
        using bool_type = bool;
        using integral_type = long;
        using floating_type = double;
        using string_type = std::basic_string<char_type>;
        using key_type = string_type;
        template <typename... Args>
        using array_type = std::vector<Args...>;
        template <typename... Args>
        using object_type = std::map<Args...>;
    };

    template <class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };
    template <class... Ts>
    overloaded(Ts...)->overloaded<Ts...>;


    template <typename T, typename types, typename array_type, typename object_type>
    std::enable_if_t<std::is_integral_v<std::decay_t<T>>, typename types::integral_type> to_compatible_value(T&& t) {
        static_assert(sizeof(typename types::integral_type) >= sizeof(T));
        return t;
    }

    template <typename T, typename types, typename array_type, typename object_type>
    std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>, typename types::floating_type>
    to_compatible_value(T&& t) {
        static_assert(sizeof(typename types::floating_type) >= sizeof(T));
        return std::forward<T>(t);
    }

    template <typename T, typename types, typename array_type, typename object_type>
    std::enable_if_t<std::is_convertible_v<std::decay_t<T>, typename types::string_type>, typename types::string_type>
    to_compatible_value(T&& t) {
        return std::forward<T>(t);
    }

    template <typename T, typename types, typename array_type, typename object_type>
    std::enable_if_t<std::is_convertible_v<std::decay_t<T>, array_type>, array_type> to_compatible_value(T&& t) {
        return std::forward<T>(t);
    }


    template <typename T, typename types, typename array_type, typename object_type>
    std::enable_if_t<std::is_convertible_v<std::decay_t<T>, object_type>, object_type> to_compatible_value(T&& t) {
        return std::forward<T>(t);
    }

    template <typename T, typename _ = void>
    struct entity_size {
        static std::size_t get(T&&) {
            return 0;
        }
    };

    template <typename T>
    struct entity_size<
        T, std::enable_if_t<std::is_same_v<typename std::decay_t<T>::size_type, decltype(std::decay_t<T>().size())>>> {
        static std::size_t get(T&& t) {
            return t.size();
        }
    };


}  // namespace detail

template <typename types>
class basic_property {
public:
    using this_t = basic_property<types>;
    using bool_t = typename types::bool_type;
    using integral_t = typename types::integral_type;
    using floating_t = typename types::floating_type;
    using string_t = typename types::string_type;
    using key_t = typename types::key_type;
    using array_t = typename types::template array_type<this_t>;
    using object_t = typename types::template object_type<key_t, this_t>;


    static auto constexpr null = std::monostate{};

    using value_t = std::variant<std::monostate, bool_t, integral_t, floating_t, string_t, array_t, object_t>;
    template <typename T>
    friend class is_compatible;

    static constexpr auto max_scalar_type_index = 3;

    struct dict;
    struct list;

    template <typename T>
    struct is_convertible {
        static constexpr bool value = (std::is_convertible_v<T, bool_t> || std::is_convertible_v<T, floating_t> ||
                                       std::is_convertible_v<T, string_t> || std::is_convertible_v<T, array_t> ||
                                       std::is_convertible_v<T, object_t>)

            &&!std::is_same_v<std::decay_t<T>, std::decay_t<this_t>>;
    };
    template <typename T>
    static constexpr bool is_convertible_v = is_convertible<T>::value;


public:
    basic_property(const this_t& other) = default;
    basic_property(this_t&& other) = default;

    template <typename T, typename _ = std::enable_if_t<is_convertible_v<T>>>
    basic_property(T&& t) : value(detail::to_compatible_value<T, types, array_t, object_t>(std::forward<T>(t))) {}

    basic_property() {}

    basic_property(const std::monostate&) {}
    basic_property(std::monostate&&) {}


    template <
        typename T,
        typename _ = std::enable_if_t<is_convertible_v<T> || std::is_same_v<std::decay_t<T>, std::decay_t<this_t>>>>
    basic_property(std::pair<key_t, T>&&) {}

    template <
        typename T,
        typename _ = std::enable_if_t<is_convertible_v<T> || std::is_same_v<std::decay_t<T>, std::decay_t<this_t>>>>
    basic_property(key_t&& k, T&& t) {
        value = object_t{std::forward_as_tuple(k, t)};
    }

    basic_property(std::initializer_list<this_t> args) {
        int x = args.begin()->value.index();
        int s = args.begin()->size();
        // assert(x + s);
        if(args.size() == 2) {
            if(args.begin()->is_string()) {
                object_t object;
                object.emplace(std::pair<key_t, this_t>{*args.begin(), *(args.begin() + 1)});
                value = std::move(object);
                return;
            }
        }

        for(auto&& arg : args) {
            if(arg.is_array() || arg.is_string() || arg.size() != 2) {
                value = std::move(args);
                return;
            }
        }
        // map
        object_t object;
        for(auto&& arg : args) {
            auto s = key_t(arg[0]);
            object.insert_or_assign(key_t(arg[0]), arg[1]);
        }
        value = std::move(object);
    }
    template <typename T, std::enable_if_t<is_convertible_v<T>, int> = 0>
    basic_property& operator=(T&& t) {
        value = detail::to_compatible_value<T, types, array_t, object_t>(std::forward<T>(t));
        return *this;
    }

    basic_property& operator=(const this_t& other) = default;
    basic_property& operator=(this_t&& other) = default;

    bool is_null() const noexcept {
        return value.valueless_by_exception() || std::holds_alternative<std::monostate>(value);
    }
    bool is_boolean() const noexcept {
        return std::holds_alternative<bool_t>(value);
    }
    bool is_integral() const noexcept {
        return std::holds_alternative<integral_t>(value);
    }
    bool is_double() const noexcept {
        return std::holds_alternative<floating_t>(value);
    }
    bool is_number() const noexcept {
        return is_double() || is_integral();
    }

    bool is_string() const noexcept {
        return std::holds_alternative<string_t>(value);
    }
    bool is_array() const noexcept {
        return std::holds_alternative<array_t>(value);
    }
    bool is_object() const noexcept {
        return std::holds_alternative<object_t>(value);
    }

    bool is_empty() {
        return is_null() || (is_array() && std::get<array_t>(value).empty()) ||
            (is_object() && std::get<object_t>(value).empty());
    }

    std::size_t size() const {

        return std::visit(
            [](auto&& e) {
                // static_assert (std::is_same_v<decltype (e)::size_t, std::size_t>);
                return detail::entity_size<decltype(e)>::get(e);
            },
            value);
    }

    const basic_property<types>& operator[](typename array_t::size_type idx) const {
        return std::get<array_t>(value)[idx];
    }

    basic_property<types>& operator[](typename array_t::size_type idx) {
        if(!is_array())
            value = array_t();
        return std::get<array_t>(value)[idx];
    }

    const basic_property<types>& operator[](const key_t& key) const {
        return std::get<object_t>(value)[key];
    }
    basic_property<types>& operator[](const key_t& key) {
        if(!is_object())
            value = object_t();
        return std::get<object_t>(value)[key];
    }

    explicit operator bool_t() const {
        const bool null = is_null();
        if(!null && is_boolean())
            return std::get<bool_t>(value);
        return !null;
    }
    template <typename type,
              std::enable_if_t<std::is_floating_point_v<type> && std::is_convertible_v<type, floating_t>, int> = 0>
    explicit operator type() const {
        if(is_double())
            return std::get<floating_t>(value);
        return std::get<integral_t>(value);
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T> && std::is_convertible_v<T, integral_t>, int> = 0>
    explicit operator T() const {
        if(is_double())
            return std::get<floating_t>(value);
        return std::get<integral_t>(value);
    }


    template <typename T,
              std::enable_if_t<std::is_floating_point_v<T> && std::is_convertible_v<T, floating_t>, int> = 0>
    bool operator==(const T& t) const {
        if(is_double())
            return std::get<floating_t>(value) == t;
        return std::get<integral_t>(value) == t;
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T> && std::is_convertible_v<T, integral_t>, int> = 0>
    bool operator==(const T& t) const {
        if(is_integral())
            return std::get<integral_t>(value) == t;
        return std::get<floating_t>(value) == t;
    }

    bool operator==(const this_t& t) const {
        const auto oi = t.value.index();
        const auto i = value.index();

        if(oi != i && (i > max_scalar_type_index || oi > max_scalar_type_index))
            return false;

        if(oi == i) {  // same type
            if(is_null())
                return true;
            if(is_number() || is_boolean() || is_string()) {
                return t.value == value;
            }
            // Todo compare array
            return false;

        } else if((is_number() || is_boolean()) && (t.is_number() || t.is_boolean())) {
            return t.value == value;
        }
        return false;
    }

    bool operator!=(const this_t& t) const {
        return !(*this == t);
    }


    template <typename T,
              std::enable_if_t<std::is_same_v<T, array_t> || std::is_same_v<T, object_t> || std::is_same_v<T, string_t>,
                               int> = 0>
    explicit operator T const&() const {
        return std::get<T>(value);
    }

    template <typename T, std::enable_if_t<std::is_same_v<T, array_t> || std::is_same_v<T, object_t>>>
    explicit operator T&() {
        if(!std::holds_alternative<object_t>(value))
            value = T();
        return value;
    }

public:
    basic_property(dict&& d) : value(std::move(d.obj)) {}
    basic_property(list&& d) : value(std::move(d.array)) {}


    value_t value = std::monostate{};
    struct dict {
        dict() = default;
        dict(const dict& other) = delete;
        dict(dict&& other) = default;
        template <typename... Args>
        dict(Args&&... args) {
            static_assert((sizeof...(Args) % 2) == 0, "dict takes a sequence of key, value");
            do_construct(std::forward<Args>(args)...);
        }

    private:
        void do_construct() {}
        template <typename K, typename T>
        void do_construct(K&& k, T&& t) {
            obj.insert_or_assign(std::forward<K>(k), std::forward<T>(t));
        }

        template <typename K, typename T, typename... Args>
        void do_construct(K&& k, T&& t, Args&&... args) {
            do_construct(std::forward<K>(k), std::forward<T>(t));
            do_construct(std::forward<Args>(args)...);
        }
        friend class basic_property;

        object_t obj;
    };
    struct list {
        list() = default;
        list(const list& other) = delete;
        list(list&& list) = default;
        template <typename... Args>
        list(Args&&... args) {
            array.reserve(sizeof...(Args));
            array.emplace_back(args...);
        }
        template <typename Rng>
        static list from_range(Rng&& rng) {
            list l;
            for(auto&& v : rng) {
                l.array.emplace_back(v);
            }
            return l;
        }

    private:
        friend class basic_property;
        array_t array;
    };

public:
    template <typename Rng>
    static auto list_from_range(Rng&& rng) {
        return basic_property(list::from_range(std::forward<Rng>(rng)));
    }
};

using property = basic_property<detail::types<char>>;

}  // namespace mobsya


template <typename types>
std::ostream& operator<<(std::ostream& os, const mobsya::basic_property<types>& p) {
    using namespace mobsya;
    std::visit(
        detail::overloaded{[&os](const std::monostate&) { os << "null"; },
                           [&os](const typename basic_property<types>::bool_t& e) { os << (e ? "true" : "false"); },
                           [&os](const typename basic_property<types>::integral_t& e) { os << e; },
                           [&os](const typename basic_property<types>::floating_t& e) { os << e; },
                           [&os](const typename basic_property<types>::string_t& e) { os << '"' << e << '"'; },
                           [&os](const typename basic_property<types>::array_t& e) {
                               os << "[";
                               for(auto it = std::begin(e); !e.empty();) {
                                   os << *it;
                                   if(++it == std::end(e))
                                       break;
                                   os << ", ";
                               }
                               os << "]";
                           },
                           [&os](const typename basic_property<types>::object_t& e) {
                               os << "{";
                               for(auto it = std::begin(e); !e.empty();) {
                                   os << '"' << it->first << '"' << " : " << it->second;
                                   if(++it == std::end(e))
                                       break;
                                   os << ", ";
                               }
                               os << "}";
                           }

        },
        p.value);
    return os;
}