# cnlp-solution: a solution of a model

Version 0.2. Conventions: [README.md](README.md).

For `min (or max) f(x)  s.t.  lcon <= c(x) <= ucon,  lvar <= x <= uvar`, with `nvar`
variables and `ncon` constraints.

## 1. Content

Root attributes (HDF5) or top-level members (JSON): `nvar`, `ncon` (required) and
`fingerprint` (optional, section 4).

`/solution` (the group ExaModelsIO writes for a stored solution):

| Field | Type | Meaning |
|---|---|---|
| `solution` | f64[nvar] | `x` |
| `multipliers` | f64[ncon] | `y` |
| `multipliers_L`, `multipliers_U` | f64[nvar] | `zL, zU >= 0` |
| `objective` | f64 | `f(x)` in the model's sense |
| `minimize` | bool | `P_minimize` |
| `status` | string | `solved`, `acceptable`, `infeasible`, `unbounded`, `iteration_limit`, `time_limit` or `error` |
| `iterations` | integer | `-1` if unknown |
| `solver`, `solver_status` | string | optional |

With the layout tier, `/blocks/<kind>/<name>/` (JSON: `variables`, `constraints`,
`parameters` objects) may add each block's slice in its shape, with `dims`:
variables `value`, `multiplier_L`, `multiplier_U`; constraints `value` (= `c(x)`),
`multiplier`; parameters `value`. The flat arrays are the reference. Parameters may
instead be a `/parameters` section ([parameters.md](parameters.md)).

## 2. Signs

The multipliers make this Lagrangian stationary:

    L = σ f(x) + yᵀ c(x) − zLᵀ (x − lvar) + zUᵀ (x − uvar),   σ = 1 (min), −1 (max),

the multipliers of `min σf` and of the ABI Hessian with `obj_weight = σ`. Writers and
readers convert (measured on HS71, both senses):

| Source | Stationarity | Conversion |
|---|---|---|
| NLPModels results (MadNLP, NLPModelsIpopt) | `∇f + Jᵀy − zL + zU = 0` | maximization: negate `y`, `zL`, `zU` |
| MathOptInterface duals | `σ∇f − Jᵀy − zL − zU = 0` | `y = −y_MOI`, `zU = −zU_MOI` |

## 3. Use

With no format given, a consumer writes with the model's `P_default_writer` if it has one,
otherwise this format (`.json` gives JSON, anything else HDF5).

Read against a model, a solution is a warm start (`x`, `y`, `zL`, `zU`). It must belong to
the instance: `nvar` and `ncon` match (always checked), and `model`, `library` and
`fingerprint` match when present. A failed check is an error unless explicitly overridden.

## 4. Fingerprint

`sha256:` plus the lowercase hex SHA-256 of, as little-endian `int64` unless stated:

1. the 18 ASCII bytes `cnlp-fingerprint/1`;
2. `nvar`, `ncon`;
3. `nnzj`, its rows, its columns (`P_jac_structure`, 1-based, in order);
4. `nnzh`, its rows, its columns (`P_hess_structure`);
5. `P_nblocks` (0 without the layout tier), then per block: `P_block`'s `kind`, `offset`,
   `length`, `ndims`, dims, then the name's byte length and UTF-8 bytes.

It covers structure only: bounds, the starting point and parameters are excluded.
Reference implementation: [conformance/fingerprint.py](../conformance/fingerprint.py).
