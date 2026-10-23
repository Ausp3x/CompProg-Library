#include "../../06-Miscellaneous/55-fractional_programming.hpp"

#ifdef _GLIBCXX_DEBUG
constexpr bool DEBUG_CONTAINERS = true;  // -O0 debug containers: stress runs as full
#else
constexpr bool DEBUG_CONTAINERS = false;
#endif
#ifdef __SANITIZE_ADDRESS__
constexpr bool SANITIZED = true;  // ASan stress runs as full, well inside the runner's 180 s limit
#else
constexpr bool SANITIZED = false;
#endif
ulng test_seed = 0;
lng checks = 0, cases = 0;
int max_iterations = 0;
string context;
[[noreturn]] void fail(const string &what, const string &want, const string &got) {
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
         << " operation=" << what << " expected=" << want << " actual=" << got << '\n';
    std::exit(1);}
void check(bool ok, const string &what) { ++checks; if (!ok) { fail(what, "true", "false"); } }
string showRatio(lll p, lll q) { return std::to_string(lng(p)) + "/" + std::to_string(lng(q)); }
string show(const vector<lng> &a) {
    string out = "[";
    for (lng x : a) { out += std::to_string(x) + ","; }
    return out + "]";}
// Exact ratio comparison p / q versus r / s for positive denominators.
int compareRatio(lll p, lll q, lll r, lll s) { lll x = p * s, y = r * q; return x < y ? -1 : x > y; }

// Brute force: every subset satisfying the constraint, best ratio by exact cross-multiplication.
pair<lll, lll> bruteBest(const vector<lng> &a, const vector<lng> &b, const std::function<bool(int, lll)> &ok) {
    int n = int(a.size()); pair<lll, lll> best{0, 0};
    for (int mask = 0; mask < 1 << n; ++mask) {
        lll x = 0, y = 0;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) { x += a[i]; y += b[i]; }}
        if (!ok(std::popcount(uint(mask)), y)) { continue; }
        if (!best.second || compareRatio(x, y, best.first, best.second) > 0) { best = {x, y}; }}
    return best;}
void checkSelection(const FractionalSelection &got, const vector<lng> &a, const vector<lng> &b, pair<lll, lll> best, int k, lll need) {
    if (!best.second) {
        check(got.den == 0 && got.num == 0 && got.items.empty(), "infeasible sentinel den=0");
        return;}
    check(got.den > 0, "feasible denominator positive");
    ++checks;
    if (compareRatio(got.num, got.den, best.first, best.second)) { fail("optimal ratio equals brute force", showRatio(best.first, best.second), showRatio(got.num, got.den)); }
    lll x = 0, y = 0;
    for (int i = 0; i < int(got.items.size()); ++i) {
        check(0 <= got.items[i] && got.items[i] < int(a.size()), "item index in range");
        if (i) { check(got.items[i - 1] < got.items[i], "items strictly increasing"); }
        x += a[got.items[i]]; y += b[got.items[i]];}
    check(x == got.num && y == got.den, "witness sums equal num/den");
    if (k >= 0) { check(int(got.items.size()) == k, "witness has exactly k items"); }
    check(y >= need, "witness meets the denominator bound");}

void selectCase(const vector<lng> &a, const vector<lng> &b, int k, int seeds = 2) {
    ++cases; context = "zeroOne a=" + show(a) + " b=" + show(b) + " k=" + std::to_string(k);
    auto best = bruteBest(a, b, [&](int size, lll) { return size == k; });
    // Two seeds: the ratio is seed-independent (Las Vegas) and each witness must be optimal.
    for (ulng seed : {test_seed + ulng(cases), ~(test_seed + ulng(cases))}) {
        if (seeds-- > 0) { checkSelection(zeroOneFractionalSelect(a, b, k, seed), a, b, best, k, 0); }}}
