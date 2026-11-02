#include "../../02-Data Structures/62-xorbasis.hpp"

namespace {
    // Debug containers check every subscript, so the checked build scales the large corpora down.
#ifdef _GLIBCXX_DEBUG
    constexpr int SCALE = 20;
#elif defined(__SANITIZE_ADDRESS__)
    constexpr int SCALE = 4;
#else
    constexpr int SCALE = 1;
#endif
    ulng seed = 20261103;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    ulng bit(int i) { return ulng(1) << i; }
    ulng wmask(int w) { return w == 64 ? ~ulng(0) : bit(w) - 1; }
    int lead(ulng x) { return 63 - std::countl_zero(x); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename A, typename B> string show(const pair<A, B> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    // what() builds the operation text only on a mismatch.
    template<typename T, typename U, typename F> void expect(const T &got, const U &want, F &&what) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + what() + " expected=" + show(want) + " actual=" + show(got)); }}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) { expect(got, want, [&] { return op; }); }

    // Oracle written independently of the header: piv[i] has leading bit i (echelon, not reduced).
    struct Ech {
        array<ulng, 64> piv{};
        int dim = 0;
        bool add(ulng x) {
            while (x) {
                int i = lead(x);
                if (!piv[i]) { piv[i] = x; ++dim; return true; }
                x ^= piv[i];}
            return false;}
        ulng low(ulng x) const {
            for (int i = 63; i >= 0; --i) {
                if (x >> i & 1 && piv[i]) { x ^= piv[i]; }}
            return x;}
        bool has(ulng x) const { return !low(x); }
        ulng high(ulng x) const {
            for (int i = 63; i >= 0; --i) {
                if (piv[i] && !(x >> i & 1)) { x ^= piv[i]; }}
            return x;}
        // Eliminating pivot bits from the highest down only adds bits that are eliminated later.
        array<ulng, 64> reduced() const {
            array<ulng, 64> t = piv;
            for (int i = 63; i >= 0; --i) {
                for (int j = i + 1; t[i] && j < 64; ++j) {
                    if (t[j] >> i & 1) { t[j] ^= t[i]; }}}
            return t;}
        vector<ulng> rref() const {
            array<ulng, 64> t = reduced();
            vector<ulng> res;
            for (int i = 63; i >= 0; --i) {
                if (t[i]) { res.push_back(t[i]); }}
            return res;}
        // Span values y < x, counted by the highest bit t where y first drops below x: reduced rows fix y's pivot bits as its coordinates.
        ulng countBelow(ulng x) const {
            array<ulng, 64> t = reduced();
            ulng res = 0, z = 0;
            int below = dim;
            for (int i = 63; i >= 0; --i) {
                if (t[i]) { --below; }
                if (x >> i & 1 && (t[i] || !(z >> i & 1))) { res += bit(below); }
                if (t[i]) {
                    if (x >> i & 1) { z ^= t[i]; }}
                else if ((z >> i & 1) != (x >> i & 1)) { break; }}
            return res;}
    };
    Ech echOf(const vector<ulng> &v) {
        Ech e;
        for (ulng x : v) { e.add(x); }
        return e;}
    Ech slice(const vector<ulng> &a, int l, int r) {
        Ech e;
        for (int i = l; i < r; ++i) { e.add(a[i]); }
        return e;}
    // Brute-force span of small-width vectors: every xor combination, sorted.
    vector<ulng> closure(const vector<ulng> &v) {
        vector<ulng> s{0};
        for (ulng x : v) {
            if (std::ranges::binary_search(s, x)) { continue; }
            size_t k = s.size();
            for (size_t i = 0; i < k; ++i) { s.push_back(s[i] ^ x); }
            sort(s.begin(), s.end());}
        return s;}

    // Successful inserts in order and their span.
    struct Model {
        Ech e;
        vector<ulng> ins;
        void push(ulng x) { ins.push_back(x); e.add(x); }
    };
    ulng comb(Rng &rng, const vector<ulng> &v) {
        ulng x = 0;
        for (ulng y : v) {
            if (rng() & 1) { x ^= y; }}
        return x;}
    // Kinds: 0 uniform, 1 sparse, 2 top bit set, 3 dependent, 4 zero, 5 power of two, 6 duplicate, 7 dependent plus a bit.
    ulng gen(Rng &rng, int w, const vector<ulng> &ins, int kind) {
        ulng M = wmask(w);
        if (kind == 0) { return rng() & M; }
        if (kind == 1) {
            ulng x = 0;
            for (lng c = rnd(rng, 1, 3); c > 0; --c) { x |= bit(int(rng() % ulng(w))); }
            return x;}
        if (kind == 2) { return bit(w - 1) | (rng() & M); }
        if (kind == 3) { return comb(rng, ins); }
        if (kind == 4) { return 0; }
        if (kind == 5) { return bit(int(rng() % ulng(w))); }
        if (kind == 6) { return ins.empty() ? 0 : ins[rng() % ins.size()]; }
        return comb(rng, ins) ^ bit(int(rng() % ulng(w)));}
    // Profiles: 0 uniform, 1 sparse, 2 top bit, 3 dependents, 4 mixed.
    int kindOf(Rng &rng, int profile) {
        int r = int(rng() % 8);
        if (profile == 0) { return r < 6 ? 0 : r == 6 ? 3 : 4; }
        if (profile == 1) { return r < 5 ? 1 : r < 7 ? 5 : 3; }
        if (profile == 2) { return r < 6 ? 2 : 7; }
        if (profile == 3) { return std::array{3, 3, 6, 4, 7, 0, 3, 6}[r]; }
        return r;}

    template<bool MASK> void checkShape(const XorBasis<MASK> &x, const string &op) {
        checkEqual(x.size(), int(x.b.size()), op + " size() == b.size()");
        checkEqual(x.masks.size(), MASK ? x.b.size() : 0, op + " masks.size()");
        for (int i = 0; i < x.size(); ++i) {
            expect(x.b[i] != 0, true, [&] { return op + " b[" + show(i) + "] nonzero"; });
            if (i) { expect(x.b[i - 1] > x.b[i], true, [&] { return op + " b strictly decreasing at " + show(i); }); }
            for (int j = 0; j < x.size(); ++j) {
                if (j != i && x.b[j] >> lead(x.b[i]) & 1) { checkEqual(true, false, op + " leading bit of b[" + show(i) + "] in b[" + show(j) + "] b=" + show(x.b)); }}
            if (MASK && x.size() < 64) { expect(x.masks[i] >> x.size(), ulng(0), [&] { return op + " masks[" + show(i) + "] within successful inserts"; }); }}}
    ulng maskXor(const vector<ulng> &ins, ulng mask) {
        ulng x = 0;
        for (int j = 0; j < 64; ++j) {
            if (mask >> j & 1) { x ^= j < int(ins.size()) ? ins[j] : 0; }}
        return x;}
    template<bool MASK> void checkBasis(const XorBasis<MASK> &x, const Model &md, const string &op) {
        checkShape(x, op);
        checkEqual(x.size(), md.e.dim, op + " size() vs oracle dim");
        checkEqual(x.size(), int(md.ins.size()), op + " size() vs successful inserts");
        expect(x.b, md.e.rref(), [&] { return op + " b is the reduced echelon form of the span"; });
        for (int i = 0; i < x.size() && MASK; ++i) { expect(maskXor(md.ins, x.masks[i]), x.b[i], [&] { return op + " xor of inserts in masks[" + show(i) + "]=" + show(x.masks[i]); }); }}

    template<bool MASK> void checkValue(const XorBasis<MASK> &x, const Model &md, ulng v) {
        expect(x.reduce(v), md.e.low(v), [&] { return "reduce(" + show(v) + ")"; });
        expect(x.contains(v), md.e.has(v), [&] { return "contains(" + show(v) + ")"; });
        expect(x.max(v), md.e.high(v), [&] { return "max(" + show(v) + ")"; });
        expect(x.rank(v), md.e.countBelow(v), [&] { return "rank(" + show(v) + ")"; });
        if constexpr (MASK) {
            ulng mask = ~ulng(0);
            bool ok = x.represent(v, mask);
            expect(ok, md.e.has(v), [&] { return "represent(" + show(v) + ") status"; });
            if (ok) {
                expect(maskXor(md.ins, mask), v, [&] { return "represent(" + show(v) + ") mask=" + show(mask); });
                if (x.size() < 64) { expect(mask >> x.size(), ulng(0), [&] { return "represent(" + show(v) + ") mask within inserts"; }); }}
            if (!v) { checkEqual(mask, ulng(0), "represent(0) mask"); }}}
    template<bool MASK> void checkKth(const XorBasis<MASK> &x, const Model &md, ulng k) {
        ulng v = x.kth(k);
        expect(md.e.has(v), true, [&] { return "kth(" + show(k) + ")=" + show(v) + " in span"; });
        expect(md.e.countBelow(v), k, [&] { return "oracle rank of kth(" + show(k) + ")=" + show(v); });
        expect(x.rank(v), k, [&] { return "rank(kth(" + show(k) + "))"; });}
    // Brute force over the whole span for widths <= 8; all x in [0, 2^w] when exhaustive.
    template<bool MASK> void checkBrute(Rng &rng, const XorBasis<MASK> &x, const Model &md, int w, bool exhaustive) {
        vector<ulng> s = closure(md.ins);
        checkEqual(s.size(), size_t(bit(md.e.dim)), "brute span size is 2^dim");
        vector<ulng> xs{~ulng(0), bit(w)};
        for (ulng v = 0; v <= wmask(w) && exhaustive; ++v) { xs.push_back(v); }
        for (int t = 0; t < 8 && !exhaustive; ++t) { xs.push_back(rng() & wmask(w)); }
        for (ulng v : xs) {
            ulng lo = ~ulng(0), hi = 0;
            for (ulng y : s) { lo = min(lo, v ^ y); hi = max(hi, v ^ y); }
            expect(x.reduce(v), lo, [&] { return "brute reduce(" + show(v) + ")"; });
            expect(x.max(v), hi, [&] { return "brute max(" + show(v) + ")"; });
            expect(x.contains(v), std::ranges::binary_search(s, v), [&] { return "brute contains(" + show(v) + ")"; });
            expect(x.rank(v), ulng(std::ranges::lower_bound(s, v) - s.begin()), [&] { return "brute rank(" + show(v) + ")"; });
            if constexpr (MASK) {
                ulng mask = 0;
                if (x.represent(v, mask)) { expect(maskXor(md.ins, mask), v, [&] { return "brute represent(" + show(v) + ")"; }); }}}
        for (size_t k = 0; k < s.size(); ++k) { expect(x.kth(k), s[k], [&] { return "brute kth(" + show(k) + ")"; }); }}
    template<bool MASK> void checkQueries(Rng &rng, const XorBasis<MASK> &x, const Model &md, int w) {
        ulng M = wmask(w), c = comb(rng, md.ins);
        for (ulng v : {ulng(0), ~ulng(0), M, ulng(rng()), ulng(rng() & M), c, c ^ 1, c + 1, c - 1, x.size() ? x.b[0] : ulng(1), x.size() ? x.b.back() : ulng(2)}) { checkValue(x, md, v); }
        checkEqual(x.max(), md.e.high(0), "max() default argument");
        int r = x.size();
        ulng top = r == 64 ? ~ulng(0) : bit(r) - 1;
        for (ulng k : {ulng(0), top, top >> 1, (top >> 1) + 1, ulng(rng() & top), ulng(rng() & top)}) {
            if (k <= top) { checkKth(x, md, k); }}
        if (w <= 8) { checkBrute(rng, x, md, w, w <= 5); }}

    // Random histories of insert, merge (other, self, empty, into empty) and queries; a fresh basis per episode, mirrored by an XorBasis<false> xu.
    void history(Rng &rng, int w, int profile, int steps) {
        for (int done = 0; done < steps;) {
            int len = int(rnd(rng, 1, 3 * w + 8));
            XorBasis x;
            XorBasis<false> xu;
            Model md;
            for (int s = 0; s < len && done < steps; ++s, ++done) {
                context = "history w=" + show(w) + " profile=" + show(profile) + " step=" + show(done) + " size=" + show(x.size()) + " seed=" + show(seed);
                int op = int(rng() % 100);
                string name;
                if (op < 70) {
                    ulng v = gen(rng, w, md.ins, kindOf(rng, profile));
                    bool want = !md.e.has(v);
                    name = "insert(" + show(v) + ")";
                    checkEqual(x.insert(v), want, name);
                    checkEqual(xu.insert(v), want, "unmasked " + name);
                    if (want) { md.push(v); }}
                else if (op < 82) {
                    XorBasis o;
                    XorBasis<false> ou;
                    Model om;
                    for (lng k = rnd(rng, 0, w + 1); k > 0; --k) {
                        ulng v = gen(rng, w, om.ins, kindOf(rng, profile));
                        bool want = !om.e.has(v);
                        checkEqual(o.insert(v), want, "other insert(" + show(v) + ")");
                        checkEqual(ou.insert(v), want, "unmasked other insert(" + show(v) + ")");
                        if (want) { om.push(v); }}
                    checkBasis(o, om, "other basis");
                    checkBasis(ou, om, "unmasked other basis");
                    xu.merge(ou);
                    for (ulng y : o.b) {
                        if (!md.e.has(y)) { md.push(y); }}
                    name = "merge(o=" + show(o.b) + ")";
                    x.merge(o);}
                else if (op < 86) {
                    XorBasis before = x;
                    x.merge(x); xu.merge(xu);
                    name = "self merge";
                    checkEqual(x.b, before.b, "self merge keeps b"); checkEqual(x.masks, before.masks, "self merge keeps masks");}
                else if (op < 88) {
                    x.merge(XorBasis{}); xu.merge(XorBasis<false>{});
                    name = "merge(empty)";}
                else if (op < 90) {
                    XorBasis c;
                    Model cm;
                    c.merge(x);
                    for (ulng y : x.b) { cm.push(y); }
                    checkBasis(c, cm, "merge into empty");
                    checkEqual(c.b, x.b, "merge into empty copies b");
                    name = "query";}
                else { name = "query"; }
                checkBasis(x, md, "after " + name);
                checkQueries(rng, x, md, w);
                checkBasis(xu, md, "unmasked after " + name);
                checkQueries(rng, xu, md, w);}}}

    // Every insert sequence of length <= 3 over [0, 8), every merge and intersection of sequences of length <= 2.
    void exhaustive() {
        Rng rng(seed);
        vector<vector<ulng>> seqs{{}};
        for (size_t i = 0; i < seqs.size(); ++i) {
            if (seqs[i].size() < 3) {
                for (ulng v = 0; v < 8; ++v) { seqs.push_back(seqs[i]); seqs.back().push_back(v); }}}
        auto build = [&](const vector<ulng> &sq, Model &md) {
            XorBasis x;
            for (ulng v : sq) {
                bool want = !md.e.has(v);
                checkEqual(x.insert(v), want, "insert(" + show(v) + ")");
                if (want) { md.push(v); }}
            return x;};
        vector<XorBasis<>> small;
        vector<Model> models;
        for (const vector<ulng> &sq : seqs) {
            context = "exhaustive seq=" + show(sq);
            Model md;
            XorBasis x = build(sq, md);
            checkBasis(x, md, "exhaustive");
            checkQueries(rng, x, md, 3);
            if (sq.size() <= 2) { small.push_back(x); models.push_back(md); }}
        for (size_t i = 0; i < small.size(); ++i) {
            for (size_t j = 0; j < small.size(); ++j) {
                context = "exhaustive pair a=" + show(small[i].b) + " b=" + show(small[j].b);
                XorBasis c = small[i], res = intersection(small[i], small[j]);
                Model cm = models[i];
                for (ulng y : small[j].b) {
                    if (!cm.e.has(y)) { cm.push(y); }}
                c.merge(small[j]);
                checkBasis(c, cm, "merge");
                checkShape(res, "intersection");
                vector<ulng> sa = closure(small[i].b), sb = closure(small[j].b), both;
                std::ranges::set_intersection(sa, sb, std::back_inserter(both));
                checkEqual(closure(res.b), both, "intersection span");}}
        std::cout << "PASS exhaustive " << seqs.size() << " sequences, " << small.size() * small.size() << " pairs\n";}

    // The oracles themselves: Ech and countBelow against brute spans, and the critical-position sweep against direct elimination.
    vector<ulng> genArray(Rng &rng, int n, int profile, int B);
    vector<vector<int>> critical(const vector<ulng> &a);
    void validateOracles(Rng &rng, int rounds) {
        for (int t = 0; t < rounds; ++t) {
            int w = int(rnd(rng, 1, 8)), k = int(rnd(rng, 0, 10));
            context = "validate Ech w=" + show(w) + " round=" + show(t);
            vector<ulng> v(k);
            for (ulng &x : v) { x = gen(rng, w, v, kindOf(rng, 4)); }
            Ech e = echOf(v);
            vector<ulng> s = closure(v);
            checkEqual(s.size(), size_t(bit(e.dim)), "Ech dim");
            checkEqual(closure(e.rref()), s, "Ech rref span");
            for (ulng x = 0; x <= wmask(w) + 1; ++x) {
                ulng lo = ~ulng(0), hi = 0;
                for (ulng y : s) { lo = min(lo, x ^ y); hi = max(hi, x ^ y); }
                checkEqual(e.low(x), lo, "Ech low"); checkEqual(e.high(x), hi, "Ech high");
                checkEqual(e.countBelow(x), ulng(std::ranges::lower_bound(s, x) - s.begin()), "Ech countBelow(" + show(x) + ")");}
            checkEqual(e.countBelow(~ulng(0)), ulng(s.size()), "Ech countBelow(max)");}
        Ech full;
        for (int i = 0; i < 64; ++i) { full.add(bit(i) | (rng() & (bit(i) - 1))); }
        for (int t = 0; t < 64; ++t) {
            ulng x = rng() >> (t % 64);
            checkEqual(full.countBelow(x), x, "full-rank countBelow");}
        for (int t = 0; t < rounds; ++t) {
            int n = int(rnd(rng, 0, 48)), B = std::array{1, 3, 8, 64}[rng() % 4];
            vector<ulng> a = genArray(rng, n, int(rng() % 8), B);
            context = "validate sweep n=" + show(n) + " B=" + show(B) + " a=" + show(a);
            vector<vector<int>> crit = critical(a);
            for (int r = 0; r <= n; ++r) {
                for (int l = 0; l <= r; ++l) {
                    Ech e;
                    for (int i : crit[r]) {
                        if (i >= l) { checkEqual(e.add(a[i]), true, "critical vectors independent"); }}
                    checkEqual(e.rref(), slice(a, l, r).rref(), "sweep span of [" + show(l) + "," + show(r) + ")");}}}
        std::cout << "PASS oracles\n";}

    // Profiles: 0 uniform, 1 sparse, 2 shared top bit, 3 low-dimensional subspace, 4 zeros and duplicates, 5 staircase of leading bits, 6 periodic, 7 mixed.
    vector<ulng> genArray(Rng &rng, int n, int profile, int B) {
        ulng M = wmask(B);
        vector<ulng> gens(size_t(rnd(rng, 1, min(B, 12)))), pool(size_t(rnd(rng, 1, 70))), a(n);
        for (ulng &g : gens) { g = rng() & M; }
        for (ulng &g : pool) { g = rng() & M; }
        for (int i = 0; i < n; ++i) {
            int p = profile == 7 ? int(rng() % 7) : profile;
            if (p == 0) { a[i] = rng() & M; }
            else if (p == 1) { a[i] = bit(int(rng() % ulng(B))) | (rng() % 2 ? bit(int(rng() % ulng(B))) : 0); }
            else if (p == 2) { a[i] = bit(B - 1) | (rng() & (M >> 1)); }
            else if (p == 3) { a[i] = comb(rng, gens); }
            else if (p == 4) { a[i] = rng() % 10 < 7 ? 0 : pool[rng() % 4 % pool.size()]; }
            else if (p == 5) {
                int j = B - 1 - i % B;
                a[i] = bit(j) | (rng() & (bit(j) - 1));}
            else { a[i] = pool[size_t(i) % pool.size()]; }}
        return a;}
    // crit[r]: positions i < r with a[i] outside span(a[i + 1, r)), decreasing; span(a[l, r)) is spanned by those >= l.
    vector<int> advance(const vector<ulng> &a, const vector<int> &crit, int r) {
        vector<int> res;
        Ech e;
        if (e.add(a[r])) { res.push_back(r); }
        for (int i : crit) {
            if (e.dim == 64) { break; }
            if (e.add(a[i])) { res.push_back(i); }}
        return res;}
    vector<vector<int>> critical(const vector<ulng> &a) {
        vector<vector<int>> res(a.size() + 1);
        for (int r = 0; r < int(a.size()); ++r) { res[r + 1] = advance(a, res[r], r); }
        return res;}
    // Calls g(j, crit) for every query j with crit the critical positions of its r.
    template<typename G> void sweep(const vector<ulng> &a, const vector<pair<int, int>> &qs, G g) {
        int n = int(a.size());
        vector<vector<int>> at(n + 1);
        for (int j = 0; j < int(qs.size()); ++j) { at[qs[j].second].push_back(j); }
        vector<int> crit;
        for (int r = 0; r <= n; ++r) {
            if (r) { crit = advance(a, crit, r - 1); }
            for (int j : at[r]) { g(j, crit); }}}

    // Kinds: random, short, empty, prefix/suffix/full, duplicate, length near 64.
    vector<pair<int, int>> genQueries(Rng &rng, int n, int q) {
        vector<pair<int, int>> qs;
        for (int j = 0; j < q; ++j) {
            int k = int(rng() % 6), l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
            if (l > r) { swap(l, r); }
            if (k == 1) { r = min(n, l + int(rnd(rng, 0, 70))); }
            else if (k == 2) { r = l; }
            else if (k == 3) { int c = int(rng() % 3); l = c == 1 ? l : 0; r = c == 0 ? r : n; }
            else if (k == 4 && !qs.empty()) { std::tie(l, r) = qs[rng() % qs.size()]; }
            else if (k == 5) { r = min(n, l + int(rnd(rng, 63, 65))); }
            qs.emplace_back(l, r);}
        return qs;}
    // Runs rangeXorBasisQuery, checks each query is answered once, in increasing (r, index) order, with an echelon basis, and returns the bases.
    vector<vector<ulng>> runRange(const vector<ulng> &a, const vector<pair<int, int>> &qs) {
        int q = int(qs.size()), last = -1;
        vector<int> calls(q);
        vector<vector<ulng>> res(q);
        XorBasisCompanion::rangeXorBasisQuery(a, qs, [&](int j, const vector<ulng> &e) {
            expect(0 <= j && j < q, true, [&] { return "callback index " + show(j); });
            expect(last < 0 || pair(qs[last].second, last) < pair(qs[j].second, j), true, [&] { return "callback order " + show(last) + " then " + show(j); });
            last = j;
            ++calls[j];
            res[j] = e;});
        for (int j = 0; j < q; ++j) {
            expect(calls[j], 1, [&] { return "query " + show(j) + "=" + show(qs[j]) + " callback count"; });
            const vector<ulng> &e = res[j];
            for (size_t i = 0; i < e.size(); ++i) {
                expect(e[i] != 0 && (!i || lead(e[i - 1]) > lead(e[i])), true, [&] { return "query " + show(qs[j]) + " echelon e=" + show(e); });}}
        return res;}

    void rangeSmall(Rng &rng, int n, int profile, int B) {
        vector<ulng> a = genArray(rng, n, profile, B);
        context = "rangeXorBasisQuery small n=" + show(n) + " profile=" + show(profile) + " B=" + show(B) + " a=" + show(a) + " seed=" + show(seed);
        vector<pair<int, int>> qs;
        for (int r = 0; r <= n; ++r) {
            for (int l = 0; l <= r; ++l) { qs.emplace_back(l, r); }}
        for (int t = int(qs.size()) / 4; t > 0; --t) { qs.push_back(qs[rng() % qs.size()]); }
        std::shuffle(qs.begin(), qs.end(), rng);
        vector<vector<ulng>> res = runRange(a, qs);
        for (size_t j = 0; j < qs.size(); ++j) { expect(echOf(res[j]).rref(), slice(a, qs[j].first, qs[j].second).rref(), [&] { return "span of " + show(qs[j]) + " e=" + show(res[j]); }); }}
    void rangeLarge(Rng &rng, int n, int q, int profile) {
        vector<ulng> a = genArray(rng, n, profile, 64);
        vector<pair<int, int>> qs = genQueries(rng, n, q);
        context = "rangeXorBasisQuery large n=" + show(n) + " q=" + show(q) + " profile=" + show(profile) + " seed=" + show(seed);
        vector<vector<ulng>> res = runRange(a, qs);
        // Equal dimension and every critical vector reducing to 0 through e's pivots give equal spans.
        sweep(a, qs, [&](int j, const vector<int> &crit) {
            array<ulng, 64> piv{};
            for (ulng y : res[j]) { piv[lead(y)] = y; }
            int d = 0;
            for (int i : crit) {
                if (i < qs[j].first) { break; }
                ++d;
                ulng x = a[i];
                while (x && piv[lead(x)]) { x ^= piv[lead(x)]; }
                expect(x, ulng(0), [&] { return "a[" + show(i) + "] in span of query " + show(qs[j]); });}
            expect(int(res[j].size()), d, [&] { return "dimension of query " + show(qs[j]); });});}

    // Prefix bases: every answer of a PrefixXorBasis<B> against an Ech of the same slice.
    template<int B> void checkPrefixQuery(Rng &rng, const PrefixXorBasis<B> &p, const vector<ulng> &a, int l, int r, const Ech &e) {
        ulng M = wmask(B);
        expect(p.rangeMax(l, r), e.high(0), [&] { return "rangeMax(" + show(l) + "," + show(r) + ")"; });
        for (ulng x : {ulng(rng() & M), ulng(rng()), M}) { expect(p.rangeMax(l, r, x), e.high(x), [&] { return "rangeMax(" + show(l) + "," + show(r) + "," + show(x) + ")"; }); }
        ulng in = 0;
        for (int t = 0; t < 3 && l < r; ++t) { in ^= a[size_t(rnd(rng, l, r - 1))]; }
        vector<ulng> xs{0, in, in ^ 1, ulng(rng() & M), ulng(rng())};
        if (B < 64) { xs.push_back(in | bit(int(rnd(rng, B, 63)))); }
        for (ulng x : xs) { expect(p.rangeContains(l, r, x), e.has(x), [&] { return "rangeContains(" + show(l) + "," + show(r) + "," + show(x) + ")"; }); }}
    template<int B> void prefixSmall(Rng &rng, int n, int profile) {
        vector<ulng> a = genArray(rng, n, profile, B);
        context = "PrefixXorBasis<" + show(B) + "> small n=" + show(n) + " profile=" + show(profile) + " a=" + show(a) + " seed=" + show(seed);
        PrefixXorBasis<B> built(a), pushed;
        for (int i = 0; i < n; ++i) {
            pushed.push(a[i]);
            checkEqual(pushed.n(), i + 1, "n() after push");
            for (int t = 0; t < 4; ++t) {
                int r = int(rnd(rng, 0, i + 1)), l = int(rnd(rng, 0, r));
                checkPrefixQuery(rng, pushed, a, l, r, slice(a, l, r));}}
        checkEqual(built.n(), n, "n() after build");
        for (int r = 0; r <= n; ++r) {
            for (int l = 0; l <= r; ++l) {
                Ech e = slice(a, l, r);
                checkPrefixQuery(rng, built, a, l, r, e); checkPrefixQuery(rng, pushed, a, l, r, e);}}}
    template<int B> void prefixLarge(Rng &rng, int n, int q, int profile, bool usePush) {
        vector<ulng> a = genArray(rng, n, profile, B);
        context = "PrefixXorBasis<" + show(B) + "> large n=" + show(n) + " q=" + show(q) + " profile=" + show(profile) + " push=" + show(usePush) + " seed=" + show(seed);
        PrefixXorBasis<B> p(usePush ? vector<ulng>{} : a);
        vector<pair<int, int>> qs;
        vector<array<ulng, 4>> xs;
        vector<array<ulng, 3>> got;
        ulng M = wmask(B);
        auto ask = [&](int cur) {
            pair<int, int> lr = genQueries(rng, cur, 1)[0];
            auto [l, r] = lr;
            ulng in = 0;
            for (int t = 0; t < 3 && l < r; ++t) { in ^= a[size_t(rnd(rng, l, r - 1))]; }
            array<ulng, 4> x{rng() % 2 ? ulng(rng()) : ulng(rng() & M), in, rng() % 2 ? ulng(rng() & M) : in ^ 1, B < 64 ? in | bit(int(rnd(rng, B, 63))) : ulng(rng())};
            qs.push_back(lr); xs.push_back(x);
            got.push_back({p.rangeMax(l, r, x[0]), p.rangeMax(l, r), ulng(p.rangeContains(l, r, x[1])) | ulng(p.rangeContains(l, r, x[2])) << 1 | ulng(p.rangeContains(l, r, x[3])) << 2});};
        for (int i = 0; i < n && usePush; ++i) {
            p.push(a[i]);
            if (p.n() != i + 1) { checkEqual(p.n(), i + 1, "n() after push"); }
            for (lng t = rnd(rng, 0, 2 * q / max(n, 1)); t > 0 && int(qs.size()) < q; --t) { ask(i + 1); }}
        checkEqual(p.n(), n, "n()");
        while (int(qs.size()) < q) { ask(n); }
        sweep(a, qs, [&](int j, const vector<int> &crit) {
            Ech e;
            for (int i : crit) {
                if (i < qs[j].first) { break; }
                e.add(a[i]);}
            const array<ulng, 4> &x = xs[j];
            ulng want = ulng(e.has(x[1])) | ulng(e.has(x[2])) << 1 | ulng(e.has(x[3])) << 2;
            expect(got[j][0], e.high(x[0]), [&] { return "rangeMax" + show(qs[j]) + " x=" + show(x[0]); });
            expect(got[j][1], e.high(0), [&] { return "rangeMax" + show(qs[j]); });
            expect(got[j][2], want, [&] { return "rangeContains" + show(qs[j]) + " bits for x=" + show(x[1]) + "," + show(x[2]) + "," + show(x[3]); });});}
    template<int B> void prefixSuite(Rng &rng, int smallRounds, int n, int q) {
        context = "PrefixXorBasis<" + show(B) + "> edges";
        PrefixXorBasis<B> z;
        checkEqual(z.n(), 0, "default n()"); checkEqual(z.rangeMax(0, 0), ulng(0), "empty rangeMax"); checkEqual(z.rangeMax(0, 0, 5), ulng(5), "empty rangeMax(x)");
        checkEqual(z.rangeContains(0, 0, 0), true, "empty contains 0"); checkEqual(z.rangeContains(0, 0, 1), false, "empty contains 1");
        PrefixXorBasis<B> one(vector<ulng>{wmask(B)});
        checkEqual(one.rangeMax(0, 1), wmask(B), "single rangeMax"); checkEqual(one.rangeMax(1, 1, 3), ulng(3), "single empty range");
        checkEqual(one.rangeContains(0, 1, wmask(B)), true, "single contains"); checkEqual(one.rangeContains(1, 1, wmask(B)), false, "single empty range contains");
        if (B < 64) { checkEqual(one.rangeContains(0, 1, bit(B)), false, "contains 2^B"); checkEqual(one.rangeContains(0, 1, ~ulng(0)), false, "contains ~0"); }
        for (int t = 0; t < smallRounds; ++t) { prefixSmall<B>(rng, int(rnd(rng, 0, 40)), t % 8); }
        for (int profile = 0; profile < 8; ++profile) { prefixLarge<B>(rng, max(1, n / 8), max(1, q / 8), profile, profile % 2); }
        prefixLarge<B>(rng, n, q, 0, false); prefixLarge<B>(rng, n, q, 7, true);
        std::cout << "PASS PrefixXorBasis<" << B << "> n=" << n << " q=" << q << '\n';}

    void intersectionRandom(Rng &rng, int w, int rounds) {
        for (int t = 0; t < rounds; ++t) {
            int c = int(rnd(rng, 0, min(w, 20))), ka = int(rnd(rng, 0, w + 2)), kb = int(rnd(rng, 0, w + 2));
            context = "intersection w=" + show(w) + " round=" + show(t) + " c=" + show(c) + " seed=" + show(seed);
            vector<ulng> common(c), va, vb;
            for (ulng &x : common) { x = gen(rng, w, common, kindOf(rng, t % 5)); }
            va = common;
            for (int i = 0; i < ka; ++i) { va.push_back(gen(rng, w, va, kindOf(rng, t % 5))); }
            for (int i = 0; i < c; ++i) { vb.push_back(comb(rng, common)); }
            for (int i = 0; i < kb; ++i) { vb.push_back(gen(rng, w, vb, kindOf(rng, (t + 1) % 5))); }
            if (t % 7 == 0) { for (int i = 0; i < w; ++i) { va.push_back(bit(i)); } }
            std::shuffle(va.begin(), va.end(), rng); std::shuffle(vb.begin(), vb.end(), rng);
            XorBasis a, b;
            for (ulng x : va) { a.insert(x); }
            for (ulng x : vb) { b.insert(x); }
            Ech ea = echOf(va), eb = echOf(vb), eu = echOf(va);
            for (ulng x : vb) { eu.add(x); }
            XorBasis res = intersection(a, b);
            checkShape(res, "intersection");
            XorBasis<false> ua, ub;
            for (ulng x : va) { ua.insert(x); }
            for (ulng x : vb) { ub.insert(x); }
            checkEqual(intersection(ua, ub).b, res.b, "unmasked intersection");
            checkEqual(res.b, echOf(res.b).rref(), "intersection b reduced echelon");
            for (ulng y : res.b) { expect(ea.has(y) && eb.has(y), true, [&] { return "intersection vector " + show(y) + " in both"; }); }
            checkEqual(res.size(), ea.dim + eb.dim - eu.dim, "intersection dimension");
            checkEqual(intersection(b, a).b, res.b, "intersection symmetric");
            checkEqual(intersection(a, a).b, a.b, "intersection with itself");
            checkEqual(intersection(a, XorBasis{}).size(), 0, "intersection with empty");
            if (w <= 8) {
                vector<ulng> sa = closure(va), sb = closure(vb), both;
                std::ranges::set_intersection(sa, sb, std::back_inserter(both));
                checkEqual(closure(res.b), both, "brute intersection span");}}
        std::cout << "PASS intersection w=" << w << " rounds=" << rounds << '\n';}

    void edges() {
        context = "edges";
        XorBasis z;
        Model zm;
        checkBasis(z, zm, "empty");
        checkEqual(z.insert(0), false, "insert(0) into empty");
        checkEqual(z.kth(0), ulng(0), "empty kth(0)"); checkEqual(z.rank(0), ulng(0), "empty rank(0)"); checkEqual(z.rank(5), ulng(1), "empty rank(5)");
        checkEqual(z.rank(~ulng(0)), ulng(1), "empty rank(max)"); checkEqual(z.max(), ulng(0), "empty max()"); checkEqual(z.reduce(9), ulng(9), "empty reduce");
        ulng mask = 7;
        checkEqual(z.represent(0, mask), true, "empty represent(0)"); checkEqual(mask, ulng(0), "empty represent(0) mask");
        checkEqual(z.represent(1, mask), false, "empty represent(1)");
        // Full rank: powers of two in random order, then extra inserts all fail.
        Rng rng(seed ^ 1);
        vector<ulng> pw;
        for (int i = 0; i < 64; ++i) { pw.push_back(bit(i)); }
        std::shuffle(pw.begin(), pw.end(), rng);
        for (int pass = 0; pass < 2; ++pass) {
            XorBasis x;
            Model md;
            for (int i = 0; i < 64; ++i) {
                ulng v = pass ? pw[i] ^ (rng() & (bit(lead(pw[i])) - 1)) : pw[i];
                if (pass && i % 2) { v |= bit(63); }
                bool want = !md.e.has(v);
                checkEqual(x.insert(v), want, "full-rank insert(" + show(v) + ")");
                if (want) { md.push(v); }}
            while (md.e.dim < 64) {
                ulng v = rng();
                if (x.insert(v)) { md.push(v); }}
            checkBasis(x, md, "full rank");
            checkEqual(x.size(), 64, "full rank size");
            for (int i = 0; i < 64; ++i) { checkEqual(x.b[i], bit(63 - i), "full rank b is the identity"); }
            for (ulng v : {ulng(0), ulng(1), ~ulng(0), ulng(rng()), bit(63)}) {
                checkEqual(x.insert(v), false, "insert into full rank");
                checkEqual(x.kth(v), v, "full rank kth"); checkEqual(x.rank(v), v, "full rank rank");
                checkEqual(x.max(v), ~ulng(0), "full rank max"); checkEqual(x.reduce(v), ulng(0), "full rank reduce");
                checkValue(x, md, v);}
            XorBasis before = x;
            x.merge(x);
            checkEqual(x.b == before.b && x.masks == before.masks, true, "full rank self merge");
            checkQueries(rng, x, md, 64);}
        // Top bit only, then a vector with bit 63 and a lower bit: the second reduces the first.
        XorBasis t;
        Model tm;
        for (ulng v : {bit(63) | 1, bit(63), ulng(1), bit(63) | 3}) {
            bool want = !tm.e.has(v);
            checkEqual(t.insert(v), want, "top insert(" + show(v) + ")");
            if (want) { tm.push(v); }
            checkBasis(t, tm, "top bit");}
        checkEqual(t.b, vector<ulng>{bit(63), 2, 1}, "top bit b");
        // Timestamp swap: x and y share the top bit, so [1, 2) needs y itself as the top pivot.
        ulng x0 = bit(63) | 1, y0 = bit(63) | 2;
        vector<ulng> a{x0, y0};
        vector<vector<ulng>> res = runRange(a, {{1, 2}, {0, 2}, {0, 1}, {2, 2}});
        checkEqual(echOf(res[0]).rref(), vector<ulng>{y0}, "swap rule [1,2)");
        checkEqual(echOf(res[1]).rref(), vector<ulng>{bit(63) | 1, 3}, "swap rule [0,2)");
        checkEqual(echOf(res[2]).rref(), vector<ulng>{x0}, "swap rule [0,1)");
        checkEqual(res[3].empty(), true, "empty range");
        PrefixXorBasis<64> p(a);
        PrefixXorBasis<> d(a);
        checkEqual(p.rangeContains(1, 2, y0), true, "prefix swap contains y"); checkEqual(p.rangeContains(1, 2, 3), false, "prefix swap excludes x^y");
        checkEqual(p.rangeMax(1, 2), y0, "prefix swap max"); checkEqual(d.rangeMax(0, 2), y0, "PrefixXorBasis<> max");
        // n = 0, no queries, many duplicates of one query.
        checkEqual(runRange({}, {{0, 0}, {0, 0}}).size(), size_t(2), "n = 0 queries");
        checkEqual(runRange(a, {}).size(), size_t(0), "no queries");
        vector<pair<int, int>> dup(100, {0, 2});
        for (const vector<ulng> &e : runRange(a, dup)) { checkEqual(echOf(e).rref(), vector<ulng>{bit(63) | 1, 3}, "duplicate queries"); }
        std::cout << "PASS edges\n";}

    void invalid(const string &name) {
        XorBasis x;
        vector<ulng> a{1, 2, 3};
        PrefixXorBasis<64> p(a);
        auto query = [&](const vector<ulng> &v, int l, int r) { XorBasisCompanion::rangeXorBasisQuery(v, {{0, 0}, {l, r}}, [](int, const vector<ulng> &) {}); };
        if (name == "kth-empty") { (void)x.kth(1); }
        else if (name == "kth-out-of-range") { x.insert(1); x.insert(2); x.insert(4); (void)x.kth(8); }
        else if (name == "kth-size-63") {
            for (int i = 0; i < 63; ++i) { x.insert(bit(i)); }
            (void)x.kth(bit(63));}
        else if (name == "range-query-l-gt-r") { query(a, 2, 1); }
        else if (name == "range-query-r-gt-n") { query(a, 0, 4); }
        else if (name == "range-query-l-negative") { query(a, -1, 2); }
        else if (name == "range-query-empty-array") { query({}, 0, 1); }
        else if (name == "prefix-max-l-gt-r") { (void)p.rangeMax(2, 1); }
        else if (name == "prefix-max-r-gt-n") { (void)p.rangeMax(0, 4, 1); }
        else if (name == "prefix-max-l-negative") { (void)p.rangeMax(-1, 2); }
        else if (name == "prefix-contains-l-gt-r") { (void)p.rangeContains(2, 1, 0); }
        else if (name == "prefix-contains-r-gt-n") { (void)p.rangeContains(0, 4, 1); }
        else if (name == "prefix-contains-l-negative") { (void)p.rangeContains(-1, 2, 1); }
        else if (name == "prefix-push-wide-b1") { PrefixXorBasis<1> q; q.push(2); }
        else if (name == "prefix-push-wide-b3") { PrefixXorBasis<3> q; q.push(1); q.push(8); }
        else if (name == "prefix-push-wide-b63") { PrefixXorBasis<63> q; q.push(bit(63)); }
        else if (name == "prefix-build-wide") { PrefixXorBasis<20> q(vector<ulng>{1, bit(20)}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else if (arg == "--invalid" && i + 1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "xorbasis seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        bool quick = mode == "quick", stress = mode == "stress";
        int scale = quick ? 1 : SCALE, steps = (quick ? 150 : stress ? 12000 : 2500) / scale, rounds = (quick ? 60 : stress ? 3000 : 600) / scale;
        int big = (quick ? 3000 : stress ? 500000 : 200000) / scale, big64 = (quick ? 3000 : 200000) / scale;
        validateOracles(rng, max(rounds, 20));
        edges();
        exhaustive();
        for (int w : {1, 2, 3, 5, 8, 16, 40, 63, 64}) {
            for (int profile = 0; profile < 5; ++profile) { history(rng, w, profile, w <= 8 ? steps / 2 : steps); }}
        std::cout << "PASS XorBasis histories steps=" << steps << '\n';
        for (int w : {1, 3, 8, 16, 40, 64}) { intersectionRandom(rng, w, rounds); }
        for (int t = 0; t < rounds; ++t) { rangeSmall(rng, int(rnd(rng, 0, 40)), t % 8, std::array{1, 3, 8, 64}[t / 8 % 4]); }
        for (int profile = 0; profile < 8; ++profile) { rangeLarge(rng, max(1, big / 10), max(1, big / 10), profile); }
        for (int profile : {0, 2, 5, 7}) { rangeLarge(rng, big, big, profile); }
        std::cout << "PASS rangeXorBasisQuery n=q=" << big << '\n';
        prefixSuite<1>(rng, rounds / 4, big, big); prefixSuite<3>(rng, rounds / 4, big, big); prefixSuite<20>(rng, rounds / 4, big, big);
        prefixSuite<63>(rng, rounds / 4, big64, big64); prefixSuite<64>(rng, rounds / 4, big64, big64);
        std::cout << "PASS xorbasis checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL xorbasis seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
