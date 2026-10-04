// ─── Left column: matrix inputs, compute button, benchmark card ──────────────
#include "app.h"

void draw_left_panel(float leftW) {
    ImGui::BeginChild("##left", ImVec2(leftW, 0), false);
    bool busy = g_computing.load();

    ImGui::TextColored(COL_GOLD, "Matrix Size  N x N");
    ImGui::SetNextItemWidth(120);
    ImGui::SliderInt("##N", &N, 2, MAX_N);
    ImGui::SameLine();
    if (ImGui::SmallButton("Load Sample 3x3")) load_sample();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    ImGui::BeginChild("##panA", ImVec2(0, 30 + N*36), true);
    ImGui::TextColored(COL_ACCENT, "Matrix  A  (%dx%d)", N, N);
    ImGui::Spacing();
    draw_matrix_input("A", matA, N);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    ImGui::BeginChild("##panB", ImVec2(0, 30 + N*36), true);
    ImGui::TextColored(COL_ACCENT, "Matrix  B  (%dx%d)", N, N);
    ImGui::Spacing();
    draw_matrix_input("B", matB, N);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (busy) {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.4f,0.4f,0.4f,1.f});
        ImGui::Button("  Computing...  ", {leftW - 16, 36});
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.18f,0.55f,0.28f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f,0.72f,0.38f,1.f});
        if (ImGui::Button("  ▶  Compute  A × B  ", {leftW - 16, 36}))
            launch_computation();
        ImGui::PopStyleColor(2);
    }

    if (g_finished.load()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.10f,0.18f,0.10f,1.f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
        ImGui::BeginChild("##speed", ImVec2(0, 178), true, ImGuiWindowFlags_NoScrollbar);
        double par_time = g_par_time.load();
        double seq_time = g_seq_time.load();
        double sp = par_time > 0 ? seq_time / par_time : 0.0;
        ImGui::TextColored(COL_GOLD,     "Benchmark");
        ImGui::Spacing();
        ImGui::TextColored(COL_DONE,     "Parallel   : %.6fs", par_time);
        ImGui::TextColored(COL_TEXT_DIM, "Sequential : %.6fs", seq_time);
        ImGui::TextColored(COL_GOLD,     "Speedup    : %.4fx  (avg)", sp);
        ImGui::Spacing();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
        ImGui::TextUnformatted("For small N, thread creation and scheduling overhead dominate. This demo shows row-wise threading architecture, not high-performance acceleration.");
        ImGui::PopTextWrapPos();
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}
