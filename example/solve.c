/* solve.c — a plain-C consumer of the model in qp.c.
 *
 * Linking against the library and declaring the symbols through cnlp.h is
 * all it takes — no dlopen needed when the prefix is known at compile time:
 *
 *     cc -shared -fPIC -O2 -o libqp.so qp.c
 *     cc -O2 -o solve solve.c -L. -lqp -Wl,-rpath,'$ORIGIN' -lm
 *     ./solve
 *
 * (A consumer that discovers models at run time uses dlopen/dlsym with the
 * names from cnlp_model_name instead — that is what the Julia and Python
 * consumers do.)
 */
#include <stdio.h>
#include <stdlib.h>
#include "../cnlp.h"

CNLP_DECLARE_MODEL(qp)

int main(void) {
    /* Instantiate at a size of our choosing — the library was compiled
     * knowing no particular n. */
    int32_t id = qp_new(4);
    if (id <= 0) { fprintf(stderr, "qp_new failed\n"); return 1; }

    int32_t n = qp_nvar(id), m = qp_ncon(id);
    printf("qp instance %d: n = %d, m = %d\n", id, n, m);

    double *x0 = malloc(n * sizeof *x0), *lv = malloc(n * sizeof *lv),
           *uv = malloc(n * sizeof *uv), lc[1], uc[1];
    if (qp_meta(id, x0, lv, uv, lc, uc) != 0) return 1;

    /* Evaluate at the optimum x* = (0.5, 0.5, 1, 1): f = 0.5, c = 1. */
    double xs[] = { 0.5, 0.5, 1.0, 1.0 };
    double f, c[1], *g = malloc(n * sizeof *g);
    qp_obj(id, xs, &f);
    qp_cons(id, xs, c);
    qp_grad(id, xs, g);
    printf("f(x*) = %g   c(x*) = %g   g(x*) = [%g %g %g %g]\n",
           f, c[0], g[0], g[1], g[2], g[3]);

    free(x0); free(lv); free(uv); free(g);
    return f == 0.5 && c[0] == 1.0 ? 0 : 1;
}
