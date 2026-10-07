# Re-audit findings

Confirmed defects from the read-only `/reaudit-review` of the Foundations re-audit packages (2026-10-06, 64 headers, 172 findings raised, 168 confirmed, 4 rejected). `plan.py show Pxxx` names the file for each package in its note; `/package Pxxx` fixes the findings as part of the re-audit. P001 and P003 own no headers and have no file. [findings.json](findings.json) holds every finding with the full skeptic reasons and the rejected list; query it with `rg` or Python.

| Package | Confirmed | Correctness | Optimality | Completeness | Test gaps | Portability | Style |
|---|---|---|---|---|---|---|---|
| [P015](P015.md) | 9 | 0 | 0 | 2 | 0 | 0 | 7 |
| [P016](P016.md) | 17 | 3 | 0 | 1 | 2 | 0 | 11 |
| [P017](P017.md) | 10 | 2 | 0 | 1 | 0 | 0 | 7 |
