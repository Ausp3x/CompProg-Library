#!/usr/bin/env python3
"""Checks for the concept search of 09-Contest Testing/_11_search.py and its CLI in 09-Contest Testing/10-grab.py
(`--find WORDS`, `-n K`, the closest rows appended to grab's unknown-name errors).

Fixture inventories (00-index.md tables, Docs evidence files) are written to per-process temporary directories; the
real repository is only read. quick: every fixture test, the T2S file checks, the repository stem, Chinese and corpus
checks, with 400 random distance cases, 200 segment cases, 300 shortened cases, 300 random texts for words (letters,
digits, separators, Traditional and Simplified characters, fullwidth forms), 300 simplified cases, and one generated
typo set (seed --seed) over the cp queries. full: quick with ten times each random count, and one generated set over
the lc and cp queries. stress: full with --rounds (default 3) times the full random cases and --rounds generated sets
(seeds --seed .. --seed + rounds - 1) over lc and cp. Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED.
--tune all|even [--start JSON] [--jobs N] runs no check: it searches the ranking constants on the full-mode --seed 1
list (00-search.md, Weights and cutoffs) and prints them, with the FLOORS they give or the odd half's results.
Oracles (written here, independent of the module): hand tables for names, aliases, rows, stem (derived from the Porter2
rules), simplified and shortened; the Snowball Porter2 document's published R1/R2 and short-syllable examples; a
character scanner for words over NFKC text with Chinese characters told by their Unicode names, after a scan
with a parenthesis count for complexity expressions; the T2S file parsed here
with a recursive longest match per dictionary and a scanning segmenter for simplified, and hand tables derived from its
dictionary lines; Python's gb2312 codec for the mainland character set and Traditional runs; a memoized
recursion of the restricted Damerau (optimal string alignment) definition for distance, itself checked on known values;
an enumeration of every split into parts of PART letters or more with known stems for segment and of every split into pieces for
shortened; BM25F recomputed from its formula over hand-written field token lists (character pairs as tokens for
Chinese); hand-derived orders on fixtures whose scores run against the name classes; Traditional and Simplified
spellings of one query giving identical results on the repository, and Taiwan words the same first row as the
mainland ones; a scanner cutting alias items around code spans; a substring scan of the documents' Chinese runs for the regional
phrases they hold; unicodedata.decomposition followed for Latin letters; an independent table-line count of the
inventories; recall@5 and MRR@10 per corpus source against FLOORS. Failing random rounds print the seed, mode, round
and input.

Feature map:
cells, split, operations (moved parser, one case each) -> test_cells_split_operations;
names (type heads and Type::member, ctor, alias heads, camelCase function head, label head, `A as B and C`, operators,
template arguments, duplicates, order) -> test_names;
rows (0N-Folder/00-index.md only, tier from ## sections, 4-column 01-Core Basic cell starting with
the word yes -> Basic, else Advanced whatever the section, module evidence documents Docs/<module stem>.md, 08-Python modules, status
prefix, stem field, aliases only from <folder>/Docs/<NN-name>.md, folder field) -> test_rows_fixture;
aliases (sections, Aliases:/Catalog aliases:/dated/bold/Search aliases/list lines, labels dropped also in sections, not
`with \\`FIXED\\``, not ordinary paragraphs, URLs and link targets dropped) -> test_aliases; alias items (cut at , ; : ( ) |,
their fullwidth forms, 、 and line ends, not inside a code span; a piece before a colon a label) -> test_alias_items,
test_rows_fixture;
words (NFKC, diacritics dropped from Latin letters in every block, Greek and Cyrillic kept, complexity expressions
dropped (nested, unclosed, after a letter), camelCase, stop words,
Chinese runs cut from letters and digits, Simplified, overlapping pairs or a lone character, with regional every run's
new regional pairs once each (Taiwan words in characters both scripts share too), with the whole or a given regional
table, mainland text kept as written, no stop words for Chinese) ->
test_words_table, test_words_random; chinese -> test_chinese;
stem (plurals, -ing by Porter2 step 1b with the at/bl/iz and short-word e and the 3-letter undoubling floor, final e by
step 5, IRREGULAR with unites and uniting) -> test_stem_table, test_stem_conflation; vowels, region, short -> test_stem_regions;
T2S file (header, line format, the six dictionaries with unique keys, tc and vc single characters that change, mw in
converted characters with each class word its own, Chinese characters only, tc keeping GB2312 in GB2312, mainland class
words fixed) and tables (with the Traditional characters) -> test_t2s_file, test_t2s_gb2312, test_t2s_mainland_fixed;
traditional, segments, replace, simplified (both forms; rp before vp and vc, OpenCC's segments) -> test_simplified_table, test_simplified_random; distance ->
test_distance_known, test_distance_random; segment (parts known by their stems, at most one shorter than 3, ties: no short part, then the longest parts from the
end) ->
test_segment_table, test_segment_random; shortened -> test_shortened_table, test_shortened_random;
tolerance -> test_tolerance;
BM25F score (default and custom weights, b, k1) -> test_bm25_one_word; name classes (exact, last :: component, header
stem for any query, whole-word runs for one-word queries only, not `sam` in `same`, concept) -> test_classes_*; status
and tier priors (custom, missing keys, defaults, order flips in and outside the name classes) -> test_priors; tie order
and n truncation -> test_tie_order;
tuned constants pinned against TUNED, --tune's parameters and bounds -> test_tuned_constants; entries and notes -> test_entries_notes;
typo (factor at distance 1 and 2, 3-letter tokens) -> test_typo; typo and prefix together (the larger factor) ->
test_typo_and_prefix; surrounding whitespace, fullwidth and accented names -> test_strip; prefix -> test_prefix; joins (exact, misspelt, no
prefix at the default factor) -> test_joins; stem segmentation (known words stay whole, stop words split and yield
nothing) -> test_segmentation; phrase
bonus (the adjacent alternatives at their factors, adjacent within a Chinese run's own or regional reading) -> test_phrase; modules without a name class (ranked by score among the concept
hits even when their name equals or contains the query) -> test_python_modules; name classes (own stem, exact
name, alias, word run) only for verified, partial and existing-unverified rows, each ordered by tier, status, then score, the concept class by score whatever the
tier -> test_class_gate_and_order; display name (also a qualifier match,
an alias-only match showing the header, the own-stem exception, the joined stem tiebreak) -> test_display_name; absent
rows -> test_tie_order, test_typo; show (`  Python`, `  in template`) -> test_show; shortened names in the word-run
class (one-word queries of 3-letter pieces only) -> test_shortened_class; Chinese ranking (alias pairs without regional
pairs, a query's regional pairs reaching mainland aliases without doubling a mainland word, the regional phrases a run of a
row's text holds in one form (any field) left out of a query's reading, Traditional and Simplified queries alike, a
lone character standing for its pairs at the prefix factor and itself, scaled to its own idf, no joins over Chinese
words, phrase bonus over pairs and lone characters, the prefix range past the BMP) -> test_chinese_ranking; alias class
(an item equal to the query or its regional reading, after exact names and before word runs, gated, simplest first)
-> test_alias_class;
CLI --find (one or more words), no match, default 10 rows, -n, argument errors, no Index, `in template` marker ->
test_cli_find; unknown-name rows (the marker, libclang stub, no inventories, no-libclang fallback over all the names) ->
test_cli_unknown_name;
repository rows, own stems (top 3, rank 1 unless another C++ row shares the stem tokens), the user's examples (bipartite, binary search, hash, debug),
Traditional and Simplified query pairs (Taiwan words, also in shared characters: the same first row), the regional
phrases the documents hold (pinned and scanned), canonical rows first for their Chinese names, Taiwan 演算法 queries,
corpus metrics and generated typos ->
test_repo_rows, test_repo_stems, test_repo_examples, test_repo_chinese, test_repo_corpus.
"""
import argparse
import contextlib
import functools
import importlib.util
import io
import json
import math
import multiprocessing
import os
from pathlib import Path
import random
import re
import shutil
import sys
import tempfile
import types
import unicodedata
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

if any(a == '--tune' or a.startswith('--tune=') for a in sys.argv[1:]):
    os.environ['CP_TEST_PREFLIGHT'] = '0'  # a tune is no test mode: a pre-flight quick run would tune twice
_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
SEARCH_PY = ROOT / '09-Contest Testing/_11_search.py'
GRAB_PY = ROOT / '09-Contest Testing/10-grab.py'
T2S = ROOT / '09-Contest Testing/_11_search_t2s.tsv'
CORPUS = ROOT / '96-Local Testing/00-Tools/14-search_corpus.json'
LEDGER = ROOT / '00-Guidelines/Ledgers/library-checker-coverage.json'
FLOORS = {'lc': (0.88, 0.79), 'cp': (0.96, 0.86), 'oi': (0.81, 0.68), 'abbr': (0.89, 0.86), 'typo': (0.90, 0.90),
          'simple': (0.92, 0.83), 'py': (0.94, 0.78), 'generated': (0.88, 0.79), 'zh': (0.69, 0.66), 'zhs': (0.87, 0.84),
          'zht': (0.84, 0.81)}
