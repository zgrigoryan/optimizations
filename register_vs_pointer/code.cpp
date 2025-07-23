// Build examples
//   g++   -O0 -std=c++20 code.cpp -o ptr_O0        # no opt
//   g++   -O3 -march=native -std=c++20 code.cpp -o ptr_O3
//   clang++ -O3 -march=native -std=c++20 code.cpp -o ptr_O3
//
// Count CPU cycles & instructions (Linux):
//   perf stat -e cycles,instructions ./ptr_O3
//
// macOS (Apple Silicon) quick-and-dirty counts:
//   sudo dtrace -qn 'profile-1ms /execname == "ptr_O3"/ { @ins[probefunc] = count(); }'
// or use Instruments ▸ “Counters” template.

#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <cstring>   // std::memcmp

// -----------------------------------------------------------------------------
// 1) Baseline: heavy pointer arithmetic inside BOTH loops
// -----------------------------------------------------------------------------
float sum_pointer(float* const* A, std::size_t rows, std::size_t cols)
{
    float s = 0.0f;
    for (std::size_t i = 0; i < rows; ++i)
        for (std::size_t j = 0; j < cols; ++j)
            s += *(*(A + i) + j);          // two derefs + two adds each time
    return s;
}

// -----------------------------------------------------------------------------
// 2) Optimised: cache row pointer once per outer loop
// -----------------------------------------------------------------------------
float sum_cached(float* const* A, std::size_t rows, std::size_t cols)
{
    float s = 0.0f;
    for (std::size_t i = 0; i < rows; ++i) {
        float* row = A[i];                 // fetched once -> likely kept in a register
        for (std::size_t j = 0; j < cols; ++j)
            s += row[j];
    }
    return s;
}

// -----------------------------------------------------------------------------
// Timing helper
// -----------------------------------------------------------------------------
template <typename F>
double time_it(F&& fun, const char* tag, float& result)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    result  = fun();
    auto t1 = std::chrono::high_resolution_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();
    std::cout << std::left << std::setw(10) << tag << ": "
              << std::fixed << std::setprecision(6) << secs << " s\n";
    return secs;
}

// -----------------------------------------------------------------------------
// Main driver
// -----------------------------------------------------------------------------
int main()
{
    constexpr std::size_t R = 4096;              // rows
    constexpr std::size_t C = 1024;              // cols  (≈ 16 M floats total)

    // single flat buffer for good spatial locality
    std::vector<float> buf(R * C);
    std::iota(buf.begin(), buf.end(), 0.0f);     // deterministic data

    // build row-pointer table
    std::vector<float*> rows(R);
    for (std::size_t i = 0; i < R; ++i)
        rows[i] = buf.data() + i * C;

    float s1 = 0, s2 = 0;
    time_it([&]{ return sum_pointer(rows.data(), R, C); }, "pointer", s1);
    time_it([&]{ return sum_cached (rows.data(), R, C); }, "cached",  s2);

    // sanity
    std::cout << "\nresults equal? " << (s1 == s2 ? "YES" : "NO") << '\n';
    std::cout << "sample sum  = " << s1 << '\n';
}
