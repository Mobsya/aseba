#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/ostream.h>

namespace mobsya {
extern std::shared_ptr<spdlog::logger> log;
}
#define _mobsya_CONCAT2(A, B) A##B
#define _mobsya_CONCAT(A, B) _mobsya_CONCAT2(A, B)
#define _mobsya_STR2(x) #x
#define _mobsya_STR(x) _mobsya_STR2(x)

#define _mobsya_Log(level, ...) mobsya::log->log(level, __FILE__ "@L" _mobsya_STR(__LINE__) ":\t" __VA_ARGS__)
#define mLogTrace(...) _mobsya_Log(spdlog::level::trace, __VA_ARGS__)
#define mLogDebug(...) _mobsya_Log(spdlog::level::debug, __VA_ARGS__)
#define mLogInfo(...) _mobsya_Log(spdlog::level::info, __VA_ARGS__)
#define mLogWarn(...) _mobsya_Log(spdlog::level::warn, __VA_ARGS__)
#define mLogError(...) _mobsya_Log(spdlog::level::err, __VA_ARGS__)
#define mLogCritical(...) _mobsya_Log(spdlog::level::critical, __VA_ARGS__)
