#include "../../02-Data Structures/22-convexhulltrick.hpp"

namespace {
    ulng seed = 20261028;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    constexpr lng LIM = (1LL << 62) - 1, LCM = 1000000000, LCB = 1000000000000000000;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<std::floating_point T> string show(const T &x) { std::ostringstream s; s << std::setprecision(std::numeric_limits<T>::max_digits10) << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }

    struct L { lng m, b; };
    // Lines of the current history, printed (first 40) as the reproducer on failure.
    const vector<L> *hist = nullptr;
    string showLines() {
        if (!hist) { return ""; }
        string s = " lines(" + show(hist->size()) + ")=[";
        for (size_t i = 0; i < hist->size() && i < 40; ++i) { s += (i ? " " : "") + show((*hist)[i].m) + "x+" + show((*hist)[i].b); }
        return s + (hist->size() > 40 ? " ...]" : "]");}
    [[noreturn]] void fail(const string &msg) { throw std::runtime_error(context + " " + msg + showLines()); }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { fail("operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}
    // checkEqual that builds the operation text only on failure.
    template<typename T, typename U, typename F> void checkLazy(const T &got, const U &want, F op) {
        ++checks;
        if (!(got == want)) { fail("operation=" + op() + " expected=" + show(want) + " actual=" + show(got)); }}

    lll at(const L &f, lng x) { return lll(f.m) * x + f.b; }
    // Floating lines; Q<T> is the brute-force oracle type (long double for double, __float128 for long double).
    template<typename T> struct FL { T m, b; };
    template<typename T> using Q = std::conditional_t<std::is_same_v<T, double>, long double, __float128>;
    __float128 at(const FL<double> &f, double x) { return __float128(f.m) * __float128(x) + __float128(f.b); }
    __float128 at(const FL<long double> &f, long double x) { return __float128(f.m) * __float128(x) + __float128(f.b); }
    lng narrow(lll v) {
        if (v <= LLONG_MIN || v >= LLONG_MAX) { fail("test input outside the domain: oracle value does not fit"); }
        return lng(v);}
    // Brute force over every added line in __int128; ls is nonempty.
    template<bool MAX> lll brute(const vector<L> &ls, lng x) {
        lll res = at(ls[0], x);
        for (auto &f : ls) { res = MAX ? max(res, at(f, x)) : min(res, at(f, x)); }
        return res;}

    // {value, id}: value equals want and line id attains it; without want, id is the front-most optimal hull line: no optimal line has a larger
    // slope (min) or smaller slope (max), and no earlier line is identical.
    template<bool MAX> void checkMono(pair<lng, int> got, const vector<L> &ls, lng x, const char *op, const char *kind, const lll *want = nullptr) {
        auto where = [&] { return string(op) + " " + kind + " x=" + show(x); };
        if (ls.empty()) {
            checkLazy(got, pair<lng, int>(MonotoneCHT<MAX>::NONE, -1), [&] { return where() + " empty"; });
            return;}
        checkLazy(got.first, narrow(want ? *want : brute<MAX>(ls, x)), [&] { return where() + " value"; });
        auto [v, id] = got;
        ++checks;
        if (id < 0 || id >= int(ls.size()) || at(ls[size_t(id)], x) != v) { fail("operation=" + where() + " line id=" + show(id) + " does not attain value " + show(v)); }
        if (want) { return; }
        const L &g = ls[size_t(id)];
        for (int i = 0; i < int(ls.size()); ++i) {
            const L &f = ls[size_t(i)];
            if (at(f, x) == v && ((MAX ? f.m < g.m : f.m > g.m) || (f.m == g.m && f.b == g.b && i < id))) {
                fail("operation=" + where() + " line id=" + show(id) + " is not the front-most optimal line: line " + show(i) + " is optimal too");}}}

    template<bool MAX> struct Mono {
        MonotoneCHT<MAX> c;
        vector<L> ls;
        lng lo = 0, hi = 0;
        bool valid(lng m) const { return ls.empty() || m <= lo || m >= hi; }
        void add(lng m, lng b) {
            lo = ls.empty() ? m : min(lo, m); hi = ls.empty() ? m : max(hi, m);
            c.addLine(m, b);
            ls.push_back({m, b});}
        // query first, then queryMonotone must return the identical {value, id}.
        void query(lng x, const char *op) {
            hist = &ls;
            auto got = std::as_const(c).query(x);
            checkMono<MAX>(got, ls, x, op, "query");
            checkLazy(c.queryMonotone(x), got, [&] { return string(op) + " queryMonotone equals query x=" + show(x); });}
    };
    template<bool MAX> struct Cont {
        LineContainer<MAX> c;
        vector<L> ls;
        void add(lng m, lng b) {
            c.addLine(m, b);
            ls.push_back({m, b});}
        void query(lng x, const char *op) {
            hist = &ls;
            lng got = std::as_const(c).query(x);
            auto where = [&] { return string(op) + " LineContainer query x=" + show(x); };
            if (ls.empty()) { checkLazy(got, LineContainer<MAX>::NONE, where); }
            else { checkLazy(got, narrow(brute<MAX>(ls, x)), where); }}
    };

    // A slope at least or at most every slope of h, at most span beyond it and inside [-M, M].
    template<bool MAX> lng monoSlope(Rng &rng, const Mono<MAX> &h, lng M, lng span) {
        if (h.ls.empty()) { return rnd(rng, -M, M); }
        bool up = rng() & 1;
        if (up && h.hi == M) { up = false; }
        if (!up && h.lo == -M) { up = true; }
        return up ? rnd(rng, h.hi, min(M, h.hi + span)) : rnd(rng, max(-M, h.lo - span), h.lo);}
    // Query x patterns: 0 increasing, 1 decreasing, 2 constant, 3 random, 4 alternating extremes, 5 zigzag.
    lng nextX(Rng &rng, int pattern, lng &cur, int k, lng X) {
        lng d = rnd(rng, 0, X / 8 + 1);
        if (pattern == 0 || (pattern == 5 && k / 16 % 2 == 0)) { cur = min(X, cur + d); }
        else if (pattern == 1 || pattern == 5) { cur = max(-X, cur - d); }
        else if (pattern == 3) { cur = rnd(rng, -X, X); }
        else if (pattern == 4) { cur = k & 1 ? rnd(rng, -X, -X / 2) : rnd(rng, X / 2, X); }
        return cur;}
    vector<lng> sweep(int kind, lng W) {
        vector<lng> xs;
        for (lng x = -W; x <= W; ++x) { xs.push_back(x); }
        if (kind == 1) { reverse(xs.begin(), xs.end()); }
        if (kind == 2) {
            vector<lng> ys;
            for (lng i = 0; i <= W; ++i) { ys.push_back(-W + i); ys.push_back(W - i); }
            xs = ys;}
        return xs;}

    template<bool MAX> void edges() {
        context = string("edges ") + (MAX ? "max" : "min");
        checkEqual(MonotoneCHT<MAX>::NONE, MAX ? LLONG_MIN : LLONG_MAX, "MonotoneCHT NONE");
        checkEqual(LineContainer<MAX>::NONE, MAX ? LLONG_MIN : LLONG_MAX, "LineContainer NONE");
        Mono<MAX> e; Cont<MAX> f;
        for (lng x : vector<lng>{0, -LIM, LIM, LLONG_MIN, LLONG_MAX}) { e.query(x, "empty"); f.query(x, "empty"); }
        // Hand values validate the oracle: a single line 3x - 7, then 3x + 1 and -2x - 6 crossing at x = -7/5.
        Mono<MAX> one; one.add(3, -7);
        hist = &one.ls;
        checkEqual(one.c.query(2), pair<lng, int>(-1, 0), "single line query"); checkEqual(one.c.query(-4), pair<lng, int>(-19, 0), "single line query x=-4");
        checkEqual(one.c.queryMonotone(-4), pair<lng, int>(-19, 0), "single line queryMonotone");
        Mono<MAX> two; two.add(3, 1); two.add(-2, -6);
        Cont<MAX> ctwo; ctwo.add(3, 1); ctwo.add(-2, -6);
        hist = &two.ls;
        checkEqual(two.c.query(-2), pair<lng, int>(MAX ? -2 : -5, MAX ? 1 : 0), "two lines x=-2");
        checkEqual(two.c.query(-1), pair<lng, int>(MAX ? -2 : -4, MAX ? 0 : 1), "two lines x=-1");
        checkEqual(two.c.queryMonotone(-1), pair<lng, int>(MAX ? -2 : -4, MAX ? 0 : 1), "two lines queryMonotone x=-1");
        checkEqual(ctwo.c.query(-2), lng(MAX ? -2 : -5), "LineContainer two lines x=-2"); checkEqual(ctwo.c.query(-1), lng(MAX ? -2 : -4), "LineContainer two lines x=-1");
        checkEqual(lng(brute<MAX>(two.ls, -2)), lng(MAX ? -2 : -5), "brute two lines x=-2"); checkEqual(lng(brute<MAX>(two.ls, -1)), lng(MAX ? -2 : -4), "brute two lines x=-1");
        for (lng x = -12; x <= 12; ++x) { one.query(x, "single"); two.query(x, "two"); ctwo.query(x, "two"); }
        // Identical and dominated lines still consume ids: the 3rd call duplicates line 0, the 4th has slope 3 and a worse intercept.
        Mono<MAX> dup; dup.add(1, 0); dup.add(1, 0); dup.add(-1, 0); dup.add(-1, MAX ? -5 : 5); dup.add(-1, MAX ? 3 : -3);
        hist = &dup.ls;
        checkEqual(dup.c.query(-10), pair<lng, int>(MAX ? 13 : -10, MAX ? 4 : 0), "duplicates x=-10");
        checkEqual(dup.c.query(10), pair<lng, int>(MAX ? 10 : -13, MAX ? 0 : 4), "duplicates x=10");
        for (lng x = -6; x <= 6; ++x) { dup.query(x, "duplicates"); }
        // Slope-zero lines at the extreme x of lng, slopes +-1 near them.
        for (int r = 0; r < 2; ++r) {
            Mono<MAX> h; Cont<MAX> g;
            for (lng b : vector<lng>{5, -LIM, LIM, 0}) { h.add(0, b); g.add(0, b); }
            if (r) { h.add(1, -5); g.add(1, -5); h.add(-1, 3); g.add(-1, 3); h.add(1, 4); g.add(1, 4); g.add(0, -2); }
            vector<lng> xs = {0, 1, -1, LLONG_MAX - 10, LLONG_MIN + 10, LIM, -LIM, 1LL << 62, -(1LL << 62)};
            if (!r) { xs.insert(xs.end(), {LLONG_MAX, LLONG_MIN}); }
            for (lng x : xs) { h.query(x, "huge x"); g.query(x, "huge x"); }}}

    // Two lines crossing at integer and non-integer points of both signs; queries around the breakpoint, both insertion orders.
    template<bool MAX> void crossings(Rng &rng, int randomPairs) {
        context = string("crossings ") + (MAX ? "max" : "min");
        for (lng m1 = -4; m1 <= 4; ++m1) {
            for (lng m2 = -4; m2 <= 4; ++m2) {
                for (lng b1 : {-9, -4, -1, 0, 2, 7}) {
                    for (lng b2 : {-8, -3, 0, 1, 6}) {
                        Mono<MAX> h; Cont<MAX> g;
                        h.add(m1, b1); h.add(m2, b2); g.add(m1, b1); g.add(m2, b2);
                        lng p = m1 == m2 ? 0 : (b2 - b1) / (m1 - m2);
                        for (lng x = p - 3; x <= p + 3; ++x) { h.query(x, "small pair"); g.query(x, "small pair"); }}}}}
        for (int r = 0; r < randomPairs; ++r) {
            lng m1 = rnd(rng, -LCM, LCM), m2 = rnd(rng, -LCM, LCM), b1 = rnd(rng, -LCB / 2, LCB / 2), p = rnd(rng, -LCM + 3, LCM - 3);
            if (r % 3 == 0) { m2 = m1 + (m1 < LCM ? 1 : -1); }
            lll b2 = b1 + lll(m1 - m2) * p + (m1 == m2 ? 0 : rnd(rng, -abs(m1 - m2) + 1, abs(m1 - m2) - 1));
            if (b2 < -LCB || b2 > LCB) { continue; }
            Mono<MAX> h; Cont<MAX> g;
            if (r & 1) { h.add(m1, b1); h.add(m2, lng(b2)); g.add(m1, b1); g.add(m2, lng(b2)); }
            else { h.add(m2, lng(b2)); h.add(m1, b1); g.add(m2, lng(b2)); g.add(m1, b1); }
            for (lng x = p - 2; x <= p + 2; ++x) { h.query(x, "random pair"); g.query(x, "random pair"); }}}

    // Concurrent and nearly concurrent triples and a quadruple in every insertion order (MonotoneCHT: every valid order).
    template<bool MAX> void concurrent() {
        context = string("concurrent ") + (MAX ? "max" : "min");
        vector<vector<L>> sets = {{{-2, 2}, {0, 5}, {2, 8}}, {{-1, 14}, {0, 7}, {1, 0}}, {{-1, 0}, {0, 0}, {1, 0}}, {{-2, 2}, {0, 5}, {2, 9}}, {{-2, 2}, {0, 5}, {2, 7}}, {{-3, 0}, {0, 0}, {0, 0}, {3, 0}}, {{-2, 0}, {-1, 0}, {1, 0}, {2, 0}}, {{-3, -3}, {-1, -1}, {1, 1}, {3, 3}}};
        for (auto &set : sets) {
            vector<int> perm(set.size());
            iota(perm.begin(), perm.end(), 0);
            do {
                Mono<MAX> h; Cont<MAX> g;
                bool ok = true;
                for (int i : perm) {
                    auto f = set[size_t(i)];
                    g.add(f.m, f.b);
                    ok = ok && h.valid(f.m);
                    if (ok) { h.add(f.m, f.b); }
                    for (lng x = -12; x <= 12; ++x) {
                        g.query(x, "concurrent");
                        if (ok) { h.query(x, "concurrent"); }}}} while (std::next_permutation(perm.begin(), perm.end()));}}

    // Tie rule: the pointer reaches each tie point x0 from both sides, step by step and by jumps, on a fresh copy of h.
    template<bool MAX> void approach(Mono<MAX> h, lng x0, const char *op) {
        for (lng x : {x0 - 6, x0 - 1, x0, x0 + 6, x0 + 1, x0, x0 - 1, x0, x0 + 1, x0, x0}) { h.query(x, op); }
        for (lng x = x0 - 4; x <= x0 + 4; ++x) { h.query(x, op); }
        for (lng x = x0 + 4; x >= x0 - 4; --x) { h.query(x, op); }}
    // y = x and y = -x at 0 (hand ids), concurrent triples and a quadruple in every valid order, and lines with every breakpoint at an integer.
    template<bool MAX> void ties() {
        context = string("ties ") + (MAX ? "max" : "min");
        for (int r = 0; r < 2; ++r) {
            Mono<MAX> h;
            h.add(r ? -1 : 1, 0); h.add(r ? 1 : -1, 0);
            hist = &h.ls;
            // Front-most: the larger slope for min, the smaller for max.
            checkEqual(std::as_const(h.c).query(0), pair<lng, int>(0, MAX != bool(r) ? 1 : 0), "y=x, y=-x query x=0");
            approach(h, 0, "y=x, y=-x");
            for (lng x0 : {-3, 3}) {
                Mono<MAX> g = h;
                g.query(x0, "y=x, y=-x from far");
                checkEqual(g.c.queryMonotone(0), pair<lng, int>(0, MAX != bool(r) ? 1 : 0), "y=x, y=-x queryMonotone x=0 from x=" + show(x0));}}
        for (lng x0 : {-3, 0, 4}) {
            for (vector<lng> ms : vector<vector<lng>>{{-1, 0, 1}, {-5, -2, 3}, {-3, -1, 0, 2}, {-2, 0, 0, 2}}) {
                do {
                    Mono<MAX> h;
                    bool ok = true;
                    for (lng m : ms) {
                        ok = ok && h.valid(m);
                        if (!ok) { break; }
                        h.add(m, 7 - m * x0);
                        approach(h, x0, "concurrent");}} while (std::next_permutation(ms.begin(), ms.end()));}}
        // b = s m (m - 1) / 2: consecutive slopes cross at integer x, so ties at every breakpoint (s = 1 min hull, s = -1 max hull).
        for (lng s : {1, -1}) {
            for (int order = 0; order < 3; ++order) {
                Mono<MAX> h;
                for (int i = 0; i <= 8; ++i) {
                    lng m = order == 0 ? i - 4 : order == 1 ? 4 - i : (i & 1 ? 1 : -1) * ((i + 1) / 2);
                    h.add(m, s * m * (m - 1) / 2);
                    for (lng x : sweep(int(i % 3), 6)) { h.query(x, "integer breakpoints"); }}
                for (lng x0 = -5; x0 <= 5; ++x0) { approach(h, x0, "integer breakpoints"); }}}}

    // Slopes and intercepts near +-(2^62 - 1) with x in {-1, 0, 1}: every ordered pair, then random histories.
    template<bool MAX> void extremes(Rng &rng, int runs) {
        context = string("extremes ") + (MAX ? "max" : "min");
        vector<lng> E = {-LIM, -LIM + 1, -(1LL << 61), -1, 0, 1, 1LL << 61, LIM - 1, LIM};
        for (lng m1 : E) {
            for (lng b1 : E) {
                for (lng m2 : E) {
                    for (lng b2 : E) {
                        Mono<MAX> h; Cont<MAX> g;
                        h.add(m1, b1); h.add(m2, b2); g.add(m1, b1); g.add(m2, b2);
                        for (lng x = -1; x <= 1; ++x) { h.query(x, "extreme pair"); g.query(x, "extreme pair"); }}}}}
        for (int r = 0; r < runs; ++r) {
            Mono<MAX> h; Cont<MAX> g;
            int n = int(rnd(rng, 1, 16));
            for (int i = 0; i < n; ++i) {
                lng b = rng() & 1 ? E[size_t(rnd(rng, 0, 8))] : rnd(rng, -LIM, LIM), m;
                if (rng() & 1) {
                    vector<lng> ok;
                    for (lng e : E) {
                        if (h.valid(e)) { ok.push_back(e); }}
                    m = ok[size_t(rnd(rng, 0, lng(ok.size()) - 1))];}
                else { m = monoSlope(rng, h, LIM, LIM); }
                h.add(m, b); g.add(rng() & 1 ? m : rnd(rng, -LIM, LIM), b);
                for (lng x = -1; x <= 1; ++x) { h.query(x, "extreme random"); g.query(x, "extreme random"); }}}}

    // |m * x| or |m * x + b| reaching 2^63 - 2 at the queried x, which overflows a negated side that assumes a narrower domain.
    template<bool MAX> void boundary() {
        context = string("boundary ") + (MAX ? "max" : "min");
        const lll K = (lll(1) << 63) - 2;
        vector<lng> S = {-3, -2, -1, 0, 1, 2, 3}, B = {-LIM, -(1LL << 61), -1, 0, 1, 1LL << 61, LIM}, X = {0};
        for (lng m = 1; m <= 3; ++m) {
            for (lll v : {K / m, (K - LIM) / m}) { X.push_back(lng(v)); X.push_back(lng(-v)); }}
        for (lng x : X) {
            for (int dir = 0; dir < 2; ++dir) {
                Mono<MAX> h; Cont<MAX> g;
                for (int i = 0; i < 7; ++i) {
                    lng m = S[size_t(dir ? 6 - i : i)];
                    for (lng b : B) {
                        lll v = lll(m) * x;
                        if (v < -K || v > K || v + b < -K || v + b > K) { continue; }
                        h.add(m, b); g.add(m, b);
                        h.query(x, "boundary"); g.query(x, "boundary");}}}}}

    // Random histories with interleaved queries in the given box; x follows one of the nextX patterns.
    template<bool MAX> void monoRandom(Rng &rng, int runs, int maxN, lng M, lng B, lng X, const string &name) {
        for (int r = 0; r < runs; ++r) {
            Mono<MAX> h;
            int n = int(rnd(rng, 1, maxN)), pattern = int(rnd(rng, 0, 5)), qs = 0;
            lng cur = pattern == 0 ? -X : pattern == 1 ? X : rnd(rng, -X, X), span = max(M >> rnd(rng, 0, 4), lng(1));
            context = string("MonotoneCHT random ") + (MAX ? "max " : "min ") + name + " run=" + show(r) + " pattern=" + show(pattern);
            h.query(cur, "start");
            while (int(h.ls.size()) < n || qs < 2 * n) {
                if (int(h.ls.size()) < n && (qs >= 2 * n || rng() % 3 == 0)) {
                    L f = h.ls.empty() ? L{0, 0} : h.ls[size_t(rnd(rng, 0, lng(h.ls.size()) - 1))];
                    if (!h.ls.empty() && rng() % 8 == 0 && h.valid(f.m)) { h.add(f.m, rng() & 1 ? f.b : rnd(rng, -B, B)); }
                    else { h.add(monoSlope(rng, h, M, span), rnd(rng, -B, B)); }}
                else { h.query(nextX(rng, pattern, cur, qs++, X), "random"); }}}}
    // Slope patterns: 0 random, 1 ascending, 2 descending, 3 few distinct, 4 alternating ends; 1/8 of lines repeat an earlier slope.
    template<bool MAX> void contRandom(Rng &rng, int runs, int maxN, lng M, lng B, lng X, const string &name) {
        for (int r = 0; r < runs; ++r) {
            Cont<MAX> g;
            int n = int(rnd(rng, 1, maxN)), pattern = int(rnd(rng, 0, 4)), qs = 0;
            vector<lng> slopes(size_t(n), 0);
            for (auto &m : slopes) { m = pattern == 3 ? rnd(rng, -2, 2) * (M / 2) : rnd(rng, -M, M); }
            sort(slopes.begin(), slopes.end());
            if (pattern == 2) { reverse(slopes.begin(), slopes.end()); }
            if (pattern == 0) { std::shuffle(slopes.begin(), slopes.end(), rng); }
            if (pattern == 4) {
                vector<lng> t;
                for (int i = 0, j = n - 1; i <= j; ++i, --j) { t.push_back(slopes[size_t(i)]); if (i < j) { t.push_back(slopes[size_t(j)]); }}
                slopes = t;}
            context = string("LineContainer random ") + (MAX ? "max " : "min ") + name + " run=" + show(r) + " pattern=" + show(pattern);
            g.query(rnd(rng, -X, X), "start");
            while (int(g.ls.size()) < n || qs < 2 * n) {
                if (int(g.ls.size()) < n && (qs >= 2 * n || rng() % 3 == 0)) {
                    L f = g.ls.empty() ? L{0, 0} : g.ls[size_t(rnd(rng, 0, lng(g.ls.size()) - 1))];
                    if (!g.ls.empty() && rng() % 8 == 0) { g.add(f.m, rng() & 1 ? f.b : rnd(rng, -B, B)); }
                    else { g.add(slopes[g.ls.size()], rnd(rng, -B, B)); }}
                else {
                    g.query(rnd(rng, -X, X), "random");
                    ++qs;}}}}

    // Every valid insertion sequence of up to depth lines from [-S, S]^2; all x in [-W, W] after each insertion, swept in varying orders.
    template<bool MAX> void monoDfs(Mono<MAX> &h, int depth, lng S, lng W, lng &nodes) {
        for (lng x : sweep(int(nodes++ % 3), W)) { h.query(x, "exhaustive"); }
        if (depth == 0) { return; }
        for (lng m = -S; m <= S; ++m) {
            for (lng b = -S; b <= S; ++b) {
                if (h.valid(m)) {
                    Mono<MAX> g = h;
                    g.add(m, b);
                    monoDfs(g, depth - 1, S, W, nodes);}}}}
    template<bool MAX> void contDfs(Cont<MAX> &h, int depth, lng S, lng W, lng &nodes) {
        ++nodes;
        for (lng x = -W; x <= W; ++x) { h.query(x, "exhaustive"); }
        if (depth == 0) { return; }
        for (lng m = -S; m <= S; ++m) {
            for (lng b = -S; b <= S; ++b) {
                Cont<MAX> g = h;
                g.add(m, b);
                contDfs(g, depth - 1, S, W, nodes);}}}
    template<bool MAX> void exhaustive(lng S, int monoDepth, int contDepth) {
        lng nodes = 0;
        context = string("exhaustive MonotoneCHT ") + (MAX ? "max" : "min") + " box=" + show(S) + " depth=" + show(monoDepth);
        Mono<MAX> h; monoDfs(h, monoDepth, S, 2 * S + 1, nodes);
        context = string("exhaustive LineContainer ") + (MAX ? "max" : "min") + " box=" + show(S) + " depth=" + show(contDepth);
        Cont<MAX> g; contDfs(g, contDepth, S, 2 * S + 1, nodes);
        Cont<MAX> g1; contDfs(g1, contDepth + 1, 1, 3, nodes);
        std::cout << "exhaustive " << (MAX ? "max" : "min") << " sequences=" << nodes << '\n';}

    // Independent oracle for large cases: minimum Li Chao tree over the sorted distinct query points, exact in __int128 (lng lines)
    // or in __float128 (floating lines: a double product is exact, a long double one rounds at 2^-113).
    template<typename X, typename F> struct LiChao {
        using V = decltype(at(F{}, X{}));
        vector<X> xs;
        vector<F> t;
        vector<char> has;
        explicit LiChao(vector<X> p) : xs(std::move(p)) {
            sort(xs.begin(), xs.end());
            xs.erase(unique(xs.begin(), xs.end()), xs.end());
            t.resize(4 * xs.size() + 4); has.assign(4 * xs.size() + 4, 0);}
        void add(F f, size_t k, size_t l, size_t r) {
            if (!has[k]) { t[k] = f; has[k] = 1; return; }
            size_t md = (l + r) / 2;
            if (at(f, xs[md]) < at(t[k], xs[md])) { std::swap(f, t[k]); }
            if (l == r) { return; }
            if (at(f, xs[l]) < at(t[k], xs[l])) { add(f, 2 * k, l, md); }
            else if (at(f, xs[r]) < at(t[k], xs[r])) { add(f, 2 * k + 1, md + 1, r); }}
        void add(F f) { add(f, 1, 0, xs.size() - 1); }
        pair<bool, V> get(X x) const {
            size_t i = 0, k = 1, l = 0, r = xs.size() - 1;
            // Hand-written lower_bound: the debug-mode std::lower_bound checks partitioning in O(n) per call.
            for (size_t step = std::bit_floor(xs.size()); step; step /= 2) {
                if (i + step <= xs.size() && xs[i + step - 1] < x) { i += step; }}
            pair<bool, V> res{false, 0};
            while (has[k]) {
                V v = at(t[k], x);
                res = {true, res.first ? min(res.second, v) : v};
                if (l == r) { break; }
                size_t md = (l + r) / 2;
                if (i <= md) { k = 2 * k; r = md; }
                else { k = 2 * k + 1; l = md + 1; }}
            return res;}
    };
    vector<lng> queryXs(Rng &rng, int q, int pattern) {
        vector<lng> xs(size_t(q), 0);
        for (auto &x : xs) { x = rnd(rng, -LCM, LCM); }
        if (pattern == 0 || pattern == 1) { sort(xs.begin(), xs.end()); }
        if (pattern == 1) { reverse(xs.begin(), xs.end()); }
        if (pattern == 3) {
            for (size_t i = 0; i < xs.size(); ++i) { xs[i] = i & 1 ? -abs(xs[i]) : abs(xs[i]); }}
        return xs;}

    // n lines grown at both ends of a sorted slope list, q queries interleaved; every query against the Li Chao oracle, a sample against brute.
    template<bool MAX> void monoScale(Rng &rng, int n, int q, lng M, int pattern) {
        context = string("MonotoneCHT scale ") + (MAX ? "max" : "min") + " n=" + show(n) + " q=" + show(q) + " M=" + show(M) + " pattern=" + show(pattern);
        vector<lng> slopes(size_t(n), 0), xs = queryXs(rng, q, pattern);
        for (auto &m : slopes) { m = rnd(rng, -M, M); }
        sort(slopes.begin(), slopes.end());
        LiChao<lng, L> oracle(xs);
        MonotoneCHT<MAX> c;
        vector<L> ls;
        hist = &ls;
        int lft = int(rnd(rng, 0, n - 1)), rgt = lft - 1, qi = 0;
        lng moves = 0;
        while (int(ls.size()) < n || qi < q) {
            if (int(ls.size()) < n && (qi == q || rnd(rng, 0, n + q - 1) < n)) {
                bool left = lft > 0 && (rgt == n - 1 || (rng() & 1));
                lng m = left ? slopes[size_t(--lft)] : slopes[size_t(++rgt)], b = rnd(rng, -LCB, LCB);
                c.addLine(m, b); ls.push_back({m, b});
                oracle.add(MAX ? L{-m, -b} : L{m, b});
                continue;}
            lng x = xs[size_t(qi)];
            auto [any, w] = oracle.get(x);
            lll want = MAX ? -w : w;
            if (any && qi % max(q / 100, 1) == 0) { checkEqual(narrow(want), narrow(brute<MAX>(ls, x)), "Li Chao oracle vs brute qi=" + show(qi) + " x=" + show(x)); }
            auto got = std::as_const(c).query(x);
            checkMono<MAX>(got, ls, x, "scale", "query", any ? &want : nullptr);
            int before = c.ptr;
            checkLazy(c.queryMonotone(x), got, [&] { return "scale queryMonotone equals query qi=" + show(qi) + " x=" + show(x); });
            moves += abs(c.ptr - before);
            ++qi;}
        ++checks;
        if (pattern < 2 && moves > 3 * lng(n + q) + 3) { fail("queryMonotone pointer moved " + show(moves) + " times for monotone x"); }}
    // n lines in a slope pattern (0 random, 1 ascending, 2 descending, 3 few distinct), q random queries interleaved, Li Chao oracle.
    template<bool MAX> void contScale(Rng &rng, int n, int q, int pattern) {
        context = string("LineContainer scale ") + (MAX ? "max" : "min") + " n=" + show(n) + " q=" + show(q) + " pattern=" + show(pattern);
        vector<lng> slopes(size_t(n), 0), xs = queryXs(rng, q, 2);
        for (auto &m : slopes) { m = pattern == 3 ? rnd(rng, -1000, 1000) : rnd(rng, -LCM, LCM); }
        if (pattern == 1 || pattern == 2) { sort(slopes.begin(), slopes.end()); }
        if (pattern == 2) { reverse(slopes.begin(), slopes.end()); }
        LiChao<lng, L> oracle(xs);
        LineContainer<MAX> c;
        vector<L> ls;
        hist = &ls;
        int qi = 0;
        while (int(ls.size()) < n || qi < q) {
            if (int(ls.size()) < n && (qi == q || rnd(rng, 0, n + q - 1) < n)) {
                lng m = slopes[ls.size()], b = rnd(rng, -LCB, LCB);
                c.addLine(m, b); ls.push_back({m, b});
                oracle.add(MAX ? L{-m, -b} : L{m, b});
                continue;}
            lng x = xs[size_t(qi)];
            auto [any, w] = oracle.get(x);
            lng got = std::as_const(c).query(x);
            auto where = [&] { return "query qi=" + show(qi) + " x=" + show(x); };
            if (!any) { checkLazy(got, LineContainer<MAX>::NONE, where); }
            else { checkLazy(got, narrow(MAX ? -w : w), where); }
            if (any && qi % max(q / 100, 1) == 0) { checkEqual(got, narrow(brute<MAX>(ls, x)), "brute qi=" + show(qi) + " x=" + show(x)); }
            ++qi;}}

    // Floating T: slopes, intercepts and x within FM; a quarter of the random runs use integers, so evaluation is exact and only the hull tests round.
    constexpr lng FM = 1000000;
    template<typename Z> Z qabs(Z v) { return v < 0 ? -v : v; }
    template<typename T> string showF(const vector<FL<T>> &ls) {
        string s = " lines(" + show(ls.size()) + ")=[";
        for (size_t i = 0; i < ls.size() && i < 40; ++i) { s += (i ? " " : "") + show(ls[i].m) + "x+" + show(ls[i].b); }
        return s + (ls.size() > 40 ? " ...]" : "]");}
    // Brute force over every line in Q<T>: {optimum, largest |m x| + |b|}; ls is nonempty.
    template<bool MAX, typename T> pair<Q<T>, Q<T>> fbrute(const vector<FL<T>> &ls, T x) {
        Q<T> res = 0, mag = 0;
        for (size_t i = 0; i < ls.size(); ++i) {
            Q<T> mx = Q<T>(ls[i].m) * Q<T>(x), v = mx + Q<T>(ls[i].b);
            if (i == 0 || (MAX ? v > res : v < res)) { res = v; }
            mag = max(mag, qabs(mx) + qabs(Q<T>(ls[i].b)));}
        return {res, mag};}
    // Tolerance 8 eps * (1 + mag): a rounded hull test drops only a line optimal on an interval of rounding width, so it and the rounded m x + b each err by about eps * mag (worst seen 0.98).
    template<typename T, typename F> void near(T got, pair<Q<T>, Q<T>> want, F where, const vector<FL<T>> &ls) {
        ++checks;
        Q<T> tol = Q<T>(8 * std::numeric_limits<T>::epsilon()) * (1 + want.second);
        if (!(qabs(Q<T>(got) - want.first) <= tol)) {
            fail("operation=" + where() + " expected~" + show((long double)want.first) + " actual=" + show(got) + " tol=" + show((long double)tol) + showF(ls));}}

    template<bool MAX, typename T> struct FMono {
        MonotoneCHT<MAX, T> c;
        vector<FL<T>> ls;
        T lo = 0, hi = 0;
        bool valid(T m) const { return ls.empty() || m <= lo || m >= hi; }
        void add(T m, T b) {
            lo = ls.empty() ? m : min(lo, m); hi = ls.empty() ? m : max(hi, m);
            c.addLine(m, b);
            ls.push_back({m, b});}
        // query, then queryMonotone: each value near the oracle (want, else brute) and its id's line evaluating in T to it exactly; rounding can
        // break unimodality, so the two may pick different near-optimal lines (the tie rule is exact for lng only).
        void query(T x, const char *op, const pair<Q<T>, Q<T>> *want = nullptr) {
            auto check = [&](pair<T, int> got, const char *kind) {
                auto where = [&] { return string(op) + " MonotoneCHT " + kind + " x=" + show(x); };
                if (ls.empty()) {
                    checkLazy(got, pair<T, int>(MonotoneCHT<MAX, T>::NONE, -1), where);
                    return;}
                near(got.first, want ? *want : fbrute<MAX>(ls, x), where, ls);
                auto [v, id] = got;
                ++checks;
                if (id < 0 || id >= int(ls.size()) || ls[size_t(id)].m * x + ls[size_t(id)].b != v) { fail("operation=" + where() + " line id=" + show(id) + " does not evaluate to " + show(v) + showF(ls)); }};
            check(std::as_const(c).query(x), "query");
            check(c.queryMonotone(x), "queryMonotone");}
    };
    template<bool MAX, typename T> struct FCont {
        LineContainer<MAX, T> c;
        vector<FL<T>> ls;
        void add(T m, T b) {
            c.addLine(m, b);
            ls.push_back({m, b});}
        void query(T x, const char *op, const pair<Q<T>, Q<T>> *want = nullptr) {
            T got = std::as_const(c).query(x);
            auto where = [&] { return string(op) + " LineContainer query x=" + show(x); };
            if (ls.empty()) { checkLazy(got, LineContainer<MAX, T>::NONE, where); }
            else { near(got, want ? *want : fbrute<MAX>(ls, x), where, ls); }}
    };

    // Uniform in [-M, M], or an integer there.
    template<typename T> T freal(Rng &rng, T M, bool integral) {
        if (integral) { return T(rnd(rng, -lng(M), lng(M))); }
        return M * (T(rng()) / T(std::numeric_limits<ulng>::max()) * 2 - 1);}
    // Next to e toward dir: 1e-12 to 1e-9 relative, or a few ulps.
    template<typename T> T beside(Rng &rng, T e, int dir) {
        if (rng() % 2) { return e + T(dir) * max(qabs(e), T(1)) * T(1e-12L) * T(rnd(rng, 1, 1000)); }
        for (lng i = rnd(rng, 1, 4); i > 0; --i) { e = std::nextafter(e, T(dir) * std::numeric_limits<T>::infinity()); }
        return e;}
    // A valid slope: an end slope, beside it (non-integral runs) or up to M / 2^k beyond it, clamped to M unless an end already passed M.
    template<bool MAX, typename T> T fmonoSlope(Rng &rng, const FMono<MAX, T> &h, T M, bool integral) {
        if (h.ls.empty()) { return freal(rng, M, integral); }
        bool up = rng() & 1;
        if (up && h.hi >= M) { up = false; }
        if (!up && h.lo <= -M) { up = true; }
        T e = up ? h.hi : h.lo, d = qabs(freal(rng, M / T(1LL << rnd(rng, 0, 12)), integral));
        int k = int(rng() % 8);
        if (k == 0) { return e; }
        if (k == 1 && !integral) { return beside(rng, e, up ? 1 : -1); }
        return up ? max(e, min(M, e + d)) : min(e, max(-M, e - d));}
    // Query x patterns as nextX: 0 increasing, 1 decreasing, 2 constant, 3 random, 4 alternating signs, 5 zigzag.
    template<typename T> T fnextX(Rng &rng, int pattern, T &cur, int k, T X, bool integral) {
        T d = qabs(freal(rng, X / 8, integral));
        if (pattern == 0 || (pattern == 5 && k / 16 % 2 == 0)) { cur = min(X, cur + d); }
        else if (pattern == 1 || pattern == 5) { cur = max(-X, cur - d); }
        else if (pattern == 3) { cur = freal(rng, X, integral); }
        else if (pattern == 4) { cur = T(k & 1 ? -1 : 1) * qabs(freal(rng, X, integral)); }
        return cur;}
    // An earlier line (mono: one with an end slope) repeated (1/2), or its slope with an intercept beside or random.
    template<typename T> FL<T> repeat(Rng &rng, const vector<FL<T>> &ls, T lo, T hi, bool mono, bool integral) {
        vector<size_t> ids;
        for (size_t i = 0; i < ls.size(); ++i) {
            if (!mono || ls[i].m == lo || ls[i].m == hi) { ids.push_back(i); }}
        FL<T> f = ls[ids[size_t(rnd(rng, 0, lng(ids.size()) - 1))]];
        int k = int(rng() % 4);
        if (k == 2) { f.b = integral ? f.b + T(rng() & 1 ? 1 : -1) : beside(rng, f.b, rng() & 1 ? 1 : -1); }
        if (k == 3) { f.b = freal(rng, T(FM), integral); }
        return f;}

    // Where two random stored lines cross (rounded to T), half the time moved by |p| 2^-k (k in [8, 64]), if inside [-X, X], else x: hull breakpoints and their rounding windows.
    template<typename T> T crossing(Rng &rng, const vector<FL<T>> &ls, T x, T X) {
        if (ls.size() < 2) { return x; }
        const FL<T> &f = ls[size_t(rnd(rng, 0, lng(ls.size()) - 1))], &g = ls[size_t(rnd(rng, 0, lng(ls.size()) - 1))];
        if (f.m == g.m) { return x; }
        Q<T> p = (Q<T>(g.b) - Q<T>(f.b)) / (Q<T>(f.m) - Q<T>(g.m));
        if (rng() % 2) { p += (rng() & 1 ? 1 : -1) * qabs(p) / Q<T>(ulll(1) << rnd(rng, 8, 64)); }
        return qabs(p) <= Q<T>(X) ? T(p) : x;}

    // An intercept sending slope m through the crossing of two stored lines, half the time missing it by (|y| + 1) 2^-k (k in [8, 64]); b if none fits FM.
    template<typename T> T through(Rng &rng, const vector<FL<T>> &ls, T m, T b) {
        if (ls.size() < 2) { return b; }
        const FL<T> &f = ls[size_t(rnd(rng, 0, lng(ls.size()) - 1))], &g = ls[size_t(rnd(rng, 0, lng(ls.size()) - 1))];
        if (f.m == g.m) { return b; }
        Q<T> p = (Q<T>(g.b) - Q<T>(f.b)) / (Q<T>(f.m) - Q<T>(g.m)), y = Q<T>(f.m) * p + Q<T>(f.b), c = y - Q<T>(m) * p;
        if (rng() % 2) { c += (rng() & 1 ? 1 : -1) * (qabs(y) + 1) / Q<T>(ulll(1) << rnd(rng, 8, 64)); }
        return qabs(c) <= Q<T>(FM) ? T(c) : b;}

    template<bool MAX, typename T> void fedges(const string &name) {
        context = "floating edges " + name;
        checkEqual(MonotoneCHT<MAX, T>::NONE, MAX ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max(), "MonotoneCHT NONE");
        checkEqual(LineContainer<MAX, T>::NONE, MAX ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max(), "LineContainer NONE");
        FMono<MAX, T> e; FCont<MAX, T> f;
        for (T x : {T(0), T(-FM), T(FM), T(0.5L)}) { e.query(x, "empty"); f.query(x, "empty"); }
        // y = x and y = -x tie at 0; the front-most line is the larger slope for min, the smaller for max.
        for (int r = 0; r < 2; ++r) {
            FMono<MAX, T> h; FCont<MAX, T> g;
            h.add(r ? -1 : 1, 0); h.add(r ? 1 : -1, 0); g.add(1, 0); g.add(-1, 0);
            checkEqual(std::as_const(h.c).query(0), pair<T, int>(0, MAX != bool(r) ? 1 : 0), "y=x, y=-x query x=0");
            for (T x : {T(-3), T(0), T(3), T(0), T(-0.25L), T(0), T(0.25L), T(0)}) { h.query(x, "y=x, y=-x"); g.query(x, "y=x, y=-x"); }}}

    // Random histories with interleaved queries: MonotoneCHT slopes at either end, LineContainer slopes anywhere (1/7 beside an earlier one); 1/8 repeats.
    // x within X = FM, 1000 or 1 (small |m x| + |b| tightens the tolerance); 1/4 of the new lines through, and 1/4 of the queries at, a crossing of two stored lines.
    template<bool MAX, typename T> void frandom(Rng &rng, int runs, int maxN, const string &name) {
        const T M = T(FM);
        for (int r = 0; r < runs; ++r) {
            bool integral = r % 4 == 0;
            int n = int(rnd(rng, 1, maxN)), pattern = int(rnd(rng, 0, 5)), qs = 0;
            T X = r % 3 == 0 ? M : r % 3 == 1 ? T(1000) : T(1), cur = pattern == 0 ? -X : pattern == 1 ? X : freal(rng, X, integral);
            context = "floating random " + name + " run=" + show(r) + " pattern=" + show(pattern) + " integral=" + show(integral) + " X=" + show(X);
            FMono<MAX, T> h; FCont<MAX, T> g;
            h.query(cur, "start"); g.query(cur, "start");
            while (int(h.ls.size()) < n || qs < 2 * n) {
                if (int(h.ls.size()) < n && (qs >= 2 * n || rng() % 3 == 0)) {
                    if (!h.ls.empty() && rng() % 8 == 0) {
                        FL<T> f = repeat(rng, h.ls, h.lo, h.hi, true, integral);
                        h.add(f.m, f.b);}
                    else {
                        T m = fmonoSlope(rng, h, M, integral), b = freal(rng, M, integral);
                        h.add(m, rng() % 4 || integral ? b : through(rng, h.ls, m, b));}
                    if (!g.ls.empty() && rng() % 8 == 0) {
                        FL<T> f = repeat(rng, g.ls, T(0), T(0), false, integral);
                        g.add(f.m, f.b);}
                    else if (!g.ls.empty() && !integral && rng() % 7 == 0) { g.add(beside(rng, g.ls[size_t(rnd(rng, 0, lng(g.ls.size()) - 1))].m, rng() & 1 ? 1 : -1), freal(rng, M, integral)); }
                    else {
                        T m = freal(rng, M, integral), b = freal(rng, M, integral);
                        g.add(m, rng() % 4 || integral ? b : through(rng, g.ls, m, b));}}
                else {
                    T x = fnextX(rng, pattern, cur, qs++, X, integral);
                    if (rng() % 4 == 0) { x = crossing(rng, rng() & 1 ? h.ls : g.ls, x, X); }
                    h.query(x, "random"); g.query(x, "random");}}}}

    // n lines and q queries interleaved (x: 0 increasing, 1 decreasing, 2 random); __float128 Li Chao oracle on every query, brute on a sample.
    // MonotoneCHT grows a sorted slope list (1/8 equal or beside the previous) at both ends; LineContainer takes random slopes.
    template<bool MAX, typename T> void fscale(Rng &rng, bool mono, int n, int q, int pattern, const string &name) {
        context = string("floating scale ") + (mono ? "MonotoneCHT " : "LineContainer ") + name + " n=" + show(n) + " q=" + show(q) + " pattern=" + show(pattern);
        const T M = T(FM);
        vector<T> slopes(size_t(n), T(0)), xs(size_t(q), T(0));
        for (auto &m : slopes) { m = freal(rng, M, false); }
        if (mono) {
            sort(slopes.begin(), slopes.end());
            for (size_t i = 1; i < slopes.size(); ++i) {
                if (rng() % 8 == 0) { slopes[i] = rng() & 1 ? slopes[i - 1] : beside(rng, slopes[i - 1], 1); }}
            sort(slopes.begin(), slopes.end());}
        for (auto &x : xs) { x = freal(rng, M, false); }
        if (pattern < 2) { sort(xs.begin(), xs.end()); }
        if (pattern == 1) { reverse(xs.begin(), xs.end()); }
        LiChao<T, FL<T>> oracle(xs);
        FMono<MAX, T> h; FCont<MAX, T> g;
        vector<FL<T>> &ls = mono ? h.ls : g.ls;
        int lft = int(rnd(rng, 0, n - 1)), rgt = lft - 1, qi = 0;
        while (int(ls.size()) < n || qi < q) {
            if (int(ls.size()) < n && (qi == q || rnd(rng, 0, n + q - 1) < n)) {
                bool left = lft > 0 && (rgt == n - 1 || (rng() & 1));
                T m = !mono ? slopes[ls.size()] : left ? slopes[size_t(--lft)] : slopes[size_t(++rgt)], b = freal(rng, M, false);
                if (mono) { h.add(m, b); }
                else { g.add(m, b); }
                oracle.add(MAX ? FL<T>{-m, -b} : FL<T>{m, b});
                continue;}
            T x = xs[size_t(qi)];
            auto [any, w] = oracle.get(x);
            // mag bounds |m x| + |b| over every line in the box.
            pair<Q<T>, Q<T>> want{Q<T>(MAX ? -w : w), Q<T>(M) * qabs(Q<T>(x)) + Q<T>(M)};
            if (any && qi % max(q / 100, 1) == 0) {
                auto bf = fbrute<MAX>(ls, x);
                ++checks;
                if (qabs(want.first - bf.first) > Q<T>(1e-15L) * (1 + bf.second)) { fail("Li Chao oracle vs brute qi=" + show(qi) + " x=" + show(x) + " oracle=" + show((long double)want.first) + " brute=" + show((long double)bf.first)); }}
            if (mono) { h.query(x, "scale", any ? &want : nullptr); }
            else { g.query(x, "scale", any ? &want : nullptr); }
            ++qi;}}

    template<typename F> void timed(const string &name, F body) {
        lng before = checks;
        auto start = std::chrono::steady_clock::now();
        body();
        hist = nullptr;
        lng ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        std::cout << "PASS " << name << " checks=" << checks - before << " ms=" << ms << '\n' << std::flush;}
    template<bool MAX, typename T> void fsuite(Rng &rng, const string &type, int runs, int maxN, int scale) {
        string name = string(MAX ? "max " : "min ") + type;
        timed(name + " floating edges", [&] { fedges<MAX, T>(name); });
        timed(name + " floating random", [&] { frandom<MAX, T>(rng, runs, maxN, name); });
        timed(name + " floating scale", [&] {
            for (int pattern = 0; pattern < 3; ++pattern) { fscale<MAX, T>(rng, true, scale, scale, pattern, name); }
            fscale<MAX, T>(rng, false, scale, scale, 2, name);});}

    template<bool MAX> void suite(Rng &rng, int scale, lng box, int monoDepth, int contDepth, int runs, int bigN) {
        auto step = [&](const string &name, auto body) { timed(string(MAX ? "max " : "min ") + name, body); };
        step("edges", [&] { edges<MAX>(); });
        step("ties", [&] { ties<MAX>(); });
        step("crossings", [&] { crossings<MAX>(rng, runs); });
        step("concurrent", [&] { concurrent<MAX>(); });
        step("extremes", [&] { extremes<MAX>(rng, runs); });
        step("boundary", [&] { boundary<MAX>(); });
        step("exhaustive", [&] { exhaustive<MAX>(box, monoDepth, contDepth); });
        step("MonotoneCHT random", [&] {
            monoRandom<MAX>(rng, runs, 12, 6, 20, 20, "small");
            monoRandom<MAX>(rng, runs / 4, bigN, LCM, LCB, LCM, "library-checker");
            monoRandom<MAX>(rng, runs / 4, bigN, LIM, LIM, 1, "extreme");});
        step("LineContainer random", [&] {
            contRandom<MAX>(rng, runs, 12, 6, 20, 20, "small");
            contRandom<MAX>(rng, runs / 4, bigN, LCM, LCB, LCM, "library-checker");
            contRandom<MAX>(rng, runs / 4, bigN, LIM, LIM, 1, "extreme");});
        step("MonotoneCHT scale", [&] {
            for (int pattern = 0; pattern < 4; ++pattern) { monoScale<MAX>(rng, scale, scale, pattern % 2 ? 1000 : LCM, pattern); }});
        step("LineContainer scale", [&] {
            for (int pattern = 0; pattern < 4; ++pattern) { contScale<MAX>(rng, scale, scale, pattern); }});}

    void invalid(const string &name) {
        MonotoneCHT<false> c; MonotoneCHT<true> cm; LineContainer<false> l; LineContainer<true> lm;
        if (name == "monotone-slope-high") { c.addLine(1LL << 62, 0); }
        else if (name == "monotone-slope-low") { c.addLine(-(1LL << 62), 0); }
        else if (name == "monotone-intercept-high") { c.addLine(0, 1LL << 62); }
        else if (name == "monotone-intercept-low") { c.addLine(0, -(1LL << 62)); }
        else if (name == "monotone-inner-slope") { c.addLine(0, 0); c.addLine(2, 0); c.addLine(1, 0); }
        else if (name == "monotone-max-slope-high") { cm.addLine(1LL << 62, 0); }
        else if (name == "monotone-max-inner-slope") { cm.addLine(0, 0); cm.addLine(-2, 5); cm.addLine(-1, -100); }
        else if (name == "container-slope-high") { l.addLine(1LL << 62, 0); }
        else if (name == "container-slope-low") { l.addLine(-(1LL << 62), 0); }
        else if (name == "container-intercept-high") { l.addLine(0, 1LL << 62); }
        else if (name == "container-intercept-low") { l.addLine(0, -(1LL << 62)); }
        else if (name == "container-max-slope-low") { lm.addLine(-(1LL << 62), 0); }
        else if (name == "monotone-float-slope-huge") { MonotoneCHT<false, double> f; f.addLine(1e200, 0); }
        else if (name == "monotone-float-intercept-tiny") { MonotoneCHT<true, double> f; f.addLine(0, 1e-200); }
        else if (name == "container-float-slope-huge") { LineContainer<false, long double> f; f.addLine(1e1300L, 0); }
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
        std::cout << "convexhulltrick seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        bool quick = mode == "quick", full = mode == "full";
        // Stress keeps the checked -O0 debug build under the runner's 180 s timeout; its exhaustive part uses a larger box at depth 3.
        int scale = quick ? 3000 : full ? 200000 : 300000, monoDepth = quick ? 3 : full ? 4 : 3, contDepth = quick ? 2 : 3;
        int runs = quick ? 300 : full ? 5000 : 6000, bigN = quick ? 40 : full ? 200 : 220;
        lng box = quick || full ? 2 : 3;
        suite<false>(rng, scale, box, monoDepth, contDepth, runs, bigN);
        suite<true>(rng, scale, box, monoDepth, contDepth, runs, bigN);
        // Floating T: quick 40 runs of up to 30 lines, scale 3000; full 400 runs of up to 80 lines, scale 1e5.
        int fruns = quick ? 40 : full ? 400 : 800, fmaxN = quick ? 30 : full ? 80 : 120, fscaleN = quick ? 3000 : full ? 100000 : 150000;
        fsuite<false, double>(rng, "double", fruns, fmaxN, fscaleN); fsuite<true, double>(rng, "double", fruns, fmaxN, fscaleN);
        fsuite<false, long double>(rng, "long double", fruns, fmaxN, fscaleN); fsuite<true, long double>(rng, "long double", fruns, fmaxN, fscaleN);
        std::cout << "PASS convexhulltrick checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL convexhulltrick seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
