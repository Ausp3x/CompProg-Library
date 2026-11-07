#!/usr/bin/env python3
"""Checks for the concept search of 09-Contest Testing/_11_search.py and its CLI in 09-Contest Testing/10-grab.py
(`--find WORDS`, `-n K`, the closest rows appended to grab's unknown-name errors).

Fixture inventories (00-index.md tables, Docs evidence files) are written to per-process temporary directories; the
real repository is only read. quick: every fixture test, the repository stem check and the corpus metrics, with 400
random distance cases, 200 segment cases, 300 random identifiers for words, and one generated typo set (seed --seed)
over the cp queries. full: quick with 4000 distance, 2000 segment and 3000 word cases, and one generated set over the
lc and cp queries. stress: full with --rounds (default 3) times the full random cases and --rounds generated sets
(seeds --seed .. --seed + rounds - 1) over lc and cp. Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED.
Oracles (written here, independent of the module): hand tables for names, aliases, rows and stem; a character scanner
for words; a memoized recursion of the restricted Damerau (optimal string alignment) definition for distance, itself
checked on known values; an enumeration of every split into parts of PART letters or more for segment; BM25F recomputed
from its formula over hand-written field token lists; hand-derived orders on fixtures whose scores run against the name
classes; an independent table-line count of the inventories; recall@5 and MRR@10 per corpus source against FLOORS.
Failing random rounds print the seed, mode, round and input.

Feature map:
cells, split, operations (moved parser, one case each) -> test_cells_split_operations;
names (type heads and Type::member, ctor, alias heads, camelCase function head, label head, `A as B and C`, operators,
template arguments, duplicates, order) -> test_names;
rows (0N-Folder/00-index.md only, tier from ## sections, 4-column 01-Core Basic cell starting with
the word yes -> Basic, else Advanced whatever the section, module evidence documents Docs/<module stem>.md, 08-Python modules, status
prefix, stem field, aliases only from <folder>/Docs/<NN-name>.md, folder field) -> test_rows_fixture;
aliases (sections, Aliases:/Catalog aliases:/dated/bold/Search aliases/list lines, labels dropped also in sections, not
`with \\`FIXED\\``, not ordinary paragraphs, URLs and link targets dropped) -> test_aliases;
words -> test_words_table, test_words_random; stem -> test_stem_table; distance -> test_distance_known,
test_distance_random; segment -> test_segment_table, test_segment_random; tolerance -> test_tolerance;
BM25F score (default and custom weights, b, k1) -> test_bm25_one_word; name classes (exact, last :: component, header
stem for any query, whole-word runs for one-word queries only, not `sam` in `same`, concept) -> test_classes_*; status
and tier priors (custom, missing keys, defaults, order flips in and outside the name classes) -> test_priors; tie order
and n truncation -> test_tie_order;
tuned constants pinned against TUNED -> test_tuned_constants; entries and notes -> test_entries_notes;
typo (factor at distance 1 and 2, 3-letter tokens) -> test_typo; typo and prefix together (the larger factor) ->
test_typo_and_prefix; surrounding whitespace -> test_strip; prefix -> test_prefix; joins (exact, misspelt, no
prefix at the default factor) -> test_joins; stem segmentation (known words stay whole) -> test_segmentation; phrase
bonus (largest idf among alternatives) -> test_phrase; modules without a name class (ranked by score among the concept
hits even when their name equals or contains the query) -> test_python_modules; name classes (own stem, exact
name, word run) only for verified, partial and existing-unverified rows, each ordered by tier, status, then score, the concept class by score whatever the
tier -> test_class_gate_and_order; display name (also a qualifier match,
an alias-only match showing the header, the own-stem exception, the joined stem tiebreak) -> test_display_name; absent
rows -> test_tie_order, test_typo; show (`  Python`, `  in template`) -> test_show;
CLI --find (one or more words), no match, default 10 rows, -n, argument errors, no Index, `in template` marker ->
test_cli_find; unknown-name rows (the marker, libclang stub, no inventories, no-libclang fallback over all the names) ->
test_cli_unknown_name;
repository rows, own stems (top 3, rank 1 unless another C++ row shares the stem tokens), the user's examples (bipartite, binary search, hash, debug), corpus metrics and generated typos ->
test_repo_rows, test_repo_stems, test_repo_examples, test_repo_corpus.
"""
import argparse
import contextlib
import functools
import importlib.util
import io
import json
import math
import os
from pathlib import Path
import random
import re
import shutil
import sys
import tempfile
import types
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
SEARCH_PY = ROOT / '09-Contest Testing/_11_search.py'
GRAB_PY = ROOT / '09-Contest Testing/10-grab.py'
CORPUS = ROOT / '96-Local Testing/00-Tools/14-search_corpus.json'
LEDGER = ROOT / '00-Guidelines/Ledgers/library-checker-coverage.json'
FLOORS = {'lc': (0.87, 0.77), 'cp': (0.95, 0.86), 'oi': (0.85, 0.70), 'abbr': (0.87, 0.80), 'typo': (0.90, 0.90),
          'simple': (0.92, 0.80), 'py': (0.94, 0.62), 'generated': (0.88, 0.79)}
# (recall@5, MRR@10) per corpus source: the 2026-11-07 full-mode results after the P263 retune minus 0.02, rounded down;
# typo and generated keep their fixed floors
TUNED = {  # the 00-search.md table; the formula tests read the module, this pins its values
    'WEIGHTS': {'stem': 1.0, 'ident': 1.0, 'ops': 1.0, 'qual': 0.25, 'aliases': 2.0, 'folder': 0.6}, 'B': 0.5, 'K1': 1.2,
    'TYPO': {1: 0.7, 2: 0.5}, 'PREFIX': 0.75, 'PHRASE': 1.0,
    'STANDING': {'verified': 1.0, 'partial': 1.0, 'existing-unverified': 1.0, 'legacy-reference': 0.9, 'planned': 0.9},
    'LEVEL': {'Basic': 1.0, 'Advanced': 1.0, 'Esoteric': 0.9}, 'TOLERANCE': (3, 7), 'PREFIX_MIN': 2, 'PART': 2}
MODE, SEED, ROUNDS = 'full', 42, 3


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


M = load(SEARCH_PY, 'search_under_test')
G = load(GRAB_PY, 'grab_for_search_test')


def cases(base):
    return base if MODE == 'quick' else 10 * base if MODE == 'full' else 10 * base * ROUNDS


def write(files):
    """A temporary tree holding files ({relative path: text}); removed at exit."""
    root = Path(tempfile.mkdtemp(prefix='search-tester-'))
    for rel, text in files.items():
        (root / rel).parent.mkdir(parents=True, exist_ok=True)
        (root / rel).write_text(text, encoding='utf-8')
    return root


def table(rows, cols=3):
    head = '| Header | Basic | Operations | Status |\n|---|---|---|---|\n' if cols == 4 else \
        '| Header | Operations | Status |\n|---|---|---|\n'
    return head + ''.join(f'| {" | ".join(r)} |\n' for r in rows)


# ---------------------------------------------------------------- oracles
def o_words(text, stop=()):
    """Character scanner: runs of letters or digits, a letter run cut before an ASCII capital after a lower-case
    letter, and before the last capital of a capital run followed by a lower-case letter."""
    out, cur, kind = [], '', None
    low, up = (lambda c: 'a' <= c <= 'z'), (lambda c: 'A' <= c <= 'Z')
    for k, ch in enumerate(text):
        c = 'L' if ch.isalpha() else 'D' if ch.isdigit() else None
        cut = c != kind or c is None or (up(ch) and k and (low(text[k - 1]) or
                                                          (up(text[k - 1]) and k + 1 < len(text) and low(text[k + 1]))))
        if cut:
            out.append(cur)
            cur = ''
        cur, kind = (cur + ch if c else ''), c
    out.append(cur)
    return [w.lower() for w in out if w and w.lower() not in stop]


def o_osa(a, b):
    @functools.lru_cache(None)
    def d(i, j):
        if not i or not j:
            return i + j
        r = min(d(i - 1, j) + 1, d(i, j - 1) + 1, d(i - 1, j - 1) + (a[i - 1] != b[j - 1]))
        if i > 1 and j > 1 and a[i - 1] == b[j - 2] and a[i - 2] == b[j - 1]:
            r = min(r, d(i - 2, j - 2) + 1)
        return r
    return d(len(a), len(b))


def o_splits(word, known):
    """Every split of word into known parts of PART letters or more."""
    if not word:
        return [[]]
    return [[word[:k]] + rest for k in range(M.PART, len(word) + 1) if word[:k] in known for rest in o_splits(word[k:], known)]


