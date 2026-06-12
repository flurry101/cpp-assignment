// ============================================================
//  Parallel Matrix Computation System  •  GLFW + OpenGL3 backend
// ============================================================
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <cmath>

using namespace std;
using namespace chrono;

// ─── Colour palette (ImVec4 helpers) ─────────────────────────────────────────
static ImVec4 COL_BG        = {0.08f, 0.09f, 0.12f, 1.f};
static ImVec4 COL_PANEL     = {0.13f, 0.14f, 0.18f, 1.f};
static ImVec4 COL_HEADER    = {0.55f, 0.35f, 0.05f, 1.f};
static ImVec4 COL_GREEN     = {0.18f, 0.55f, 0.28f, 1.f};
static ImVec4 COL_ACCENT    = {0.30f, 0.60f, 1.00f, 1.f};
static ImVec4 COL_DONE      = {0.25f, 0.85f, 0.40f, 1.f};
static ImVec4 COL_TEXT_DIM  = {0.55f, 0.57f, 0.62f, 1.f};
static ImVec4 COL_GOLD      = {1.00f, 0.80f, 0.20f, 1.f};

// ─── Thread result ────────────────────────────────────────────────────────────
struct TResult {
    int            thread_id = 0;
    int            row       = 0;
    vector<int>    row_data;
    vector<int>    result;
    string         log_line;
    bool           done = false;
};

// ─── App state ────────────────────────────────────────────────────────────────
static constexpr int MAX_N = 8;
using Matrix = array<array<int, MAX_N>, MAX_N>;

static int  N          = 3;
static int  matA[MAX_N][MAX_N] = {};
static int  matB[MAX_N][MAX_N] = {};

static vector<TResult>  g_results;
static mutex            g_mutex;
static atomic<int>      g_done_count{0};
static atomic<bool>     g_computing{false};
static atomic<bool>     g_finished{false};
static atomic<bool>     g_shutdown_requested{false};
static thread           g_coordinator;
static atomic<int>      g_result_n{0};
static atomic<double>   g_par_time{0.0};
static atomic<double>   g_seq_time{0.0};
static atomic<long long> g_bench_sink{0};

// log ring-buffer for the console panel
static vector<string>   g_log;
static mutex            g_log_mutex;
static void push_log(const string& s) {
    lock_guard<mutex> lk(g_log_mutex);
    g_log.push_back(s);
}

static Matrix snapshot_matrix(int src[MAX_N][MAX_N]) {
    Matrix out = {};
    for (int i = 0; i < MAX_N; ++i)
        for (int j = 0; j < MAX_N; ++j)
            out[i][j] = src[i][j];
    return out;
}

static vector<int> multiply_row(int row, int n,
                                const Matrix& A,
                                const Matrix& B)
{
    vector<int> local(n, 0);
    for (int j = 0; j < n; ++j)
        for (int k = 0; k < n; ++k)
            local[j] += A[row][k] * B[k][j];
    return local;
}

// ─── Worker ──────────────────────────────────────────────────────────────────
void worker(int row, int n,
            const Matrix& A,
            const Matrix& B,
            bool visible = true)
{
    vector<int> local = multiply_row(row, n, A, B);
    if (!visible) {
        if (!local.empty())
            g_bench_sink.fetch_add(local[0], memory_order_relaxed);
        return;
    }
    // build log string
    ostringstream oss;
    oss << "Thread-T" << (row + 1) << " -> Row" << row << ": [";
    for (int k = 0; k < n; ++k) { oss << A[row][k]; if (k < n-1) oss << ","; }
    oss << "]xB = [";
    for (int j = 0; j < n; ++j) { oss << local[j]; if (j < n-1) oss << ", "; }
    oss << "] DONE";
    {
        lock_guard<mutex> lk(g_mutex);
        auto& res = g_results[row];
        res.result   = move(local);
        res.log_line = oss.str();
        res.done     = true;
    }
    push_log(oss.str());
    ++g_done_count;
}

// ─── Benchmark helpers ───────────────────────────────────────────────────────
static int bench_reps(int n) {
    if (n <= 3)  return 500000;
    if (n <= 4)  return 200000;
    if (n <= 5)  return  80000;
    if (n <= 6)  return  30000;
    if (n <= 7)  return  10000;
    return 4000;
}

static void sequential_multiply(const Matrix& A, const Matrix& B,int n)
{
    volatile int dummy = 0;
    int C[MAX_N][MAX_N] = {};
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            for (int k = 0; k < n; ++k)
                C[i][j] += A[i][k] * B[k][j];
    dummy = C[0][0];
    (void)dummy;
}

