# SSO vs. Heap Allocation – Micro-benchmark

This little suite illustrates how the **Small-String-Optimization (SSO)** in
`std::string` keeps short strings on the stack and
how much the program pays when SSO is deliberately turned off.

---

## What the program does

| Phase | Action |
|-------|--------|
| 1 | Builds **1 000 000 short strings** (8 chars) and **1 000 000 long strings** (128 chars). |
| 2 | Runs the test twice: <br/>• *With SSO* – ordinary `std::string`. <br/>• *Without SSO* – the same string type but with a user-supplied allocator (`CountingAllocator`). In both libstdc++ and libc++ a custom allocator disables SSO, forcing every construction onto the heap. |
| 3 | For each scenario it captures: construction time, heap bytes requested, and number of `allocate()` calls. |

---

## Build & run

```bash
g++ -std=c++17 -O2 -march=native sso_bench.cpp -o sso_bench
./sso_bench            # uses default N = 1,000,000
./sso_bench 5000000    # run any N you like
```

## Typical output (Apple Clang, Apple M2):
```bash
Running with N = 1000000 strings

Case                  Time(ms)       Bytes alloc  Alloc calls
std::string  SHORT        9.77                 0             0
std::string  LONG        45.92                 0             0
no-SSO       SHORT        3.42                 0             0
no-SSO       LONG        28.50         136000000       1000000

* std::string uses the implementation’s Small-String-Optimization (SSO).
* Replacing the allocator disables SSO in libstdc++ / libc++, so every construction goes to the heap – our “without-SSO” baseline.
```
