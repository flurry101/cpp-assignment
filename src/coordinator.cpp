// ─── Coordinator: launches the parallel computation ─────────────────────
#include "app.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <sstream>

void launch_computation() {
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
        pool_run(runN, A, B, false);
        // Timed parallel runs dispatch to the persistent pool (no spawn cost).
        int par_reps = max(1, min(200, REPS / max(1, runN)));
        double par_total = 0.0;
        for (int r = 0; r < par_reps; ++r) {
            // Reset done flags so workers run again
            {
                lock_guard<mutex> lk(g_mutex);
                for (auto& res : g_results) { res.done = false; res.result.clear(); }
            }
            auto tp0 = high_resolution_clock::now();
            pool_run(runN, A, B, r == par_reps - 1);
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
