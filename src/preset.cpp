#include "spinox/preset.h"
#include "spinox/il2cpp.h"
#include "spinox/jni_util.h"
#include "spinox/log.h"
#include <fstream>
#include <sstream>
#include <cstring>
namespace spinox::preset {
static std::vector<FieldMod> g_mods;
static std::string g_dir;
std::string presetDir() {
    if (!g_dir.empty()) return g_dir;
    JNIEnv* e = spinox::jni_util::env();
    if (!e) { g_dir = "/sdcard/spinox"; return g_dir; }
    jobject a = spinox::jni_util::activity();
    if (!a) { g_dir = "/sdcard/spinox"; return g_dir; }
    jclass c = e->GetObjectClass(a);
    jmethodID m = e->GetMethodID(c, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    jobject f = e->CallObjectMethod(a, m, nullptr);
    if (!f) { g_dir = "/sdcard/spinox"; return g_dir; }
    jclass fc = e->GetObjectClass(f);
    jmethodID gp = e->GetMethodID(fc, "getAbsolutePath", "()Ljava/lang/String;");
    jstring s = (jstring)e->CallObjectMethod(f, gp);
    const char* p = e->GetStringUTFChars(s, nullptr);
    g_dir = std::string(p) + "/presets";
    e->ReleaseStringUTFChars(s, p);
    return g_dir;
}
std::vector<FieldMod>& mods() { return g_mods; }
void saveCurrent(const char* label) {
    std::string path = presetDir() + "/" + label + ".spx";
    std::ofstream f(path);
    if (!f) return;
    for (auto& m : g_mods) {
        f << m.classNs << "|" << m.className << "|" << m.fieldName << "|"
          << (m.isStatic ? 1 : 0) << "|" << m.typeName << "|" << m.valueStr << "\n";
    }
}
void loadPreset(const char* path) {
    std::ifstream f(path);
    if (!f) return;
    g_mods.clear();
    std::string line;
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        FieldMod m; std::string s;
        std::getline(ss, m.classNs, '|');
        std::getline(ss, m.className, '|');
        std::getline(ss, m.fieldName, '|');
        std::getline(ss, s, '|'); m.isStatic = s == "1";
        std::getline(ss, m.typeName, '|');
        std::getline(ss, m.valueStr, '|');
        g_mods.push_back(m);
    }
    applyAll();
}
void applyAll() {
    for (auto& m : g_mods) {
        void* k = spinox::il2cpp::getClass(m.classNs.c_str(), m.className.c_str());
        if (!k) continue;
        struct Cap { const char* name; void* out; };
        Cap cap{ m.fieldName.c_str(), nullptr };
        spinox::il2cpp::forEachField(k, [](void* f, void* u) {
            auto* c = (Cap*)u;
            if (!strcmp(spinox::il2cpp::fieldName(f), c->name)) c->out = f;
        }, &cap);
        if (!cap.out) continue;
        uint8_t buf[16] = {0};
        if (m.typeName == "System.Int32") *(int*)buf = std::stoi(m.valueStr);
        else if (m.typeName == "System.Single") *(float*)buf = std::stof(m.valueStr);
        else if (m.typeName == "System.Boolean") buf[0] = (m.valueStr == "true" || m.valueStr == "1") ? 1 : 0;
        else if (m.typeName == "System.Int64") *(int64_t*)buf = std::stoll(m.valueStr);
        else if (m.typeName == "System.UInt32") *(uint32_t*)buf = std::stoul(m.valueStr);
        if (m.isStatic) spinox::il2cpp::fieldStaticSet(cap.out, buf);
    }
}
}
