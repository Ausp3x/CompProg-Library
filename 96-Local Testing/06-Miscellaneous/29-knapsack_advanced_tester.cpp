#include "../../06-Miscellaneous/29-knapsack_advanced.hpp"
#include "../../01-Core/05-modint.hpp"

#ifdef _GLIBCXX_DEBUG
constexpr bool DEBUG_CONTAINERS = true;  // -O0 debug containers: large fixtures shrink and stress runs as full
#else
constexpr bool DEBUG_CONTAINERS = false;
#endif
#ifdef __SANITIZE_ADDRESS__
constexpr bool SANITIZED = true;  // ASan stress would exceed the runner's 180 s limit, so it runs as full
#else
constexpr bool SANITIZED = false;
#endif
using Item = KnapsackItem;
using Item2 = KnapsackItem2D;
using M1 = ModInt<998244353>;
using M2 = ModInt<1000000007>;
constexpr lll U = KNAPSACK_UNREACHABLE;
ulng test_seed = 0;
lng checks = 0, cases = 0;
string mode = "full", context;
std::mt19937_64 gen;

string show(lll x) {
    if (x == U) { return "UNREACHABLE"; }
    if (x < 0) { return "-" + show(-x); }
    string s;
    do { s += char('0' + int(x % 10)); x /= 10; } while (x);
    return string(s.rbegin(), s.rend());}
