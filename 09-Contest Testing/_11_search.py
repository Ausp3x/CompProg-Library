"""Concept search over the library's inventory rows: `10-grab.py --find WORDS` and the rows grab lists for an unknown name.

A document is one row of a folder's 00-index.md (01-08): its header or module stem split at `_`, the identifiers of its
Operations cell (camelCase split), the cell's parenthesized qualifiers, its evidence document's aliases and its folder.
Ranking: field-weighted BM25 over normalized tokens (lower case, light stemming, stop words dropped; Chinese as
overlapping character pairs in Simplified, Traditional through OpenCC's conversion, a query also read in Taiwan and Hong
Kong usage), per-token typo and prefix tolerance, adjacent query words joined, a phrase bonus, times
factors by status and tier (finished and simple rows first); a grabbable row whose header stem, name or alias equals the
query, then one whose name contains a one-word query, ranks first, simplest first; equal scores go to the higher tier,
then the more finished status.
Standard library and the T2S table (OpenCC's dictionaries) only, no index or cache.
Rules: 00-Guidelines/08-contest-tools.md; weights, cutoffs and their evaluation: 96-Local Testing/Docs/00-search.md.
"""
from __future__ import annotations

import bisect
import functools
import math
import re
import unicodedata
from collections import Counter, namedtuple
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
T2S = Path(__file__).with_name('_11_search_t2s.tsv')  # OpenCC's Traditional-to-Simplified dictionaries
HAN = '\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff\U00020000-\U0003ffff'  # Chinese characters (a regex class)
WORD = re.compile(f'([{HAN}]+)|[^\\W\\d_{HAN}]+|\\d+')  # a run of Chinese characters (group 1), letters or digits
TIERS = ('Basic', 'Advanced', 'Esoteric')  # tie order; a 01-Core row outside Basic counts as Advanced
STATUSES = ('verified', 'partial', 'existing-unverified', 'legacy-reference', 'planned')
FIELDS = ('stem', 'ident', 'ops', 'qual', 'aliases', 'folder')
WEIGHTS = {'stem': 1.0, 'ident': 1.0, 'ops': 1.0, 'qual': 0.5, 'aliases': 1.5, 'folder': 4.5}  # chosen on the corpus (00-search.md)
B, K1 = 0.5, 1.2
TYPO = {1: 0.85, 2: 0.8}  # factor of a vocabulary token at that edit distance from an unknown query token
PREFIX = 1.0  # factor of a vocabulary token an unknown query token (PREFIX_MIN letters or more) begins
PHRASE = 1.0  # bonus per adjacent query pair adjacent in a field, times the pair's mean idf
TOLERANCE = (3, 7)  # an unknown query token tolerates no edit up to the first length, 1 up to the second, then 2
PREFIX_MIN = 2  # letters an unknown query token needs to match the tokens it begins
PART = 2  # letters of each known word a header stem splits into
STANDING = {'verified': 1.0, 'partial': 1.0, 'existing-unverified': 1.0, 'legacy-reference': 0.9, 'planned': 0.9}  # score
LEVEL = {'Basic': 1.0, 'Advanced': 1.0, 'Esoteric': 0.9}  # factors by status and tier: finished, simple rows first
STOP = frozenset('a an and as at by for from in into is it of on or the to via with'.split())
IRREGULAR = {'vertices': 'vertex', 'indices': 'index', 'matrices': 'matrix', 'series': 'series', 'species': 'species', 'tries': 'trie',
             'alias': 'alias', 'aliases': 'alias', 'unite': 'unite', 'unites': 'unite', 'uniting': 'unite'}
# irregular plurals, and words the rules would mangle or merge (unite and unit)
BOUND = re.compile(r'(?<![^\W_])[OoΘΩ]\(')  # where a complexity expression may start (after no letter or digit)
ITEM = re.compile(r'((?:`[^`\n]*`|[^,;:()|\n、])+)(:?)')  # an alias item (a code span whole), the colon after a label (folded text)
Row = namedtuple('Row', 'header folder tier status names key fields notes items runs')
Hit = namedtuple('Hit', 'name header tier status score')


# ---------------------------------------------------------------- Operations cells (also 96-Local Testing/05-extraction.py)
def cells(line):
    return [x.strip() for x in re.split(r'(?<!\\)\|', line.strip())[1:-1]]


