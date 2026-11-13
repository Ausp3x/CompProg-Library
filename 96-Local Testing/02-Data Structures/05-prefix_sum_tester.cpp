#include "../../02-Data Structures/05-prefix_sum.hpp"
#include "../../02-Data Structures/00-monoids.hpp"

ulng seed = 20260927;
string mode = "full", context, history;
lng checks = 0;

string show(lll x) {
    bool negative = x < 0; ulll u = negative ? ulll(-(x + 1)) + 1 : ulll(x);
    string s;
    do { s.push_back(char('0' + u % 10)); u /= 10; } while (u);
    if (negative) { s.push_back('-'); } reverse(s.begin(), s.end()); return s;}
template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
template<typename T, size_t N> string show(const array<T, N> &v) {
    string s = "<";
    for (const auto &x : v) { s += show(x) + ','; } return s + '>';}
template<typename T> string show(const vector<T> &v) {
    string s = "[";
    for (const auto &x : v) { s += show(x) + ','; } return s + ']';}
template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
    ++checks;
    if (got != want) {
        throw std::runtime_error("case=" + context + " history=" + history + " operation=" + op
                                 + " expected=" + show(want) + " actual=" + show(got));}}
template<typename T> T sum(const vector<T> &a, int l, int r) {
    T ans = T(0); for (int i = l; i < r; ++i) { ans += a[i]; } return ans;}
template<typename T> T sum(const vector<vector<T>> &a, int x1, int y1, int x2, int y2) {
    T ans = T(0);
    for (int i = x1; i < x2; ++i) { for (int j = y1; j < y2; ++j) { ans += a[i][j]; }}
    return ans;}
using V3 = vector<vector<vector<lng>>>;
template<typename T> T sum3(const vector<vector<vector<T>>> &a, int x1, int y1, int z1, int x2, int y2, int z2) {
    T ans = T(0);
    for (int x = x1; x < x2; ++x) { for (int y = y1; y < y2; ++y) { for (int z = z1; z < z2; ++z) { ans += a[x][y][z]; }}}
    return ans;}
template<typename T> void add3(vector<vector<vector<T>>> &a, int x1, int y1, int z1, int x2, int y2, int z2, T w) {
    for (int x = x1; x < x2; ++x) { for (int y = y1; y < y2; ++y) { for (int z = z1; z < z2; ++z) { a[x][y][z] += w; }}}}
template<typename T> void addArithmetic(vector<T> &a, int l, int r, T x, T step) {
    for (int i = l; i < r; ++i) { a[i] += x + T(i - l) * step; }}
V3 zeros3(int n, int m, int k) { return V3(n, vector<vector<lng>>(m, vector<lng>(k))); }
// A nested vector cannot carry k without rows or m without planes.
bool expressible(int n, int m, int k) { return (n || (!m && !k)) && (m || !k); }
void check1D(const vector<lng> &a) {
    context = "array=" + show(a); int n = int(a.size()); PrefixSum<lng> p(a); DifferenceArray<lng> d(a);
    checkEqual(p.n, n, "size"); checkEqual(d.n, n, "difference size");
    checkEqual(d.values(), a, "difference inverse"); checkEqual(d.values(), a, "repeat values");
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
        checkEqual(p.sum(l, r), sum(a, l, r), "sum(" + show(l) + ',' + show(r) + ')');
        if (!l) { checkEqual(p.prefixSum(r), sum(a, 0, r), "prefix"); }
        for (lng x : {-2, 0, 2}) {
            auto b = a; auto copy = d; copy.add(l, r, x);
            for (int i = l; i < r; ++i) { b[i] += x; }
            checkEqual(copy.values(), b, "range add(" + show(l) + ',' + show(r) + ',' + show(x) + ')');}}}
    auto copy = p; auto moved = std::move(copy);
    checkEqual(moved.sum(0, n), sum(a, 0, n), "prefix copy/move");
    p = PrefixSum<lng>({4, -3}); checkEqual(p.sum(0, 2), lng(1), "prefix reconstruction");
    checkEqual(moved.sum(0, n), sum(a, 0, n), "copy independence");
    auto dm = std::move(d); checkEqual(dm.values(), a, "difference move");
    d = DifferenceArray<lng>({1, -2}); d.clear(); checkEqual(d.values(), vector<lng>({0, 0}), "difference clear/reconstruction");}
void arrays() {
    int limit = mode == "quick" ? 4 : mode == "full" ? 7 : 8;
    for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
        for (int code = 0; code < count; ++code) {
            vector<lng> a(n); int x = code;
            for (lng &v : a) { v = x % 3 - 1; x /= 3; }
            history.clear(); check1D(a);}}
    int depth = mode == "quick" ? 2 : mode == "full" ? 3 : 4;
    auto dfs = [&] (auto &&self, DifferenceArray<lng> d, vector<lng> a, int left) -> void {
        context = "exhaustive n=3 zero initial"; checkEqual(d.values(), a, "history values");
        if (!left) { return; }
        for (int l = 0; l <= 3; ++l) { for (int r = l; r <= 3; ++r) { for (lng x : {-1, 1}) {
            auto b = a; auto copy = d; copy.add(l, r, x);
            for (int i = l; i < r; ++i) { b[i] += x; }
            string old = history; history += '(' + show(l) + ',' + show(r) + ',' + show(x) + ')';
            self(self, std::move(copy), std::move(b), left - 1); history = old;}}}};
    dfs(dfs, DifferenceArray<lng>(3), vector<lng>(3), depth);
    cout << "PASS 1D ternary arrays n<=" << limit << ", update histories depth=" << depth << '\n';}
void check2D(const vector<vector<lng>> &a, bool updates) {
    context = "matrix=" + show(a); int n = int(a.size()), m = n ? int(a[0].size()) : 0;
    PrefixSum2D<lng> p(a); DifferenceArray2D<lng> d(a);
    checkEqual(p.n, n, "rows"); checkEqual(p.m, m, "columns"); checkEqual(d.values(), a, "2D inverse");
    for (int x1 = 0; x1 <= n; ++x1) { for (int x2 = x1; x2 <= n; ++x2) {
        for (int y1 = 0; y1 <= m; ++y1) { for (int y2 = y1; y2 <= m; ++y2) {
            string rect = '(' + show(x1) + ',' + show(y1) + ',' + show(x2) + ',' + show(y2) + ')';
            checkEqual(p.sum(x1, y1, x2, y2), sum(a, x1, y1, x2, y2), "rectangle sum" + rect);
            if (!x1 && !y1) { checkEqual(p.prefixSum(x2, y2), sum(a, 0, 0, x2, y2), "2D prefix"); }
            if (updates) {
                for (lng value : {-1, 1}) {
                    auto b = a; auto copy = d; copy.add(x1, y1, x2, y2, value);
                    for (int i = x1; i < x2; ++i) { for (int j = y1; j < y2; ++j) { b[i][j] += value; }}
                    checkEqual(copy.values(), b, "rectangle add" + rect + " delta=" + show(value));}}}}}}
    auto copy = p; auto moved = std::move(copy);
    p = PrefixSum2D<lng>(vector<vector<lng>>{{1, 2}, {-4, 5}}); checkEqual(p.sum(0, 0, 2, 2), lng(4), "2D reconstruction");
    checkEqual(moved.sum(0, 0, n, m), sum(a, 0, 0, n, m), "2D copy independence/move");
    auto dm = std::move(d); checkEqual(dm.values(), a, "2D move");
    d = DifferenceArray2D<lng>(vector<vector<lng>>{{3}, {-2}}); d.clear(); checkEqual(d.values(), vector<vector<lng>>({{0}, {0}}), "2D clear/reconstruction");}
