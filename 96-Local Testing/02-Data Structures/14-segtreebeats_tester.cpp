#include "../../02-Data Structures/14-segtreebeats.hpp"
#include "../../02-Data Structures/00-monoids.hpp"

namespace {
    ulng seed = 20261010;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Independent oracles: floor by repeated subtraction of the remainder, integer sqrt by exact search.
    lng floorDiv(lng x, lng q) { lng r = ((x % q) + q) % q; return (x - r) / q; }
    lng isqrt(lng x) {
        lng lo = 0, hi = 3037000500;
        while (hi - lo > 1) { lng m = (lo + hi) / 2; (lll(m) * m <= x ? lo : hi) = m; }
        return lo;}
    pair<int, int> range(Rng &rng, int n) {
        int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
        if (l > r) { swap(l, r); }
        return {l, r};}

    // kinds: 0 add, 1 chmin, 2 chmax, 3 set, 4 divide, 5 sqrt, 6 mod
    void beatsRun(Rng &rng, int n, int steps, const vector<int> &kinds, lng lo, lng span, const string &name) {
        vector<lng> a(n);
        for (auto &x : a) { x = rnd(rng, lo, span); }
        SegTreeBeats t(a);
        for (int s = 0; s < steps; ++s) {
            auto [l, r] = range(rng, n);
            int kind = kinds[rng() % kinds.size()];
            lng x = rnd(rng, lo, span);
            context = name + " n=" + show(n) + " step=" + show(s) + " kind=" + show(kind) + " l=" + show(l) + " r=" + show(r) + " x=" + show(x);
            lng mn = std::numeric_limits<lng>::max();
            for (int i = l; i < r; ++i) { mn = min(mn, a[i]); }
            if ((kind == 5 || kind == 6) && mn < 0) { kind = 0; x = rnd(rng, 0, span); }
            if (kind == 0) { t.addUpdate(l, r, x); for (int i = l; i < r; ++i) { a[i] += x; }}
            else if (kind == 1) { t.chminUpdate(l, r, x); for (int i = l; i < r; ++i) { a[i] = min(a[i], x); }}
            else if (kind == 2) { t.chmaxUpdate(l, r, x); for (int i = l; i < r; ++i) { a[i] = max(a[i], x); }}
            else if (kind == 3) { t.setUpdate(l, r, x); for (int i = l; i < r; ++i) { a[i] = x; }}
            else if (kind == 4) { lng q = rnd(rng, 1, 5); t.divideUpdate(l, r, q); for (int i = l; i < r; ++i) { a[i] = floorDiv(a[i], q); }}
            else if (kind == 5) { t.sqrtUpdate(l, r); for (int i = l; i < r; ++i) { a[i] = isqrt(a[i]); }}
            else { lng m = rnd(rng, 1, max<lng>(span / 2, 2)); t.modUpdate(l, r, m); for (int i = l; i < r; ++i) { a[i] %= m; }}
            for (int qn = 0; qn < 3; ++qn) {
                auto [ql, qr] = range(rng, n);
                lng sum = 0, hi = SegTreeBeats::LOW, lo = SegTreeBeats::HIGH;
                for (int i = ql; i < qr; ++i) { sum += a[i]; hi = max(hi, a[i]); lo = min(lo, a[i]); }
                context += " query=" + show(ql) + "," + show(qr);
                checkEqual(t.sumQuery(ql, qr), sum, "sumQuery"); checkEqual(t.maxQuery(ql, qr), hi, "maxQuery"); checkEqual(t.minQuery(ql, qr), lo, "minQuery");}
            if (s % 64 == 0) {
                for (int i = 0; i < n; ++i) { checkEqual(t.sumQuery(i, i + 1), a[i], "point " + show(i)); }}}}

