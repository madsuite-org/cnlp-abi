# cnlp — a C ABI for nonlinear programs

One shared library carries any number of optimization models; each model
publishes what it is, what it instantiates from, and how to evaluate it —
objective, gradient, constraints, sparse Jacobian and Lagrangian Hessian —
behind plain C symbols, consumable from any language with a `dlopen`.

- **[cnlp.h](cnlp.h)** — the specification. The header *is* the normative
  document (the sqlite3.h model — one artifact, nothing to drift from it):
  every contract lives in its comments, and its X-macros declare the full
  surface for a symbol prefix, so implementers get prototype checking
  against the same file they read.

## Relation to prior work

The evaluation semantics are [NLPModels.jl](https://jso.dev/NLPModels.jl/stable/api/)'s,
transliterated to C; the Lagrangian-Hessian convention (objective weight +
multipliers, lower triangle) goes back to the
[AMPL Solver Library](https://www.netlib.org/ampl/solvers/asl.h). The
compiled-model-per-library idea is [CUTEst](https://github.com/ralna/CUTEst)'s,
here made size-parametric at runtime and multi-model;
[CppADCodeGen](https://github.com/joaoleal/CppADCodeGen) pioneered the
in-library model catalogue this ABI's fixed-name symbols follow, and
[CasADi](https://web.casadi.org/docs/)'s codegen convention is the
function-level cousin. [FMI](https://fmi-standard.org) is the architectural
analogue from simulation — shared library, typed description, instantiation,
live parameters — with no NLP contract; this ABI is, structurally, FMI for
NLPs. Solver C APIs (Ipopt, nlopt) define the same callbacks in the opposite
direction — the consumer implements them for a solver — which is why a cnlp
library binds directly to them. No standalone NLP ABI specification
predates this one, to our knowledge.

Consumers: [CNLPModels.jl](https://github.com/madsuite-org/CNLPModels.jl)
(Julia) and [cnlpmodels-py](https://github.com/madsuite-org/cnlpmodels-py)
(Python, ctypes + numpy). Both carry `tinyqp.c`, a complete reference
implementation in plain C, and their test suites are the ABI's conformance
tests. The primary producer is
[ExaModelsCompiler](https://github.com/madsuite-org/ExaModels.jl), which
compiles [ExaModels](https://github.com/madsuite-org/ExaModels.jl) model
recipes ahead of time — but the ABI is producer-neutral by design: anything
that can export these symbols is a model library.