def split(text, sep, angle=False):
    """text split at sep outside parentheses and brackets, a half-open [l, r) included, and with angle outside template
    brackets (a `<` right after a name other than `operator`)."""
    out, depth, angles, cur = [], 0, 0, ''
    for ch in text:
        depth += (ch in '([') - (ch in ')]')
        if angle and ch == '<' and re.search(r'\w$', cur) and not re.search(r'\boperator$', cur):
            angles += 1
        elif angle and ch == '>' and angles:
            angles -= 1
        if ch == sep and depth <= 0 and not angles:
            out.append(cur)
            cur = ''
        else:
            cur += ch
    return [x.strip() for x in out + [cur] if x.strip()]


def unqualified(text):
    """text without its parenthesized qualifiers (balanced, brackets counted as in split)."""
    out, depth = '', 0
    for ch in text:
        depth += ch in '(['
        if depth <= 0:
            out += ch
        depth -= ch in ')]'
    return re.sub(r'\s+(?=[,:]|$)', '', out)


def depths(text):
    """Bracket depth before each character (parentheses and brackets, as in split)."""
    out, depth = [], 0
    for ch in text:
        out.append(depth)
        depth += (ch in '([') - (ch in ')]')
    return out


def groups(cell):
    """(group head with its qualifiers, operations) per `;` item of an Operations cell: 'A, B (alias): x, y; z' ->
    ('A, B (alias)', ['x', 'y']), ('', ['z'])."""
    out = []
    for item in split(cell.replace('internals:', ';'), ';'):
        cut = next((k for k, d in enumerate(depths(item)) if d == 0 and item.startswith(': ', k)), None)
        head, ops = (item[:cut], item[cut + 2:]) if cut is not None and re.fullmatch(r'[\w:<>, ]+', unqualified(item[:cut])) else ('', item)
        out.append((head, split(ops, ',', True)))
    return out


def operations(cell):
    """(group types, operation) pairs of an Operations cell: 'A, B: x, y; z' -> ([A, B], x), ([A, B], y), ([], z)."""
    out = []
    for head, ops in groups(cell):
        types = [re.sub(r'<.*', '', unqualified(t)).strip() for t in split(head, ',', True)] if head else []
        out += [([t for t in types if re.fullmatch(r'[\w:]+', t)], name(op)) for op in ops]
    return out


def name(op):
    """The name of one Operations entry: `operator+=` for `+=`, `kth` for `kth (persistent)`."""
    op = op.replace('\\|', '|')
    sym = re.match(r'operator\s*(\(\)|\[\]|[^\w\s(\[]+)|([^\w\s(\[]+)(?:\s|$)', op)  # operator(), +=, ==
    return 'operator' + (sym[1] or sym[2]) if sym else re.sub(r'\s*\(.*', '', op).strip()


def names(cell):
    """The names a row offers to grab, in cell order: a type head (capitalized, or a lower-case alias such as `mint`;
    a head of several words gives its capitalized ones) and its members as Type::member (a constructor names the type),
    a camelCase function head alone (its entries are its variants or fields), and the entries of a lower-case label
    head (`required:`) or of no head as they are."""
    return list(dict.fromkeys(n for n, _ in entries(cell)))


def notes(cell):
    """{name: the parenthesized qualifiers of its entries}, and for a function head the text of its entries too."""
    out = {}
    for n, q in entries(cell):
        out[n] = (out.get(n, '') + ' ' + q).strip()
    return out