def o_score(fields, i, terms, w, b, k1):
    """BM25F of row i for [(token, factor)] (one alternative per query group) over field token lists."""
    n, total = len(fields), 0.0
    for t, f in terms:
        df = sum(any(t in r[x] for x in r) for r in fields)
        idf = math.log(1 + (n - df + 0.5) / (df + 0.5))
        x = 0.0
        for fld in w:
            tf = fields[i][fld].count(t)
            if tf:
                avg = sum(len(r[fld]) for r in fields) / n or 1
                x += w[fld] * tf / (1 - b + b * len(fields[i][fld]) / avg)
        total += f * (idf * x * (k1 + 1) / (k1 + x) if x else 0.0)
    return total


def o_idf(fields, t):
    df = sum(any(t in r[x] for x in r) for r in fields)
    return math.log(1 + (len(fields) - df + 0.5) / (df + 0.5))


def o_show(name, header, tier, status, held=()):
    return (name.ljust(30) + ' ' + header.ljust(46) + ' ' + tier.ljust(9) + ' ' + status + ('  Python' if header.endswith('.py') else '')
            + ('  in template' if header in held else ''))


FLAT_S = {k: 1.0 for k in ('verified', 'partial', 'existing-unverified', 'legacy-reference', 'planned')}
FLAT_L = {k: 1.0 for k in ('Basic', 'Advanced', 'Esoteric')}


def flat(root, **kw):
    """Search with every status and tier prior 1, so scores are the BM25F sums the oracles recompute."""
    return M.Search(root, **{'standing': FLAT_S, 'level': FLAT_L, **kw})


# ---------------------------------------------------------------- fixtures
DS = '02-Data Structures'
ROWS_FX = {
    '01-Core/00-index.md': '# Core\n\n## Esoteric\n\n' + table([('`05-modint.hpp`', 'yes', 'ModInt<MOD>, mint (alias): ctor, pow',
                                                                'verified; [05-modint.md](Docs/05-modint.md)'),
                                                               ('`09-polyfull.hpp`', 'yes until mini exists', 'Poly: mul', 'verified')], 4)
                           + '\n## Basic\n\n' + table([('`07-infint.hpp`', 'no', 'InfInt, iint (alias): ctor, +=, abs',
                                                          'partial; missing: sqrtRem')], 4),
    f'{DS}/00-index.md': '# DS\n\n## Basic\n\n' + table([
        ('`01-dsu.hpp`', 'DSU: ctor, unite, same', 'verified; [evidence](Docs/01-dsu.md)'),
        ('`24-interval_set.hpp`', 'IntervalSet<T>: insert, erase; required: covered, gaps', 'existing-unverified; note'),
        ('`notes.txt`', 'x', 'verified')]) + 'Some text | `x.hpp` | not a row |\n\n## Advanced\n\n'
        + table([('`30-heavy_light.hpp`', 'cartesianTree (cmp): parent, root', 'legacy-reference')])
        + '\n## Esoteric\n\n' + table([('`40-odd_one.hpp`', 'A as B and C: x', 'planned; later'),
                                       ('`41-skipped.hpp`', 'foo', 'verified')]),
    f'{DS}/Docs/01-dsu.md': '# DSU\n\nAliases: union find, disjoint set\n',
    f'{DS}/Docs/24-interval_set.md': '# Interval set\n\nPlain text mentions nothing.\n',
    f'{DS}/Docs/30-other.md': 'Aliases: hld\n',
    '03-Geometry/Docs/30-heavy_light.md': 'Aliases: hld\n',
    '08-Python/00-index.md': '## Basic\n\n| Module | Operations | Status |\n|---|---|---|\n| `_06_dsu.py` | DSU: find, unite | planned; pyrival |\n'
                             '| `_03_mod.py` | modPow | planned |\n',
    '08-Python/Docs/_03_mod.md': '# mod\n\nAliases: modular exponent\n',
    '09-Contest Testing/00-index.md': table([('`01-x.hpp`', 'foo', 'verified')]),
    '00-Guidelines/00-index.md': table([('`01-y.hpp`', 'foo', 'verified')]),
}

RANK_FX = {f'{DS}/00-index.md': '## Basic\n\n' + table([
    ('`01-fenwick.hpp`', 'Fenwick<T>: ctor, add, sum', 'verified'),
    ('`02-sum_sum.hpp`', 'total (sum sum sum)', 'verified'),
    ('`03-prefix.hpp`', 'prefixSum; rangeSum', 'verified'),
    ('`04-bit_ops.hpp`', 'BitOps: popcount, parity', 'verified'),
    ('`05-bit_bit_tricks.hpp`', 'lowbit (bit bit bit)', 'verified'),
    ('`06-segtree_beats.hpp`', 'SegTreeBeats: chmin (segtree segtree segtree segtree segtree), query', 'verified'),
    ('`07-segtree.hpp`', 'build', 'existing-unverified'),
    ('`08-dsu.hpp`', 'DSU: unite, same, find', 'verified'),
    ('`09-samples.hpp`', 'draw (sample sample)', 'verified'),
    ('`10-max_flow.hpp`', 'flowPush; maxFlowRun', 'verified'),
    ('`11-other.hpp`', 'pushFlow; flowPush', 'verified'),
    ('`12-wombat.hpp`', '(wombat)', 'verified'),
    ('`13-gamma.hpp`', 'alpha; betaGamma; gammaDelta', 'verified'),
    ('`14-segtree_segtree_segtree.hpp`', 'walk (segtree segtree segtree segtree segtree segtree)', 'verified'),
    ('`15-pbs.hpp`', 'parallelBinarySearch (offline)', 'verified'),
    ('`16-binary_search_tools.hpp`', 'lowerBound (binary search binary search)', 'verified'),
    ('`17-seg_tree.hpp`', 'build', 'verified'),
    ('`18-forest.hpp`', 'grow (seg tree seg tree seg tree)', 'verified')])}

TIE_FX = {
    '01-Core/00-index.md': table([('`30-core.hpp`', 'no', 'zork', 'verified'), ('`31-corb.hpp`', 'yes', 'zork', 'partial; x')], 4),
    f'{DS}/00-index.md': '## Basic\n\n' + table([('`20-aa.hpp`', 'zork', 'planned'), ('`21-ab.hpp`', 'zork', 'verified'),
                                                 ('`24-ae.hpp`', 'zork', 'verified'), ('`25-af.hpp`', 'other', 'verified'),
                                                 ('`26-ag.hpp`', 'zork', 'legacy-reference'),
                                                 ('`27-ah.hpp`', 'zork', 'existing-unverified; note')])
                         + '## Advanced\n\n' + table([('`22-ac.hpp`', 'zork', 'verified')])
                         + '## Esoteric\n\n' + table([('`23-ad.hpp`', 'zork', 'verified')])}
TIE_ORDER = [f'{DS}/21-ab.hpp', f'{DS}/24-ae.hpp', '01-Core/31-corb.hpp', f'{DS}/27-ah.hpp', '01-Core/30-core.hpp',
             f'{DS}/22-ac.hpp', f'{DS}/23-ad.hpp',  # exact names: tier (30-core is Advanced), status, score, header
             f'{DS}/26-ag.hpp', f'{DS}/20-aa.hpp']  # legacy-reference and planned rows take no name class

OWN_FX = {'04-Graphs/00-index.md': '## Basic\n\n' + table([('`01-mst.hpp`', 'kruskal; prim', 'verified'),
                                                     ('`03-forest.hpp`', 'kruskalForest', 'verified')])
                               + '## Advanced\n\n' + table([('`10-kruskal.hpp`', 'reconstructionTree', 'verified')])}

GATE_FX = {f'{DS}/00-index.md': table([('`01-heap.hpp`', 'Heap: push (heap heap heap)', 'planned'),
                                       ('`02-pairing.hpp`', 'PairingHeap: meld', 'verified'),
                                       ('`03-oldheap.hpp`', 'heap (legacy)', 'legacy-reference'),
                                       ('`04-heaps.hpp`', 'heap', 'existing-unverified')])}

CLASS_FX = {f'{DS}/00-index.md': '## Basic\n\n' + table([
    ('`01-bip.hpp`', 'bipartiteCheck', 'verified'),
    ('`03-bip_more.hpp`', 'bipartiteColor (bipartite bipartite)', 'verified'),
    ('`04-x.hpp`', 'foo (bipartite)', 'verified')]) + '## Advanced\n\n' + table([
    ('`02-match.hpp`', 'BipartiteMatching: run (bipartite bipartite bipartite bipartite)', 'verified'),
    ('`05-y.hpp`', 'bar (bipartite bipartite bipartite)', 'verified')])}

