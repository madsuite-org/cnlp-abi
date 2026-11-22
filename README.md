# cnlp: the C ABI for nonlinear programs

A cnlp library is a shared library that carries one or more optimization models. Each model
exports plain C symbols to instantiate it and to evaluate its objective, gradient,
constraints, sparse Jacobian and Lagrangian Hessian. Any language that can call C can load
the library and solve the models, with no modeling tool installed.

## What is here

- **[cnlp.h](cnlp.h)**: the specification, version 0.2. The header is the normative
  document: each contract is in its comments, and its macros declare every symbol for a
  model prefix, so an implementation is checked against the file it follows.
- **[formats/](formats/)**: the files that go with a library: the input a model is
  instantiated from ([input.md](formats/input.md)), parameter values
  ([parameters.md](formats/parameters.md)) and solutions ([solution.md](formats/solution.md)),
  in HDF5, and the number formats a model can be evaluated in
  ([numbers.md](formats/numbers.md)).
- **[example/](example/)**: a complete model library and a consumer in plain C.

A model always has the core symbols (sizes, bounds, starting point, evaluation). Optional
tiers add a library-level model list, instantiation from structured data, named blocks of
variables, constraints and parameters, the objective sense, solution writers, readers of
the model's own file formats, evaluation on a GPU with device pointers, evaluation in
binary32, and evaluation in other number formats (binary128, double-double, arbitrary
precision). A consumer checks one symbol per tier and works without the tiers a library
does not export.

## Example

```sh
cd example
cc -shared -fPIC -O2 -o libqp.so qp.c -lm
cc -O2 -o solve solve.c -L. -lqp -Wl,-rpath,'$ORIGIN' -lm && ./solve
```

The example model also evaluates in double-double (`double_binary64`) through the
number-format tier: a format with no C type, carried as two `double`s per value.

## Producers and consumers

- [ExaModelsCompiler](https://github.com/madsuite-org/ExaModels.jl) compiles
  [ExaModels](https://github.com/madsuite-org/ExaModels.jl) models into cnlp libraries
  ahead of time, optionally with GPU evaluation.
- [CNLPModels.jl](https://github.com/madsuite-org/CNLPModels.jl) (Julia) and
  [cnlpmodels-py](https://github.com/madsuite-org/cnlpmodels-py) (Python) load cnlp
  libraries as NLPModels and Python objects. Their test suites are the conformance tests.
- [exalib](https://github.com/madsuite-org/exalib) installs compiled libraries by name.

The ABI does not depend on any of these: anything that exports the symbols is a cnlp library.

## Prior work

The evaluation semantics follow [NLPModels.jl](https://jso.dev/NLPModels.jl/stable/api/),
and the Lagrangian Hessian convention (objective weight and multipliers, lower triangle)
follows the [AMPL Solver Library](https://www.netlib.org/ampl/solvers/asl.h).
[CUTEst](https://github.com/ralna/CUTEst) compiles test problems into libraries;
[CppADCodeGen](https://github.com/joaoleal/CppADCodeGen) introduced the in-library model
list; [FMI](https://fmi-standard.org) is the analogous standard for simulation models.
Solver C APIs such as Ipopt's define the same callbacks in the other direction, so a cnlp
library binds to them directly.