string show(lng x) { return show(lll(x)); }
string show(int x) { return show(lll(x)); }
template<class T> requires requires(T a) { a.val(); } string show(T x) { return show(lll(x.val())); }
string show(const Item &x) { return "{" + show(x.weight) + "," + show(x.value) + "," + show(x.count) + "}"; }
string show(const Item2 &x) { return "{" + show(x.weight1) + "," + show(x.weight2) + "," + show(x.value) + "," + show(x.count) + "}"; }
template<class A, class B> string show(const pair<A, B> &p);
template<class T> string show(const vector<T> &a);
template<class A, class B> string show(const pair<A, B> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
template<class T> string show(const vector<T> &a) {
    string s = "[";
    for (const auto &x : a) { s += show(x) + ","; }
    return s + "]";}
void fail(const char *what, const string &want, const string &got) {
    cerr << "FAIL seed=" << test_seed << " mode=" << mode << " smallest known reproducer=" << context << " operation=" << what
         << " expected=" << want << " actual=" << got << '\n';
    std::exit(1);}
void check(bool ok, const char *what) { ++checks; if (!ok) { fail(what, "true", "false"); } }
template<class A> void same(const A &got, const A &want, const char *what) { ++checks; if (!(got == want)) { fail(what, show(want), show(got)); } }
int pick(int quick, int full, int stress) { return mode == "quick" ? quick : mode == "full" ? full : stress; }
lng num(lng r) { return lng(gen() % ulng(2 * r + 1)) - r; }
int randomCount() {
    switch (gen() % 7) {
        case 0: return -1;
        case 1: return 0;
        case 2: return 1;
        case 3: return 2;
        case 4: return INT_MAX;
        default: return 1 + int(gen() % 6);}}
Item randomItem(int cap, lng r, bool zero_one = false) {
    ulng kind = gen() % 10;
    Item x{kind == 0 ? 0 : kind == 1 ? cap + 1 + lng(gen() % 5) : 1 + lng(gen() % ulng(max(1, cap / 2))), num(r), zero_one ? int(gen() % 4 != 0) : randomCount()};
    if (!x.weight && x.count == -1) { x.value = -abs(x.value); }
    return x;}

// Every multiplicity vector of weight <= cap; unless exact, a zero-weight item with count above 3 tries only 0 and count, enough for the linear max oracles.
template<class F> void enumerate(const vector<Item> &items, lng cap, F f, bool exact = false) {
    int n = int(items.size());
    auto go = [&](auto &&go, int i, lll w, lll v) -> void {
        if (i == n) { f(w, v); return; }
        const auto &x = items[i];
        vector<lng> options;
        if (x.weight) {
            for (lng c = 0; (x.count == -1 || c <= x.count) && w + lll(c) * x.weight <= cap; ++c) { options.push_back(c); }}
        else if (exact || x.count <= 3) {
            for (lng c = 0; c <= max(x.count, 0); ++c) { options.push_back(c); }}
        else { options = {0, x.count}; }
        for (lng c : options) { go(go, i + 1, w + lll(c) * x.weight, v + lll(c) * x.value); }};
    go(go, 0, 0, 0);}
vector<lll> bruteExact(int cap, const vector<Item> &items) {
    vector<lll> res(cap + 1, U);
    enumerate(items, cap, [&](lll w, lll v) { res[int(w)] = max(res[int(w)], v); });
    return res;}
// Copy expansion: every copy is a separate 0/1 item.
vector<lll> naiveExact(int cap, const vector<Item> &items) {
    vector<lll> dp(cap + 1, U); dp[0] = 0;
    for (const auto &x : items) {
        if (!x.weight) {
            if (x.count > 0 && x.value > 0) { for (auto &d : dp) { if (d != U) { d += lll(x.value) * x.count; } } }
            continue;}
        lng copies = x.weight > cap ? 0 : x.count == -1 ? cap / x.weight : min(lng(x.count), cap / x.weight);
        for (lng c = 0; c < copies; ++c) {
            for (lng j = cap; j >= x.weight; --j) {
                if (dp[j - x.weight] != U) { dp[j] = max(dp[j], dp[j - x.weight] + x.value); }}}}
    return dp;}

// Exact tables: queue and SMAWK against copy expansion; 0/1 inputs also as singleton groups and a forest of roots; 2D as diagonal and flat.
void tableCase(int cap, const vector<Item> &items, bool brute) {
    ++cases;
    context = "capacity=" + show(cap) + " items=" + show(items);
    auto want = naiveExact(cap, items);
    if (brute) { same(want, bruteExact(cap, items), "copy-expansion oracle versus enumeration"); }
    same(boundedKnapsackMonotoneQueue(cap, items), want, "boundedKnapsackMonotoneQueue");
    same(smallWeightKnapsack(cap, items), want, "smallWeightKnapsack");
    vector<Item2> diagonal, flat;
    for (const auto &x : items) { diagonal.push_back({x.weight, x.weight, x.value, x.count}); flat.push_back({x.weight, 0, x.value, x.count}); }
    auto d = twoDimensionalKnapsack(cap, cap, diagonal), f = twoDimensionalKnapsack(cap, 0, flat);
    for (int i = 0; i <= cap; ++i) {
        for (int j = 0; j <= cap; ++j) { same(d[i][j], i == j ? want[i] : U, "twoDimensionalKnapsack diagonal embedding"); }
        same(f[i][0], want[i], "twoDimensionalKnapsack flat embedding");}
    if (std::all_of(items.begin(), items.end(), [](const Item &x) { return 0 <= x.count && x.count <= 1; })) {
        vector<vector<Item>> groups;
        for (const auto &x : items) { groups.push_back({x}); }
        same(groupKnapsack(cap, groups), want, "groupKnapsack singleton groups");
        same(treeKnapsack(cap, vector<int>(items.size(), -1), items), want, "treeKnapsack forest of roots");}}

void exhaustiveTables() {
    vector<Item> alphabet;
    for (lng w : {0, 1, 2, 3}) {
        for (lng v : {-2, 0, 1, 3}) {
            for (int c : {-1, 0, 1, 2, INT_MAX}) {
                if (w || c != -1 || v <= 0) { alphabet.push_back({w, v, c}); }}}}
    int top = pick(5, 7, 7);
    for (int cap = 0; cap <= top; ++cap) {
        tableCase(cap, {}, true);
        for (const auto &x : alphabet) {
            tableCase(cap, {x}, true);
            for (const auto &y : alphabet) { tableCase(cap, {x, y}, true); }}}
    cout << "PASS exhaustive tables: " << alphabet.size() << "-item alphabet, n <= 2, capacity <= " << top << ", enumeration oracle\n";}

void randomTables() {
    int rounds = pick(300, 3000, 20000);
    for (int round = 0; round < rounds; ++round) {
        int n = int(gen() % 7), cap = round % 5 == 0 ? int(gen() % 300) : int(gen() % 30);
        lng r = round % 3 == 0 ? lng(1) << 61 : 6;
        vector<Item> items;
        for (int i = 0; i < n; ++i) { items.push_back(randomItem(cap, r, round % 4 == 0)); }
        tableCase(cap, items, n <= 4 && cap <= 12);}
    // Few distinct weights with many copies, the SMAWK regime.
    int big = DEBUG_CONTAINERS ? 600 : pick(600, 5000, 20000);
    for (int round = 0; round < 6; ++round) {
        vector<Item> items;
        for (int i = 0; i < 20; ++i) { items.push_back({20 + lng(gen() % 3) * 7, num(1000000), round % 2 ? -1 : int(gen() % 300)}); }
        context = "large tables capacity=" + show(big) + " items=" + show(items);
        auto want = naiveExact(big, items);
        same(boundedKnapsackMonotoneQueue(big, items), want, "boundedKnapsackMonotoneQueue large");
        same(smallWeightKnapsack(big, items), want, "smallWeightKnapsack large");}
    cout << "PASS seeded tables rounds=" << rounds << " (n <= 6, capacity < 300, values up to 2^61) and capacity " << big << " with 3 distinct weights\n";}

vector<lll> bruteGroup(int cap, const vector<vector<Item>> &groups) {
    vector<lll> res(cap + 1, U);
    auto go = [&](auto &&go, int g, lll w, lll v) -> void {
        if (w > cap) { return; }
        if (g == int(groups.size())) { res[int(w)] = max(res[int(w)], v); return; }
        go(go, g + 1, w, v);
        for (const auto &x : groups[g]) {
            if (x.count) { go(go, g + 1, w + x.weight, v + x.value); }}};
    go(go, 0, 0, 0);
    return res;}
vector<vector<lll>> brute2D(int c1, int c2, const vector<Item2> &items) {
    vector<vector<lll>> res(c1 + 1, vector<lll>(c2 + 1, U));
    int n = int(items.size());
    auto go = [&](auto &&go, int i, lll a, lll b, lll v) -> void {
        if (i == n) { res[int(a)][int(b)] = max(res[int(a)][int(b)], v); return; }
        const auto &x = items[i];
        if (!x.weight1 && !x.weight2) {
            vector<lng> options{0};
            if (x.count > 0) { options.push_back(x.count); }
            for (lng c : options) { go(go, i + 1, a, b, v + lll(c) * x.value); }
            return;}
        for (lng c = 0; (x.count == -1 || c <= x.count) && a + lll(c) * x.weight1 <= c1 && b + lll(c) * x.weight2 <= c2; ++c) {
            go(go, i + 1, a + lll(c) * x.weight1, b + lll(c) * x.weight2, v + lll(c) * x.value);}};
    go(go, 0, 0, 0, 0);
    return res;}
void groupAnd2DCases() {
    int rounds = pick(200, 2000, 12000);
    for (int round = 0; round < rounds; ++round) {
        int cap = int(gen() % 16), g = int(gen() % 5);
        lng r = round % 3 == 0 ? lng(1) << 61 : 5;
        vector<vector<Item>> groups(g);
        for (auto &group : groups) {
            group.resize(gen() % 4);
            for (auto &x : group) { x = randomItem(cap, r, true); }}
        ++cases;
        context = "groupKnapsack capacity=" + show(cap) + " groups=" + show(groups);
        same(groupKnapsack(cap, groups), bruteGroup(cap, groups), "groupKnapsack");
        int c1 = int(gen() % 7), c2 = int(gen() % 7), n = int(gen() % 5);
        vector<Item2> items(n);
        for (auto &x : items) {
            x = {lng(gen() % 4), lng(gen() % 4), num(r), randomCount()};
            if (gen() % 8 == 0) { x.weight1 = c1 + 1; }
            if (!x.weight1 && !x.weight2 && x.count == -1) { x.value = -abs(x.value); }}
        ++cases;
        context = "twoDimensionalKnapsack capacities=" + show(c1) + "," + show(c2) + " items=" + show(items);
        same(twoDimensionalKnapsack(c1, c2, items), brute2D(c1, c2, items), "twoDimensionalKnapsack");}
    cout << "PASS seeded groupKnapsack and twoDimensionalKnapsack rounds=" << rounds << " against enumeration\n";}

vector<lll> bruteTree(int cap, const vector<int> &parent, const vector<Item> &items) {
    int n = int(parent.size());
    vector<lll> res(cap + 1, U);
    for (int mask = 0; mask < 1 << n; ++mask) {
        lll w = 0, v = 0;
        bool ok = true;
        for (int i = 0; i < n && ok; ++i) {
            if (!(mask >> i & 1)) { continue; }
            ok = items[i].count == 1 && (parent[i] < 0 || (mask >> parent[i] & 1));
            w += items[i].weight; v += items[i].value;}
        if (ok && w <= cap) { res[int(w)] = max(res[int(w)], v); }}
    return res;}
// Subtree tables merged child by child, O(n * W^2); independent of the heavy-last preorder.
vector<lll> naiveTree(int cap, const vector<int> &parent, const vector<Item> &items) {
    int n = int(parent.size());
    vector<vector<int>> child(n);
    vector<int> order;
    for (int v = 0; v < n; ++v) {
        if (parent[v] < 0) { order.push_back(v); }
        else { child[parent[v]].push_back(v); }}
    for (int i = 0; i < int(order.size()); ++i) {
        for (int c : child[order[i]]) { order.push_back(c); }}
    auto merge = [&](vector<lll> &a, const vector<lll> &b) {
        vector<lll> r = a;
        for (int i = 0; i <= cap; ++i) {
            if (a[i] == U) { continue; }
            for (int j = 0; i + j <= cap; ++j) {
                if (b[j] != U) { r[i + j] = max(r[i + j], a[i] + b[j]); }}}
        a.swap(r);};
    vector<vector<lll>> t(n);
    for (int i = n - 1; i >= 0; --i) {
        int v = order[i];
        t[v].assign(cap + 1, U);
        if (items[v].count == 1 && items[v].weight <= cap) { t[v][items[v].weight] = items[v].value; }
        for (int c : child[v]) { merge(t[v], t[c]); vector<lll>().swap(t[c]); }}
    vector<lll> res(cap + 1, U); res[0] = 0;
    for (int v = 0; v < n; ++v) {
        if (parent[v] < 0) { merge(res, t[v]); }}
    return res;}
vector<int> randomForest(int n, int shape) {
    vector<int> perm(n), parent(n, -1);
    iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), gen);
    for (int i = 1; i < n; ++i) {
        int p = shape == 0 ? int(gen() % i) : shape == 1 ? i - 1 : shape == 2 ? 0 : (i - 1) / 2;
        if (shape != 0 || gen() % 6) { parent[perm[i]] = perm[p]; }}
    return parent;}
