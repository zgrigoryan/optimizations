#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#ifndef UF               // unroll factor (1, 4, 8 …)
#   define UF 4
#endif
#ifndef ITERS            // outer repetitions to warm caches & smooth timers
#   define ITERS 200
#endif

constexpr std::size_t SIZE = 1'000'000;

template<int K>
void copy_unrolled(const int* __restrict src, int* __restrict dst) {
    std::size_t i = 0;
    for (; i + K - 1 < SIZE; i += K) {
#pragma unroll
        for (int j = 0; j < K; ++j)
            dst[i + j] = src[i + j];
    }
    for (; i < SIZE; ++i) dst[i] = src[i];      // tail
}

template<>
void copy_unrolled<1>(const int* __restrict s, int* __restrict d) {
    for (std::size_t i = 0; i < SIZE; ++i) d[i] = s[i];
}

template<int K>
std::uint64_t time_once(const int* s, int* d) {
    auto t0 = std::chrono::high_resolution_clock::now();
    copy_unrolled<K>(s, d);
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
}

int main() {
    std::vector<int> src(SIZE), dst(SIZE);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 100);
    std::generate(src.begin(), src.end(), [&](){ return dist(rng); });

    std::uint64_t total = 0;
    for (int i = 0; i < ITERS; ++i)
        total += time_once<UF>(src.data(), dst.data());

    std::cout << "UF=" << UF << "  avg=" << (double)total / ITERS << " ns\n";
}
