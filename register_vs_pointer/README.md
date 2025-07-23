# Algebraic Reductions & Auto‑Vectorisation Mini‑Benchmark

This micro‑benchmark explores three things:

1. **Algebraic simplification** – can the compiler fold `2*a + 3*b − 10`
   into a pair of fused‑multiply‑adds (FMAs)?
2. **Auto‑vectorisation** – does the loop turn into SIMD instructions at `-O3`?
3. **Pointer arithmetic vs. index arithmetic** – what overhead remains if we
   constantly dereference pointers?

`code.cpp` implements two kernels over 16 M floats:

| Kernel | Expression | Notes |
|--------|------------|-------|
| **baseline** | `out[i] = a[i]*2 + b[i]*3 - 10` | straightforward scalar formula |
| **fma**      | `out[i] = fma(b[i],3,fma(a[i],2,-10))` | one `fma` per multiply‑add |

Both are wrapped in a driver that:

* fills vectors with deterministic data,
* times each kernel,
* compares the bytewise result (using `memcmp`),
* prints one sample so nothing is optimised away.

---

## Build & run

```bash
# Reference build – no optimisation, no vectorisation
g++ -O0 -std=c++20 code.cpp -o ptr_O0

# Optimised build – allow algebraic rewrite & SIMD
g++ -O3 -march=native -ffast-math \
    -Rpass=loop-vectorize -std=c++20 \
    code.cpp -o ptr_O3          # Clang on macOS
````

> On GCC use `-fopt-info-vec-optimized` instead of `-Rpass=loop-vectorize`.

---

## Results (Apple Silicon M‑series, Clang 17)

| build          |   baseline |        fma | equal? |
| -------------- | ---------: | ---------: | :----: |
| **`./ptr_O0`** | 0.021985 s | 0.050732 s | **NO** |
| **`./ptr_O3`** | 0.012109 s | 0.007186 s | **NO** |

---

### What do we learn?

1. **Vectorisation works.**
   Compilation emits “`vectorized loop (width: 4, interleaved: 4)`” remarks for both kernels at `-O3`. The SIMD version of **fma** is \~40 % faster than **baseline**.

2. **But the bytewise results differ.**
   The FMA path produces slightly different low‑order bits than the separate multiply‑add path. Because the driver uses `memcmp`, any tiny rounding divergence flips `equal?` to **NO**.
   *That’s expected*: FMA respects IEEE 754 but rounds once instead of twice.

3. **If you need numeric—rather than bit‑exact—equality**
   switch to an epsilon comparison:

   ```cpp
   const float eps = 1e-6f;
   bool ok = std::equal(out1.begin(), out1.end(), out2.begin(),
                        [=](float x, float y){ return std::abs(x - y) < eps; });
   ```

4. **At `-O0` the FMA kernel is slower**
   because the compiler can’t inline `std::fma` or vectorise the loop; each iteration calls the libm scalar FMA routine.

---

## Interpreting the Clang vectorisation remark

```
remark: vectorized loop (vectorization width: 4, interleaved count: 4)
```

* **width = 4** – four `float`s per SIMD register (`s4` on Neon, `xmm` on SSE).
* **interleave = 4** – four such vectors issued before the next load/store round.

Use

```bash
clang++ -O3 -march=native -ffast-math -S -std=c++20 code.cpp
```

and search for `fmla`/`fmadd` (Arm) or `vfmaddps` (x86) to see the fused instructions.

---

## Profiling instruction counts (macOS)

```bash
sudo dtrace -qn 'profile-500hz /execname == "ptr_O3"/ { @func[probefunc] = count(); }' -c ./ptr_O3
```

Press **Ctrl‑C** (or let it exit) and you should see something like:

```
  saxpy_fma                           580
  saxpy_baseline                      975
  main                                 34
```

Sampling confirms most time is spent in the two kernels; the FMA version gets
fewer samples, mirroring its lower runtime.

---

## Take‑aways

* **`-O3 -ffast-math` lets the optimizer do the same algebra you might do by hand**.
* **Auto‑vectorised FMA beats scalar code** on any modern CPU with SIMD hardware.
* **Bit‑exact checks can be misleading** when different floating‑point evaluation orders are allowed; prefer epsilon checks unless strict reproducibility is vital.

Happy benchmarking!