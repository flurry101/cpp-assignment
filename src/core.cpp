// Global state + small helpers
#include "app.h"

#include <sstream>

ImVec4 COL_BG        = {0.08f, 0.09f, 0.12f, 1.f};
ImVec4 COL_PANEL     = {0.13f, 0.14f, 0.18f, 1.f};
ImVec4 COL_HEADER    = {0.55f, 0.35f, 0.05f, 1.f};
ImVec4 COL_GREEN     = {0.18f, 0.55f, 0.28f, 1.f};
ImVec4 COL_ACCENT    = {0.30f, 0.60f, 1.00f, 1.f};
ImVec4 COL_DONE      = {0.25f, 0.85f, 0.40f, 1.f};
ImVec4 COL_TEXT_DIM  = {0.55f, 0.57f, 0.62f, 1.f};
ImVec4 COL_GOLD      = {1.00f, 0.80f, 0.20f, 1.f};

int  N          = 3;
int  matA[MAX_N][MAX_N] = {};
int  matB[MAX_N][MAX_N] = {};

vector<TResult>  g_results;
mutex            g_mutex;
atomic<int>      g_done_count{0};
atomic<bool>     g_computing{false};
atomic<bool>     g_finished{false};
atomic<bool>     g_shutdown_requested{false};
thread           g_coordinator;
atomic<int>      g_result_n{0};
atomic<double>   g_par_time{0.0};
atomic<double>   g_seq_time{0.0};
atomic<long long> g_bench_sink{0};

vector<string>   g_log;
mutex            g_log_mutex;

void push_log(const string& s) {
    lock_guard<mutex> lk(g_log_mutex);
    g_log.push_back(s);
}

Matrix snapshot_matrix(int src[MAX_N][MAX_N]) {
    Matrix out = {};
    for (int i = 0; i < MAX_N; ++i)
        for (int j = 0; j < MAX_N; ++j)
            out[i][j] = src[i][j];
    return out;
}

vector<int> multiply_row(int row, int n,
                        const Matrix& A,
                        const Matrix& B)
{
    vector<int> local(n, 0);
    for (int j = 0; j < n; ++j)
        for (int k = 0; k < n; ++k)
            local[j] += A[row][k] * B[k][j];
    return local;
}
