#include "log.h"

std::shared_ptr<spdlog::logger> mobsya::log = []() {
    auto log = spdlog::stdout_color_mt("console");
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::trace);
    return log;
}();
