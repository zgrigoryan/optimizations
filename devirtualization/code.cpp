/*  devirt_bench.cpp
 *
 *  Goal: show how much overhead a virtual call adds,
 *        how the `final` keyword helps the optimiser,
 *        and when the compiler de-virtualises calls for us.
 *
 *  Build (GCC ≥9 or Clang ≥12):
 *      g++  -std=c++20 -O3 -march=native -DNDEBUG code.cpp -o devirt_bench
 *      # or: clang++ …
 *
 *  Run:
 *      ./devirt_bench 100000000        # 1e8 iterations (default)
 */

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using ns      = std::chrono::nanoseconds;

volatile std::uint64_t sink = 0;   // prevents the loop from being optimised away

/* ------------------------------------------------------------------ */
/* 1. A classic polymorphic hierarchy                                 */
/* ------------------------------------------------------------------ */
struct Base {
    virtual std::uint64_t foo(std::uint64_t x) const = 0;
    virtual ~Base() = default;
};

// `final` => cannot be subclassed further: gives compiler licence
// to replace indirect calls with direct jumps if the dynamic type
// is known at the call-site.
struct Derived final : Base {
    std::uint64_t factor;
    explicit Derived(std::uint64_t f = 2) : factor(f) {}
    std::uint64_t foo(std::uint64_t x) const override {
        return x * factor + 1;
    }
};

/* ------------------------------------------------------------------ */
/* 2. Timing helpers                                                  */
/* ------------------------------------------------------------------ */
template <typename F>
double time_ms(F&& f)
{
    auto beg = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    return static_cast<double>(std::chrono::duration_cast<ns>(end - beg).count()) /
           1'000'000.0;
}

/* ------------------------------------------------------------------ */
/* 3. Three flavours to compare                                       */
/* ------------------------------------------------------------------ */
double bench_virtual(std::size_t N)
{
    Derived d(2);
    const Base* p = &d;        // call via *base* pointer  → always virtual
    return time_ms([&] {
        for (std::size_t i = 1; i <= N; ++i)
            sink += p->foo(i);
    });
}

double bench_devirt_known_ptr(std::size_t N)
{
    Derived d(2);
    const Base* p = &d;        // still a Base*, *but* derived is `final`
    return time_ms([&] {
        for (std::size_t i = 1; i <= N; ++i)
            sink += p->foo(i); // many compilers can de-virtualise this
    });
}

double bench_direct(std::size_t N)
{
    Derived d(2);
    return time_ms([&] {
        for (std::size_t i = 1; i <= N; ++i)
            sink += d.foo(i);  // non-virtual → normal inlining
    });
}

/* ------------------------------------------------------------------ */
int main(int argc, char* argv[])
{
    std::size_t N = 100'000'000ULL;          // default: 1e8 iterations
    if (argc == 2) N = std::strtoull(argv[1], nullptr, 10);

    std::cout << "Iterations: " << N << "\n\n";

    const double tVirt   = bench_virtual(N);
    const double tDevirt = bench_devirt_known_ptr(N);
    const double tDirect = bench_direct(N);

    std::cout << std::left << std::setw(28) << "Case"
              << std::right << std::setw(12) << "Time (ms)\n"
              << std::string(40, '-') << '\n'
              << std::left << std::setw(28) << "1) Pure virtual call"
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << tVirt   << '\n'
              << std::left << std::setw(28) << "2) Base* + final (devirt?)"
              << std::right << std::setw(12) << tDevirt << '\n'
              << std::left << std::setw(28) << "3) Direct Derived::foo"
              << std::right << std::setw(12) << tDirect << '\n';
}
