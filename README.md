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
- **[RELATED_WORK.md](RELATED_WORK.md)** — how this relates to CUTEst,
  CppADCodeGen, CasADi, FMI, ASL, and NLPModels.jl, and what it borrows.

Consumers: [CNLPModels.jl](https://github.com/madsuite-org/CNLPModels.jl)
(Julia) and [cnlpmodels-py](https://github.com/madsuite-org/cnlpmodels-py)
(Python, ctypes + numpy). Both carry `tinyqp.c`, a complete reference
implementation in plain C, and their test suites are the ABI's conformance
tests. The primary producer is
[ExaModelsCompiler](https://github.com/madsuite-org/ExaModels.jl), which
compiles [ExaModels](https://github.com/madsuite-org/ExaModels.jl) model
recipes ahead of time — but the ABI is producer-neutral by design: anything
that can export these symbols is a model library.
