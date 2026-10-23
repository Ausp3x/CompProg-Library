#include "../../06-Miscellaneous/54-kbest_enumeration.hpp"

#ifdef _GLIBCXX_DEBUG
constexpr bool DEBUG_CONTAINERS = true;  // debug heaps validate the whole range on every push and pop
#else
constexpr bool DEBUG_CONTAINERS = false;
#endif
#ifdef __SANITIZE_ADDRESS__
constexpr bool SANITIZED = true;  // ASan stress would exceed the runner's 180 s limit, so it runs as full
#else
constexpr bool SANITIZED = false;
#endif
ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
[[noreturn]] void fail(const string &what, const string &want, const string &got) {
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
         << " operation=" << what << " expected=" << want << " actual=" << got << '\n';
    std::exit(1);}
void check(bool ok, const string &what) { ++checks; if (!ok) { fail(what, "true", "false"); } }
template<class T> string show(const vector<T> &a) {
    std::ostringstream out; out << '[';
    for (const auto &x : a) { out << x << ','; }
    out << ']'; return out.str();}
template<class T> string str(const T &x) { std::ostringstream out; out << x; return out.str(); }
template<class A, class B> string str(const pair<A, B> &x) { return "(" + str(x.first) + "," + str(x.second) + ")"; }
// Strict order on (sum under cmp, then tie tuple); the brute force sorts with it independently of the frontier.
template<class T, class C, class Tie> bool before(const T &x, const T &y, const Tie &tx, const Tie &ty, C cmp) {
    return cmp(x, y) || (!cmp(y, x) && tx < ty);}
template<class T, class C> bool equivalent(const T &x, const T &y, C cmp) { return !cmp(x, y) && !cmp(y, x); }
template<class A> void same(const A &got, const A &want, const char *what) { ++checks; if (!(got == want)) { fail(what, str(want), str(got)); } }
template<class T, class C> void sameUnder(const T &got, const T &want, C cmp, const char *what) {
    ++checks;
    if (!equivalent(got, want, cmp)) { fail(what, str(want), str(got)); }}

// A comparator carrying state: s = 1 orders like less, s = -1 like greater, and a default-constructed one (s = 0) ties everything.
struct Scaled {
    lng s = 0;
    bool operator()(lng x, lng y) const { return s * x < s * y; }
};

template<class T, class C> void pairCase(vector<T> a, vector<T> b, int k, C cmp) {
    ++cases;
    std::stable_sort(a.begin(), a.end(), cmp); std::stable_sort(b.begin(), b.end(), cmp);
    context = "pairs a=" + show(a) + " b=" + show(b) + " k=" + std::to_string(k);
    vector<tuple<T, int, int>> all;
    for (int i = 0; i < int(a.size()); ++i) { for (int j = 0; j < int(b.size()); ++j) { all.emplace_back(a[i] + b[j], i, j); } }
    sort(all.begin(), all.end(), [&](const auto &x, const auto &y) {
        return before(std::get<0>(x), std::get<0>(y), pair(std::get<1>(x), std::get<2>(x)), pair(std::get<1>(y), std::get<2>(y)), cmp);});
    auto got = kSmallestPairSums(a, b, k, cmp);
    same(int(got.size()), min(k, int(all.size())), "pair count min(k, n * m)");
    for (int e = 0; e < int(got.size()); ++e) {
        same(got[e], pair(std::get<1>(all[e]), std::get<2>(all[e])), "pair order (sum, i, j)");}}

template<class T, class C> void subarrayCase(const vector<T> &a, int k, int L, int R, C cmp) {
    ++cases; int n = int(a.size());
    context = "subarrays a=" + show(a) + " k=" + std::to_string(k) + " L=" + std::to_string(L) + " R=" + std::to_string(R);
    vector<tuple<T, int, int>> all;
    for (int l = 0; l < n; ++l) {
        T s = T(0);
        for (int r = l + 1; r <= n; ++r) {
            s += a[r - 1];
            if (L <= r - l && r - l <= R) { all.emplace_back(s, l, r); }}}
    sort(all.begin(), all.end(), [&](const auto &x, const auto &y) {
        return before(std::get<0>(x), std::get<0>(y), pair(std::get<1>(x), std::get<2>(x)), pair(std::get<1>(y), std::get<2>(y)), cmp);});
    auto got = kSmallestSubarraySums(a, k, L, R, cmp);
    same(int(got.size()), min(k, int(all.size())), "subarray count");
    for (int e = 0; e < int(got.size()); ++e) {
        same(got[e], pair(std::get<1>(all[e]), std::get<2>(all[e])), "subarray order (sum, l, r)");}}

