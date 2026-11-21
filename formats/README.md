# cnlp data formats

Draft for version 0.2 of the cnlp ABI.

`cnlp.h` specifies how a program talks to a model library. These documents specify the
files that go with it: what a model is instantiated from, the parameter values it is
evaluated at, and the solutions computed for it. Any consumer of a cnlp library can read
and write them using only what the ABI publishes, so a file written by one consumer
(CNLPModels.jl, cnlpmodels-py, a command-line solver) is read by any other.

| Format | Document | Holds | ABI it maps to |
|---|---|---|---|
| `cnlp-input` | [input.md](input.md) | the values a model is instantiated from | `P_new`, `P_new_str`, the builder tier |
| `cnlp-parameters` | [parameters.md](parameters.md) | parameter values, by block name | the layout tier, `P_set_value` |
| `cnlp-solution` | [solution.md](solution.md) | a solution and its multipliers | the core and layout tiers |

A solution file is also a valid warm start: its arrays are the initial point and
multipliers of a new solve (solution.md, section 6).

## Conventions shared by all three

### Encodings

Each format has two encodings carrying the same content.

| Encoding | Use | Extension |
|---|---|---|
| HDF5 | the primary encoding: binary, fast, one file | `.h5` |
| JSON | readable, for small files and hand-written inputs | `.json` |

A reader must accept both. A writer should produce HDF5 unless asked otherwise. For a
million double-precision values, HDF5 is written in milliseconds; JSON takes about a
hundred times longer and is over twice the size.

### Identification

Every file carries, in HDF5 as attributes of the root group and in JSON as top-level
members:

| Field | Type | Meaning |
|---|---|---|
| `format` | string | the format of the main section: `"cnlp-input"`, `"cnlp-parameters"` or `"cnlp-solution"` |
| `format_version` | string | `"MAJOR.MINOR"` of these documents; this draft is `"0.2"` |
| `library` | string | optional: the library the model comes from |
| `model` | string | optional: the model name, which is its symbol prefix |

A reader rejects a file whose major version it does not know. It may read a newer minor
version, ignoring members it does not recognize.

### Sections and composition

Each format's content lives in one section: `/input`, `/parameters` or `/solution` in
HDF5, and members of the same names in JSON. A file may carry more than one section. A
complete instance is input plus parameters, and a solution file may record the
parameters it was solved with. A reader uses the sections it needs and ignores the rest.
`format` names the section the file is primarily about.

### Names

Every name in a file comes from the ABI and is never invented by the writer:

- input fields are the names in the model's schema (`P_schema`);
- blocks are the names the layout tier publishes (`P_block_name`).

### Arrays

- Floating-point values are IEEE 754 binary64, integers are signed 64-bit.
- A block's array is stored with its shape, in **column-major** order: the first index
  varies fastest, as in Fortran, Julia and MATLAB. `dims` always gives the shape in that
  order. A row-major reader (NumPy, C) sees the shape reversed and reads with
  `order="F"` or transposes.
- In JSON every array is flat, alongside its `dims`. Non-finite numbers, which JSON cannot
  represent, are written as `null`.

### Reading against a model

A reader given a model checks a file before using it: the names it refers to exist with
the right kind, type and length, and, for a solution, the sizes match the model's. A
failed check is an error. A consumer may offer an explicit override for deliberate use on
a different instance.

## Conformance

Reference files for each format and encoding live in [conformance/](../conformance/).
The consumer packages' test suites read and write them. As with the ABI, those test
suites are the conformance tests.