void treeCases() {
    int rounds = pick(200, 2000, 12000);
    for (int round = 0; round < rounds; ++round) {
        bool small = round % 2 == 0;
        int n = small ? int(gen() % 12) : int(gen() % 60), cap = small ? int(gen() % 20) : int(gen() % 40);
        auto parent = randomForest(n, round % 4);
        vector<Item> items(n);
        for (auto &x : items) { x = randomItem(cap, round % 3 ? 6 : lng(1) << 61, true); }
        ++cases;
        context = "treeKnapsack capacity=" + show(cap) + " parent=" + show(parent) + " items=" + show(items);
        auto want = naiveTree(cap, parent, items);
        if (small) { same(want, bruteTree(cap, parent, items), "merge oracle versus closed-set enumeration"); }
        same(treeKnapsack(cap, parent, items), want, "treeKnapsack");}
    // Long path (closed sets are prefixes) and a large random tree: n * (W + 1) work, O(W * log(n)) live tables under the memory cap.
    int n = DEBUG_CONTAINERS ? 5000 : pick(5000, 200000, 300000), cap = DEBUG_CONTAINERS ? 50 : pick(50, 1000, 1000);
    auto parent = randomForest(n, 1);
    vector<Item> items(n);
    for (auto &x : items) { x = {lng(gen() % 3), num(1000000), 1}; }
    vector<int> seq;
    for (int v = 0; v < n; ++v) {
        if (parent[v] < 0) { seq.push_back(v); }}
    vector<int> next(n, -1);
    for (int v = 0; v < n; ++v) {
        if (parent[v] >= 0) { next[parent[v]] = v; }}
    while (next[seq.back()] >= 0) { seq.push_back(next[seq.back()]); }
    items[seq[n / 2]].count = 0;
    vector<lll> want(cap + 1, U); want[0] = 0;
    lll w = 0, v = 0;
    for (int i = 0; i < n / 2 && w + items[seq[i]].weight <= cap; ++i) {
        w += items[seq[i]].weight; v += items[seq[i]].value; want[int(w)] = max(want[int(w)], v);}
    context = "treeKnapsack long path n=" + show(n) + " capacity=" + show(cap);
    same(treeKnapsack(cap, parent, items), want, "treeKnapsack long path");
    int m = DEBUG_CONTAINERS ? 2000 : pick(2000, 50000, 100000), wide = 20;
    parent = randomForest(m, 0);
    items.resize(m);
    for (auto &x : items) { x = {1 + lng(gen() % 4), num(1000), int(gen() % 10 != 0)}; }
    context = "treeKnapsack random forest n=" + show(m) + " capacity=" + show(wide);
    same(treeKnapsack(wide, parent, items), naiveTree(wide, parent, items), "treeKnapsack large forest");
    // Caterpillar: with the heavy child first every spine vertex would keep its own skip table, spine * (W + 1) * 16 bytes, above the cap in full and stress.
    int spine = DEBUG_CONTAINERS ? 2000 : pick(2000, 150000, 150000), deep = DEBUG_CONTAINERS ? 50 : pick(50, 2000, 2000);
    parent.assign(2 * spine, -1); items.assign(2 * spine, {});
    for (int i = 0; i < spine; ++i) {
        parent[i] = i - 1; parent[spine + i] = i;
        items[i] = {lng(gen() % 3), num(1000), 1}; items[spine + i] = {lng(gen() % 3), num(1000), 1};}
    // Closed sets are a spine prefix plus any leaves hanging from it: extend the prefix one vertex at a time.
    vector<lll> prefix(deep + 1, U), best(deep + 1, U);
    prefix[0] = 0; best[0] = 0;
    for (int i = 0; i < spine; ++i) {
        vector<lll> next(deep + 1, U);
        for (int j = int(items[i].weight); j <= deep; ++j) {
            if (prefix[j - items[i].weight] != U) { next[j] = prefix[j - items[i].weight] + items[i].value; }}
        for (int j = deep; j >= int(items[spine + i].weight); --j) {
            if (next[j - items[spine + i].weight] != U) { next[j] = max(next[j], next[j - items[spine + i].weight] + items[spine + i].value); }}
        prefix.swap(next);
        for (int j = 0; j <= deep; ++j) { best[j] = max(best[j], prefix[j]); }}
    context = "treeKnapsack caterpillar spine=" + show(spine) + " capacity=" + show(deep);
    same(treeKnapsack(deep, parent, items), best, "treeKnapsack caterpillar");
    // Star: every leaf but the last is a light child; keeping each leaf's skip table would take leaves * (W + 1) * 16 bytes, above the cap in full and stress.
    int leaves = spine;
    parent.assign(leaves + 1, 0); parent[0] = -1; items.assign(leaves + 1, {});
    vector<lll> knap(deep + 1, U), star(deep + 1, U);
    knap[0] = 0; star[0] = 0;
    for (int i = 0; i <= leaves; ++i) {
        items[i] = {lng(gen() % 3), num(1000), 1};
        for (int j = deep; i > 0 && j >= int(items[i].weight); --j) {
            if (knap[j - items[i].weight] != U) { knap[j] = max(knap[j], knap[j - items[i].weight] + items[i].value); }}}
    // Nonempty closed sets hold the root plus any leaves.
    for (int j = int(items[0].weight); j <= deep; ++j) {
        if (knap[j - items[0].weight] != U) { star[j] = max(star[j], knap[j - items[0].weight] + items[0].value); }}
    context = "treeKnapsack star leaves=" + show(leaves) + " capacity=" + show(deep);
    same(treeKnapsack(deep, parent, items), star, "treeKnapsack star");
    cout << "PASS treeKnapsack rounds=" << rounds << " (enumeration n < 12, merge oracle n < 60), path n=" << n << " W=" << cap << ", forest n=" << m << ", caterpillar spine=" << spine << " and star leaves=" << leaves << " W=" << deep << "\n";}

template<class T> vector<T> naiveCounts(int cap, const vector<Item> &items) {
    vector<T> ways(cap + 1, T(0)); ways[0] = T(1);
    for (const auto &x : items) {
        if (!x.count) { continue; }
        if (!x.weight) {
            T f = T(lng(x.count) + 1);
            for (auto &y : ways) { y = T(y * f); }
            continue;}
        vector<T> next(cap + 1, T(0));
        for (int j = 0; j <= cap; ++j) {
            for (lng c = 0; (x.count == -1 || c <= x.count) && c * x.weight <= j; ++c) { next[j] = T(next[j] + ways[j - c * x.weight]); }}
        ways = std::move(next);}
    return ways;}
// Running sums along each residue class, O(W) per item.
template<class T> vector<T> slidingCounts(int t, const vector<Item> &items) {
    vector<T> ways(t + 1, T(0)), next(t + 1, T(0)); ways[0] = T(1);
    for (const auto &x : items) {
        if (!x.count) { continue; }
        if (!x.weight) {
            T f = T(lng(x.count) + 1);
            for (auto &y : ways) { y = T(y * f); }
            continue;}
        lll span = x.count == -1 ? lll(t) + 1 : (lll(x.count) + 1) * x.weight;
        for (int j = 0; j <= t; ++j) {
            next[j] = ways[j];
            if (j >= x.weight) { next[j] = T(next[j] + next[j - x.weight]); }
            if (span <= j) { next[j] = T(next[j] - ways[j - int(span)]); }}
        swap(ways, next);}
    return ways;}
template<class T> void removableRun(int cap, int ops) {
    RemovableKnapsackCounts<T> counts(cap);
    vector<Item> cur;
    for (int op = 0; op < ops; ++op) {
        ++cases;
        if (!cur.empty() && (cur.size() >= 10 || gen() % 3 == 0)) {
            int i = int(gen() % cur.size());
            counts.remove(cur[i]); cur.erase(cur.begin() + i);}
        else {
            Item x{1 + lng(gen() % ulng(cap + 3)), num(5), randomCount()};
            counts.add(x); cur.push_back(x);}
        context = "RemovableKnapsackCounts capacity=" + show(cap) + " current items=" + show(cur);
        same(counts.ways, naiveCounts<T>(cap, cur), "RemovableKnapsackCounts after add or remove");}}
template<class T> void fpsCase(int t, const vector<Item> &items) {
    ++cases;
    context = "knapsackCountsFps t=" + show(t) + " items=" + show(items);
    auto want = naiveCounts<T>(t, items);
    same(slidingCounts<T>(t, items), want, "sliding oracle versus naive product");
    same(knapsackCountsFps<T>(t, items), want, "knapsackCountsFps");}
