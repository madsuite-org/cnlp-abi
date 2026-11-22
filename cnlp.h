/* cnlp.h — cnlp, the C ABI for nonlinear programs.
 *
 * Version 0.2 (2026-11-22; 0.1 was 2026-08-14).  THIS HEADER IS THE SPECIFICATION
 * of the interface — there is no separate prose document to drift from it.
 * The files that go with it (instance input, parameter values, solutions)
 * are specified in formats/, in terms of what this header publishes.  The
 * README records how it relates to CUTEst, CppADCodeGen, FMI, ASL and
 * NLPModels.jl.
 * Worked code: example/qp.c + example/solve.c in this repository (one
 * model, both sides).  Reference files and the reference fingerprint
 * implementation are in conformance/; the consumer suites (CNLPModels.jl,
 * cnlpmodels-py) test against them, and their fixture `tinyqp.c` is a
 * fuller multi-model reference.
 *
 * MODEL.  A library carries any number of MODELS, each under a symbol
 * prefix that IS its name.  A model is instantiated into INSTANCES behind
 * positive integer ids; instances are evaluated through sparse first- and
 * second-order callbacks.  Two library-level symbols have fixed names;
 * everything else is per model.  Because symbols are per-prefix, this
 * header declares them through X-macros:
 *
 *     #include "cnlp.h"
 *     CNLP_DECLARE_MODEL(acopf)         // required: core + instantiation
 *         // "##" below is C token pasting: this expands to acopf_new,
 *         // acopf_obj, ... — the exported symbols carry no macro
 *         // machinery.  See example/qp.c for a complete worked model.
 *     CNLP_DECLARE_NEW_STR(acopf)       // if the entry point is a string
 *     CNLP_DECLARE_BUILDER(acopf)       // if the model is structured
 *     CNLP_DECLARE_LAYOUT(acopf)        // if it publishes named blocks
 *     CNLP_DECLARE_SENSE(acopf)         // if it maximizes (0.2)
 *     CNLP_DECLARE_WRITERS(acopf)       // if it carries solution writers (0.2)
 *     CNLP_DECLARE_READERS(acopf)       // if it reads its own input formats (0.2)
 *     CNLP_DECLARE_DEVICE(acopf)        // if it evaluates on a GPU (0.2)
 *     CNLP_DECLARE_SINGLE(acopf)        // if it evaluates in binary32 (0.2)
 *     CNLP_DECLARE_FORMATS(acopf)       // if it evaluates in other formats (0.2)
 *
 * SEMANTICS.  The evaluation surface is a C transliteration of
 * NLPModels.jl's API (https://jso.dev/NLPModels.jl/stable/api/), inheriting
 * that ecosystem's conventions unchanged:
 *
 *   - indices are 1-BASED inside the ABI (Ipopt vocabulary: index_style =
 *     FORTRAN_STYLE); consumers shift to their language's convention;
 *   - sparse structures are COO triplets;
 *   - the Hessian is the LOWER TRIANGLE of
 *         obj_weight * grad^2 f(x)  +  sum_i y_i * grad^2 c_i(x);
 *   - buffers are caller-allocated dense double / int64 arrays;
 *   - every integer in the ABI (ids, sizes, indices, counts, lengths and
 *     returned statuses) is int64_t (0.2; int32_t in 0.1).
 *
 * STATUSES.  Every function returns int64.  0 = success, except the
 * functions documented as returning a positive id, count, or byte length —
 * for those, 0 from an id-returning function means instantiation failure,
 * and -1 means an index out of range.  1 = invalid id, unknown index, or a
 * block of the wrong kind; 3 = a length disagreeing with the addressed
 * object; 4 = a precision the library cannot evaluate at (format tier);
 * any other nonzero from an evaluator is a model-defined failure.
 * Never let an exception or longjmp cross the boundary.
 *
 * STRINGS use one copy-out convention everywhere: the function returns the
 * needed byte length and copies what fits in `cap` — call once with
 * (NULL, 0) to size, once to fill.  UTF-8, not NUL-terminated within the
 * returned length.
 *
 * TIERS.  A library implements an optional tier fully or not at all.
 * Consumers probe ONE witness symbol per tier (noted at each block below)
 * and degrade gracefully: a tier's absence is an answer, never an error.
 *
 * SCOPE.  The ABI publishes only what a library of ANY origin can honestly
 * answer: names, sizes, offsets, dimensions, argument types, descriptions.
 * It deliberately stops short of a model's algebraic structure — an
 * expression tree is a particular modelling package's representation, and
 * asking for one would make this that package's interface in C clothing
 * rather than a generic abstraction over "an NLP behind a C boundary".
 *
 * CANDIDATE EXTENSIONS (absent today, in likely order): Hessian/Jacobian-
 * vector products (P_hprod / P_jprod — consumers currently assemble
 * products from the COO callbacks); constraint linearity/equality
 * classification and integer variables; an explicit reentrancy contract
 * (today: distinct instances from distinct threads is expected to work, a
 * single instance is not promised reentrant); per-block descriptions
 * (P_block_desc).
 */
