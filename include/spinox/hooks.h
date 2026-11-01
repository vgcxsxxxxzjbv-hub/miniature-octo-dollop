#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace spinox::hooks {
enum Mode { MODE_LOG, MODE_SKIP, MODE_RETURN, MODE_ARG };
struct Entry {
    void* target = nullptr;
    void* method = nullptr;
    std::string name;
    Mode mode = MODE_LOG;
    int argIndex = -1;
    int64_t returnVal = 0;
    int64_t argVal = 0;
};
bool installLog(void* method, const char* name);
bool installSkip(void* method, const char* name);
bool installReturn(void* method, const char* name, int64_t val);
bool installArg(void* method, const char* name, int argIndex, int64_t val);
bool updateArg(void* method, int64_t val);
bool uninstall(void* target);
void uninstallAll();
std::vector<Entry> list();
bool isHooked(void* method);
}