void countingCases() {
    int rounds = pick(60, 500, 3000);
    for (int round = 0; round < rounds; ++round) {
        int cap = int(gen() % 31), ops = 30;
        removableRun<lng>(cap, ops); removableRun<M1>(cap, ops); removableRun<ModInt<2>>(cap, ops);
        int t = int(gen() % 13), n = int(gen() % 7);
        vector<Item> items(n);
        for (auto &x : items) {
            x = {lng(gen() % ulng(t + 3)), 0, randomCount()};
            if (!x.weight && x.count == -1) { x.count = int(gen() % 3); }}
        fpsCase<M1>(t, items); fpsCase<M2>(t, items);
        if (t < 7) { fpsCase<ModInt<7>>(t, items); }
        if (t < 2) { fpsCase<ModInt<2>>(t, items); }}
    int t = DEBUG_CONTAINERS ? 2000 : pick(2000, 30000, 100000), n = DEBUG_CONTAINERS ? 200 : pick(200, 1000, 2000);
    vector<Item> items(n);
    for (auto &x : items) { x = {lng(gen() % 3 == 0 ? gen() % 20 : gen() % ulng(t + 1)), 0, randomCount()}; }
    for (auto &x : items) {
        if (!x.weight && x.count == -1) { x.count = 5; }}
    context = "knapsackCountsFps large t=" + show(t) + " n=" + show(n);
    same(knapsackCountsFps<M1>(t, items), slidingCounts<M1>(t, items), "knapsackCountsFps large NTT modulus");
    same(knapsackCountsFps<M2>(t, items), slidingCounts<M2>(t, items), "knapsackCountsFps large arbitrary modulus");
    cout << "PASS counting rounds=" << rounds << " (30 random add/remove steps per type lng, ModInt 998244353, ModInt 2; FPS modulo 998244353, 1e9+7, 7, 2) and FPS t=" << t << " n=" << n << "\n";}

// Direct recurrence over the copies t of each item, with no window or doubling: the k best lists and the optimal counts.
vector<vector<lll>> naiveKth(int cap, const vector<Item> &items, int k, bool distinct) {
    vector<vector<lll>> dp(cap + 1, vector<lll>{0});
    for (const auto &x : items) {
        auto next = dp;
        for (int j = 0; j <= cap; ++j) {
            vector<lll> all;
            for (lng t = 0; (x.count == -1 || t <= x.count) && lll(t) * x.weight <= j; ++t) {
                for (lll v : dp[j - t * x.weight]) { all.push_back(v + lll(t) * x.value); }}
            sort(all.rbegin(), all.rend());
            if (distinct) { all.erase(unique(all.begin(), all.end()), all.end()); }
            if (int(all.size()) > k) { all.resize(k); }
            next[j] = all;}
        dp.swap(next);}
    return dp;}
template<class T> vector<pair<lll, T>> naiveOptimal(int cap, const vector<Item> &items) {
    vector<pair<lll, T>> dp(cap + 1, {0, T(1)});
    for (const auto &x : items) {
        auto next = dp;
        for (int j = 0; j <= cap; ++j) {
            for (lng t = 0; (x.count == -1 || t <= x.count) && lll(t) * x.weight <= j; ++t) {
                const auto &[v, c] = dp[j - t * x.weight];
                lll z = v + lll(t) * x.value;
                if (!t || z > next[j].first) { next[j] = {z, c}; }
                else if (z == next[j].first) { next[j].second = T(next[j].second + c); }}}
        dp.swap(next);}
    return dp;}
// An unlimited zero-weight item (value <= 0) as k + 3 copies: a selection with more has k at least as good ones with fewer, and none is optimal when value < 0.
vector<Item> finiteCopies(vector<Item> items, int k) {
    for (auto &x : items) {
        if (!x.weight && x.count == -1) { x.count = k + 3; }}
    return items;}
void selectionCase(int cap, const vector<Item> &items, int k) {
    ++cases;
    context = "capacity=" + show(cap) + " k=" + show(k) + " items=" + show(items);
    auto finite = finiteCopies(items, k);
    vector<pair<lll, lll>> sel;
    enumerate(finite, cap, [&](lll w, lll v) { sel.push_back({v, w}); }, true);
    sort(sel.rbegin(), sel.rend());
    for (bool distinct : {false, true}) {
        auto got = kthBestKnapsack(cap, items, k, distinct), naive = naiveKth(cap, finite, k, distinct);
        check(int(got.size()) == cap + 1, "kthBestKnapsack table size");
        for (int c = 0; c <= cap; ++c) {
            vector<lll> want;
            for (auto [v, w] : sel) {
                if (w <= c && int(want.size()) < k && (!distinct || want.empty() || want.back() != v)) { want.push_back(v); }}
            same(naive[c], want, "naive k-best oracle versus enumeration");
            same(got[c], want, distinct ? "kthBestKnapsack distinct values" : "kthBestKnapsack per selection");}}
    // Unlimited zero-weight zero-value items have infinitely many optima.
    if (std::ranges::any_of(items, [](const Item &x) { return !x.weight && x.count == -1 && !x.value; })) { return; }
    auto plain = countOptimalSelections<lng>(cap, items);
    auto modular = countOptimalSelections<M1>(cap, items);
    auto naive = naiveOptimal<lng>(cap, finite);
    for (int c = 0; c <= cap; ++c) {
        lll best = 0;
        lng count = 0;
        for (auto [v, w] : sel) {
            if (w > c) { continue; }
            if (!count || v > best) { best = v; count = 1; }
            else if (v == best) { ++count; }}
        same(naive[c], pair<lll, lng>(best, count), "naive optimal-count oracle versus enumeration");
        same(plain[c], pair<lll, lng>(best, count), "countOptimalSelections<lng>");
        same(modular[c], pair<lll, M1>(best, M1(count)), "countOptimalSelections<ModInt>");}}
void selectionCases() {
    int rounds = pick(300, 3000, 20000);
    for (int round = 0; round < rounds; ++round) {
        bool wide = round % 4 == 3;
        int cap = int(gen() % 11), n = int(gen() % 5), k = 1 + int(gen() % (wide ? 60 : 12));
        vector<Item> items(n);
        // Bounded counts below and above capacity / weight, zero-weight copies up to 7 (the first item up to 69 when wide), all enumerated.
        for (auto &x : items) {
            x = {lng(gen() % 5), num(round % 3 ? 3 : lng(1) << 61), vector<int>{-1, 0, 1, 2, 3, 5, 7}[gen() % 7]};
            if (wide && &x == &items[0]) { x.weight = 0; x.count = int(gen() % 70); }
            if (!x.weight && x.count == -1) {
                if (gen() % 2) { x.weight = 1 + lng(gen() % 4); }
                else { x.value = -abs(x.value); }}}
        selectionCase(cap, items, k);}
    // Larger tables against the direct recurrence: bounded counts take the sliding window, zero weights the doubling.
    int cap = DEBUG_CONTAINERS ? 150 : pick(150, 1500, 2000), k = 6;
    vector<Item> items;
    for (int i = 0; i < 30; ++i) {
        lng w = i % 10 == 0 ? 0 : 1 + lng(gen() % 60);
        int c = i % 10 == 0 ? int(gen() % 40) : i % 3 == 0 ? -1 : int(gen() % 30);
        if (c == -1) { w += 30; }
        items.push_back({w, num(1000), c});}
    items.push_back({0, -1 - lng(gen() % 1000), -1});
    context = "selection large capacity=" + show(cap) + " items=" + show(items);
    auto finite = finiteCopies(items, k);
    for (bool distinct : {false, true}) { same(kthBestKnapsack(cap, items, k, distinct), naiveKth(cap, finite, k, distinct), "kthBestKnapsack large"); }
    same(countOptimalSelections<M1>(cap, items), naiveOptimal<M1>(cap, finite), "countOptimalSelections large");
    cout << "PASS kthBestKnapsack and countOptimalSelections rounds=" << rounds << " against enumerated selections and a direct recurrence (bounded counts, zero-weight copies, unlimited zero-weight items of value <= 0, k to 60), capacity " << cap << " with 31 items\n";}