// Checks sums against the sorted brute force and every witness: valid, sum-consistent, pairwise distinct.
template<class T, class C> void checkSubsets(const KSmallestSubsets<T> &got, const vector<T> &a, vector<T> sums, int k, int size, C cmp) {
    std::stable_sort(sums.begin(), sums.end(), cmp);
    int want = min(k, int(sums.size()));
    same(int(got.sums.size()), want, "subset output count");
    std::set<vector<int>> seen;
    for (int e = 0; e < want; ++e) {
        sameUnder(got.sums[e], sums[e], cmp, "subset sums equal sorted brute force");
        auto s = got.subset(e); T t = T(0);
        for (int i = 0; i < int(s.size()); ++i) {
            check(0 <= s[i] && s[i] < int(a.size()) && (!i || s[i - 1] < s[i]), "subset indices increasing");
            t += a[s[i]];}
        sameUnder(t, got.sums[e], cmp, "witness sum equals reported sum");
        check(size < 0 || int(s.size()) == size, "witness size");
        check(seen.insert(s).second, "witnesses pairwise distinct");}}
template<class T, class C> void subsetCase(const vector<T> &a, int k, C cmp) {
    ++cases; int n = int(a.size());
    context = "subsets a=" + show(a) + " k=" + std::to_string(k);
    vector<T> sums;
    for (int mask = 0; mask < 1 << n; ++mask) {
        T s = T(0);
        for (int i = 0; i < n; ++i) { if (mask >> i & 1) { s += a[i]; } }
        sums.push_back(s);}
    auto got = kSmallestSubsetSums(a, k, cmp);
    checkSubsets(got, a, sums, k, -1, cmp);
    if (k) {
        vector<int> neg;
        for (int i = 0; i < n; ++i) { if (cmp(a[i], T(0))) { neg.push_back(i); } }
        check(got.subset(0) == neg, "first output is the set of cmp-negative elements");}}
template<class T, class C> void fixedCase(const vector<T> &a, int m, int k, C cmp) {
    ++cases; int n = int(a.size());
    context = "fixed a=" + show(a) + " m=" + std::to_string(m) + " k=" + std::to_string(k);
    vector<T> sums;
    for (int mask = 0; mask < 1 << n; ++mask) {
        if (std::popcount(uint(mask)) != m) { continue; }
        T s = T(0);
        for (int i = 0; i < n; ++i) { if (mask >> i & 1) { s += a[i]; } }
        sums.push_back(s);}
    checkSubsets(kSmallestFixedSizeSubsetSums(a, m, k, cmp), a, sums, k, m, cmp);}
template<class T, class C> void listCase(vector<vector<T>> lists, int k, C cmp) {
    ++cases;
    for (auto &l : lists) { std::stable_sort(l.begin(), l.end(), cmp); }
    context = "lists k=" + std::to_string(k) + " lists=";
    for (auto &l : lists) { context += show(l); }
    vector<T> sums; vector<int> pick(lists.size());
    bool any = std::none_of(lists.begin(), lists.end(), [](const auto &l) { return l.empty(); });
    auto visit = [&](auto &&visit, int i, T s) -> void {
        if (i == int(lists.size())) { sums.push_back(s); return; }
        for (int j = 0; j < int(lists[i].size()); ++j) { visit(visit, i + 1, T(s + lists[i][j])); }};
    if (any) { visit(visit, 0, T(0)); }
    std::stable_sort(sums.begin(), sums.end(), cmp);
    auto got = kSmallestSortedListSums(lists, k, cmp);
    int want = min(k, int(sums.size()));
    same(int(got.sums.size()), want, "list output count");
    std::set<vector<int>> seen;
    for (int e = 0; e < want; ++e) {
        sameUnder(got.sums[e], sums[e], cmp, "list sums equal sorted brute force");
        auto c = got.choice(e); T t = T(0);
        check(c.size() == lists.size(), "choice length");
        for (int i = 0; i < int(c.size()); ++i) {
            check(0 <= c[i] && c[i] < int(lists[i].size()), "choice in range");
            t += lists[i][c[i]];}
        sameUnder(t, got.sums[e], cmp, "choice sum equals reported sum");
        check(seen.insert(c).second, "choices pairwise distinct");}}

