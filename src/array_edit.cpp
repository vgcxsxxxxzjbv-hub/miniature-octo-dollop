#include "spinox/array_edit.h"
#include "spinox/il2cpp.h"
#include <cstring>
namespace spinox::array_edit {
bool isArray(void* klass) {
    if (!klass) return false;
    const char* n = spinox::il2cpp::className(klass);
    return n && strstr(n, "[]") != nullptr;
}
bool isList(void* klass) {
    if (!klass) return false;
    const char* n = spinox::il2cpp::className(klass);
    if (!n) return false;
    return strstr(n, "List") || strstr(n, "IList");
}
int length(void* obj) {
    if (!obj) return 0;
#if defined(__aarch64__)
    uint32_t len = *(uint32_t*)((char*)obj + 24);
#else
    uint32_t len = *(uint32_t*)((char*)obj + 12);
#endif
    return len > 100000 ? 0 : (int)len;
}
void* at(void* obj, int i) {
    if (!obj || i < 0) return nullptr;
#if defined(__aarch64__)
    size_t base = 32;
#else
    size_t base = 16;
#endif
    uint8_t* elem = (uint8_t*)obj + base + (size_t)i * sizeof(void*);
    return *(void**)elem;
}
std::vector<Item> readAll(void* obj) {
    std::vector<Item> out;
    int n = length(obj);
    for (int i = 0; i < n && i < 200; i++) {
        Item it; it.ptr = at(obj, i);
        char buf[32]; snprintf(buf, sizeof(buf), "#%d %p", i, it.ptr);
        it.preview = buf;
        out.push_back(it);
    }
    return out;
}
std::string elementTypeName(void* klass) {
    const char* n = spinox::il2cpp::className(klass);
    if (!n) return "?";
    std::string s = n;
    auto p = s.find("[]");
    if (p != std::string::npos) s = s.substr(0, p);
    return s;
}
size_t elementSize(void*) { return sizeof(void*); }
}
