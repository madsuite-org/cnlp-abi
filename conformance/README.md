# Conformance

- [qp-4/](qp-4/): `example/qp.c` at `n = 4` (`min Σ(xᵢ − 1)²  s.t.  x₁ + x₂ = 1`), its
  input and exact solution (`x = (0.5, 0.5, 1, 1)`, `f = 0.5`, `y = 1`, `zL = zU = 0`), in
  JSON and HDF5.
- [check.py](check.py): builds the example and checks these files against the spec and the
  library: `python3 conformance/check.py` (HDF5 checks need `h5py`).