# (recall@5, MRR@10) per corpus source: the 2026-11-08 full-mode results (--seed 1) after the third run's retune minus
# 0.02, rounded down; typo and generated keep their fixed floors
TUNED = {  # the 00-search.md table; the formula tests read the module, this pins its values
    'WEIGHTS': {'stem': 1.0, 'ident': 1.0, 'ops': 1.0, 'qual': 0.5, 'aliases': 1.5, 'folder': 4.5}, 'B': 0.5, 'K1': 1.2,
    'TYPO': {1: 0.85, 2: 0.8}, 'PREFIX': 1.0, 'PHRASE': 1.0,
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
def o_han(ch):
    """Whether ch is a Chinese character, by its Unicode name."""
    return unicodedata.name(ch, '').startswith(('CJK UNIFIED IDEOGRAPH-', 'CJK COMPATIBILITY IDEOGRAPH-'))


@functools.cache
def o_t2s():
    """The T2S file read here: {dictionary: {key: value}}."""
    d = {}
    for line in T2S.read_text(encoding='utf-8').split('\n')[1:]:
        if line:
            kind, k, v = line.split('\t')
            d.setdefault(kind, {})[k] = v
    return d


def o_gb(c):
    """Whether c is in GB2312, the mainland Simplified set, by Python's codec."""
    try:
        return bool(c.encode('gb2312'))
    except UnicodeEncodeError:
        return False


def o_traditional(text):
    return any(not o_gb(c) and (c in o_t2s()['tc'] or c in o_t2s()['vc']) for c in text)


def o_replace(text, names):
    """Recursively: the longest front key of the first dictionary (named, or given) holding one, as its value, then the
    rest; with none, the first character as it is, then the rest."""
    if not text:
        return ''
    for d in (o_t2s()[n] if isinstance(n, str) else n for n in names):
        k = max((text[:j] for j in range(1, len(text) + 1) if text[:j] in d), key=len, default='')
        if k:
            return d[k] + o_replace(text[len(k):], names)
    return text[0] + o_replace(text[1:], names)


def o_segments(text):
    """OpenCC's segmentation by a scan: each longest ts key a segment, the characters between them gathered."""
    ts, out, gap, i = o_t2s()['ts'], [], '', 0
    while i < len(text):
        j = max((j for j in range(i + 1, len(text) + 1) if text[i:j] in ts), default=None)
        out, gap, i = (out, gap + text[i], i + 1) if j is None else (out + [gap] * bool(gap) + [text[i:j]], '', j)
    return out + [gap] * bool(gap)


def o_simplified(text, regional=False):
    """A Traditional text (a character outside GB2312 that tc or vc changes), or with regional (True: rp; or a given
    dictionary in its place) any text, segment by segment through rp (regional), vp, vc, then ts, tc, as OpenCC's tw2s,
    hk2s and tw2sp chain them; then any text through tc, then mw."""
    if regional is not False or o_traditional(text):
        rp = ['rp'] if regional is True else [] if regional is False else [regional]
        text = ''.join(o_replace(o_replace(p, rp + ['vp', 'vc']), ['ts', 'tc']) for p in o_segments(text))
    return o_replace(o_replace(text, ['tc']), ['mw'])


def o_base(ch):
    """ch's canonical decomposition followed to its first character when that is another, Latin, letter (by its
    Unicode name); else ch."""
    b = ch
    while unicodedata.decomposition(b) and not unicodedata.decomposition(b).startswith('<'):
        b = chr(int(unicodedata.decomposition(b).split()[0], 16))
    return b if b != ch and unicodedata.name(b, '').startswith('LATIN') else ch


def o_unbounded(text):
    """text without complexity expressions, by a scan: O, o, Θ or Ω after no letter or digit, then ( and all up to its
    matching ), nested parentheses counted, replaced by a space; an unclosed one stays."""
    out, i = '', 0
    while i < len(text):
        if text[i] in 'OoΘΩ' and text[i + 1:i + 2] == '(' and not (i and text[i - 1].isalnum()):
            depth = 0
            for j in range(i + 1, len(text)):
                depth += (text[j] == '(') - (text[j] == ')')
                if depth == 0:
                    out, i = out + ' ', j + 1
                    break
            else:
                out, i = out + text[i], i + 1
            continue
        out, i = out + text[i], i + 1
    return out


def o_words(text, stop=(), regional=False):
    """Character scanner over NFKC text with Latin letters' diacritics dropped: runs of Chinese characters, letters or
    digits, a letter run cut before an ASCII capital after a lower-case letter, and before the last capital of a capital
    run followed by a lower-case letter; a Chinese run gives the overlapping pairs of its Simplified form (one character
    alone), with regional (as in o_simplified) then the new pairs of its regional form, never dropped as a stop word;
    complexity expressions first dropped (o_unbounded)."""
    text = o_unbounded(''.join(map(o_base, unicodedata.normalize('NFKC', text))))
    out, cur, kind = [], '', None
    low, up = (lambda c: 'a' <= c <= 'z'), (lambda c: 'A' <= c <= 'Z')
    for k, ch in enumerate(text):
        c = 'C' if o_han(ch) else 'L' if ch.isalpha() else 'D' if ch.isdigit() else None
        cut = c != kind or c is None or (up(ch) and k and (low(text[k - 1]) or
                                                          (up(text[k - 1]) and k + 1 < len(text) and low(text[k + 1]))))
        if cut:
            out.append((cur, kind))
            cur = ''
        cur, kind = (cur + ch if c else ''), c
    out.append((cur, kind))
    res = []
    for w, c in out:
        if w and c == 'C':
            pairs = lambda s: [s] if len(s) == 1 else [s[i:i + 2] for i in range(len(s) - 1)]
            base, more = pairs(o_simplified(w)), pairs(o_simplified(w, regional)) if regional is not False else []
            res += base + [p for k, p in enumerate(more) if p not in base and p not in more[:k]]
        elif w and w.lower() not in stop:
            res.append(w.lower())
    return res


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
    """Every split of word into parts of PART letters or more whose stems are known."""
    if not word:
        return [[]]
    return [[word[:k]] + rest for k in range(M.PART, len(word) + 1) if M.stem(word[:k]) in known for rest in o_splits(word[k:], known)]


def o_shortened(q, ws):
    """Whether some split of q into two or more pieces of 3 letters or more has its pieces begin consecutive words of
    ws, by enumerating the splits (compositions of len(q)) and the starting word."""
    def pieces(s):
        return [[s]] + [[s[:k]] + r for k in range(3, len(s) - 2) for r in pieces(s[k:])] if len(s) >= 3 else []
    return any(len(p) >= 2 and all(ws[s + j].startswith(x) for j, x in enumerate(p))
               for p in pieces(q) for s in range(len(ws) - len(p) + 1))


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


def o_items(text):
    """Each alias's tokens, by a scan: the aliases text cut at , ; : ( ) | and their fullwidth forms, 、 and line ends
    outside code spans (a backtick, then the next on its line); a piece cut at a colon is a label, no alias. The text is
    NFKC-normalized with Latin diacritics dropped and complexity expressions removed first."""
    out, cur, i, text = set(), '', 0, o_unbounded(''.join(map(o_base, unicodedata.normalize('NFKC', text)))) + '\n'
    while i < len(text):
        close = text.find('`', i + 1) if text[i] == '`' else -1
        if close > 0 and '\n' not in text[i:close]:  # a code span, whole
            cur, i = cur + text[i:close + 1], close + 1
            continue
        if text[i] in ',;:()|\n，；：（）、｜':
            if text[i] not in ':：':
                out.add(tuple(M.stem(w) for w in o_words(cur, M.STOP)))
            cur = ''
        else:
            cur += text[i]
        i += 1
    return out - {()}


def o_held(texts):
    """The rp keys that some text's run of Chinese characters, in one form, holds."""
    runs = [o_simplified(r) for t in texts for r in re.findall(r'\S+', ''.join(c if o_han(c) else ' ' for c in t))]
    return {k for k in o_t2s()['rp'] if any(k in r for r in runs)}


def o_lone(fields, c):
    """The idf of a Chinese character over the rows holding it in any token."""
    df = sum(any(c in t for x in r for t in r[x]) for r in fields)
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

STOPSEG_FX = {f'{GR}/00-index.md': table([('`01-meetinthemiddle.hpp`', 'split', 'verified'), ('`02-meetonit.hpp`', 'join', 'verified'),
                                          ('`03-words.hpp`', 'meet; middle', 'verified'), ('`04-intothe.hpp`', 'other', 'verified')])}

TPL_FX = {'01-Core/00-index.md': table([('`01-template.hpp`', 'yes', 'macros: fi, se; chmin, chmax', 'verified'),
                                            ('`02-debug.hpp`', 'yes', 'debug, trace (macros)', 'verified')], 4),
          f'{GR}/00-index.md': table([('`05-relax.hpp`', 'chminEdge', 'verified')])}

STRIP_FX = {'06-Miscellaneous/00-index.md': '## Basic\n\n' + table([('`02-b.hpp`', 'zapAll', 'verified'), ('`03-c.hpp`', 'other', 'verified')])
                                         + '## Advanced\n\n' + table([('`01-a.hpp`', 'zap', 'verified')]),
            '06-Miscellaneous/Docs/03-c.md': 'Aliases: zap\n'}

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

SHORT_FX = {f'{DS}/00-index.md': '## Basic\n\n' + table([
    ('`01-segment_tree.hpp`', 'SegmentTree: ctor, query', 'verified'),
    ('`02-lazy_segment_tree.hpp`', 'LazySegmentTree: apply', 'verified'),
    ('`03-beats.hpp`', 'SegTreeBeats: chmin', 'verified'),
    ('`04-walk.hpp`', 'walk (segtree segtree segtree segtree)', 'verified')])}

DESERET = '\U00010428'  # a lower-case letter past the BMP
ZH_FX = {f'{DS}/00-index.md': '## Basic\n\n' + table([
    ('`01-segment_tree.hpp`', 'SegmentTree: query', 'verified'),
    ('`02-fenwick.hpp`', 'Fenwick: add', 'verified'),
    ('`03-dsu.hpp`', 'DSU: unite', 'verified'),
    ('`04-rooted.hpp`', 'Rooted: root', 'verified'),
    ('`05-deseret.hpp`', 'build', 'verified'),
    ('`06-seq.hpp`', 'run', 'verified')]),
    f'{DS}/Docs/01-segment_tree.md': 'Aliases: 线段树\n', f'{DS}/Docs/02-fenwick.md': 'Aliases: 树状数组, BIT\n',
    f'{DS}/Docs/03-dsu.md': 'Aliases: 並查集\n', f'{DS}/Docs/04-rooted.md': 'Aliases: 树\n',
    f'{DS}/Docs/05-deseret.md': f'Aliases: qux{DESERET}\n', f'{DS}/Docs/06-seq.md': 'Aliases: 序列\n'}
ZH_ALIASES = [['线段', '段树'], ['树状', '状数', '数组', 'bit'], ['并查', '查集'], ['树'], ['qux' + DESERET], ['序列']]
# hand-converted: an alias keeps its own pairs only (序列 gains no 串行); 並查集 is Traditional, so 并查集

ALIAS_FX = {f'{DS}/00-index.md': '## Basic\n\n' + table([
    ('`01-heavy.hpp`', 'Heavy: run', 'verified'),
    ('`02-plain.hpp`', 'Plain: query', 'verified'),
    ('`03-also.hpp`', 'Also: query', 'existing-unverified'),
    ('`04-later.hpp`', 'Later: query', 'planned'),
    ('`05-bitty.hpp`', 'BitOps: lowbit', 'verified'),
    ('`06-heap.hpp`', 'Heap: push', 'verified')]) + '## Advanced\n\n' + table([('`07-adv.hpp`', 'Adv: build', 'verified')]),
    f'{DS}/Docs/01-heavy.md': 'Aliases: 线段树合并, 线段树分裂, 线段树套线段树, 线段树优化建图, 线段树上二分\n',
    f'{DS}/Docs/02-plain.md': 'Aliases: zkw tree (OI Wiki: 线段树)\n', f'{DS}/Docs/03-also.md': 'Aliases: 线段树\n',
    f'{DS}/Docs/04-later.md': 'Aliases: 线段树\n', f'{DS}/Docs/06-heap.md': 'Aliases: BIT, priority queue\n',
    f'{DS}/Docs/07-adv.md': 'Aliases: 线段树; 后缀数组; plain\n'}


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
        self.assertEqual({r.header: r.items for r in M.rows(root) if r.items},
                         {f'{DS}/01-dsu.hpp': {('union', 'find'), ('disjoint', 'set')}, '08-Python/_03_mod.py': {('modular', 'exponent')}})
        self.assertEqual([f[h]['aliases'] for h in (f'{DS}/24-interval_set.hpp', f'{DS}/30-heavy_light.hpp')], [[], []])
        self.assertEqual(f[f'{DS}/24-interval_set.hpp']['stem'], ['interval', 'set'])
        self.assertEqual(f[f'{DS}/24-interval_set.hpp']['folder'], ['data', 'structur'])  # Porter2 step 5: e in R2
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

    def test_alias_items(self):
        doc = ('Aliases: alpha beta, gamma; delta: epsilon (zeta) eta，theta；iota：kappa（lambda）mu、nu | `get_max(x)`, '
               '`flow(s, t)` pi｜rho\n- xi omega `tick, sigma\n')
        root = self.tree({f'{DS}/00-index.md': table([('`01-x.hpp`', 'run', 'verified')]), f'{DS}/Docs/01-x.md': doc})
        want = {('alpha', 'beta'), ('gamma',), ('epsilon',), ('zeta',), ('eta',), ('theta',), ('kappa',), ('lambda',), ('mu',),
                ('nu',), ('get', 'max', 'x'), ('flow', 's', 't', 'pi'), ('rho',), ('xi', 'omega', 'tick'), ('sigma',)}
        # cut at , ; : ( ) |, their fullwidth forms, 、 and line ends, not inside a code span (a lone backtick is text);
        # delta and iota label what follows
        self.assertEqual((M.rows(root)[0].items, o_items(M.aliases(doc))), (want, want))
        for text in ('线段树 (OI Wiki: 線段樹), the segment trees', 'a, , b;;', '', 'Z 函数（扩展 KMP）、KMP算法',
                     '`f(a, b)`: label, `g(x` | h, x：y z', 'O(NM) `a`b`c, d', 'k-th root (O(log n)), Ｏ（NM） ops, O(n'):
            root = self.tree({f'{DS}/00-index.md': table([('`01-x.hpp`', 'run', 'verified')]), f'{DS}/Docs/01-x.md': f'Aliases: {text}\n'})
            self.assertEqual(M.rows(root)[0].items, o_items(text), text)

    def test_words_table(self):
        for text, want in [('LCPArray', ['lcp', 'array']), ('segTree2D', ['seg', 'tree', '2', 'd']),
                           ('max_flow-dinic', ['max', 'flow', 'dinic']), ('HLDecomposition', ['hl', 'decomposition']),
                           ('getXMLHttpRequest', ['get', 'xml', 'http', 'request']), ('k2Tree', ['k', '2', 'tree']),
                           ('mod998244353', ['mod', '998244353']), ('ModInt64', ['mod', 'int', '64']), ('A as B', ['b']),
                           ('x__y', ['x', 'y']), ('', []), ('Fenwick::sum', ['fenwick', 'sum']), ('operator+=', ['operator']),
                           ('the sum of a tree', ['sum', 'tree']),
                           ('Ｓｅｇ', ['seg']), ('ＳｅｇＴｒｅｅ１', ['seg', 'tree', '1']),  # fullwidth: NFKC first
                           ('KMP算法', ['kmp', '算法']), ('線段樹', ['线段', '段树']), ('线段树', ['线段', '段树']), ('樹', ['树']),
                           ('演算法', ['演算', '算法']), ('字串', ['字串']),  # GB2312 only: mainland text, as written
                           ('樹狀陣列', ['树状', '状阵', '阵列']), ('查詢', ['查询']),
                           ('什麼', ['什么']), ('什么', ['什么']), ('么半群', ['么半', '半群']), ('想像', ['想象']),
                           ('Prüfer', ['prufer']), ('PólyaEnumeration', ['polya', 'enumeration']), ('Möbius', ['mobius']),
                           ('ǖ', ['u']), ('Øx', ['øx']), ('x×y', ['x', 'y']), ('D´Esopo', ['d', 'esopo']),  # diacritics dropped
                           ('Thuật toán', ['thuat', 'toan']), ('Ǣ', ['æ']), ('Ωά', ['ωά']), ('й', ['й']),  # Latin letters only
                           ('x樹2線段b', ['x', '树', '2', '线段', 'b']), ('線段樹segTree', ['线段', '段树', 'seg', 'tree']),
                           ('樹 狀', ['树', '状']), ('\uf907', ['龟']),  # a compatibility ideograph through NFKC, then T2S
                           ('\U00020000', ['\U00020000']), ('線段 of 樹', ['线段', '树']),
                           ('Finding Bridges in O(N+M)', ['finding', 'bridges']), ('O(T(n) log n) deletion', ['deletion']),
                           ('lca o(1)', ['lca']), ('FOO(x) Θ(n²) Ω(1)', ['foo', 'x']), ('Ｏ（log n）', []),  # complexity dropped
                           ('O(n', ['o', 'n']), ('O(O(n)', ['o']), ('xO(1)', ['x', 'o', '1']), ('(O(1))z', ['z'])]:  # or not
            self.assertEqual((M.words(text), o_words(text, M.STOP)), (want, want), text)
        for text, want in [('字串', ['字串', '字符', '符串']), ('演算法', ['演算', '算法']),  # rp 字串 字符串, 演算法 算法
                           ('樹狀陣列', ['树状', '状阵', '阵列', '状数', '数组']), ('查詢', ['查询', '查找']),  # rp 陣列 數組, 查詢 查找
                           ('么半群', ['么半', '半群', '幺半']), ('什么', ['什么']), ('序列', ['序列', '串行']),  # vc 么 幺; vp 什么 什麼
                           ('线段树', ['线段', '段树']), ('KMP字串', ['kmp', '字串', '字符', '符串']),  # rp 序列 串行
                           ('字串字串', ['字串', '串字', '字串', '字符', '符串'])]:  # a new pair once
            self.assertEqual((M.words(text, regional=True), o_words(text, M.STOP, True)), (want, want), text)
        letters = [chr(c) for a, b in ((0xc0, 0x250), (0x1e00, 0x1f00), (0x370, 0x530)) for c in range(a, b)]  # Latin, Greek, Cyrillic
        self.assertEqual([c for c in letters if (M.words(c, ()), M.words('x' + c + 'y', ())) != (o_words(c), o_words('x' + c + 'y'))], [])
        rp = {k: v for k, v in o_t2s()['rp'].items() if k != '序列'}  # a regional table without 序列 串行
        self.assertEqual((M.words('子序列字串', regional=(rp, max(map(len, rp)))), o_words('子序列字串', M.STOP, rp)),
                         (['子序', '序列', '列字', '字串', '字符', '符串'],) * 2)
        self.assertEqual(M.words('the sum of a tree', ()), ['the', 'sum', 'of', 'a', 'tree'])
        self.assertEqual(M.words('A as B', stop=()), ['a', 'as', 'b'])
        self.assertEqual(M.words('線段 of 樹 tree', stop={'线段', '線段', '树', 'tree'}), ['线段', 'of', '树'])  # never a Chinese stop word

    def test_words_random(self):
        rng = random.Random(SEED)
        alphabet = list('aAbBzZ19_- :') + list('樹树線段後后陣列字串演算法序么') + ['Ａ', 'ｂ', '１', '\uf907'] + list('üÉǖØ×ạỹά') + list('Oo((())Θ')
        for r in range(cases(300)):  # fullwidth, compatibility, accented Latin letters, complexity expressions
            s = ''.join(rng.choice(alphabet) for _ in range(rng.randint(0, 12)))
            for regional in (False, True):
                self.assertEqual(M.words(s, (), regional), o_words(s, (), regional), f'seed={SEED} mode={MODE} round={r} text={s!r} regional={regional}')

    def test_chinese(self):
        for w in ('线段', '线', '\U00020000', '\uf907', '算法', 'seg', '2', '', 'ｓ', 'の', '\U00010428', '一'):
            self.assertEqual(M.chinese(w), bool(w) and o_han(w[0]), w)
        self.assertEqual([M.chinese(w) for w in ('线段', 'seg', '\U00020000')], [True, False, True])

    def test_stem_table(self):  # each derived by hand from the plural rules, Porter2 steps 1b (-ing) and 5 (final e)
        table_ = {'matches': 'match', 'vertices': 'vertex', 'queries': 'query', 'spanning': 'span', 'rolling': 'roll',
                  'strings': 'string', 'string': 'string', 'series': 'series', 'hashing': 'hash', 'classes': 'class',
                  'class': 'class', 'bus': 'bus', 'gas': 'gas', 'status': 'status', 'analysis': 'analysis', 'trees': 'tree',
                  'boxes': 'box', 'dishes': 'dish', 'processes': 'process', 'indices': 'index', 'matrices': 'matrix',
                  'species': 'species', 'ties': 'tie', 'tries': 'trie', 'trie': 'trie', 'sorting': 'sort', 'king': 'king',
                  'ring': 'ring', 'running': 'run', 'passing': 'pass', 'buzzing': 'buzz', 'agreeing': 'agre', 'agree': 'agre',
                  'freeing': 'free', 'free': 'free', 'filling': 'fill',
                  'ceiling': 'ceil', 'hashings': 'hash', 'axes': 'axe', 'sizes': 'size', 'buzzes': 'buzz', 'gases': 'gase', 'set': 'set',
                  'sets': 'set', 'ops': 'ops', 'bits': 'bit',
                  'ing': 'ing', 'sing': 'sing', 'bring': 'bring', 'thing': 'thing',  # no vowel before -ing
                  'doing': 'do', 'going': 'go',  # a vowel just before -ing
                  'sliding': 'slide', 'slide': 'slide', 'using': 'use', 'coding': 'code', 'scaling': 'scale', 'hoping': 'hope',
                  'sizing': 'size',  # a short word gets its e back
                  'rotating': 'rotat', 'rotate': 'rotat', 'doubling': 'doubl', 'double': 'doubl',  # at/bl, then step 5
                  'padding': 'pad', 'mapping': 'map', 'programming': 'program', 'hopping': 'hop', 'adding': 'add', 'add': 'add',
                  'stuffing': 'stuff', 'diffing': 'diff',  # undoubling: bb dd gg mm nn pp rr tt, 3 letters left at least
                  'merging': 'merg', 'merge': 'merg', 'deleting': 'delet', 'delete': 'delet', 'dividing': 'divid',
                  'divide': 'divid', 'computing': 'comput', 'compute': 'comput', 'bridging': 'bridg', 'bridge': 'bridg',
                  'sampling': 'sampl', 'sample': 'sampl', 'scheduling': 'schedul', 'schedule': 'schedul', 'cycling': 'cycl',
                  'cycle': 'cycl', 'edges': 'edg', 'edge': 'edg', 'structure': 'structur', 'segmenttree': 'segmenttre',  # e in R2 or R1
                  'prime': 'prime', 'primes': 'prime', 'face': 'face', 'plane': 'plane', 'mode': 'mode', 'line': 'line',
                  'type': 'type', 'use': 'use', 'code': 'code', 'scale': 'scale', 'tree': 'tree', 'queue': 'queue',  # e kept
                  'alias': 'alias', 'aliases': 'alias', 'unite': 'unite', 'unites': 'unite', 'uniting': 'unite', 'unit': 'unit',
                  'units': 'unit'}  # IRREGULAR
        self.assertEqual({w: M.stem(w) for w in table_}, table_)

    def test_stem_conflation(self):
        same = [('slide', 'sliding'), ('merge', 'merging'), ('rotate', 'rotating'), ('use', 'using'), ('code', 'coding'),
                ('pad', 'padding'), ('add', 'adding'), ('map', 'mapping'), ('program', 'programming'), ('double', 'doubling'),
                ('delete', 'deleting'), ('divide', 'dividing'), ('scale', 'scaling'), ('bridge', 'bridging'),
                ('sample', 'sampling'), ('schedule', 'scheduling'), ('cycle', 'cycling'), ('alias', 'aliases'), ('edge', 'edges'),
                ('prime', 'primes'), ('query', 'queries'), ('string', 'strings'), ('compute', 'computing'), ('hope', 'hoping')]
        apart = [('prim', 'prime'), ('fac', 'face'), ('unite', 'unit'), ('str', 'string'), ('lc', 'lcs'), ('li', 'lis'),
                 ('so', 'sos'), ('plan', 'plane'), ('mod', 'mode'), ('hopping', 'hoping'), ('stuffing', 'stuf')]
        self.assertEqual([(a, b) for a, b in same if M.stem(a) != M.stem(b)], [])
        self.assertEqual([(a, b) for a, b in apart if M.stem(a) == M.stem(b)], [])

    def test_stem_regions(self):  # the Snowball Porter2 document's examples
        for w, flags in (('yes', 'cvc'), ('say', 'cvc'), ('fly', 'ccv'), ('rhythm', 'ccvccc'), ('ayy', 'vcv'), ('queue', 'cvvvv')):
            self.assertEqual(M.vowels(w), [f == 'v' for f in flags], w)  # y: a vowel after a non-vowel only
        for w, r1, r2 in (('beautiful', 'iful', 'ul'), ('beauty', 'y', ''), ('beau', '', ''),
                          ('animadversion', 'imadversion', 'adversion'), ('sprinkled', 'kled', ''), ('eucharist', 'harist', 'ist')):
            v = M.vowels(w)
            self.assertEqual((w[M.region(v):], w[M.region(v, M.region(v)):]), (r1, r2), w)
        for w, want in (('rap', True), ('trap', True), ('entrap', True), ('ow', True), ('on', True), ('at', True),
                        ('uproot', False), ('bestow', False), ('disturb', False)):
            self.assertEqual(M.short(w, M.vowels(w)), want, w)
        for w, want in (('bed', True), ('shed', True), ('shred', True), ('bead', False), ('embed', False), ('beds', False)):
            v = M.vowels(w)  # a short word: a short syllable and an empty R1
            self.assertEqual(M.short(w, v) and M.region(v) == len(w), want, w)

    def test_t2s_file(self):
        raw = T2S.read_text(encoding='utf-8')
        lines = raw.split('\n')
        self.assertTrue(lines[0].startswith('#') and lines[-1] == '' and '\r' not in raw)
        self.assertEqual([l for l in lines[1:-1] if not re.fullmatch(r'(ts|tc|vp|vc|rp|mw)\t[^\t\s]+\t[^\t\s]+', l)], [])
        rows_, d = [l.split('\t') for l in lines[1:-1]], o_t2s()
        self.assertEqual((sorted(d), sum(map(len, d.values()))), (sorted(['ts', 'tc', 'vp', 'vc', 'rp', 'mw']), len(rows_)))  # unique keys
        self.assertEqual(sorted({c for _, k, v in rows_ for c in k + v if not o_han(c)}), [])
        self.assertEqual([(n, k, v) for n in ('tc', 'vc') for k, v in d[n].items() if len(k) != 1 or len(v) != 1 or k == v], [])
        self.assertEqual([(k, v) for k, v in d['mw'].items() if any(c in d['tc'] for c in k + v)], [])  # in converted characters
        self.assertEqual([(k, v) for k, v in d['mw'].items() if d['mw'].get(v, v) != v], [])  # a class's word maps to itself
        self.assertEqual(M.tables(), ({n: (x, max(map(len, x))) for n, x in d.items()},
                                      frozenset(c for c in set(d['tc']) | set(d['vc']) if not o_gb(c))))
        self.assertIs(M.tables(), M.tables())

    def test_t2s_gb2312(self):  # a character of GB2312, the mainland Simplified set, keeps a Simplified form in it
        self.assertEqual(sorted((k, v) for k, v in o_t2s()['tc'].items() if o_gb(k) and not o_gb(v)), [])

    def test_t2s_mainland_fixed(self):  # a mainland class's word stays as it is
        self.assertEqual(sorted((v, M.simplified(v), o_simplified(v)) for v in set(o_t2s()['mw'].values())
                                if (M.simplified(v), o_simplified(v)) != (v, v)), [])

    def test_simplified_table(self):  # derived by hand from the dictionary lines named
        table_ = {'線段樹': '线段树', '最短路徑': '最短路径', '動態規劃': '动态规划', '並查集': '并查集', '後綴自動機': '后缀自动机',
                  '什麼': '什么', '後綴陣列': ('后缀阵列', '后缀数组'), '樹狀陣列': ('树状阵列', '树状数组'),  # rp 陣列 數組
                  '遞迴': ('递回', '递归'), '雜湊表': ('杂凑表', '哈希表'), '查詢': ('查询', '查找'),  # rp 遞迴 遞歸, 雜湊 哈希, 查詢 查找
                  '顯著': '显著', '乾隆年間': '干隆年间',  # vp 顯著 顯著 keeps 著 from vc; ts 乾隆年間 乾隆年间, then tc 乾 干
                  '想像': '想象', '乾隆': '干隆',  # GB2312 only: tc, then mw (STPhrases 想象 想像, TSPhrases 想像 想像 想象)
                  '演算法': ('演算法', '算法'), '字串': ('字串', '字符串'), '什么': '什么', '么半群': ('么半群', '幺半群'),
                  '用于单条': '用于单条', '链表': '链表', '线段树': '线段树', '后缀数组': '后缀数组', '查询': '查询',
                  '显著': ('显著', '显着'), '': '',  # mainland text stays; its regional form: rp 演算法 算法, 字串 字符串, vp 什么
                  # 什麼, vc 么 幺, 著 着
                  '核心': ('核心', '内核'), '啟用': ('启用', '激活'),  # rp 核心 內核 before vp 核心 核心; rp 啟用 激活 before vc 啟 啓
                  '乾元件': ('干元件', '干元件'), '例項鍊': '例项链',  # segments: ts 乾元 keeps rp 元件 組件 out; ts 項鍊 項鍊 keeps
                  # rp 例項 實例 out (例 its own segment); tc 乾 干
                  '著書': '著书', '醖藉': '酝借'}  # vp 著書 著書 before vc 著 着 (first dictionary), then tc 書 书; the
                  # variant group before the Traditional one: vc 醖 醞, then ts 醞藉 酝借
        want = {t: v if isinstance(v, tuple) else (v, v) for t, v in table_.items()}
        self.assertEqual({t: (M.simplified(t), M.simplified(t, True)) for t in table_}, want)
        trad = {'線段樹', '最短路徑', '動態規劃', '並查集', '後綴自動機', '什麼', '後綴陣列', '樹狀陣列', '遞迴', '雜湊表', '查詢', '顯著', '乾隆年間',
                '啟用', '例項鍊', '著書', '醖藉'}
        self.assertEqual({t for t in table_ if M.traditional(t)}, trad)
        group = [({'a': 'x', 'cd': 'z'}, 2), ({'ab': 'y', 'b': 'v'}, 2)]  # the first dictionary holding a key at a position
        self.assertEqual(M.replace('abcde', group), 'xvze')  # wins, though a later one holds a longer key there

    def test_simplified_random(self):
        d = o_t2s()
        back = {}
        for n in ('tc', 'vc'):
            for k, v in d[n].items():
                back.setdefault(v, []).append(k)
        pool = [sorted(d[n]) for n in d] + [sorted(set(d[n].values())) for n in d]
        rng = random.Random(SEED + 4)
        for r in range(cases(300)):
            parts = [rng.choice(rng.choice(pool)) for _ in range(rng.randint(0, 4))]
            text = ''.join(rng.choice(back.get(c, []) + [c]) for c in ''.join(parts))  # a character or a form converting to it
            for regional in (False, True):
                self.assertEqual((M.traditional(text), M.simplified(text, regional)), (o_traditional(text), o_simplified(text, regional)),
                                 f'seed={SEED} mode={MODE} round={r} text={text!r} regional={regional}')

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
        short = lambda s: sum(len(p) < 3 for p in s)  # fewest parts, then no short part, then the longest parts from the end
        want = min((s for s in o_splits(word, known) if short(s) <= 1), key=lambda s: (len(s), short(s), [-len(p) for p in s[::-1]]),
                   default=[])
        self.assertEqual(M.segment(word, known), want if len(want) > 1 else [], info)

    def test_segment_table(self):
        known = {'segment', 'tree', 'block', 'cut', 'seg', 'men', 'ttree', 'ab', 'x'}  # hand-derived for PART = 2
        for w, want in (('segmenttree', ['segment', 'tree']), ('blockcuttree', ['block', 'cut', 'tree']), ('abab', []),
                        ('cutab', ['cut', 'ab']), ('segment', []), ('treex', []), ('abx', []), ('', [])):  # x: shorter than PART
            self.assertEqual(M.segment(w, known), want, w)  # abab: two parts shorter than 3
        known = {'ri', 'nd', 'ex', 'li', 'chao', 'ab', 'cdefghi', 'jk', 'abc', 'def', 'ghi'}
        for w, want in (('rindex', []), ('lichao', ['li', 'chao']), ('lili', []),
                        ('abcdefghijk', ['abc', 'def', 'ghi', 'jk'])):  # not ab, cdefghi, jk: more parts, one short
            self.assertEqual(M.segment(w, known), want, w)
        known = {'two', 'sat', 'at', 'disjoint', 'spars', 'pars', 'tabl'}  # ties: the repository's twosat and disjointsparsetable
        for w, want in (('twosat', ['two', 'sat']), ('disjointsparsetable', ['disjoint', 'sparse', 'table'])):  # not twos at (short),
            self.assertEqual(M.segment(w, known), want, w)  # not disjoints parse (a shorter part before the last)
        known = {'merg', 'sort', 'tree', 'segment', 'queue'}  # stems: a part is known by its stem, and comes back as written
        for w, want in (('mergesort', ['merge', 'sort']), ('mergesorttree', ['merge', 'sort', 'tree']), ('segmenttree', ['segment', 'tree']),
                        ('mergsort', ['merg', 'sort']), ('persistentqueue', []), ('sortsorting', ['sort', 'sorting'])):
            self.assertEqual(M.segment(w, known), want, w)

    def test_segment_random(self):
        rng = random.Random(SEED + 2)
        for r in range(cases(200)):
            word = ''.join(rng.choice('abes') for _ in range(rng.randint(0, 12)))  # e and s: stems that differ from the part
            known = {M.stem(''.join(rng.choice('abes') for _ in range(rng.randint(1, 6)))) for _ in range(rng.randint(0, 8))}
            known |= {M.stem(word[i:j]) for i, j in (sorted(rng.sample(range(len(word) + 1), 2)) for _ in range(rng.randint(0, 3) if word else 0))}
            self.check_segment(word, known, f'seed={SEED} mode={MODE} round={r} word={word!r} known={sorted(known)}')

    def test_shortened_table(self):
        st, lst, stb = ['segment', 'tree'], ['lazy', 'segment', 'tree'], ['seg', 'tree', 'beats']
        for q, ws, want in (('segtree', st, True), ('lazyseg', lst, True), ('lazysegtree', lst, True), ('segtree', lst, True),
                            ('lazsegtre', lst, True), ('segtre', st, True), ('segmenttree', st, True), ('segtreebea', stb, True),
                            ('seg', st, False), ('segment', st, False),  # one piece
                            ('treeseg', st, False), ('lazytree', lst, False),  # out of order, not consecutive
                            ('setr', st, False), ('segtr', st, False), ('setree', st, False),  # a piece of 2 letters
                            ('segtreex', st, False), ('sebtree', st, False), ('', st, False), ('segtree', [], False)):
            self.assertEqual((M.shortened(q, ws), o_shortened(q, ws)), (want, want), (q, ws))

    def test_shortened_random(self):
        rng = random.Random(SEED + 3)
        for r in range(cases(300)):
            ws = [''.join(rng.choice('ab') for _ in range(rng.randint(1, 5))) for _ in range(rng.randint(0, 4))]
            q = ''.join(rng.choice('ab') for _ in range(rng.randint(0, 9)))
            if ws and rng.random() < 0.6:  # prefixes of consecutive words, some shorter than 3
                s = rng.randrange(len(ws))
                q = ''.join(w[:rng.randint(1, len(w))] for w in ws[s:s + rng.randint(1, 3)])
            self.assertEqual(M.shortened(q, ws), o_shortened(q, ws), f'seed={SEED} mode={MODE} round={r} q={q!r} ws={ws}')

    def test_tolerance(self):
        lo, hi = M.TOLERANCE
        self.assertEqual([M.tolerance('x' * n) for n in range(13)], [0 if n <= lo else 1 if n <= hi else 2 for n in range(13)])

    def test_tuned_constants(self):
        self.assertEqual({k: getattr(M, k) for k in TUNED}, TUNED)
        p = tune_params()  # --tune's parameters round-trip to the module constants, inside its bounds
        self.assertEqual(tune_constants(p), TUNED)
        self.assertTrue(tune_valid(p))
        for k, v in (('t1', 1.0), ('qual', 0), ('phrase', 0), ('folder', 0), ('b', 1.0), ('prefix', 1.0), ('esoteric', 1.0)):
            self.assertTrue(tune_valid(dict(p, **{k: v})), (k, v))
        for k, v in (('t1', 1.05), ('t2', 0.9), ('t2', 0), ('prefix', 1.1), ('prefix', 0), ('planned', 1.0), ('planned', 0),
                     ('existing', 0.8), ('existing', 1.1), ('advanced', 0.8), ('advanced', 1.1), ('esoteric', 0), ('b', 0),
                     ('b', 1.1), ('stem', 0), ('k1', 0), ('qual', -0.1), ('phrase', -0.5), ('folder', -0.3), ('tol2', 3),
                     ('prefix_min', 0), ('part', 0)):
            self.assertFalse(tune_valid(dict(p, **{k: v})), (k, v))

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
        s = flat(self.tree(STRIP_FX))  # an exact name (Advanced), an alias, a word run (Basic): ' zap ' is `zap`
        h = lambda n: f'06-Miscellaneous/{n}'
        hits = s.find('zap')
        self.assertEqual([x.header for x in hits], [h('01-a.hpp'), h('03-c.hpp'), h('02-b.hpp')])
        self.assertLess(hits[1].score, hits[2].score)  # the classes, not the scores
        for q in (' zap ', '\tzap\n', 'ZAP ', 'ｚａｐ', 'ＺＡＰ', 'zäp', '\u3000zap', 'zap O(n log n)'):  # fullwidth, accented
            # and with its complexity: the name too
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
        self.assertEqual(fields[i]['stem'], ['blockcuttre'])  # no known parts here; step 5 drops the e in R2
        w = o_score(fields, i, [('blockcuttre', 1.0)], M.WEIGHTS, M.B, M.K1)
        for q, f in (('block cut tree', 1.0), ('blok cut tree', M.TYPO[1]), ('Block Cut Tree', 1.0)):
            hits = s.find(q, 10)
            self.assertEqual(hits[0].header, s.rows[i].header, q)
            self.assertLess(abs(hits[0].score - f * w), 2e-9, q)
        self.assertIsNone(self.hit(s.find('block cut', 10), s.rows[i].header))
        s = flat(self.tree(FUZZY_FX))  # default prefix: `block` begins blockcuttree; the join `blockcut` takes no prefix
        hits = s.find('block cut', 10)
        self.assertEqual([x.header for x in hits], [s.rows[i].header])
        self.assertLess(abs(hits[0].score - M.PREFIX * o_score([r.fields for r in s.rows], i, [('blockcuttre', 1.0)], M.WEIGHTS, M.B, M.K1)), 2e-9)

    def test_segmentation(self):
        s = flat(self.tree(FUZZY_FX))
        seg = f'{GR}/08-segmenttree.hpp'
        self.assertEqual(next(r for r in s.rows if r.header == seg).fields['stem'], ['segmenttre', 'segment', 'tree'])
        # the stem token stemmed, and the known words it joins
        self.assertIsNotNone(self.hit(s.find('segment', 10), seg))
        self.assertIn(seg, [x.header for x in s.find('segment tree', 2)])
        self.assertEqual(next(r for r in s.rows if r.header.endswith('11-lowbit.hpp')).fields['stem'], ['lowbit'])  # a known word
        s = flat(self.tree(STOPSEG_FX), phrase=0.0)
        stems = {r.header.rsplit('/', 1)[-1]: r.fields['stem'] for r in s.rows}
        self.assertEqual(stems, {'01-meetinthemiddle.hpp': ['meetinthemiddl', 'meet', 'middl'],  # stop words split, yield nothing
                                 '02-meetonit.hpp': ['meetonit'],  # on, it: two parts shorter than 3
                                 '03-words.hpp': ['word'], '04-intothe.hpp': ['intoth']})  # only stop words: no known word joined
        fields = [{f: r.fields[f] for f in M.FIELDS} for r in s.rows]
        o = sum(o_score(fields, 0, [(t, 1.0)], M.WEIGHTS, M.B, M.K1) for t in ('meet', 'middl'))  # found by the yielded words
        self.assertLess(abs(self.hit(s.find('meet in the middle', 10), f'{GR}/01-meetinthemiddle.hpp').score - o), 2e-9)

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
        root = self.tree(PHRASE_FX)  # `ome` begins omega (3 rows) and omelet (1 row): the bonus takes the adjacent one, omega
        p0, p1 = flat(root, phrase=0.0, prefix=0.6), flat(root, phrase=0.8, prefix=0.6)  # at its factor, though omelet has
        fields = [{f: r.fields[f] for f in M.FIELDS} for r in p0.rows]  # the larger idf (a prefix factor below 1 shows it)
        bonus = 0.8 * (0.6 * o_idf(fields, 'omega') + o_idf(fields, 'zeta')) / 2
        self.assertGreater(o_idf(fields, 'omelet'), o_idf(fields, 'omega'))
        a, b = ({x.header: x.score for x in s.find('ome zeta', 10)} for s in (p0, p1))
        self.assertEqual(set(a), {f'{MA}/0{k}-{c}.hpp' for k, c in enumerate('abcd', 1)})
        for hd in a:
            self.assertLess(abs(b[hd] - a[hd] - (bonus if hd.endswith('01-a.hpp') else 0)), 3e-9, hd)
        root = self.tree(ALIAS_FX)  # a Taiwan query's own pairs (后缀 缀阵 阵列) and regional ones (后缀 缀数 数组) are each
        p0, p1 = flat(root, phrase=0.0), flat(root, phrase=0.8)  # their own sequence: the mainland bonus, no junction pair
        fields = [{f: r.fields[f] for f in M.FIELDS} for r in p0.rows]
        bonus = 0.8 * ((o_idf(fields, '后缀') + o_idf(fields, '缀数')) / 2 + (o_idf(fields, '缀数') + o_idf(fields, '数组')) / 2)
        adv = f'{DS}/07-adv.hpp'
        for q in ('后缀数组', '後綴陣列'):
            a, b = ({x.header: x.score for x in t.find(q, 10)} for t in (p0, p1))
            self.assertLess(abs(b[adv] - a[adv] - bonus), 3e-9, q)

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

    def test_shortened_class(self):
        s = flat(self.tree(SHORT_FX), weights={**M.WEIGHTS, 'qual': 4.0})  # heavy qualifiers: the walk row scores highest
        h = lambda n: f'{DS}/{n}'
        three = {h('01-segment_tree.hpp'): 'SegmentTree', h('02-lazy_segment_tree.hpp'): 'LazySegmentTree',
                 h('03-beats.hpp'): 'SegTreeBeats'}
        hits = s.find('segtree', 10)  # pieces of consecutive words lift all three into the word-run class
        self.assertEqual(({x.header: x.name for x in hits[:3]}, [x.header for x in hits[3:]]), (three, [h('04-walk.hpp')]))
        score = {x.header: x.score for x in hits}
        self.assertTrue(all(score[h('04-walk.hpp')] > score[x] for x in three), score)
        self.assertEqual([(x.header, x.name) for x in s.find('lazyseg', 10)[:1]], [(h('02-lazy_segment_tree.hpp'), 'LazySegmentTree')])
        self.assertEqual([x.header for x in s.find('seg', 10)], [h('03-beats.hpp')])  # one piece: SegTreeBeats' whole word only
        for q in ('seg tree', 'treeseg', 'setr', 'segtr'):  # two words, out of order, pieces of 2 letters: no name class
            hits = s.find(q, 10)
            self.assertEqual([x.header for x in hits], [x.header for x in sorted(hits, key=lambda x: (-x.score, x.header))], q)
            self.assertEqual([x.header for x in hits if not x.score], [], q)  # only a name class lists a row scoring 0
        order = [x.header for x in s.find('seg tree', 10)]
        self.assertEqual(set(order), {h('04-walk.hpp')} | set(three))
        self.assertLess(order.index(h('04-walk.hpp')), min(order.index(h('01-segment_tree.hpp')), order.index(h('02-lazy_segment_tree.hpp'))))

    def test_chinese_ranking(self):
        root, P = self.tree(ZH_FX), 0.6  # a prefix factor below 1, so the pair factors show
        s = flat(root, phrase=0.0, prefix=P)
        self.assertEqual([r.fields['aliases'] for r in s.rows], ZH_ALIASES)
        held = o_held([v for k, v in ZH_FX.items() if '/Docs/' in k])  # a regional phrase the documents hold is mainland text
        self.assertEqual((set(s.regional[0]), s.regional[1]), (set(o_t2s()['rp']) - held, max(map(len, s.regional[0]))))
        self.assertEqual(({'序列'} <= held, '陣列' in held), (True, False))
        self.assertEqual(M.words('查詢', regional=s.regional), ['查询', '查找'])  # only Traditional writes it: still read regionally
        doc = 'Aliases: 序列，列埠\n'  # 序列埠 (rp: 串口) lies in no run, though its pairs follow each other in the field
        two = flat(self.tree({f'{DS}/00-index.md': table([('`01-x.hpp`', 'run', 'verified')]), f'{DS}/Docs/01-x.md': doc}))
        self.assertEqual(set(two.regional[0]), set(o_t2s()['rp']) - o_held([doc]))
        self.assertEqual(M.words('序列埠', regional=two.regional), ['序列', '列埠', '串口'])
        index, doc = table([('`01-x.hpp`', 'run 文件', 'verified')]), 'Aliases: 外，接，嚮量\n'  # 外接 in no run; 嚮量 is 向量
        three = flat(self.tree({f'{DS}/00-index.md': index, f'{DS}/Docs/01-x.md': doc}))  # in one form; 文件 in an Operations cell
        self.assertEqual(set(three.regional[0]), set(o_t2s()['rp']) - o_held([index, doc]))
        self.assertEqual([k in three.regional[0] for k in ('外接', '向量', '文件')], [True, False, False])
        fields, hd = [{f: r.fields[f] for f in M.FIELDS} for r in s.rows], [r.header for r in s.rows]
        w = lambda i, t: o_score(fields, i, [(t, 1.0)], M.WEIGHTS, M.B, M.K1)

        def check(q, want):  # want: {row: oracle score}
            hits = s.find(q, 10)
            self.assertEqual({x.header for x in hits}, {hd[i] for i in want}, q)
            for i, o in want.items():
                self.assertLess(abs(self.hit(hits, hd[i]).score - o), 2e-9, (q, i))
        for q in ('线段树', '線段樹'):  # pairs as tokens, Traditional or Simplified
            check(q, {0: w(0, '线段') + w(0, '段树')})
        self.assertEqual([x.header for x in s.find('樹狀陣列')], [x.header for x in s.find('树状数组')])
        for q in ('树状数组', '樹狀陣列'):  # the Taiwan word reaches the mainland alias through its regional pairs 状数, 数组
            check(q, {1: w(1, '树状') + w(1, '状数') + w(1, '数组')})
        check('序列', {5: w(5, '序列')})  # the documents hold 序列: the query reads it as mainland text, no 串行
        self.assertEqual(s.terms('序列')[0], [[('序列', 1.0)]])
        self.assertEqual(M.words('序列', regional=True), ['序列', '串行'])  # read with the whole regional table
        lone = lambda c, i, t, f: f * o_lone(fields, c) / o_idf(fields, t) * w(i, t)  # a lone character through token t
        check('樹', {0: lone('树', 0, '段树', P), 1: lone('树', 1, '树状', P), 3: lone('树', 3, '树', 1.0)})  # itself, its pairs
        check('状', {1: max(lone('状', 1, t, P) for t in ('树状', '状数'))})  # not itself a token: its pairs, the best per row
        check('线 段', {0: lone('线', 0, '线段', P) + max(lone('段', 0, t, P) for t in ('线段', '段树'))})  # no join 线段
        check('qux', {4: P * w(4, 'qux' + DESERET)})  # the prefix range reaches a letter past the BMP

        def same(got, want):  # [(token, factor)] alike up to rounding
            self.assertEqual(sorted(t for t, _ in got), sorted(t for t, _ in want))
            for (t, f), (_, g) in zip(sorted(got), sorted(want)):
                self.assertLess(abs(f - g), 1e-12, t)
        factor = lambda c, t, f: f * o_lone(fields, c) / o_idf(fields, t)
        same(s.alternatives('树'), [('树', factor('树', '树', 1.0)), ('树状', factor('树', '树状', P)), ('段树', factor('树', '段树', P))])
        same(s.alternatives('状'), [('树状', factor('状', '树状', P)), ('状数', factor('状', '状数', P))])
        self.assertEqual(s.alternatives('qux'), [('qux' + DESERET, P)])
        groups, adjacent = s.terms('线 段')
        self.assertEqual((groups, adjacent), ([s.alternatives('线'), s.alternatives('段')], [tuple(groups)]))
        self.assertIn([('fenwick', 1.0)], s.terms('fen wick')[0])  # letters still join
        a, b = ({x.header: x.score for x in flat(root, phrase=p, prefix=P).find('線段樹')} for p in (0.0, 0.8))
        bonus = 0.8 * (o_idf(fields, '线段') + o_idf(fields, '段树')) / 2  # adjacent pairs in the aliases field
        self.assertLess(abs(b[hd[0]] - a[hd[0]] - bonus), 3e-9)
        a, b = ({x.header: x.score for x in flat(root, phrase=p, prefix=P).find('线 段')} for p in (0.0, 0.8))
        bonus = 0.8 * (P * o_lone(fields, '线') + P * o_lone(fields, '段')) / 2  # 线段 then 段树, each at its lone factor
        self.assertLess(abs(b[hd[0]] - a[hd[0]] - bonus), 3e-9)

    def test_alias_class(self):
        s = flat(self.tree(ALIAS_FX))
        h = lambda n: f'{DS}/{n}'
        fields = [{f: r.fields[f] for f in M.FIELDS} for r in s.rows]
        hd = [r.header for r in s.rows]
        o = lambda q, i: sum(o_score(fields, i, [(t, 1.0)], M.WEIGHTS, M.B, M.K1) for t in o_words(q, M.STOP))
        rest = sorted((h('01-heavy.hpp'), h('04-later.hpp')), key=lambda x: -o('线段树', hd.index(x)))
        for q in ('线段树', '線段樹'):  # an alias equal to the query: Basic before Advanced, verified first; planned: no class
            hits = s.find(q, 10)
            self.assertEqual([(x.header, x.name) for x in hits], [(x, x) for x in [h('02-plain.hpp'), h('03-also.hpp'), h('07-adv.hpp')] + rest], q)
            score = {x.header: x.score for x in hits}
            self.assertTrue(all(score[h('01-heavy.hpp')] > score[x] for x in (h('02-plain.hpp'), h('03-also.hpp'), h('07-adv.hpp'))), score)
        for q, first in (('zkw tree', '02-plain.hpp'), ('ZKW Trees', '02-plain.hpp'),  # several words, stemmed
                         ('後綴陣列', '07-adv.hpp'), ('字尾数组', '07-adv.hpp'),  # the regional reading: rp 陣列 數組, 字尾 後綴
                         ('bit', '06-heap.hpp')):  # an alias before a name holding the word (BitOps)
            self.assertEqual(s.find(q, 1)[0].header, h(first), q)
        self.assertEqual([x.header for x in s.find('bit', 2)], [h('06-heap.hpp'), h('05-bitty.hpp')])
        self.assertEqual([x.header for x in s.find('plain', 2)], [h('02-plain.hpp'), h('07-adv.hpp')])  # an exact name first
        two = flat(self.tree({f'{DS}/00-index.md': '## Basic\n\n' + table([('`01-b.hpp`', 'B: other', 'verified')]) + '## Advanced\n\n'
                              + table([('`02-a.hpp`', 'A: zork', 'verified')]), f'{DS}/Docs/01-b.md': 'Aliases: zork\n'}))
        self.assertEqual([x.header for x in two.find('zork', 2)], [h('02-a.hpp'), h('01-b.hpp')])  # exact name, then alias
        two = flat(self.tree({f'{DS}/00-index.md': table([('`01-c.hpp`', 'C: run', 'verified'), ('`02-d.hpp`', 'D: run', 'verified')]),
                              f'{DS}/Docs/01-c.md': 'Aliases: 序列\n', f'{DS}/Docs/02-d.md': 'Aliases: 串行\n'}))
        self.assertEqual([x.header for x in two.find('序列', 10)], [h('01-c.hpp')])  # the documents hold 序列: no 串行 reading
        self.assertNotEqual(s.find('后缀', 1)[0].header if s.find('后缀', 1) else None, None)
        hits = s.find('后缀 数组 extra', 10)  # no alias equals the whole query: by score
        self.assertEqual([x.header for x in hits], [x.header for x in sorted(hits, key=lambda x: (-x.score, x.header))])


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


def corpus():
    """The query list: the corpus file's queries in file order, the Library Checker problems in ledger order, then the
    generated typo variants (MODE, SEED, ROUNDS)."""
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

    def test_repo_chinese(self):  # metamorphic: one query in Simplified and in Traditional (Taiwan words) ranks alike
        for t, s in (('線段樹', '线段树'), ('最短路徑', '最短路径'), ('並查集', '并查集'), ('後綴自動機', '后缀自动机')):
            hits = self.s.find(s, 10)
            self.assertTrue(hits, s)
            self.assertEqual(self.s.find(t, 10), hits, (t, s))  # the same words: the same tokens
        for t, s in (('樹狀陣列', '树状数组'), ('後綴陣列', '后缀数组'), ('雜湊表', '哈希表'), ('遞迴', '递归')):
            hits = self.s.find(s, 1)
            self.assertTrue(hits, s)
            self.assertEqual(self.s.find(t, 1)[0].header, hits[0].header, (t, s))  # the regional pairs reach the mainland words
        for t, s in (('字尾', '后缀'), ('字首', '前缀'), ('函式', '函数'), ('字元', '字符'), ('字串匹配', '字符串匹配')):
            hits = self.s.find(s, 1)  # Taiwan words in characters both scripts share: read as mainland text, then regionally
            self.assertTrue(hits, s)
            self.assertEqual(self.s.find(t, 1)[0].header, hits[0].header, (t, s))
        docs = [M.aliases(d.read_text(encoding='utf-8')) for d in sorted(ROOT.glob('0[1-8]-*/Docs/*.md'))]
        held = {o_simplified(k) for k in o_held(docs)}  # the regional phrases the documents hold, read as mainland words:
        self.assertEqual(held, {M.simplified(k) for k in set(M.tables()[0]['rp'][0]) - set(self.s.regional[0])})
        self.assertEqual(held, {'外接', '序列', '文件', '行程', '宏'})  # reviewed: a new one
        # is a Taiwan word to write as its regional reading, or a mainland word the Taiwan tables also map (00-search.md)
        for q, want in (('线段树', '02-Data Structures/03-segmenttree.hpp'), ('線段樹', '02-Data Structures/03-segmenttree.hpp'),
                        ('并查集', '02-Data Structures/01-dsu.hpp'), ('树状数组', '02-Data Structures/02-fenwick.hpp'),
                        ('樹狀陣列', '02-Data Structures/02-fenwick.hpp'), ('稀疏表', '02-Data Structures/04-sparsetable.hpp'),
                        ('KMP 演算法', '07-Strings/01-prefixfunction.hpp'), ('Dijkstra 演算法', '04-Graphs/05-shortest_path.hpp'),
                        ('最长公共子序列', '07-Strings/19-lcs.hpp')):  # canonical rows by their alias; Taiwan words in no alias
            self.assertEqual(self.s.find(q, 1)[0].header, want, q)
        self.assertEqual(M.words('最长公共子序列', regional=self.s.regional), M.words('最长公共子序列'))  # no 子串 from 子串行

    def test_repo_corpus(self):
        queries = corpus()
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


# ---------------------------------------------------------------- --tune (00-search.md, Weights and cutoffs)
OBJECTIVE = ('cp', 'oi', 'abbr', 'typo', 'simple', 'py', 'lc', 'generated', 'zhs', 'zht')  # zh: the titles in the aliases
GRID = {'stem': [0.5, 1, 1.5, 2, 3], 'ident': [0.5, 1, 1.5, 2, 3], 'ops': [0.5, 0.75, 1, 1.5, 2], 'qual': [0, 0.1, 0.25, 0.5, 1],
        'aliases': [1, 1.5, 2, 2.5, 3, 4], 'folder': [0, 0.3, 0.6, 0.9, 1.2], 'b': [0.2, 0.3, 0.5, 0.75, 0.9],
        'k1': [0.6, 0.9, 1.2, 1.6, 2.4], 'phrase': [0, 0.5, 1, 1.5, 2], 'prefix': [0.45, 0.6, 0.75, 0.9], 't1': [0.5, 0.6, 0.7, 0.85],
        't2': [0.3, 0.4, 0.5, 0.6, 0.7], 'planned': [0.6, 0.7, 0.8, 0.9], 'existing': [0.8, 0.9, 1.0], 'advanced': [0.8, 0.9, 1.0],
        'esoteric': [0.6, 0.7, 0.8, 0.9, 1.0], 'tol2': [6, 7, 8, 9], 'prefix_min': [2, 3, 4], 'part': [2, 3]}  # in search order
STEP = {'stem': 0.5, 'ident': 0.5, 'ops': 0.25, 'qual': 0.1, 'aliases': 1, 'folder': 0.3, 'b': 0.1, 'k1': 0.8, 'phrase': 0.5,
        'prefix': 0.1, 't1': 0.1, 't2': 0.1, 'planned': 0.1, 'existing': 0.1, 'advanced': 0.1, 'esoteric': 0.1, 'tol2': 1,
        'prefix_min': 1, 'part': 1}  # a grid grows by its step past a best value at its edge, while tune_valid holds


def tune_params():
    """The module's constants as --tune parameters (planned stands for legacy-reference too)."""
    assert M.STANDING['legacy-reference'] == M.STANDING['planned']
    return {**M.WEIGHTS, 'b': M.B, 'k1': M.K1, 'phrase': M.PHRASE, 'prefix': M.PREFIX, 't1': M.TYPO[1], 't2': M.TYPO[2],
            'planned': M.STANDING['planned'], 'existing': M.STANDING['existing-unverified'], 'advanced': M.LEVEL['Advanced'],
            'esoteric': M.LEVEL['Esoteric'], 'tol2': M.TOLERANCE[1], 'prefix_min': M.PREFIX_MIN, 'part': M.PART}


def tune_constants(p):
    """The module constants (TUNED's layout) of parameters p."""
    return {'WEIGHTS': {f: float(p[f]) for f in M.FIELDS}, 'B': p['b'], 'K1': p['k1'], 'TYPO': {1: p['t1'], 2: p['t2']},
            'PREFIX': p['prefix'], 'PHRASE': p['phrase'],
            'STANDING': dict(M.STANDING, **{'existing-unverified': p['existing'], 'legacy-reference': p['planned'], 'planned': p['planned']}),
            'LEVEL': dict(M.LEVEL, Advanced=p['advanced'], Esoteric=p['esoteric']), 'TOLERANCE': (M.TOLERANCE[0], p['tol2']),
            'PREFIX_MIN': p['prefix_min'], 'PART': p['part']}


def tune_valid(p):
    """Factors in (0, 1] with typo factors falling with the distance, planned below 1 and the existing-unverified factor,
    Esoteric at most Advanced; positive weights but qual, phrase and folder, which may be 0; b in (0, 1]."""
    return (0 < p['t2'] <= p['t1'] <= 1 and 0 < p['prefix'] <= 1 and 0 < p['planned'] <= p['existing'] <= 1 and p['planned'] < 1
            and 0 < p['esoteric'] <= p['advanced'] <= 1 and 0 < p['b'] <= 1 and min(p['stem'], p['ident'], p['ops'], p['aliases'], p['k1']) > 0
            and min(p['qual'], p['phrase'], p['folder']) >= 0 and p['tol2'] > M.TOLERANCE[0] and p['prefix_min'] >= 1 and p['part'] >= 1)


def tune_ranks(task):
    """[(source, rank of the first target in the top 10, else 0)] of the queries under parameters p."""
    p, queries = task
    c = tune_constants(p)
    M.TOLERANCE, M.PREFIX_MIN, M.PART = c['TOLERANCE'], c['PREFIX_MIN'], c['PART']  # globals: each evaluation sets them
    s = M.Search(ROOT, weights=c['WEIGHTS'], b=c['B'], k1=c['K1'], typo=c['TYPO'], prefix=c['PREFIX'], phrase=c['PHRASE'],
                 standing=c['STANDING'], level=c['LEVEL'])
    return [(src, next((k + 1 for k, h in enumerate(s.find(q, 10)) if h.header in t), 0)) for src, q, t in queries]


def tune_stats(pool, cands, queries, k=3):
    """Per parameter set, {source: (recall@5, MRR@10)} over queries, each set's queries in k chunks across the pool."""
    tasks = [(c, queries[j::k]) for c in cands for j in range(k)]
    parts = pool.map(tune_ranks, tasks) if pool else list(map(tune_ranks, tasks))
    out = []
    for i in range(len(cands)):
        by = {}
        for src, r in (x for part in parts[i * k:(i + 1) * k] for x in part):
            by.setdefault(src, []).append(r)
        out.append({src: (sum(0 < r <= 5 for r in rs) / len(rs), sum(1 / r for r in rs if r) / len(rs)) for src, rs in by.items()})
    return out


def objective(st):
    """The mean over the OBJECTIVE sources of (recall@5 + MRR@10)/2."""
    have = [src for src in OBJECTIVE if src in st]
    return round(sum(sum(st[src]) / 2 for src in have) / len(have), 9)


def tune(half, start, jobs):
    """Coordinate descent from start over the full-mode --seed 1 query list (half all) or its even positions: each
    parameter in GRID order to the best value of its grid (the current one on a tie), the grid grown past an edge, until
    a round moves none. Prints each step, then the values, the module constants, and on all the per-source results and
    the FLOORS they give, on even the odd half's objective of start, the tuned values and the module's constants."""
    queries, module = corpus(), tune_params()  # before a serial evaluation sets the globals
    chosen = queries if half == 'all' else queries[0::2]
    ctx = multiprocessing.get_context('fork') if jobs > 1 and 'fork' in multiprocessing.get_all_start_methods() else None
    with (ctx.Pool(jobs) if ctx else contextlib.nullcontext()) as pool:
        score = lambda cands, qs=chosen: [objective(st) for st in tune_stats(pool, cands, qs)]
        cur = dict(start)
        assert tune_valid(cur), cur
        best = score([cur])[0]
        print('start', best, json.dumps(cur), flush=True)
        for rnd in range(10):
            moved = False
            for k in GRID:
                grid = sorted(set(GRID[k]) | {cur[k]})
                while True:
                    cands = [c for c in (dict(cur, **{k: v}) for v in grid) if tune_valid(c)]
                    res = score(cands)
                    vals = [c[k] for c in cands]
                    choice = cur[k] if res[vals.index(cur[k])] == max(res) else vals[res.index(max(res))]
                    ext = round(choice + (STEP[k] if choice == max(vals) else -STEP[k]), 4)
                    if (len(vals) > 1 and choice in (min(vals), max(vals)) and ext not in grid
                            and tune_valid(dict(cur, **{k: ext}))):
                        grid = sorted(grid + [ext])
                        continue
                    break
                print(f'round {rnd} {k}: ' + ' '.join(f'{v}={r:.4f}' for v, r in zip(vals, res)) + f' -> {choice}', flush=True)
                if choice != cur[k]:
                    cur[k], best, moved = choice, max(res), True
            print(f'round {rnd} best {best} {json.dumps(cur)}', flush=True)
            if not moved:
                break
        print(json.dumps(cur))
        for name, value in tune_constants(cur).items():
            print(f'{name} = {value!r}')
        if half == 'even':
            for label, p in (('start', start), ('tuned', cur), ('module', module)):
                print(f'odd half, {label}: {score([p], queries[1::2])[0]}', flush=True)
            return
        st = tune_stats(pool, [cur], queries)[0]
        floor = lambda x: math.floor(round((x - 0.02) * 100, 6)) / 100
        for src, (rec, mrr) in st.items():
            print(f'{src:10} {rec:.6f} {mrr:.6f}')
        print('FLOORS =', {src: FLOORS[src] if src in ('typo', 'generated') else (floor(rec), floor(mrr)) for src, (rec, mrr) in st.items()})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=3, help='stress: multiple of the full random cases and generated typo seeds')
    parser.add_argument('--tune', choices=('all', 'even'), help='search the constants on the full-mode --seed 1 list (00-search.md)')
    parser.add_argument('--start', default='{}', help='--tune: JSON parameters over the module constants to start from')
    parser.add_argument('--jobs', type=int, default=os.cpu_count() or 1, help='--tune: worker processes')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, max(1, args.rounds)
    if args.tune:
        MODE, SEED = 'full', 1
        tune(args.tune, {**tune_params(), **json.loads(args.start)}, max(1, args.jobs))
        raise SystemExit(0)
    suite = unittest.TestSuite()
    for case in (Units, Ranking, Cli, Real):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case))
    print(f"search checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}",
          flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