// Optimality certificate: at t = num / den the k largest keys a - t * b sum to exactly zero; a second seed must reach the same ratio.
void certifiedCase(const vector<lng> &a, const vector<lng> &b, int k) {
    ++cases;
    int n = int(a.size());
    auto got = zeroOneFractionalSelect(a, b, k, test_seed + ulng(cases)), other = zeroOneFractionalSelect(a, b, k, ~(test_seed + ulng(cases)));
    vector<lll> key(n);
    for (int i = 0; i < n; ++i) { key[i] = lll(got.den) * a[i] - lll(got.num) * b[i]; }
    std::nth_element(key.begin(), key.begin() + (k - 1), key.end(), std::greater<>());
    lll top = 0;
    for (int i = 0; i < k; ++i) { top += key[i]; }
    check(top == 0 && got.den > 0, "optimality certificate");
    check(compareRatio(got.num, got.den, other.num, other.den) == 0, "ratio independent of the seed");
    for (auto &res : {got, other}) {
        lll x = 0, y = 0;
        for (int i = 0; i < int(res.items.size()); ++i) {
            check(0 <= res.items[i] && res.items[i] < n && (!i || res.items[i - 1] < res.items[i]), "items in range and strictly increasing");
            x += a[res.items[i]]; y += b[res.items[i]];}
        check(x == res.num && y == res.den && int(res.items.size()) == k, "witness sums and size");}}
// Tangents to (2^(L+2) - t)^2 at distances 2^(j+1), j < L, r copies each: Dinkelbach needs about L steps from a low start, so the Eppstein-Hirschberg rounds run.
void tangents(vector<lng> &a, vector<lng> &b, int L, int r) {
    for (int j = 0; j < L; ++j) {
        for (int c = 0; c < r; ++c) { b.push_back(lng(4) << j); a.push_back((lng(4) << j) * ((lng(1) << (L + 2)) - (lng(1) << j))); }}}
void knapsackCase(const vector<lng> &a, const vector<lng> &b, int W) {
    ++cases;
    context = "ratioKnapsack a=" + show(a) + " b=" + show(b) + " W=" + std::to_string(W);
    auto best = bruteBest(a, b, [&](int size, lll y) { return size > 0 && y >= W; });
    checkSelection(ratioKnapsack(a, b, W), a, b, best, -1, W);}

// Dinkelbach over an explicit family of (N, D) pairs; the oracle is a brute-force argmax of q * N - p * D.
void dinkelbachCase(const vector<pair<lng, lng>> &family, std::mt19937_64 &gen) {
    ++cases; context = "dinkelbach family size=" + std::to_string(family.size());
    pair<lng, lng> best = family[0], worst = family[0];
    lng A = 0, B = 1;
    for (auto f : family) {
        if (compareRatio(f.first, f.second, best.first, best.second) > 0) { best = f; }
        if (compareRatio(f.first, f.second, worst.first, worst.second) < 0) { worst = f; }
        A = max(A, abs(f.first)); B = max(B, f.second);}
    int calls = 0;
    auto oracle = [&](lng p, lng q) {
        ++calls; pair<lng, lng> arg = family[0]; lll top = lll(q) * arg.first - lll(p) * arg.second;
        for (auto f : family) {
            lll key = lll(q) * f.first - lll(p) * f.second;
            if (key > top) { top = key; arg = f; }}
        return arg;};
    // Feasible starts and the infeasible lower bound -INT64_MAX / 1, at most every ratio with |N| < 2^63.
    for (auto start : {worst, best, family[gen() % family.size()], pair<lng, lng>(-INT64_MAX, 1)}) {
        calls = 0;
        auto got = dinkelbach(oracle, start.first, start.second);
        ++checks;
        if (compareRatio(got.first, got.second, best.first, best.second)) { fail("dinkelbach optimum from a start at most the optimum", showRatio(best.first, best.second), showRatio(got.first, got.second)); }
        check(std::find(family.begin(), family.end(), got) != family.end(), "dinkelbach returns a feasible pair");
        check(calls <= 4 + std::log2(double(A) + 1) + 3 * std::log2(double(B)), "dinkelbach oracle calls <= 4 + log2(A + 1) + 3 * log2(B)");
        max_iterations = max(max_iterations, calls);}
    // Minimization by negating numerators.
    auto neg = family;
    for (auto &f : neg) { f.first = -f.first; }
    pair<lng, lng> low = family[0];
    for (auto f : family) {
        if (compareRatio(f.first, f.second, low.first, low.second) < 0) { low = f; }}
    auto negOracle = [&](lng p, lng q) {
        pair<lng, lng> arg = neg[0]; lll top = lll(q) * arg.first - lll(p) * arg.second;
        for (auto f : neg) {
            lll key = lll(q) * f.first - lll(p) * f.second;
            if (key > top) { top = key; arg = f; }}
        return arg;};
    auto got = dinkelbach(negOracle, neg[0].first, neg[0].second);
    check(compareRatio(-lll(got.first), got.second, low.first, low.second) == 0, "minimum ratio by negated numerators");}

