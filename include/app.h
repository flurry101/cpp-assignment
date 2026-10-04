// ============================================================
//  Shared declarations for the Parallel Matrix Computation System
// ============================================================
#pragma once

#include "imgui.h"

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace chrono;

// ─── Colour palette (ImVec4 helpers) ─────────────────────────────────────────
extern ImVec4 COL_BG;
extern ImVec4 COL_PANEL;
extern ImVec4 COL_HEADER;
extern ImVec4 COL_GREEN;
extern ImVec4 COL_ACCENT;
extern ImVec4 COL_DONE;
extern ImVec4 COL_TEXT_DIM;
extern ImVec4 COL_GOLD;

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
inline constexpr int MAX_N = 8;
using Matrix = array<array<int, MAX_N>, MAX_N>;

extern int  N;
extern int  matA[MAX_N][MAX_N];
extern int  matB[MAX_N][MAX_N];

extern vector<TResult>  g_results;
extern mutex            g_mutex;
extern atomic<int>      g_done_count;
extern atomic<bool>     g_computing;
extern atomic<bool>     g_finished;
extern atomic<bool>     g_shutdown_requested;
extern thread           g_coordinator;
extern atomic<int>      g_result_n;
extern atomic<double>   g_par_time;
extern atomic<double>   g_seq_time;
extern atomic<long long> g_bench_sink;

// log ring-buffer for the console panel
extern vector<string>   g_log;
extern mutex            g_log_mutex;

void push_log(const string& s);
Matrix snapshot_matrix(int src[MAX_N][MAX_N]);
vector<int> multiply_row(int row, int n, const Matrix& A, const Matrix& B);

// ─── Worker ──────────────────────────────────────────────────────────────────
void worker(int row, int n, const Matrix& A, const Matrix& B, bool visible = true);

// ─── Benchmark / computation ─────────────────────────────────────────────────
int bench_reps(int n);
void sequential_multiply(const Matrix& A, const Matrix& B, int n);
void launch_computation();
void load_sample();

// ─── UI drawing helpers ──────────────────────────────────────────────────────
void draw_matrix_input(const char* id, int mat[MAX_N][MAX_N], int n);
void draw_result_matrix(int n);
void draw_thread_progress(int n);

// ─── Panels / frame ──────────────────────────────────────────────────────────
struct GLFWwindow;
void setup_style();
void run_ui(GLFWwindow* win);
void draw_left_panel(float leftW);
void draw_right_panel(float rightW);
