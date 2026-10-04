// ─── Right column: thread progress, result, console log ──────────────────────
#include "app.h"

#include <algorithm>
#include <mutex>

void draw_right_panel(float rightW) {
    ImGui::BeginChild("##right", ImVec2(rightW, 0), false);
    int resultN = g_result_n.load();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
    float progH = 26.f + max(N, resultN) * 30.f;
    ImGui::BeginChild("##prog", ImVec2(0, progH), true);
    ImGui::TextColored(COL_GOLD, "Thread-wise Computation");
    ImGui::Spacing();
    if (resultN > 0 && (int)g_results.size() == resultN)
        draw_thread_progress(resultN);
    else
        ImGui::TextDisabled("Press 'Compute' to start.");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (g_finished.load() && resultN > 0) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
        float resH = 28.f + resultN * 26.f;
        ImGui::BeginChild("##res", ImVec2(0, resH), true);
        ImGui::TextColored(COL_GREEN, "Final Result Matrix  A × B");
        ImGui::Spacing();
        draw_result_matrix(resultN);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.06f, 0.07f, 0.09f, 1.f});
    ImGui::BeginChild("##log", ImVec2(0, 0), true);
    ImGui::TextColored(COL_TEXT_DIM, "Console Output");
    ImGui::Separator();
    {
        lock_guard<mutex> lk(g_log_mutex);
        bool auto_scroll = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
        for (auto& line : g_log) {
            bool isDone    = line.find("DONE") != string::npos;
            bool isHeader  = line.find("===") != string::npos;
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
            if (isHeader) {
                ImGui::PushStyleColor(ImGuiCol_Text, COL_GOLD);
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            } else if (isDone) {
                ImGui::PushStyleColor(ImGuiCol_Text, COL_DONE);
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::PopTextWrapPos();
        }
        if (auto_scroll)
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndChild();
}