def entries(cell):
    """(name, qualifier text) per name an entry of cell gives, in cell order (names)."""
    out, inner = [], lambda x: ''.join(ch for ch, d in zip(x, depths(x)) if d)
    for head, ops in groups(cell):
        heads = [(x, raw) for raw in split(head, ',', True) for t in [re.sub(r'<.*', '', unqualified(raw)).strip()]
                 for x in ([t] if re.fullmatch(r'[A-Za-z_][\w:]*', t) else re.findall(r'\b[A-Z]\w*', t))]  # `A as B and C`
        kind = 'label' if not heads else 'type' if any(t[0].isupper() or '::' in t or re.search(r'\(alias\b', raw) for t, raw in heads) \
            else 'function' if any(re.search(r'[A-Z\d]', t) for t, _ in heads) else 'label'
        if kind != 'label':
            out += [(t, inner(raw) + (' ' + ' '.join(ops) if kind == 'function' else '')) for t, raw in heads]
        if kind == 'function':
            continue
        for raw, op in ((x, y if y.startswith('operator') else re.sub(r'<.*', '', y)) for x in ops for y in [name(x)]):
            if re.fullmatch(r'[A-Za-z_][\w:]*|operator\S+', op) and not (kind == 'label' and op.startswith('operator') or op.startswith('std::')):
                out.append((heads[0][0] if kind == 'type' and op == 'ctor' else f'{heads[0][0]}::{op}' if kind == 'type' and '::' not in op
                            else op, inner(raw)))  # an entry the cell qualifies (`PolyCompanion::fft`) stays as written; a `std::` one is a
            # specialization grab keeps with its type, no name
    return out


# ---------------------------------------------------------------- tokens
@functools.cache
def stem(word):
    """Light stemming: a plural (ies, ches/shes/sses/xes/zzes, s), then -ing and a final e as Porter2 (steps 1b and 5)
    treats them, so a word and its -ing form share a stem (sliding, slide -> slide; merging, merge -> merg; rotating,
    rotate -> rotat; spanning -> span, adding -> add; string stays), and IRREGULAR."""
    if word in IRREGULAR:
        return IRREGULAR[word]
    if len(word) > 4 and word.endswith('ies'):
        word = word[:-3] + 'y'
    elif len(word) > 4 and re.search(r'(?:ch|sh|ss|x|zz)es$', word):
        word = word[:-2]
    elif len(word) > 3 and word.endswith('s') and not re.search(r'(?:ss|us|is)$', word):
        word = word[:-1]
    if word.endswith('ing') and any(vowels(word[:-3])):
        word = word[:-3]
        v = vowels(word)
        if word.endswith(('at', 'bl', 'iz')) or short(word, v) and region(v) == len(word):
            word += 'e'
        elif len(word) > 3 and re.search(r'(?:bb|dd|gg|mm|nn|pp|rr|tt)$', word):  # padding -> pad, not adding -> ad
            word = word[:-1]
    if word.endswith('e'):  # in R2, or in R1 and not after a short syllable
        v = vowels(word)
        if region(v, region(v)) < len(word) or region(v) < len(word) and not short(word[:-1], v[:-1]):
            word = word[:-1]
    return word


def vowels(word):
    """Porter2's vowel flags of word: a e i o u, and y after a non-vowel."""
    out = []
    for i, c in enumerate(word):
        out.append(c in 'aeiou' or c == 'y' and i > 0 and not out[-1])
    return out


def region(v, start=0):
    """Porter2's R1 (R2 from R1's start): the index after the first non-vowel that follows a vowel at start or later."""
    return next((i + 1 for i in range(start + 1, len(v)) if v[i - 1] and not v[i]), len(v))


def short(word, v):
    """Whether word ends in a short syllable: non-vowel, vowel, non-vowel other than w x y, or is vowel, non-vowel."""
    return len(word) >= 3 and not v[-3] and v[-2] and not v[-1] and word[-1] not in 'wxy' or len(word) == 2 and v[0] and not v[1]


