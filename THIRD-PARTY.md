# Third-party material

[LICENSE](LICENSE) (CC0-1.0) covers the original work in this repository. It does not cover:

- `95-Resources/`: reference documents by other authors, kept for study under their own terms; no licence is granted for them.
- `OLD/`: the archived originals, including the co-authored team notebook; no licence is granted for them.
- The third-party-derived code below, which keeps its source's licence. Each file opens with a one-line notice pointing here.

| File | Derived part | Source | Licence |
|---|---|---|---|
| `98-Team Notebook/06-site.py` | `Emphasis` and `Heading` classes, heading ids, the table-cell rule, the `TICK`, `CODE_PIPES`, `TAG`, `AUTOLINK` and `KEY` patterns (ported) | [Python-Markdown](https://github.com/Python-Markdown/markdown) 3.11 | BSD-3-Clause |
| `98-Team Notebook/06-site.py` | `MATH` (verbatim), `FENCE` and `OPTION` (adapted) | [pymdown-extensions](https://github.com/facelessuser/pymdown-extensions) `arithmatex`, `superfences` | MIT |
| `98-Team Notebook/06-site.py` | the `MATHJAX` configuration (adapted) | [Material for MkDocs](https://github.com/squidfunk/mkdocs-material) documentation, `reference/math.md` | MIT |
| `96-Local Testing/00-Tools/06-site_tester.py` | the `BACKTICK` pattern, `code_pass` and the heading-id oracle | Python-Markdown | BSD-3-Clause |
| `96-Local Testing/04-Graphs/35-subgraph_enumeration_tester.cpp` | the `TRICODES` and triad-name tables (verbatim), the `cluster.py` oracles (transliterated) | [NetworkX](https://github.com/networkx/networkx) `triads.py`, `cluster.py` | BSD-3-Clause |

The SplitMix64 finalizer used across headers and tests (`06-Miscellaneous/02-customhash.hpp` and others) follows [splitmix64.c](https://prng.di.unimi.it/splitmix64.c), which Sebastiano Vigna dedicated to the public domain in 2015, so it needs no exclusion. Provenance audit and method: [00-contribution.md](00-Guidelines/13-Plan/Docs/00-contribution.md).

## Python-Markdown (BSD-3-Clause)

```text
Copyright 2007, 2008 The Python Markdown Project (v. 1.7 and later)
Copyright 2004, 2005, 2006 Yuri Takhteyev (v. 0.2-1.6b)
Copyright 2004 Manfred Stienstra (the original version)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## pymdown-extensions (MIT)

```text
The MIT License (MIT)

Copyright (c) 2014 - 2025 Isaac Muse

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

The `FENCE` and `OPTION` patterns come from `superfences`, whose notice reads:

> `superfences.py` is derived from Python Markdown's fenced_code extension.

```
Fenced Code Extension for Python Markdown
 =========================================
This extension adds Fenced Code Blocks to Python-Markdown.
See <https://python-markdown.github.io/extensions/fenced_code_blocks/>
for documentation.
Original code Copyright 2007-2008 [Waylan Limberg](https://github.com/waylan).
All changes Copyright 2008-2014 The Python Markdown Project
License: [BSD](http://www.opensource.org/licenses/bsd-license.php)
```

## Material for MkDocs (MIT)

```text
Copyright (c) 2016-2025 Martin Donath <martin.donath@squidfunk.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
```

## NetworkX (BSD-3-Clause)

```text
Copyright (c) 2004-2026, NetworkX Developers
Aric Hagberg <hagberg@lanl.gov>
Dan Schult <dschult@colgate.edu>
Pieter Swart <swart@lanl.gov>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

  * Neither the name of the NetworkX Developers nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