void bnbCase(lng cap, const vector<Item> &items, lll best) {
    ++cases;
    int n = int(items.size());
    auto [val, take] = knapsackBranchAndBound(cap, items);
    same(val, best, "knapsackBranchAndBound value");
    check(int(take.size()) == n, "knapsackBranchAndBound witness size");
    lll w = 0, v = 0;
    for (int i = 0; i < n; ++i) {
        check(take[i] == 0 || (take[i] == 1 && items[i].count == 1), "witness takes available items once");
        if (take[i]) { w += items[i].weight; v += items[i].value; }}
    check(w <= cap && v == val, "witness within capacity with the reported value");}
lll bruteZeroOne(lng cap, const vector<Item> &items) {
    int n = int(items.size());
    lll best = 0;
    for (int mask = 0; mask < 1 << n; ++mask) {
        lll w = 0, v = 0;
        bool ok = true;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) { ok &= items[i].count == 1; w += items[i].weight; v += items[i].value; }}
        if (ok && w <= cap) { best = max(best, v); }}
    return best;}
void bnbCases() {
    int rounds = pick(300, 3000, 20000);
    for (int round = 0; round < rounds; ++round) {
        int n = int(gen() % 15);
        bool huge = round % 3 == 0;
        lng r = huge ? lng(1) << 61 : 20, cap = huge ? lng(gen() % (ulng(1) << 62)) : lng(gen() % 60);
        vector<Item> items(n);
        for (auto &x : items) {
            x = {lng(gen() % ulng(huge ? r : 25)), num(r), int(gen() % 5 != 0)};
            if (round % 5 == 1) { x.value = x.weight * 3; }}
        context = "knapsackBranchAndBound capacity=" + show(cap) + " items=" + show(items);
        bnbCase(cap, items, bruteZeroOne(cap, items));}
    int dp_rounds = DEBUG_CONTAINERS ? 10 : pick(10, 100, 400);
    for (int round = 0; round < dp_rounds; ++round) {
        int n = 30 + int(gen() % 30), cap = int(gen() % 3000);
        vector<Item> items(n);
        for (auto &x : items) { x = {1 + lng(gen() % 300), lng(gen() % 1000), 1}; }
        vector<lll> f(cap + 1);
        for (const auto &x : items) {
            for (int j = cap; j >= x.weight; --j) { f[j] = max(f[j], f[j - x.weight] + x.value); }}
        context = "knapsackBranchAndBound n=" + show(n) + " capacity=" + show(cap) + " items=" + show(items);
        bnbCase(cap, items, f[cap]);}
    // A dominant item leaves capacity 1 that no other item fits, yet every fractional bound is >= 1: the search path is n items deep.
    int big = DEBUG_CONTAINERS ? 2000 : pick(2000, 100000, 200000), cap = 1000;
    vector<Item> items(big);
    vector<lll> f(cap + 1);
    for (int i = 0; i < big; ++i) {
        lng w = 2 + lng(gen() % 999);
        items[i] = i == big / 2 ? Item{999, 2000000, 1} : Item{w, w * (1 + lng(gen() % 1000)), 1};
        for (int j = cap; j >= items[i].weight; --j) { f[j] = max(f[j], f[j - items[i].weight] + items[i].value); }}
    context = "knapsackBranchAndBound large n=" + show(big) + " capacity=" + show(cap);
    bnbCase(cap, items, f[cap]);
    cout << "PASS knapsackBranchAndBound rounds=" << rounds << " against 2^n enumeration (n < 15, magnitudes to 2^62, equal ratios), " << dp_rounds << " DP rounds n < 60 and a DP-checked n=" << big << "\n";}

lng reachBest(lng t, const vector<int> &w) {
    lng total = 0;
    for (int x : w) { total += x; }
    int lim = int(min(t, total));
    vector<char> can(lim + 1);
    can[0] = 1;
    for (int x : w) {
        for (int s = lim; s >= x; --s) { can[s] |= can[s - x]; }}
    int s = lim;
    while (!can[s]) { --s; }
    return s;}
void pisingerCase(lng t, const vector<int> &w, lng best) {
    ++cases;
    auto [got, take] = pisingerSubsetSum(t, w, true);
    same(got, best, "pisingerSubsetSum best");
    check(take.size() == w.size(), "pisingerSubsetSum witness size");
    lng s = 0;
    for (int i = 0; i < int(w.size()); ++i) {
        check(take[i] == 0 || take[i] == 1, "pisingerSubsetSum 0/1 witness");
        s += take[i] * lng(w[i]);}
    same(s, got, "pisingerSubsetSum witness sum");
    auto plain = pisingerSubsetSum(t, w);
    same(plain.first, best, "pisingerSubsetSum untraced");
    check(plain.second.empty(), "pisingerSubsetSum untraced witness empty");}
void pisingerCases() {
    int rounds = pick(500, 5000, 30000);
    for (int round = 0; round < rounds; ++round) {
        int n = round % 4 == 0 ? int(gen() % 200) : int(gen() % 13), top = round % 3 == 0 ? 100 : 12;
        vector<int> w(n);
        lng total = 0;
        for (auto &x : w) { x = gen() % 7 == 0 ? 0 : int(gen() % ulng(top + 1)); total += x; }
        if (round % 5 == 0) { fill(w.begin(), w.end(), top); total = lng(n) * top; }
        lng t = round % 11 == 0 ? lng(gen() >> 2) : lng(gen() % ulng(total + 3));
        context = "pisingerSubsetSum t=" + show(t) + " w=" + show(w);
        pisingerCase(t, w, reachBest(t, w));}
    // Many small weights, then few weights up to 1e5: n * m stays below the bitset oracle's 2^23.
    int big = DEBUG_CONTAINERS ? 1000 : pick(1000, 5000, 10000);
    for (auto [n, m] : {pair(big, 800), pair(60, 100000)}) {
        vector<int> w(n);
        lng total = 0;
        for (auto &x : w) { x = 1 + int(gen() % m); total += x; }
        check(total < 1 << 23, "large pisinger oracle bitset width");
        static std::bitset<1 << 23> can;
        can.reset(); can[0] = 1;
        for (int x : w) { can |= can << x; }
        for (lng t : {total / 2 + 1, total / 3, total - 1, lng(m) - 1}) {
            lng best = t;
            while (!can[best]) { --best; }
            context = "pisingerSubsetSum large n=" + show(n) + " m=" + show(m) + " t=" + show(t);
            pisingerCase(t, w, best);}}
    cout << "PASS pisingerSubsetSum rounds=" << rounds << " against reachability (zero weights, equal weights, t beyond the total), n=" << big << " m=800 and n=60 m=1e5 with traced witnesses\n";}

lll plainUnbounded(int cap, const vector<Item> &items) {
    vector<lll> f(cap + 1);
    for (int c = 1; c <= cap; ++c) {
        f[c] = f[c - 1];
        for (const auto &x : items) {
            if (x.weight <= c) { f[c] = max(f[c], f[c - x.weight] + x.value); }}}
    return f[cap];}
// At-most bounded table by binary splitting (copies capped by cap / weight).
vector<lll> atMostBounded(int cap, const vector<Item> &items) {
    vector<lll> f(cap + 1);
    for (const auto &x : items) {
        if (!x.count || x.value <= 0 || x.weight > cap) { continue; }
        lng left = x.count == -1 ? cap / x.weight : min(lng(x.count), cap / x.weight);
        for (lng block = 1; left > 0; block *= 2) {
            lng c = min(block, left);
            left -= c;
            for (lng j = cap; j >= c * x.weight; --j) { f[j] = max(f[j], f[j - c * x.weight] + lll(c) * x.value); }}}
    return f;}