MA = '05-Mathematics'
PRIOR_FX = {f'{MA}/00-index.md': table([('`01-a.hpp`', 'alpha (foo foo)', 'planned'), ('`02-b.hpp`', 'beta (foo)', 'verified')])}
BM25_FX = {f'{MA}/00-index.md': '## Basic\n\n' + table([
    ('`01-alpha_beta.hpp`', 'gamma; delta (omega)', 'verified'),
    ('`02-gamma.hpp`', 'gammaRay; epsilon; gamma', 'verified'),
    ('`03-zeta.hpp`', 'Zeta: ctor, eta', 'verified'),
    ('`04-theta.hpp`', 'iota; gamma (gamma gamma)', 'verified')])}
MATH = ['mathematic']
BM25_FIELDS = [  # hand-derived from the fixture: stem, ident (names lower-cased, unsplit), ops, qual, aliases, folder
    {'stem': ['alpha', 'beta'], 'ident': ['alphabeta', 'gamma', 'delta'], 'ops': ['gamma', 'delta'], 'qual': ['omega'],
     'aliases': [], 'folder': MATH},
    {'stem': ['gamma'], 'ident': ['gamma', 'gammaray', 'epsilon', 'gamma'], 'ops': ['gamma', 'ray', 'epsilon', 'gamma'],
     'qual': [], 'aliases': [], 'folder': MATH},
    {'stem': ['zeta'], 'ident': ['zeta', 'zeta', 'zeta', 'eta'], 'ops': ['zeta', 'ctor', 'eta'], 'qual': [], 'aliases': [],
     'folder': MATH},
    {'stem': ['theta'], 'ident': ['theta', 'iota', 'gamma'], 'ops': ['iota', 'gamma'], 'qual': ['gamma', 'gamma'],
     'aliases': [], 'folder': MATH}]

PY_FX = {f'{MA}/00-index.md': table([('`05-kappa.hpp`', 'kappaFn', 'verified')]),
         '08-Python/00-index.md': '## Basic\n\n' + table([('`_05_kappa.py`', 'kappaFn', 'verified')])}

PY2_FX = {f'{MA}/00-index.md': table([('`06-lam.hpp`', 'lamb (lambda fn)', 'verified'), ('`07-nu.hpp`', 'solveAll (omega omega)', 'verified'),
                                     ('`08-lambda_fn.hpp`', 'build', 'verified'), ('`09-omega.hpp`', 'build', 'verified')]),
          '08-Python/00-index.md': table([('`_06_lam.py`', 'lambdaFn', 'verified'), ('`_07_nu.py`', 'omegaSolve', 'verified')])}

PHRASE_FX = {f'{MA}/00-index.md': table([('`01-a.hpp`', 'omega; zeta', 'verified'), ('`02-b.hpp`', 'omega', 'verified'),
                                         ('`03-c.hpp`', 'omega', 'verified'), ('`04-d.hpp`', 'omelet', 'verified')])}

GR = '04-Graphs'
FUZZY_FX = {f'{GR}/00-index.md': '## Basic\n\n' + table([
    ('`05-dijkstra.hpp`', 'dijkstra (heap); path', 'verified'),
    ('`06-comb.hpp`', 'binomialTable; choose', 'verified'),
    ('`07-blockcuttree.hpp`', 'build (biconnected)', 'verified'),
    ('`08-segmenttree.hpp`', 'build; query (point update)', 'verified'),
    ('`09-lines.hpp`', 'segment (tree)', 'verified'),
    ('`10-dsu.hpp`', 'DSU: unite', 'verified'),
    ('`11-lowbit.hpp`', 'build', 'verified'),
    ('`12-bits.hpp`', 'lowbit; low; bit', 'verified')])}

TPL_FX = {'01-Core/00-index.md': table([('`01-template.hpp`', 'yes', 'macros: fi, se; chmin, chmax', 'verified'),
                                            ('`02-debug.hpp`', 'yes', 'debug, trace (macros)', 'verified')], 4),
          f'{GR}/00-index.md': table([('`05-relax.hpp`', 'chminEdge', 'verified')])}

STRIP_FX = {'06-Miscellaneous/00-index.md': '## Basic\n\n' + table([('`02-b.hpp`', 'zapAll', 'verified')])
                                         + '## Advanced\n\n' + table([('`01-a.hpp`', 'zap', 'verified')])}

NAME_FX = {'06-Miscellaneous/00-index.md': table([
    ('`01-seg_tree.hpp`', 'queryAll (x); segtreeBuild (x)', 'verified'),
    ('`05-prime.hpp`', 'isPrime (deterministic Miller-Rabin); factor (pollard)', 'verified'),
    ('`08-sequence_algorithms.hpp`', 'maxSubarray; lis', 'verified'),
    ('`09-mst.hpp`', 'kruskal; prim', 'verified')]),
    '06-Miscellaneous/Docs/08-sequence_algorithms.md': 'Aliases: kadane\n'}

MANY_FX = {'06-Miscellaneous/00-index.md': table([(f'`{k:02}-z{k}.hpp`', 'zork', 'verified') for k in range(1, 13)])}

CLI_FX = {f'{GR}/00-index.md': '## Basic\n\n' + table([
    ('`05-shortest_path.hpp`', 'dijkstra; bellmanFord', 'verified'),
    ('`06-dial.hpp`', 'dialDijkstra', 'verified'),
    ('`07-astar.hpp`', 'aStar (dijkstra variant)', 'verified'),
    ('`08-kpaths.hpp`', 'kShortest (dijkstra based)', 'partial; missing: y'),
    ('`09-dsu.hpp`', 'DSU: unite', 'verified')])}


class Fixtures(unittest.TestCase):
    roots = []

    @classmethod
    def tearDownClass(cls):
        for r in cls.roots:
            shutil.rmtree(r, ignore_errors=True)

    def tree(self, files):
        r = write(files)
        self.roots.append(r)
        return r

    def hit(self, hits, header):
        return next((h for h in hits if h.header == header), None)


