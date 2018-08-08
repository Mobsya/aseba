#include "log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/utility/string_view.hpp>
#include <fmt/format.h>

std::shared_ptr<spdlog::logger> mobsya::logger = []() {
    auto log = spdlog::stdout_color_mt("console");
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::trace);
    return log;
}();

std::string mobsya::log_filename(const char* path) {
    auto sw = boost::string_view(path);
    sw = sw.substr(sw.find_last_of("/\\") + 1);
    sw = sw.substr(0, std::min(sw.size(), std::size_t(20)));
    auto str = std::string(sw.data(), sw.size());
    str.insert(str.begin(), 20 - str.size(), ' ');
    return str;
}