// Pigeonhole: with s an unlimited item of maximum ratio, some optimum has fewer than w_s copies of other items, so their weight is <= (w_s - 1) * max weight.
lll pigeonhole(lng cap, const vector<Item> &items) {
    int s = -1;
    lng m = 0;
    for (int i = 0; i < int(items.size()); ++i) {
        const auto &x = items[i];
        if (!x.count || x.weight > cap || x.value <= 0) { continue; }
        m = max(m, x.weight);
        if (x.count == -1 && (s < 0 || lll(x.value) * items[s].weight > lll(items[s].value) * x.weight)) { s = i; }}
    if (!m) { return 0; }
    check(s >= 0, "pigeonhole oracle needs an unlimited item");
    for (const auto &x : items) {
        if (x.count && x.weight <= cap && x.value > 0) { check(lll(x.value) * items[s].weight <= lll(items[s].value) * x.weight, "pigeonhole oracle item has maximum ratio"); }}
    lng lim = min(cap, (items[s].weight - 1) * m);
    auto g = atMostBounded(int(lim), items);
    lll res = 0;
    for (lng u = 0; u <= lim; ++u) { res = max(res, g[u] + lll((cap - u) / items[s].weight) * items[s].value); }
    return res;}
void unboundedCases() {
    int rounds = pick(300, 3000, 20000);
    for (int round = 0; round < rounds; ++round) {
        int n = 1 + int(gen() % 6), m = 1 + int(gen() % 40);
        bool small = round % 2 == 0;
        lng cap = small ? lng(gen() % 400) : round % 7 == 1 ? INT64_MAX - lng(gen() % 4) : lng(gen() >> (1 + gen() % 40));
        lng r = round % 3 == 0 ? lng(1) << 61 : 1000;
        vector<Item> items(n);
        for (auto &x : items) {
            x = {1 + lng(gen() % ulng(m)), num(r), -1};
            if (gen() % 9 == 0) {
                x.weight = lng(gen() >> 2) + 1;
                if (x.weight <= cap) { x.value = -abs(x.value); }}}
        context = "unboundedKnapsackLargeCapacity capacity=" + show(cap) + " items=" + show(items);
        ++cases;
        lll got = unboundedKnapsackLargeCapacity(cap, items);
        if (small) { same(got, plainUnbounded(int(cap), items), "unboundedKnapsackLargeCapacity versus plain DP"); }
        same(got, pigeonhole(cap, items), "unboundedKnapsackLargeCapacity versus pigeonhole");}
    int m = DEBUG_CONTAINERS ? 100 : pick(100, 600, 1000);
    vector<Item> items;
    for (int i = 0; i < 40; ++i) { items.push_back({m - lng(gen() % ulng(m / 2)), lng(gen() % 1000000000), -1}); }
    lng cap = lng(1e18) + lng(gen() % 1000000);
    context = "unboundedKnapsackLargeCapacity large m=" + show(m) + " capacity=" + show(cap);
    same(unboundedKnapsackLargeCapacity(cap, items), pigeonhole(cap, items), "unboundedKnapsackLargeCapacity large");
    cout << "PASS unboundedKnapsackLargeCapacity rounds=" << rounds << " (plain DP capacity < 400, pigeonhole capacity to INT64_MAX with values to 2^61) and m=" << m << " capacity 1e18\n";}

// Weights above INT_MAX never fit an int axis: adding such items leaves every table unchanged.
void wideCases() {
    int rounds = pick(50, 300, 1000);
    for (int round = 0; round < rounds; ++round) {
        int cap = int(gen() % 30), n = 1 + int(gen() % 5);
        vector<Item> base(n), zero_one(n), selection(n), counting(n);
        for (int i = 0; i < n; ++i) {
            base[i] = randomItem(cap, 1000);
            zero_one[i] = randomItem(cap, 1000, true);
            selection[i] = {1 + lng(gen() % 6), num(1000), int(gen() % 4) - 1};
            counting[i] = {1 + lng(gen() % 6), 0, randomCount()};}
        Item wide{(lng(1) << 32) + 1 + lng(gen() % 1000), 1 + lng(gen() % 1000), vector<int>{-1, 1, 2, INT_MAX}[gen() % 4]}, wide01 = wide;
        wide01.count = 1;
        auto plus = [](vector<Item> a, const Item &x) { a.push_back(x); return a; };
        ++cases;
        context = "wide weight=" + show(wide.weight) + " capacity=" + show(cap) + " items=" + show(base);
        same(boundedKnapsackMonotoneQueue(cap, plus(base, wide)), boundedKnapsackMonotoneQueue(cap, base), "boundedKnapsackMonotoneQueue wide weight");
        same(smallWeightKnapsack(cap, plus(base, wide)), smallWeightKnapsack(cap, base), "smallWeightKnapsack wide weight");
        vector<vector<Item>> groups;
        for (const auto &x : zero_one) { groups.push_back({x}); }
        auto more = groups;
        more[0].push_back(wide01); more.push_back({wide01});
        same(groupKnapsack(cap, more), groupKnapsack(cap, groups), "groupKnapsack wide weight");
        vector<Item2> two;
        for (const auto &x : base) { two.push_back({x.weight, x.weight % 3, x.value, x.count}); }
        auto two_more = two;
        two_more.push_back({wide.weight, 0, wide.value, wide.count}); two_more.push_back({1, wide.weight, wide.value, wide.count});
        same(twoDimensionalKnapsack(cap, cap, two_more), twoDimensionalKnapsack(cap, cap, two), "twoDimensionalKnapsack wide weight");
        auto parent = randomForest(n, round % 4);
        auto parent_more = parent;
        parent_more.push_back(int(gen() % (n + 1)) - 1);
        same(treeKnapsack(cap, parent_more, plus(zero_one, wide01)), treeKnapsack(cap, parent, zero_one), "treeKnapsack wide weight");
        for (bool distinct : {false, true}) { same(kthBestKnapsack(cap, plus(selection, wide), 4, distinct), kthBestKnapsack(cap, selection, 4, distinct), "kthBestKnapsack wide weight"); }
        same(countOptimalSelections<M1>(cap, plus(selection, wide)), countOptimalSelections<M1>(cap, selection), "countOptimalSelections wide weight");
        RemovableKnapsackCounts<M1> removable(cap), plain(cap);
        for (const auto &x : counting) { removable.add(x); plain.add(x); }
        removable.add(wide);
        same(removable.ways, plain.ways, "RemovableKnapsackCounts add wide weight");
        removable.remove(wide);
        same(removable.ways, plain.ways, "RemovableKnapsackCounts remove wide weight");
        same(knapsackCountsFps<M1>(cap, plus(counting, wide)), knapsackCountsFps<M1>(cap, counting), "knapsackCountsFps wide weight");}
    cout << "PASS weights above INT_MAX leave the int-axis tables unchanged, rounds=" << rounds << "\n";}

