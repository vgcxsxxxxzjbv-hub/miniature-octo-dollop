#include "spinox/memory.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <cstdio>
namespace spinox::memory {
static bool unprotect(uintptr_t addr, size_t size) {
    size_t ps = sysconf(_SC_PAGESIZE);
    uintptr_t page = addr & ~(ps - 1);
    size_t span = ((addr + size) - page + ps - 1) & ~(ps - 1);
    return mprotect((void*)page, span, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}
bool read(uintptr_t addr, void* out, size_t size) {
    if (!addr || !out || !size) return false;
    return memcpy(out, (const void*)addr, size) != nullptr;
}
bool write(uintptr_t addr, const void* in, size_t size) {
    if (!addr || !in || !size) return false;
    if (!unprotect(addr, size)) return false;
    memcpy((void*)addr, in, size);
    __builtin___clear_cache((char*)addr, (char*)(addr + size));
    return true;
}
std::vector<Region> regions() {
    std::vector<Region> out;
    std::ifstream f("/proc/self/maps");
    if (!f) return out;
    std::string line;
    while (std::getline(f, line)) {
        unsigned long s = 0, e = 0; char perms[5] = {0}, path[256] = {0};
        if (sscanf(line.c_str(), "%lx-%lx %4s %*s %*s %*s %255s", &s, &e, perms, path) < 3) continue;
        int prot = 0;
        if (perms[0] == 'r') prot |= PROT_READ;
        if (perms[1] == 'w') prot |= PROT_WRITE;
        if (perms[2] == 'x') prot |= PROT_EXEC;
        out.push_back({s, e, prot, path});
    }
    return out;
}
uintptr_t moduleBase(const char* name) {
    std::ifstream f("/proc/self/maps");
    std::string line; uintptr_t lo = 0;
    while (std::getline(f, line)) {
        if (line.find(name) == std::string::npos) continue;
        unsigned long s = 0;
        if (sscanf(line.c_str(), "%lx-", &s) == 1) if (!lo || s < lo) lo = s;
    }
    return lo;
}
size_t moduleSize(const char* name) {
    std::ifstream f("/proc/self/maps");
    std::string line; uintptr_t lo = 0, hi = 0;
    while (std::getline(f, line)) {
        if (line.find(name) == std::string::npos) continue;
        unsigned long s = 0, e = 0;
        if (sscanf(line.c_str(), "%lx-%lx", &s, &e) == 2) {
            if (!lo || s < lo) lo = s;
            if (e > hi) hi = e;
        }
    }
    return hi > lo ? (hi - lo) : 0;
}
static int hexNib(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
std::vector<uintptr_t> scanPattern(const char* module, const char* pat) {
    std::vector<uintptr_t> hits;
    std::vector<uint8_t> bytes, mask;
    const char* p = pat;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        if (p[0] == '?') { bytes.push_back(0); mask.push_back(0); p++; if (*p == '?') p++; }
        else {
            int hi = hexNib(p[0]), lo = hexNib(p[1]);
            if (hi < 0 || lo < 0) return hits;
            bytes.push_back((uint8_t)((hi << 4) | lo)); mask.push_back(0xFF); p += 2;
        }
    }
    if (bytes.empty()) return hits;
    uintptr_t base = module ? moduleBase(module) : 0;
    size_t msize = module ? moduleSize(module) : 0;
    for (auto& r : regions()) {
        if (!(r.prot & PROT_READ)) continue;
        if (base && (r.start < base || r.start >= base + msize)) continue;
        const uint8_t* b = (const uint8_t*)r.start;
        size_t len = r.end - r.start, plen = bytes.size();
        if (len < plen) continue;
        for (size_t i = 0; i <= len - plen; i++) {
            bool ok = true;
            for (size_t j = 0; j < plen; j++)
                if (mask[j] && b[i + j] != bytes[j]) { ok = false; break; }
            if (ok) { hits.push_back(r.start + i); if (hits.size() >= 128) return hits; }
        }
    }
    return hits;
}
}
