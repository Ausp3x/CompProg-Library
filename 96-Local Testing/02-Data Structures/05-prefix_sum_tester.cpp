#include "../../02-Data Structures/05-prefix_sum.hpp"

ulng seed = 20260927;
string mode = "full", context, history;
lng checks = 0;

string show(lll x) {
    bool negative = x < 0; ulll u = negative ? ulll(-(x + 1)) + 1 : ulll(x);
    string s;
    do { s.push_back(char('0' + u % 10)); u /= 10; } while (u);
    if (negative) { s.push_back('-'); } reverse(s.begin(), s.end()); return s;}
template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
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
    p.rebuild({4, -3}); checkEqual(p.sum(0, 2), lng(1), "prefix rebuild");
    checkEqual(moved.sum(0, n), sum(a, 0, n), "copy independence");
    auto dm = std::move(d); checkEqual(dm.values(), a, "difference move");
    d.rebuild({1, -2}); d.clear(); checkEqual(d.values(), vector<lng>({0, 0}), "difference clear/rebuild");}
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
    p.rebuild({{1, 2}, {-4, 5}}); checkEqual(p.sum(0, 0, 2, 2), lng(4), "2D rebuild");
    checkEqual(moved.sum(0, 0, n, m), sum(a, 0, 0, n, m), "2D copy independence/move");
    auto dm = std::move(d); checkEqual(dm.values(), a, "2D move");
    d.rebuild({{3}, {-2}}); d.clear(); checkEqual(d.values(), vector<vector<lng>>({{0}, {0}}), "2D clear/rebuild");}
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
            if (op % 17 == 0 && n) { d.rebuild(a); }
            if (op % 31 == 0) { auto copy = d; copy.clear(); checkEqual(d.values(), a, "copy independence"); }}}
    cout << "PASS seeded rectangle histories/rebuild/materialization trials=" << trials << '\n';}
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
    alias.rebuild(alias.d); checkEqual(alias.values(), vector<lng>({4, 1, -1}), "rebuild input alias");
    DifferenceArray2D<lng> ad(vector<vector<lng>>{{2, 3}, {4, 6}});
    ad.add(0, 0, 2, 2, ad.d[0][0]); checkEqual(ad.values(), vector<vector<lng>>({{4, 5}, {6, 8}}), "2D delta alias");
    auto before = ad.d; ad.rebuild(ad.d); checkEqual(ad.values(), before, "2D rebuild input alias");
    PrefixSum<lng> ap(vector<lng>{1, 2}); auto old = ap.p; ap.rebuild(ap.p);
    checkEqual(ap.sum(0, 3), sum(old, 0, 3), "prefix rebuild input alias");
    PrefixSum<lng> single({7}), pair{7, 8}; DifferenceArray<lng> dsingle({7}), dpair{7, 8};
    checkEqual(single.n, 1, "braced singleton prefix size"); checkEqual(single.sum(0, 1), lng(7), "braced singleton prefix");
    checkEqual(pair.sum(0, 2), lng(15), "braced prefix list");
    checkEqual(dsingle.values(), vector<lng>({7}), "braced singleton difference"); checkEqual(dpair.values(), vector<lng>({7, 8}), "braced difference list");
    static_assert(!std::is_convertible_v<int, PrefixSum<lng>> && !std::is_convertible_v<int, DifferenceArray<lng>>);
    cout << "PASS wide/modular/exact dyadic types, empty dimensions, endpoint and alias regressions\n";}
void invalid(const string &name) {
    PrefixSum<lng> p(3); PrefixSum2D<lng> q(2, 3); DifferenceArray<lng> d(3); DifferenceArray2D<lng> e(2, 3);
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
        cout << "PASS prefix_sum checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL prefix_sum seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
