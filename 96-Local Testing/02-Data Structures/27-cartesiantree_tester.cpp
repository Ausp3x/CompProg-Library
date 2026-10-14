#include "../../02-Data Structures/27-cartesiantree.hpp"

namespace {
    ulng seed = 20261014;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) {
        if constexpr (std::is_integral_v<T>) { return std::to_string(x); }
        else { std::ostringstream s; s << x; return s.str(); }}
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Per-element checks build their message only on failure.
    template<typename F> void expect(bool ok, F msg) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + msg()); }}

    struct AbsLess { bool operator()(int x, int y) const { return abs(x) < abs(y); } };

    // Recursive definition: the root of [l, r) is its leftmost (or rightmost) minimum under the strict order base.
    template<typename T, typename C>
    CartesianTree brute(const vector<T> &a, C base, bool rightmost) {
        int n = int(a.size());
        CartesianTree t{-1, vector<int>(n, -1), vector<int>(n, -1), vector<int>(n, -1), vector<pair<int, int>>(n)};
        auto rec = [&](auto &&self, int l, int r, int par) -> int {
            if (l >= r) { return -1; }
            int m = l;
            for (int i = l + 1; i < r; ++i) {
                if (base(a[i], a[m]) || (rightmost && !base(a[m], a[i]))) { m = i; }}
            t.parent[m] = par; t.range[m] = {l, r};
            t.leftChild[m] = self(self, l, m, m); t.rightChild[m] = self(self, m + 1, r, m);
            return m;};
        t.root = rec(rec, 0, n, -1);
        return t;}

    void sameTree(const CartesianTree &got, const CartesianTree &want) {
        checkEqual(got.root, want.root, "root"); checkEqual(got.parent, want.parent, "parent");
        checkEqual(got.leftChild, want.leftChild, "leftChild"); checkEqual(got.rightChild, want.rightChild, "rightChild");
        for (size_t i = 0; i < got.range.size(); ++i) {
            expect(got.range[i] == want.range[i], [&] { return "range " + show(i) + " expected=" + show(want.range[i]) + " actual=" + show(got.range[i]); });}}

    // lcaRmqBridge: shape of the tour, then every (l, r) when small: min depth between first[l], first[r - 1] is argmin and lca.
    template<typename T, typename C>
    void checkBridge(const vector<T> &a, const CartesianTree &t, C base, bool rightmost, bool all) {
        int n = int(a.size());
        auto [tour, depth, first] = lcaRmqBridge(t);
        checkEqual(int(tour.size()), n ? 2 * n - 1 : 0, "tour size"); checkEqual(int(depth.size()), int(tour.size()), "depth size");
        checkEqual(int(first.size()), n, "first size");
        vector<int> dep(n, -1), seen(n, -1);
        for (int i = 0; i < int(tour.size()); ++i) {
            int v = tour[i];
            if (seen[v] < 0) { seen[v] = i; }
            if (i) {
                expect(abs(depth[i] - depth[i - 1]) == 1, [&] { return "depth step " + show(i); });
                int u = tour[i - 1];
                expect(t.parent[u] == v || t.parent[v] == u, [&] { return "tour edge " + show(i); });}
            if (dep[v] < 0) { dep[v] = depth[i]; }
            expect(depth[i] == dep[v], [&] { return "depth consistent " + show(i); });}
        if (n) {
            checkEqual(tour[0], t.root, "tour starts at root"); checkEqual(depth[0], 0, "root depth");
            checkEqual(tour.back(), t.root, "tour ends at root");}
        checkEqual(first, seen, "first occurrences");
        for (int v = 0; v < n; ++v) {
            if (t.parent[v] >= 0) { expect(dep[v] == dep[t.parent[v]] + 1, [&] { return "depth by parent " + show(v); }); }}
        auto query = [&](int l, int r) {
            int i = min(first[l], first[r - 1]), j = max(first[l], first[r - 1]), best = i;
            for (int p = i + 1; p <= j; ++p) { if (depth[p] < depth[best]) { best = p; } }
            int m = l;
            for (int x = l + 1; x < r; ++x) { if (base(a[x], a[m]) || (rightmost && !base(a[m], a[x]))) { m = x; } }
            int u = l, v = r - 1;
            while (u != v) { if (dep[u] >= dep[v]) { u = t.parent[u]; } else { v = t.parent[v]; } }
            expect(tour[best] == m, [&] { return "argmin [" + show(l) + "," + show(r) + ") expected=" + show(m) + " actual=" + show(tour[best]); });
            expect(tour[best] == u, [&] { return "lca " + show(l) + " " + show(r - 1) + " expected=" + show(u) + " actual=" + show(tour[best]); });};
        if (all) {
            for (int l = 0; l < n; ++l) { for (int r = l + 1; r <= n; ++r) { query(l, r); } }}
        else {
            Rng rng(seed ^ ulng(n));
            for (int s = 0; s < 200 && n; ++s) {
                int l = int(rnd(rng, 0, n - 1)), r = int(rnd(rng, l + 1, min(n, l + 64)));
                query(l, r);}}}

    template<typename T, typename C>
    void againstBrute(const vector<T> &a, C base, bool rightmost, bool reflexive, bool bridge) {
        CartesianTree got = reflexive ? cartesianTree(a, [&](const T &x, const T &y) { return !base(y, x); }) : cartesianTree(a, base);
        sameTree(got, brute(a, base, rightmost));
        if (bridge) { checkBridge(a, got, base, rightmost, true); }}

    // Every array of length <= len over {0, 1, 2} with less, less_equal, greater, greater_equal.
    void exhaustive(int len) {
        for (int n = 0; n <= len; ++n) {
            int total = 1;
            for (int i = 0; i < n; ++i) { total *= 3; }
            for (int code = 0; code < total; ++code) {
                vector<int> a(n);
                for (int i = 0, c = code; i < n; ++i, c /= 3) { a[i] = c % 3; }
                context = "exhaustive a=" + show(a);
                sameTree(cartesianTree(a), brute(a, std::less<int>(), false));
                sameTree(cartesianTree(a, std::less_equal<int>()), brute(a, std::less<int>(), true));
                sameTree(cartesianTree(a, std::greater<int>()), brute(a, std::greater<int>(), false));
                sameTree(cartesianTree(a, std::greater_equal<int>()), brute(a, std::greater<int>(), true));
                checkBridge(a, cartesianTree(a), std::less<int>(), false, true);
                checkBridge(a, cartesianTree(a, std::greater_equal<int>()), std::greater<int>(), true, true);}}}

    void randomSmall(Rng &rng, int rounds) {
        for (int s = 0; s < rounds; ++s) {
            int n = int(rnd(rng, 0, 60)), v = int(rnd(rng, 1, 1 << int(rnd(rng, 0, 30))));
            vector<int> a(n);
            for (int &x : a) { x = int(rnd(rng, -v, v)); }
            context = "random n=" + show(n) + " a=" + show(a);
            againstBrute(a, std::less<int>(), false, false, true);
            againstBrute(a, std::less<int>(), true, true, true);
            againstBrute(a, std::greater<int>(), false, false, false);
            againstBrute(a, AbsLess(), false, false, true);
            vector<string> w(n);
            for (auto &x : w) { x = string(size_t(rnd(rng, 0, 2)), char('a' + rnd(rng, 0, 2))); }
            againstBrute(w, std::less<string>(), false, false, false);
            vector<double> d(n);
            for (auto &x : d) { x = double(rnd(rng, -3, 3)) / 2; }
            againstBrute(d, std::greater<double>(), true, true, false);}}

    // Large and deep inputs: heap order with the tie rule, in-order = 0..n-1, and subtree intervals determine the tree.
    template<typename C>
    void properties(const vector<int> &a, C base, bool rightmost, const string &name) {
        context = "properties " + name + " n=" + show(a.size());
        int n = int(a.size());
        CartesianTree t = rightmost ? cartesianTree(a, [&](int x, int y) { return !base(y, x); }) : cartesianTree(a, base);
        int roots = 0;
        for (int i = 0; i < n; ++i) {
            int p = t.parent[i];
            if (p < 0) { ++roots; checkEqual(t.root, i, "root"); continue; }
            expect(t.leftChild[p] == i || t.rightChild[p] == i, [&] { return "child link " + show(i); });
            expect(!base(a[i], a[p]), [&] { return "heap order " + show(i); });
            if (!base(a[p], a[i])) { expect(rightmost ? p > i : p < i, [&] { return "tie rule " + show(i); }); }}
        checkEqual(roots, n ? 1 : 0, "single root");
        for (int i = 0; i < n; ++i) {
            if (t.leftChild[i] >= 0) { expect(t.parent[t.leftChild[i]] == i, [&] { return "left parent " + show(i); }); }
            if (t.rightChild[i] >= 0) { expect(t.parent[t.rightChild[i]] == i, [&] { return "right parent " + show(i); }); }}
        vector<int> order, st, lo(n), hi(n);
        for (int v = t.root; v >= 0 || !st.empty();) {
            if (v >= 0) { st.push_back(v); v = t.leftChild[v]; continue; }
            v = st.back(); st.pop_back(); order.push_back(v); v = t.rightChild[v];}
        vector<int> id(n);
        iota(id.begin(), id.end(), 0);
        checkEqual(order == id, true, "in-order is the index order");
        for (int i = 0; i < n; ++i) { lo[i] = i; hi[i] = i + 1; }
        vector<int> bfs;
        if (n) { bfs.push_back(t.root); }
        for (size_t k = 0; k < bfs.size(); ++k) {
            int v = bfs[k];
            if (t.leftChild[v] >= 0) { bfs.push_back(t.leftChild[v]); }
            if (t.rightChild[v] >= 0) { bfs.push_back(t.rightChild[v]); }}
        for (int k = int(bfs.size()) - 1; k >= 0; --k) {
            int v = bfs[k];
            if (t.leftChild[v] >= 0) { lo[v] = lo[t.leftChild[v]]; }
            if (t.rightChild[v] >= 0) { hi[v] = hi[t.rightChild[v]]; }}
        for (int i = 0; i < n; ++i) { expect(t.range[i] == pair<int, int>(lo[i], hi[i]), [&] { return "range " + show(i) + " expected=" + show(pair<int, int>(lo[i], hi[i])) + " actual=" + show(t.range[i]); }); }
        checkBridge(a, t, base, rightmost, false);}

    void large(Rng &rng, int n) {
        vector<int> up(n), down(n), same(n, 7), few(n), zig(n), rnd30(n);
        for (int i = 0; i < n; ++i) {
            up[i] = i; down[i] = n - i; few[i] = int(rnd(rng, 0, 3)); zig[i] = i % 2 ? i : -i;
            rnd30[i] = int(rnd(rng, INT_MIN, INT_MAX));}
        for (auto *a : {&up, &down, &same, &few, &zig, &rnd30}) {
            properties(*a, std::less<int>(), false, "less"); properties(*a, std::less<int>(), true, "less_equal");
            properties(*a, std::greater<int>(), false, "greater"); properties(*a, std::greater<int>(), true, "greater_equal");}}

    void edges() {
        context = "edges";
        CartesianTree e = cartesianTree(vector<int>{});
        checkEqual(e.root, -1, "empty root"); checkEqual(e.parent.size(), size_t(0), "empty parent");
        auto [tour, depth, first] = lcaRmqBridge(e);
        checkEqual(tour.size() + depth.size() + first.size(), size_t(0), "empty bridge");
        CartesianTree one = cartesianTree(vector<lng>{-5});
        checkEqual(one.root, 0, "single root"); checkEqual(one.range[0], pair<int, int>(0, 1), "single range");
        auto [t1, d1, f1] = lcaRmqBridge(one);
        checkEqual(t1, vector<int>{0}, "single tour"); checkEqual(d1, vector<int>{0}, "single depth"); checkEqual(f1, vector<int>{0}, "single first");
        CartesianTree lc = cartesianTree(vector<int>{3, 1, 4, 1, 5, 9, 2, 6});
        checkEqual(lc.root, 1, "leftmost of equal minima"); checkEqual(lc.parent[3], 1, "equal minimum is right descendant");
        checkEqual(cartesianTree(vector<int>{3, 1, 4, 1, 5, 9, 2, 6}, std::less_equal<int>()).root, 3, "rightmost with less_equal");}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "cartesiantree seed=" << seed << " mode=" << mode << '\n';
        Rng rng(seed);
        edges();
        exhaustive(mode == "quick" ? 5 : mode == "full" ? 8 : 9);
        randomSmall(rng, mode == "quick" ? 200 : mode == "full" ? 3000 : 8000);
        large(rng, mode == "quick" ? 5000 : mode == "full" ? 200000 : 500000);
        std::cout << "PASS cartesiantree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL cartesiantree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
