/* qp.c — a complete cnlp model library in plain C.
 *
 * One size-parametric model under the prefix `qp`:
 *
 *     minimize   sum_i (x_i - 1)^2       over x in R^n
 *     subject to x_1 + x_2 = 1
 *
 * Build:    cc -shared -fPIC -O2 -o libqp.so qp.c
 * Consume:  see solve.c next to this file — or any cnlp consumer:
 *           CNLPModels.jl:  CNLPModel("path/to/libqp.so", 4)
 *           cnlpmodels-py:  cnlpmodels.CModel("path/to/libqp.so", 4)
 *
 * Including cnlp.h and expanding the X-macros first is what buys prototype
 * checking: if a definition below drifts from the ABI, the compiler says so.
 * CNLP_DECLARE_MODEL(qp) expands P##_new, P##_obj, ... to qp_new, qp_obj, ...
 */
#include <math.h>
#include "../cnlp.h"

CNLP_DECLARE_MODEL(qp)

/* Instances are just sizes behind ids; a static table suffices. */
#define QP_MAX_INSTANCES 32
static int64_t qp_sizes[QP_MAX_INSTANCES];
static int64_t qp_count = 0;

static int64_t valid(int64_t id) { return id >= 1 && id <= qp_count; }

/* ── instantiation ──────────────────────────────────────────────────────── */

int64_t qp_nargs(void) { return 1; }             /* one value closes it     */

int64_t qp_argtype(uint8_t* buf, int64_t cap) {  /* "int|size", copy-out    */
    static const char sig[] = "int|size";
    int64_t len = (int64_t)sizeof(sig) - 1;
    for (int64_t i = 0; i < cap && i < len; i++) buf[i] = (uint8_t)sig[i];
    return len;
}

int64_t qp_new(int64_t n) {                      /* -> id > 0; 0 = failure  */
    if (n < 2 || qp_count >= QP_MAX_INSTANCES) return 0;
    qp_sizes[qp_count++] = n;
    return qp_count;
}

/* ── metadata ───────────────────────────────────────────────────────────── */

int64_t qp_nvar(int64_t id) { return valid(id) ? qp_sizes[id - 1] : -1; }
int64_t qp_ncon(int64_t id) { return valid(id) ? 1 : -1; }
int64_t qp_nnzj(int64_t id) { return valid(id) ? 2 : -1; }  /* d(x1+x2)     */
int64_t qp_nnzh(int64_t id) { return valid(id) ? qp_sizes[id - 1] : -1; }

int64_t qp_meta(int64_t id, double* x0, double* lvar, double* uvar,
                double* lcon, double* ucon) {
    if (!valid(id)) return 1;
    int64_t n = qp_sizes[id - 1];
    for (int64_t i = 0; i < n; i++) {
        x0[i] = 0.0;
        lvar[i] = -INFINITY;
        uvar[i] = INFINITY;
    }
    lcon[0] = 1.0;                               /* x1 + x2 = 1: both bounds */
    ucon[0] = 1.0;
    return 0;
}

/* ── evaluation ─────────────────────────────────────────────────────────── */

int64_t qp_obj(int64_t id, const double* x, double* out) {
    if (!valid(id)) return 1;
    int64_t n = qp_sizes[id - 1];
    double f = 0.0;
    for (int64_t i = 0; i < n; i++) f += (x[i] - 1.0) * (x[i] - 1.0);
    *out = f;
    return 0;
}

int64_t qp_grad(int64_t id, const double* x, double* g) {
    if (!valid(id)) return 1;
    int64_t n = qp_sizes[id - 1];
    for (int64_t i = 0; i < n; i++) g[i] = 2.0 * (x[i] - 1.0);
    return 0;
}

int64_t qp_cons(int64_t id, const double* x, double* c) {
    if (!valid(id)) return 1;
    c[0] = x[0] + x[1];
    return 0;
}

/* Indices are 1-based inside the ABI. */
int64_t qp_jac_structure(int64_t id, int64_t* rows, int64_t* cols) {
    if (!valid(id)) return 1;
    rows[0] = 1; cols[0] = 1;                    /* dc1/dx1 */
    rows[1] = 1; cols[1] = 2;                    /* dc1/dx2 */
    return 0;
}

int64_t qp_jac(int64_t id, const double* x, double* vals) {
    (void)x;
    if (!valid(id)) return 1;
    vals[0] = 1.0;
    vals[1] = 1.0;
    return 0;
}

/* Lower triangle of obj_weight*grad^2 f + y1*grad^2 c1.  The objective's
 * Hessian is 2I and the constraint's is zero, so the multiplier drops out —
 * but the signature still takes y, as the ABI requires. */
int64_t qp_hess_structure(int64_t id, int64_t* rows, int64_t* cols) {
    if (!valid(id)) return 1;
    int64_t n = qp_sizes[id - 1];
    for (int64_t i = 0; i < n; i++) { rows[i] = i + 1; cols[i] = i + 1; }
    return 0;
}

int64_t qp_hess(int64_t id, const double* x, const double* y,
                double obj_weight, double* vals) {
    (void)x; (void)y;
    if (!valid(id)) return 1;
    int64_t n = qp_sizes[id - 1];
    for (int64_t i = 0; i < n; i++) vals[i] = 2.0 * obj_weight;
    return 0;
}

/* ── the library-level catalogue (optional tier, three lines here) ──────── */

int64_t cnlp_nmodels(void) { return 1; }

int64_t cnlp_model_name(int64_t k, uint8_t* buf, int64_t cap) {
    if (k != 0) return -1;
    static const char nm[] = "qp";
    int64_t len = (int64_t)sizeof(nm) - 1;
    for (int64_t i = 0; i < cap && i < len; i++) buf[i] = (uint8_t)nm[i];
    return len;
}