class Units(Fixtures):
    def test_cells_split_operations(self):
        self.assertEqual(M.cells(r'| a | b \| c | d |'), ['a', r'b \| c', 'd'])
        self.assertEqual(M.split('a, (b, c), [l, r), d', ','), ['a', '(b, c)', '[l, r)', 'd'])
        self.assertEqual(M.split('f<A, B>, g', ',', True), ['f<A, B>', 'g'])
        self.assertEqual(M.operations('A, B: x, y; z'), [(['A', 'B'], 'x'), (['A', 'B'], 'y'), ([], 'z')])

    def test_names(self):
        table_ = [
            ('Fenwick<T>: ctor, add, sum', ['Fenwick', 'Fenwick::add', 'Fenwick::sum']),
            ('InfInt, iint (alias): ctor, +=, abs', ['InfInt', 'iint', 'InfInt::operator+=', 'InfInt::abs']),
            ('iint (alias): x', ['iint', 'iint::x']),
            ('cartesianTree (cmp): parent, root', ['cartesianTree']),
            ('required: a, b', ['a', 'b']),
            ('required: +=, a', ['a']),
            ('A as B and C: x', ['A', 'B', 'C', 'A::x']),
            ('dfs; bfs (multi-source)', ['dfs', 'bfs']),
            ('foo; bar; foo', ['foo', 'bar']),
            ('Seg<T, F>: query<K>, ==, set (point)', ['Seg', 'Seg::query', 'Seg::operator==', 'Seg::set']),
            ('A: x; B: x, y; A: x', ['A', 'A::x', 'B', 'B::x', 'B::y']),
            ('(wombat)', []),
            ('sqrt2: a, b', ['sqrt2']),  # a digit makes a function head
            ('isPrime (deterministic Miller-Rabin); isPrime (trial)', ['isPrime']),
            ('aliases: uint, lng', ['uint', 'lng']),  # a label naming aliases is no `(alias` head
            ('Poly<T>: ctor, PolyCompanion::fft (n), size', ['Poly', 'PolyCompanion::fft', 'Poly::size']),  # qualified entry as written
            ('required: std::hash, a', ['a']),  # a std specialization is no name
            ('Q: std::formatter, x', ['Q', 'Q::x']),
        ]
        for cell, want in table_:
            self.assertEqual(M.names(cell), want, cell)

    def test_entries_notes(self):
        w = lambda pairs: [(n, o_words(q)) for n, q in pairs]
        for cell, want in [
            ('Fenwick<T>: ctor, add (point), sum (prefix)', [('Fenwick', []), ('Fenwick', []), ('Fenwick::add', ['point']),
                                                            ('Fenwick::sum', ['prefix'])]),
            ('cartesianTree (cmp): parent, root', [('cartesianTree', ['cmp', 'parent', 'root'])]),  # head: its entries too
            ('required: a (x), b', [('a', ['x']), ('b', [])]),
            ('InfInt, iint (alias): abs (magnitude)', [('InfInt', []), ('iint', ['alias']), ('InfInt::abs', ['magnitude'])]),
            ('isPrime (deterministic Miller-Rabin); isPrime (trial)', [('isPrime', ['deterministic', 'miller', 'rabin']),
                                                                       ('isPrime', ['trial'])])]:
            self.assertEqual(w(M.entries(cell)), want, cell)
            merged = {}
            for n, q in want:
                merged[n] = merged.get(n, []) + q
            self.assertEqual({n: o_words(q) for n, q in M.notes(cell).items()}, merged, cell)
            self.assertEqual(M.names(cell), list(dict.fromkeys(n for n, _ in want)), cell)

    def test_rows_fixture(self):
        root = self.tree(ROWS_FX)
        got = [(r.header, r.folder, r.tier, r.status, r.names, r.key) for r in M.rows(root)]
        want = [
            ('01-Core/05-modint.hpp', '01-Core', 'Basic', 'verified', ['ModInt', 'mint', 'ModInt::pow'], ['modint']),
            ('01-Core/09-polyfull.hpp', '01-Core', 'Basic', 'verified', ['Poly', 'Poly::mul'], ['polyfull']),  # `yes until ...`
            ('01-Core/07-infint.hpp', '01-Core', 'Advanced', 'partial', ['InfInt', 'iint', 'InfInt::operator+=', 'InfInt::abs'], ['infint']),
            (f'{DS}/01-dsu.hpp', DS, 'Basic', 'verified', ['DSU', 'DSU::unite', 'DSU::same'], ['dsu']),
            (f'{DS}/24-interval_set.hpp', DS, 'Basic', 'existing-unverified',
             ['IntervalSet', 'IntervalSet::insert', 'IntervalSet::erase', 'covered', 'gaps'], ['interval', 'set']),
            (f'{DS}/30-heavy_light.hpp', DS, 'Advanced', 'legacy-reference', ['cartesianTree'], ['heavy', 'light']),
            (f'{DS}/40-odd_one.hpp', DS, 'Esoteric', 'planned', ['A', 'B', 'C', 'A::x'], ['odd', 'one']),
            (f'{DS}/41-skipped.hpp', DS, 'Esoteric', 'verified', ['foo'], ['skipped']),
            ('08-Python/_06_dsu.py', '08-Python', 'Basic', 'planned', ['DSU', 'DSU::find', 'DSU::unite'], ['dsu']),
            ('08-Python/_03_mod.py', '08-Python', 'Basic', 'planned', ['modPow'], ['mod'])]
        self.assertEqual(got, want)
        f = {r.header: r.fields for r in M.rows(root)}
        self.assertEqual(f[f'{DS}/01-dsu.hpp']['aliases'], ['union', 'find', 'disjoint', 'set'])  # the label is dropped
        self.assertEqual([f[h]['aliases'] for h in (f'{DS}/24-interval_set.hpp', f'{DS}/30-heavy_light.hpp')], [[], []])
        self.assertEqual(f[f'{DS}/24-interval_set.hpp']['stem'], ['interval', 'set'])
        self.assertEqual(f[f'{DS}/24-interval_set.hpp']['folder'], ['data', 'structure'])
        self.assertEqual((f['08-Python/_06_dsu.py']['stem'], f['08-Python/_06_dsu.py']['folder']), (['dsu'], ['python']))
        self.assertEqual((f['08-Python/_03_mod.py']['aliases'], f['08-Python/_06_dsu.py']['aliases']), (['modular', 'exponent'], []))
        self.assertEqual(M.rows(self.tree({})), [])

    def test_aliases(self):
        text = '\n'.join([
            '# Doc', 'Intro mentions aliases: never', 'Other text: aliases never2', '',
            '## Aliases and names', 'sec1 [link](https://x.org/nolink1) sec2', '', 'sec3', 'Catalog aliases: sec5',
            '- **Aliases elsewhere:** sec6', '## Next', 'out1',
            '### Aliases and catalog names', 'sec4', '#### Other', 'out2', '',
            'Aliases: par1 https://example.com/nourl par1b', 'par1c', '', 'out3',
            'Catalog aliases: par2', '', 'Aliases with `FIXED`: out4', '',
            '- Aliases (2026-10-19 sweep): par3', '- **Aliases elsewhere:** par4', '  - par4b', '',
            'Search aliases (P263, grab --find): par5', '',
            'Aliases in the fetched libraries:', '- par6 ([ref](Docs/nolink2.md))', '- par6b', '', 'out5',
            'Aliases with `FIXED`: out6', 'out7', '', 'An ordinary paragraph about aliases.', 'out8'])
        got = set(re.findall(r'\w+', M.aliases(text)))
        inside = {'sec1', 'sec2', 'sec3', 'sec4', 'sec5', 'sec6', 'par1', 'par1b', 'par1c', 'par2', 'par3', 'par4', 'par4b', 'par5', 'par6',
                  'par6b', 'link', 'ref'}
        outside = {'never', 'never2', 'out1', 'out2', 'out3', 'out4', 'out5', 'out6', 'out7', 'out8', 'nolink1', 'nolink2',
                   'nourl', 'example', 'Next', 'Other', 'Doc'}
        labels = {'Aliases', 'aliases', 'Catalog', 'Search', 'P263', 'grab', 'find', '2026', '10', '19', 'sweep', 'elsewhere',
                  'in', 'the', 'fetched', 'libraries'}  # opening labels, also inside sections
        self.assertEqual((inside - got, outside & got, labels & got), (set(), set(), set()))

    def test_words_table(self):
        for text, want in [('LCPArray', ['lcp', 'array']), ('segTree2D', ['seg', 'tree', '2', 'd']),
                           ('max_flow-dinic', ['max', 'flow', 'dinic']), ('HLDecomposition', ['hl', 'decomposition']),
                           ('getXMLHttpRequest', ['get', 'xml', 'http', 'request']), ('k2Tree', ['k', '2', 'tree']),
                           ('mod998244353', ['mod', '998244353']), ('ModInt64', ['mod', 'int', '64']), ('A as B', ['b']),
                           ('x__y', ['x', 'y']), ('', []), ('Fenwick::sum', ['fenwick', 'sum']), ('operator+=', ['operator']),
                           ('the sum of a tree', ['sum', 'tree'])]:
            self.assertEqual((M.words(text), o_words(text, M.STOP)), (want, want), text)
        self.assertEqual(M.words('the sum of a tree', ()), ['the', 'sum', 'of', 'a', 'tree'])
        self.assertEqual(M.words('A as B', stop=()), ['a', 'as', 'b'])

    def test_words_random(self):
        rng = random.Random(SEED)
        for r in range(cases(300)):
            s = ''.join(rng.choice('aAbBzZ19_- :') for _ in range(rng.randint(0, 12)))
            self.assertEqual(M.words(s, ()), o_words(s), f'seed={SEED} mode={MODE} round={r} text={s!r}')

    def test_stem_table(self):
        table_ = {'matches': 'match', 'vertices': 'vertex', 'queries': 'query', 'spanning': 'span', 'rolling': 'roll',
                  'strings': 'string', 'string': 'string', 'series': 'series', 'hashing': 'hash', 'classes': 'class',
                  'class': 'class', 'bus': 'bus', 'gas': 'gas', 'status': 'status', 'analysis': 'analysis', 'trees': 'tree',
                  'boxes': 'box', 'dishes': 'dish', 'processes': 'process', 'indices': 'index', 'matrices': 'matrix',
                  'species': 'species', 'ties': 'tie', 'tries': 'trie', 'trie': 'trie', 'sorting': 'sort', 'king': 'king',
                  'ring': 'ring', 'running': 'run', 'passing': 'pass', 'buzzing': 'buzz', 'agreeing': 'agree', 'freeing': 'free', 'filling': 'fill',
                  'ceiling': 'ceil', 'hashings': 'hash', 'axes': 'axe', 'sizes': 'size', 'buzzes': 'buzz', 'gases': 'gase', 'set': 'set',
                  'sets': 'set', 'ops': 'ops', 'bits': 'bit'}
        self.assertEqual({w: M.stem(w) for w in table_}, table_)

    def test_distance_known(self):
        for a, b, d in [('', '', 0), ('', 'abc', 3), ('ab', 'ba', 1), ('ca', 'abc', 3), ('kitten', 'sitting', 3),
                        ('dijkstar', 'dijkstra', 1), ('abcd', 'badc', 2), ('a', 'b', 1)]:
            self.assertEqual(o_osa(a, b), d, (a, b))
            for cap in range(4):
                self.assertEqual(M.distance(a, b, cap), d if d <= cap else cap + 1, (a, b, cap))

    def test_distance_random(self):
        rng = random.Random(SEED + 1)
        for r in range(cases(400)):
            a, b = (''.join(rng.choice('abc') for _ in range(rng.randint(0, 6))) for _ in range(2))
            d = o_osa(a, b)
            self.assertEqual(d, o_osa(b, a), (a, b))
            for cap in range(4):
                self.assertEqual(M.distance(a, b, cap), d if d <= cap else cap + 1,
                                 f'seed={SEED} mode={MODE} round={r} a={a!r} b={b!r} cap={cap} osa={d}')

    def check_segment(self, word, known, info):
        got, splits = M.segment(word, known), o_splits(word, known)
        best = min(map(len, splits), default=0)
        if best <= 1:
            self.assertEqual(got, [], info)
        else:
            self.assertEqual((''.join(got), len(got), all(p in known and len(p) >= M.PART for p in got)), (word, best, True), info)

    def test_segment_table(self):
        known = {'segment', 'tree', 'block', 'cut', 'seg', 'men', 'ttree', 'ab', 'x'}  # hand-derived for PART = 2
        for w, want in (('segmenttree', ['segment', 'tree']), ('blockcuttree', ['block', 'cut', 'tree']), ('abab', ['ab', 'ab']),
                        ('cutab', ['cut', 'ab']), ('segment', []), ('treex', []), ('abx', []), ('', [])):  # x: shorter than PART
            self.assertEqual(M.segment(w, known), want, w)

    def test_segment_random(self):
        rng = random.Random(SEED + 2)
        for r in range(cases(200)):
            word = ''.join(rng.choice('ab') for _ in range(rng.randint(0, 12)))
            known = {''.join(rng.choice('ab') for _ in range(rng.randint(1, 6))) for _ in range(rng.randint(0, 8))}
            known |= {word[i:j] for i, j in (sorted(rng.sample(range(len(word) + 1), 2)) for _ in range(rng.randint(0, 3) if word else 0))}
            self.check_segment(word, known, f'seed={SEED} mode={MODE} round={r} word={word!r} known={sorted(known)}')

    def test_tolerance(self):
        lo, hi = M.TOLERANCE
        self.assertEqual([M.tolerance('x' * n) for n in range(13)], [0 if n <= lo else 1 if n <= hi else 2 for n in range(13)])

    def test_tuned_constants(self):
        self.assertEqual({k: getattr(M, k) for k in TUNED}, TUNED)

    def test_show(self):
        hits = [M.Hit('Fenwick::sum', f'{DS}/01-fenwick.hpp', 'Basic', 'verified', 1.5),
                M.Hit('x' * 40, '08-Python/_06_dsu.py', 'Esoteric', 'planned', 0.1)]
        self.assertEqual(M.show(hits), [o_show(*h[:4]) for h in hits])
        self.assertTrue(M.show(hits)[1].endswith('planned  Python'))
        self.assertEqual(M.show([]), [])
        held = ('01-Core/01-template.hpp', '08-Python/_06_dsu.py')
        hits.append(M.Hit('chmin', '01-Core/01-template.hpp', 'Basic', 'verified', 2.0))
        self.assertEqual(M.show(hits, held), [o_show(*h[:4], held) for h in hits])
        self.assertEqual([l.endswith('planned  Python  in template') for l in M.show(hits, held)], [False, True, False])
        self.assertTrue(M.show(hits, held)[2].endswith('verified  in template'))


