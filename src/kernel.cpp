// ─── Per-row multiply kernel + UI reporting ─────────────────────────────────
#include "app.h"

#include <sstream>

// One output row: C[row][:] = A[row][:] * B   (i-k-j order, register-hoisted)
vector<int> compute_row(int row, int n,
                               const Matrix& A,
                               const Matrix& B)
{
    vector<int> local(n, 0);
    for (int k = 0; k < n; ++k) {
        const int a = A[row][k];
        for (int j = 0; j < n; ++j)
            local[j] += a * B[k][j];
    }
    return local;
}

void run_row(int row, int n, const Matrix& A, const Matrix& B, bool visible)
{
    vector<int> local = compute_row(row, n, A, B);
    if (!visible) {
        if (!local.empty())
            g_bench_sink.fetch_add(local[0], memory_order_relaxed);
        return;
    }
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
