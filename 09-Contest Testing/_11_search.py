"""Concept search over the library's inventory rows: `10-grab.py --find WORDS` and the rows grab lists for an unknown name.

A document is one row of a folder's 00-index.md (01-08): its header or module stem split at `_`, the identifiers of its
Operations cell (camelCase split), the cell's parenthesized qualifiers, its evidence document's aliases and its folder.
Ranking: field-weighted BM25 over normalized tokens (lower case, light stemming, stop words dropped), per-token typo and
prefix tolerance, adjacent query words joined, a phrase bonus, times factors by status and tier (finished and simple rows
first); a grabbable name equal to the query, then one containing a one-word query, ranks first, simplest first; equal
scores go to the higher tier, then the more finished status. Standard library only, no index or cache.
Rules: 00-Guidelines/08-contest-tools.md; weights, cutoffs and their evaluation: 96-Local Testing/Docs/00-search.md.
"""
from __future__ import annotations

import bisect
import math
import re
from collections import Counter, namedtuple
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TIERS = ('Basic', 'Advanced', 'Esoteric')  # tie order; a 01-Core row outside Basic counts as Advanced
STATUSES = ('verified', 'partial', 'existing-unverified', 'legacy-reference', 'planned')
FIELDS = ('stem', 'ident', 'ops', 'qual', 'aliases', 'folder')
WEIGHTS = {'stem': 1.0, 'ident': 1.0, 'ops': 1.0, 'qual': 0.25, 'aliases': 2.0, 'folder': 0.6}  # chosen on the corpus (00-search.md)
B, K1 = 0.5, 1.2
TYPO = {1: 0.7, 2: 0.5}  # factor of a vocabulary token at that edit distance from an unknown query token
PREFIX = 0.75  # factor of a vocabulary token an unknown query token (3 letters or more) begins
PHRASE = 1.0  # bonus per adjacent query pair adjacent in a field, times the pair's mean idf
TOLERANCE = (3, 7)  # an unknown query token tolerates no edit up to the first length, 1 up to the second, then 2
PREFIX_MIN = 2  # letters an unknown query token needs to match the tokens it begins
PART = 2  # letters of each known word a header stem splits into
STANDING = {'verified': 1.0, 'partial': 1.0, 'existing-unverified': 1.0, 'legacy-reference': 0.9, 'planned': 0.9}  # score
LEVEL = {'Basic': 1.0, 'Advanced': 1.0, 'Esoteric': 0.9}  # factors by status and tier: finished, simple rows first
STOP = frozenset('a an and as at by for from in into is it of on or the to via with'.split())
IRREGULAR = {'vertices': 'vertex', 'indices': 'index', 'matrices': 'matrix', 'series': 'series', 'species': 'species', 'tries': 'trie'}
Row = namedtuple('Row', 'header folder tier status names key fields notes')
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
def stem(word):
    """Light stemming: plural (ies, ches/shes/sses/xes/zzes, s) and -ing, with a few irregular plurals."""
    if word in IRREGULAR:
        return IRREGULAR[word]
    if len(word) > 4 and word.endswith('ies'):
        word = word[:-3] + 'y'
    elif len(word) > 4 and re.search(r'(?:ch|sh|ss|x|zz)es$', word):
        word = word[:-2]
    elif len(word) > 3 and word.endswith('s') and not re.search(r'(?:ss|us|is)$', word):
        word = word[:-1]
    if len(word) >= 7 and word.endswith('ing'):
        word = word[:-3]
        if word[-1] == word[-2] and word[-1] not in 'aeioulsz':  # spanning -> span, but rolling -> roll, agreeing -> agree
            word = word[:-1]
    return word