#ifndef CNLP_H
#define CNLP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Library level (optional; witness: cnlp_nmodels) ──────────────────────
 * What the library carries.  The only fixed-name symbols in the ABI: every
 * per-model entry point needs a prefix, and this is where a prefix comes
 * from.  Model k's name (k in 0..n-1) is returned by the string convention
 * above; the name IS the symbol prefix its functions are exported under. */
int64_t cnlp_nmodels(void);
int64_t cnlp_model_name(int64_t k, uint8_t* buf, int64_t cap);

/* The version of this specification a library implements, as
 * major * 1000 + minor (2 for 0.2).  Optional (0.2; witness: the symbol
 * itself); a library without it implements 0.1. */
int64_t cnlp_abi_version(void);

/* ── Core + instantiation, per prefix P (required) ────────────────────────
 * Exactly ONE of P##_new / P##_new_str / the builder is the model's entry
 * point.  P##_argtype says which, and what to pass, WITHOUT symbol probing:
 * it publishes what the model instantiates FROM as a comma-separated list
 * of types, each optionally `type|description`:
 *
 *     ""                                          fixed; P##_new, arg ignored
 *     "int|size"                                  P##_new(n)
 *     "string|path to a case file"                P##_new_str(s)
 *     "int|arg1,Vector{f64}|v0,Table{i::int w::f64}|bus"    the builder,
 *                                                 one value per schema field
 *
 * P##_nargs is the count of values that close the model (0 = fixed). */
#define CNLP_DECLARE_MODEL(P)                                                 \
    int64_t P##_nargs(void);                                                  \
    int64_t P##_argtype(uint8_t* buf, int64_t cap);                           \
    int64_t P##_new(int64_t n);                  /* -> id > 0; 0 = failure */ \
    /* metadata, per instance id.  P##_meta fills caller buffers of length    \
       nvar (x0, lvar, uvar) and ncon (lcon, ucon); +-INFINITY is fine. */    \
    int64_t P##_nvar(int64_t id);                                             \
    int64_t P##_ncon(int64_t id);                                             \
    int64_t P##_nnzj(int64_t id);                                             \
    int64_t P##_nnzh(int64_t id);                                             \
    int64_t P##_meta(int64_t id, double* x0, double* lvar, double* uvar,      \
                     double* lcon, double* ucon);                             \
    /* evaluation, per instance id; buffer lengths in comments */             \
    int64_t P##_obj (int64_t id, const double* x, double* out);  /* 1     */ \
    int64_t P##_grad(int64_t id, const double* x, double* g);    /* nvar  */ \
    int64_t P##_cons(int64_t id, const double* x, double* c);    /* ncon  */ \
    int64_t P##_jac_structure(int64_t id, int64_t* rows, int64_t* cols);      \
    int64_t P##_jac (int64_t id, const double* x, double* vals); /* nnzj  */ \
    int64_t P##_hess_structure(int64_t id, int64_t* rows, int64_t* cols);     \
    int64_t P##_hess(int64_t id, const double* x, const double* y,            \
                     double obj_weight, double* vals);           /* nnzh  */

/* String entry point — a model instantiated from one string (a case-file
 * path, say).  Declared separately because most models do not have it. */
#define CNLP_DECLARE_NEW_STR(P)                                               \
    int64_t P##_new_str(const char* s);          /* -> id > 0; 0 = failure */

