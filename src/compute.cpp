// ─── Benchmark helpers + computation launch ──────────────────────────────────
#include "app.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <sstream>

int bench_reps(int n) {
    if (n <= 3)  return 500000;
    if (n <= 4)  return 200000;
    if (n <= 5)  return  80000;
    if (n <= 6)  return  30000;
    if (n <= 7)  return  10000;
    return 4000;
}

void sequential_multiply(const Matrix& A, const Matrix& B, int n)
{
    volatile int dummy = 0;
    int C[MAX_N][MAX_N] = {};
    for (int i = 0; i < n; ++i)
        for (int k = 0; k < n; ++k) {
            const int a = A[i][k];
            for (int j = 0; j < n; ++j)
                C[i][j] += a * B[k][j];
        }
    dummy = C[0][0];
    (void)dummy;
}


void load_sample() {
    N = 3;
    int a[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    int b[3][3] = {{7,8,9},{2,3,4},{5,6,7}};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            matA[i][j] = a[i][j];
            matB[i][j] = b[i][j];
        }
}
