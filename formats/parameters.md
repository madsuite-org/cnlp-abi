# cnlp-parameters: values of a model's parameters

Version 0.2. Conventions: [README.md](README.md).

Parameters are the layout tier's parameter blocks (kind 2): values changed in place with
`P_set_value`, without re-instantiating or changing structure.

`/parameters` holds one entry per block being set, named by `P_block_name`:

| Encoding | Entry |
|---|---|
| HDF5 | dataset `/parameters/<name>`, with `dims` as an attribute |
| JSON | `"<name>": {"dims": [...], "value": [...]}` |

Omitted blocks keep their values.

**Applying.** For every entry, find the block by name and check it is a parameter block
with matching length and `dims`; only then call `P_set_value` for each. A bad file leaves
the instance unchanged. New values apply to the next evaluation, in every tier.

**Writing.** Read each parameter block with `P_get_value`. A solution file records its
parameters this way in its own `/parameters` section.

```json
{"format": "cnlp-parameters", "format_version": "0.2", "model": "pm",
 "parameters": {"w": {"dims": [3], "value": [3.0, 1.0, 1.0]}}}
```
