#include "log.h"
#include <boost/utility/string_view.hpp>
#include <spdlog/fmt/fmt.h>

std::shared_ptr<spdlog::logger> mobsya::log = []() {
    auto log = spdlog::stdout_color_mt("console");
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::trace);
    return log;
}();

std::string mobsya::log_filename(const char* path) {
    auto sw = boost::string_view(path);
    sw = sw.substr(sw.find_last_of("/\\") + 1);
    return std::string(sw.data(), sw.size());
}