    void historicRun(Rng &rng, int n, int steps, lng span) {
        vector<lng> a(n), hx, hn, hs(n);
        for (auto &x : a) { x = rnd(rng, -span, span); }
        hx = hn = a;
        HistoricSegTree t(a);
        for (int s = 0; s < steps; ++s) {
            context = "historic n=" + show(n) + " step=" + show(s);
            if (rng() % 3 == 0) { t.tick(); for (int i = 0; i < n; ++i) { hs[i] += a[i]; }}
            else {
                auto [l, r] = range(rng, n);
                lng x = rnd(rng, -span, span);
                t.addUpdate(l, r, x);
                for (int i = l; i < r; ++i) { a[i] += x; hx[i] = max(hx[i], a[i]); hn[i] = min(hn[i], a[i]); }}
            auto [ql, qr] = range(rng, n);
            lng sum = 0, mx = SegTreeBeats::LOW, mn = SegTreeBeats::HIGH, hmx = mx, hmn = mn, hsum = 0;
            for (int i = ql; i < qr; ++i) { sum += a[i]; mx = max(mx, a[i]); mn = min(mn, a[i]); hmx = max(hmx, hx[i]); hmn = min(hmn, hn[i]); hsum += hs[i]; }
            context += " query=" + show(ql) + "," + show(qr);
            checkEqual(t.sumQuery(ql, qr), sum, "sumQuery"); checkEqual(t.maxQuery(ql, qr), mx, "maxQuery"); checkEqual(t.minQuery(ql, qr), mn, "minQuery");
            checkEqual(t.historicMaxQuery(ql, qr), hmx, "historicMaxQuery"); checkEqual(t.historicMinQuery(ql, qr), hmn, "historicMinQuery");
            checkEqual(t.historicSumQuery(ql, qr), hsum, "historicSumQuery");}}

    // Tester-local BeatsMonoid: range chmin with sum and max, failing when the bound reaches the second maximum.
    struct ChminSum {
        static constexpr lng NEG = std::numeric_limits<lng>::min();
        struct S { lng sum, mx, mx2; int cnt, len; bool bad; };
        using F = lng;
        static S op(const S &x, const S &y) {
            if (!x.len) { return y; }
            if (!y.len) { return x; }
            if (x.mx == y.mx) { return {x.sum + y.sum, x.mx, max(x.mx2, y.mx2), x.cnt + y.cnt, x.len + y.len, false}; }
            const S &p = x.mx > y.mx ? x : y, &q = x.mx > y.mx ? y : x;
            return {x.sum + y.sum, p.mx, max(p.mx2, q.mx), p.cnt, x.len + y.len, false};}
        static S e() { return {0, NEG, NEG, 0, 0, false}; }
        static S mapping(const F &c, const S &x) {
            if (!x.len || c >= x.mx) { return x; }
            if (c <= x.mx2) { S res = x; res.bad = true; return res; }
            return {x.sum - (x.mx - c) * x.cnt, c, x.mx2, x.cnt, x.len, false};}
        static F composition(const F &f, const F &g) { return min(f, g); }
        static F id() { return std::numeric_limits<lng>::max(); }
        static bool fail(const S &x) { return x.bad; }
        static S leaf(lng v) { return {v, v, NEG, 1, 1, false}; }
    };
    struct AffineNoFail : RangeAffineRangeSum<lng> { static bool fail(const S &) { return false; } };