void matrices() {
    for (int n = 0; n <= 2; ++n) { for (int m = 0; m <= (mode == "quick" ? 2 : 3); ++m) {
        int count = 1; for (int i = 0; i < n * m; ++i) { count *= 3; }
        for (int code = 0; code < count; ++code) {
            vector<vector<lng>> a(n, vector<lng>(m)); int x = code;
            for (auto &row : a) { for (lng &v : row) { v = x % 3 - 1; x /= 3; }}
            check2D(a, true);}}}
    cout << "PASS exhaustive rectangular matrices, all empty/border/interior sums and updates\n";}
void randomized() {
    std::mt19937_64 rng(seed); int trials = mode == "quick" ? 15 : mode == "full" ? 100 : 400;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng() % 14), m = int(rng() % 13); vector<vector<lng>> a(n, vector<lng>(m));
        for (auto &row : a) { for (lng &x : row) { x = lng(rng() % 101) - 50; }}
        DifferenceArray2D<lng> d(a); history.clear(); context = "random initial=" + show(a);
        for (int op = 0; op < 120; ++op) {
            int x1 = int(rng() % (n + 1)), x2 = int(rng() % (n + 1));
            int y1 = int(rng() % (m + 1)), y2 = int(rng() % (m + 1));
            if (x1 > x2) { swap(x1, x2); } if (y1 > y2) { swap(y1, y2); }
            // A vector with no rows carries no column width; preserve it explicitly.
            if (!n && d.m != m) { d = DifferenceArray2D<lng>(n, m); }
            lng x = lng(rng() % 101) - 50;
            history += '(' + show(x1) + ',' + show(y1) + ',' + show(x2) + ',' + show(y2) + ',' + show(x) + ')';
            d.add(x1, y1, x2, y2, x);
            for (int i = x1; i < x2; ++i) { for (int j = y1; j < y2; ++j) { a[i][j] += x; }}
            checkEqual(d.values(), a, "random imos"); checkEqual(d.values(), a, "repeated materialization");
            PrefixSum2D<lng> p(a);
            if (!n) { p = PrefixSum2D<lng>(n, m); }
            checkEqual(p.sum(x1, y1, x2, y2), sum(a, x1, y1, x2, y2), "random rectangle");
            if (op % 17 == 0 && n) { d = DifferenceArray2D<lng>(a); }
            if (op % 31 == 0) { auto copy = d; copy.clear(); checkEqual(d.values(), a, "copy independence"); }}}
    cout << "PASS seeded rectangle histories/reconstruction/materialization trials=" << trials << '\n';}
void boundaries() {
    context = "types/empty dimensions/aliasing"; history.clear();
    PrefixSum<lng> p(vector<lng>{LLONG_MIN, 0, LLONG_MAX});
    checkEqual(p.sum(0, 3), lng(-1), "int64 cancellation"); checkEqual(p.sum(1, 3), lng(LLONG_MAX), "int64 max");
    DifferenceArray<lng> d(1); d.add(0, 1, LLONG_MIN); checkEqual(d.values()[0], lng(LLONG_MIN), "no unused negative border");
    DifferenceArray2D<lng> dm(1, 1); dm.add(0, 0, 1, 1, LLONG_MIN);
    checkEqual(dm.values()[0][0], lng(LLONG_MIN), "2D no unused negative border");
    lll big = lll(1) << 100;
    PrefixSum<lll> huge(vector<lll>{big, big, -big}); checkEqual(huge.sum(0, 3), big, "128-bit prefix");
    DifferenceArray2D<lll> hd(2, 2); hd.add(0, 0, 2, 2, big); PrefixSum2D<lll> hp(hd.values());
    checkEqual(hp.sum(0, 0, 2, 2), 4 * big, "128-bit matrix");
    DifferenceArray<uint> ud(vector<uint>{UINT_MAX, 0, 2}); ud.add(0, 2, 1);
    checkEqual(ud.values(), vector<uint>({0, 1, 2}), "modular differences");
    PrefixSum<uint> up(ud.values()); checkEqual(up.sum(0, 3), uint(3), "modular prefix");
    DifferenceArray2D<uint> um(vector<vector<uint>>{{UINT_MAX, 0}, {1, UINT_MAX}});
    um.add(0, 0, 2, 2, 1); PrefixSum2D<uint> ump(um.values());
    checkEqual(ump.sum(0, 0, 2, 2), uint(3), "modular 2D");
    PrefixSum<double> fp(vector<double>{0.5, 1.25, -0.25}); checkEqual(fp.sum(0, 3), 1.5, "exact dyadic prefix");
    for (auto [n, m] : vector<pair<int,int>>{{0, 0}, {0, 7}, {7, 0}}) {
        PrefixSum2D<lng> z(n, m); DifferenceArray2D<lng> zd(n, m);
        zd.add(0, 0, n, m, 7); checkEqual(z.sum(0, 0, n, m), lng(0), "empty 2D query");
        checkEqual(zd.values(), vector<vector<lng>>(n, vector<lng>(m)), "empty 2D output");}
    DifferenceArray<lng> alias(vector<lng>{2, 3, 4}); alias.add(0, 2, alias.d[0]);
    checkEqual(alias.values(), vector<lng>({4, 5, 4}), "delta alias");
    alias = DifferenceArray<lng>(alias.d); checkEqual(alias.values(), vector<lng>({4, 1, -1}), "reconstruction from own storage");
    DifferenceArray2D<lng> ad(vector<vector<lng>>{{2, 3}, {4, 6}});
    ad.add(0, 0, 2, 2, ad.d[0][0]); checkEqual(ad.values(), vector<vector<lng>>({{4, 5}, {6, 8}}), "2D delta alias");
    auto before = ad.d; ad = DifferenceArray2D<lng>(ad.d); checkEqual(ad.values(), before, "2D reconstruction from own storage");
    PrefixSum<lng> ap(vector<lng>{1, 2}); auto old = ap.p; ap = PrefixSum<lng>(ap.p);
    checkEqual(ap.sum(0, 3), sum(old, 0, 3), "prefix reconstruction from own storage");
    PrefixSum<lng> single({7}), pair{7, 8}; DifferenceArray<lng> dsingle({7}), dpair{7, 8};
    checkEqual(single.n, 1, "braced singleton prefix size"); checkEqual(single.sum(0, 1), lng(7), "braced singleton prefix");
    checkEqual(pair.sum(0, 2), lng(15), "braced prefix list");
    checkEqual(dsingle.values(), vector<lng>({7}), "braced singleton difference"); checkEqual(dpair.values(), vector<lng>({7, 8}), "braced difference list");
    static_assert(!std::is_convertible_v<int, PrefixSum<lng>> && !std::is_convertible_v<int, DifferenceArray<lng>>);
    cout << "PASS wide/modular/exact dyadic types, empty dimensions, endpoint and alias regressions\n";}
