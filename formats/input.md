# cnlp-input: what a model is instantiated from

Version 0.2. Shared conventions (encodings, identification, arrays):
[README.md](README.md).

A model is instantiated through exactly one entry point, and `P_argtype` says which.
An input file holds the values for that entry point, so that any consumer can create an
instance from a file instead of from code.

## 1. Content

The section `/input` (HDF5 group, or JSON member `input`) holds one entry per value the
model is instantiated from.

| Entry point (`P_argtype`) | `/input` holds |
|---|---|
| fixed model (`""`) | nothing; the section may be absent |
| `P_new(n)` (`"int|…"`) | one integer, named `n` |
| `P_new_str(s)` (`"string|…"`) | one string, named `s`. A path is relative to the input file's directory. |
| the builder (`P_schema`) | one entry per schema field, named as in the schema |

### 1.1 Builder fields

The schema (`P_schema`) lists the fields as
`{"fields":[{"name":…,"kind":"scalar"|"array"|"table", …}]}`. A file stores each field
by its kind:

| Schema kind | HDF5 | JSON |
|---|---|---|
| `scalar`, `type` i64 or f64 | a scalar dataset `/input/<name>` | a number |
| `array`, `type` i64 or f64 | a one-dimensional dataset `/input/<name>` | an array of numbers |
| `table`, `columns` `[{name, type}]` | a group `/input/<name>` with one dataset per column, all of equal length | an object with one array per column, all of equal length |

The file's types must be the schema's. An `i64` field holds integers, and nothing is
converted, exactly as the builder's setters check.

## 2. Reading

A consumer instantiating a model from an input file:

1. reads the model's entry point (`P_argtype`) and, for the builder, its schema;
2. checks that the file has exactly the entries the entry point needs: every schema field
   present with the right kind and type, every table's columns present and of equal
   length, and nothing else;
3. calls the entry point: `P_new(n)`, `P_new_str(s)`, or, for the builder, `P_data_begin`,
   then one `P_set_*` per field (tables column by column), `P_data_ready`, `P_new_from_data`.

A file that fails a check is an error, reported before any instance is created. The
library has the last word: `P_data_ready` may still reject data that is well-formed but
inconsistent.

## 3. Writing

A writer given the values a model was (or will be) instantiated with writes them under the
names and types of the model's schema. A writer for the builder obtains the schema from the
library, never from the modelling tool, so the file matches what the library accepts.

## 4. Example

A model whose schema is
`{"fields":[{"name":"N","kind":"scalar","type":"i64"},{"name":"bus","kind":"table","columns":[{"name":"i","type":"i64"},{"name":"pd","type":"f64"}]}]}`
is instantiated from this JSON file:

```json
{
  "format": "cnlp-input", "format_version": "0.2", "library": "grid", "model": "acopf",
  "input": {
    "N": 3,
    "bus": {"i": [1, 2, 3], "pd": [0.0, 0.9, 1.2]}
  }
}
```

In HDF5 this is `/input/N` (a scalar) and `/input/bus/i`, `/input/bus/pd` (datasets of
length 3).
