# cnlp data formats

Version 0.2. The files that go with a cnlp library, defined only through what `cnlp.h`
publishes, so any consumer reads what another wrote.

| Format | Document | Holds |
|---|---|---|
| `cnlp-input` | [input.md](input.md) | the values a model is instantiated from |
| `cnlp-parameters` | [parameters.md](parameters.md) | parameter values, by block name |
| `cnlp-solution` | [solution.md](solution.md) | a solution and its multipliers; also a warm start |

[numbers.md](numbers.md) defines the number formats a model can be evaluated in.

## Conventions

**Encodings.** HDF5 (`.h5`, primary) and JSON (`.json`), same content. Readers accept
both; writers produce HDF5 unless asked otherwise.

**Identification.** Root attributes (HDF5) or top-level members (JSON):

| Field | Meaning |
|---|---|
| `format` | `"cnlp-input"`, `"cnlp-parameters"` or `"cnlp-solution"` |
| `format_version` | `"0.2"`; a reader rejects an unknown major version and ignores unknown members of a newer minor one |
| `library`, `model` | optional: the library, and the model's symbol prefix |

**Sections.** Content lives in `/input`, `/parameters` or `/solution` (HDF5 groups, JSON
members). A file may carry several; readers ignore sections they do not need.

**Names** come from the ABI: input fields from `P_schema`, blocks from `P_block_name`.

**Values.** Floats are binary64, integers int64, booleans `true`/`false` (JSON) or int8
0/1 (HDF5). A block is stored with its `dims`, column-major (first index fastest). In JSON
arrays are flat and non-finite numbers are `null`.

**Reading against a model.** Names must exist with the right kind, type and length, and a
solution's sizes must match the model's. A failed check is an error unless the consumer
offers an explicit override.

## Model-specific formats

A library may read and write its own formats (an OPF model reads MATPOWER cases), through
the readers and writers tiers: `P_new_from_<name>`, `P_set_from_<name>`,
`P_start_from_<name>`, `P_write_<name>`. They add to the standard formats and never
replace them; `P_default_reader` / `P_default_writer` name the default. Consumers write a
model format as `name:path`.

Reference files: [conformance/](../conformance/).
