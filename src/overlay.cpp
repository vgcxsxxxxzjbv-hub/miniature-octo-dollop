#include "spinox/overlay.h"
#include "spinox/editor.h"
#include "spinox/hooks.h"
#include "spinox/log.h"
#include "spinox/icons.h"
#if SPINOX_FA_AVAILABLE
#include "fa_solid.h"
#endif
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <dobby.h>
#include <dlfcn.h>
#include <mutex>
#include <cmath>
namespace spinox::overlay {
static bool g_init = false, g_imgui = false, g_open = false;
static float g_tx = 0, g_ty = 0;
static int g_taction = -1;
static bool g_tdown = false;
static float g_bx = 60, g_by = 500;
static const float BR = 26;
static bool g_bdrag = false, g_bmoved = false;
static float g_bdx = 0, g_bdy = 0;
static EGLBoolean (*orig_swap)(EGLDisplay, EGLSurface) = nullptr;
static std::mutex g_mu;
static void style() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 14; s.FrameRounding = 8; s.GrabRounding = 8;
    s.WindowBorderSize = 0; s.FramePadding = ImVec2(8, 4); s.ItemSpacing = ImVec2(8, 6);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.055f, 0.078f, 1);
    c[ImGuiCol_ChildBg] = ImVec4(0.078f, 0.078f, 0.109f, 1);
    c[ImGuiCol_Text] = ImVec4(0.96f, 0.96f, 0.97f, 1);
    c[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.60f, 1);
    c[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.22f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.98f, 0.14f, 0.25f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(0.72f, 0.05f, 0.14f, 1);
    c[ImGuiCol_SliderGrab] = ImVec4(0.98f, 0.14f, 0.25f, 1);
    c[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.14f, 0.25f, 1);
    c[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.14f, 1);
    c[ImGuiCol_Header] = ImVec4(0.98f, 0.14f, 0.25f, 0.4f);
    c[ImGuiCol_Separator] = ImVec4(0.13f, 0.13f, 0.17f, 1);
}
static void loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig textCfg; textCfg.SizePixels = 16.0f;
    io.Fonts->AddFontDefault(&textCfg);
#if SPINOX_FA_AVAILABLE
    ImFontConfig iconCfg; iconCfg.MergeMode = true; iconCfg.PixelSnapH = true;
    static const ImWchar iconRanges[] = { 0xf000, 0xf8ff, 0 };
    io.Fonts->AddFontFromMemoryTTF((void*)SPINOX_FA_DATA, SPINOX_FA_SIZE,
        16.0f, &iconCfg, iconRanges);
#endif
    io.Fonts->Build();
}
static void drawBubble() {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 c(g_bx, g_by);
    dl->AddCircleFilled(c, BR + 8, IM_COL32(255, 35, 64, 40));
    dl->AddCircleFilled(c, BR + 3, IM_COL32(255, 35, 64, 90));
    dl->AddCircleFilled(c, BR, IM_COL32(255, 35, 64, 255));
    ImVec2 ts = ImGui::CalcTextSize(ICON_MICROCHIP);
    ImGui::GetFont()->RenderText(dl, BR * 1.6f,
        ImVec2(g_bx - ts.x * 0.5f, g_by - ts.y * 0.5f),
        IM_COL32(255, 255, 255, 255), ICON_MICROCHIP);
}
static void drawWindow(ImGuiIO& io) {
    float w = io.DisplaySize.x - 40;
    float h = io.DisplaySize.y - 200;
    if (w > 700) w = 700;
    if (h > 900) h = 900;
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - w) / 2, (io.DisplaySize.y - h) / 2), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
    ImGui::Begin("##spx", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.14f, 0.25f, 1));
    ImGui::TextUnformatted(ICON_MICROCHIP " SPINOX");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.60f, 1));
    ImGui::TextUnformatted("IL2CPP EDITOR");
    ImGui::PopStyleColor();
    float btnW = 100;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - btnW * 2 - 24);
    if (ImGui::Button(ICON_TRASH " Unhook", ImVec2(btnW, 26)))
        for (auto& e : spinox::hooks::list()) spinox::hooks::uninstall(e.target);
    ImGui::SameLine();
    if (ImGui::Button(ICON_TIMES " Close", ImVec2(btnW, 26))) g_open = false;
    ImGui::Separator();
    spinox::editor::draw();
    ImGui::End();
    ImGui::PopStyleVar();
}
static void onFrame(EGLDisplay dpy, EGLSurface surf) {
    if (!g_imgui) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        EGLint w = 0, h = 0;
        eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
        eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
        io.DisplaySize = ImVec2((float)w, (float)h);
        loadFonts();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        style();
        g_imgui = true;
        LOGI("overlay imgui %dx%d", w, h);
    }
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(g_tx, g_ty);
    io.MouseDown[0] = g_tdown;
    if (!g_open) {
        float dx = g_tx - g_bx, dy = g_ty - g_by;
        bool inside = (dx * dx + dy * dy) <= BR * BR;
        if (g_taction == 0) {
            if (inside) { g_bdrag = true; g_bmoved = false; g_bdx = g_tx - g_bx; g_bdy = g_ty - g_by; }
        } else if (g_taction == 2) {
            if (g_bdrag) {
                float nx = g_tx - g_bdx, ny = g_ty - g_bdy;
                if (fabsf(nx - g_bx) > 4 || fabsf(ny - g_by) > 4) g_bmoved = true;
                g_bx = nx; g_by = ny;
            }
        } else if (g_taction == 1) {
            if (g_bdrag) { g_bdrag = false; if (!g_bmoved && inside) g_open = true; }
        }
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    if (g_open) drawWindow(io); else drawBubble();
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
static EGLBoolean hook_swap(EGLDisplay dpy, EGLSurface surf) {
    { std::lock_guard<std::mutex> lk(g_mu); onFrame(dpy, surf); }
    return orig_swap(dpy, surf);
}
void init() {
    if (g_init) return;
    void* egl_lib = dlopen("libEGL.so", RTLD_NOW);
    if (!egl_lib) { LOGI("libEGL load failed"); return; }
    void* target = dlsym(egl_lib, "eglSwapBuffers");
    if (!target) target = (void*)eglSwapBuffers;
    if (DobbyHook(target, (void*)hook_swap, (void**)&orig_swap) != RS_SUCCESS) return;
    g_init = true;
    LOGI("overlay ready");
}
void toggle() { g_open = !g_open; }
bool isOpen() { return g_open; }
void setTouch(int action, float x, float y) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_tx = x; g_ty = y; g_taction = action;
    if (action == 0) g_tdown = true;
    else if (action == 1) g_tdown = false;
}
}
