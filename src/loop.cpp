// ─── Render loop ─────────────────────────────────────────────────────────────
#include "app.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <algorithm>

void run_ui(GLFWwindow* win) {
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        int fbW, fbH;
        glfwGetFramebufferSize(win, &fbW, &fbH);

        // Full-screen window
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({(float)fbW, (float)fbH});
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove       |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Title bar
        ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_HEADER);
        ImGui::BeginChild("##title", ImVec2(0, 52), false);
        ImGui::SetCursorPos({14, 8});
        ImGui::TextColored(COL_GOLD, "PROBLEM 06 / 10");
        ImGui::SameLine(0, 20);
        ImGui::TextColored({1,1,1,1}, "Parallel Matrix Computation System");
        ImGui::SameLine(0, 20);
        ImGui::TextColored(COL_TEXT_DIM, "— Multithreaded Matrix Multiply");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        float leftW  = 380.f;
        float rightW = (float)fbW - leftW - 28.f;
        draw_left_panel(leftW);
        ImGui::SameLine(0, 12);
        draw_right_panel(rightW);
        ImGui::End();

        // Render
        ImGui::Render();
        glViewport(0, 0, fbW, fbH);
        glClearColor(COL_BG.x, COL_BG.y, COL_BG.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }
}