void arithmeticHistories() {
    int limit = mode == "quick" ? 4 : 6, depth = mode == "quick" ? 2 : 3; lng nodes = 0;
    const vector<pair<lng, lng>> deltas = {{1, -1}, {-2, 1}, {0, 2}};
    auto dfs = [&](auto &&self, const DifferenceArray<lng> &d, const vector<lng> &a, int left) -> void {
        ++nodes; checkEqual(d.values(), a, "arithmetic history values");
        if (!left) { return; }
        int n = d.n; string old = history;
        auto go = [&](const DifferenceArray<lng> &next, const vector<lng> &b, const string &op) {
            history = old + op; self(self, next, b, left - 1); history = old;};
        auto copy = d; copy.clear(); checkEqual(copy.s.empty(), true, "clear resets slopes");
        go(copy, vector<lng>(n), "C");
        for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
            string range = show(l) + ',' + show(r);
            copy = d; copy.add(l, r, -1); auto b = a;
            for (int i = l; i < r; ++i) { b[i] -= 1; }
            go(copy, b, "A(" + range + ",-1)");
            for (auto [x, step] : deltas) {
                copy = d; copy.addArithmetic(l, r, x, step); b = a; addArithmetic(b, l, r, x, step);
                if (l < r) { checkEqual(copy.s.size(), size_t(n), "slope allocation"); }
                go(copy, b, "S(" + range + ',' + show(x) + ',' + show(step) + ')');}}}};
    for (int n = 0; n <= limit; ++n) {
        vector<lng> a(n);
        for (int i = 0; i < n; ++i) { a[i] = i % 3 - 1; }
        context = "arithmetic exhaustive n=" + show(n) + " initial=" + show(a); history.clear();
        DifferenceArray<lng> d(a); checkEqual(d.s.empty(), true, "no slopes after construction");
        dfs(dfs, d, a, depth);}
    cout << "PASS arithmetic add/addArithmetic/clear/values histories n<=" << limit << " depth=" << depth << " nodes=" << nodes << '\n';}
template<typename T> void randomArithmetic(std::mt19937_64 &rng, int trials, auto value, const string &type) {
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng() % 41); vector<T> a(n);
        for (T &x : a) { x = value(); }
        DifferenceArray<T> d(a); history.clear(); context = "random arithmetic " + type + " initial=" + show(a);
        for (int op = 0; op < 150; ++op) {
            int l = int(rng() % ulng(n + 1)), r = int(rng() % ulng(n + 1)), kind = int(rng() % 8);
            if (l > r) { swap(l, r); }
            T x = value(), step = value();
            if (kind == 0) { d.clear(); fill(a.begin(), a.end(), T(0)); history += 'C'; }
            else if (kind < 3) {
                d.add(l, r, x);
                for (int i = l; i < r; ++i) { a[i] += x; }
                history += "A(" + show(l) + ',' + show(r) + ',' + show(x) + ')';}
            else {
                d.addArithmetic(l, r, x, step); addArithmetic(a, l, r, x, step);
                history += "S(" + show(l) + ',' + show(r) + ',' + show(x) + ',' + show(step) + ')';}
            checkEqual(d.values(), a, "random arithmetic");
            if (op % 37 == 0) { auto copy = d; copy.addArithmetic(0, n, x, step); checkEqual(d.values(), a, "arithmetic copy independence"); }}}}
void arithmeticRandom() {
    std::mt19937_64 rng(seed + 1); int trials = mode == "quick" ? 15 : mode == "full" ? 100 : 400;
    randomArithmetic<lng>(rng, trials, [&] { return lng(rng() % 2000001) - 1000000; }, "int64");
    randomArithmetic<lll>(rng, trials, [&] { return lll(lng(rng())) * 65536; }, "int128");
    randomArithmetic<uint>(rng, trials, [&] { return uint(rng()); }, "uint32");
    cout << "PASS seeded arithmetic histories int64/int128/uint32 n<=40 ops=150 trials=" << trials << '\n';}
