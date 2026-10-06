---
name: research
description: Completeness research for one library family or folder against the reference catalogs; returns missing operations with sources.
disable-model-invocation: true
argument-hint: "<folder or header, e.g. 04-Graphs or 04-Graphs/07-lca.hpp>"
context: fork
agent: researcher
---

Target: $ARGUMENTS

Run the completeness sweep described in `00-Guidelines/09-sources.md` for the target. If the target is a folder, sweep every row of its `00-index.md`; if it is a header, sweep that row only.

Report in Markdown, grouped by row:

- `row`: header name and current operations.
- `missing`: each operation absent from the row, with the usual algorithm name, one line on why it belongs here, and the source URL where it was found.
- `misplaced`: operations that another folder's inventory already owns, with the owner.
- `vague`: wording in the row that does not name a concrete operation.
- `sources`: every catalog page actually fetched, with the date.

Do not edit files. Keep the report under 1500 words per folder.