void smallValueCases() {
    int rounds = pick(300, 3000, 20000);
    for (int round = 0; round < rounds; ++round) {
        int kind = round % 6, n = int(gen() % (kind == 4 ? 3 : kind == 5 ? 5 : 8));
        lng V = vector<lng>{1, 2, 5, 30}[gen() % 4];
        vector<Item> items(n);
        lll want = 0;
        lng cap = kind == 0 || kind == 4 ? lng(gen() % 3000) : kind == 3 ? lng(gen() % 1000) : kind == 5 ? lng(gen() >> (1 + gen() % 62)) : 30 + lng(gen() >> (2 + gen() % 62));
        lng ws = 1 + lng(gen() % 30), vs = 1 + lng(gen() % ulng(V));
        for (auto &x : items) {
            if (kind == 0 || kind == 4) { x = {1 + lng(gen() % 60), lng(gen() % ulng(V + 1)), gen() % 5 ? randomCount() : 1000000000}; }
            if (kind == 1) { x = {1 + lng(gen() % 1000), lng(gen() % ulng(V + 1)), int(gen() % 61)}; }
            if (kind == 2) { x.weight = 1 + lng(gen() % 30); x.value = lng(gen() % ulng(vs * x.weight / ws + 1)); x.count = gen() % 3 ? randomCount() : -1; }
            if (kind == 3) { x = {1 + lng(gen() % 1000000), lng(gen() % ulng(V + 1)), gen() % 2 ? INT_MAX : int(gen() % 100)}; cap += x.count * x.weight; }
            // Weights log-uniform up to 2^62 and capacities up to 2^63 against enumeration.
            if (kind == 5) { x = {1 + lng(gen() >> (2 + gen() % 62)), lng(gen() % ulng(V + 1)), int(gen() % 6)}; }}
        if (kind == 2) { items.push_back({ws, vs, -1}); }
        // Near ties (w, v), (w + 1, v + 1) with v > w, and unit items (1, 1) beside (u + 1, u): optima leave up to V - 1 copies outside the greedy fill.
        if (kind == 4) {
            lng w = 1 + lng(gen() % 29), v = w + 1 + lng(gen() % ulng(30 - w)), u = 2 + lng(gen() % 29);
            if (gen() % 2) { items.push_back({w, v, gen() % 2 ? -1 : 20 + int(gen() % 200)}); items.push_back({w + 1, v + 1, gen() % 2 ? -1 : int(gen() % 40)}); }
            else { items.push_back({1, 1, int(u) + int(gen() % 300)}); items.push_back({u + 1, u, gen() % 2 ? -1 : int(gen() % 40)}); }
            std::shuffle(items.begin(), items.end(), gen);}
        context = "boundedKnapsackSmallValue capacity=" + show(cap) + " items=" + show(items);
        if (kind == 0 || kind == 4) {
            want = atMostBounded(int(cap), items)[cap];
            if (cap <= 12 && n <= 4) {
                lll brute = 0;
                enumerate(items, cap, [&](lll, lll v) { brute = max(brute, v); });
                same(want, brute, "bounded oracle versus enumeration");}}
        if (kind == 1) {
            lng axis = 0;
            for (const auto &x : items) { axis += x.count * x.value; }
            want = ValueKnapsackDP(int(axis), items).bestWithin(cap);}
        if (kind == 2) { want = pigeonhole(cap, items); }
        if (kind == 3) {
            for (const auto &x : items) { want += lll(x.count) * x.value; }}
        if (kind == 5) { enumerate(items, cap, [&](lll, lll v) { want = max(want, v); }); }
        ++cases;
        same(boundedKnapsackSmallValue(cap, items), want, kind == 0 || kind == 4 ? "boundedKnapsackSmallValue versus capacity DP" : kind == 1 ? "boundedKnapsackSmallValue versus full value DP" : kind == 2 ? "boundedKnapsackSmallValue versus pigeonhole" : kind == 3 ? "boundedKnapsackSmallValue everything fits" : "boundedKnapsackSmallValue huge weights versus enumeration");}
    int n = DEBUG_CONTAINERS ? 20 : pick(20, 60, 100);
    vector<Item> items(n);
    lng axis = 0;
    for (auto &x : items) { x = {1 + lng(gen() % 100000), 1 + lng(gen() % 30), int(gen() % 60)}; axis += x.count * x.value; }
    lng cap = 0;
    for (const auto &x : items) { cap += x.count * x.weight; }
    cap /= 2;
    ValueKnapsackDP dp(int(axis), items);
    context = "boundedKnapsackSmallValue large n=" + show(n) + " capacity=" + show(cap);
    same(boundedKnapsackSmallValue(cap, items), lll(dp.bestWithin(cap)), "boundedKnapsackSmallValue large");
    cout << "PASS boundedKnapsackSmallValue rounds=" << rounds << " (capacity DP with near-tie pairs, full value DP, pigeonhole, all-fit, enumeration with weights to 2^62; counts to INT_MAX, capacity to 2^63) and n=" << n << "\n";}

