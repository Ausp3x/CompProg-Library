---
paths:
  - "98-Team Notebook/**"
---

# Team Notebook

`98-Team Notebook` owns a curated manifest, the generator and selector, and separate contest notes. Algorithm code stays canonical in `01`–`08`: listings include those paths, never diverging copies. Full types, minis and Python are all selectable. Default discovery excludes topic-local `97-Legacy` excerpts, generated expanded solutions and `OLD` (`02-structure.md`, Archive). A selection does not mean an algorithm is verified; show inventory status where practical.

The notebook stays LaTeX, built with LuaLaTeX for its fonts: JetBrains Mono for code, Inter for text. Design B: restrained cyan accents on sections and navigation, thin rules, a clear hierarchy, compact well-aligned columns and a minimalist technical look, with a monochrome print mode. No decorative background or dark fill that harms printing or reading.

A small local browser UI offers algorithm checkboxes (ticking a header also ticks its include dependencies; each printed section names the includes it needs) and controls for order, page budget, paper size, columns and font sizes. It saves a reproducible manifest and configuration; the command line can generate and build too. A local server binds to loopback only and treats selection paths as project-relative canonical files; a submitted configuration can neither traverse paths nor run arbitrary commands. Missing fonts and build prerequisites are reported clearly.

Page budget and fonts are adjustable. A build reports the actual page count and any overrun; it never silently drops selected material or shrinks below the chosen settings. Generated TeX and PDF files stay separate and git-ignored. Contest notes may cover strategy, debugging, complexity estimates, mathematical identities and implementation pitfalls, and keep references apart from unverified folklore. Other team notebooks are researched, minus the exclusions in `09-sources.md`.

The documentation site is generated, like the notebook, from the canonical sources, inventories and evidence documents into the git-ignored `site/`; nothing generated is checked in. Each existing header or module gets one page. Each folder gets an overview, with an include chart for `01`–`07` and a notes page where `Docs/00-notes.md` exists. A hand-written article is `Articles/<NN-Folder>/<NN-stem>.md`, named like the header's evidence document, or `Articles/<NN-Folder>/00-index.md` for the folder, with its images beside it. It holds explanation, arguments, examples, pitfalls and practice lists. Contracts, operations, complexity and status stay in their owning documents and reach the page through the generator. A local link becomes a site link when its target and anchor are on the site, otherwise a repository link. One generated `mkdocs.yml` serves Zensical and MkDocs Material (user decisions 2026-10-25): `build` prefers Zensical and falls back to MkDocs, and its strict build, anchor validation included, must pass on each installed engine. Fonts and scripts are self-hosted, so the built site works offline.