/* ── Structured instantiation (optional per model; witness: P##_schema) ───
 * For models built from structured data.  P##_schema returns JSON:
 *
 *   {"fields":[{"name":...,"kind":"scalar"|"array"|"table",
 *               "type":"i64"|"f64", ...}, ...]}
 *
 * (tables carry "columns").  The consumer opens a builder, sets one value
 * per field BY NAME — tables cross the boundary as columns — and asks for
 * an instance.  Values are checked against the declared kind and type;
 * nothing is coerced.  P##_data_ready returns 1 iff the builder's data is
 * complete and consistent; the library decides, not the consumer. */
#define CNLP_DECLARE_BUILDER(P)                                               \
    int64_t P##_schema(uint8_t* buf, int64_t cap);                            \
    int64_t P##_data_begin(void);                /* -> builder id > 0      */ \
    int64_t P##_set_scalar_i64(int64_t b, const char* field, int64_t v);      \
    int64_t P##_set_scalar_f64(int64_t b, const char* field, double v);       \
    int64_t P##_set_array_i64 (int64_t b, const char* field,                  \
                               const int64_t* v, int64_t len);                \
    int64_t P##_set_array_f64 (int64_t b, const char* field,                  \
                               const double* v, int64_t len);                 \
    int64_t P##_set_col_i64   (int64_t b, const char* table, const char* col, \
                               const int64_t* v, int64_t len);                \
    int64_t P##_set_col_f64   (int64_t b, const char* table, const char* col, \
                               const double* v, int64_t len);                 \
    int64_t P##_data_ready    (int64_t b);       /* 1 iff complete         */ \
    int64_t P##_new_from_data (int64_t b);       /* -> id > 0; 0 = failure */

/* ── Named blocks (optional per model; witness: P##_nblocks) ──────────────
 * The blocks the model's author NAMED — variables, constraints, parameters
 * — so a consumer holding only the library addresses solution slices by
 * name.  P##_block fills out = [kind, offset, length, ndims, dims...] with
 * kind 0 = variable, 1 = constraint, 2 = parameter and offset 0-BASED into
 * the corresponding vector (the solution, the constraint/multiplier vector,
 * or the parameter vector).  The layout is INSTANCE state: a model
 * instantiated at another size reports that size's offsets.  Unnamed blocks
 * still occupy the vectors — offsets account for them — but are not
 * published: the layout answers "what did this model name", not "what does
 * the solution vector contain".
 *
 * Parameters are LIVE, PER-INSTANCE state: P##_set_value takes effect for
 * every later evaluation of that instance, and other instances keep their
 * own values.  get/set on a non-parameter block returns 1; a length
 * disagreeing with the block returns 3. */
#define CNLP_DECLARE_LAYOUT(P)                                                \
    int64_t P##_nblocks(int64_t id);                                          \
    int64_t P##_block(int64_t id, int64_t k, int64_t* out);                   \
    int64_t P##_block_name(int64_t id, int64_t k, uint8_t* buf, int64_t cap); \
    int64_t P##_get_value(int64_t id, int64_t k, double* vals, int64_t len);  \
    int64_t P##_set_value(int64_t id, int64_t k,                              \
                          const double* vals, int64_t len);

/* ── Objective sense (0.2; optional per model; witness: P##_minimize) ─────
 * 1 = minimize, 0 = maximize, -1 = invalid id.  A consumer that does not
 * find it assumes minimize — so a library MUST export it for every model
 * it maximizes, or consumers solve the wrong problem (and report success).
 * P##_obj returns f(x) in the model's own sense, never negated. */
#define CNLP_DECLARE_SENSE(P)                                                 \
    int64_t P##_minimize(int64_t id);

/* ── Solution writers (0.2; optional per model; witness: P##_nwriters) ────
 * Code that turns a solution into the model's own file format (a solved
 * MATPOWER case, say).  Writer k (0-based) has a name, by the string
 * convention; writer "name" is exported as
 *
 *     int64_t P##_write_<name>(int64_t id, const char* path,
 *                              const double* x,  const double* y,
 *                              const double* zl, const double* zu,
 *                              double objective, const char* status);
 *
 * with host arrays of length nvar, ncon, nvar, nvar, and status any string
 * (NULL allowed).  P##_default_writer names the writer a consumer uses when
 * asked for "the" solution file without a format; 0 means none, and the
 * consumer writes the standard cnlp-solution format (formats/solution.md). */