// Lawler against all feasible 0/1 vectors: value = sum c_i x_i + d * popcount^2, feasible when sum w_i x_i <= cap.
// With shuffle the oracle returns a uniformly random optimum, so a re-solve may return another vector of equal value.
template<class C> void lawlerCase(const vector<lng> &c, const vector<lng> &w, lng cap, lng d, int k, C cmp, bool shuffle = false) {
    ++cases; int n = int(c.size());
    context = "lawler c=" + show(c) + " w=" + show(w) + " cap=" + std::to_string(cap) + " d=" + std::to_string(d) + " k=" + std::to_string(k);
    vector<lng> val(1 << n); vector<char> ok(1 << n);
    for (int mask = 0; mask < 1 << n; ++mask) {
        lng s = 0, p = std::popcount(uint(mask));
        val[mask] = d * p * p;
        for (int i = 0; i < n; ++i) { if (mask >> i & 1) { val[mask] += c[i]; s += w[i]; } }
        ok[mask] = s <= cap;}
    auto value = [&](int mask) { return val[mask]; };
    auto feasible = [&](int mask) { return bool(ok[mask]); };
    int calls = 0;
    std::mt19937_64 tie(test_seed + cases);
    auto oracle = [&](const vector<int> &fixed) -> std::optional<pair<lng, vector<int>>> {
        ++calls; check(int(fixed.size()) == n, "oracle receives n constraints");
        int best = -1, ties = 0;
        for (int mask = 0; mask < 1 << n; ++mask) {
            bool ok = feasible(mask);
            for (int i = 0; i < n && ok; ++i) { ok = fixed[i] == -1 || fixed[i] == (mask >> i & 1); }
            if (!ok) { continue; }
            if (best < 0 || cmp(value(mask), value(best))) { best = mask; ties = 1; }
            else if (shuffle && !cmp(value(best), value(mask)) && tie() % ++ties == 0) { best = mask; }}
        if (best < 0) { return std::nullopt; }
        vector<int> x(n);
        for (int i = 0; i < n; ++i) { x[i] = best >> i & 1; }
        return pair(value(best), x);};
    vector<lng> all;
    for (int mask = 0; mask < 1 << n; ++mask) { if (feasible(mask)) { all.push_back(value(mask)); } }
    std::stable_sort(all.begin(), all.end(), cmp);
    auto got = lawlerKBest(n, k, oracle, cmp);
    int want = min(k, int(all.size()));
    same(int(got.size()), want, "lawler output count");
    std::set<vector<int>> seen;
    for (int e = 0; e < want; ++e) {
        same(got[e].first, all[e], "lawler values equal sorted brute force");
        int mask = 0;
        for (int i = 0; i < n; ++i) { check(got[e].second[i] == 0 || got[e].second[i] == 1, "0/1 solution"); mask |= got[e].second[i] << i; }
        check(feasible(mask) && value(mask) == got[e].first, "lawler solution feasible with its value");
        check(seen.insert(got[e].second).second, "lawler solutions distinct");}
    check(calls <= 1 + want * (n + 1), "lawler oracle calls within k * (n + 1) + 1");}

