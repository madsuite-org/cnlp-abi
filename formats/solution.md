# cnlp-solution: a solution of a model

Version 0.2. Shared conventions (encodings, identification, arrays):
[README.md](README.md).

The problem is

    minimize (or maximize)  f(x)
    subject to              lcon <= c(x) <= ucon,   lvar <= x <= uvar,

with `x` of length `nvar` and `c` of length `ncon`. A solution file records a point, its
multipliers, the objective and the termination status, and, when the model names its
blocks, the same data split by block.

## 1. The problem the solution belongs to

Root attributes (HDF5) or top-level members (JSON):

| Field | Type | Meaning |
|---|---|---|
| `nvar` | integer | required: number of variables |
| `ncon` | integer | required: number of constraints |
| `fingerprint` | string | optional: a digest of the instance's structure (section 6.2) |

## 2. The solution: section `/solution`

| Field | Type | Length | Meaning |
|---|---|---|---|
| `solution` | f64 array | `nvar` | the point `x` |
| `multipliers` | f64 array | `ncon` | constraint multipliers `y` |
| `multipliers_L` | f64 array | `nvar` | lower-bound multipliers `zL >= 0` |
| `multipliers_U` | f64 array | `nvar` | upper-bound multipliers `zU >= 0` |
| `objective` | f64 | | `f(x)` in the model's own sense, not negated for maximization |
| `minimize` | bool | | the objective sense (`P_minimize`) |
| `status` | string | | termination status (section 4) |
| `solver_status` | string | | optional: the solver's own status string |
| `iterations` | integer | | iterations taken, or `-1` if unknown |
| `solver` | string | | optional: solver name and version |

In HDF5, `/solution` is a group whose arrays are datasets and whose scalars are scalar
datasets. This is the same group ExaModelsIO writes for a solution stored with a model,
so the two read alike. In JSON it is the member `solution`.

### 2.1 Sign convention

The multipliers satisfy stationarity of

    L(x, y, zL, zU) = σ f(x) + yᵀ c(x) − zLᵀ (x − lvar) + zUᵀ (x − uvar),

with σ = 1 for minimization and −1 for maximization: the Lagrangian of the ABI's Hessian
(`obj_weight * ∇²f + Σ yᵢ ∇²cᵢ`, with `obj_weight` = σ at a solution). Equivalently, the
multipliers are those of the problem written as a minimization of σf, so `zL` and `zU` are
nonnegative and the multipliers of a model do not change with how its sense is written.

A writer converts from its solver's convention, and a reader converts back before using a
solution as a warm start. The conventions of common interfaces, measured on Hock–Schittkowski
problem 71 in both senses:

| Source | Stationarity it satisfies | To this convention |
|---|---|---|
| NLPModels / SolverCore results (MadNLP, NLPModelsIpopt) | `∇f + Jᵀy − zL + zU = 0` | for maximization negate `y`, `zL`, `zU`; unchanged for minimization |
| MathOptInterface duals (JuMP) | `σ∇f − Jᵀy − zL − zU = 0`, upper-bound duals ≤ 0 | `y = −y_MOI`, `zL = zL_MOI`, `zU = −zU_MOI`, in both senses |

## 3. Named blocks: section `/blocks`

When the model publishes named blocks (the layout tier), the file also carries each
block's slice, in its shape, under `/blocks/<kind>/<name>/` (HDF5 groups), or as members
of `variables`, `constraints` and `parameters` (JSON objects keyed by block name):

| Kind | Fields |
|---|---|
| `variables` | `value`, `multiplier_L`, `multiplier_U`: slices of `solution`, `multipliers_L`, `multipliers_U` |
| `constraints` | `value`, `multiplier`: `c(x)` and `multipliers` on the block |
| `parameters` | `value`: the parameters the solution was computed with |

Every block records `dims`, an HDF5 attribute or a JSON member. The flat arrays of
section 2 are always present and are the reference. The blocks are a convenience and must
agree with them. Instead of `/blocks/parameters`, a writer may record parameters as a
`/parameters` section in the [parameters format](parameters.md), so that the file can be
applied as a parameter file directly.

## 4. Status

`status` is solver-neutral. A writer maps its solver's outcome to the nearest of:

| Value | Meaning |
|---|---|
| `"solved"` | converged to the requested tolerance |
| `"acceptable"` | converged to a looser, acceptable tolerance |
| `"infeasible"` | the problem appears infeasible |
| `"unbounded"` | the objective appears unbounded |
| `"iteration_limit"` | stopped at the iteration limit |
| `"time_limit"` | stopped at the time limit |
| `"error"` | any other failure |

## 5. A model's default output

A model can name its own output: a library may export `P_default_writer`, naming one of
its writers (e.g. `"matpower"` for an OPF model; see the writers tier in `cnlp.h`). A
consumer asked for "the solution file" without a format:

1. uses that writer when the library names one;
2. otherwise writes this format, choosing the encoding by extension (`.json` for JSON,
   anything else HDF5).

A model's writer adds an output and does not replace this format, which a consumer can
always be asked for explicitly.

## 6. Reading

### 6.1 On its own, or against a model

A reader on its own returns the content. A reader given a model checks that the file
belongs to it, then returns the solution attached to the model, with blocks addressed
through the model's layout, so a file written without blocks still gets named access.

A solution read against a model is a warm start: `solution`, `multipliers`,
`multipliers_L` and `multipliers_U` give the initial `x`, `y`, `zL` and `zU`.

### 6.2 What "belongs" means

1. `nvar` and `ncon` equal the model's. This is required and always checked.
2. `model` and `library` match, when the file carries them.
3. `fingerprint` matches, when both the file and the model provide one.

The fingerprint digests the instance's **structure**: `nvar`, `ncon`, the Jacobian and
Hessian sparsity patterns, and the block layout. It excludes bounds and parameters, so a
solution can warm-start a perturbed instance of the same structure. It is SHA-256 over the
serialization of section 6.3. A consumer computes it from the ABI's structure calls;
`conformance/fingerprint.py` is the reference implementation. A digest that also covers bounds and
parameters, telling "the same instance" from "the same structure", may be added later as a
separate optional field; it would not change this one.

A failed check is an error. A consumer may offer an explicit override, for deliberately
using a solution on a different instance.

### 6.3 The fingerprint

The fingerprint is the string `sha256:` followed by the lowercase hexadecimal SHA-256 of
these bytes, in order, every integer a little-endian `int64`:

1. the 18 ASCII bytes `cnlp-fingerprint/1`;
2. `nvar`, `ncon`;
3. `nnzj`, then the `nnzj` row indices, then the `nnzj` column indices, as
   `P_jac_structure` returns them (1-based, in its order);
4. `nnzh`, then its row indices, then its column indices, as `P_hess_structure` returns
   them;
5. the number of named blocks (`P_nblocks`, or 0 for a model without the layout tier),
   then for each block k in order: `kind`, `offset`, `length`, `ndims`, the `ndims`
   dimensions (as `P_block` returns them), the byte length of its name, and the name's
   UTF-8 bytes.

Bounds, the starting point and parameter values are not included. The prefix names the
serialization, so a future one gets a new prefix rather than a different meaning.

## 7. Not in 0.2

Several solutions in one file (a sweep) are out of scope: one file holds one solution.