void arithmeticBoundaries() {
    context = "arithmetic types/extremes/aliasing"; history.clear();
    DifferenceArray<lng> d(3);
    d.addArithmetic(0, 3, LLONG_MIN, 1); checkEqual(d.values(), vector<lng>({LLONG_MIN, LLONG_MIN + 1, LLONG_MIN + 2}), "int64 min start");
    d.clear(); d.addArithmetic(0, 3, LLONG_MAX, -1); checkEqual(d.values(), vector<lng>({LLONG_MAX, LLONG_MAX - 1, LLONG_MAX - 2}), "int64 max start");
    d.clear(); d.addArithmetic(0, 2, LLONG_MAX - 5, 5); checkEqual(d.values(), vector<lng>({LLONG_MAX - 5, LLONG_MAX, 0}), "int64 max end before border");
    d = DifferenceArray<lng>(2); d.addArithmetic(0, 2, LLONG_MAX, LLONG_MIN); checkEqual(d.values(), vector<lng>({LLONG_MAX, -1}), "int64 min step");
    d = DifferenceArray<lng>(1); d.addArithmetic(0, 1, LLONG_MIN, LLONG_MIN); checkEqual(d.values(), vector<lng>({LLONG_MIN}), "singleton unused step");
    d = DifferenceArray<lng>(vector<lng>{1, 2});
    d.addArithmetic(0, 0, 5, 5); d.addArithmetic(1, 1, 5, 5); d.addArithmetic(2, 2, 5, 5);
    checkEqual(d.values(), vector<lng>({1, 2}), "empty arithmetic ranges");
    DifferenceArray<lng> none; none.addArithmetic(0, 0, 1, 1); checkEqual(none.values(), vector<lng>(), "empty array arithmetic");
    lll big = lll(1) << 100;
    DifferenceArray<lll> hd(3); hd.addArithmetic(0, 3, big, -big); checkEqual(hd.values(), vector<lll>({big, 0, -big}), "128-bit descending");
    DifferenceArray<lll> h4(4); h4.addArithmetic(1, 3, big, big); checkEqual(h4.values(), vector<lll>({0, big, 2 * big, 0}), "128-bit interior");
    DifferenceArray<uint> ud(vector<uint>{UINT_MAX, 0, 2}); ud.addArithmetic(0, 3, 1, UINT_MAX);
    checkEqual(ud.values(), vector<uint>({0, 0, 1}), "modular arithmetic");
    ud.addArithmetic(0, 2, 3, UINT_MAX); checkEqual(ud.values(), vector<uint>({3, 2, 1}), "modular arithmetic border");
    ud.add(1, 3, UINT_MAX); checkEqual(ud.values(), vector<uint>({3, 1, 0}), "modular mixed add");
    DifferenceArray<lng> alias({2, 3, 4}); alias.addArithmetic(0, 3, alias.d[0], alias.d[1]);
    checkEqual(alias.values(), vector<lng>({4, 6, 8}), "arithmetic delta alias");
    alias.addArithmetic(0, 2, alias.s[1], alias.d[0]); checkEqual(alias.values(), vector<lng>({5, 11, 8}), "arithmetic slope alias");
    alias.clear(); checkEqual(alias.s.empty(), true, "clear resets slopes"); checkEqual(alias.values(), vector<lng>({0, 0, 0}), "arithmetic clear");
    alias.addArithmetic(1, 3, 2, 3); checkEqual(alias.s.size(), size_t(3), "slopes after clear");
    checkEqual(alias.values(), vector<lng>({0, 2, 5}), "arithmetic after clear");
    auto copy = alias; copy.addArithmetic(0, 3, 1, 1); checkEqual(alias.values(), vector<lng>({0, 2, 5}), "arithmetic copy independence");
    auto moved = std::move(copy); checkEqual(moved.values(), vector<lng>({1, 4, 8}), "arithmetic move");
    DifferenceArray<lng> braced{1, 2}; braced.addArithmetic(0, 2, 1, 1); checkEqual(braced.values(), vector<lng>({2, 4}), "braced arithmetic");
    cout << "PASS arithmetic int64 extremes, 128-bit, modular, empty ranges, alias/clear/copy regressions\n";}
// Permutations of 4 points composed left to right: a noncommutative group.
struct Perm {
    using S = array<int, 4>;
    static S op(const S &a, const S &b) { S c{}; for (int i = 0; i < 4; ++i) { c[i] = b[a[i]]; } return c; }
    static S e() { return {0, 1, 2, 3}; }
    static S inv(const S &a) { S c{}; for (int i = 0; i < 4; ++i) { c[a[i]] = i; } return c; }
};
// Invertible 2x2 matrices modulo a runtime prime p: a stateful noncommutative group.
struct Matrix2 {
    using S = array<ulng, 4>;
    ulng p = 7;
    S op(const S &a, const S &b) const {
        return {(a[0] * b[0] + a[1] * b[2]) % p, (a[0] * b[1] + a[1] * b[3]) % p, (a[2] * b[0] + a[3] * b[2]) % p, (a[2] * b[1] + a[3] * b[3]) % p};}
    S e() const { return {1, 0, 0, 1}; }
    ulng det(const S &a) const { return (a[0] * a[3] % p + p - a[1] * a[2] % p) % p; }
    S inv(const S &a) const {
        ulng x = det(a), r = 1;
        for (ulng b = p - 2; b; b >>= 1, x = x * x % p) { if (b & 1) { r = r * x % p; }}
        return {a[3] * r % p, (p - a[1]) % p * r % p, (p - a[2]) % p * r % p, a[0] * r % p};}
};
template<typename G> void checkProduct(const vector<typename G::S> &a, const G &g) {
    PrefixProduct<G> p(a, g); int n = int(a.size()); checkEqual(p.n, n, "product size");
    for (const auto &x : a) { checkEqual(g.op(g.inv(x), x), g.e(), "oracle left inverse"); checkEqual(g.op(x, g.inv(x)), g.e(), "oracle right inverse"); }
    for (int l = 0; l <= n; ++l) {
        auto want = g.e();
        for (int r = l; r <= n; ++r) {
            if (r > l) { want = g.op(want, a[r - 1]); }
            checkEqual(p.prod(l, r), want, "prod(" + show(l) + ',' + show(r) + ')');}}}