class Ranking(Fixtures):
    def test_bm25_one_word(self):
        root = self.tree(BM25_FX)
        custom = {'stem': 1.5, 'ident': 0.7, 'ops': 2.0, 'qual': 1.0, 'aliases': 0.4, 'folder': 0.9}
        for kw in ({}, {'weights': custom, 'b': 0.4, 'k1': 1.7}):
            s = flat(root, **kw)
            self.assertEqual([{f: r.fields[f] for f in M.FIELDS} for r in s.rows], BM25_FIELDS)
            w, b, k1 = kw.get('weights', M.WEIGHTS), kw.get('b', M.B), kw.get('k1', M.K1)
            for q in ('gamma', 'omega', 'eta', 'Gamma'):
                hits = s.find(q, 10)
                want = {i for i, r in enumerate(BM25_FIELDS) if any(q.lower() in v for v in r.values())}
                self.assertEqual({h.header for h in hits}, {s.rows[i].header for i in want}, q)
                for i in want:
                    o = o_score(BM25_FIELDS, i, [(q.lower(), 1.0)], w, b, k1)
                    self.assertLess(abs(self.hit(hits, s.rows[i].header).score - o), 2e-9, (q, i, kw))

    def test_classes_exact_and_within(self):
        s = flat(self.tree(RANK_FX), weights={**M.WEIGHTS, 'qual': 4.0})  # heavy qualifiers: concept rows score highest
        h = lambda n: f'{DS}/{n}'
        for q, order, names in [('sum', ['01-fenwick.hpp', '03-prefix.hpp', '02-sum_sum.hpp'], ['Fenwick::sum', 'rangeSum', 'total']),
                                ('bit', ['04-bit_ops.hpp', '05-bit_bit_tricks.hpp'], ['BitOps', 'lowbit']),
                                ('segtree', ['07-segtree.hpp', '06-segtree_beats.hpp', '14-segtree_segtree_segtree.hpp',
                                             '17-seg_tree.hpp'], ['build', 'SegTreeBeats', 'walk', f'{DS}/17-seg_tree.hpp']),  # 17: ident only; no name shares it
                                ('sam', ['09-samples.hpp', '08-dsu.hpp'], ['draw', 'DSU::same']),
                                ('same', ['08-dsu.hpp'], ['DSU::same']), ('FENWICK', ['01-fenwick.hpp'], ['Fenwick'])]:
            hits = s.find(q, 20)
            self.assertEqual([(x.header, x.name) for x in hits], [(h(o), n) for o, n in zip(order, names)], q)
            score = {x.header: x.score for x in hits}
            strong = {'sum': 2, 'bit': 1, 'segtree': 2}.get(q)  # the classes run against the scores: this concept row scores highest
            if strong is not None:
                self.assertTrue(all(score[h(order[strong])] > score[h(o)] for o in order[:strong]), (q, score))
        for q, first, others in (('BinarySearch', '15-pbs.hpp', ['16-binary_search_tools.hpp']),  # one word: the word-run class
                                 ('binarysearch', '15-pbs.hpp', []),
                                 ('binary search', '16-binary_search_tools.hpp', ['15-pbs.hpp']),  # two words: no word-run class
                                 ('seg tree', '17-seg_tree.hpp', ['18-forest.hpp', '06-segtree_beats.hpp'])):  # own stem: any query
            hits = s.find(q, 20)
            self.assertEqual([x.header for x in hits[:len(others) + 1]], [h(first)] + [h(o) for o in others], q)
            score = {x.header: x.score for x in hits}
            if q != 'binarysearch':
                self.assertGreater(max(score[h(o)] for o in others), score[h(first)] if q != 'binary search' else 0, (q, score))
        sc = {x.header: x.score for x in s.find('segtree', 5)}
        self.assertGreater(sc[h('06-segtree_beats.hpp')], sc[h('07-segtree.hpp')])  # the own stem outranks a higher score

    def test_tie_order(self):
        s = flat(self.tree(TIE_FX))
        hits = s.find('zork', 100)
        self.assertEqual([x.header for x in hits], TIE_ORDER)
        self.assertEqual(len({x.score for x in hits}), 1)
        self.assertEqual([x.header for x in s.find('zork', 3)], TIE_ORDER[:3])
        self.assertEqual([x.header for x in s.find('zork', 1)], TIE_ORDER[:1])
        self.assertEqual([x.header for x in s.find('zork')], TIE_ORDER)
        self.assertEqual(s.find('nothing here'), [])

    def test_priors(self):
        root = self.tree(TIE_FX)
        kind = {r.header: (r.status, r.tier) for r in M.rows(root)}
        base = {x.header: x.score for x in flat(root).find('zork', 100)}
        st, lv = {**FLAT_S, 'planned': 0.5}, {**FLAT_L, 'Esoteric': 0.25}
        for kw, f in (({'standing': st, 'level': lv}, lambda h: st[kind[h][0]] * lv[kind[h][1]]),
                      ({'standing': {'verified': 0.5}}, lambda h: 0.5 if kind[h][0] == 'verified' else 1.0),  # missing keys: 1
                      ({'level': {'Esoteric': 3.0}}, lambda h: 3.0 if kind[h][1] == 'Esoteric' else 1.0)):
            hits = flat(root, **kw).find('zork', 100)
            self.assertEqual(set(base), {x.header for x in hits}, kw)
            for x in hits:
                self.assertLess(abs(x.score - f(x.header) * base[x.header]), 2e-9, (kw, x))
        hits = M.Search(root).find('zork', 100)  # the defaults read M.STANDING and M.LEVEL
        for x in hits:
            self.assertLess(abs(x.score - M.STANDING.get(x.status, 1) * M.LEVEL.get(x.tier, 1) * base[x.header]), 2e-9, x)
        kw = {'level': {**FLAT_L, 'Basic': 0.1}, 'standing': {**FLAT_S, 'legacy-reference': 0.1}}
        hits = flat(root, **kw).find('zork', 100)  # the name class ignores the scores; the concept class follows them
        self.assertEqual([x.header for x in hits], TIE_ORDER[:7] + [f'{DS}/20-aa.hpp', f'{DS}/26-ag.hpp'])
        self.assertLess(hits[0].score, hits[4].score)
        root = self.tree(PRIOR_FX)
        a, b = f'{MA}/01-a.hpp', f'{MA}/02-b.hpp'
        base = {x.header: x.score for x in flat(root).find('foo')}
        self.assertEqual([x.header for x in flat(root).find('foo')], [a, b])
        self.assertTrue(base[a] > base[b] > 0.5 * base[a], base)
        hits = flat(root, standing=st).find('foo')
        self.assertEqual([(x.header, x.score) for x in hits], [(b, base[b]), (a, round(0.5 * base[a], 9))])

    def test_class_gate_and_order(self):
        s = flat(self.tree(GATE_FX))  # planned and legacy-reference rows take no name class, even with an exact name
        hits = s.find('heap', 10)
        h = lambda n: f'{DS}/{n}'
        self.assertEqual([x.header for x in hits[:2]], [h('04-heaps.hpp'), h('02-pairing.hpp')])
        self.assertEqual({x.header for x in hits[2:]}, {h('01-heap.hpp'), h('03-oldheap.hpp')})
        score = {x.header: x.score for x in hits}
        self.assertGreater(score[h('01-heap.hpp')], score[h('02-pairing.hpp')])  # it would lead on score alone
        self.assertEqual([x.score for x in hits[2:]], sorted((x.score for x in hits[2:]), reverse=True))
        self.assertEqual(self.hit(hits, h('01-heap.hpp')).name, 'Heap')
        hits = flat(self.tree(OWN_FX)).find('kruskal', 10)  # own stem, then exact name, then word run, whatever the tiers
        self.assertEqual([(x.header, x.tier, x.name) for x in hits], [(f'{GR}/10-kruskal.hpp', 'Advanced', 'reconstructionTree'),
                                                                       (f'{GR}/01-mst.hpp', 'Basic', 'kruskal'),
                                                                       (f'{GR}/03-forest.hpp', 'Basic', 'kruskalForest')])
        s = flat(self.tree(CLASS_FX), weights={**M.WEIGHTS, 'qual': 4.0})
        hits = s.find('bipartite', 10)  # word runs: tier, status, then score; the concept class: score, whatever the tier
        self.assertEqual([x.header for x in hits], [h(n) for n in ('03-bip_more.hpp', '01-bip.hpp', '02-match.hpp', '05-y.hpp', '04-x.hpp')])
        score = {x.header: x.score for x in hits}
        self.assertTrue(score[h('02-match.hpp')] > score[h('03-bip_more.hpp')] > score[h('01-bip.hpp')], score)
        self.assertGreater(score[h('05-y.hpp')], score[h('04-x.hpp')])

    def test_typo(self):
        root = self.tree(FUZZY_FX)
        for typo in (None, {1: 0.4, 2: 0.2}):
            s = flat(root, typo=typo, prefix=0.0)
            exact, near = s.find('dijkstra', 10), s.find('dijkstar', 10)
            self.assertEqual([x.header for x in near], [f'{GR}/05-dijkstra.hpp'])
            self.assertLess(abs(near[0].score - (typo or M.TYPO)[1] * exact[0].score), 2e-9)
            far = s.find('dijxstrq', 10)  # 8 letters, 2 substitutions
            self.assertEqual([x.header for x in far], [f'{GR}/05-dijkstra.hpp'])
            self.assertLess(abs(far[0].score - (typo or M.TYPO)[2] * exact[0].score), 2e-9)
        s = flat(root)
        self.assertEqual(s.find('dsx'), [])  # 3 letters: no typo match (dsu is 1 edit away)
        self.assertEqual([x.header for x in s.find('dsuu')], [f'{GR}/10-dsu.hpp'])
        self.assertEqual([x.header for x in s.find('dijxstrq')], [f'{GR}/05-dijkstra.hpp'])  # 8 letters: 2 edits
        self.assertEqual(s.find('dijxstqq'), [])

    def test_typo_and_prefix(self):
        root = self.tree(FUZZY_FX)  # `dijkstr` is 1 edit from dijkstra and begins it: the larger factor counts
        self.assertGreaterEqual(M.tolerance('dijkstr'), 1)
        for typo, prefix in (({1: 0.9, 2: 0.5}, 0.6), ({1: 0.4, 2: 0.2}, 0.6)):
            s = flat(root, typo=typo, prefix=prefix)
            self.assertEqual(s.alternatives('dijkstr'), [('dijkstra', max(typo[1], prefix))])
            near, exact = s.find('dijkstr'), s.find('dijkstra')
            self.assertLess(abs(near[0].score - max(typo[1], prefix) * exact[0].score), 2e-9)

    def test_strip(self):
        s = flat(self.tree(STRIP_FX))  # an exact name (Advanced) before a word run (Basic): ' zap ' is `zap`
        h = lambda n: f'06-Miscellaneous/{n}'
        self.assertEqual([x.header for x in s.find('zap')], [h('01-a.hpp'), h('02-b.hpp')])
        for q in (' zap ', '\tzap\n', 'ZAP '):
            self.assertEqual(s.find(q), s.find('zap'), repr(q))

    def test_prefix(self):
        root = self.tree(FUZZY_FX)
        a, b = flat(root).find('binom'), flat(root, prefix=M.PREFIX / 2).find('binom')
        self.assertEqual([(x.header, x.name) for x in a], [(f'{GR}/06-comb.hpp', 'binomialTable')])
        self.assertLess(abs(b[0].score - a[0].score / 2), 2e-9)
        self.assertEqual(flat(root, prefix=0.0).find('binom'), [])
        self.assertEqual(flat(root).find('b'), [])  # shorter than PREFIX_MIN = 2 (and no typo below 4 letters)
        self.assertIn(f'{GR}/06-comb.hpp', [x.header for x in flat(root).find('bi', 20)])  # 2 letters begin binomial

    def test_joins(self):
        s = flat(self.tree(FUZZY_FX), prefix=0.0)
        fields = [r.fields for r in s.rows]
        i = next(k for k, r in enumerate(s.rows) if r.header.endswith('07-blockcuttree.hpp'))
        self.assertEqual(fields[i]['stem'], ['blockcuttree'])
        w = o_score(fields, i, [('blockcuttree', 1.0)], M.WEIGHTS, M.B, M.K1)
        for q, f in (('block cut tree', 1.0), ('blok cut tree', M.TYPO[1]), ('Block Cut Tree', 1.0)):
            hits = s.find(q, 10)
            self.assertEqual(hits[0].header, s.rows[i].header, q)
            self.assertLess(abs(hits[0].score - f * w), 2e-9, q)
        self.assertIsNone(self.hit(s.find('block cut', 10), s.rows[i].header))
        s = flat(self.tree(FUZZY_FX))  # default prefix: `block` begins blockcuttree; the join `blockcut` takes no prefix
        hits = s.find('block cut', 10)
        self.assertEqual([x.header for x in hits], [s.rows[i].header])
        self.assertLess(abs(hits[0].score - M.PREFIX * o_score([r.fields for r in s.rows], i, [('blockcuttree', 1.0)], M.WEIGHTS, M.B, M.K1)), 2e-9)

    def test_segmentation(self):
        s = flat(self.tree(FUZZY_FX))
        seg = f'{GR}/08-segmenttree.hpp'
        self.assertEqual(next(r for r in s.rows if r.header == seg).fields['stem'], ['segmenttree', 'segment', 'tree'])
        self.assertIsNotNone(self.hit(s.find('segment', 10), seg))
        self.assertIn(seg, [x.header for x in s.find('segment tree', 2)])
        self.assertEqual(next(r for r in s.rows if r.header.endswith('11-lowbit.hpp')).fields['stem'], ['lowbit'])  # a known word

    def test_phrase(self):
        root = self.tree(BM25_FX)
        p0, p1 = flat(root, phrase=0.0), flat(root, phrase=0.8)
        fields = [{f: r.fields[f] for f in M.FIELDS} for r in p0.rows]
        bonus = 0.8 * (o_idf(fields, 'gamma') + o_idf(fields, 'ray')) / 2
        a, b = ({x.header: x.score for x in s.find('gamma ray', 10)} for s in (p0, p1))
        self.assertEqual(set(a), set(b))
        for hd in a:
            self.assertLess(abs(b[hd] - a[hd] - (bonus if hd.endswith('02-gamma.hpp') else 0)), 3e-9, hd)
        a, b = ({x.header: x.score for x in s.find('ray gamma', 10)} for s in (p0, p1))
        self.assertEqual(a, b)  # not adjacent in that order
        root = self.tree(PHRASE_FX)  # `ome` begins omega (3 rows) and omelet (1 row): the bonus takes the larger idf
        p0, p1 = flat(root, phrase=0.0), flat(root, phrase=0.8)
        fields = [{f: r.fields[f] for f in M.FIELDS} for r in p0.rows]
        bonus = 0.8 * (max(o_idf(fields, 'omega'), o_idf(fields, 'omelet')) + o_idf(fields, 'zeta')) / 2
        self.assertGreater(o_idf(fields, 'omelet'), o_idf(fields, 'omega'))
        a, b = ({x.header: x.score for x in s.find('ome zeta', 10)} for s in (p0, p1))
        self.assertEqual(set(a), {f'{MA}/0{k}-{c}.hpp' for k, c in enumerate('abcd', 1)})
        for hd in a:
            self.assertLess(abs(b[hd] - a[hd] - (bonus if hd.endswith('01-a.hpp') else 0)), 3e-9, hd)

    def test_python_modules(self):
        root = self.tree(PY_FX)  # a module identical to its header scores the same and takes no name class
        for q in ('kappaFn', 'fn', 'kappa'):  # exact name, word run, own stem
            hits = flat(root).find(q, 10)
            self.assertEqual([x.header for x in hits], [f'{MA}/05-kappa.hpp', '08-Python/_05_kappa.py'], q)
            self.assertEqual(hits[0].score, hits[1].score, q)
        hits = M.Search(root).find('kapa', 10)  # both concept hits: equal scores, then the header
        self.assertEqual([x.header for x in hits], [f'{MA}/05-kappa.hpp', '08-Python/_05_kappa.py'])
        self.assertEqual(hits[0].score, hits[1].score)
        self.assertTrue(M.show(hits)[1].endswith('  Python') and not M.show(hits)[0].endswith('Python'))
        root = self.tree(PY2_FX)
        s = flat(root, weights={**M.WEIGHTS, 'qual': 4.0})
        hits = s.find('lambdaFn', 10)  # the module's name equals the query: it still ranks by score among the concept hits
        lam = self.hit(hits, '08-Python/_06_lam.py')
        self.assertEqual(lam.name, 'lambdaFn')
        self.assertEqual(hits[0].header, f'{MA}/08-lambda_fn.hpp')  # own stem: the name class comes first
        self.assertGreater(lam.score, hits[0].score)
        self.assertEqual([x.score for x in hits[1:]], sorted((x.score for x in hits[1:]), reverse=True))
        hits = s.find('omega', 10)  # a word run in the module's name lifts it above no header
        self.assertEqual([(x.header, x.name) for x in hits], [(f'{MA}/09-omega.hpp', 'build'), (f'{MA}/07-nu.hpp', 'solveAll'),
                                                              ('08-Python/_07_nu.py', 'omegaSolve')])
        self.assertGreater(hits[1].score, hits[2].score)

    def test_display_name(self):
        s = flat(self.tree(RANK_FX))
        name = lambda q, n: self.hit(s.find(q, 20), f'{DS}/{n}').name
        self.assertEqual(name('flows', '10-max_flow.hpp'), 'maxFlowRun')  # equal query weight: more header-stem words
        self.assertEqual(name('flows', '11-other.hpp'), 'pushFlow')  # full tie: the first
        self.assertEqual(name('delta gamma', '13-gamma.hpp'), 'gammaDelta')  # most query weight
        self.assertEqual(name('wombat', '12-wombat.hpp'), f'{DS}/12-wombat.hpp')  # no names: the header
        self.assertEqual(name('sum', '03-prefix.hpp'), 'rangeSum')  # shortest whole-word run
        self.assertEqual(name('sum', '01-fenwick.hpp'), 'Fenwick::sum')  # exact last component
        s = flat(self.tree(NAME_FX))
        name = lambda q, n: self.hit(s.find(q, 20), f'06-Miscellaneous/{n}').name
        self.assertEqual(name('x', '01-seg_tree.hpp'), 'segtreeBuild')  # equal weight: the joined stem `segtree` decides
        self.assertEqual(name('miller rabin', '05-prime.hpp'), 'isPrime')  # only its qualifier matches
        self.assertEqual(name('kadane', '08-sequence_algorithms.hpp'), '06-Miscellaneous/08-sequence_algorithms.hpp')  # alias
        self.assertEqual(name('mst', '09-mst.hpp'), 'kruskal')  # own stem: a name although none shares a query word