def words(text, stop=STOP, regional=False):
    """Lower-case words of NFKC-normalized text, diacritics dropped from Latin letters (Prüfer: prufer), complexity
    expressions dropped (unbounded), identifiers
    split at `_`, camelCase and letter-digit boundaries, unstemmed, without the words in stop; a run of Chinese
    characters gives the overlapping pairs (a lone character itself) of its simplified form, with regional (a query: as
    in simplified) then the new pairs of its regional form (Taiwan's 字尾 is the mainland's 后缀)."""
    text = unbounded(fold(text))
    text = re.sub(r'([a-z\d])([A-Z])', r'\1 \2', re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1 \2', text))
    out = []
    for m in WORD.finditer(text.lower()):
        if m[1]:
            pairs = [w[i:i + 2] for w in [simplified(m[1])] for i in range(max(1, len(w) - 1))]
            more = [w[i:i + 2] for w in [simplified(m[1], regional)] for i in range(max(1, len(w) - 1))] if regional else []
            out += pairs + [p for p in dict.fromkeys(more) if p not in pairs]
        elif m[0] not in stop:
            out.append(m[0])
    return out


def fold(text):
    """NFKC-normalized text with diacritics dropped from Latin letters (ＩｎｆＩｎｔ: InfInt, Prüfer: Prufer)."""
    text = unicodedata.normalize('NFKC', text)
    return text if text.isascii() else ''.join(map(latin, text))


def unbounded(text):
    """text without its complexity expressions, which name no concept: O, o, Θ or Ω after no letter or digit, then a
    parenthesized stretch with its nested parentheses (`O(N+M)`, `O(T(n) log n)`); an unclosed one stays."""
    out, i = '', 0
    for m in BOUND.finditer(text):
        if m.start() < i:
            continue  # inside the last expression
        depth, j = 0, m.end() - 1
        while j < len(text) and (depth := depth + (text[j] == '(') - (text[j] == ')')):
            j += 1
        if j < len(text):
            out, i = out + text[i:m.start()] + ' ', j + 1
    return out + text[i:]


@functools.cache
def latin(c):
    """A Latin letter without its diacritics (ü: u, ǖ: u, ạ: a, Ǣ: Æ), any other character itself."""
    d = unicodedata.normalize('NFD', c)[0]
    return d if unicodedata.name(d, '').startswith('LATIN') else c


@functools.cache
def tables():
    """T2S as {dictionary: (entries, longest key)}: OpenCC's Traditional-to-Simplified phrases (ts) and characters
    (tc), its Taiwan and Hong Kong variant phrases (vp) and characters (vc) and regional phrases (rp), and the
    mainland word classes (mw); and the characters outside GB2312 (the mainland character set) tc or vc converts."""
    d = {k: {} for k in ('ts', 'tc', 'vp', 'vc', 'rp', 'mw')}
    for line in T2S.read_text(encoding='utf-8').splitlines()[1:]:
        kind, k, v = line.split('\t')
        d[kind][k] = v
    return {k: (x, max(map(len, x))) for k, x in d.items()}, frozenset(c for c in {**d['tc'], **d['vc']} if not gb2312(c))


def gb2312(c):
    try:
        return bool(c.encode('gb2312'))
    except UnicodeEncodeError:
        return False


def traditional(text):
    """Whether Chinese text is Traditional: it holds a character outside GB2312 that T2S converts."""
    return not tables()[1].isdisjoint(text)


def segments(text, d, longest):
    """OpenCC's segmentation: each longest key of d a segment, and each stretch between them one."""
    out, i, start = [], 0, 0
    while i < len(text):
        k = next((k for k in range(min(longest, len(text) - i), 0, -1) if text[i:i + k] in d), 0)
        if k:
            out += [text[start:i]] * (start < i) + [text[i:i + k]]
            start = i + k
        i += k or 1
    return out + [text[start:]] * (start < len(text))


def replace(text, group):
    """OpenCC's conversion by a group of (dictionary, longest key): at each position, left to right, the longest key
    of the first dictionary holding one there, else the character itself."""
    out, i = [], 0
    while i < len(text):
        v, k = next(((d[text[i:i + k]], k) for d, longest in group for k in range(min(longest, len(text) - i), 0, -1)
                     if text[i:i + k] in d), (text[i], 1))
        out.append(v)
        i += k
    return ''.join(out)


def simplified(text, regional=False):
    """Chinese text in one form for every script: a Traditional text, or with regional any text read as Taiwan or Hong
    Kong text, through OpenCC's conversion (with regional first their words for a thing the mainland names otherwise,
    演算法 and 字尾 for 算法 and 后缀, from T2S's regional phrases or, given as (phrases, longest key), from those;
    then their variants, then Traditional to Simplified, phrase by phrase), then any text's characters in Simplified
    and its words as their mainland class's word where T2S lists one (想像 and 想象 as one)."""
    t = tables()[0]
    if regional or traditional(text):
        parts = segments(text, *t['ts'])
        for group in ([t['rp'] if regional is True else regional] * bool(regional) + [t['vp'], t['vc']], [t['ts'], t['tc']]):
            parts = [replace(p, group) for p in parts]
        text = ''.join(parts)
    return replace(replace(text, [t['tc']]), [t['mw']])


def chinese(word):
    """Whether a word of words is Chinese (a character pair or a lone character)."""
    return bool(re.match(f'[{HAN}]', word))


def tokens(text):
    return [stem(w) for w in words(text)]


def distance(a, b, cap):
    """Optimal string alignment distance of a and b (adjacent transpositions count 1), or cap + 1 once it exceeds cap."""
    if abs(len(a) - len(b)) > cap:
        return cap + 1
    prev2, prev = None, list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i] + [0] * len(b)
        for j, cb in enumerate(b, 1):
            cur[j] = min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (ca != cb))
            if i > 1 and j > 1 and ca == b[j - 2] and a[i - 2] == cb:
                cur[j] = min(cur[j], prev2[j - 2] + 1)
        if min(cur) > cap:
            return cap + 1
        prev2, prev = prev, cur
    return min(prev[-1], cap + 1)


