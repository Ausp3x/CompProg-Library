---
paths:
  - "98-Team Notebook/**"
---

# Team notebook

98-Team Notebook owns a curated manifest, generator/selector and separate contest notes. Canonical algorithm files remain in 01–08; include listings from those paths instead of maintaining divergent implementation copies. Full types, minis and Python are all selectable. Default discovery excludes topic-local 97-Legacy excerpts and generated expanded solutions, and `OLD` (`02-structure.md`, Archive). User selection does not imply an algorithm has been verified; show inventory status where practical.

Retain LaTeX; use LuaLaTeX for fonts. Code: JetBrains Mono. Body: Inter. Design B: restrained cyan section/navigation accents, thin rules, clear hierarchy, compact well-aligned columns and a minimalist technical appearance. Provide monochrome print mode. No decorative background or dark fill that harms printing/readability.

A small local browser UI provides algorithm checkboxes (ticking a header also ticks its include dependencies; each printed section names the includes it needs) and controls for order, page budget, paper size, columns and font sizes. Save a reproducible manifest/config; allow CLI generation/build too. Bind any local server to loopback; treat selection paths as project-relative canonical files. Do not allow traversal or arbitrary server-side commands from submitted configuration. Fonts/build prerequisites must be reported clearly.

Page budget and font constraints are adjustable. Report actual page count and an over-budget result after build; never silently drop selected material or shrink below chosen settings. Keep generated tex/PDF/LaTeX artifacts separate/ignored. Contest notes may cover strategy, debugging, complexity estimates, mathematical identities and implementation pitfalls; distinguish references from unverified folklore. Research other team notebooks, minus the exclusions in `09-sources.md`.

The documentation site is generated, like the notebook, from the canonical sources, inventories and evidence documents into the git-ignored `site/`; nothing generated is checked in. Each existing header or module gets one page. Each folder gets an overview, with an include chart for `01`–`07` and a notes page where `Docs/00-notes.md` exists. A hand-written article is `Articles/<NN-Folder>/<NN-stem>.md`, named like the header's evidence document, or `Articles/<NN-Folder>/00-index.md` for the folder, with its images beside it. It holds explanation, arguments, examples, pitfalls and practice lists. Contracts, operations, complexity and status stay in their owning documents and reach the page through the generator. A local link becomes a site link when its target and anchor are on the site, otherwise a repository link. One generated `mkdocs.yml` serves Zensical and MkDocs Material (user decisions 2026-10-25): `build` prefers Zensical and falls back to MkDocs, and its strict build, anchor validation included, must pass on each installed engine. Fonts and scripts are self-hosted, so the built site works offline.