class Cli(Fixtures):
    def run_main(self, argv, root):
        out, err = io.StringIO(), io.StringIO()
        boom = lambda *a, **k: (_ for _ in ()).throw(AssertionError('Index constructed'))
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err), patch.object(G, 'ROOT', root), \
                patch.object(G, 'Index', boom):
            try:
                code = G.main(argv)
            except SystemExit as e:
                code = e.code
        return code, out.getvalue(), err.getvalue()

    def test_cli_find(self):
        root = self.tree(CLI_FX)
        want = M.show(M.Search(root).find('dijkstra', 10), G.EXCLUDED)
        self.assertEqual(len(want), 4)
        self.assertTrue(want[0].startswith(o_show('dijkstra', f'{GR}/05-shortest_path.hpp', 'Basic', 'verified')))
        self.assertEqual(self.run_main(['--find', 'dijkstra'], root), (0, '\n'.join(want) + '\n', ''))
        self.assertEqual(self.run_main(['--find', 'dijkstra', '-n', '2'], root), (0, '\n'.join(want[:2]) + '\n', ''))
        self.assertEqual(self.run_main(['--find', 'zzzzqq'], root), (0, 'no match\n', ''))
        two = M.show(M.Search(root).find('shortest path', 10), G.EXCLUDED)
        self.assertTrue(two and two[0].startswith(o_show('dijkstra', f'{GR}/05-shortest_path.hpp', 'Basic', 'verified')), two)
        self.assertEqual(self.run_main(['--find', 'shortest', 'path'], root), (0, '\n'.join(two) + '\n', ''))
        tpl = self.tree(TPL_FX)
        marked = M.show(M.Search(tpl).find('chmin', 10), G.EXCLUDED)
        self.assertIn(o_show('chmin', '01-Core/01-template.hpp', 'Basic', 'verified', G.EXCLUDED), marked)
        self.assertEqual([l.endswith('  in template') for l in marked], [l.startswith('chmin ') for l in marked])
        self.assertEqual(self.run_main(['--find', 'chmin'], tpl), (0, '\n'.join(marked) + '\n', ''))
        many = self.tree(MANY_FX)
        for argv, k in ((['--find', 'zork'], 10), (['--find', 'zork', '-n', '12'], 12), (['--find', 'zork', '-n', '11'], 11)):
            code, out, err = self.run_main(argv, many)
            self.assertEqual((code, len(out.splitlines()), err), (0, k, ''), argv)
        for argv, msg in ((['--find', 'dijkstra', '-n', '0'], '-n K needs --find'), (['dsu', '-n', '3'], '-n K needs --find'),
                          (['-n', '3'], 'give names, --into FILE or --find WORDS'), (['--find'], 'error'),
                          (['dsu', '--find', 'x'], 'give names, --into FILE or --find WORDS')):
            code, out, err = self.run_main(argv, root)
            self.assertEqual((code, out), (2, ''), argv)
            self.assertIn(msg, err, argv)

    def test_cli_unknown_name(self):
        root = self.tree(CLI_FX)
        stub = types.SimpleNamespace(ci=object(), resolve=lambda n: [], units=[], toplevel=lambda i: True, graph={})
        rows = M.show(M.Search(root).find('dijkstar', 3), G.EXCLUDED)
        block = '\n  closest rows (name, header, tier, status):' + ''.join('\n  ' + l for l in rows)
        self.assertEqual(len(rows), 3)
        self.assertTrue(rows[0].startswith(o_show('dijkstra', f'{GR}/05-shortest_path.hpp', 'Basic', 'verified')))
        for ix, head in ((stub, "unknown name 'dijkstar'"), (types.SimpleNamespace(ci=None, graph={}), 'no header defines dijkstar')):
            with self.assertRaises(G.GrabError) as cm:
                G.grab(['dijkstar'], root, ix)
            msg = str(cm.exception)
            self.assertTrue(msg.startswith(head) and msg.endswith(block), msg)
            self.assertEqual(msg.count('\n'), 4, msg)
            if ix.ci is None:  # the fallback searches all the names: only the second matches a row
                two = M.show(M.Search(root).find('qqqzz dijkstar', 3), G.EXCLUDED)
                self.assertTrue(two and two[0].startswith(rows[0][:77]), two)
                with self.assertRaises(G.GrabError) as cm:
                    G.grab(['qqqzz', 'dijkstar'], root, ix)
                self.assertEqual(str(cm.exception), 'no header defines qqqzz, dijkstar\n  closest rows (name, header, tier, status):'
                                 + ''.join('\n  ' + l for l in two))
            tpl = self.tree(TPL_FX)
            marked = M.show(M.Search(tpl).find('chmn', 3), G.EXCLUDED)
            self.assertIn(o_show('chmin', '01-Core/01-template.hpp', 'Basic', 'verified', G.EXCLUDED), marked)
            with self.assertRaises(G.GrabError) as cm:
                G.grab(['chmn'], tpl, ix)
            self.assertTrue(str(cm.exception).endswith(''.join('\n  ' + l for l in marked)) and 'in template' in str(cm.exception))
            with self.assertRaises(G.GrabError) as cm:
                G.grab(['dijkstar'], self.tree({}), ix)
            self.assertTrue(str(cm.exception).startswith(head) and '\n' not in str(cm.exception), str(cm.exception))