    template<typename T>
    void andOrRun(Rng &rng, int n, int steps, int bits) {
        using A = RangeAndOrRangeSumMax<T>;
        T full = bits >= int(8 * sizeof(T)) ? T(~T(0)) : T((T(1) << bits) - 1);
        auto value = [&] { return T(rng() & ulng(full)); };
        vector<T> a(n);
        vector<typename A::S> init;
        for (auto &x : a) { x = value(); init.push_back(A::leaf(x)); }
        LazySegmentTreeBeats<A> t(init);
        for (int s = 0; s < steps; ++s) {
            auto [l, r] = range(rng, n);
            int kind = int(rng() % 4);
            T x = value();
            if (rng() % 2) { x = T(x & value() & value()); }
            context = "andOr bits=" + show(bits) + " n=" + show(n) + " step=" + show(s) + " kind=" + show(kind) + " l=" + show(l) + " r=" + show(r);
            if (kind == 3 && n) { int p = int(rnd(rng, 0, n - 1)); t.set(p, A::leaf(x)); a[p] = x; }
            else {
                typename A::F f = kind == 0 ? A::andWith(T(x | ~full)) : kind == 1 ? A::orWith(x) : A::assign(x);
                if (kind == 0) { x = T(x | ~full); }
                t.apply(l, r, f);
                for (int i = l; i < r; ++i) { a[i] = kind == 0 ? T(a[i] & x) : kind == 1 ? T(a[i] | x) : x; }}
            auto [ql, qr] = range(rng, n);
            T sum = 0, mx = 0;
            for (int i = ql; i < qr; ++i) { sum = T(sum + a[i]); mx = max(mx, a[i]); }
            auto got = t.prod(ql, qr);
            checkEqual(got.sum, sum, "prod.sum"); checkEqual(got.len, lng(qr - ql), "prod.len");
            if (ql < qr) { checkEqual(got.mx, mx, "prod.mx"); }
            if (n) { int p = int(rnd(rng, 0, n - 1)); checkEqual(t.get(p).sum, a[p], "get"); }
            if (s % 32 == 0) { auto all = t.allProd(); T tot = 0; for (T v : a) { tot = T(tot + v); } checkEqual(all.sum, tot, "allProd"); }}}

    void chminGenericRun(Rng &rng, int n, int steps) {
        vector<lng> a(n);
        vector<ChminSum::S> init;
        for (auto &x : a) { x = rnd(rng, -20, 20); init.push_back(ChminSum::leaf(x)); }
        LazySegmentTreeBeats<ChminSum> t(init);
        for (int s = 0; s < steps; ++s) {
            auto [l, r] = range(rng, n);
            lng x = rnd(rng, -20, 20);
            context = "chminGeneric n=" + show(n) + " step=" + show(s) + " l=" + show(l) + " r=" + show(r) + " x=" + show(x);
            if (rng() % 5 == 0 && n) { int p = int(rnd(rng, 0, n - 1)); t.set(p, ChminSum::leaf(x)); a[p] = x; }
            else { t.apply(l, r, x); for (int i = l; i < r; ++i) { a[i] = min(a[i], x); }}
            auto [ql, qr] = range(rng, n);
            lng sum = 0, mx = ChminSum::NEG;
            for (int i = ql; i < qr; ++i) { sum += a[i]; mx = max(mx, a[i]); }
            auto got = t.prod(ql, qr);
            checkEqual(got.sum, sum, "prod.sum"); checkEqual(got.mx, mx, "prod.mx");}}

    void affineGenericRun(Rng &rng, int n, int steps) {
        vector<lng> a(n);
        vector<AffineNoFail::S> init;
        for (auto &x : a) { x = rnd(rng, -9, 9); init.push_back(AffineNoFail::leaf(x)); }
        LazySegmentTreeBeats<AffineNoFail> t(init);
        for (int s = 0; s < steps; ++s) {
            auto [l, r] = range(rng, n);
            lng b = rnd(rng, -5, 5), c = rnd(rng, -1, 1);
            context = "affineGeneric n=" + show(n) + " step=" + show(s);
            t.apply(l, r, {c, b});
            for (int i = l; i < r; ++i) { a[i] = c * a[i] + b; }
            auto [ql, qr] = range(rng, n);
            lng sum = 0;
            for (int i = ql; i < qr; ++i) { sum += a[i]; }
            checkEqual(t.prod(ql, qr).sum, sum, "prod.sum");}}

