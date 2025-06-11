# De-virtualisation Micro-Benchmark

## 0. TL;DR (Quick-start)
```bash
# Clone / copy the two files
g++     -std=c++20 -O3 -march=native -DNDEBUG code.cpp -o devirt_bench
./devirt_bench 100000000          # default: 1e8 iterations

# Inspect the generated machine code (GCC syntax; replace with clang++)
objdump -dC devirt_bench | less
````

| Case                         | What we measure                                | Typical speed gap\* |
| ---------------------------- | ---------------------------------------------- | ------------------- |
| **1) Pure virtual call**     | Indirect jump via v-table **every** iteration  | reference           |
| **2) `Base*` + `final`**     | Same pointer, but compiler *may* de-virtualise | **6-10 × faster**   |
| **3) Direct `Derived::foo`** | Static dispatch, usually fully inlined         | fastest baseline    |

\* Clang 17 / Apple M3 @ -O3, 10 ⁸ iterations.

---

## 1. Why de-virtualisation matters

Virtual dispatch is powerful but costs a few **nanoseconds** per call:

* One **extra pointer indirection** (load v-ptr, then load function address).
* In tight loops it **breaks branch prediction** and **prevents inlining**, creating
  pipeline stalls and reducing instruction-level parallelism.
* Modern optimisers recover most of the lost performance if they can *prove* the
  dynamic type. This transformation is called **de-virtualisation**.

The little benchmark quantifies that rescue.

---

## 2. Anatomy of the benchmark

```text
Base (abstract)   ───► foo()  (virtual and pure)
    ▲
    │
    └── Derived final ─ foo()  (overrides;    factor * x + 1)
```

We test three scenarios:

| # | Call-site                           | Why it’s interesting                                           |
| - | ----------------------------------- | -------------------------------------------------------------- |
| 1 | `Base* p = &d; p->foo(i);`          | Classic polymorphism; compiler **must** emit an indirect call. |
| 2 | Same as #1 but `Derived` is `final` | Dynamic type known to be unique ⇒ *may* be optimised away.     |
| 3 | `d.foo(i);` (direct object)         | No polymorphism ⇒ serves as theoretical lower bound.           |

A `volatile std::uint64_t sink` swallows the return values so nothing gets
“optimised out”.

---

## 3. Building

### GCC

```bash
g++ -std=c++20 -O3 -march=native -flto -DNDEBUG code.cpp -o devirt_bench
```

* `-O3`: full optimisation
* `-march=native`: use host CPU features (AVX2, NEON, …)
* `-flto`: (optional) enables **link-time de-virtualisation** across TUs

### Clang / AppleClang

```bash
clang++ -std=c++20 -O3 -march=native -flto=thin -DNDEBUG code.cpp -o devirt_bench
```
--- 

## 4. Running & interpreting results

```bash
./devirt_bench 200000000      # choose iteration count
```

Sample on GCC 14 + Zen 4 (numbers in ms):

```
Iterations: 200000000

Case                          Time (ms)
----------------------------------------
1) Pure virtual call              304.1
2) Base* + final (devirt?)         27.9
3) Direct Derived::foo             27.8
```

**Rule of thumb**

* If row 2 ≈ row 3 → compiler successfully de-virtualised.
* If row 2 ≈ row 1 → optimiser failed (try LTO, higher `-O`, or add `final`).

---


### Friendly disassembly viewers

* **Godbolt (Compiler Explorer)** – paste code, choose *“clang -O3 -S”*, inspect.
* **LLVM-mca** – throughput/latency analysis (Godbolt sidebar ▶ *Tools*).
* **perf record / perf stat** – hardware counters on Linux.

---

## 5. License

The benchmark is released under the **MIT license** – hack, copy, embed, profit.

---

*Happy hunting – may all your v-tables collapse into mere multiplications!*