void exhaustiveCases(const string &mode) {
    int top = mode == "quick" ? 3 : mode == "full" ? 4 : 5;
    vector<lng> values{-2, 0, 1, 3}, weights{1, 2, 3};
    for (int n = 1; n <= top; ++n) {
        vector<lng> a(n), b(n);
        auto visit = [&](auto &&visit, int i) -> void {
            if (i == n) {
                // One pass over the subsets gives the brute-force best for every size k and every floor W.
                vector<pair<lll, lll>> by_size(n + 1, {0, 0}), by_floor(3 * n + 2, {0, 0});
                for (int mask = 1; mask < 1 << n; ++mask) {
                    lll x = 0, y = 0;
                    for (int j = 0; j < n; ++j) {
                        if (mask >> j & 1) { x += a[j]; y += b[j]; }}
                    auto better = [&](pair<lll, lll> &best) { if (!best.second || compareRatio(x, y, best.first, best.second) > 0) { best = {x, y}; } };
                    better(by_size[std::popcount(uint(mask))]);
                    for (int W = 1; W <= min(lll(3 * n + 1), y); ++W) { better(by_floor[W]); }}
                string base = " a=" + show(a) + " b=" + show(b);
                for (int k = 1; k <= n; ++k) {
                    ++cases; context = "zeroOne" + base + " k=" + std::to_string(k);
                    checkSelection(zeroOneFractionalSelect(a, b, k, test_seed + ulng(cases)), a, b, by_size[k], k, 0);}
                for (int W = 1; W <= 3 * n + 1; ++W) {
                    ++cases; context = "ratioKnapsack" + base + " W=" + std::to_string(W);
                    checkSelection(ratioKnapsack(a, b, W), a, b, by_floor[W], -1, W);}
                return;}
            for (lng x : values) { for (lng y : weights) { a[i] = x; b[i] = y; visit(visit, i + 1); } }};
        visit(visit, 0);}
    cout << "PASS exhaustive a in {-2,0,1,3}, b in {1,2,3}, n<=" << top << ", every k and W<=3n+1\n";}

void regressionCases() {
    selectCase({5}, {7}, 1);
    selectCase({1, 1, 1}, {1, 1, 1}, 2);
    selectCase({-5, -1, -3}, {2, 1, 4}, 2);
    selectCase({0, 0, 0, 0}, {1, 2, 3, 4}, 3);
    selectCase({7, 1, 7, 1}, {3, 1, 3, 1}, 2);
    knapsackCase({5}, {7}, 7); knapsackCase({5}, {7}, 8); knapsackCase({}, {}, 1);
    knapsackCase({-1, -2}, {1, 1}, 1); knapsackCase({3, 3}, {2, 2}, 3);
    // Weights near and above INT_MAX are capped at W by the oracle's state space.
    knapsackCase({5, 3, -2}, {INT_MAX, 1, INT_MAX - 1}, 2); knapsackCase({7, 1}, {INT_MAX, INT_MAX}, 1000); knapsackCase({-7, -1}, {INT_MAX, 3}, 4);
    knapsackCase({9, 2, 4}, {lng(1) << 40, 1, (lng(1) << 61) - 1}, 1000); knapsackCase({-3, 8}, {lng(INT_MAX) + 1, lng(1) << 50}, 7);
    lng big = lng(1) << 59, top = lng(1) << 61;
    // Both sums exactly 2^62 - 1, the largest values in the domain.
    selectCase({top - 1, top}, {top, top - 1}, 1); selectCase({-top, top - 1, 0}, {1, top - 1, top - 1}, 2);
    // Near the documented bound: sum |a| < 2^62 and sum b < 2^62.
    selectCase({big, -big, big - 1, 3}, {big, 1, big - 3, 7}, 2);
    selectCase({big, big, big}, {1, big, 2}, 2);
    knapsackCase({big, big - 5, -big}, {3, 2, 1}, 4);
    // Hand-checked judge samples: USACO 2018 Open Gold "Talent Show" (32/30) and POJ 2976 (5/6 and 16/16).
    context = "hand-checked samples";
    auto talent = ratioKnapsack({21, 11, 31}, {20, 10, 30}, 15);
    check(talent.num == 32 && talent.den == 30 && talent.items == vector<int>{0, 1}, "Talent Show sample 1066");
    auto drop1 = zeroOneFractionalSelect({5, 0, 2}, {5, 1, 6}, 2);
    check(drop1.num == 5 && drop1.den == 6 && drop1.items == vector<int>{0, 1}, "POJ 2976 sample 83");
    auto drop2 = zeroOneFractionalSelect({1, 2, 7, 9}, {5, 6, 7, 9}, 2);
    check(drop2.num == 16 && drop2.den == 16 && drop2.items == vector<int>{2, 3}, "POJ 2976 sample 100");
    cout << "PASS regressions: singletons, ties, negative and zero values, infeasible W, values near 2^62, weights near and above INT_MAX, judge samples\n";}

