#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstring>   // std::memcmp
#include <cmath>     // std::fma

// -----------------------------------------------------------------------------
//  Baseline:  out[i] = a[i] * 2 + b[i] * 3 - 10
// -----------------------------------------------------------------------------
void saxpy_baseline(const float* __restrict a,
                    const float* __restrict b,
                    float* __restrict out,
                    std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i)
        out[i] = a[i] * 2.0f + b[i] * 3.0f - 10.0f;
}

// -----------------------------------------------------------------------------
//  Rewrite using two fused‑multiply‑adds.
//  Same arithmetic, fewer floating‑point operations when FMA is available.
// -----------------------------------------------------------------------------
void saxpy_fma(const float* __restrict a,
               const float* __restrict b,
               float* __restrict out,
               std::size_t n)
{
    constexpr float C1 = 2.0f;
    constexpr float C2 = 3.0f;
    constexpr float C3 = -10.0f;

    for (std::size_t i = 0; i < n; ++i)
        out[i] = std::fma(b[i], C2, std::fma(a[i], C1, C3));
}

// -----------------------------------------------------------------------------
//  Timing helper
// -----------------------------------------------------------------------------
template <typename F>
double time_it(F&& fun, const char* tag)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    fun();
    auto t1 = std::chrono::high_resolution_clock::now();

    double secs = std::chrono::duration<double>(t1 - t0).count();
    std::cout << std::left << std::setw(12) << tag << " : "
              << std::fixed << std::setprecision(6) << secs << " s\n";
    return secs;
}

// -----------------------------------------------------------------------------
//  Main driver
// -----------------------------------------------------------------------------
int main()
{
    constexpr std::size_t N = 1u << 24;            // 16 M elements (~64 MiB I/O)

    std::vector<float> a(N), b(N), out1(N), out2(N);

    for (std::size_t i = 0; i < N; ++i) {
        a[i] = 0.1f * static_cast<float>(i);
        b[i] = 0.2f * static_cast<float>(i);
    }

    // run & time both kernels
    time_it([&]{ saxpy_baseline(a.data(), b.data(), out1.data(), N); }, "baseline");
    time_it([&]{ saxpy_fma     (a.data(), b.data(), out2.data(), N); }, "fma");

    // verify results (bit‑exact)
    bool ok = !std::memcmp(out1.data(), out2.data(), N * sizeof(float));
    std::cout << "\nresults equal? " << (ok ? "YES" : "NO") << '\n';

    // print one value so nothing is optimised away
    std::cout << "sample out = " << out1[N / 2] << '\n';
}