void products() {
    history.clear(); context = "group oracles";
    vector<Perm::S> perms; Perm::S q = Perm::e();
    do { perms.push_back(q); } while (std::next_permutation(q.begin(), q.end()));
    checkEqual(perms.size(), size_t(24), "permutation count");
    checkEqual(Perm::op(perms[1], perms[2]) != Perm::op(perms[2], perms[1]), true, "permutations noncommutative");
    Matrix2 g7{7}; Matrix2::S u{1, 1, 0, 1}, v{1, 0, 1, 1};
    checkEqual(g7.op(u, v) != g7.op(v, u), true, "matrices noncommutative");
    int length = mode == "quick" ? 2 : mode == "full" ? 3 : 4; lng sequences = 0;
    for (int n = 0, count = 1; n <= length; ++n, count *= 24) {
        for (int code = 0; code < count; ++code) {
            vector<Perm::S> a(n); int x = code;
            for (auto &s : a) { s = perms[x % 24]; x /= 24; }
            context = "permutations=" + show(a); checkProduct(a, Perm{}); ++sequences;}}
    std::mt19937_64 rng(seed + 3); int trials = mode == "quick" ? 20 : mode == "full" ? 150 : 600;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng() % 41);
        vector<Perm::S> a(n);
        for (auto &s : a) { s = perms[rng() % 24]; }
        context = "random permutations=" + show(a); checkProduct(a, Perm{});
        for (ulng mod : {ulng(7), ulng(1000000007)}) {
            Matrix2 g{mod}; vector<Matrix2::S> b(n);
            for (auto &s : b) { do { s = {rng() % mod, rng() % mod, rng() % mod, rng() % mod}; } while (!g.det(s)); }
            context = "matrices mod " + show(mod) + '=' + show(b); checkProduct(b, g);
            PrefixProduct<Matrix2> stored(b, g); checkEqual(stored.m.p, mod, "stored group state");}
        vector<lng> c(n);
        for (lng &s : c) { s = lng(rng() % 2000000000001) - 1000000000000; }
        context = "Sum<lng>=" + show(c); checkProduct(c, Sum<lng>{});
        vector<ulng> w(n);
        for (ulng &s : w) { s = rng(); }
        context = "Xor<ulng>=" + show(w); checkProduct(w, Xor<ulng>{});
        vector<uint> z(n);
        for (uint &s : z) { s = uint(rng()); }
        context = "Sum<uint>=" + show(z); checkProduct(z, Sum<uint>{});}
    context = "product boundaries";
    vector<Matrix2::S> diag{{2, 0, 0, 1}, {4, 0, 0, 1}};
    checkEqual(PrefixProduct<Matrix2>(diag).prod(0, 2), Matrix2::S{1, 0, 0, 1}, "default group state");
    checkEqual(PrefixProduct<Matrix2>(diag, Matrix2{1000000007}).prod(0, 2), Matrix2::S{8, 0, 0, 1}, "passed group state");
    PrefixProduct<Sum<lng>> empty; checkEqual(empty.n, 0, "empty product size"); checkEqual(empty.prod(0, 0), lng(0), "empty Sum product");
    checkEqual(PrefixProduct<Perm>(vector<Perm::S>{}).prod(0, 0), Perm::e(), "empty permutation product");
    checkEqual(PrefixProduct<Matrix2>({}, Matrix2{11}).prod(0, 0), Matrix2::S{1, 0, 0, 1}, "empty matrix product");
    PrefixProduct<Sum<lng>> extreme(vector<lng>{LLONG_MAX, LLONG_MIN + 1});
    checkEqual(extreme.prod(1, 2), lng(LLONG_MIN + 1), "int64 extreme product"); checkEqual(extreme.prod(0, 2), lng(0), "int64 cancellation product");
    auto copy = extreme; auto moved = std::move(copy); extreme = PrefixProduct<Sum<lng>>(vector<lng>{5});
    checkEqual(moved.prod(0, 1), lng(LLONG_MAX), "product copy/move"); checkEqual(extreme.prod(0, 1), lng(5), "product reconstruction");
    cout << "PASS prefix products: S4 sequences n<=" << length << " (" << sequences << "), random S4/GL2(7)/GL2(1e9+7)/Sum/Xor/uint trials=" << trials << ", empty and state regressions\n";}
void check3D(const V3 &a, int n, int m, int k, bool updates) {
    context = "box shape=" + show(n) + 'x' + show(m) + 'x' + show(k) + " values=" + show(a);
    bool vec = expressible(n, m, k);
    PrefixSum3D<lng> p = vec ? PrefixSum3D<lng>(a) : PrefixSum3D<lng>(n, m, k);
    DifferenceArray3D<lng> d = vec ? DifferenceArray3D<lng>(a) : DifferenceArray3D<lng>(n, m, k);
    checkEqual(array<int, 3>{p.n, p.m, p.k}, array<int, 3>{n, m, k}, "3D prefix shape");
    checkEqual(array<int, 3>{d.n, d.m, d.k}, array<int, 3>{n, m, k}, "3D difference shape");
    checkEqual(d.values(), a, "3D inverse"); checkEqual(d.values(), a, "3D repeat values");
    for (int x1 = 0; x1 <= n; ++x1) { for (int x2 = x1; x2 <= n; ++x2) {
        for (int y1 = 0; y1 <= m; ++y1) { for (int y2 = y1; y2 <= m; ++y2) {
            for (int z1 = 0; z1 <= k; ++z1) { for (int z2 = z1; z2 <= k; ++z2) {
                string box = '(' + show(x1) + ',' + show(y1) + ',' + show(z1) + ',' + show(x2) + ',' + show(y2) + ',' + show(z2) + ')';
                checkEqual(p.sum(x1, y1, z1, x2, y2, z2), sum3(a, x1, y1, z1, x2, y2, z2), "box sum" + box);
                if (!x1 && !y1 && !z1) { checkEqual(p.prefixSum(x2, y2, z2), sum3(a, 0, 0, 0, x2, y2, z2), "3D prefix"); }
                if (updates) {
                    for (lng w : {-1, 1}) {
                        auto b = a; auto copy = d; copy.add(x1, y1, z1, x2, y2, z2, w); add3(b, x1, y1, z1, x2, y2, z2, w);
                        checkEqual(copy.values(), b, "box add" + box + " delta=" + show(w));}}}}}}}}}
void boxes() {
    int dim = mode == "quick" ? 2 : 3, cells = mode == "quick" ? 4 : mode == "full" ? 6 : 8, fills = mode == "quick" ? 2 : mode == "full" ? 10 : 40;
    std::mt19937_64 rng(seed + 5); lng arrays = 0;
    for (int n = 0; n <= dim; ++n) { for (int m = 0; m <= dim; ++m) { for (int k = 0; k <= dim; ++k) {
        int c = n * m * k, count = 1; bool all = c <= cells;
        if (all) { for (int i = 0; i < c; ++i) { count *= 3; }}
        else { count = fills; }
        for (int code = 0; code < count; ++code) {
            V3 a = zeros3(n, m, k); int x = code;
            for (auto &plane : a) { for (auto &row : plane) { for (lng &v : row) {
                if (all) { v = x % 3 - 1; x /= 3; }
                else { v = lng(rng() % 3) - 1; }}}}
            history.clear(); check3D(a, n, m, k, true); ++arrays;}}}}
    int depth = mode == "quick" ? 2 : mode == "full" ? 3 : 4; lng nodes = 0;
    for (auto s : vector<array<int, 3>>{{2, 2, 2}, {1, 3, 2}, {3, 1, 1}}) {
        int n = s[0], m = s[1], k = s[2], t = 0;
        V3 a = zeros3(n, m, k);
        for (auto &plane : a) { for (auto &row : plane) { for (lng &v : row) { v = t++ % 3 - 1; }}}
        auto dfs = [&](auto &&self, const DifferenceArray3D<lng> &d, const V3 &b, int left) -> void {
            ++nodes; checkEqual(d.values(), b, "3D history values");
            if (!left) { return; }
            string old = history; auto copy = d; copy.clear(); history = old + 'C';
            self(self, copy, zeros3(n, m, k), left - 1);
            // Empty boxes are no-ops, checked exhaustively in check3D; histories branch on nonempty boxes only.
            for (int x1 = 0; x1 < n; ++x1) { for (int x2 = x1 + 1; x2 <= n; ++x2) {
                for (int y1 = 0; y1 < m; ++y1) { for (int y2 = y1 + 1; y2 <= m; ++y2) {
                    for (int z1 = 0; z1 < k; ++z1) { for (int z2 = z1 + 1; z2 <= k; ++z2) {
                        for (lng w : {-1, 2}) {
                            copy = d; copy.add(x1, y1, z1, x2, y2, z2, w); auto c = b; add3(c, x1, y1, z1, x2, y2, z2, w);
                            history = old + '(' + show(x1) + ',' + show(y1) + ',' + show(z1) + ',' + show(x2) + ',' + show(y2) + ',' + show(z2) + ',' + show(w) + ')';
                            self(self, copy, c, left - 1);}}}}}}}
            history = old;};
        context = "3D exhaustive histories initial=" + show(a); history.clear();
        dfs(dfs, DifferenceArray3D<lng>(a), a, n * m * k == 8 ? min(depth, 3) : depth);}
    cout << "PASS exhaustive boxes dims<=" << dim << " (" << arrays << " arrays, all boxes and updates), 3D nonempty-box/clear histories depth=" << depth << " (2x2x2 at most 3) nodes=" << nodes << '\n';}
