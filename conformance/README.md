# Conformance files

Reference files for cnlp 0.2, for testing producers and consumers against.

- **[fingerprint.py](fingerprint.py)**: the reference implementation of the structure
  fingerprint (formats/solution.md, section 6.3). It needs only Python's standard library:

      python3 fingerprint.py path/to/libqp.so qp 4

- **[qp-4/](qp-4/)**: model `qp` of [example/qp.c](../example/qp.c) at `n = 4`, minimize
  `Σ (xᵢ − 1)²` subject to `x₁ + x₂ = 1`, whose solution is known exactly:
  `x = (0.5, 0.5, 1, 1)`, objective 0.5, constraint multiplier 1 (in the sign convention
  of formats/solution.md, section 2.1), bound multipliers 0.

  | File | Format |
  |---|---|
  | `input.json`, `input.h5` | `cnlp-input`: the instance, `n = 4` |
  | `solution.json`, `solution.h5` | `cnlp-solution`: the solution, with its fingerprint |

A consumer passes when it reads both encodings of each file to the same content,
instantiates `qp` from the input file, computes the same fingerprint, accepts the solution
as belonging to that instance, and writes a solution that reads back equal to this one.