def segment(word, known):
    """The fewest pieces of word (PART letters or more, at most one shorter than 3) whose stems are known words, on a
    tie without a short piece, then with the longest pieces from the end (`disjointsparsetable` -> disjoint, sparse,
    table, not disjoints, parse; `twosat` -> two, sat, not twos, at), or [] when there are none or word is one
    (`segmenttree` -> segment, tree; `rindex` stays whole, not ri, nd, ex)."""
    best = {(0, 0): []}  # (end, parts shorter than 3 letters) -> fewest parts
    for e in range(PART, len(word) + 1):
        for b in range(e - PART + 1):
            for s in (0, 1):
                n = s + (e - b < 3)
                if (b, s) in best and n <= 1 and stem(word[b:e]) in known and ((e, n) not in best or len(best[b, s]) + 1 < len(best[e, n])):
                    best[e, n] = best[b, s] + [word[b:e]]
    out = min((best[len(word), s] for s in (0, 1) if (len(word), s) in best), key=len, default=[])
    return out if len(out) > 1 else []


def shortened(q, ws):
    """Whether q joins two or more pieces of 3 letters or more that begin consecutive words of ws (`segtree` in
    `SegmentTree`, `lazyseg` in `LazySegmentTree`)."""
    def fits(q, ws, k):
        return not q and k > 1 or bool(ws) and any(ws[0].startswith(q[:n]) and fits(q[n:], ws[1:], k + 1) for n in range(3, min(len(q), len(ws[0])) + 1))
    return len(q) >= 6 and any(fits(q, ws[s:], 0) for s in range(len(ws)) if ws[s].startswith(q[:3]))  # 2 pieces of 3 at least


def tolerance(word):
    """Edit distance an unknown query token of this length tolerates (TOLERANCE)."""
    return 0 if len(word) <= TOLERANCE[0] else 1 if len(word) <= TOLERANCE[1] else 2


# ---------------------------------------------------------------- rows
def aliases(text):
    """The aliases text of an evidence document: sections headed `Aliases...` and paragraphs or lists opened by a line
    `Aliases...:` (`Catalog aliases:`, `- **Aliases elsewhere:**`, `Aliases (2026-10-19 sweep):`), without those labels
    and links' targets."""
    out, section, paragraph = [], False, False
    for line in text.splitlines():
        label = re.match(r'\s*(?:[-*]\s+)?(?:\*\*)?(?:\w+\s+)?aliases\b[^:`]*:(?:\*\*)?', line, re.I)
        if line.startswith('#'):
            section, paragraph = bool(re.match(r'#+\s*aliases\b', line, re.I)), False
        elif section or paragraph and line.strip() or label:
            out.append(line[label.end():] if label else line)  # the label itself is no alias
            paragraph = paragraph or bool(label)
        else:
            paragraph = False
    return re.sub(r'https?://\S+|\]\([^)]*\)', ' ', '\n'.join(out))


def bare(file):
    """A header's or module's file stem without its NN- or _NN_ prefix."""
    return re.sub(r'^(?:\d{2}-|_\d{2}_)', '', Path(file).stem)