#define CNLP_DECLARE_WRITERS(P)                                               \
    int64_t P##_nwriters(void);                                               \
    int64_t P##_writer_name(int64_t k, uint8_t* buf, int64_t cap);            \
    int64_t P##_default_writer(int64_t id, uint8_t* buf, int64_t cap);

/* ── Readers (0.2; optional per model; witness: P##_nreaders) ─────────────
 * The mirror of writers: the model's own input formats, alongside the
 * standard ones of formats/ (which every consumer can always use).  Reader
 * k (0-based) has a name, by the string convention, and a kind:
 *
 *     P##_reader_kind(k):  0 = instance, 1 = parameters, 2 = start;
 *                          -1 = k out of range.
 *
 * By kind, reader "name" is exported as
 *
 *     instance    int64_t P##_new_from_<name>(const char* path);
 *                 -> id > 0; 0 = failure.  Creates an instance from the
 *                 model's own file (an OPF case, say).
 *     parameters  int64_t P##_set_from_<name>(int64_t id, const char* path);
 *                 -> 0, or a status as for P##_set_value.  Sets parameter
 *                 blocks from the file; live, like P##_set_value.
 *     start       int64_t P##_start_from_<name>(int64_t id, const char* path,
 *                     double* x0, double* y0, double* zl, double* zu);
 *                 -> 0 or a status.  Fills host arrays of length nvar, ncon,
 *                 nvar, nvar with a starting point read from the file, every
 *                 entry written, 0 where the file has no value.
 *
 * Names are unique per model across kinds.  P##_default_reader names the
 * instance reader a consumer uses for a bare path (0 = none); it is
 * library-level, since it is needed before any instance exists.  When a
 * model also exports P##_new_str, the two must agree: P##_new_str is the
 * default instance reader under its 0.1 name. */
#define CNLP_DECLARE_READERS(P)                                               \
    int64_t P##_nreaders(void);                                               \
    int64_t P##_reader_name(int64_t k, uint8_t* buf, int64_t cap);            \
    int64_t P##_reader_kind(int64_t k);                                       \
    int64_t P##_default_reader(uint8_t* buf, int64_t cap);

/* ── Device evaluation (0.2; optional per model; witness: P##_device) ─────
 * Evaluation on a GPU with device pointers in and out.  P##_device(id)
 * returns the memory space the _d callbacks expect: 1 = CUDA, device 0's
 * PRIMARY context (the one the CUDA runtime and CUDA.jl use), -1 = invalid
 * id; 0 is reserved for "no device" and never returned by a library that
 * exports the tier.  Every array argument of a _d callback is a device
 * pointer of the length named in the core tier, except P##_obj_d's `out`:
 * one double in HOST memory.  Structures (P##_jac_structure,
 * P##_hess_structure) and metadata stay host-side; their order is the order
 * of the _d value arrays.  Synchronous: each call returns with its results
 * complete in device memory, ordered after all prior work in the context.
 * Parameters set with P##_set_value take effect for the next _d call.
 * Candidate extensions, signature-preserving: P##_set_device(id, ordinal),
 * P##_set_stream(id, stream) for asynchronous calls on the caller's stream. */
#define CNLP_DECLARE_DEVICE(P)                                                \
    int64_t P##_device(int64_t id);                                           \
    int64_t P##_obj_d (int64_t id, const double* x, double* out);            \
    int64_t P##_grad_d(int64_t id, const double* x, double* g);              \
    int64_t P##_cons_d(int64_t id, const double* x, double* c);              \
    int64_t P##_jac_d (int64_t id, const double* x, double* vals);           \
    int64_t P##_hess_d(int64_t id, const double* x, const double* y,         \
                       double obj_weight, double* vals);

/* ── Single precision (0.2; optional per model; witness: P##_sobj) ─────────
 * The core evaluation callbacks in IEEE binary32 (C `float`), on the same
 * instance as the double ones: instantiation, sizes, structures and P##_meta
 * are shared.  The BLAS convention: the unlettered symbols are the binary64
 * family, `s` the binary32 one.  A library with the device tier may also
 * export the binary32 device callbacks (witness: P##_sobj_d), with the
 * conventions of the device tier. */
