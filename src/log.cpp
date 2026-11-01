#include "spinox/log.h"
namespace spinox::log {
static std::deque<Line> g_lines;
static std::mutex g_mu;
void write(int lvl, const char* fmt, ...) {
    char buf[1024];
    va_list a; va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    __android_log_print(lvl, "IL2CPPED", "%s", buf);
    std::lock_guard<std::mutex> lk(g_mu);
    g_lines.push_back({buf, lvl});
    if (g_lines.size() > 200) g_lines.pop_front();
}
std::deque<Line> snapshot() {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_lines;
}
void clear() {
    std::lock_guard<std::mutex> lk(g_mu);
    g_lines.clear();
}
}