def rows(root=ROOT):
    """Every inventory row of 01-08 with its names, tier, status and field tokens."""
    out = []
    for index in sorted(root.glob('0[1-8]-*/00-index.md')):
        folder, tier = index.parent.name, 'Basic'
        for line in index.read_text(encoding='utf-8').splitlines():
            if line.startswith('## '):
                tier = line[3:].strip()
            c = cells(line) if line.startswith('| `') else []
            m = re.match(r'`([^`]+\.(?:hpp|py))`', c[0]) if len(c) >= 3 else None
            if not m:
                continue
            cell, status, header = c[-2], c[-1], f'{folder}/{m[1]}'
            doc = index.parent / 'Docs' / (Path(m[1]).stem + '.md')
            qual = ''.join(ch for ch, d in zip(cell, depths(cell)) if d) + ' '
            base, offered = bare(m[1]), names(cell)
            ident = [base.replace('_', '')] + [x for n in offered for x in n.split('::') if not x.startswith('operator')]
            fields = {'stem': base, 'ident': ' '.join(ident), 'ops': unqualified(cell), 'qual': qual,
                      'aliases': aliases(doc.read_text(encoding='utf-8')) if doc.is_file() else '',
                      'folder': re.sub(r'^\d{2}-', '', folder)}
            items = {tuple(tokens(m[1])) for m in ITEM.finditer(unbounded(fold(fields['aliases']))) if not m[2]} - {()}
            # each alias's tokens; a piece before a colon labels the next (`OI Wiki: 线段树`), no alias
            runs = {simplified(x) for t in fields.values() for x in re.findall(f'[{HAN}]+', fold(t))}  # its Chinese runs
            out.append(Row(header, folder, ('Basic' if re.match(r'yes\b', c[1]) else 'Advanced') if len(c) == 4 else tier,  # Basic: `yes until mini exists`
                           next((s for s in STATUSES if status.startswith(s)), status.split(';')[0]), offered, tokens(base),
                           {f: [stem(w.lower()) for w in t.split()] if f == 'ident' else tokens(t) for f, t in fields.items()},
                           notes(cell), items, runs))
    return out