    void edges() {
        context = "edges";
        SegTreeBeats z(0), sized(5);
        checkEqual(z.sumQuery(0, 0), 0, "empty sum"); checkEqual(z.maxQuery(0, 0), SegTreeBeats::LOW, "empty max"); checkEqual(z.minQuery(0, 0), SegTreeBeats::HIGH, "empty min");
        z.addUpdate(0, 0, 5); z.chminUpdate(0, 0, 1); z.divideUpdate(0, 0, 3); z.sqrtUpdate(0, 0); z.modUpdate(0, 0, 2);
        checkEqual(sized.sumQuery(0, 5), 0, "sized zeros"); sized.addUpdate(1, 4, 7); checkEqual(sized.maxQuery(0, 5), 7, "sized add");
        checkEqual(sized.minQuery(2, 2), SegTreeBeats::HIGH, "empty interior min");
        // Two distinct values: chmin must move the second minimum too, or the next chmax is tagged wrongly.
        SegTreeBeats two(vector<lng>{1, 5});
        two.chminUpdate(0, 2, 3); two.chmaxUpdate(0, 2, 4);
        checkEqual(two.sumQuery(0, 2), 8, "two-valued chmin then chmax");
        two = SegTreeBeats(vector<lng>{1, 5});
        two.chmaxUpdate(0, 2, 3); two.chminUpdate(0, 2, 2);
        checkEqual(two.sumQuery(0, 2), 4, "two-valued chmax then chmin");
        // Extreme magnitudes: n * |x| < 2^62.
        lng big = (lng(1) << 59) - 1;
        SegTreeBeats w(vector<lng>{big, -big, big, -big, 0, 1, 2, 3});
        w.chminUpdate(0, 8, big - 5); w.chmaxUpdate(0, 8, 5 - big);
        checkEqual(w.sumQuery(0, 4), lng(0), "big sum"); checkEqual(w.maxQuery(0, 8), big - 5, "big max"); checkEqual(w.minQuery(0, 8), 5 - big, "big min");
        lng before = w.sumQuery(0, 8);
        w.divideUpdate(0, 8, 1);
        checkEqual(w.sumQuery(0, 8), before, "divide by one");
        w.divideUpdate(0, 8, 7);
        checkEqual(w.minQuery(0, 2), floorDiv(5 - big, 7), "big floor");
        w.setUpdate(0, 8, big); w.sqrtUpdate(0, 8);
        checkEqual(w.maxQuery(0, 8), isqrt(big), "big sqrt"); checkEqual(w.sumQuery(0, 8), 8 * isqrt(big), "big sqrt sum");
        w.setUpdate(0, 8, big); w.modUpdate(3, 5, 1000000007);
        checkEqual(w.sumQuery(3, 5), 2 * (big % 1000000007), "big mod");
        SegTreeBeats copy = w;
        copy.addUpdate(0, 8, 1);
        checkEqual(w.sumQuery(0, 1), big, "copy independent");
        HistoricSegTree h(3);
        h.addUpdate(0, 3, 5); h.addUpdate(0, 3, -9); h.tick(); h.tick();
        checkEqual(h.historicMaxQuery(0, 3), lng(5), "historic peak"); checkEqual(h.historicMinQuery(0, 3), lng(-4), "historic low");
        checkEqual(h.historicSumQuery(0, 3), lng(-24), "historic sum"); checkEqual(h.historicSumQuery(1, 1), lng(0), "historic empty");
        LazySegmentTreeBeats<RangeAndOrRangeSumMax<>> g(0);
        checkEqual(g.prod(0, 0).len, lng(0), "generic empty"); checkEqual(g.allProd().len, lng(0), "generic empty all");
        LazySegmentTreeBeats<RangeAndOrRangeSumMax<lng>> s(vector<RangeAndOrRangeSumMax<lng>::S>{RangeAndOrRangeSumMax<lng>::leaf(6), RangeAndOrRangeSumMax<lng>::leaf(3)});
        s.apply(0, 2, RangeAndOrRangeSumMax<lng>::orWith(8)); s.apply(0, 2, RangeAndOrRangeSumMax<lng>::andWith(10));
        checkEqual(s.prod(0, 2).sum, lng(10 + 10), "signed preset sum"); checkEqual(s.prod(0, 2).mx, lng(10), "signed preset max");}

