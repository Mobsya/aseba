#pragma once
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace mobsya {

#if WIN32
extern std::string get_last_win32_error_string();
#endif

extern std::shared_ptr<spdlog::logger> logger;
extern std::string log_filename(const char* path);
template <typename... Args>
void log(spdlog::level::level_enum level, const char* file, int line, const char* message, Args... args) {
    mobsya::logger->log(
        level, fmt::format("{}@L{}:\t{}", mobsya::log_filename(file), line, fmt::format(message, args...)).c_str());
}

void setLogLevel(spdlog::level::level_enum level);

}  // namespace mobsya
#define _mobsya_CONCAT2(A, B) A##B
#define _mobsya_CONCAT(A, B) _mobsya_CONCAT2(A, B)
#define _mobsya_STR2(x) #x
#define _mobsya_STR(x) _mobsya_STR2(x)

#ifdef _MSC_VER
#    define mLogTrace(...) mobsya::log(spdlog::level::trace, __FILE__, __LINE__, __VA_ARGS__)
#    define mLogDebug(...) mobsya::log(spdlog::level::debug, __FILE__, __LINE__, __VA_ARGS__)
#    define mLogInfo(...) mobsya::log(spdlog::level::info, __FILE__, __LINE__, __VA_ARGS__)
#    define mLogWarn(...) mobsya::log(spdlog::level::warn, __FILE__, __LINE__, __VA_ARGS__)
#    define mLogError(...) mobsya::log(spdlog::level::err, __FILE__, __LINE__, __VA_ARGS__)
#    define mLogCritical(...) mobsya::log(spdlog::level::critical, __FILE__, __LINE__, __VA_ARGS__)
#else
#    define mLogTrace(...) mobsya::log(spdlog::level::trace, __FILE__, __LINE__, ##__VA_ARGS__)
#    define mLogDebug(...) mobsya::log(spdlog::level::debug, __FILE__, __LINE__, ##__VA_ARGS__)
#    define mLogInfo(...) mobsya::log(spdlog::level::info, __FILE__, __LINE__, ##__VA_ARGS__)
#    define mLogWarn(...) mobsya::log(spdlog::level::warn, __FILE__, __LINE__, ##__VA_ARGS__)
#    define mLogError(...) mobsya::log(spdlog::level::err, __FILE__, __LINE__, ##__VA_ARGS__)
#    define mLogCritical(...) mobsya::log(spdlog::level::critical, __FILE__, __LINE__, ##__VA_ARGS__)
#endif
