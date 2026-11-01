#pragma once
#include <android/log.h>
#include <cstdarg>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
namespace spinox::log {
struct Line { std::string text; int level; };
void write(int lvl, const char* fmt, ...);
std::deque<Line> snapshot();
void clear();
}
#define LOGI(fmt, ...) ::spinox::log::write(ANDROID_LOG_INFO,  fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) ::spinox::log::write(ANDROID_LOG_WARN,  fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) ::spinox::log::write(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)
