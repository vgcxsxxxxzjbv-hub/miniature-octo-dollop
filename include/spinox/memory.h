#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
namespace spinox::memory {
bool read(uintptr_t addr, void* out, size_t size);
bool write(uintptr_t addr, const void* in, size_t size);
template <typename T> T read(uintptr_t a, T def = T{}) {
    T v{}; return read(a, &v, sizeof(T)) ? v : def;
}
template <typename T> bool write(uintptr_t a, T v) {
    return write(a, &v, sizeof(T));
}
struct Region { uintptr_t start, end; int prot; std::string name; };
std::vector<Region> regions();
uintptr_t moduleBase(const char* name);
size_t moduleSize(const char* name);
std::vector<uintptr_t> scanPattern(const char* module, const char* pattern);
}
