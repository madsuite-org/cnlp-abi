# cnlp-solution: a solution of a model

Draft for version 0.2. Shared conventions (encodings, identification, arrays):
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
(`obj_weight * ∇²f + Σ yᵢ ∇²cᵢ`, with `obj_weight` = σ at a solution). A writer converts if
its solver uses another convention.

> **To confirm before this version is final:** that MadNLP's and Ipopt's results, as returned
> through NLPModels, follow this convention, including for maximization.

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
solution can warm-start a perturbed instance of the same structure. It is SHA-256 over a
canonical serialization given with the conformance files. A library may export it, or a
consumer computes it from the ABI's structure calls.

A failed check is an error. A consumer may offer an explicit override, for deliberately
using a solution on a different instance.

## 7. Open questions

1. The sign convention of section 2.1: confirm it against MadNLP and Ipopt, including
   maximization.
2. Should the fingerprint have a second digest covering bounds and parameters, so a
   reader can tell "the same instance" from "the same structure"?
3. Several solutions in one file (a sweep): proposed out of scope for 0.2, one file per
   solution.