void randomBoxes() {
    std::mt19937_64 rng(seed + 7); int trials = mode == "quick" ? 10 : mode == "full" ? 60 : 250;
    auto coordinate = [&](int n) { return int(rng() % ulng(n + 1)); };
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng() % 8), m = int(rng() % 8), k = int(rng() % 8); bool vec = expressible(n, m, k);
        V3 a = zeros3(n, m, k);
        for (auto &plane : a) { for (auto &row : plane) { for (lng &v : row) { v = lng(rng() % 101) - 50; }}}
        DifferenceArray3D<lng> d = vec ? DifferenceArray3D<lng>(a) : DifferenceArray3D<lng>(n, m, k);
        history.clear(); context = "random 3D shape=" + show(n) + 'x' + show(m) + 'x' + show(k) + " initial=" + show(a);
        for (int op = 0; op < 80; ++op) {
            int x1 = coordinate(n), x2 = coordinate(n), y1 = coordinate(m), y2 = coordinate(m), z1 = coordinate(k), z2 = coordinate(k);
            if (x1 > x2) { swap(x1, x2); }
            if (y1 > y2) { swap(y1, y2); }
            if (z1 > z2) { swap(z1, z2); }
            lng w = lng(rng() % 101) - 50;
            history += '(' + show(x1) + ',' + show(y1) + ',' + show(z1) + ',' + show(x2) + ',' + show(y2) + ',' + show(z2) + ',' + show(w) + ')';
            d.add(x1, y1, z1, x2, y2, z2, w); add3(a, x1, y1, z1, x2, y2, z2, w);
            checkEqual(d.values(), a, "random 3D imos");
            PrefixSum3D<lng> p = vec ? PrefixSum3D<lng>(a) : PrefixSum3D<lng>(n, m, k);
            checkEqual(p.sum(x1, y1, z1, x2, y2, z2), sum3(a, x1, y1, z1, x2, y2, z2), "random box");
            checkEqual(p.prefixSum(x2, y2, z2), sum3(a, 0, 0, 0, x2, y2, z2), "random 3D prefix");
            if (op % 17 == 0 && vec) { d = DifferenceArray3D<lng>(d.values()); }
            if (op % 23 == 0) { auto copy = d; copy.clear(); checkEqual(d.values(), a, "3D copy independence"); }
            if (op % 41 == 40) { d.clear(); a = zeros3(n, m, k); history += 'C'; checkEqual(d.values(), a, "3D clear"); }}}
    cout << "PASS seeded box histories dims<=7 ops=80 trials=" << trials << '\n';}
void boxBoundaries() {
    context = "3D types/empty dimensions/aliasing"; history.clear();
    PrefixSum3D<lng> p(V3{{{LLONG_MIN, 0, LLONG_MAX}}});
    checkEqual(p.sum(0, 0, 0, 1, 1, 3), lng(-1), "3D int64 cancellation"); checkEqual(p.sum(0, 0, 1, 1, 1, 3), lng(LLONG_MAX), "3D int64 max");
    DifferenceArray3D<lng> d(2, 2, 2); d.add(0, 0, 0, 2, 2, 2, LLONG_MIN);
    checkEqual(d.values(), V3(2, vector<vector<lng>>(2, vector<lng>(2, LLONG_MIN))), "3D no unused negative border");
    lll big = lll(1) << 100;
    DifferenceArray3D<lll> hd(2, 2, 2); hd.add(0, 0, 0, 2, 2, 2, big); PrefixSum3D<lll> hp(hd.values());
    checkEqual(hp.sum(0, 0, 0, 2, 2, 2), 8 * big, "128-bit box");
    DifferenceArray3D<uint> ud(vector<vector<vector<uint>>>{{{UINT_MAX, 0}}, {{1, UINT_MAX}}}); ud.add(0, 0, 0, 2, 1, 2, 1);
    checkEqual(ud.values(), vector<vector<vector<uint>>>{{{0, 1}}, {{2, 0}}}, "modular 3D differences");
    PrefixSum3D<uint> up(ud.values()); checkEqual(up.sum(0, 0, 0, 2, 1, 2), uint(3), "modular 3D prefix");
    for (auto s : vector<array<int, 3>>{{0, 0, 0}, {0, 5, 3}, {4, 0, 3}, {4, 5, 0}, {2, 0, 0}, {3, 4, 0}, {5, 0, 7}}) {
        int n = s[0], m = s[1], k = s[2];
        PrefixSum3D<lng> z(n, m, k); DifferenceArray3D<lng> zd(n, m, k); zd.add(0, 0, 0, n, m, k, 7);
        checkEqual(z.sum(0, 0, 0, n, m, k), lng(0), "empty 3D query"); checkEqual(z.prefixSum(n, m, k), lng(0), "empty 3D prefix");
        checkEqual(array<int, 3>{zd.n, zd.m, zd.k}, s, "empty 3D difference shape");
        checkEqual(zd.values(), zeros3(n, m, k), "empty 3D output");
        zd.add(n / 2, m / 2, k / 2, n, m, k, -3); zd.clear(); checkEqual(zd.values(), zeros3(n, m, k), "empty 3D clear");}
    PrefixSum3D<lng> planes(V3(2)); checkEqual(array<int, 3>{planes.n, planes.m, planes.k}, array<int, 3>{2, 0, 0}, "planes without rows");
    DifferenceArray3D<lng> rows(V3(1, vector<vector<lng>>(2))); checkEqual(array<int, 3>{rows.n, rows.m, rows.k}, array<int, 3>{1, 2, 0}, "rows without cells");
    checkEqual(rows.values(), V3(1, vector<vector<lng>>(2)), "rows without cells output");
    PrefixSum3D<lng> none(V3{}); checkEqual(array<int, 3>{none.n, none.m, none.k}, array<int, 3>{0, 0, 0}, "empty 3D vector");
    DifferenceArray3D<lng> ad(V3{{{2, 3}}, {{4, 6}}}); ad.add(0, 0, 0, 2, 1, 2, ad.d[0]);
    checkEqual(ad.values(), V3{{{4, 5}}, {{6, 8}}}, "3D delta alias");
    ad = DifferenceArray3D<lng>(ad.values()); checkEqual(ad.values(), V3{{{4, 5}}, {{6, 8}}}, "3D reconstruction from own values");
    auto dcopy = ad; dcopy.clear(); auto dm = std::move(dcopy);
    checkEqual(ad.values(), V3{{{4, 5}}, {{6, 8}}}, "3D difference copy independence"); checkEqual(dm.values(), zeros3(2, 1, 2), "3D difference move");
    auto copy = p; auto moved = std::move(copy); p = PrefixSum3D<lng>(V3{{{1, 2}}, {{-4, 5}}});
    checkEqual(p.sum(0, 0, 0, 2, 1, 2), lng(4), "3D reconstruction"); checkEqual(moved.sum(0, 0, 1, 1, 1, 3), lng(LLONG_MAX), "3D copy/move");
    DifferenceArray3D<lng> full(V3{{{LLONG_MAX}}});
    full.add(0, 0, 0, 0, 1, 1, 1); full.add(0, 0, 0, 1, 0, 1, 1); full.add(0, 0, 0, 1, 1, 0, 1);
    checkEqual(full.values(), V3{{{LLONG_MAX}}}, "3D empty boxes leave a full cell untouched");
    // No planes but m * k > INT_MAX cells per plane: the per-axis passes must not form m * k.
    DifferenceArray3D<lng> wide(0, 50000, 50000); wide.add(0, 0, 0, 0, 50000, 50000, 1);
    checkEqual(wide.values().size(), size_t(0), "3D difference with no planes and wide planes"); wide.clear();
    // values() must not build an m-row plane when there are no planes: m = INT_MAX empty rows would be 48 GiB.
    DifferenceArray3D<lng> tall(0, INT_MAX, 0); checkEqual(tall.values().size(), size_t(0), "3D difference with no planes and INT_MAX rows");
    if (mode != "quick") {
        // INT_MAX is prime, so (n + 1) * 1 * 1 = INT_MAX cells is the largest admitted shape; 256 MiB as bool.
        PrefixSum3D<bool> edge(INT_MAX - 1, 0, 0);
        checkEqual(array<lng, 4>{edge.n, edge.m, edge.k, lng(edge.p.size())}, array<lng, 4>{INT_MAX - 1, 0, 0, INT_MAX}, "3D largest admitted cell count");}
    static_assert(!std::is_convertible_v<int, PrefixSum3D<lng>> && !std::is_convertible_v<int, DifferenceArray3D<lng>>);
    static_assert(!std::is_convertible_v<vector<lng>, PrefixProduct<Sum<lng>>>);
    cout << "PASS 3D int64 extremes, 128-bit, modular, empty dimensions, nested-vector shapes, alias/copy regressions\n";}
