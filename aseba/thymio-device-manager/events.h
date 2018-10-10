#pragma once
#include <string>

namespace mobsya {

enum class event_type { aseba };
struct event {
    std::string name;
    event_type type;
    std::size_t size;
    event(std::string name, std::size_t s) : name(std::move(name)), type(event_type::aseba), size(s) {}
};

}  // namespace mobsya