def o_index_rows():
    """(folder/header) of every inventory table line of 01-08, read without the module."""
    out = []
    for index in sorted(ROOT.glob('0[1-8]-*/00-index.md')):
        for line in index.read_text(encoding='utf-8').splitlines():
            m = re.match(r'\| `([^`|]+\.(?:hpp|py))`', line)
            if m and len(re.split(r'(?<!\\)\|', line.strip())) >= 5:
                out.append(f'{index.parent.name}/{m[1]}')
    return out


def typo_variant(query, rng):
    """query with one random edit inside its longest word of 5 letters or more, the first letter kept; None without one."""
    ws = [m for m in re.finditer(r'[A-Za-z]+', query) if len(m[0]) >= 5]
    if not ws:
        return None
    m = max(ws, key=lambda m: len(m[0]))
    w = m[0]
    letter = lambda c: rng.choice('ABCDEFGHIJKLMNOPQRSTUVWXYZ' if c.isupper() else 'abcdefghijklmnopqrstuvwxyz')
    op = rng.choice(('swap', 'delete', 'substitute', 'insert'))
    swaps = [p for p in range(1, len(w) - 1) if w[p] != w[p + 1]]
    if op == 'swap' and swaps:
        p = rng.choice(swaps)
        v = w[:p] + w[p + 1] + w[p] + w[p + 2:]
    elif op == 'delete':
        p = rng.randrange(1, len(w))
        v = w[:p] + w[p + 1:]
    elif op == 'insert':
        p = rng.randint(1, len(w))
        v = w[:p] + letter(w[p - 1]) + w[p:]
    else:
        p = rng.randrange(1, len(w))
        v = w[:p] + next(c for c in iter(lambda: letter(w[p]), None) if c.lower() != w[p].lower()) + w[p + 1:]
    return query[:m.start()] + v + query[m.end():]


