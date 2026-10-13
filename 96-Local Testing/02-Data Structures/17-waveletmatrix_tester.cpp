#include "../../02-Data Structures/17-waveletmatrix.hpp"

namespace {
    ulng seed = 20261013;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { ulng span = ulng(hi) - ulng(lo) + 1; return lng(ulng(lo) + (span ? rng() % span : rng())); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    string show(const pair<lng, int> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) { string s = "["; for (auto &x : v) { s += show(x) + ","; } return s + "]"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Values from a distribution: span 0 (one value), tiny, small, wide signed, extremes only, or k exact distinct values.
    lng value(Rng &rng, int dist, int k) {
        switch (dist) {
            case 0: return 7;
            case 1: return rnd(rng, -2, 1);
            case 2: return rnd(rng, 0, 20);
            case 3: return rnd(rng, LO, HI);
            case 4: { lng e[] = {LO, LO + 1, -1, 0, 1, HI - 1, HI}; return e[rng() % 7]; }
            default: return 1000 * rnd(rng, 0, k - 1) - 3;}}
    // Query points: present values, their neighbours (without overflow) and the type extremes.
    template<typename T> vector<T> probes(const vector<T> &a) {
        vector<T> res(a);
        if constexpr (std::is_integral_v<T>) {
            for (T x : a) {
                if (x > std::numeric_limits<T>::min()) { res.push_back(T(x - 1)); }
                if (x < std::numeric_limits<T>::max()) { res.push_back(T(x + 1)); }}
            res.push_back(std::numeric_limits<T>::min()); res.push_back(std::numeric_limits<T>::max()); res.push_back(T(0));}
        else { res.push_back(T()); res.push_back(T("zz")); res.push_back(T("b")); }
        sort(res.begin(), res.end());
        res.erase(unique(res.begin(), res.end()), res.end());
        return res;}

    // Checks every operation on [l, r) against the sorted window; weights may differ from values.
    template<typename T, typename W>
    void window(const WaveletMatrix<T, W> &wm, const vector<T> &a, const vector<W> &w, int l, int r, const vector<T> &xs, Rng &rng, bool all) {
        vector<pair<T, int>> s;
        for (int i = l; i < r; ++i) { s.push_back({a[i], i}); }
        sort(s.begin(), s.end());
        int len = r - l;
        for (int k = 0; k < len; ++k) {
            if (!all && rng() % uint(len) >= 6) { continue; }
            checkEqual(wm.kth(l, r, k), s[k].first, "kth k=" + show(k));
            checkEqual(wm.kthMax(l, r, k), s[len - 1 - k].first, "kthMax k=" + show(k));}
        if (!w.empty()) {
            W acc = W();
            for (int k = 0; k <= len; ++k) {
                if (all || rng() % uint(len + 1) < 6) { checkEqual(wm.kthSum(l, r, k), acc, "kthSum k=" + show(k)); }
                if (k < len) { acc += w[s[k].second]; }}}
        for (const T &x : xs) {
            if (!all && rng() % xs.size() >= 6) { continue; }
            int less = 0, eq = 0;
            W sl = W();
            for (int i = l; i < r; ++i) {
                if (a[i] < x) { ++less; if (!w.empty()) { sl += w[i]; }}
                if (a[i] == x) { ++eq; }}
            checkEqual(wm.countLess(l, r, x), less, "countLess x=" + show(x));
            checkEqual(wm.countEqual(l, r, x), eq, "countEqual x=" + show(x));
            if (!w.empty()) { checkEqual(wm.sumLess(l, r, x), sl, "sumLess x=" + show(x)); }
            T res = x, sentinel = x;
            bool ok = wm.maxLeq(l, r, x, res);
            checkEqual(ok, less + eq > 0, "maxLeq found x=" + show(x));
            checkEqual(res, ok ? s[less + eq - 1].first : sentinel, "maxLeq value x=" + show(x));
            res = x;
            ok = wm.minGeq(l, r, x, res);
            checkEqual(ok, less < len, "minGeq found x=" + show(x));
            checkEqual(res, ok ? s[less].first : sentinel, "minGeq value x=" + show(x));
            const T &y = xs[rng() % xs.size()];
            int range = 0;
            for (int i = l; i < r; ++i) { range += !(a[i] < x) && a[i] < y; }
            checkEqual(wm.countRange(l, r, x, y), range, "countRange lo=" + show(x) + " hi=" + show(y));}}

    // rank and select for every probe, or for 20 sampled probes when not exhaustive.
    template<typename T, typename W>
    void wholeArray(const WaveletMatrix<T, W> &wm, const vector<T> &a, const vector<T> &xs, Rng &rng, bool all) {
        int n = int(a.size());
        for (int i = 0; i < n; ++i) { checkEqual(wm.access(i), a[i], "access i=" + show(i)); }
        for (const T &x : xs) {
            if (!all && rng() % xs.size() >= 20) { continue; }
            vector<int> at;
            for (int i = 0; i < n; ++i) { if (a[i] == x) { at.push_back(i); }}
            for (int k = 0; k <= int(at.size()) + 1; ++k) { checkEqual(wm.select(x, k), k < int(at.size()) ? at[k] : -1, "select x=" + show(x) + " k=" + show(k)); }
            for (int r = 0, c = 0; r <= n; ++r) {
                checkEqual(wm.rank(r, x), c, "rank r=" + show(r) + " x=" + show(x));
                if (r < n && a[r] == x) { ++c; }}}}

    template<typename T, typename W>
    void runArray(Rng &rng, const vector<T> &a, const vector<W> &w, bool exhaustive) {
        context = "array n=" + show(a.size()) + " a=" + (a.size() <= 16 ? show(a) : string("...")) + " weighted=" + show(!w.empty());
        WaveletMatrix<T, W> wm(a, w);
        vector<T> xs = probes(a);
        wholeArray(wm, a, xs, rng, exhaustive);
        int n = int(a.size());
        if (exhaustive) {
            for (int l = 0; l <= n; ++l) {
                for (int r = l; r <= n; ++r) { window(wm, a, w, l, r, xs, rng, true); }}}
        else {
            for (int q = 0; q < 60; ++q) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, l, n));
                window(wm, a, w, l, r, xs, rng, false);}}}

    void arrays(Rng &rng) {
        int small = mode == "quick" ? 9 : mode == "full" ? 22 : 40;
        for (int n = 0; n <= small; ++n) {
            for (int dist = 0; dist < 5; ++dist) {
                vector<lng> a(n), w(n);
                for (int i = 0; i < n; ++i) { a[i] = value(rng, dist, 0); w[i] = rnd(rng, -1000000, 1000000); }
                runArray(rng, a, w, true);
                runArray(rng, a, vector<lng>{}, n % 4 == 0);}}
        // Distinct counts around powers of two set the level count; x above the maximum reaches rank k.
        for (int k : {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 64}) {
            for (int rep = 0; rep < (mode == "quick" ? 1 : 3); ++rep) {
                int n = k + int(rnd(rng, 0, 12));
                vector<lng> a(n), w(n);
                for (int i = 0; i < n; ++i) { a[i] = i < k ? 1000 * i - 3 : value(rng, 5, k); w[i] = a[i]; }
                std::shuffle(a.begin(), a.end(), rng);
                w = a;
                runArray(rng, a, w, n <= (mode == "quick" ? 8 : 20));}}
        int big = mode == "quick" ? 1000 : mode == "full" ? 20000 : 100000;
        for (int dist : {2, 3, 5}) {
            vector<lng> a(big), w(big);
            for (int i = 0; i < big; ++i) { a[i] = value(rng, dist, 300); w[i] = rnd(rng, -1000, 1000); }
            runArray(rng, a, w, false);}
        for (int n = 0; n <= (mode == "quick" ? 8 : 20); ++n) {
            vector<string> a(n);
            vector<lng> w(n);
            for (int i = 0; i < n; ++i) { a[i] = string(1, char('a' + rnd(rng, 0, 3))) + (rng() % 3 ? "" : "x"); w[i] = rnd(rng, 0, 9); }
            runArray(rng, a, w, true);}
        for (int n : {0, 1, 9, 30}) {
            vector<int> a(n);
            for (int i = 0; i < n; ++i) { a[i] = int(rnd(rng, std::numeric_limits<int>::min(), std::numeric_limits<int>::max())); }
            runArray(rng, a, vector<lng>{}, true);}}

    // Legacy defect: x - 1 at INT64_MIN; every bound query at the type extremes.
    void extremes() {
        context = "extremes";
        vector<lng> a = {LO, HI, 0, LO, HI, -1};
        WaveletMatrix<lng> wm(a, {1, 10, 100, 1000, 10000, 100000});
        lng res = 5;
        checkEqual(wm.countEqual(0, 6, LO), 2, "countEqual LO");
        checkEqual(wm.countLess(0, 6, LO), 0, "countLess LO");
        checkEqual(wm.countRange(0, 6, LO, HI), 4, "countRange LO HI");
        checkEqual(wm.maxLeq(0, 6, LO, res) && res == LO, true, "maxLeq LO");
        res = 5;
        checkEqual(wm.maxLeq(1, 3, -2, res), false, "maxLeq none");
        checkEqual(res, lng(5), "maxLeq untouched");
        checkEqual(wm.minGeq(0, 6, HI, res) && res == HI, true, "minGeq HI");
        checkEqual(wm.minGeq(0, 1, LO + 1, res), false, "minGeq none");
        checkEqual(wm.kth(0, 6, 0), LO, "kth min"); checkEqual(wm.kthMax(0, 6, 0), HI, "kthMax max");
        checkEqual(wm.sumLess(1, 3, HI), lng(100), "sumLess below HI");
        checkEqual(wm.sumLess(0, 6, LO), lng(0), "sumLess below LO");
        checkEqual(wm.kthSum(0, 6, 0), lng(0), "kthSum 0");
        checkEqual(wm.kthSum(0, 6, 2), lng(1001), "kthSum ties by index");
        WaveletMatrix<lng> empty;
        checkEqual(empty.countLess(0, 0, 5), 0, "empty countLess"); checkEqual(empty.select(5, 0), -1, "empty select");
        checkEqual(empty.sumLess(0, 0, 5), lng(0), "empty sumLess"); checkEqual(empty.kthSum(0, 0, 0), lng(0), "empty kthSum");
        checkEqual(empty.maxLeq(0, 0, 5, res), false, "empty maxLeq"); checkEqual(empty.rank(0, 1), 0, "empty rank");}

    // Points with duplicate and extreme coordinates; rectangles include empty and reversed sides.
    void rectangles(Rng &rng) {
        int rounds = mode == "quick" ? 30 : mode == "full" ? 300 : 1500;
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, round % 10 == 0 ? 400 : 25)), dist = round % 5;
            vector<lng> x(n), y(n), w(n);
            for (int i = 0; i < n; ++i) { x[i] = value(rng, dist == 0 ? 1 : dist, 0); y[i] = value(rng, (dist + round / 5) % 5, 0); w[i] = rnd(rng, -1000000000, 1000000000); }
            context = "rectangles round=" + show(round) + " n=" + show(n) + (n <= 8 ? " x=" + show(x) + " y=" + show(y) : string());
            RectangleWaveletMatrix<lng> rw(x, y, w), rc(x, y);
            vector<lng> cx = probes(x), cy = probes(y);
            for (int q = 0; q < 40; ++q) {
                lng xl = cx[rng() % cx.size()], xr = cx[rng() % cx.size()], yl = cy[rng() % cy.size()], yr = cy[rng() % cy.size()];
                int cnt = 0;
                lng sum = 0;
                for (int i = 0; i < n; ++i) {
                    if (!(x[i] < xl) && x[i] < xr && !(y[i] < yl) && y[i] < yr) { ++cnt; sum += w[i]; }}
                string at = " rect=[" + show(xl) + "," + show(xr) + ")x[" + show(yl) + "," + show(yr) + ")";
                checkEqual(rw.rectangleCount(xl, xr, yl, yr), cnt, "rectangleCount" + at);
                checkEqual(rc.rectangleCount(xl, xr, yl, yr), cnt, "rectangleCount unweighted" + at);
                checkEqual(rw.rectangleSum(xl, xr, yl, yr), sum, "rectangleSum" + at);}}}

    struct SumMod {
        using S = lng;
        static S op(S a, S b) { return (a + b) % P; }
        static S e() { return 0; }
    };
    // Maximum with multiplicity: commutative, not invertible, not idempotent.
    struct MaxCount {
        using S = pair<lng, int>;
        static S op(S a, S b) { return a.first != b.first ? max(a, b) : S{a.first, a.second + b.second}; }
        static S e() { return {LO, 0}; }
    };
    template<typename M>
    void monoidRun(Rng &rng, const string &name, auto gen) {
        int rounds = mode == "quick" ? 20 : mode == "full" ? 150 : 700;
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, round % 10 == 0 ? 300 : 20)), dist = round % 5;
            vector<lng> x(n), y(n);
            vector<typename M::S> v(n);
            for (int i = 0; i < n; ++i) { x[i] = value(rng, dist, 0); y[i] = value(rng, (dist + 2) % 5, 0); v[i] = gen(rng); }
            context = name + " round=" + show(round) + " n=" + show(n);
            WaveletMatrixMonoid<lng, M> wm(x, y, round % 3 ? v : vector<typename M::S>{});
            if (round % 3 == 0) { std::fill(v.begin(), v.end(), M::e()); }
            vector<lng> cx = probes(x), cy = probes(y);
            for (int q = 0; q < 60; ++q) {
                if (n && rng() % 2) {
                    int i = int(rnd(rng, 0, n - 1));
                    auto s = gen(rng);
                    if (rng() % 2) { wm.set(i, s); v[i] = s; } else { wm.apply(i, s); v[i] = M::op(v[i], s); }
                    checkEqual(wm.get(i), v[i], "get i=" + show(i));}
                lng xl = cx[rng() % cx.size()], xr = cx[rng() % cx.size()], yl = cy[rng() % cy.size()], yr = cy[rng() % cy.size()];
                auto want = M::e();
                for (int i = 0; i < n; ++i) {
                    if (!(x[i] < xl) && x[i] < xr && !(y[i] < yl) && y[i] < yr) { want = M::op(want, v[i]); }}
                checkEqual(wm.prod(xl, xr, yl, yr), want, "prod rect=[" + show(xl) + "," + show(xr) + ")x[" + show(yl) + "," + show(yr) + ")");}
            if (n) {
                auto copy = wm;
                copy.set(0, gen(rng));
                checkEqual(wm.get(0), v[0], "copy independence");}}}

    // Power-of-two point counts with one y value: the whole bottom level folds through the segment-tree root.
    void monoidEdges() {
        for (int n : {1, 2, 4, 8, 16, 32}) {
            context = "monoid edges n=" + show(n);
            vector<lng> x(n), y(n, 5), v(n);
            for (int i = 0; i < n; ++i) { x[i] = i - 3; v[i] = i + 1; }
            WaveletMatrixMonoid<lng, SumMod> wm(x, y, v);
            lng total = lng(n) * (n + 1) / 2;
            checkEqual(wm.prod(LO, HI, LO, HI), total, "whole plane");
            wm.set(n - 1, 100); total += 100 - n;
            checkEqual(wm.prod(LO, HI, LO, HI), total, "after set");
            wm.apply(0, 7); total += 7;
            checkEqual(wm.prod(LO, HI, 5, 6), total, "after apply");
            checkEqual(wm.prod(LO, HI, 6, HI), lng(0), "above every point");
            checkEqual(wm.prod(-3, -2, LO, HI), lng(n == 1 ? 107 : 8), "first column");}}

    void invalid(const string &name) {
        vector<lng> a = {3, 1, 4};
        WaveletMatrix<lng> wm(a), ww(a, a);
        RectangleWaveletMatrix<lng> rc(a, a);
        WaveletMatrixMonoid<lng, SumMod> mm(a, a);
        lng res = 0;
        if (name == "wavelet-access-range") { (void)wm.access(3); }
        else if (name == "wavelet-access-negative") { (void)wm.access(-1); }
        else if (name == "wavelet-reversed") { (void)wm.countLess(2, 1, 0); }
        else if (name == "wavelet-end") { (void)wm.countEqual(0, 4, 0); }
        else if (name == "wavelet-countrange-end") { (void)wm.countRange(0, 4, 5, 1); }
        else if (name == "wavelet-kth-range") { (void)wm.kth(0, 3, 3); }
        else if (name == "wavelet-kth-empty") { (void)wm.kth(1, 1, 0); }
        else if (name == "wavelet-kthmax-negative") { (void)wm.kthMax(0, 3, -1); }
        else if (name == "wavelet-maxleq-range") { (void)wm.maxLeq(0, 4, 1, res); }
        else if (name == "wavelet-mingeq-range") { (void)wm.minGeq(-1, 2, 1, res); }
        else if (name == "wavelet-select-negative") { (void)wm.select(3, -1); }
        else if (name == "wavelet-rank-range") { (void)wm.rank(4, 3); }
        else if (name == "wavelet-sum-unweighted") { (void)wm.sumLess(0, 3, 4); }
        else if (name == "wavelet-kthsum-unweighted") { (void)wm.kthSum(0, 3, 1); }
        else if (name == "wavelet-kthsum-range") { (void)ww.kthSum(0, 3, 4); }
        else if (name == "wavelet-weights-size") { WaveletMatrix<lng> bad(a, vector<lng>{1}); }
        else if (name == "rectangle-size") { RectangleWaveletMatrix<lng> bad(a, vector<lng>{1}); }
        else if (name == "rectangle-sum-unweighted") { (void)rc.rectangleSum(0, 9, 0, 9); }
        else if (name == "monoid-size") { WaveletMatrixMonoid<lng, SumMod> bad(a, a, vector<lng>{1}); }
        else if (name == "monoid-set-range") { mm.set(3, 1); }
        else if (name == "monoid-apply-negative") { mm.apply(-1, 1); }
        else if (name == "monoid-get-range") { (void)mm.get(3); }
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
        std::cout << "waveletmatrix seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        extremes();
        monoidEdges();
        arrays(rng);
        rectangles(rng);
        monoidRun<SumMod>(rng, "monoid summod", [](Rng &g) { return rnd(g, 0, P - 1); });
        monoidRun<MaxCount>(rng, "monoid maxcount", [](Rng &g) { return pair<lng, int>{rnd(g, -3, 3), int(rnd(g, 1, 3))}; });
        std::cout << "PASS waveletmatrix checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL waveletmatrix seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
