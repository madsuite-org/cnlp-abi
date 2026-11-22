# cnlp-parameters: values of a model's parameters

Version 0.2. Shared conventions (encodings, identification, arrays):
[README.md](README.md).

Parameters are the values a model is evaluated at that can change without
re-instantiating it: the layout tier's parameter blocks (kind 2), read and written with
`P_get_value` and `P_set_value`. Their structure (sizes, sparsity) never changes, so a
sweep sets parameters and solves again, reusing the instance.

Parameters are distinct from input ([input.md](input.md)): input decides what instance
exists, possibly changing its size and structure; parameters change values inside it.

## 1. Content

The section `/parameters` (HDF5 group, or JSON member `parameters`) holds one entry per
parameter block being set, named by the block's name (`P_block_name`).

| Encoding | Entry |
|---|---|
| HDF5 | a dataset `/parameters/<name>` with the block's values, and the block's `dims` as an attribute |
| JSON | `"<name>": {"dims": [...], "value": [...]}`, with `value` flat in column-major order |

A file need not set every parameter block. Blocks it omits keep the values they have.

## 2. Applying

A consumer applying a parameter file to an instance, for each entry:

1. finds the block by name among the instance's blocks (`P_nblocks`, `P_block_name`);
2. checks that it is a parameter block (kind 2 in `P_block`), and that its length and
   `dims` match the entry's;
3. calls `P_set_value(id, k, values, length)`.

Every entry is checked before any is applied, so a bad file leaves the instance
unchanged. The checks are those `P_set_value` itself applies (1 for a block that is not a
parameter, 3 for a wrong length), done up front for the whole file. A parameter set takes
effect for the next evaluation, through every tier including the device tier.

## 3. Writing

A writer records an instance's current parameters by reading each parameter block with
`P_get_value`. A solution file records the parameters it was computed with this way, in
its own `/parameters` section ([solution.md](solution.md)).

## 4. Example

```json
{
  "format": "cnlp-parameters", "format_version": "0.2", "model": "pm",
  "parameters": {
    "w": {"dims": [3], "value": [3.0, 1.0, 1.0]}
  }
}
```
