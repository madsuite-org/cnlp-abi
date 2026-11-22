# cnlp-numbers: number formats

Version 0.2. This document names the number formats a model can be evaluated in and fixes
their memory layout. `cnlp.h` uses it in two places: the single-precision family (`P_s*`)
and the format tier (`P_nformats`, `P_format_name`, `P_x*`).

## 1. Names

A format has one canonical name, and some have short aliases. Canonical names are the
standards' own spellings wherever a standard exists, so nothing here renames a format that
already has a name.

- A library lists canonical names only (`P_format_name`).
- A consumer accepts a canonical name or an alias, compares names as exact ASCII strings,
  and never guesses at a name it does not know: an unknown format is not used.
- Names defined here never change meaning. New formats are added as new names.
- A format defined outside this document uses a reverse-DNS prefix it controls
  (`org.example.posit32`), so independent definitions cannot collide.
- A name may carry parameters after a colon, `name:key=value[,key=value]`.

## 2. Defined formats

Elements are stored in the platform's native byte order, as the corresponding C types
are. Arrays of elements are contiguous, with no padding between elements.

| Canonical name | Alias | Size (bytes) | Value and layout | Source of the definition |
|---|---|---|---|---|
| `binary32` | `f32` | 4 | IEEE 754 binary32 | IEEE 754-2019; C23 Annex H `_Float32` |
| `binary64` | `f64` | 8 | IEEE 754 binary64 | IEEE 754-2019; C23 Annex H `_Float64` |
| `binary128` | `f128` | 16 | IEEE 754 binary128 (never a platform `long double` of another format) | IEEE 754-2019; C23 Annex H `_Float128` |
| `double_binary64` | `dd` | 16 | double-double, section 2.1 | layout of IBM extended double (GCC `__ibm128`, LLVM `ppc_fp128`) |
| `quad_binary64` | `qd` | 32 | quad-double, section 2.1 | layout of the QD library's C interface (`double[4]`) |
| `apfloat:prec=N` | | 16 + 8⌈N/64⌉ | arbitrary precision, section 2.2 | semantics of MPFR; layout defined here |

### 2.1 Double-double and quad-double

`double_binary64` is two `binary64` values `{hi, lo}`, in that order. The represented
value is the exact sum `hi + lo`. A value is normalized: `hi` is `hi + lo` rounded to
nearest, so `|lo| <= ulp(hi)/2`.

`quad_binary64` is four `binary64` values `x[0..3]` of decreasing magnitude. The
represented value is the exact sum of the four, and each term is at most half an ulp of
the one before it.

Infinities and NaNs are carried in the first term. The other terms are then zero.

No standard names these formats. The layout of `double_binary64` is that of IBM extended
double, a long-shipped platform format; the name is this document's.

### 2.2 Arbitrary precision

`apfloat:prec=N` carries a binary floating-point number with an `N`-bit significand,
`N >= 2`. An element is

    struct { int32_t kind; int32_t sign; int64_t exp; uint64_t limb[L]; }   L = ceil(N/64)

with no padding (16 + 8L bytes).

- `kind`: 0 finite nonzero, 1 zero, 2 infinity, 3 NaN.
- `sign`: +1 or −1 (also for zero and infinity; ignored for NaN).
- For a finite nonzero value: value = sign × m × 2^exp with 1/2 <= m < 1. The
  significand m is `limb[L-1] limb[L-2] ... limb[0]` read as a binary fraction, most
  significant limb last, so the top bit of `limb[L-1]` is set. Bits below the precision
  are zero.
- For the other kinds, `exp` and `limb` are zero.

This is MPFR's number model (precision in bits, significand in [1/2, 1)), stored in a
layout this document owns. A library converts at the boundary to whatever it computes
with (MPFR, Julia `BigFloat`), and the ABI never carries another library's types or
pointers. All buffers are allocated by the caller, so no memory crosses the boundary.

A model evaluated at `apfloat:prec=N` computes with `N`-bit arithmetic in which each
operation is correctly rounded to nearest, as MPFR and Julia `BigFloat` do at precision
`N`. It does not promise that the final result is correctly rounded.

## 3. Reserved names

These names are reserved with the meaning given by their source and are not yet defined
for evaluation:

| Names | Source |
|---|---|
| `binary16`, `binaryN` (N >= 128, a multiple of 32), `decimalN` (N >= 32, a multiple of 32) | IEEE 754-2019; C23 Annex H |
| `bfloat16` | DLPack `kDLBfloat`; LLVM `bfloat` |
| `float8_e4m3fn`, `float8_e5m2`, and the other 8-, 6- and 4-bit formats | DLPack `kDLFloat8_*` / `kDLFloat6_*` / `kDLFloat4_*` spellings |

## 4. DLPack

[DLPack](https://github.com/dmlc/dlpack) describes array elements as
`{code, bits, lanes}`. The formats that have a DLPack description:

| Name | DLPack `{code, bits, lanes}` |
|---|---|
| `binary32` | `{kDLFloat (2), 32, 1}` |
| `binary64` | `{kDLFloat (2), 64, 1}` |
| `binary128` | `{kDLFloat (2), 128, 1}`, not listed by DLPack itself |

`double_binary64`, `quad_binary64` and `apfloat` have no DLPack description. A
`double_binary64` array is not a DLPack array of `lanes = 2`, which means a SIMD vector of
two values. `cnlp.h` provides this mapping as `cnlp_format_to_dlpack`.
