# Implementation batch directory

Use [the copyable prompt](../../01-prompts.md), then read only the selected folder table and its matching inventory. IDs identify ownership; they are not a mandatory execution order. Dependencies take precedence.

| Folder | Batch table | Inventory |
|---|---|---|
| 01-Core | [C batches](01-core.md) | [Feature inventory](../../01-Core/00-index.md) |
| 02-Data Structures | [DS batches](02-data_structures.md) | [Feature inventory](<../../02-Data Structures/00-index.md>) |
| 03-Geometry | [GE batches](03-geometry.md) | [Feature inventory](../../03-Geometry/00-index.md) |
| 04-Graphs | [GR batches](04-graphs.md) | [Feature inventory](../../04-Graphs/00-index.md) |
| 05-Mathematics | [MA batches](05-mathematics.md) | [Feature inventory](../../05-Mathematics/00-index.md) |
| 06-Miscellaneous | [MI batches](06-miscellaneous.md) | [Feature inventory](../../06-Miscellaneous/00-index.md) |
| 07-Strings | [ST batches](07-strings.md) | [Feature inventory](../../07-Strings/00-index.md) |
| 08-Python | [PY batches](08-python.md) | [Feature inventory](../../08-Python/00-index.md) |

[Support/integration batches](09-support.md) cover 09 and 96–99. Resources (95) need no implementation batch.

The map covers **335 batches and 428 numbered algorithm targets**, each assigned once, plus five support passes. The DS shared monoid helper has an explicit owner. Aggregates, tests and provenance travel with their owning changes; they are not extra independent algorithm targets. [Machine-readable target paths](99-batches.json) are for lookup/coverage auditing, not required reading for every task.

For the recommended grouped assignment order, use the **session checklist at the bottom of [01-prompts.md](../../01-prompts.md)**. The smaller IDs above are stable ownership units, not the number of sessions you must schedule yourself. Scope splits are recorded in the JSON map; all original targets retain an owner.
