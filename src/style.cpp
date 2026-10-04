// ─── App style setup ─────────────────────────────────────────────────────────
#include "app.h"

void setup_style() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.f;
    style.FrameRounding     = 5.f;
    style.ChildRounding     = 6.f;
    style.ItemSpacing       = {8, 6};
    style.FramePadding      = {6, 4};
    style.ScrollbarRounding = 4.f;
    style.Colors[ImGuiCol_WindowBg]        = COL_BG;
    style.Colors[ImGuiCol_FrameBg]         = {0.20f, 0.22f, 0.27f, 1.f};
    style.Colors[ImGuiCol_FrameBgHovered]  = {0.26f, 0.28f, 0.35f, 1.f};
    style.Colors[ImGuiCol_Button]          = {0.28f, 0.52f, 0.90f, 0.80f};
    style.Colors[ImGuiCol_ButtonHovered]   = {0.35f, 0.62f, 1.00f, 1.f};
    style.Colors[ImGuiCol_Header]          = COL_HEADER;
    style.Colors[ImGuiCol_TitleBgActive]   = {0.18f, 0.19f, 0.24f, 1.f};
    style.Colors[ImGuiCol_Tab]             = {0.18f, 0.19f, 0.24f, 1.f};
    style.Colors[ImGuiCol_TabActive]       = {0.28f, 0.52f, 0.90f, 1.f};
}