void randomCases(const string &mode) {
    std::mt19937_64 gen(test_seed);
    int rounds = mode == "quick" ? 200 : mode == "full" ? 3000 : 20000;
    for (int round = 0; round < rounds; ++round) {
        int n = 1 + int(gen() % 12);
        lng range = round % 3 == 0 ? 1000000000000LL : 50;
        vector<lng> a(n), b(n), bi(n);
        for (int i = 0; i < n; ++i) {
            // Some rounds put weights near INT_MAX or up to 2^40 above it.
            a[i] = lng(gen() % (2 * range + 1)) - range; bi[i] = round % 7 == 3 && gen() % 2 ? INT_MAX - lng(gen() % 3) : round % 7 == 5 && gen() % 2 ? lng(INT_MAX) + lng(gen() % (lng(1) << 40)) : 1 + lng(gen() % 20);
            b[i] = round % 3 == 0 ? 1 + lng(gen() % 1000000000000LL) : bi[i];}
        selectCase(a, b, 1 + int(gen() % n));
        knapsackCase(a, bi, 1 + int(gen() % (20 * n)));
        int m = 1 + int(gen() % 40);
        vector<pair<lng, lng>> family(m);
        for (auto &f : family) { f = {lng(gen() % 2000000001) - 1000000000, 1 + lng(gen() % 1000000000)}; }
        dinkelbachCase(family, gen);
        // Operands at the 2^63 bound: |N| and D within 1 of INT64_MAX, or D tiny.
        for (auto &f : family) {
            lng x = INT64_MAX - lng(gen() % 2);
            f = {gen() % 2 ? x : -x, gen() % 2 ? INT64_MAX - lng(gen() % 2) : 1 + lng(gen() % 3)};}
        dinkelbachCase(family, gen);}
    cout << "PASS seeded zeroOne/ratioKnapsack/dinkelbach rounds=" << rounds << " max_n=12, dinkelbach families near 2^63, max dinkelbach oracle calls=" << max_iterations << '\n';}

