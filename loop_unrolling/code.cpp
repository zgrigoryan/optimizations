#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <fstream>

#ifndef UF
#define UF 1
#endif
#ifndef ITERS
#define ITERS 100
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
    for (; i < SIZE; ++i)
        dst[i] = src[i];
}

template<int K>
uint64_t time_once(const std::vector<int>& src, std::vector<int>& dst) {
    auto t0 = std::chrono::high_resolution_clock::now();
    copy_unrolled<K>(src.data(), dst.data());
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
}

int main() {
    std::vector<int> src(SIZE), dst(SIZE);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 100);
    for (auto& x : src) x = dist(rng);

    uint64_t total = 0;
    for (int i = 0; i < ITERS; ++i)
        total += time_once<UF>(src, dst);

    double avg = static_cast<double>(total) / ITERS;

    std::ofstream out("results.csv", std::ios::app);
    out << UF << ", " << avg << '\n';
    std::cout << "Unroll factor: " << UF << ", Avg time: " << avg << " ns\n";
}