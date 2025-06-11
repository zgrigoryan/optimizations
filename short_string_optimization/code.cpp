/*
 * Compare short-string vs. long-string performance
 * both with SSO (regular std::string) and without SSO
 * (std::string that uses a counting allocator).
 */

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <iomanip>

// ---------- Counting allocator ------------------------------------------------
template<typename T>
struct CountingAllocator {
    using value_type = T;

    static std::atomic<size_t> bytes_allocated;
    static std::atomic<size_t> alloc_calls;

    CountingAllocator() noexcept = default;
    template<class U> CountingAllocator(const CountingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        bytes_allocated += n * sizeof(T);
        ++alloc_calls;
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept {
        std::allocator<T>{}.deallocate(p, n);
    }

    template<class U>
    bool operator==(const CountingAllocator<U>&) const noexcept { return true; }
    template<class U>
    bool operator!=(const CountingAllocator<U>&) const noexcept { return false; }

    static void reset() {
        bytes_allocated = 0;
        alloc_calls    = 0;
    }
};

template<typename T>
std::atomic<size_t> CountingAllocator<T>::bytes_allocated{0};
template<typename T>
std::atomic<size_t> CountingAllocator<T>::alloc_calls{0};

using CountingString = std::basic_string<char,
                                        std::char_traits<char>,
                                        CountingAllocator<char>>;

// ---------- Measurement helpers ----------------------------------------------
struct Result {
    std::string label;
    double      ms;
    size_t      bytes;
    size_t      calls;
};

template<typename StringT>
Result run_test(std::string_view label, std::size_t N, std::size_t len)
{
    using clock = std::chrono::high_resolution_clock;

    if constexpr (!std::is_same_v<StringT, std::string>)
        CountingAllocator<char>::reset();

    std::vector<StringT> v;
    v.reserve(N);

    const auto start = clock::now();
    for (std::size_t i = 0; i < N; ++i)
        v.emplace_back(len, 'x');
    const auto stop  = clock::now();

    const double ms =
        std::chrono::duration<double, std::milli>(stop - start).count();

    return {
        std::string(label),
        ms,
        std::is_same_v<StringT, std::string> ? 0 :
              CountingAllocator<char>::bytes_allocated.load(),
        std::is_same_v<StringT, std::string> ? 0 :
              CountingAllocator<char>::alloc_calls.load()
    };
}

void benchmark(std::size_t N = 1'000'000)
{
    constexpr std::size_t SHORT_LEN = 8;   // within typical SSO buffer
    constexpr std::size_t LONG_LEN  = 128; // forces heap allocation

    std::cout << "Running with N = " << N << " strings\n\n";

    std::vector<Result> results;
    results.push_back(run_test<std::string>   ("std::string  SHORT", N, SHORT_LEN));
    results.push_back(run_test<std::string>   ("std::string  LONG ", N, LONG_LEN));
    results.push_back(run_test<CountingString>("no-SSO       SHORT", N, SHORT_LEN));
    results.push_back(run_test<CountingString>("no-SSO       LONG ", N, LONG_LEN));

    std::cout << std::left << std::setw(18) << "Case"
              << std::right << std::setw(12) << "Time(ms)"
              << std::setw(18) << "Bytes alloc"
              << std::setw(14) << "Alloc calls\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(18) << r.label
                  << std::right << std::setw(12) << std::fixed << std::setprecision(2) << r.ms
                  << std::setw(18) << r.bytes
                  << std::setw(14) << r.calls << '\n';
    }

    std::cout << "\n* std::string uses the implementation’s Small-String-Optimization (SSO).\n"
                 "* Replacing the allocator disables SSO in libstdc++ / libc++, "
                 "so every construction goes to the heap – our “without-SSO” baseline.\n";
}

int main(int argc, char* argv[])
{
    std::size_t N = 1'000'000;
    if (argc == 2)
        N = std::strtoull(argv[1], nullptr, 10);

    benchmark(N);
    return 0;
}
