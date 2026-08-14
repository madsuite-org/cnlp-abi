# Prior art: C interfaces for nonlinear programs

Survey conducted 2026-08-14 against primary sources (headers, official docs);
each claim below was verified against the cited artifact unless marked
best-effort. Conclusion first: **no existing standard defines what cnlp
defines** — a solver-independent, self-describing, size-parametric-at-runtime
NLP evaluation ABI behind a plain shared library — but several systems supply
precedent for individual pieces, and the spec deliberately aligns with or
borrows from them where noted.

## Ranked by closeness

### CUTEst (+ SIFDecode)
The benchmark-world standard: SIF problems decoded to per-problem sources and
compiled into a **per-problem** shared library with a fixed evaluation API
(`cutest.h`: `CUTEST_cfn`, `CUTEST_csgr`, `CUTEST_csh` — triplet sparsity,
1-based indices, integer statuses). Sizes are chosen at **decode time**
(`sifdecoder -param N=1000`) and are outputs of `CUTEST_csetup`, not inputs;
data arrives via a Fortran logical unit on `OUTSDIF.d`; state is global per
loaded library (no instance handles). Shares cnlp's index and sparsity
discipline; lacks runtime sizing, multi-model libraries, instances,
discoverability, and `obj_weight` in the Hessian call. Has what cnlp lacks:
Jacobian/Hessian-vector products, constraint classification flags, precision
variants (`ipc_`/`rpc_` typedefs — the template if cnlp ever adds f32/i64).

### CppADCodeGen model libraries
The closest **structural** precedent: one dynamic library carrying a *bundle
of models*, with a library-level catalogue symbol (`cppad_cg_models`) and an
ABI-version symbol (`cppad_cg_version`) — the design cnlp's `cnlp_nmodels` /
`cnlp_model_name` parallels, found nowhere else. Per model: forward/Jacobian/
Hessian entry points and separate sparsity-structure functions returning
pointers to in-library index arrays. Lacks NLP semantics entirely (generic
taped functions — no objective/constraint contract, no Lagrangian Hessian),
dimensions fixed at tape generation, and the ABI is an implementation detail
of its C++ loader rather than a specified standard.

### CasADi code generation (+ `casadi_c.h`, acados)
Generated C with a documented convention per function: `fname(arg, res, iw,
w, mem)` plus meta-symbols (`fname_n_in`, `fname_name_in`,
`fname_sparsity_in` — compact CCS via pointer to a const in-library array,
zero-copy; `fname_work` size pre-query; `incref`/`decref` lifecycle).
Function-level, not model-level: NLP meaning exists only by naming convention
(`nlp_f`, `nlp_jac_g`, `nlp_hess_l`), sizes are baked at codegen, and there is
no in-library discovery (CasADi's catalogue lives in its loader for
serialized `.casadi` data files, not in compiled artifacts). acados consumes
this convention and adds live-parameter precedent (`set_param`, including
sparse update). The existence of **CasADiNLPModels.jl** — CasADi-codegenned
NLP `.so` files plus an ad-hoc sidecar JSON for x0/bounds/parameters, wrapped
as NLPModels — is independent evidence of demand for exactly cnlp's artifact,
assembled from parts.

### FMI / FMU
The architectural analogue from the simulation world: shared library + typed
model description + instantiation + get/set parameters between runs + named
variables. Nobody has applied it to NLP exchange (the 2026 `fmi-ls-dae`
layered-standard proposal reaches DAE residuals for optimization workflows,
not NLPs); an FMU has no objective or constraint-Jacobian/Lagrangian-Hessian
contract. cnlp is, structurally, "FMI for NLPs". Worth borrowing: the
versioned description schema, layered optional extensions, and the
conformance-test culture (cnlp's `tinyqp.c` + consumer suites play that role).

### AMPL Solver Library (ASL) + .nl (+ ampl/mp, PyNumero)
The opposite packaging philosophy: the model is a portable **data file**
(.nl) interpreted by one big evaluation library. `sphes(h, no, ow, y)` is
~30 years of precedent for cnlp's exact obj_weight+multipliers Lagrangian
Hessian semantics; typed **suffixes** (named auxiliary data on variables and
constraints) are the closest analogue to cnlp's named blocks. No compiled
model artifact, no runtime re-parameterization (parameters fold when the .nl
is written). ampl/mp (the successor) deliberately punts on AD; PyNumero's
`AmplInterface` is the Python-consumer pattern cnlpmodels parallels.

### NLPModels.jl
Not an ABI but the semantic ancestor: cnlp's evaluation surface is a
one-for-one C transliteration of `obj` / `grad!` / `cons!` /
`jac_structure!` / `jac_coord!` / `hess_structure!` / `hess_coord!` — COO,
lower triangle, `obj_weight`, multipliers, 1-based — inheriting a decade of
ecosystem validation (CUTEst.jl, NLPModelsJuMP, MadNLP, ExaModels all
conform). What cnlp lacks from it: `jprod`/`hprod` operators and the
least-squares sub-API — the most likely extension requests.

### Solver-callback C APIs (Ipopt, nlopt, GALAHAD, KNITRO, madnlp_c)
The same functions with the direction reversed: the **consumer implements**
evaluators and hands function pointers to a solver
(`IpStdCInterface.h`: `Eval_F_CB` … `Eval_H_CB` with `index_style`;
`nlopt_set_min_objective`; GALAHAD's C interfaces; madnlp_c's callback
struct). cnlp's contribution is precisely the flip — the model library
*exports* the evaluators — which makes a cnlp model directly bindable to
every one of these APIs. Ipopt's explicit `index_style` flag is the
vocabulary cnlp's "1-based inside the ABI" statement follows.

### GAMS GMO; COIN-OR Optimization Services
GMO: solver-independent evaluation semantics (incl. Hessian-vector) against
an in-process GAMS instance — proprietary, not a distributable artifact.
COIN-OR OS (OSiL XML + C++ evaluation): the last serious attempt at a
solver-independent NLP instance standard, dormant for a decade — the
cautionary example of heavyweight representation without a binary ABI or a
conformance ecosystem. (Best-effort; not re-verified against current source.)

### Julia compilation infrastructure
PackageCompiler/juliac export whatever `@ccallable` symbols a user defines —
no domain ABI. No other compiled-Julia model ABI was found; publishing this
spec now is what prevents ad-hoc sidecar conventions from proliferating.

## Naming

Verified 2026-08-14: PyPI `cnlp` is **taken** (a Chinese-NLP framework) and
`cnlp-transformers` (clinical NLP) is the most visible "cnlp" project; the
GitHub user `cnlp` is squatted. Free and unconflicted: `cnlpmodels` (PyPI),
`CNLPModels` (Julia General), `libcnlp`, crates/npm `cnlp`. Consequence:
never publish a bare `cnlp` Python package; the spelled-out expansion
("cnlp — a C ABI for NLPs") is what search engines need to separate this
from clinical/Chinese NLP.