// Tangent families in shuffled order: n <= 14 against brute force for every k; up to 98 and up to 1204 items by certificate.
void slowCases(const string &mode) {
    std::mt19937_64 gen(test_seed ^ 99);
    int rounds = mode == "quick" ? 150 : mode == "full" ? 9000 : 45000;
    for (int round = 0; round < rounds; ++round) {
        int kind = round % 3;
        int L = 9 + int(gen() % (kind == 0 ? 3 : kind == 1 ? 6 : 10)), r = kind == 0 ? 1 : 1 + int(gen() % (kind == 1 ? 4 : 30));
        lng T = lng(1) << (L + 2);
        vector<lng> a, b;
        tangents(a, b, L, r);
        // Fillers (0, 2^(L+12)); noise below ratio T / 2, often on a tangent's b; items above T, in every optimal set; tangent copies lowered (equal b).
        auto filler = [&] { a.push_back(0); b.push_back(lng(1) << (L + 12)); };
        auto noise = [&] { b.push_back(gen() % 2 ? lng(4) << (gen() % L) : 1 + lng(gen() % ulng(4 * T))); a.push_back(lng(gen() % ulng(T / 2 * b.back() + 1))); };
        auto high = [&] { b.push_back(1 + lng(gen() % 4)); a.push_back(b.back() * (T + 1 + lng(gen() % ulng(T)))); };
        auto lowered = [&] { int j = int(gen() % L); b.push_back(lng(4) << j); a.push_back((lng(4) << j) * (T - (lng(1) << j)) - 1 - lng(gen() % ulng(lng(1) << j))); };
        int m = 0;
        if (kind == 0) {
            for (int c = int(gen() % ulng(15 - L)); c > 0; --c) {
                int type = int(gen() % 4);
                if (type == 0) { filler(); }
                else if (type == 1) { noise(); }
                else if (type == 2) { high(); }
                else { lowered(); }}}
        else {
            for (int i = 0; i < L * r; ++i) { a[i] -= i % r && gen() % 2 ? 1 + lng(gen() % ulng(b[i] / 4)) : 0; }
            m = int(gen() % ulng(2 * r + 1));
            for (int c = int(gen() % 5); c > 0; --c) { filler(); }
            for (int c = int(gen() % (kind == 1 ? 31 : 601)); c > 0; --c) { noise(); }
            for (int c = m; c > 0; --c) { high(); }}
        // Subtracting shift * b from every a lowers every ratio by shift and keeps the Dinkelbach path, now through negative values.
        lng shift = gen() % 2 ? 0 : lng(gen() % (ulng(8) << L)) - (lng(4) << L);
        for (int i = 0; i < int(a.size()); ++i) { a[i] -= shift * b[i]; }
        vector<int> order(a.size());
        iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), gen);
        vector<lng> pa, pb;
        for (int i : order) { pa.push_back(a[i]); pb.push_back(b[i]); }
        int n = int(pa.size()), k = kind == 0 ? 1 + int(gen() % ulng(n)) : min(n, m + 1 + int(gen() % ulng(4 * r)));
        context = "tangent round=" + std::to_string(round) + " L=" + std::to_string(L) + " r=" + std::to_string(r) + " n=" + std::to_string(n) + " k=" + std::to_string(k);
        if (kind == 0) { selectCase(pa, pb, k); }
        else { certifiedCase(pa, pb, k); }}
    cout << "PASS tangent families rounds=" << rounds << " (n<=14 brute force for every k; n<=98 and n<=1204 certified, with lowered copies, equal-b noise, always-chosen items, half shifted to negative values)\n";}