# ---------------------------------------------------------------- ranking
class Search:
    """BM25F index of the rows; find(text, n) ranks them."""

    def __init__(self, root=ROOT, weights=None, b=B, k1=K1, typo=None, prefix=PREFIX, phrase=PHRASE, standing=None,
                 level=None):
        self.rows, self.w, self.b, self.k1 = rows(root), weights or WEIGHTS, b, k1
        self.typo, self.prefix, self.phrase = typo or TYPO, prefix, phrase
        self.standing, self.level = standing or STANDING, level or LEVEL
        known = {t for r in self.rows for f in ('ops', 'qual', 'aliases') for t in r.fields[f] if len(t) >= PART}
        pieces = known | STOP  # stop words split a stem part too (`meetinthemiddle`), yielding nothing
        for r in self.rows:  # a stem part that is no known word (`segmenttree`) also yields the known words it joins
            r.fields['stem'] += [stem(p) for w in words(bare(r.header)) if stem(w) not in known for p in segment(w, pieces) if p not in STOP]
        self.tf = [{f: Counter(r.fields[f]) for f in FIELDS} for r in self.rows]
        self.pairs = [{p for f in FIELDS for p in zip(r.fields[f], r.fields[f][1:])} for r in self.rows]
        self.avg = {f: sum(len(r.fields[f]) for r in self.rows) / max(1, len(self.rows)) or 1 for f in FIELDS}
        self.where, self.cache = {}, {}
        for i, tf in enumerate(self.tf):
            for t in set().union(*tf.values()):
                self.where.setdefault(t, set()).add(i)
        df = {t: len(rows) for t, rows in self.where.items()}
        self.idf = {t: math.log(1 + (len(self.rows) - d + 0.5) / (d + 0.5)) for t, d in df.items()}
        self.vocabulary, self.by_length, self.holding = sorted(self.idf), {}, {}
        for t in (t for t in self.vocabulary if chinese(t)):
            for c in set(t):
                self.holding.setdefault(c, []).append(t)  # a Chinese character -> the tokens holding it
        self.lone = {c: math.log(1 + (len(self.rows) - d + 0.5) / (d + 0.5)) for c, ts in self.holding.items()
                     for d in [len(set().union(*(self.where[t] for t in ts)))]}  # its idf over the rows holding it
        for t in self.vocabulary:
            self.by_length.setdefault(len(t), []).append((t, frozenset(t)))
        held = '\n'.join(sorted({x for r in self.rows for x in r.runs}))  # the documents' Chinese runs
        rp = {k: v for k, v in tables()[0]['rp'][0].items() if k not in held}
        self.regional = (rp, max(map(len, rp), default=1))  # a phrase the documents hold in one form is mainland text
        # (序列 is no Taiwan 串行 there); one only Traditional writes (查詢) is in no one form and keeps its regional reading
        self.name_words = [[set(tokens(n)) | set(tokens(r.notes.get(n, ''))) for n in r.names] for r in self.rows]  # qualifiers count
        self.parts = [[words(n, ()) for n in r.names] for r in self.rows]  # each name's words
        self.joined = [stem(''.join(words(bare(r.header)))) for r in self.rows]  # the stem as one word (`segtree`)
        self.runs = [[{''.join(ws[i:j]) for i in range(len(ws)) for j in range(i + 1, len(ws) + 1)} for ws in p] for p in self.parts]
        # each name's runs of whole words, joined

    def alternatives(self, word, prefix=True):
        """[(vocabulary token, factor)] a query token stands for: itself when known, else its typo matches and, with
        prefix, the tokens it begins; a lone Chinese character stands for the tokens holding it, a pair by the prefix
        factor, each scaled to the character's own idf."""
        if word in self.holding:
            return [(t, (1.0 if t == word else self.prefix) * self.lone[word] / self.idf[t]) for t in self.holding[word]]
        if word in self.idf:
            return [(word, 1.0)]
        cap, out, letters = tolerance(word), {}, set(word)
        for t, chars in (p for n in (range(len(word) - cap, len(word) + cap + 1) if cap else ()) for p in self.by_length.get(n, ())):
            if len(letters ^ chars) <= 2 * cap and (d := distance(word, t, cap)) <= cap:  # an edit changes the letter set by 2 at most
                out[t] = self.typo.get(d, 0.0)
        if prefix and len(word) >= PREFIX_MIN:
            for t in self.vocabulary[bisect.bisect_left(self.vocabulary, word):bisect.bisect_left(self.vocabulary, word + '\U0010ffff')]:
                out[t] = max(out.get(t, 0.0), self.prefix)
        return [(t, f) for t, f in out.items() if f]

    def weight(self, i, t):
        """BM25F weight of token t in row i."""
        x = sum(self.w[f] * self.tf[i][f][t] / (1 - self.b + self.b * len(self.rows[i].fields[f]) / self.avg[f])
                for f in FIELDS if self.tf[i][f][t])
        return self.idf[t] * x * (self.k1 + 1) / (self.k1 + x) if x else 0.0

    def reading(self, text):
        """text with each run of Chinese characters in its regional reading (Taiwan's 後綴陣列: 后缀数组)."""
        return re.sub(f'[{HAN}]+', lambda m: simplified(m[0], self.regional), fold(text))

    def terms(self, text):
        """Query groups: one per token, plus one per 2 or 3 adjacent words, none Chinese, that join to a known token
        (`block cut tree` -> `blockcuttree`); and the group pairs adjacent in the text's own or regional reading."""
        alt = functools.cache(lambda w: self.alternatives(stem(w)))
        ws = words(text, regional=self.regional)
        joins = [self.alternatives(stem(''.join(ws[s:s + k])), prefix=False) for k in (2, 3) for s in range(len(ws) - k + 1)
                 if not any(map(chinese, ws[s:s + k]))]  # Chinese pairs overlap already; a join matches as typed or
        # within its typo tolerance only. ws holds a run's regional pairs after its own: adjacency is per reading
        readings = dict.fromkeys(tuple(words(t)) for t in (text, self.reading(text)))
        adjacent = dict.fromkeys(p for r in readings for p in zip(r, r[1:]))
        return [g for g in map(alt, ws) if g] + [g for g in joins if g], [(alt(x), alt(y)) for x, y in adjacent]

    def postings(self, t):
        """{row: BM25F weight} of the rows holding token t (cached)."""
        if t not in self.cache:
            self.cache[t] = {i: self.weight(i, t) for i in self.where[t]}
        return self.cache[t]

    def scores(self, groups, adjacent):
        """{row: score}: per group its best alternative's weight times its factor, plus for each pair of adjacent groups
        the phrase bonus of their best alternatives adjacent in a field: their mean idf, each times its factor."""
        out = Counter()
        for g in groups:
            best = {}
            for t, f in g:
                for i, w in self.postings(t).items():
                    best[i] = max(best.get(i, 0.0), f * w)
            out.update(best)
        for a, b in adjacent:
            both = set().union(*(self.where[x] for x, _ in a)) & set().union(*(self.where[y] for y, _ in b))
            for i in both:
                if bonus := max(((f * self.idf[x] + g * self.idf[y]) / 2 for x, f in a for y, g in b if (x, y) in self.pairs[i]), default=0):
                    out[i] += self.phrase * bonus
        return out

    def find(self, text, n=10):
        """The n best rows for text, best first: rows whose header stem has the tokens of text, then rows with a name
        equal to text (case, fullwidth forms, Latin diacritics and complexity expressions ignored, also a name's last `::`
        part), then rows with an alias whose tokens equal those of text or of its regional reading (`線段樹` and `并查集`
        name their rows), then, for a one-word text, rows with a name holding it as a run of whole words (`bit` in
        `BitOps`, not `sam` in `same`) or as pieces that begin consecutive words (shortened), then the others, which
        always hold 08-Python modules and planned or legacy-reference rows. The name classes order by tier, status, then
        score, the others by score times the status and tier factors. Each row comes with the name to grab: the name
        equal to text, else the shortest containing it, else the name sharing the most query weight, then the most
        header-stem words (the first on a tie)."""
        groups, adjacent = self.terms(text)
        score = self.scores(groups, adjacent)
        q, joined, typed = unbounded(fold(text)).strip().lower(), ''.join(words(text, ())), tokens(text)
        read = {tuple(typed), tuple(tokens(self.reading(text)))}
        one = len(q.split()) == 1  # words with spaces describe a concept, not part of a name
        ranked = []
        for i, r in enumerate(self.rows):
            exact = [x for x in r.names if q in (x.lower(), x.lower().split('::')[-1])]
            within = sorted((x for x, runs, ws in zip(r.names, self.runs[i], self.parts[i]) if joined in runs or shortened(joined, ws)),
                            key=len) if one else []
            s = round(score[i] * self.standing.get(r.status, 1) * self.level.get(r.tier, 1), 9)
            own = typed and typed == r.key  # the header stem names its row too
            alias = not read.isdisjoint(r.items)
            if not (exact or within or own or alias or s > 0):
                continue
            shared = lambda k: sum(max((f * self.idf[t] for t, f in g if t in k), default=0) for g in groups)
            near = set(r.fields['stem']) | {self.joined[i]}
            top = max(zip(r.names, self.name_words[i]), key=lambda p: (shared(p[1]), len(p[1] & near)), default=None)
            best = (exact or within or [top[0] if top and (shared(top[1]) or own) else r.header])[0]  # no name shares a word
            cls = 0 if r.header.endswith('.py') or r.status not in STATUSES[:3] else -4 if own else -3 if exact else -2 if alias else -1 if within else 0
            # a module, a planned or a legacy-reference row has nothing grab can copy, so it takes no name class
            plain = (TIERS.index(r.tier) if r.tier in TIERS else len(TIERS),
                     STATUSES.index(r.status) if r.status in STATUSES else len(STATUSES))  # simpler, more finished first
            ranked.append(((cls, plain if cls else (), -s, plain, r.header), Hit(best, r.header, r.tier, r.status, s)))
        return [h for _, h in sorted(ranked)[:n]]


def show(hits, held=()):
    """One line per hit: name (- for a row no name of which matched, whose name is its header), header, tier, status, in
    columns as wide as their longest entry (at least 30, 46 and 9); a Python module also says Python, a header in held
    (the ones the contest template already holds) says in template."""
    hits = [h._replace(name='-') if h.name == h.header else h for h in hits]
    w = [max([m, *(len(h[i]) for h in hits)]) for i, m in enumerate((30, 46, 9, 0))]
    return [(f"{h.name:{w[0]}} {h.header:{w[1]}} {h.tier:{w[2]}} {h.status:{w[3]}}" + ('  Python' if h.header.endswith('.py') else '')
             + ('  in template' if h.header in held else '')).rstrip() for h in hits]