void regressionCases() {
    context = "hand-checked fixtures";
    // Capacity 7, items (3, 4) x2 and (2, 3) unlimited: exact weights 0..7.
    vector<Item> items{{3, 4, 2}, {2, 3, -1}};
    vector<lll> want{0, U, 3, 4, 6, 7, 9, 10};
    same(boundedKnapsackMonotoneQueue(7, items), want, "boundedKnapsackMonotoneQueue fixture");
    same(smallWeightKnapsack(7, items), want, "smallWeightKnapsack fixture");
    // Tree: 0 root (1, 1), children 1 (2, 5) and 2 (1, 2); 1 has child 3 (1, 10); capacity 4.
    same(treeKnapsack(4, {-1, 0, 0, 1}, {{1, 1, 1}, {2, 5, 1}, {1, 2, 1}, {1, 10, 1}}), vector<lll>{0, 1, 3, 6, 16}, "treeKnapsack fixture");
    // 1e18 = 1 mod 3: two weight-2 items and (1e18 - 4) / 3 weight-3 items beat filling with threes alone.
    same(unboundedKnapsackLargeCapacity(lng(1e18), {{2, 3, -1}, {3, 5, -1}}), lll((lng(1e18) - 4) / 3) * 5 + 6, "unboundedKnapsackLargeCapacity fixture");
    // INT64_MAX = 1 mod 3 as well; the top of the capacity domain.
    same(unboundedKnapsackLargeCapacity(INT64_MAX, {{2, 3, -1}, {3, 5, -1}}), lll((INT64_MAX - 4) / 3) * 5 + 6, "unboundedKnapsackLargeCapacity INT64_MAX");
    // Copies 0..INT_MAX of a zero-weight item: all when positive, none when negative, any number when zero.
    lll M = INT_MAX;
    same(countOptimalSelections<lng>(1, {{0, 0, INT_MAX}, {1, 2, 1}}), vector<pair<lll, lng>>{{0, lng(M) + 1}, {2, lng(M) + 1}}, "countOptimalSelections zero-weight zero-value copies");
    same(countOptimalSelections<lng>(0, {{0, 5, INT_MAX}, {0, -5, INT_MAX}}), vector<pair<lll, lng>>{{5 * M, 1}}, "countOptimalSelections zero-weight signed copies");
    same(kthBestKnapsack(0, {{0, 5, INT_MAX}}, 3), vector<vector<lll>>{{5 * M, 5 * M - 5, 5 * M - 10}}, "kthBestKnapsack zero-weight positive copies");
    same(kthBestKnapsack(0, {{0, -5, INT_MAX}}, 3, true), vector<vector<lll>>{{0, -5, -10}}, "kthBestKnapsack zero-weight negative copies");
    same(kthBestKnapsack(0, {{0, 0, INT_MAX}}, 3), vector<vector<lll>>{{0, 0, 0}}, "kthBestKnapsack zero-weight zero copies");
    same(kthBestKnapsack(0, {{0, 0, INT_MAX}}, 3, true), vector<vector<lll>>{{0}}, "kthBestKnapsack zero-weight zero copies distinct");
    // Copies t of {0, 1} and s <= 2 of {1, 3} at capacity 2: values t + 3s, the top four M + 6, M + 5, M + 4, M + 3.
    same(kthBestKnapsack(2, {{0, 1, INT_MAX}, {1, 3, 2}}, 4)[2], vector<lll>{M + 6, M + 5, M + 4, M + 3}, "kthBestKnapsack zero-weight with bounded copies");
    // Unlimited zero-weight items: copies t of {0, -2} beside {1, 3} give 3 - 2t and -2t; zero values repeat the top value.
    same(kthBestKnapsack(1, {{0, -2, -1}, {1, 3, 1}}, 4), vector<vector<lll>>{{0, -2, -4, -6}, {3, 1, 0, -1}}, "kthBestKnapsack unlimited zero-weight negative");
    same(kthBestKnapsack(0, {{0, 0, -1}}, 3), vector<vector<lll>>{{0, 0, 0}}, "kthBestKnapsack unlimited zero-weight zero");
    same(countOptimalSelections<lng>(1, {{0, -3, -1}, {1, 2, 1}}), vector<pair<lll, lng>>{{0, 1}, {2, 1}}, "countOptimalSelections unlimited zero-weight negative");
    same(pisingerSubsetSum(10, {6, 6, 7, 3}, true).first, lng(10), "pisingerSubsetSum fixture");
    // One (10, 11) and nine (11, 12) fill 109 for 119; at most V / 2 = 6 copies in the value DP would give 116.
    same(boundedKnapsackSmallValue(109, {{10, 11, 100}, {11, 12, -1}}), lll(119), "boundedKnapsackSmallValue near-tie fixture");
    // 96 unit items and four (6, 5) fill 120 for 116, leaving V - 1 = 4 unit copies unused; at most V - 2 copies in the value DP give 115.
    same(boundedKnapsackSmallValue(120, {{1, 1, 100}, {6, 5, -1}}), lll(116), "boundedKnapsackSmallValue tight exchange fixture");
    same(boundedKnapsackSmallValue(lng(4e18), {{1, 1, INT_MAX}, {2, 1, -1}}), lll(INT_MAX) + (lll(lng(4e18)) - INT_MAX) / 2, "boundedKnapsackSmallValue huge capacity");
    cout << "PASS hand-checked fixtures\n";}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i];
            if (probe == "monotone-capacity") { boundedKnapsackMonotoneQueue(-1, {}); }
            if (probe == "monotone-capacity-max") { boundedKnapsackMonotoneQueue(INT_MAX, {}); }
            if (probe == "monotone-weight") { boundedKnapsackMonotoneQueue(3, {{-1, 1, 1}}); }
            if (probe == "monotone-count") { boundedKnapsackMonotoneQueue(3, {{1, 1, -2}}); }
            if (probe == "monotone-free") { boundedKnapsackMonotoneQueue(3, {{0, 1, -1}}); }
            if (probe == "small-weight-capacity") { smallWeightKnapsack(INT_MAX, {}); }
            if (probe == "small-weight-capacity-negative") { smallWeightKnapsack(-1, {}); }
            if (probe == "small-weight-free") { smallWeightKnapsack(3, {{0, 1, -1}}); }
            if (probe == "group-capacity") { groupKnapsack(-1, {}); }
            if (probe == "group-capacity-max") { groupKnapsack(INT_MAX, {}); }
            if (probe == "group-count") { groupKnapsack(3, {{{1, 1, 2}}}); }
            if (probe == "group-unlimited") { groupKnapsack(3, {{{1, 1, -1}}}); }
            if (probe == "two-capacity") { twoDimensionalKnapsack(-1, 0, {}); }
            if (probe == "two-capacity2") { twoDimensionalKnapsack(0, -1, {}); }
            if (probe == "two-capacity-max") { twoDimensionalKnapsack(INT_MAX, 0, {}); }
            if (probe == "two-capacity2-max") { twoDimensionalKnapsack(0, INT_MAX, {}); }
            if (probe == "two-weight") { twoDimensionalKnapsack(1, 1, {{-1, 0, 1, 1}}); }
            if (probe == "two-free") { twoDimensionalKnapsack(1, 1, {{0, 0, 1, -1}}); }
            if (probe == "two-count") { twoDimensionalKnapsack(1, 1, {{1, 1, 1, -2}}); }
            if (probe == "tree-capacity") { treeKnapsack(-1, {}, {}); }
            if (probe == "tree-capacity-max") { treeKnapsack(INT_MAX, {}, {}); }
            if (probe == "tree-size") { treeKnapsack(3, {-1}, {}); }
            if (probe == "tree-parent") { treeKnapsack(3, {1}, {{1, 1, 1}}); }
            if (probe == "tree-cycle") { treeKnapsack(3, {1, 0}, {{1, 1, 1}, {1, 1, 1}}); }
            if (probe == "tree-count") { treeKnapsack(3, {-1}, {{1, 1, 2}}); }
            if (probe == "removable-capacity") { RemovableKnapsackCounts<lng> bad(-1); }
            if (probe == "removable-capacity-max") { RemovableKnapsackCounts<lng> bad(INT_MAX); }
            if (probe == "removable-weight") { RemovableKnapsackCounts<lng>(3).add({0, 1, 1}); }
            if (probe == "removable-count") { RemovableKnapsackCounts<lng>(3).remove({1, 1, -2}); }
            if (probe == "kth-capacity") { kthBestKnapsack(-1, {}, 1); }
            if (probe == "kth-capacity-max") { kthBestKnapsack(INT_MAX, {}, 1); }
            if (probe == "kth-k") { kthBestKnapsack(3, {}, 0); }
            if (probe == "kth-count") { kthBestKnapsack(3, {{1, 1, -2}}, 1); }
            if (probe == "kth-free") { kthBestKnapsack(3, {{0, 1, -1}}, 1); }
            if (probe == "kth-weight") { kthBestKnapsack(3, {{-1, 1, 1}}, 1); }
            if (probe == "count-optimal-capacity") { countOptimalSelections<lng>(-1, {}); }
            if (probe == "count-optimal-capacity-max") { countOptimalSelections<lng>(INT_MAX, {}); }
            if (probe == "count-optimal-count") { countOptimalSelections<lng>(3, {{1, 1, -2}}); }
            if (probe == "count-optimal-free") { countOptimalSelections<lng>(3, {{0, 0, -1}}); }
            if (probe == "count-optimal-weight") { countOptimalSelections<lng>(3, {{-1, 1, 1}}); }
            if (probe == "fps-modulus") { knapsackCountsFps<ModInt<7>>(7, {}); }
            if (probe == "fps-free") { knapsackCountsFps<M1>(3, {{0, 1, -1}}); }
            if (probe == "fps-size") { knapsackCountsFps<M1>(-1, {}); }
            if (probe == "fps-count") { knapsackCountsFps<M1>(3, {{1, 1, -2}}); }
            if (probe == "fps-weight") { knapsackCountsFps<M1>(3, {{-1, 1, 1}}); }
            if (probe == "bnb-capacity") { knapsackBranchAndBound(-1, {}); }
            if (probe == "bnb-count") { knapsackBranchAndBound(3, {{1, 1, 2}}); }
            if (probe == "pisinger-target") { pisingerSubsetSum(-1, {}); }
            if (probe == "pisinger-weight") { pisingerSubsetSum(3, {-1}); }
            if (probe == "pisinger-width") { pisingerSubsetSum(1, {INT_MAX / 2 + 1}); }
            if (probe == "unbounded-capacity") { unboundedKnapsackLargeCapacity(-1, {}); }
            if (probe == "unbounded-count") { unboundedKnapsackLargeCapacity(3, {{1, 1, 1}}); }
            if (probe == "unbounded-weight") { unboundedKnapsackLargeCapacity(3, {{0, 1, -1}}); }
            if (probe == "unbounded-width") { unboundedKnapsackLargeCapacity(lng(1) << 40, {{INT_MAX, 1, -1}}); }
            if (probe == "small-value-capacity") { boundedKnapsackSmallValue(-1, {}); }
            if (probe == "small-value-weight") { boundedKnapsackSmallValue(3, {{0, 1, 1}}); }
            if (probe == "small-value-negative") { boundedKnapsackSmallValue(3, {{1, -1, 1}}); }
            if (probe == "small-value-count") { boundedKnapsackSmallValue(3, {{1, 1, -2}}); }
            if (probe == "small-value-axis") { boundedKnapsackSmallValue(1, {{1, lng(1) << 40, 1}, {1, 2, -1}}); }
            return 1;}}
    if ((DEBUG_CONTAINERS || SANITIZED) && mode == "stress") { mode = "full"; }
    gen.seed(test_seed);
    regressionCases();
    exhaustiveTables(); randomTables(); groupAnd2DCases(); treeCases();
    countingCases(); selectionCases(); bnbCases(); pisingerCases(); unboundedCases(); smallValueCases(); wideCases();
    cout << "PASS knapsack advanced cases=" << cases << " checks=" << checks << " seed=" << test_seed << '\n';}