class Real(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.s = M.Search(ROOT)
        cls.headers = {r.header for r in cls.s.rows}

    def test_repo_rows(self):
        self.assertEqual(sorted(r.header for r in self.s.rows), sorted(o_index_rows()))
        self.assertEqual({r.header: r.tier for r in self.s.rows if r.tier not in M.TIERS}, {})
        self.assertEqual({r.tier for r in self.s.rows if r.folder == '01-Core'}, {'Basic', 'Advanced'})

    def test_repo_stems(self):
        same = {}
        for r in self.s.rows:
            same.setdefault(tuple(r.key), []).append(r.header)
        bad, n = [], 0
        for r in self.s.rows:
            if not re.match(r'0[1-7]-', r.header) or r.status == 'planned' or not (ROOT / r.header).is_file():
                continue
            n += 1
            q = re.sub(r'^\d{2}-', '', Path(r.header).stem)
            top = [h.header for h in self.s.find(q, 3)]
            if r.header not in top or (top[0] != r.header and sum(not h.endswith('.py') for h in same[tuple(r.key)]) == 1):
                bad.append(f'{q!r}: want {r.header}, top 3 {top}')
        self.assertGreater(n, 100)
        self.assertEqual(bad, [], '\n'.join(bad))

    def test_repo_examples(self):
        first = lambda q, k=1: [(x.header, x.tier, x.status) for x in self.s.find(q, k)]
        self.assertEqual(first('bipartite')[0][0], '04-Graphs/02-traversal.hpp')
        self.assertEqual(first('binary search')[0][0], '05-Mathematics/02-search_algorithms.hpp')
        self.assertEqual([x[1:] for x in first('hash', 3)], [('Basic', 'verified')] * 3, first('hash', 3))
        self.assertEqual(first('debug')[0][0], '01-Core/02-debug.hpp')

    def corpus(self):
        data = json.loads(CORPUS.read_text(encoding='utf-8'))['queries']
        lc = json.loads(LEDGER.read_text(encoding='utf-8'))['records']
        out = [(q['source'], q['query'], q['targets']) for q in data]
        out += [('lc', r['problem'].rsplit('/', 1)[-1].replace('_', ' '), r['targets']) for r in lc]
        gen = ('cp',) if MODE == 'quick' else ('lc', 'cp')
        for seed in range(SEED, SEED + (ROUNDS if MODE == 'stress' else 1)):
            rng = random.Random(seed)
            out += [('generated', v, t) for src, q, t in list(out) if src in gen and src != 'generated'
                    for v in [typo_variant(q, rng)] if v]
        return out

    def test_repo_corpus(self):
        queries = self.corpus()
        missing = sorted({t for _, _, ts in queries for t in ts if t not in self.headers})
        self.assertEqual(missing, [], 'corpus targets that are not inventory rows')
        stats, misses = {}, []
        for src, q, targets in queries:
            top = [h.header for h in self.s.find(q, 10)]
            rank = next((k + 1 for k, h in enumerate(top) if h in targets), None)
            st = stats.setdefault(src, [0, 0, 0.0])
            st[0] += 1
            st[1] += bool(rank and rank <= 5)
            st[2] += 1 / rank if rank else 0.0
            if not rank or rank > 5:
                misses.append((src, q, targets, top[:3]))
        lines, failed = [f'{"source":10} {"queries":>7} {"recall@5":>9} {"MRR@10":>7}  floors'], []
        for src in sorted(stats, key=lambda x: (x not in FLOORS, list(FLOORS).index(x) if x in FLOORS else x)):
            n, hit, rr = stats[src]
            rec, mrr = hit / n, rr / n
            floor = FLOORS.get(src)
            ok = floor is None or (rec >= floor[0] and mrr >= floor[1])
            failed += [] if ok else [src]
            lines.append(f'{src:10} {n:7} {rec:9.3f} {mrr:7.3f}  ' + (f'{floor[0]:.2f} {floor[1]:.2f}' if floor else '-')
                         + ('' if ok else '  BELOW'))
        print('\n' + '\n'.join(lines), flush=True)
        if failed:
            print('misses (source, query, targets, top 3):', flush=True)
            for src, q, t, top in misses:
                if src in failed:
                    print(f'  {src}: {q!r} -> {t}; top 3 {top}', flush=True)
        self.assertEqual(failed, [], 'sources below FLOORS')
        self.assertEqual(set(FLOORS) - set(stats), set(), 'FLOORS sources without queries')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=3, help='stress: multiple of the full random cases and generated typo seeds')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, max(1, args.rounds)
    suite = unittest.TestSuite()
    for case in (Units, Ranking, Cli, Real):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case))
    print(f"search checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}",
          flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