void largeCases(const string &mode) {
    int n = mode == "quick" ? 2000 : mode == "full" ? 200000 : 1000000;
    std::mt19937_64 gen(test_seed ^ 77);
    vector<lng> a(n), b(n);
    // Random values, a 5-value alphabet over weights 1..3 (heavy duplicates), lines a = 7b - 5 that all cross at t = 7, and 28 tangents behind fillers (0, 2^30).
    for (int family = 0; family < 4; ++family) {
        for (int i = 0; i < n; ++i) {
            b[i] = family == 1 ? 1 + lng(gen() % 3) : family == 3 ? lng(1) << 30 : 1 + lng(gen() % 1000);
            a[i] = family == 0 ? lng(gen() % 1000000) : family == 1 ? lng(gen() % 5) : family == 2 ? 7 * b[i] - 5 : 0;}
        if (family == 3) {
            vector<lng> ta, tb;
            tangents(ta, tb, 28, 1);
            std::copy(ta.begin(), ta.end(), a.end() - 28); std::copy(tb.begin(), tb.end(), b.end() - 28);}
        for (int k : {n / 3, 1, n - 1}) {
            context = "large zeroOne n=" + std::to_string(n) + " family=" + std::to_string(family) + " k=" + std::to_string(k);
            certifiedCase(a, b, k);}}
    int m = mode == "quick" ? 60 : 250, W = mode == "quick" ? 300 : 1000;
    vector<lng> ta(m), tb(m);
    for (int i = 0; i < m; ++i) { ta[i] = 1 + lng(gen() % 1000); tb[i] = 1 + lng(gen() % 1000000); }
    for (int i = 0; i < m; ++i) { tb[i] = 1 + tb[i] % 1000; }
    context = "large ratioKnapsack n=" + std::to_string(m) + " W=" + std::to_string(W);
    auto ratio = ratioKnapsack(ta, tb, W);
    // Certificate: the capped knapsack of a - t * b at the returned t has optimum zero (computed independently forward).
    vector<lll> dp(W + 1, -(lll(1) << 120)); dp[0] = 0;
    for (int i = 0; i < m; ++i) {
        lll v = lll(ratio.den) * ta[i] - lll(ratio.num) * tb[i];
        auto old = dp;
        for (int c = 0; c <= W; ++c) {
            if (old[c] > -(lll(1) << 119)) { int to = int(min(lng(W), c + tb[i])); dp[to] = max(dp[to], old[c] + v); }}}
    check(dp[W] == 0 && ratio.den >= W, "large ratioKnapsack optimality certificate");
    lll ra = 0, rb = 0;
    for (int i : ratio.items) { ra += ta[i]; rb += tb[i]; }
    check(ra == ratio.num && rb == ratio.den, "large ratioKnapsack witness sums");
    cout << "PASS large zeroOne n=" << n << " (random, duplicates, concurrent lines, tangents; k = n/3, 1, n-1) ratioKnapsack n=" << m << " W=" << W << '\n';}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i];
            auto one = [](lng, lng) { return pair<lng, lng>(1, 1); };
            if (probe == "dinkelbach-denominator") { dinkelbach(one, 0, 0); }
            if (probe == "dinkelbach-oracle-denominator") { dinkelbach([](lng, lng) { return pair<lng, lng>(1, 0); }, 0, 1); }
            // Start 2 / 1 above the optimum 1 of the family {10 / 10, 1 / 2}.
            // A nonmaximizing oracle whose ratio keeps rising passes every per-call check.
            if (probe == "dinkelbach-calls") { dinkelbach([](lng p, lng q) { return pair<lng, lng>(p + 1, q); }, 0, 1); }
            if (probe == "dinkelbach-start") { dinkelbach([](lng p, lng q) { return q * 10 - p * 10 >= q * 1 - p * 2 ? pair<lng, lng>(10, 10) : pair<lng, lng>(1, 2); }, 2, 1); }
            if (probe == "select-k-zero") { zeroOneFractionalSelect({1}, {1}, 0); }
            if (probe == "select-k-large") { zeroOneFractionalSelect({1}, {1}, 2); }
            if (probe == "select-size") { zeroOneFractionalSelect({1, 2}, {1}, 1); }
            if (probe == "select-weight") { zeroOneFractionalSelect({1, 2}, {1, 0}, 1); }
            if (probe == "select-sum") { zeroOneFractionalSelect({lng(1) << 61, lng(1) << 61}, {1, 1}, 1); }
            if (probe == "select-weight-sum") { zeroOneFractionalSelect({1, 1}, {lng(1) << 61, lng(1) << 61}, 1); }
            if (probe == "ratio-bound") { ratioKnapsack({1}, {1}, 0); }
            if (probe == "ratio-bound-max") { ratioKnapsack({1}, {1}, INT_MAX); }
            if (probe == "ratio-size") { ratioKnapsack({1}, {1, 2}, 1); }
            if (probe == "ratio-weight") { ratioKnapsack({1, 2}, {1, 0}, 1); }
            if (probe == "ratio-sum") { ratioKnapsack({lng(1) << 61, lng(1) << 61}, {1, 1}, 1); }
            if (probe == "ratio-weight-sum") { ratioKnapsack({1, 1}, {lng(1) << 61, lng(1) << 61}, 1); }
            return 1;}}
    if ((DEBUG_CONTAINERS || SANITIZED) && mode == "stress") { mode = "full"; }
    exhaustiveCases(mode); regressionCases(); randomCases(mode); slowCases(mode); largeCases(mode);
    cout << "PASS fractional programming cases=" << cases << " checks=" << checks << " seed=" << test_seed << '\n';}
