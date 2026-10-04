// ─── Persistent worker pool + per-row kernel ─────────────────────────────────
#include "app.h"

#include <condition_variable>

namespace {

struct Pool {
    vector<thread>       threads;
    mutex                m;
    condition_variable   start_cv;
    condition_variable   done_cv;
    int                  generation = 0;   // bumped per job
    int                  n          = 0;   // matrix size in current job
    int                  rows       = 0;   // rows in current job
    int                  remaining  = 0;   // worker threads yet to finish
    bool                 visible   = false;
    bool                 stop      = false;
    const Matrix*        A         = nullptr;
    const Matrix*        B         = nullptr;
} P;

} // namespace

// One output row: C[row][:] = A[row][:] * B   (i-k-j order, register-hoisted)


static void pool_thread(int tid, int nthreads)
{
    int seen = 0;
    for (;;) {
        unique_lock<mutex> lk(P.m);
        P.start_cv.wait(lk, [&]{ return P.stop || P.generation != seen; });
        if (P.stop) return;
        seen = P.generation;
        int n = P.n, rows = P.rows;
        bool vis = P.visible;
        const Matrix *A = P.A, *B = P.B;
        lk.unlock();

        for (int row = tid; row < rows; row += nthreads)
            run_row(row, n, *A, *B, vis);

        lk.lock();
        if (--P.remaining == 0) {
            lk.unlock();
            P.done_cv.notify_one();
        }
    }
}

void pool_init()
{
    if (!P.threads.empty()) return;
    int nthreads = (int)thread::hardware_concurrency();
    if (nthreads <= 0) nthreads = 4;
    for (int i = 0; i < nthreads; ++i)
        P.threads.emplace_back(pool_thread, i, nthreads);
}

void pool_shutdown()
{
    {
        lock_guard<mutex> lk(P.m);
        P.stop = true;
    }
    P.start_cv.notify_all();
    for (auto& t : P.threads)
        if (t.joinable()) t.join();
    P.threads.clear();
}

// Runs one full parallel multiply. Blocking: returns when all rows are done.
void pool_run(int n, const Matrix& A, const Matrix& B, bool visible)
{
    pool_init();
    unique_lock<mutex> lk(P.m);
    P.n = n; P.rows = n; P.visible = visible;
    P.A = &A; P.B = &B;
    P.remaining = (int)P.threads.size();
    ++P.generation;
    lk.unlock();
    P.start_cv.notify_all();
    lk.lock();
    P.done_cv.wait(lk, []{ return P.remaining == 0; });
}
