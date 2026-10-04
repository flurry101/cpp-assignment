// ─── UI drawing helpers ──────────────────────────────────────────────────────
#include "app.h"

#include <algorithm>
#include <cstdio>
#include <mutex>

void draw_matrix_input(const char* id, int mat[MAX_N][MAX_N], int n) {
    ImGui::PushID(id);
    float cell = 58.0f;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (j) ImGui::SameLine(0, 6);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", mat[i][j]);
            ImGui::SetNextItemWidth(cell);
            char label[32]; snprintf(label, sizeof(label), "##r%dc%d", i & 0xFF, j & 0xFF);
            if (ImGui::InputInt(label, &mat[i][j], 0, 0))
                mat[i][j] = max(-999, min(9999, mat[i][j]));
        }
    }
    ImGui::PopID();
}

void draw_result_matrix(int n) {
    if ((int)g_results.size() < n) return;
    lock_guard<mutex> lk(g_mutex);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    float lineH = ImGui::GetTextLineHeightWithSpacing();
    ImGui::BeginChild("##resmat", ImVec2(0, lineH * (n + 1) + 12), false);
    for (int i = 0; i < n; ++i) {
        auto& r = g_results[i];
        if (!r.done) {
            ImGui::TextDisabled("Row %d  ... computing ...", i);
            continue;
        }
        ImGui::TextColored(COL_TEXT_DIM, "| ");
        for (int j = 0; j < n; ++j) {
            ImGui::SameLine(0, 0);
            ImGui::TextColored(COL_ACCENT, "%-6d", r.result[j]);
            ImGui::SameLine(0, 0);
        }
        ImGui::SameLine(0, 0);
        ImGui::TextColored(COL_TEXT_DIM, "|");
        ImGui::SameLine();
        ImGui::TextColored(COL_DONE, "<- T%d", r.thread_id);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void draw_thread_progress(int n) {
    if ((int)g_results.size() < n) return;
    lock_guard<mutex> lk(g_mutex);
    for (int i = 0; i < n; ++i) {
        auto& r = g_results[i];
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "T%d  Row%d", r.thread_id, r.row);
        float avail = ImGui::GetContentRegionAvail().x;
        float barW = min(260.0f, max(150.0f, avail * 0.42f));
        float frac = r.done ? 1.0f : 0.0f;
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, r.done ? (ImVec4)COL_GREEN : (ImVec4)COL_HEADER);
        ImGui::ProgressBar(frac, ImVec2(barW, 18), r.done ? "DONE" : "...");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 6);
        ImGui::TextColored(r.done ? COL_DONE : COL_TEXT_DIM, "%s", lbl);
    }
}
