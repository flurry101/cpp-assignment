// ─── Worker ──────────────────────────────────────────────────────────────────
#include "app.h"

#include <sstream>

void worker(int row, int n,
            const Matrix& A,
            const Matrix& B,
            bool visible)
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