template<class C> void smallCases(const string &mode, C cmp, const string &name) {
    int top = mode == "quick" ? 3 : mode == "full" ? 4 : 5;
    vector<lng> alphabet{-2, 0, 1, 1, 3};
    for (int n = 0; n <= top; ++n) {
        vector<lng> a(n);
        auto visit = [&](auto &&visit, int i) -> void {
            if (i == n) {
                // Outputs for k are prefixes of those for 2^n, so n = 5 keeps k <= 6 and 2^n - 1 .. 2^n + 1.
                for (int k = 0; k <= (1 << n) + 1; k += n == 5 && 6 <= k && k < (1 << n) - 1 ? (1 << n) - 1 - k : 1) {
                    subsetCase(a, k, cmp);
                    for (int m = 0; m <= n + 1; ++m) { fixedCase(a, m, k, cmp); }
                    for (int L = 1; L <= n + 1; ++L) { for (int R = L; R <= n + 1; ++R) { subarrayCase(a, k, L, R, cmp); } }}
                for (int m = 0; m <= n; ++m) {
                    vector<lng> b(a.begin(), a.begin() + m);
                    for (int k = 0; k <= n * m + 1; ++k) { pairCase(a, b, k, cmp); }}
                return;}
            for (lng x : alphabet) { a[i] = x; visit(visit, i + 1); }};
        visit(visit, 0);}
    for (const auto &lists : vector<vector<vector<lng>>>{{}, {{}}, {{1}, {}}, {{5}}, {{1, 1}, {1, 1}}, {{0, 2}, {-1}, {3, 3, 4}},
                                                         {{-2, 0, 1}, {1, 3}, {0}, {2, 2}}, {{1, 2, 3}, {1, 2, 3}, {1, 2, 3}}}) {
        for (int k = 0; k <= 30; ++k) { listCase(lists, k, cmp); }}
    for (int k = 0; k <= 10; ++k) {
        for (bool shuffle : {false, true}) {
            lawlerCase({}, {}, 0, 0, k, cmp, shuffle); lawlerCase({3}, {5}, 4, 0, k, cmp, shuffle);
            lawlerCase({1, -2, 3}, {1, 1, 1}, 2, 1, k, cmp, shuffle); lawlerCase({0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, k, cmp, shuffle);}}
    cout << "PASS exhaustive " << name << " alphabet {-2,0,1,1,3} n<=" << top << ", every k (n = 5: k <= 6 and 31..33), m, L <= R, list and Lawler fixtures\n";}

template<class C> void randomCases(const string &mode, C cmp, const string &name) {
    std::mt19937_64 gen(test_seed ^ ulng(std::hash<string>{}(name)));
    int rounds = (mode == "quick" ? 150 : mode == "full" ? 2000 : 12000) / (name.starts_with("stateful") ? 4 : 1);
    auto num = [&](lng r) { return lng(gen() % (2 * r + 1)) - r; };
    for (int round = 0; round < rounds; ++round) {
        lng r = round % 4 == 0 ? 1000000000000LL : 6;
        int n = int(gen() % 11), m = int(gen() % 9);
        vector<lng> a(n), b(m);
        for (auto &x : a) { x = num(r); }
        for (auto &x : b) { x = num(r); }
        pairCase(a, b, int(gen() % (n * m + 3)), cmp);
        int L = 1 + int(gen() % (n + 1));
        subarrayCase(a, int(gen() % (n * n + 3)), L, L + int(gen() % (n + 1)), cmp);
        subsetCase(a, int(gen() % ((1 << n) + 3)), cmp);
        fixedCase(a, int(gen() % (n + 2)), int(gen() % ((1 << n) + 3)), cmp);
        vector<vector<lng>> lists(gen() % 5);
        for (auto &l : lists) { l.resize(gen() % 5); for (auto &x : l) { x = num(r); } }
        listCase(lists, int(gen() % 700), cmp);
        int q = int(gen() % 7);
        vector<lng> c(q), w(q);
        for (int i = 0; i < q; ++i) { c[i] = num(r); w[i] = lng(gen() % 6); }
        lawlerCase(c, w, lng(gen() % (3 * q + 2)), num(3), int(gen() % 40), cmp, round % 2);}
    cout << "PASS seeded " << name << " rounds=" << rounds << " max n=10, values up to 1e12, all six operations\n";}

void typeCases() {
    context = "int and double element types";
    vector<int> ai{3, -1, 4, -1, 5};
    auto si = kSmallestSubsetSums(ai, 32);
    vector<int> sums;
    for (int mask = 0; mask < 32; ++mask) { int s = 0; for (int i = 0; i < 5; ++i) { if (mask >> i & 1) { s += ai[i]; } } sums.push_back(s); }
    sort(sums.begin(), sums.end());
    check(si.sums == sums, "int subset sums");
    vector<double> ad{0.5, 1.5, 2.5}, bd{-1.0, 0.25};
    auto pd = kSmallestPairSums(ad, bd, 6);
    vector<pair<int, int>> want{{0, 0}, {1, 0}, {0, 1}, {2, 0}, {1, 1}, {2, 1}};
    check(pd == want, "double pair sums");
    auto ld = kSmallestSortedListSums(vector<vector<double>>{{1.0, 0.5}, {2.0, 0.25}}, 4, std::greater<>());
    check(ld.sums == vector<double>{3.0, 2.5, 1.25, 0.75}, "double list sums under greater");
    cout << "PASS element types int and double\n";}

void largeCases(const string &mode) {
    std::mt19937_64 gen(test_seed ^ 5);
    int n = DEBUG_CONTAINERS ? 1000 : mode == "quick" ? 2000 : mode == "full" ? 100000 : 300000, k = n;
    vector<lng> a(n), b(n);
    for (auto &x : a) { x = lng(gen() % 1000000000); }
    for (auto &x : b) { x = lng(gen() % 1000000000); }
    sort(a.begin(), a.end()); sort(b.begin(), b.end());
    context = "large pair sums n=" + std::to_string(n);
    auto got = kSmallestPairSums(a, b, k);
    // Certificate: sorted, distinct, and exactly the pairs below the last sum are present (two-pointer count).
    lng last = a[got.back().first] + b[got.back().second];
    lng below = 0, inside = 0;
    for (int i = 0, j = n; i < n; ++i) { while (j > 0 && a[i] + b[j - 1] >= last) { --j; } below += j; }
    std::set<pair<int, int>> seen(got.begin(), got.end());
    check(int(got.size()) == k, "large pair count");
    for (int e = 0; e < k; ++e) {
        check(0 <= got[e].first && got[e].first < n && 0 <= got[e].second && got[e].second < n, "large pair indices in range");
        lng s = a[got[e].first] + b[got[e].second];
        if (e) { check(a[got[e - 1].first] + b[got[e - 1].second] <= s, "large pair sums sorted"); }
        inside += s < last;}
    check(int(seen.size()) == k && below == inside && below < k, "large pair sums certificate");
    context = "large subarray sums n=" + std::to_string(n);
    vector<lng> c(n);
    for (auto &x : c) { x = lng(gen() % 2000000001) - 1000000000; }
    auto ranges = kSmallestSubarraySums(c, k, 1, n / 3 + 1, std::greater<>());
    vector<lll> pre(n + 1);
    for (int i = 0; i < n; ++i) { pre[i + 1] = pre[i] + c[i]; }
    std::set<pair<int, int>> rs(ranges.begin(), ranges.end());
    for (int e = 0; e < k; ++e) {
        auto [l, r] = ranges[e];
        check(0 <= l && l < r && r <= n && r - l <= n / 3 + 1, "large subarray range valid");
        if (e) { check(pre[ranges[e - 1].second] - pre[ranges[e - 1].first] >= pre[r] - pre[l], "large subarray sums nonincreasing"); }}
    check(int(rs.size()) == k, "large subarray ranges distinct");
    context = "large subset sums n=" + std::to_string(n);
    auto subsets = kSmallestSubsetSums(c, k);
    auto fixed = kSmallestFixedSizeSubsetSums(c, n / 2, k);
    for (int e = 0; e < k; ++e) {
        if (e) { check(subsets.sums[e - 1] <= subsets.sums[e] && fixed.sums[e - 1] <= fixed.sums[e], "large subset sums nondecreasing"); }}
    for (int e : {0, 1, k / 2, k - 1}) {
        auto s = subsets.subset(e), f = fixed.subset(e);
        lll x = 0, y = 0;
        for (int i : s) { x += c[i]; }
        for (int i : f) { y += c[i]; }
        check(x == subsets.sums[e] && y == fixed.sums[e] && int(f.size()) == n / 2, "large subset witnesses");}
    vector<lng> sorted = c;
    sort(sorted.begin(), sorted.end());
    check(fixed.sums[0] == accumulate(sorted.begin(), sorted.begin() + n / 2, lng(0)), "large fixed-size minimum");
    context = "large sorted lists";
    vector<vector<lng>> lists(n / 10);
    for (auto &l : lists) { l.resize(1 + gen() % 10); for (auto &x : l) { x = lng(gen() % 1000); } sort(l.begin(), l.end()); }
    auto ls = kSmallestSortedListSums(lists, k);
    lng base = 0;
    for (auto &l : lists) { base += l[0]; }
    check(ls.sums[0] == base, "large list minimum");
    for (int e = 1; e < k; ++e) { check(ls.sums[e - 1] <= ls.sums[e], "large list sums nondecreasing"); }
    for (int e : {1, k / 2, k - 1}) {
        auto ch = ls.choice(e); lng s = 0;
        for (int i = 0; i < int(lists.size()); ++i) { s += lists[i][ch[i]]; }
        check(s == ls.sums[e], "large list witness");}
    cout << "PASS large n=k=" << n << " pair-sum certificate, subarray/subset/fixed/list order and sampled witnesses\n";}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i];
            vector<lng> a{1, 2};
            auto none = [](const vector<int> &) -> std::optional<pair<lng, vector<int>>> { return std::nullopt; };
            if (probe == "pair-k") { kSmallestPairSums(a, a, -1); }
            if (probe == "subarray-k") { kSmallestSubarraySums(a, -1, 1, 1); }
            if (probe == "subarray-low") { kSmallestSubarraySums(a, 1, 0, 1); }
            if (probe == "subarray-order") { kSmallestSubarraySums(a, 1, 2, 1); }
            if (probe == "subset-k") { kSmallestSubsetSums(a, -1); }
            if (probe == "subset-witness") { kSmallestSubsetSums(a, 2).subset(2); }
            if (probe == "subset-witness-negative") { kSmallestSubsetSums(a, 2).subset(-1); }
            if (probe == "fixed-m") { kSmallestFixedSizeSubsetSums(a, -1, 1); }
            if (probe == "fixed-k") { kSmallestFixedSizeSubsetSums(a, 1, -1); }
            if (probe == "lists-k") { kSmallestSortedListSums(vector<vector<lng>>{a}, -1); }
            if (probe == "lists-choice") { kSmallestSortedListSums(vector<vector<lng>>{a}, 1).choice(1); }
            if (probe == "lists-choice-negative") { kSmallestSortedListSums(vector<vector<lng>>{a}, 1).choice(-1); }
            if (probe == "lawler-n") { lawlerKBest(-1, 1, none); }
            if (probe == "lawler-k") { lawlerKBest(1, -1, none); }
            // An oracle that cannot solve its own output again.
            int calls = 0;
            auto flaky = [&](const vector<int> &) -> std::optional<pair<lng, vector<int>>> {
                if (calls++) { return std::nullopt; }
                return pair<lng, vector<int>>(0, {0});};
            if (probe == "lawler-resolve") { lawlerKBest(1, 1, flaky); }
            // An oracle whose re-solve reports another value for the same vector.
            auto drift = [&](const vector<int> &) -> std::optional<pair<lng, vector<int>>> { return pair<lng, vector<int>>(calls++, {0}); };
            if (probe == "lawler-value") { lawlerKBest(1, 1, drift); }
            return 1;}}
    if ((DEBUG_CONTAINERS || SANITIZED) && mode == "stress") { mode = "full"; }
    smallCases(mode, std::less<>(), "less"); smallCases(mode, std::greater<>(), "greater");
    randomCases(mode, std::less<>(), "less"); randomCases(mode, std::greater<>(), "greater");
    randomCases(mode, Scaled{1}, "stateful less"); randomCases(mode, Scaled{-1}, "stateful greater");
    typeCases(); largeCases(mode);
    cout << "PASS kbest enumeration cases=" << cases << " checks=" << checks << " seed=" << test_seed << '\n';}