template<typename T> T stepSum(const vector<T> &a, int step, int k, int l, int r) {
    T ans = T(0);
    for (int i = l; i < r; ++i) {
        if (i % step == k) { ans += a[i]; }}
    return ans;}
template<typename T> void checkStep(const vector<T> &a, int step) {
    StepSum<T> s(a, step); int n = int(a.size());
    checkEqual(array<int, 2>{s.n, s.step}, array<int, 2>{n, step}, "step shape");
    for (int k = 0; k < step; ++k) {
        for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
            checkEqual(s.sum(k, l, r), stepSum(a, step, k, l, r), "step sum(" + show(k) + ',' + show(l) + ',' + show(r) + ')');}}}}
void stepSums() {
    history.clear(); int limit = mode == "quick" ? 5 : 8; lng cases = 0;
    std::mt19937_64 rng(seed + 9);
    for (int n = 0; n <= limit; ++n) {
        // Distinct powers of two identify every summed subset; random ternary values add signs and duplicates.
        vector<lng> powers(n), ternary(n);
        for (int i = 0; i < n; ++i) { powers[i] = lng(1) << i; ternary[i] = lng(rng() % 3) - 1; }
        for (int step = 1; step <= n + 3; ++step) {
            context = "step exhaustive n=" + show(n) + " step=" + show(step) + " powers"; checkStep(powers, step);
            context = "step exhaustive ternary=" + show(ternary) + " step=" + show(step); checkStep(ternary, step); cases += 2;}
        PrefixSum<lng> p(powers); StepSum<lng> one(powers, 1);
        context = "step 1 against PrefixSum n=" + show(n);
        for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) { checkEqual(one.sum(0, l, r), p.sum(l, r), "step 1 equals PrefixSum"); }}}
    int trials = mode == "quick" ? 10 : mode == "full" ? 100 : 400;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng() % 201), step = trial % 2 ? int(rng() % ulng(n + 6)) + 1 : int(rng() % 8) + 1;
        vector<lng> a(n);
        for (lng &x : a) { x = lng(rng() % 2000000001) - 1000000000; }
        StepSum<lng> s(a, step); context = "random step=" + show(step) + " array=" + show(a);
        for (int q = 0; q < 200; ++q) {
            int k = int(rng() % ulng(step)), l = int(rng() % ulng(n + 1)), r = int(rng() % ulng(n + 1));
            if (l > r) { swap(l, r); }
            checkEqual(s.sum(k, l, r), stepSum(a, step, k, l, r), "random step sum(" + show(k) + ',' + show(l) + ',' + show(r) + ')');}}
    context = "step types/boundaries";
    lll big = lll(1) << 100;
    vector<lll> wide{big, -big, big, 3 * big, -big};
    for (int step = 1; step <= 7; ++step) { checkStep(wide, step); }
    vector<uint> wrap{UINT_MAX, 2, UINT_MAX, 5, 1};
    for (int step = 1; step <= 7; ++step) { checkStep(wrap, step); }
    StepSum<uint> ws(wrap, 2); checkEqual(ws.sum(0, 0, 5), uint(UINT_MAX), "modular step wrap");
    StepSum<lng> empty(vector<lng>{}, 1), emptyWide(vector<lng>{}, 5);
    checkEqual(empty.sum(0, 0, 0), lng(0), "empty step 1"); checkEqual(emptyWide.sum(4, 0, 0), lng(0), "empty step 5");
    StepSum<lng> sparse(vector<lng>{4, -7, 9}, INT_MAX);
    checkEqual(sparse.sum(1, 0, 3), lng(-7), "step INT_MAX class 1"); checkEqual(sparse.sum(INT_MAX - 1, 0, 3), lng(0), "class beyond n");
    checkEqual(sparse.sum(2, 1, 2), lng(0), "range before class element"); checkEqual(sparse.sum(2, 2, 3), lng(9), "class element alone");
    StepSum<lng> e(vector<lng>{LLONG_MIN, 1, LLONG_MAX, 1}, 2);
    checkEqual(e.sum(0, 0, 4), lng(-1), "int64 step cancellation"); checkEqual(e.sum(0, 1, 4), lng(LLONG_MAX), "int64 step unaligned l");
    checkEqual(e.sum(1, 0, 3), lng(1), "unaligned r");
    auto copy = e; auto moved = std::move(copy); e = StepSum<lng>(vector<lng>{5}, 3);
    checkEqual(moved.sum(0, 0, 4), lng(-1), "step copy/move"); checkEqual(e.sum(0, 0, 1), lng(5), "step reconstruction"); checkEqual(e.sum(2, 0, 1), lng(0), "step class k >= n");
    cout << "PASS step sums: exhaustive n<=" << limit << " step<=n+3 every k/l/r (" << cases << " arrays), random n<=200 trials=" << trials << ", 128-bit/modular/empty/INT_MAX step\n";}
