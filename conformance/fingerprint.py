#!/usr/bin/env python3
"""Reference implementation of the cnlp structure fingerprint (formats/solution.md, 6.3).

    python3 fingerprint.py LIBRARY MODEL [ARG]

Instantiates MODEL from LIBRARY (with P_new(ARG) for an integer ARG, P_new_str(ARG)
otherwise, P_new(0) without one) and prints its fingerprint. Uses only the standard
library (ctypes, hashlib, struct).
"""
import ctypes, hashlib, struct, sys

I64 = ctypes.c_int64


def fingerprint(lib, p, iid):
    def f(name, restype=I64, *argtypes):
        fn = getattr(lib, p + "_" + name)
        fn.restype, fn.argtypes = restype, list(argtypes)
        return fn

    out = bytearray(b"cnlp-fingerprint/1")
    put = lambda *vs: out.extend(struct.pack("<%dq" % len(vs), *vs))
    nvar, ncon = f("nvar", I64, I64)(iid), f("ncon", I64, I64)(iid)
    nnzj, nnzh = f("nnzj", I64, I64)(iid), f("nnzh", I64, I64)(iid)
    put(nvar, ncon)
    for nnz, name in ((nnzj, "jac_structure"), (nnzh, "hess_structure")):
        rows, cols = (I64 * nnz)(), (I64 * nnz)()
        f(name, I64, I64, ctypes.POINTER(I64), ctypes.POINTER(I64))(iid, rows, cols)
        put(nnz, *rows, *cols)
    if hasattr(lib, p + "_nblocks"):
        nb = f("nblocks", I64, I64)(iid)
        put(nb)
        block = f("block", I64, I64, I64, ctypes.POINTER(I64))
        bname = f("block_name", I64, I64, I64, ctypes.c_char_p, I64)
        for k in range(nb):
            buf = (I64 * 64)()
            block(iid, k, buf)
            ndims = buf[3]
            put(*buf[: 4 + ndims])
            n = bname(iid, k, None, 0)
            s = ctypes.create_string_buffer(n)
            bname(iid, k, s, n)
            put(n)
            out.extend(s.raw[:n])
    else:
        put(0)
    return "sha256:" + hashlib.sha256(bytes(out)).hexdigest()


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    lib, p = ctypes.CDLL(argv[1]), argv[2]
    if len(argv) > 3 and not argv[3].lstrip("-").isdigit():
        new = getattr(lib, p + "_new_str"); new.restype, new.argtypes = I64, [ctypes.c_char_p]
        iid = new(argv[3].encode())
    else:
        new = getattr(lib, p + "_new"); new.restype, new.argtypes = I64, [I64]
        iid = new(int(argv[3]) if len(argv) > 3 else 0)
    if iid <= 0:
        sys.exit("instantiation failed")
    print(fingerprint(lib, p, iid))


if __name__ == "__main__":
    main(sys.argv)
