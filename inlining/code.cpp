#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>
#include <string>
#include <cstdlib>

// use -DFORCE_INLINE or -DNO_INLINE during compilation
#if defined(_MSC_VER)
    #define ALWAYS_INLINE __forceinline
    #define NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define ALWAYS_INLINE inline __attribute__((always_inline))
    #define NO_INLINE __attribute__((noinline))
#else
    #define ALWAYS_INLINE inline
    #define NO_INLINE
#endif

#if defined(FORCE_INLINE_MODE)
    #define INLINE_ATTR ALWAYS_INLINE
#elif defined(NO_INLINE_MODE)
    #define INLINE_ATTR NO_INLINE
#else
    #define INLINE_ATTR inline
#endif

using namespace std;
using namespace std::chrono;

// Small functions used in loop
INLINE_ATTR int add(int a, int b) {
    return a + b;
}

INLINE_ATTR int multiply(int a, int b) {
    return a * b;
}

// Computation workload
int compute(int n) {
    int sum = 0;
    int product = 1;

    for (int i = 1; i <= n; ++i) {
        sum = add(sum, i);
        product = multiply(product, i); // product may overflow, it's OK for benchmark
    }

    return sum + product;
}

int main(int argc, char* argv[]) {
    int n = 1000000;
    int repeats = 10;
    string output_file = "results.csv";

    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) repeats = atoi(argv[2]);
    if (argc > 3) output_file = argv[3];

    vector<double> durations;
    durations.reserve(repeats);

    for (int i = 0; i < repeats; ++i) {
        auto start = high_resolution_clock::now();
        volatile int result = compute(n);  // volatile to prevent optimization-out
        auto end = high_resolution_clock::now();
        durations.push_back(duration<double>(end - start).count());
    }

    ofstream out(output_file, ios::app);
    if (!out) {
        cerr << "Failed to open " << output_file << endl;
        return 1;
    }

    // detect optimization type from macros
    #if defined(FORCE_INLINE)
        string mode = "forced_inline";
    #elif defined(NO_INLINE)
        string mode = "no_inline";
    #else
        string mode = "default_inline";
    #endif

    for (int i = 0; i < repeats; ++i) {
        out << mode << "," << n << "," << durations[i] << endl;
    }

    cout << "Benchmark completed. Results written to " << output_file << endl;
    return 0;
}