    void invalid(const string &name) {
        SegTreeBeats t(vector<lng>{1, -2, 3});
        HistoricSegTree h(3);
        LazySegmentTreeBeats<RangeAndOrRangeSumMax<>> g(3);
        if (name == "negative-size") { SegTreeBeats x(-1); }
        else if (name == "oversize") { SegTreeBeats x(SegTreeBeats::MAX_SIZE + 1); }
        else if (name == "add-reversed") { t.addUpdate(2, 1, 1); }
        else if (name == "chmin-end") { t.chminUpdate(0, 4, 1); }
        else if (name == "chmax-negative") { t.chmaxUpdate(-1, 2, 1); }
        else if (name == "set-end") { t.setUpdate(1, 4, 1); }
        else if (name == "divide-zero") { t.divideUpdate(0, 3, 0); }
        else if (name == "divide-one-end") { t.divideUpdate(0, 4, 1); }
        else if (name == "chmin-sentinel") { t.chminUpdate(0, 3, SegTreeBeats::LOW); }
        else if (name == "chmax-sentinel") { t.chmaxUpdate(0, 3, SegTreeBeats::LOW); }
        else if (name == "sqrt-negative") { t.sqrtUpdate(0, 3); }
        else if (name == "mod-zero") { t.modUpdate(2, 3, 0); }
        else if (name == "mod-negative") { t.modUpdate(0, 2, 5); }
        else if (name == "sum-end") { (void)t.sumQuery(0, 4); }
        else if (name == "max-reversed") { (void)t.maxQuery(3, 2); }
        else if (name == "min-negative") { (void)t.minQuery(-1, 0); }
        else if (name == "historic-negative-size") { HistoricSegTree x(-1); }
        else if (name == "historic-add-end") { h.addUpdate(0, 4, 1); }
        else if (name == "historic-query-reversed") { (void)h.historicSumQuery(2, 1); }
        else if (name == "generic-negative-size") { LazySegmentTreeBeats<RangeAndOrRangeSumMax<>> x(-1); }
        else if (name == "generic-get-end") { (void)g.get(3); }
        else if (name == "generic-set-negative") { g.set(-1, RangeAndOrRangeSumMax<>::leaf(1)); }
        else if (name == "generic-apply-reversed") { g.apply(2, 1, RangeAndOrRangeSumMax<>::orWith(1)); }
        else if (name == "generic-prod-end") { (void)g.prod(0, 4); }
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
        std::cout << "segtreebeats seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        vector<int> sizes = mode == "quick" ? vector<int>{0, 1, 2, 3, 5, 8, 13} : vector<int>{0, 1, 2, 3, 4, 5, 7, 8, 9, 16, 17, 31, 33, 64, 100};
        if (mode == "stress") { sizes.push_back(257); sizes.push_back(1000); }
        int steps = mode == "quick" ? 150 : mode == "full" ? 1500 : 6000;
        for (int n : sizes) {
            beatsRun(rng, n, steps, {0, 1, 2, 3}, -12, 12, "addChminChmaxSet");
            beatsRun(rng, n, steps, {1, 2}, -30, 30, "chminChmax");
            beatsRun(rng, n, steps, {0, 3, 4, 5}, 0, 40, "addSetDivideSqrt");
            beatsRun(rng, n, steps, {0, 4}, -1000000, 1000000, "addDivideWide");
            beatsRun(rng, n, steps, {3, 6, 1, 4, 5}, 0, 60, "setModChminDivideSqrt");
            beatsRun(rng, n, steps, {0, 1, 2, 3, 4, 5, 6}, -25, 25, "allMixed");
            beatsRun(rng, n, steps, {0, 1, 2, 3, 4, 5, 6}, 0, 100000, "allMixedNonnegative");
            historicRun(rng, n, steps, 9);
            andOrRun<ulng>(rng, n, steps, 64); andOrRun<uint>(rng, n, steps, 5); andOrRun<lng>(rng, n, steps, 7);
            chminGenericRun(rng, n, steps); affineGenericRun(rng, n, steps);}
        std::cout << "PASS segtreebeats checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL segtreebeats seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