def words(text, stop=STOP):
    """Lower-case words of text, identifiers split at `_`, camelCase and letter-digit boundaries, unstemmed, without
    the words in stop."""
    text = re.sub(r'([a-z\d])([A-Z])', r'\1 \2', re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1 \2', text))
    return [w for w in re.findall(r'[^\W\d_]+|\d+', text.lower()) if w not in stop]


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
    """The fewest known words (PART letters or more) that join to word, or [] when there are none or word is one."""
    best = [[]] + [None] * len(word)
    for e in range(PART, len(word) + 1):
        for b in range(e - PART + 1):
            if best[b] is not None and word[b:e] in known and (best[e] is None or len(best[b]) + 1 < len(best[e])):
                best[e] = best[b] + [word[b:e]]
    return best[-1] if best[-1] and len(best[-1]) > 1 else []


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
            base, offered = re.sub(r'^(?:\d{2}-|_\d{2}_)', '', Path(m[1]).stem), names(cell)
            ident = [base.replace('_', '')] + [x for n in offered for x in n.split('::') if not x.startswith('operator')]
            fields = {'stem': base, 'ident': ' '.join(ident), 'ops': unqualified(cell), 'qual': qual,
                      'aliases': aliases(doc.read_text(encoding='utf-8')) if doc.is_file() else '',
                      'folder': re.sub(r'^\d{2}-', '', folder)}
            out.append(Row(header, folder, ('Basic' if re.match(r'yes\b', c[1]) else 'Advanced') if len(c) == 4 else tier,  # Basic: `yes until mini exists`
                           next((s for s in STATUSES if status.startswith(s)), status.split(';')[0]), offered, tokens(base),
                           {f: [stem(w.lower()) for w in t.split()] if f == 'ident' else tokens(t) for f, t in fields.items()},
                           notes(cell)))
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
        for r in self.rows:  # a stem without underscores (`segmenttree`) also yields the words it joins
            r.fields['stem'] += [p for t in r.key if t not in known for p in segment(t, known)]
        self.tf = [{f: Counter(r.fields[f]) for f in FIELDS} for r in self.rows]
        self.pairs = [{p for f in FIELDS for p in zip(r.fields[f], r.fields[f][1:])} for r in self.rows]
        self.avg = {f: sum(len(r.fields[f]) for r in self.rows) / max(1, len(self.rows)) or 1 for f in FIELDS}
        self.where, self.cache = {}, {}
        for i, tf in enumerate(self.tf):
            for t in set().union(*tf.values()):
                self.where.setdefault(t, set()).add(i)
        df = {t: len(rows) for t, rows in self.where.items()}
        self.idf = {t: math.log(1 + (len(self.rows) - d + 0.5) / (d + 0.5)) for t, d in df.items()}
        self.vocabulary, self.by_length = sorted(self.idf), {}
        for t in self.vocabulary:
            self.by_length.setdefault(len(t), []).append((t, frozenset(t)))
        self.name_words = [[set(tokens(n)) | set(tokens(r.notes.get(n, ''))) for n in r.names] for r in self.rows]  # qualifiers count
        self.runs = [[{''.join(ws[i:j]) for i in range(len(ws)) for j in range(i + 1, len(ws) + 1)} for ws in (words(n, ()) for n in r.names)]
                     for r in self.rows]  # each name's runs of whole words, joined

    def alternatives(self, word, prefix=True):
        """[(vocabulary token, factor)] a query token stands for: itself when known, else its typo matches and, with
        prefix, the tokens it begins."""
        if word in self.idf:
            return [(word, 1.0)]
        cap, out, letters = tolerance(word), {}, set(word)
        for t, chars in (p for n in (range(len(word) - cap, len(word) + cap + 1) if cap else ()) for p in self.by_length.get(n, ())):
            if len(letters ^ chars) <= 2 * cap and (d := distance(word, t, cap)) <= cap:  # an edit changes the letter set by 2 at most
                out[t] = self.typo.get(d, 0.0)
        if prefix and len(word) >= PREFIX_MIN:
            for t in self.vocabulary[bisect.bisect_left(self.vocabulary, word):bisect.bisect_left(self.vocabulary, word + '\uffff')]:
                out[t] = max(out.get(t, 0.0), self.prefix)
        return [(t, f) for t, f in out.items() if f]

    def weight(self, i, t):
        """BM25F weight of token t in row i."""
        x = sum(self.w[f] * self.tf[i][f][t] / (1 - self.b + self.b * len(self.rows[i].fields[f]) / self.avg[f])
                for f in FIELDS if self.tf[i][f][t])
        return self.idf[t] * x * (self.k1 + 1) / (self.k1 + x) if x else 0.0

    def terms(self, text):
        """Query groups: one per token, plus one per 2 or 3 adjacent words that join to a known token (`block cut tree`
        -> `blockcuttree`)."""
        ws = words(text)
        single = [self.alternatives(stem(w)) for w in ws]
        joins = [self.alternatives(stem(''.join(ws[s:s + k])), prefix=False) for k in (2, 3) for s in range(len(ws) - k + 1)]
        return [g for g in single + joins if g], single  # a join matches as typed or within its typo tolerance only

    def postings(self, t):
        """{row: BM25F weight} of the rows holding token t (cached)."""
        if t not in self.cache:
            self.cache[t] = {i: self.weight(i, t) for i in self.where[t]}
        return self.cache[t]

    def scores(self, groups, sequence):
        """{row: score}: per group its best alternative's weight times its factor, plus the phrase bonus of each
        adjacent pair of query tokens adjacent in a field."""
        out = Counter()
        for g in groups:
            best = {}
            for t, f in g:
                for i, w in self.postings(t).items():
                    best[i] = max(best.get(i, 0.0), f * w)
            out.update(best)
        for a, b in zip(sequence, sequence[1:]):
            both = set().union(*(self.where[x] for x, _ in a)) & set().union(*(self.where[y] for y, _ in b))
            for i in (i for i in both if any((x, y) in self.pairs[i] for x, _ in a for y, _ in b)):
                out[i] += self.phrase * (max(self.idf[x] for x, _ in a) + max(self.idf[y] for y, _ in b)) / 2
        return out

    def find(self, text, n=10):
        """The n best rows for text, best first: rows whose header stem has the tokens of text, then rows with a name
        equal to text (case ignored, also a name's last `::` part), then, for a one-word text, rows with a name holding
        it as a run of whole words (`bit` in `BitOps`, `segtree` in `SegTreeBeats`, not `sam` in `same`), then the
        others, which always hold 08-Python modules and planned or legacy-reference rows. The name classes order by
        tier, status, then score, the others by score times the status and tier factors. Each row comes with the name
        to grab: the name equal to text, else the shortest containing it, else the name sharing the most query weight,
        then the most header-stem words (the first on a tie)."""
        groups, sequence = self.terms(text)
        score = self.scores(groups, sequence)
        q, joined, typed = text.strip().lower(), ''.join(words(text, ())), tokens(text)
        one = len(q.split()) == 1  # words with spaces describe a concept, not part of a name
        ranked = []
        for i, r in enumerate(self.rows):
            exact = [x for x in r.names if q in (x.lower(), x.lower().split('::')[-1])]
            within = sorted((x for x, runs in zip(r.names, self.runs[i]) if joined in runs), key=len) if one else []
            s = round(score[i] * self.standing.get(r.status, 1) * self.level.get(r.tier, 1), 9)
            own = typed and typed == r.key  # the header stem names its row too
            if not (exact or within or own or s > 0):
                continue
            shared = lambda k: sum(max((f * self.idf[t] for t, f in g if t in k), default=0) for g in groups)
            near = set(r.fields['stem']) | {''.join(r.key)}
            top = max(zip(r.names, self.name_words[i]), key=lambda p: (shared(p[1]), len(p[1] & near)), default=None)
            best = (exact or within or [top[0] if top and (shared(top[1]) or own) else r.header])[0]  # no name shares a word
            cls = 0 if r.header.endswith('.py') or r.status not in STATUSES[:3] else -3 if own else -2 if exact else -1 if within else 0
            # a module, a planned or a legacy-reference row has nothing grab can copy, so it takes no name class
            plain = (TIERS.index(r.tier) if r.tier in TIERS else len(TIERS),
                     STATUSES.index(r.status) if r.status in STATUSES else len(STATUSES))  # simpler, more finished first
            ranked.append(((cls, plain if cls else (), -s, plain, r.header), Hit(best, r.header, r.tier, r.status, s)))
        return [h for _, h in sorted(ranked)[:n]]


def show(hits, held=()):
    """One line per hit: name, header, tier, status; a Python module also says Python, a header in held (the ones the
    contest template already holds) says in template."""
    return [f"{h.name:30} {h.header:46} {h.tier:9} {h.status}" + ('  Python' if h.header.endswith('.py') else '')
            + ('  in template' if h.header in held else '') for h in hits]
