#include "spinox/hooks.h"
#include "spinox/il2cpp.h"
#include "spinox/log.h"
#include <dobby.h>
#include <mutex>
#include <unordered_map>

namespace spinox::hooks {
static std::vector<Entry> g_entries;
static std::mutex g_mu;

struct Ctx {
    Entry entry;
    void* orig = nullptr;
};
static std::unordered_map<void*, Ctx*> g_ctxMap;
static std::mutex g_ctxMu;

static Ctx* findCtx(void* addr) {
    std::lock_guard<std::mutex> lk(g_ctxMu);
    auto it = g_ctxMap.find(addr);
    return it == g_ctxMap.end() ? nullptr : it->second;
}

static void instrument_cb(void* addr, DobbyRegisterContext* ctx) {
    Ctx* c = findCtx(addr);
    if (!c) return;
    Entry& e = c->entry;
    switch (e.mode) {
        case MODE_LOG: {
#if defined(__aarch64__)
            LOGI("[%s] x0=%p x1=%p x2=%p x3=%p", e.name.c_str(),
                (void*)ctx->general.x[0], (void*)ctx->general.x[1],
                (void*)ctx->general.x[2], (void*)ctx->general.x[3]);
#else
            LOGI("[%s] r0=%08x r1=%08x r2=%08x r3=%08x", e.name.c_str(),
                (unsigned)ctx->general.r[0], (unsigned)ctx->general.r[1],
                (unsigned)ctx->general.r[2], (unsigned)ctx->general.r[3]);
#endif
            break;
        }
        case MODE_ARG: {
            int idx = e.argIndex;
            if (idx < 0 || idx > 7) break;
#if defined(__aarch64__)
            ctx->general.x[idx] = (uint64_t)e.argVal;
#else
            ctx->general.r[idx] = (uint32_t)e.argVal;
#endif
            LOGI("[%s] arg%d = %lld", e.name.c_str(), idx, (long long)e.argVal);
            break;
        }
        default: break;
    }
}

using SkipFn = int64_t(*)(void*, void*, void*, void*);
static SkipFn g_origSkips[16];
static int g_skipCount = 0;
static std::mutex g_skipMu;

template <int N>
static int64_t skip_stub(void*, void*, void*, void*) { return 0; }
static SkipFn g_skipStubs[] = {
    skip_stub<0>, skip_stub<1>, skip_stub<2>, skip_stub<3>,
    skip_stub<4>, skip_stub<5>, skip_stub<6>, skip_stub<7>,
    skip_stub<8>, skip_stub<9>, skip_stub<10>, skip_stub<11>,
    skip_stub<12>, skip_stub<13>, skip_stub<14>, skip_stub<15>};

static bool ensureCtx(void* method, const char* name, Mode m,
                      void*& target, Ctx*& ctxOut) {
    target = spinox::il2cpp::methodPtr(method);
    if (!target) return false;
    std::lock_guard<std::mutex> lk(g_ctxMu);
    auto it = g_ctxMap.find(target);
    if (it != g_ctxMap.end()) {
        ctxOut = it->second;
        ctxOut->entry.mode = m;
        ctxOut->entry.name = name ? name : "";
        return true;
    }
    ctxOut = new Ctx();
    ctxOut->entry.target = target;
    ctxOut->entry.method = method;
    ctxOut->entry.name = name ? name : "";
    ctxOut->entry.mode = m;
    g_ctxMap[target] = ctxOut;
    return true;
}

bool installLog(void* method, const char* name) {
    void* target = nullptr; Ctx* ctx = nullptr;
    if (!ensureCtx(method, name, MODE_LOG, target, ctx)) return false;
    std::lock_guard<std::mutex> lk(g_mu);
    for (auto& e : g_entries) if (e.target == target) return true;
    int rc = DobbyInstrument(target, instrument_cb);
    if (rc != RS_SUCCESS) { LOGI("instrument rc=%d", rc); return false; }
    g_entries.push_back(ctx->entry);
    return true;
}

bool installArg(void* method, const char* name, int idx, int64_t val) {
    void* target = nullptr; Ctx* ctx = nullptr;
    if (!ensureCtx(method, name, MODE_ARG, target, ctx)) return false;
    ctx->entry.argIndex = idx;
    ctx->entry.argVal = val;
    std::lock_guard<std::mutex> lk(g_mu);
    for (auto& e : g_entries) if (e.target == target) {
        e.mode = MODE_ARG; e.argIndex = idx; e.argVal = val; return true;
    }
    int rc = DobbyInstrument(target, instrument_cb);
    if (rc != RS_SUCCESS) return false;
    g_entries.push_back(ctx->entry);
    return true;
}

bool installSkip(void* method, const char* name) {
    void* target = spinox::il2cpp::methodPtr(method);
    if (!target) return false;
    std::lock_guard<std::mutex> lk(g_skipMu);
    if (g_skipCount >= 16) { LOGI("skip slots full"); return false; }
    int slot = g_skipCount++;
    int rc = DobbyHook(target, (void*)g_skipStubs[slot], (void**)&g_origSkips[slot]);
    if (rc != RS_SUCCESS) { g_skipCount--; return false; }
    Ctx* ctx = new Ctx();
    ctx->entry.target = target;
    ctx->entry.method = method;
    ctx->entry.name = name ? name : "";
    ctx->entry.mode = MODE_SKIP;
    { std::lock_guard<std::mutex> lk2(g_ctxMu); g_ctxMap[target] = ctx; }
    { std::lock_guard<std::mutex> lk3(g_mu); g_entries.push_back(ctx->entry); }
    return true;
}

bool installReturn(void*, const char*, int64_t) {
    LOGI("installReturn not supported — use code generator");
    return false;
}
bool updateArg(void* method, int64_t val) {
    void* target = spinox::il2cpp::methodPtr(method);
    if (!target) return false;
    std::lock_guard<std::mutex> lk(g_ctxMu);
    auto it = g_ctxMap.find(target);
    if (it == g_ctxMap.end()) return false;
    it->second->entry.argVal = val;
    return true;
}
bool uninstall(void* target) {
    std::lock_guard<std::mutex> lk(g_mu);
    for (size_t i = 0; i < g_entries.size(); i++) {
        if (g_entries[i].target == target) {
            DobbyDestroy(target);
            g_entries.erase(g_entries.begin() + i);
            std::lock_guard<std::mutex> lk2(g_ctxMu);
            auto it = g_ctxMap.find(target);
            if (it != g_ctxMap.end()) { delete it->second; g_ctxMap.erase(it); }
            return true;
        }
    }
    return false;
}
void uninstallAll() {
    std::lock_guard<std::mutex> lk(g_mu);
    for (auto& e : g_entries) DobbyDestroy(e.target);
    g_entries.clear();
    std::lock_guard<std::mutex> lk2(g_ctxMu);
    for (auto& p : g_ctxMap) delete p.second;
    g_ctxMap.clear();
}
std::vector<Entry> list() {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_entries;
}
bool isHooked(void* method) {
    void* target = spinox::il2cpp::methodPtr(method);
    if (!target) return false;
    std::lock_guard<std::mutex> lk(g_mu);
    for (auto& e : g_entries) if (e.target == target) return true;
    return false;
}
}
