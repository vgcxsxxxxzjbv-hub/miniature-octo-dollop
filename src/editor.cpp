#include "spinox/editor.h"
#include "spinox/il2cpp.h"
#include "spinox/hooks.h"
#include "spinox/log.h"
#include "spinox/memory.h"
#include "spinox/array_edit.h"
#include "spinox/codegen.h"
#include "spinox/preset.h"
#include "spinox/icons.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <algorithm>
namespace spinox::editor {
enum View { V_LIST, V_CLASS, V_INSTANCE, V_METHOD };
enum Tab  { T_CLASSES, T_HOOKS, T_ARRAYS, T_AOB, T_PRESET, T_LOG, T_COUNT };
static Tab g_tab = T_CLASSES;
static View g_view = V_LIST;
static std::vector<std::string> g_classNames;
static std::vector<void*> g_classPtrs;
static bool g_loaded = false;
static char g_filter[64] = "";
static void* g_class = nullptr;
static void* g_instance = nullptr;
static void* g_method = nullptr;
static std::vector<void*> g_instances;
static std::vector<std::pair<View, void*>> g_history;
static void push() { g_history.push_back({g_view, g_class}); }
static bool back() {
    if (g_history.empty()) return false;
    auto p = g_history.back(); g_history.pop_back();
    g_view = p.first; g_class = p.second; return true;
}
static void loadClasses() {
    if (g_loaded) return;
    g_classNames.clear(); g_classPtrs.clear();
    spinox::il2cpp::forEachClass([](void* k, void*) {
        std::string ns = spinox::il2cpp::classNamespace(k);
        std::string nm = spinox::il2cpp::className(k);
        if (nm.empty()) return;
        std::string full = ns.empty() ? nm : (ns + "." + nm);
        g_classNames.push_back(std::move(full));
        g_classPtrs.push_back(k);
    }, nullptr);
    std::vector<size_t> order(g_classNames.size());
    for (size_t i = 0; i < order.size(); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [](size_t a, size_t b) {
        return g_classNames[a] < g_classNames[b];
    });
    std::vector<std::string> nn(order.size());
    std::vector<void*> np(order.size());
    for (size_t i = 0; i < order.size(); i++) {
        nn[i] = g_classNames[order[i]]; np[i] = g_classPtrs[order[i]];
    }
    g_classNames.swap(nn); g_classPtrs.swap(np);
    g_loaded = true;
}
static size_t primSize(const char* tn) {
    if (!tn) return sizeof(void*);
    if (!strcmp(tn, "System.Boolean") || !strcmp(tn, "System.Byte") || !strcmp(tn, "System.SByte")) return 1;
    if (!strcmp(tn, "System.Int16") || !strcmp(tn, "System.UInt16") || !strcmp(tn, "System.Char")) return 2;
    if (!strcmp(tn, "System.Int32") || !strcmp(tn, "System.UInt32") || !strcmp(tn, "System.Single")) return 4;
    if (!strcmp(tn, "System.Int64") || !strcmp(tn, "System.UInt64") || !strcmp(tn, "System.Double")) return 8;
    return sizeof(void*);
}
static bool isStr(const char* tn) { return tn && !strcmp(tn, "System.String"); }
static std::string shortPtr(void* p) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%08x", (unsigned)((uintptr_t)p & 0xFFFFFFFF));
    return buf;
}
static const char* iconForType(const char* tname) {
    if (!tname) return ICON_PUZZLE;
    if (!strcmp(tname, "System.Single") || !strcmp(tname, "System.Double")) return ICON_SLIDERS;
    if (!strcmp(tname, "System.Int32") || !strcmp(tname, "System.Int64") ||
        !strcmp(tname, "System.UInt32") || !strcmp(tname, "System.UInt16") ||
        !strcmp(tname, "System.Int16") || !strcmp(tname, "System.Byte") ||
        !strcmp(tname, "System.SByte") || !strcmp(tname, "System.UInt64")) return ICON_COG;
    if (!strcmp(tname, "System.Boolean")) return ICON_CHECK;
    if (isStr(tname)) return ICON_EDIT;
    if (strstr(tname, "[]")) return ICON_DATABASE;
    if (strstr(tname, "List")) return ICON_LIST;
    if (strstr(tname, "Vector")) return ICON_CUBES;
    return ICON_PUZZLE;
}
static void fieldWidget(void* obj, void* field) {
    void* ft = spinox::il2cpp::fieldType(field);
    char* tn = spinox::il2cpp::typeNameAlloc(ft);
    const char* tname = tn ? tn : "?";
    size_t sz = primSize(tname);
    bool isStat = spinox::il2cpp::fieldIsStatic(field);
    uint8_t buf[16] = {0};
    if (sz > sizeof(buf)) sz = sizeof(buf);
    if (isStat) spinox::il2cpp::fieldStaticGet(field, buf);
    else        spinox::il2cpp::fieldGet(obj, field, buf);
    ImGui::PushID(field);
    ImGui::TextUnformatted(iconForType(tname));
    ImGui::SameLine();
    ImGui::TextUnformatted(spinox::il2cpp::fieldName(field));
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", tname);
    bool changed = false;
    if (!strcmp(tname, "System.Single")) {
        float v = *(float*)buf;
        if (ImGui::DragFloat("##v", &v, 0.1f)) { *(float*)buf = v; changed = true; }
    } else if (!strcmp(tname, "System.Int32")) {
        int v = *(int*)buf;
        if (ImGui::DragInt("##v", &v)) { *(int*)buf = v; changed = true; }
    } else if (!strcmp(tname, "System.UInt32")) {
        unsigned v = *(unsigned*)buf;
        if (ImGui::DragScalar("##v", ImGuiDataType_U32, &v)) { *(unsigned*)buf = v; changed = true; }
    } else if (!strcmp(tname, "System.Int64")) {
        long long v = *(long long*)buf;
        if (ImGui::DragScalar("##v", ImGuiDataType_S64, &v)) { *(long long*)buf = v; changed = true; }
    } else if (!strcmp(tname, "System.Boolean")) {
        bool v = buf[0] != 0;
        if (ImGui::Checkbox("##v", &v)) { buf[0] = v ? 1 : 0; changed = true; }
    } else if (isStr(tname)) {
        void* s = *(void**)buf;
        std::string cur = spinox::il2cpp::readIl2cppString(s);
        static char editBuf[256];
        snprintf(editBuf, sizeof(editBuf), "%s", cur.c_str());
        if (ImGui::InputText("##str", editBuf, sizeof(editBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            void* newStr = spinox::il2cpp::makeIl2cppString(editBuf);
            if (newStr) { *(void**)buf = newStr; changed = true; }
        }
    } else {
        void* ptr = *(void**)buf;
        ImGui::Text("%p", ptr);
        ImGui::SameLine();
        if (ptr && ImGui::SmallButton(ICON_ARROW_RIGHT " Open")) {
            void* k = spinox::il2cpp::objectGetClass(ptr);
            if (k) {
                push(); g_class = k; g_instance = ptr;
                g_instances = { ptr }; g_view = V_INSTANCE;
                ImGui::PopID();
                spinox::il2cpp::freeString(tn);
                return;
            }
        }
        void* klass = spinox::il2cpp::objectGetClass(ptr);
        if (klass && spinox::array_edit::isArray(klass)) {
            if (ImGui::TreeNode("##arr", ICON_DATABASE " Array[%d]", spinox::array_edit::length(ptr))) {
                int n = spinox::array_edit::length(ptr);
                for (int i = 0; i < n && i < 50; i++) {
                    void* item = spinox::array_edit::at(ptr, i);
                    ImGui::Text("  [%d] %p", i, item);
                }
                ImGui::TreePop();
            }
        }
    }
    if (changed) {
        if (isStat) spinox::il2cpp::fieldStaticSet(field, buf);
        else        spinox::il2cpp::fieldSet(obj, field, buf);
    }
    ImGui::PopID();
    spinox::il2cpp::freeString(tn);
}
static void screenList() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.14f, 0.25f, 1));
    ImGui::TextUnformatted(ICON_SEARCH);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##f", "search class...", g_filter, sizeof(g_filter));
    ImGui::TextDisabled(ICON_CUBES " %zu classes", g_classNames.size());
    ImGui::Separator();
    ImGui::BeginChild("##list");
    int shown = 0;
    std::string f(g_filter);
    for (auto& c : f) c = tolower(c);
    for (size_t i = 0; i < g_classNames.size() && shown < 300; i++) {
        if (!f.empty()) {
            std::string lc = g_classNames[i];
            for (auto& ch : lc) ch = tolower(ch);
            if (lc.find(f) == std::string::npos) continue;
        }
        shown++;
        std::string label = std::string(ICON_CUBES "  ") + g_classNames[i];
        if (ImGui::Selectable(label.c_str())) {
            push(); g_class = g_classPtrs[i];
            g_view = V_CLASS; g_instances.clear();
        }
    }
    if (shown == 0) ImGui::TextDisabled(ICON_INFO "  no matches");
    ImGui::EndChild();
}
static void screenClass() {
    if (!g_class) { g_view = V_LIST; return; }
    if (ImGui::Button(ICON_ARROW_LEFT " Back")) { if (!back()) g_view = V_LIST; return; }
    ImGui::SameLine();
    std::string title = spinox::il2cpp::classNamespace(g_class);
    if (!title.empty()) title += ".";
    title += spinox::il2cpp::className(g_class);
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_CUBES " %s", title.c_str());
    ImGui::Separator();
    if (ImGui::Button(ICON_SEARCH " Find Instances")) {
        g_instances.clear();
        spinox::il2cpp::forEachInstance(g_class, [](void* o, void*) {
            g_instances.push_back(o);
        }, nullptr);
        if (!g_instances.empty()) { push(); g_instance = g_instances[0]; g_view = V_INSTANCE; }
    }
    if (!g_instances.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled(ICON_DOT " (%zu)", g_instances.size());
    }
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_HEART " Static Fields");
    ImGui::BeginChild("##sf", ImVec2(0, 200), true);
    spinox::il2cpp::forEachField(g_class, [](void* f, void*) {
        if (spinox::il2cpp::fieldIsStatic(f)) fieldWidget(nullptr, f);
    }, nullptr);
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_CODE " Methods");
    ImGui::BeginChild("##mt");
    spinox::il2cpp::forEachMethod(g_class, [](void* m, void*) {
        char label[200];
        snprintf(label, sizeof(label), "%s %s / %d  RVA:0x%x",
            ICON_BOLT, spinox::il2cpp::methodName(m),
            spinox::il2cpp::methodArgc(m), spinox::il2cpp::methodRva(m));
        if (ImGui::Selectable(label)) {
            g_method = m; push(); g_view = V_METHOD;
        }
    }, nullptr);
    ImGui::EndChild();
}
static void screenInstance() {
    if (!g_class || !g_instance) { g_view = V_CLASS; return; }
    if (ImGui::Button(ICON_ARROW_LEFT " Back")) { if (!back()) g_view = V_CLASS; return; }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1),
        ICON_PUZZLE " instance %s", shortPtr(g_instance).c_str());
    ImGui::Separator();
    if (g_instances.size() > 1) {
        int cur = 0;
        for (size_t i = 0; i < g_instances.size(); i++)
            if (g_instances[i] == g_instance) cur = (int)i;
        char combo[64];
        snprintf(combo, sizeof(combo), "#%d / %zu", cur + 1, g_instances.size());
        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("##sel", combo)) {
            for (size_t i = 0; i < g_instances.size(); i++) {
                char lbl[48];
                snprintf(lbl, sizeof(lbl), "#%zu %s", i + 1, shortPtr(g_instances[i]).c_str());
                if (ImGui::Selectable(lbl, (int)i == cur)) g_instance = g_instances[i];
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();
    }
    ImGui::BeginChild("##fs");
    spinox::il2cpp::forEachField(g_class, [](void* f, void*) {
        if (!spinox::il2cpp::fieldIsStatic(f)) fieldWidget(g_instance, f);
    }, nullptr);
    ImGui::EndChild();
}
static void screenMethod() {
    if (!g_method) { g_view = V_CLASS; return; }
    if (ImGui::Button(ICON_ARROW_LEFT " Back")) { if (!back()) g_view = V_CLASS; return; }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1),
        ICON_CODE " %s", spinox::il2cpp::methodName(g_method));
    ImGui::Separator();
    int argc = spinox::il2cpp::methodArgc(g_method);
    ImGui::TextDisabled("Args: %d   RVA: 0x%x   Ptr: %p", argc,
        spinox::il2cpp::methodRva(g_method), spinox::il2cpp::methodPtr(g_method));
    bool hooked = spinox::hooks::isHooked(g_method);
    if (hooked) {
        if (ImGui::Button(ICON_TIMES " Unhook"))
            spinox::hooks::uninstall(spinox::il2cpp::methodPtr(g_method));
    } else {
        if (ImGui::Button(ICON_TERMINAL " Log Hook"))
            spinox::hooks::installLog(g_method, spinox::il2cpp::methodName(g_method));
        ImGui::SameLine();
        if (ImGui::Button(ICON_SKULL " Skip Hook"))
            spinox::hooks::installSkip(g_method, spinox::il2cpp::methodName(g_method));
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_CODE " Gen Log")) {
        auto c = spinox::codegen::generateLogHook(g_method);
        ImGui::SetClipboardText(c.c_str());
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_MAGIC " Gen Return")) {
        auto c = spinox::codegen::generateReturnHook(g_method, "9999");
        ImGui::SetClipboardText(c.c_str());
    }
    if (argc > 0) {
        static int argIdx = 0, argVal = 0;
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_SLIDERS " Arg Override");
        ImGui::SetNextItemWidth(80); ImGui::InputInt("arg#", &argIdx);
        ImGui::SetNextItemWidth(120); ImGui::InputInt("value", &argVal);
        ImGui::SameLine();
        if (ImGui::Button(ICON_CROSSHAIRS " Install"))
            spinox::hooks::installArg(g_method, spinox::il2cpp::methodName(g_method), argIdx, argVal);
    }
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_BOLT " Invoke");
    static int iargs[8] = {0};
    static float fargs[8] = {0};
    static bool bargs[8] = {false};
    static char sargs[8][128] = {{0}};
    void* argv[8] = {0};
    for (int i = 0; i < argc && i < 8; i++) {
        char* pt = spinox::il2cpp::typeNameAlloc(spinox::il2cpp::methodParamType(g_method, i));
        const char* tname = pt ? pt : "?";
        ImGui::PushID(i);
        if (!strcmp(tname, "System.Int32") || !strcmp(tname, "System.UInt32")) {
            ImGui::InputInt(tname, &iargs[i]); argv[i] = &iargs[i];
        } else if (!strcmp(tname, "System.Single")) {
            ImGui::InputFloat(tname, &fargs[i]); argv[i] = &fargs[i];
        } else if (!strcmp(tname, "System.Boolean")) {
            ImGui::Checkbox(tname, &bargs[i]); argv[i] = &bargs[i];
        } else if (!strcmp(tname, "System.String")) {
            ImGui::InputText(tname, sargs[i], sizeof(sargs[i]));
        }
        ImGui::PopID();
        spinox::il2cpp::freeString(pt);
    }
    auto prep = [&]() {
        for (int i = 0; i < argc && i < 8; i++) {
            char* pt = spinox::il2cpp::typeNameAlloc(spinox::il2cpp::methodParamType(g_method, i));
            if (pt && !strcmp(pt, "System.String")) argv[i] = spinox::il2cpp::makeIl2cppString(sargs[i]);
            spinox::il2cpp::freeString(pt);
        }
    };
    if (ImGui::Button(ICON_PLAY " Invoke")) {
        prep();
        void* exc = nullptr;
        void* ret = spinox::il2cpp::runtimeInvoke(g_method, g_instance, argv, &exc);
        if (exc) LOGI("threw"); else LOGI("ret=%p", ret);
    }
}
static void tabHooks() {
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_BOLT " Active Hooks");
    if (ImGui::Button(ICON_TRASH " Unhook All")) spinox::hooks::uninstallAll();
    ImGui::Separator();
    auto list = spinox::hooks::list();
    if (list.empty()) { ImGui::TextDisabled(ICON_INFO "  none"); return; }
    for (size_t i = 0; i < list.size(); i++) {
        const char* modeStr = "LOG"; const char* modeIcon = ICON_TERMINAL;
        if (list[i].mode == spinox::hooks::MODE_SKIP) { modeStr = "SKIP"; modeIcon = ICON_SKULL; }
        else if (list[i].mode == spinox::hooks::MODE_ARG) { modeStr = "ARG"; modeIcon = ICON_CROSSHAIRS; }
        else if (list[i].mode == spinox::hooks::MODE_RETURN) { modeStr = "RET"; modeIcon = ICON_MAGIC; }
        ImGui::PushID((int)i);
        ImGui::Text("%s [%s] %s", modeIcon, modeStr, list[i].name.c_str());
        ImGui::SameLine(); ImGui::TextDisabled("(%p)", list[i].target);
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_TIMES " Unhook")) spinox::hooks::uninstall(list[i].target);
        ImGui::PopID();
    }
}
static void tabArrays() {
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_DATABASE " Array Editor");
    ImGui::TextDisabled(ICON_INFO "  open a class with array/list fields");
    ImGui::Separator();
    if (!g_instance || !g_class) { ImGui::TextDisabled("no instance selected"); return; }
    spinox::il2cpp::forEachField(g_class, [](void* f, void*) {
        char* tn = spinox::il2cpp::typeNameAlloc(spinox::il2cpp::fieldType(f));
        const char* tname = tn ? tn : "?";
        if (strstr(tname, "[]") || strstr(tname, "List")) {
            uint8_t buf[8] = {0};
            if (spinox::il2cpp::fieldIsStatic(f)) spinox::il2cpp::fieldStaticGet(f, buf);
            else                                 spinox::il2cpp::fieldGet(g_instance, f, buf);
            void* obj = *(void**)buf;
            ImGui::Text(ICON_LIST " %s:", spinox::il2cpp::fieldName(f));
            ImGui::SameLine(); ImGui::TextDisabled("(%s)", tname);
            if (obj) {
                int n = spinox::array_edit::length(obj);
                ImGui::TextDisabled("  length=%d", n);
                ImGui::Indent(12);
                for (int i = 0; i < n && i < 30; i++) {
                    void* item = spinox::array_edit::at(obj, i);
                    ImGui::Text("  [%d] %p", i, item);
                }
                ImGui::Unindent(12);
            }
        }
        spinox::il2cpp::freeString(tn);
    }, nullptr);
}
static void tabAob() {
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_SEARCH " AOB Scanner");
    ImGui::TextDisabled("Pattern: '48 8B ?? 89' — ?? wildcard");
    static char module[64] = "libil2cpp.so";
    static char pattern[128] = "";
    static std::vector<uintptr_t> hits;
    ImGui::InputText("Module", module, sizeof(module));
    ImGui::InputText("Pattern", pattern, sizeof(pattern));
    if (ImGui::Button(ICON_SEARCH " Scan")) {
        hits = spinox::memory::scanPattern(module, pattern);
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_TIMES " Clear")) hits.clear();
    ImGui::Separator();
    for (size_t i = 0; i < hits.size() && i < 50; i++) {
        ImGui::Text("%08x", (unsigned)(hits[i] & 0xFFFFFFFF));
        ImGui::SameLine();
        ImGui::PushID((int)i);
        if (ImGui::SmallButton(ICON_SAVE " Copy")) {
            char buf[32]; snprintf(buf, sizeof(buf), "0x%lx", (unsigned long)hits[i]);
            ImGui::SetClipboardText(buf);
        }
        ImGui::PopID();
    }
}
static void tabPreset() {
    ImGui::TextColored(ImVec4(0.98f, 0.14f, 0.25f, 1), ICON_SAVE " Presets");
    ImGui::Separator();
    static char label[64] = "default";
    ImGui::InputText("Name", label, sizeof(label));
    if (ImGui::Button(ICON_SAVE " Save Mods")) spinox::preset::saveCurrent(label);
    ImGui::SameLine();
    if (ImGui::Button(ICON_PLAY " Apply All")) spinox::preset::applyAll();
    ImGui::Separator();
    ImGui::TextDisabled(ICON_INFO "  %zu mods", spinox::preset::mods().size());
    for (auto& m : spinox::preset::mods()) {
        ImGui::Text("%s.%s::%s = %s",
            m.classNs.c_str(), m.className.c_str(),
            m.fieldName.c_str(), m.valueStr.c_str());
    }
}
static void tabLog() {
    if (ImGui::Button(ICON_TRASH " Clear")) spinox::log::clear();
    ImGui::Separator();
    ImGui::BeginChild("##lg");
    auto lines = spinox::log::snapshot();
    for (auto& l : lines) {
        ImVec4 c(0.7f, 0.7f, 0.75f, 1);
        if (l.level == ANDROID_LOG_ERROR) c = ImVec4(1, 0.4f, 0.4f, 1);
        else if (l.level == ANDROID_LOG_WARN) c = ImVec4(1, 0.8f, 0.3f, 1);
        ImGui::TextColored(c, "%s", l.text.c_str());
    }
    ImGui::EndChild();
}
void draw() {
    if (!spinox::il2cpp::init()) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), ICON_WARNING " IL2CPP not loaded");
        return;
    }
    loadClasses();
    static const char* tabs[T_COUNT] = {
        ICON_CUBES "  Classes", ICON_BOLT "  Hooks", ICON_DATABASE "  Arrays",
        ICON_SEARCH "  AOB", ICON_SAVE "  Preset", ICON_TERMINAL "  Log"};
    for (int i = 0; i < T_COUNT; i++) {
        if (i) ImGui::SameLine();
        bool active = (i == (int)g_tab);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.98f, 0.14f, 0.25f, 1));
        if (ImGui::Button(tabs[i], ImVec2(110, 30))) g_tab = (Tab)i;
        if (active) ImGui::PopStyleColor();
    }
    ImGui::Separator();
    ImGui::BeginChild("##content");
    switch (g_tab) {
        case T_CLASSES:
            switch (g_view) {
                case V_LIST: screenList(); break;
                case V_CLASS: screenClass(); break;
                case V_INSTANCE: screenInstance(); break;
                case V_METHOD: screenMethod(); break;
            }
            break;
        case T_HOOKS: tabHooks(); break;
        case T_ARRAYS: tabArrays(); break;
        case T_AOB: tabAob(); break;
        case T_PRESET: tabPreset(); break;
        case T_LOG: tabLog(); break;
        default: break;
    }
    ImGui::EndChild();
}
void reset() {
    g_loaded = false;
    g_classNames.clear(); g_classPtrs.clear();
    g_class = nullptr; g_instance = nullptr; g_method = nullptr;
    g_instances.clear(); g_history.clear(); g_view = V_LIST;
}
}
