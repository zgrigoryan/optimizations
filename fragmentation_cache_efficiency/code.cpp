// -------------------------------------------------------------
// Memory‑fragmentation vs cache‑friendly pool benchmark
// -------------------------------------------------------------
//
// Build:
//   g++  -O3 -std=c++20 -march=native code.cpp -o mem_bench
//   clang++ -O3 -std=c++20 -march=native code.cpp -o mem_bench
//
// Run:
//   ./mem_bench
//
// macOS "ground‑truth" peak RSS:  /usr/bin/time -l ./mem_bench
// Linux equivalent:              /usr/bin/time -v ./mem_bench
//
// -------------------------------------------------------------

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#if defined(__APPLE__)
    #include <mach/mach.h>
#elif defined(__linux__)
    #include <sys/resource.h>
    #include <unistd.h>
#endif

// -------------------------------------------------------------
// cross‑platform resident‑set‑size
// -------------------------------------------------------------
std::uint64_t current_rss_bytes()
{
#if defined(__APPLE__)
    mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<std::uint64_t>(info.resident_size);
    return 0;

#elif defined(__linux__)
    long rss_pages = 0;
    FILE* fp = std::fopen("/proc/self/statm", "r");
    if (fp && std::fscanf(fp, "%*s%ld", &rss_pages) == 1) {
        std::fclose(fp);
        return static_cast<std::uint64_t>(rss_pages) *
               static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
    }
    if (fp) std::fclose(fp);
    return 0;
#else
    return 0;   // unsupported OS – always 0
#endif
}

// -------------------------------------------------------------
// helpers
// -------------------------------------------------------------
using clock = std::chrono::high_resolution_clock;

struct Result {
    double        secs;
    std::uint64_t peak_bytes;
};

// -------------------------------------------------------------
// 1) Baseline: millions of tiny new[] / delete[] calls
// -------------------------------------------------------------
Result baseline(std::size_t N)
{
    constexpr std::size_t MIN_SZ = 8;
    constexpr std::size_t MAX_SZ = 256;

    std::mt19937_64 rng(42);
    std::uniform_int_distribution<std::size_t> dist(MIN_SZ, MAX_SZ);
    std::uniform_int_distribution<std::size_t> victim(0, N - 1);

    std::vector<char*> ptrs;
    ptrs.reserve(N);

    std::uint64_t peak = current_rss_bytes();
    auto t0 = clock::now();

    for (std::size_t i = 0; i < N; ++i) {
        std::size_t sz = dist(rng);
        char* p = new char[sz];
        // touch memory so pages are committed
        p[0] = static_cast<char>(sz);
        p[sz - 1] = static_cast<char>(sz >> 1);
        ptrs.push_back(p);

        // randomly delete an earlier block every ~3 allocations
        if (i > 10 && (i % 3 == 0)) {
            std::size_t k = victim(rng) % ptrs.size();
            delete[] ptrs[k];
            ptrs[k] = ptrs.back();
            ptrs.pop_back();
        }
        peak = std::max(peak, current_rss_bytes());
    }

    // clean up any survivors
    for (char* p : ptrs) delete[] p;

    auto t1 = clock::now();
    return { std::chrono::duration<double>(t1 - t0).count(), peak };
}

// -------------------------------------------------------------
// 2) Cache‑friendly slab: one big vector, index arithmetic
// -------------------------------------------------------------
Result pooled(std::size_t N)
{
    constexpr std::size_t MIN_SZ = 8;
    constexpr std::size_t MAX_SZ = 256;

    std::mt19937_64 rng(43);
    std::uniform_int_distribution<std::size_t> dist(MIN_SZ, MAX_SZ);

    // pessimistic upper bound so we never reallocate
    std::size_t pool_bytes = N * MAX_SZ;
    std::vector<std::uint8_t> pool(pool_bytes);
    std::size_t offset = 0;

    std::uint64_t peak = current_rss_bytes();
    auto t0 = clock::now();

    for (std::size_t i = 0; i < N; ++i) {
        std::size_t sz = dist(rng);
        if (offset + sz >= pool_bytes) break;            // out of space
        std::uint8_t* p = pool.data() + offset;
        p[0]          = static_cast<std::uint8_t>(sz);   // touch
        p[sz - 1]     = static_cast<std::uint8_t>(sz>>1);
        offset += sz;                                    // bump once
        // no per‑allocation delete → fragmentation‑free
        peak = std::max(peak, current_rss_bytes());
    }

    auto t1 = clock::now();
    return { std::chrono::duration<double>(t1 - t0).count(), peak };
}

// -------------------------------------------------------------
// main
// -------------------------------------------------------------
int main()
{
    constexpr std::size_t Ops = 1'000'000;      // total allocations

    Result r1 = baseline(Ops);
    Result r2 = pooled  (Ops);

    auto fmt_mb = [](std::uint64_t bytes) {
        return static_cast<double>(bytes) / (1024.0 * 1024.0);
    };

    std::cout << "\n=== Allocation‑intensive benchmark ===\n";
    std::cout << "ops = " << Ops << '\n\n';
    std::cout << "mode          time [s]   peak RSS [MiB]\n"
              << "------------- ---------- ---------------\n";
    std::cout << "baseline      "
              << r1.secs << "   "
              << fmt_mb(r1.peak_bytes) << '\n';
    std::cout << "pooled        "
              << r2.secs << "   "
              << fmt_mb(r2.peak_bytes) << '\n';
}