#define CNLP_DECLARE_SINGLE(P)                                                \
    int64_t P##_sobj (int64_t id, const float* x, float* out);   /* 1     */ \
    int64_t P##_sgrad(int64_t id, const float* x, float* g);     /* nvar  */ \
    int64_t P##_scons(int64_t id, const float* x, float* c);     /* ncon  */ \
    int64_t P##_sjac (int64_t id, const float* x, float* vals);  /* nnzj  */ \
    int64_t P##_shess(int64_t id, const float* x, const float* y,            \
                      float obj_weight, float* vals);            /* nnzh  */
#define CNLP_DECLARE_SINGLE_DEVICE(P)                                         \
    int64_t P##_sobj_d (int64_t id, const float* x, float* out);             \
    int64_t P##_sgrad_d(int64_t id, const float* x, float* g);               \
    int64_t P##_scons_d(int64_t id, const float* x, float* c);               \
    int64_t P##_sjac_d (int64_t id, const float* x, float* vals);            \
    int64_t P##_shess_d(int64_t id, const float* x, const float* y,          \
                        float obj_weight, float* vals);

/* ── Number formats (0.2; optional per model; witness: P##_nformats) ───────
 * Evaluation in any of the formats of formats/numbers.md: binary128,
 * double-double, quad-double, arbitrary precision, or a format defined
 * elsewhere under a reverse-DNS name.  P##_format_name(id, k, ...) returns
 * the canonical name of format k (k in 0..P##_nformats(id)-1) by the string
 * convention.  An arbitrary-precision entry is listed without its precision
 * (`apfloat`); the caller chooses the precision per call through `prec`, in
 * bits, which every other format ignores (pass 0).
 *
 * The _x callbacks are the core ones with type-erased buffers: every array
 * is a contiguous array of elements of format k, of the length named in the
 * core tier, laid out as formats/numbers.md specifies; `obj_weight` points
 * to one element.  P##_xmeta fills x0, lvar, uvar, lcon, ucon in format k.
 * All buffers belong to the caller.  Status 1 for k out of range, 4 for a
 * `prec` the library cannot evaluate at. */
#define CNLP_DECLARE_FORMATS(P)                                               \
    int64_t P##_nformats(int64_t id);                                         \
    int64_t P##_format_name(int64_t id, int64_t k, uint8_t* buf, int64_t cap); \
    int64_t P##_xmeta(int64_t id, int64_t k, int64_t prec, void* x0,          \
                      void* lvar, void* uvar, void* lcon, void* ucon);         \
    int64_t P##_xobj (int64_t id, int64_t k, int64_t prec, const void* x,     \
                      void* out);                                             \
    int64_t P##_xgrad(int64_t id, int64_t k, int64_t prec, const void* x,     \
                      void* g);                                               \
    int64_t P##_xcons(int64_t id, int64_t k, int64_t prec, const void* x,     \
                      void* c);                                               \
    int64_t P##_xjac (int64_t id, int64_t k, int64_t prec, const void* x,     \
                      void* vals);                                            \
    int64_t P##_xhess(int64_t id, int64_t k, int64_t prec, const void* x,     \
                      const void* y, const void* obj_weight, void* vals);

/* A format name's DLPack description {code, bits, lanes} (formats/numbers.md,
 * section 4), for consumers that exchange arrays with DLPack.  Returns 0 and
 * fills the three values for binary32, binary64 and binary128 (canonical
 * names or aliases), and returns 1 for any other format. */
static inline int64_t cnlp_format_to_dlpack(const char* name, uint8_t* code,
                                            uint8_t* bits, uint16_t* lanes) {
    struct { const char* n; uint8_t b; } t[] = {
        {"binary32", 32}, {"f32", 32}, {"binary64", 64}, {"f64", 64},
        {"binary128", 128}, {"f128", 128}};
    for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++) {
        const char* a = t[i].n; const char* b = name;
        while (*a && *a == *b) { a++; b++; }
        if (*a == 0 && *b == 0) { *code = 2; *bits = t[i].b; *lanes = 1; return 0; }
    }
    return 1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CNLP_H */