void invalid(const string &name) {
    PrefixSum<lng> p(3); PrefixSum2D<lng> q(2, 3); DifferenceArray<lng> d(3); DifferenceArray2D<lng> e(2, 3);
    PrefixProduct<Sum<lng>> pp(vector<lng>{1, 2, 3}); PrefixSum3D<lng> q3(2, 2, 2); DifferenceArray3D<lng> e3(2, 2, 2);
    StepSum<lng> ss(vector<lng>{1, 2, 3, 4}, 2);
    if (name == "negative-size") { PrefixSum<lng> bad(-1); }
    else if (name == "oversize") { prefix_sum_detail::checkedSize(size_t(INT_MAX)); }
    else if (name == "negative-dimension") { DifferenceArray2D<lng> bad(0, -1); }
    else if (name == "ragged-prefix") { PrefixSum2D<lng> bad(vector<vector<lng>>{{1}, {}}); }
    else if (name == "ragged-difference") { DifferenceArray2D<lng> bad(vector<vector<lng>>{{}, {1}}); }
    else if (name == "prefix-negative") { p.prefixSum(-1); }
    else if (name == "prefix-end") { p.prefixSum(4); }
    else if (name == "range-reversed") { p.sum(2, 1); }
    else if (name == "range-end") { p.sum(0, 4); }
    else if (name == "2d-prefix") { q.prefixSum(2, 4); }
    else if (name == "2d-range") { q.sum(0, 2, 2, 1); }
    else if (name == "difference-negative") { d.add(-1, 2, 1); }
    else if (name == "difference-end") { d.add(0, 4, 1); }
    else if (name == "difference-reversed") { d.add(2, 1, 1); }
    else if (name == "2d-add-end") { e.add(0, 0, 3, 3, 1); }
    else if (name == "2d-add-reversed") { e.add(1, 0, 0, 3, 1); }
    else if (name == "arith-negative") { d.addArithmetic(-1, 2, 1, 1); }
    else if (name == "arith-end") { d.addArithmetic(0, 4, 1, 1); }
    else if (name == "arith-reversed") { d.addArithmetic(2, 1, 1, 1); }
    else if (name == "product-negative") { pp.prod(-1, 1); }
    else if (name == "product-end") { pp.prod(0, 4); }
    else if (name == "product-reversed") { pp.prod(2, 1); }
    else if (name == "3d-negative-dimension") { PrefixSum3D<lng> bad(0, -1, 0); }
    else if (name == "3d-oversize") { PrefixSum3D<lng> bad(1290, 1290, 1290); }
    else if (name == "3d-difference-negative") { DifferenceArray3D<lng> bad(0, 0, -1); }
    else if (name == "3d-difference-oversize") { DifferenceArray3D<lng> bad(1291, 1291, 1291); }
    else if (name == "3d-difference-flat-oversize") { DifferenceArray3D<int> bad(50000, 50000, 0); }
    else if (name == "step-zero") { StepSum<lng> bad(vector<lng>{1, 2}, 0); }
    else if (name == "step-class-end") { ss.sum(2, 0, 4); }
    else if (name == "step-class-past-end") { ss.sum(3, 0, 4); }
    else if (name == "difference-negative-size") { DifferenceArray<lng> bad(-1); }
    else if (name == "step-class-negative") { ss.sum(-1, 0, 4); }
    else if (name == "step-range-reversed") { ss.sum(0, 3, 2); }
    else if (name == "step-range-end") { ss.sum(1, 0, 5); }
    else if (name == "3d-ragged-plane") { PrefixSum3D<lng> bad(V3{{{1}}, {}}); }
    else if (name == "3d-ragged-row") { V3 a(1, vector<vector<lng>>(2, vector<lng>(1))); a[0][1].push_back(2); DifferenceArray3D<lng> bad(a); }
    else if (name == "3d-prefix") { q3.prefixSum(0, 0, 3); }
    else if (name == "3d-range-reversed") { q3.sum(0, 0, 2, 1, 1, 1); }
    else if (name == "3d-range-end") { q3.sum(0, 0, 0, 3, 1, 1); }
    else if (name == "3d-add-reversed") { e3.add(0, 1, 0, 1, 0, 1, 1); }
    else if (name == "3d-add-end") { e3.add(0, 0, 0, 1, 1, 3, 1); }
    else if (name == "difference-max-size") { DifferenceArray<bool> bad(INT_MAX); }
    else if (name == "2d-difference-max-rows") { DifferenceArray2D<bool> bad(INT_MAX, 0); }
    else if (name == "2d-difference-max-columns") { DifferenceArray2D<bool> bad(0, INT_MAX); }
    else { throw std::runtime_error("unknown invalid probe " + name); }
    throw std::runtime_error("invalid precondition survived");}
int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string a = argv[i]; if (i + 1 == argc) { return 2; }
            if (a == "--seed") { seed = std::stoull(argv[++i]); }
            else if (a == "--mode") { mode = argv[++i]; }
            else if (a == "--invalid") { probe = argv[++i]; }
            else { return 2; }}
        if (!probe.empty()) { invalid(probe); }
        if (mode != "quick" && mode != "full" && mode != "stress") { return 2; }
        cout << "prefix_sum seed=" << seed << " mode=" << mode << '\n';
        arrays(); matrices(); randomized(); boundaries();
        arithmeticHistories(); arithmeticRandom(); arithmeticBoundaries(); products(); boxes(); randomBoxes(); boxBoundaries(); stepSums();
        cout << "PASS prefix_sum checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL prefix_sum seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