// ─── Launch parallel computation ─────────────────────────────────────────────
static void launch_computation() {
    if (g_computing) return;
    if (g_coordinator.joinable())
        g_coordinator.join();
    Matrix A = snapshot_matrix(matA);
    Matrix B = snapshot_matrix(matB);
    const int runN = N;
    // reset state
    {
        lock_guard<mutex> lk(g_mutex);
        g_results.clear();
        g_results.resize(runN);
        for (int i = 0; i < runN; ++i) {
            g_results[i].thread_id = i + 1;
            g_results[i].row       = i;
            g_results[i].row_data.assign(A[i].begin(), A[i].begin() + runN);
        }
    }
    {
        lock_guard<mutex> lk(g_log_mutex);
        g_log.clear();
        g_log.push_back("=== Computation started (N=" + to_string(runN) + ") ===");
    }
    g_done_count = 0;
    g_result_n.store(runN);
    g_finished.store(false);
    g_par_time.store(0.0);
    g_seq_time.store(0.0);
    g_computing.store(true);

    // run in owned coordinator thread
    g_coordinator = thread([A, B, runN](){
        const int REPS = bench_reps(runN);
        // ── Sequential: repeated to get measurable wall time ────────────────
        auto ts = high_resolution_clock::now();
        for (int r = 0; r < REPS; ++r)
            sequential_multiply(A, B, runN);
        auto te = high_resolution_clock::now();
        double seq_time = duration<double>(te - ts).count() / REPS;  // per-call average
        // ── Parallel: one real run (results shown to user) ───────────────────
        // Warm-up pass, isolated from UI state and console output
        {
            vector<thread>  wt;
            for (int i = 0; i < runN; ++i)
                wt.emplace_back(worker, i, runN, cref(A), cref(B), false);
            for (auto& t : wt) t.join();
        }
        // Timed parallel runs include thread creation/join overhead by design.
        int par_reps = max(1, min(200, REPS / max(1, runN)));
        double par_total = 0.0;
        for (int r = 0; r < par_reps; ++r) {
            // Reset done flags so workers run again
            {
                lock_guard<mutex> lk(g_mutex);
                for (auto& res : g_results) { res.done = false; res.result.clear(); }
            }
            auto tp0 = high_resolution_clock::now();
            vector<thread> threads;
            bool visible = (r == par_reps - 1);
            for (int i = 0; i < runN; ++i)
                threads.emplace_back(worker, i, runN, cref(A), cref(B), visible);
            for (auto& t : threads) t.join();
            auto tp1 = high_resolution_clock::now();
            par_total += duration<double>(tp1 - tp0).count();

            if (g_shutdown_requested.load())
                break;
        }
        double par_time = par_total / par_reps;
        g_seq_time.store(seq_time);
        g_par_time.store(par_time);
        g_finished.store(true);
        g_computing.store(false);
        double sp = (par_time > 0) ? seq_time / par_time : 0.0;
        ostringstream oss;
        oss << fixed << setprecision(6)
            << "=== Done | Parallel: " << par_time << "s"
            << " | Sequential: " << seq_time << "s"
            << " | Speedup: " << sp << "x"
            << "  [avg over " << REPS << " seq / " << par_reps << " par reps] ===";
        push_log(oss.str());
    });
}

// ─── Load sample matrix  ──────────────────────────────────
static void load_sample() {
    N = 3;
    int a[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    int b[3][3] = {{7,8,9},{2,3,4},{5,6,7}};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            matA[i][j] = a[i][j];
            matB[i][j] = b[i][j];
        }
}

// ─── Draw integer matrix grid ─────────────────────────────────────────────────
static void draw_matrix_input(const char* id, int mat[MAX_N][MAX_N], int n) {
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

// ─── Draw read-only result matrix ─────────────────────────────────────────────
static void draw_result_matrix(int n) {
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

// ─── Progress bar for threads ─────────────────────────────────────────────────
static void draw_thread_progress(int n) {
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
// GLFW error callback
static void glfw_error(int, const char* desc) { fprintf(stderr,"GLFW: %s\n",desc); }

int main() {
    glfwSetErrorCallback(glfw_error);
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1100, 780, "Parallel Matrix Computation System", nullptr, nullptr);
    if (!win) { glfwTerminate(); return 1; }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 1.1f;

    // Style
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
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    load_sample();

    // ── Render loop ──────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        int fbW, fbH;
        glfwGetFramebufferSize(win, &fbW, &fbH);
        // ── Full-screen window ───────────────────────────────────────────────
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize({(float)fbW,(float)fbH});
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove       |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        // ── Title bar ────────────────────────────────────────────────────────
        {
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
        }
        ImGui::Spacing();
        // ── Left column: inputs ──────────────────────────────────────────────
        float leftW  = 380.f;
        float rightW = (float)fbW - leftW - 28.f;
        ImGui::BeginChild("##left", ImVec2(leftW, 0), false);
        {
            bool busy = g_computing.load();
            // N selector
            ImGui::TextColored(COL_GOLD, "Matrix Size  N x N");
            ImGui::SetNextItemWidth(120);
            ImGui::SliderInt("##N", &N, 2, MAX_N);
            ImGui::SameLine();
            if (ImGui::SmallButton("Load Sample 3x3")) load_sample();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            // Matrix A
            ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_PANEL);
            ImGui::BeginChild("##panA", ImVec2(0, 30 + N*36), true);
            ImGui::TextColored(COL_ACCENT, "Matrix  A  (%dx%d)", N, N);
            ImGui::Spacing();
            draw_matrix_input("A", matA, N);
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::Spacing();
            // Matrix B
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
            // Compute button
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
            // Speedup card
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
        }
        ImGui::EndChild();
        ImGui::SameLine(0, 12);
        // ── Right column: threads + result ───────────────────────────────────
        ImGui::BeginChild("##right", ImVec2(rightW, 0), false);
        {
            int resultN = g_result_n.load();
            // Thread progress
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
            // Result matrix
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
        }
        ImGui::EndChild();

        ImGui::End();

        // Render
        ImGui::Render();
        glViewport(0, 0, fbW, fbH);
        glClearColor(COL_BG.x, COL_BG.y, COL_BG.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }
    g_shutdown_requested.store(true);
    if (g_coordinator.joinable())
        g_coordinator.join();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}