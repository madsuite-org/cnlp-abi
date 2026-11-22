#!/usr/bin/env python3
"""Check the conformance files against the spec and against the example library.

    python3 conformance/check.py

Builds example/qp.c, runs example/solve.c, then checks conformance/qp-4: the files'
identification and shapes, the input instantiates the model, the solution is the model's
(objective, feasibility, stationarity in the sign convention of formats/solution.md,
fingerprint), and each HDF5 file holds the same content as its JSON twin (needs h5py).
Exits nonzero on any failure.
"""
import ctypes, json, math, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from fingerprint import fingerprint  # noqa: E402

I64, F64 = ctypes.c_int64, ctypes.c_double
STATUSES = {"solved", "acceptable", "infeasible", "unbounded", "iteration_limit", "time_limit", "error"}
failures, passed = [], 0


def check(cond, what):
    global passed
    if cond:
        passed += 1
    else:
        failures.append(what)
        print("FAIL", what)


def build(tmp):
    lib, exe = os.path.join(tmp, "libqp.so"), os.path.join(tmp, "solve")
    ex = os.path.join(ROOT, "example")
    subprocess.run(["cc", "-std=c99", "-Wall", "-Werror", "-O2", "-shared", "-fPIC", "-o", lib,
                    os.path.join(ex, "qp.c"), "-lm"], check=True)
    subprocess.run(["cc", "-std=c99", "-Wall", "-Werror", "-O2", "-o", exe, os.path.join(ex, "solve.c"),
                    "-L" + tmp, "-lqp", "-Wl,-rpath," + tmp, "-lm"], check=True)
    check(subprocess.run([exe], capture_output=True).returncode == 0, "example/solve runs and checks its results")
    return ctypes.CDLL(lib)


def fn(lib, name, *args):
    f = getattr(lib, name)
    f.restype, f.argtypes = I64, list(args)
    return f


def check_ident(d, fmt, where):
    check(d.get("format") == fmt, f"{where}: format is {fmt}")
    check(d.get("format_version") == "0.2", f"{where}: format_version is 0.2")


def check_solution(lib, d, where):
    check_ident(d, "cnlp-solution", where)
    iid = fn(lib, "qp_new", I64)(4)
    n, m = fn(lib, "qp_nvar", I64)(iid), fn(lib, "qp_ncon", I64)(iid)
    check((d.get("nvar"), d.get("ncon")) == (n, m), f"{where}: nvar, ncon match the model")
    s = d["solution"]
    for k, L in (("solution", n), ("multipliers", m), ("multipliers_L", n), ("multipliers_U", n)):
        check(len(s[k]) == L, f"{where}: {k} has length {L}")
    check(s["status"] in STATUSES, f"{where}: status is solver-neutral")
    check(isinstance(s["minimize"], bool), f"{where}: minimize is a boolean")
    check(d.get("fingerprint") == fingerprint(lib, "qp", iid), f"{where}: fingerprint matches the model")

    vec = lambda v: (F64 * len(v))(*v)
    x, y, zl, zu = (vec(s[k]) for k in ("solution", "multipliers", "multipliers_L", "multipliers_U"))
    f = F64(); fn(lib, "qp_obj", I64, ctypes.POINTER(F64), ctypes.POINTER(F64))(iid, x, ctypes.byref(f))
    check(abs(f.value - s["objective"]) <= 1e-12, f"{where}: objective = f(x)")
    x0, lv, uv, lc, uc = (F64 * n)(), (F64 * n)(), (F64 * n)(), (F64 * m)(), (F64 * m)()
    fn(lib, "qp_meta", I64, *[ctypes.POINTER(F64)] * 5)(iid, x0, lv, uv, lc, uc)
    c = (F64 * m)(); fn(lib, "qp_cons", I64, ctypes.POINTER(F64), ctypes.POINTER(F64))(iid, x, c)
    check(all(lc[i] - 1e-12 <= c[i] <= uc[i] + 1e-12 for i in range(m)), f"{where}: constraints hold")
    check(all(lv[i] <= x[i] <= uv[i] for i in range(n)), f"{where}: bounds hold")
    check(min(list(zl) + list(zu)) >= 0, f"{where}: bound multipliers are nonnegative")
    # sigma*grad f + J'y - zL + zU = 0
    g = (F64 * n)(); fn(lib, "qp_grad", I64, ctypes.POINTER(F64), ctypes.POINTER(F64))(iid, x, g)
    nnz = fn(lib, "qp_nnzj", I64)(iid)
    rows, cols, vals = (I64 * nnz)(), (I64 * nnz)(), (F64 * nnz)()
    fn(lib, "qp_jac_structure", I64, ctypes.POINTER(I64), ctypes.POINTER(I64))(iid, rows, cols)
    fn(lib, "qp_jac", I64, ctypes.POINTER(F64), ctypes.POINTER(F64))(iid, x, vals)
    sigma = 1.0 if s["minimize"] else -1.0
    r = [sigma * g[i] - zl[i] + zu[i] for i in range(n)]
    for k in range(nnz):
        r[cols[k] - 1] += vals[k] * y[rows[k] - 1]
    check(max(abs(v) for v in r) <= 1e-12, f"{where}: stationarity in the cnlp sign convention")


def h5_to_dict(path):
    import h5py
    out = {}
    with h5py.File(path, "r") as f:
        for k, v in f.attrs.items():
            out[k] = v.decode() if isinstance(v, bytes) else (v.item() if hasattr(v, "item") else v)
        for sec in f:
            out[sec] = {}
            for k, ds in f[sec].items():
                v = ds[()]
                out[sec][k] = v.decode() if isinstance(v, bytes) else (v.tolist() if hasattr(v, "tolist") else v)
    return out


def same(a, b):
    if isinstance(a, dict) and isinstance(b, dict):
        return a.keys() == b.keys() and all(same(a[k], b[k]) for k in a)
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(same(u, v) for u, v in zip(a, b))
    if isinstance(a, bool) or isinstance(b, bool):
        return int(a) == int(b)
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return a == b or (math.isnan(a) and math.isnan(b))
    return a == b


def main():
    d = os.path.join(HERE, "qp-4")
    with tempfile.TemporaryDirectory() as tmp:
        lib = build(tmp)
        inp = json.load(open(os.path.join(d, "input.json")))
        check_ident(inp, "cnlp-input", "input.json")
        check(fn(lib, "qp_new", I64)(inp["input"]["n"]) > 0, "input.json: instantiates qp")
        check_solution(lib, json.load(open(os.path.join(d, "solution.json"))), "solution.json")
        try:
            import h5py  # noqa: F401
        except ImportError:
            check(False, "HDF5 files: h5py is not installed")
        else:
            for name in ("input", "solution"):
                j = json.load(open(os.path.join(d, name + ".json")))
                check(same(h5_to_dict(os.path.join(d, name + ".h5")), j), f"{name}.h5 holds the content of {name}.json")
    print(f"{passed} passed, {len(failures)} failed")
    sys.exit(1 if failures or passed == 0 else 0)


if __name__ == "__main__":
    main()
