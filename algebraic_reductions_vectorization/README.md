# Algebraic Reductions & Auto‑Vectorisation Mini‑Benchmark

This micro‑benchmark shows how modern C++ compilers

1. **rewrite algebraic expressions** (constant factoring, reassociation, FMA folding)  
2. **auto‑vectorise** simple loops, and  
3. how both affect run time.

We implement the same element‑wise operation two ways:

| Kernel | Expression |
|--------|------------|
| **baseline** | `out[i] = a[i]*2 + b[i]*3 - 10` |
| **fma**      | `out[i] = fma(b[i], 3, fma(a[i], 2, -10))` |

---

## Build

```bash
# Debug/reference build – no optimisation, no vectorisation
g++ -O0 -std=c++20 code.cpp -o vec_O0        # GCC
# clang++ -O0 -std=c++20 code.cpp -o vec_O0  # Clang

# Optimised build – let the compiler simplify & vectorise
g++ -O3 -march=native -ffast-math \
    -fopt-info-vec-optimized -std=c++20 \
    code.cpp -o vec_O3

# Clang equivalent (shows vectorisation remarks):
clang++ -O3 -march=native -ffast-math \
        -Rpass=loop-vectorize -std=c++20 \
        code.cpp -o vec_O3
````

* `-march=native` enables AVX/Neon/ASIMD/… on your CPU.
* `-ffast-math` allows the compiler to reassociate FP expressions (required for some optimisations).
* `-fopt-info-vec-optimized` (GCC) or `-Rpass=loop-vectorize` (Clang) prints **“vectorized loop”** notes.

---

## Example output (Apple Silicon M‑series, Clang 17)

```text
$ ./vec_O0
baseline     : 0.023900 s
fma          : 0.051879 s
results equal? NO
sample out = 6710876.500000

$ ./vec_O3
baseline     : 0.006659 s
fma          : 0.006219 s
results equal? YES
sample out = 6710876.500000
```

### What happened?

| Flag      | Vectorised?                     | FMA used?                                    | Notes                                                                                                 |
| --------- | ------------------------------- | -------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| **`-O0`** | **no**                          | *baseline*: no<br>*fma*: yes (library `fma`) | Different rounding paths → bit‑wise mismatch.                                                         |
| **`-O3`** | yes (width = 4, interleave = 4) | both kernels                                 | The optimiser rewrote **baseline** into the same two FMAs, so `memcmp` succeeds and it’s \~4× faster. |

If you need *numeric* rather than *bit‑exact* equality at `-O0`, replace the `memcmp` check with a tolerance‑based comparison, e.g.

```cpp
float eps = 1e-6f;
bool ok = std::equal(out1.begin(), out1.end(), out2.begin(),
                     [=](float x, float y){ return std::abs(x - y) < eps; });
```

---

## Interpreting the vectorisation remarks

```
remark: vectorized loop (vectorization width: 4, interleaved count: 4)
```

* **width = 4**   → the compiler packs four `float`s into one 128‑bit SIMD register (Neon/SSE).
* **interleave = 4** → it issues four such vector ops before the next load/store round, hiding latency.

Use `-S -fverbose-asm` (GCC) or `-S -fno-color-diagnostics` (Clang) if you want to read the assembly; look for `fmadd`, `fmla`, `vaddps /vmulps`, etc.

---

## Re‑running on x86‑64

On an AVX2 laptop you should see:

| build     | baseline  | fma (kernel 2) |
| --------- | --------- | -------------- |
| **`-O0`** | \~0.75 s  | \~0.78 s       |
| **`-O3`** | \~0.040 s | \~0.030 s      |

The second kernel is faster because one FMA replaces a multiply + add pair.

---

## Take‑aways

1. **Compilers already know basic algebra.** At `-O3` the naïve formula and the hand‑fused version compile identically (when `-ffast-math` is allowed).
2. **Auto‑vectorisation is free performance** on straight‑line FP loops—always check the diagnostics.
3. **FMA vs non‑FMA** can change low‑level rounding; only test for bit‑exact equality when that actually matters. Otherwise compare with a tolerance.

Happy benchmarking!