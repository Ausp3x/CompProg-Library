#include "../../04-Graphs/16-centroiddecomposition.hpp"
#include "../../02-Data Structures/00-monoids.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    std::mt19937_64 rng;
#ifdef _GLIBCXX_DEBUG
    constexpr bool CHECKED = true;
#else
    constexpr bool CHECKED = false;
#endif
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    int rnd(int n) { return int(rng() % ulng(n)); }
    lng rndRange(lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }

    // Forest of comps trees in a shape, vertices relabelled at random, weights in [lo, hi]; edges and orientations shuffled.
    Graph forest(int n, int shape, int comps, lng lo, lng hi) {
        vector<int> label(n);
        iota(label.begin(), label.end(), 0);
        std::shuffle(label.begin(), label.end(), rng);
        vector<GraphEdge> es;
        for (int b = 0, c = 0; b < n; ++c) {
            int e = c + 1 >= comps ? n : min(n, b + max(1, n / comps)), h = (e - b) / 2;
            for (int i = 1; i < e - b; ++i) {
                int p = shape == 0 ? rnd(i) : shape == 1 ? i - 1 : shape == 2 ? 0 : shape == 3 ? (i % 2 ? i - 1 : max(0, i - 2))
                    : shape == 4 ? (i - 1) / 2 : i < h ? i - 1 : rnd(max(1, h));
                es.push_back({label[b + p], label[b + i], rndRange(lo, hi)});}
            b = e;}
        std::shuffle(es.begin(), es.end(), rng);
        Graph g(n);
        for (auto [u, v, w] : es) {
            if (rng() & 1) { g.addEdge(u, v, w); } else { g.addEdge(v, u, w); }}
        return g;}
    string show(const Graph &g) {
        std::ostringstream out; out << "n=" << g.n << " edges=";
        for (auto e : g.edges) { out << e.u << '-' << e.v << ':' << e.w << ' '; }
        return out.str();}

    // Oracle: distances by DFS from every vertex; comp[v] identifies v's tree.
    struct Oracle {
        int n;
        vector<int> comp;
        vector<vector<lng>> d;
        Oracle(const Graph &g, bool weighted) : n(g.n), comp(n, -1), d(n, vector<lng>(n, 0)) {
            vector<vector<pair<int, lng>>> adj(n);
            for (auto e : g.edges) { adj[e.u].push_back({e.v, weighted ? e.w : 1}); adj[e.v].push_back({e.u, weighted ? e.w : 1}); }
            for (int s = 0; s < n; ++s) {
                vector<int> st{s}; vector<char> seen(n); seen[s] = 1;
                if (comp[s] == -1) { comp[s] = s; }
                while (!st.empty()) {
                    int u = st.back(); st.pop_back();
                    for (auto [v, w] : adj[u]) {
                        if (!seen[v]) { seen[v] = 1; d[s][v] = d[s][u] + w; comp[v] = comp[s]; st.push_back(v); }}}}}
    };

    // Structure: preorder blocks, nesting, block = component of c among levels >= level[c], centroid halving, distances.
    template<class W>
    void verifyStructure(const Graph &g, const CentroidDecomposition<W> &cd, const Oracle *o) {
        int n = g.n;
        check(cd.n == n && cd.L == int(std::bit_width(uint(n))), "n and L");
        vector<int> seen(n, 0), mark(n, -1), part(n, -1), queue;
        for (int i = 0; i < n; ++i) { check(0 <= cd.order[i] && cd.order[i] < n && !seen[cd.order[i]]++ && cd.tin[cd.order[i]] == i, "preorder permutation"); }
        vector<vector<int>> adj(n);
        for (auto e : g.edges) { adj[e.u].push_back(e.v); adj[e.v].push_back(e.u); }
        int roots = 0;
        for (int c = 0; c < n; ++c) {
            int p = cd.parent[c];
            check(0 <= cd.level[c] && cd.level[c] < cd.L, "level below L");
            if (p == -1) { ++roots; check(cd.level[c] == 0, "root level 0"); }
            else { check(cd.level[c] == cd.level[p] + 1 && cd.tin[p] < cd.tin[c] && cd.tin[c] + cd.size[c] <= cd.tin[p] + cd.size[p], "nested child block"); }
            queue.assign(1, c); mark[c] = c;
            for (int i = 0; i < int(queue.size()); ++i) {
                for (int v : adj[queue[i]]) {
                    if (mark[v] != c && cd.level[v] >= cd.level[c]) { mark[v] = c; queue.push_back(v); }}}
            check(int(queue.size()) == cd.size[c], "block size is the component size");
            for (int v : queue) { check(cd.tin[c] <= cd.tin[v] && cd.tin[v] < cd.tin[c] + cd.size[c], "component inside block"); }
            for (int s : adj[c]) {
                if (mark[s] != c || s == c || part[s] == c) { continue; }
                vector<int> piece{s};
                part[s] = c;
                for (int i = 0; i < int(piece.size()); ++i) {
                    for (int v : adj[piece[i]]) {
                        if (v != c && mark[v] == c && part[v] != c) { part[v] = c; piece.push_back(v); }}}
                check(2 * int(piece.size()) <= cd.size[c], "centroid halves its component");}
            for (int k = cd.level[c], a = c; a != -1; a = cd.parent[a], --k) {
                check(cd.level[a] == k, "ancestor levels");
                if (o) { check(lng(cd.distance(c, k)) == o->d[c][a] && o->comp[a] == o->comp[c], "distance to centroid ancestor"); }}}
        if (o) {
            std::set<int> trees(o->comp.begin(), o->comp.end());
            check(roots == int(trees.size()), "one centroid root per tree");}}

    struct SumMonoid { using S = lng; static S op(S a, S b) { return a + b; } static S e() { return 0; } };
    // (max, count of the max): commutative, neither invertible nor idempotent, so double counting shows.
    struct MaxCount {
        using S = pair<lng, int>;
        static S op(S a, S b) { return a.fi != b.fi ? max(a, b) : S{a.fi, a.se + b.se}; }
        static S e() { return {LLONG_MIN, 0}; }
    };
    struct AddMax {
        struct F { lng add, mx; };
        static F composition(F f, F g) { return {f.add + g.add, max(f.mx, g.mx)}; }
        static F id() { return {0, LLONG_MIN}; }
    };

    void operations(const Graph &g, int ops) {
        int n = g.n;
        Oracle unit(g, false), wt(g, true);
        verifyStructure(g, CentroidDecomposition<int>(g), &unit);
        verifyStructure(g, CentroidDecomposition<lng>(g, true), &wt);
        verifyStructure(g, CentroidDecomposition<lng>(CsrGraph(g), true), &wt);
        if (!n) {
            check(treeDistanceFrequency(g).empty() && countPathsWithin(g, 0) == 0, "empty graph");
            return;}
        bool nonneg = std::ranges::all_of(g.edges, [](const GraphEdge &e) { return e.w >= 0; });
        NearestMarked<lng> nm(g, nonneg);
        NearestMarked<int> nmc{CsrGraph(g)};
        DistanceSum<lll, lng> ds(g, true);
        DistanceSum<lng> dsu(g);
        vector<lng> a(n);
        for (auto &x : a) { x = rndRange(-5, 5); }
        ContourSum<lng> cs(g, a);
        ContourAdd<lng> ca(CsrGraph(g), a);
        ContourFold<SumMonoid> fs(g, a);
        vector<pair<lng, int>> fa(n);
        for (int v = 0; v < n; ++v) { fa[v] = {a[v], 1}; }
        ContourFold<MaxCount> fm(CsrGraph(g), fa);
        ContourApply<AddMax> ap(g);
        vector<AddMax::F> applied(n, AddMax::id());
        // Runtime state: contour sums and contour adds modulo a captured modulus.
        lng mod = n % 2 ? 7 : 1000000007;
        auto plusMod = [mod](lng x, lng y) { return ((x + y) % mod + mod) % mod; };
        vector<lng> am(n), addm(n, 0);
        for (int v = 0; v < n; ++v) { am[v] = plusMod(a[v], 0); }
        ContourFold fmod(g, am, Monoid{lng(0), plusMod});
        ContourApply apmod(g, MonoidAction{Monoid{lng(0), plusMod}});
        vector<char> marked(n);
        vector<lll> weight(n, 0);
        vector<lng> cur = a, plus(n, 0);
        const auto &D = nonneg ? wt.d : unit.d;
        for (int t = 0; t < ops; ++t) {
            int v = rnd(n), type = rnd(7), l = rnd(n + 2), r = l + rnd(n + 2);
            if (rng() % 8 == 0) { r = INT_MAX; }
            if (rng() % 8 == 0) { r = l; }
            lng x = rndRange(-9, 9);
            if (type == 0) {
                if (rng() % 3) { nm.mark(v); nmc.mark(v); marked[v] = 1; } else { nm.unmark(v); nmc.unmark(v); marked[v] = 0; }}
            else if (type == 1) {
                lng best = -1, bu = -1;
                for (int u = 0; u < n; ++u) {
                    if (marked[u] && unit.comp[u] == unit.comp[v]) {
                        if (best == -1 || D[v][u] < best) { best = D[v][u]; }
                        if (bu == -1 || unit.d[v][u] < bu) { bu = unit.d[v][u]; }}}
                check(nm.nearest(v) == best && nmc.nearest(v) == bu, "nearest marked");}
            else if (type == 2) {
                ds.add(v, x); dsu.add(v, x); weight[v] += x;
                lll e = 0, eu = 0;
                int q = rnd(n);
                for (int u = 0; u < n; ++u) {
                    if (unit.comp[u] == unit.comp[q]) { e += weight[u] * wt.d[q][u]; eu += weight[u] * unit.d[q][u]; }}
                check(ds.sum(q) == e && lll(dsu.sum(q)) == eu, "distance sum");}
            else if (type == 3) {
                cs.pointAdd(v, x); fs.set(v, cur[v] + x); cur[v] += x;
                fm.set(v, {cur[v], 1}); am[v] = plusMod(cur[v], 0); fmod.set(v, am[v]);}
            else if (type == 4) {
                lng e = 0, emod = 0; pair<lng, int> em = MaxCount::e();
                for (int u = 0; u < n; ++u) {
                    if (unit.comp[u] == unit.comp[v] && l <= unit.d[v][u] && unit.d[v][u] < r) { e += cur[u]; em = MaxCount::op(em, {cur[u], 1}); emod = plusMod(emod, am[u]); }}
                check(fmod.rangeContourFold(v, l, r) == emod, "rangeContourFold stateful");
                check(cs.rangeContourSum(v, l, r) == e, "rangeContourSum");
                check(fs.rangeContourFold(v, l, r) == e, "rangeContourFold sum");
                check(fm.rangeContourFold(v, l, r) == em, "rangeContourFold max-count");}
            else if (type == 5) {
                ca.rangeContourAdd(v, l, r, x);
                AddMax::F f{x, rndRange(-20, 20)};
                ap.rangeContourApply(v, l, r, f); apmod.rangeContourApply(v, l, r, x);
                for (int u = 0; u < n; ++u) {
                    if (unit.comp[u] == unit.comp[v] && l <= unit.d[v][u] && unit.d[v][u] < r) { plus[u] += x; applied[u] = AddMax::composition(f, applied[u]); addm[u] = plusMod(addm[u], x); }}}
            else {
                auto f = ap.pointGet(v);
                check(ca.pointGet(v) == a[v] + plus[v], "ContourAdd pointGet");
                check(f.add == applied[v].add && f.mx == applied[v].mx, "ContourApply pointGet");
                check(apmod.pointGet(v) == addm[v], "ContourApply stateful pointGet");}}
        vector<lng> freq(n, 0);
        lng pairs = 0, k = n ? wt.d[rnd(n)][rnd(n)] + rndRange(-1, 1) : 0;
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) {
                if (unit.comp[u] == unit.comp[v]) { ++freq[unit.d[u][v]]; pairs += wt.d[u][v] <= k; }}}
        check(treeDistanceFrequency(g) == freq && treeDistanceFrequency(CsrGraph(g)) == freq, "treeDistanceFrequency");
        check(countPathsWithin(g, k) == pairs, "countPathsWithin");
        lng all = 0;
        for (lng f : freq) { all += f; }
        check(countPathsWithin(g, LLONG_MAX) == all && countPathsWithin(g, LLONG_MIN) == 0, "countPathsWithin extremes");}

    void small() {
        int rounds = mode == "quick" ? 150 : mode == "full" ? 1500 : 8000;
        for (int t = 0; t < rounds; ++t) {
            int n = t < 6 ? t : 1 + rnd(t % 10 == 0 ? 120 : 40), shape = rnd(6), comps = rng() % 4 ? 1 : 1 + rnd(4);
            lng w = t % 4 == 0 ? 1 : t % 4 == 1 ? 1000 : t % 4 == 2 ? lng(1) << 40 : ((lng(1) << 62) - 1) / max(1, n - 1);
            Graph g = forest(n, shape, comps, t % 4 == 0 ? 1 : t % 2 ? 0 : -w, w);
            ++cases; context = "seed=" + std::to_string(seed) + " round=" + std::to_string(t) + " " + show(g);
            operations(g, 3 * n + 10);}
        // Saved regression: two centroid distances of 2^62 - 1 sum to 2^63 - 2 (the review case had 2^62, outside the domain).
        Graph g(7);
        for (auto [u, v, w] : vector<GraphEdge>{{0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {0, 4, (lng(1) << 62) - 1}, {4, 5, 0}, {4, 6, 0}}) { g.addEdge(u, v, w); }
        context = "regression path sums at 2^62 - 1";
        for (lng k : {lng(0), (lng(1) << 62) - 2, (lng(1) << 62) - 1, lng(LLONG_MAX), lng(LLONG_MIN)}) {
            check(countPathsWithin(g, k) == (k < 0 ? 0 : k < (lng(1) << 62) - 1 ? 9 : 21), "countPathsWithin at the domain bound");}
        cout << "PASS random forests n<=120 (6 shapes, 1-4 trees, unit/signed/2^40/2^62/(n-1) weights) rounds=" << rounds << '\n';}

    void frequencyMedium() {
        vector<int> sizes = mode == "quick" ? vector<int>{1700} : mode == "full" || CHECKED ? vector<int>{1700, 3000} : vector<int>{1700, 3000, 4000};
        for (int n : sizes) {
            for (int shape = 0; shape < 6; ++shape) {
                Graph g = forest(n, shape, 1, 1, 1);
                ++cases; context = "frequency n=" + std::to_string(n) + " shape=" + std::to_string(shape);
                vector<vector<int>> adj(n);
                for (auto e : g.edges) { adj[e.u].push_back(e.v); adj[e.v].push_back(e.u); }
                vector<lng> freq(n, 0);
                vector<int> d(n), queue;
                for (int s = 0; s < n; ++s) {
                    std::fill(d.begin(), d.end(), -1); d[s] = 0; queue.assign(1, s);
                    for (int i = 0; i < int(queue.size()); ++i) {
                        for (int v : adj[queue[i]]) {
                            if (d[v] == -1) { d[v] = d[queue[i]] + 1; queue.push_back(v); }}}
                    for (int v = s + 1; v < n; ++v) { ++freq[d[v]]; }}
                check(treeDistanceFrequency(g) == freq, "treeDistanceFrequency medium (NTT squares on paths)");}}
        for (int n = 1599; n <= 1602; ++n) {
            Graph g = forest(n, 1, 1, 1, 1);
            context = "frequency threshold neighbour path n=" + std::to_string(n);
            auto f = treeDistanceFrequency(g);
            for (int d = 1; d < n; ++d) { check(f[d] == n - d, "path frequency around the naive/NTT threshold (top 799/800)"); }}
        cout << "PASS frequency against BFS pairs n<=" << sizes.back() << " six shapes, paths n=1599..1602\n";}

    void large() {
#ifdef __SANITIZE_ADDRESS__
        int cap = 200000;
#else
        int cap = INT_MAX;
#endif
        int n = min(cap, mode == "quick" ? 20000 : CHECKED ? 50000 : mode == "full" ? 200000 : 1000000);
        {
            Graph path(n);
            for (int v = 1; v < n; ++v) { path.addEdge(v - 1, v); }
            context = "large path n=" + std::to_string(n);
            auto f = treeDistanceFrequency(path);
            for (int d = 1; d < n; ++d) { check(f[d] == n - d, "path frequency"); }
            check(countPathsWithin(path, 5) == 5 * lng(n) - 15, "path countPathsWithin");
            ContourSum<lng> cs(path, vector<lng>(n, 1));
            NearestMarked<int> nm(path);
            std::set<int> marks;
            for (int t = 0; t < 2000; ++t) {
                int v = rnd(n), l = rnd(n), r = l + rnd(n);
                lng e = 0;
                for (int s : {-1, 1}) { e += max(0, min(s < 0 ? v : n - 1 - v, r - 1) - max(l, 1) + 1); }
                check(cs.rangeContourSum(v, l, r) == e + (l == 0 && r > 0), "path contour count");
                if (t % 2) { nm.mark(v); marks.insert(v); }
                int q = rnd(n), best = INT_MAX;
                auto it = marks.lower_bound(q);
                if (it != marks.end()) { best = *it - q; }
                if (it != marks.begin()) { best = min(best, q - *prev(it)); }
                check(nm.nearest(q) == (marks.empty() ? -1 : best), "path nearest");}
            verifyStructure(path, CentroidDecomposition<int>(path), nullptr);}
        {
            Graph star(n);
            for (int v = 1; v < n; ++v) { star.addEdge(0, v, v); }
            context = "large star n=" + std::to_string(n);
            auto fs = treeDistanceFrequency(star);
            check(fs[1] == n - 1 && (n < 3 || fs[2] == lng(n - 1) * (n - 2) / 2), "star frequency");
            ContourFold<MaxCount> fm(star, vector<pair<lng, int>>(n, {7, 1}));
            check(fm.rangeContourFold(5, 2, 3) == pair<lng, int>{7, n - 2} && fm.rangeContourFold(0, 1, 2) == pair<lng, int>{7, n - 1}, "star contour fold");
            DistanceSum<lng, lng> ds(star, true);
            ds.add(0, 1); ds.add(n - 1, 2);
            check(ds.sum(1) == 1 + 2 * lng(n), "star weighted distance sum");
            verifyStructure(star, CentroidDecomposition<lng>(star, true), nullptr);}
        {
            Graph g = forest(n, 0, 1, 1, 1);
            context = "large random n=" + std::to_string(n);
            verifyStructure(g, CentroidDecomposition<int>(g), nullptr);
            lng total = 0;
            for (lng x : treeDistanceFrequency(g)) { total += x; }
            check(total == lng(n) * (n - 1) / 2, "random frequency total");
            ContourSum<lng> rs(g, vector<lng>(n, 1));
            ContourAdd<lng> ra(g, vector<lng>(n, 0));
            for (int t = 0; t < 200; ++t) {
                int v = rnd(n);
                check(rs.rangeContourSum(v, 0, n) == n, "random contour total");
                ra.rangeContourAdd(v, 0, n, 1);
                check(ra.pointGet(rnd(n)) == t + 1, "random contour add total");}}
        {
            Graph cat = forest(n, 3, 1, 1, 1), bin = forest(n, 4, 1, 1, 1);
            verifyStructure(cat, CentroidDecomposition<int>(cat), nullptr);
            verifyStructure(bin, CentroidDecomposition<int>(bin), nullptr);
            ContourApply<AddMax> ap(cat);
            ap.rangeContourApply(0, 0, n, {1, 3});
            check(ap.pointGet(rnd(n)).add == 1, "caterpillar apply");}
        cout << "PASS large path/star/random/caterpillar/binary n=" << n << '\n';}

    void invalid(const string &name) {
        Graph g(3); g.addEdge(0, 1); g.addEdge(1, 2);
        Graph cyc = g; cyc.addEdge(2, 0);
        if (name == "directed") { CentroidDecomposition<int>(Graph(2, true)); }
        else if (name == "cycle") { CentroidDecomposition<int>{cyc}; }
        else if (name == "distance-level") { CentroidDecomposition<int>(g).distance(0, 2); }
        else if (name == "nearest-range") { NearestMarked<int>(g).nearest(3); }
        else if (name == "negative-weight") { Graph h(2); h.addEdge(0, 1, -1); NearestMarked<lng>(h, true); }
        else if (name == "sum-range") { DistanceSum<lng>(g).add(-1, 1); }
        else if (name == "contour-size") { ContourSum<lng>(g, vector<lng>(2)); }
        else if (name == "contour-range") { ContourSum<lng>(g, vector<lng>(3)).rangeContourSum(0, 2, 1); }
        else if (name == "add-negative") { ContourAdd<lng>(g, vector<lng>(3)).rangeContourAdd(0, -1, 2, 1); }
        else if (name == "fold-cycle") { ContourFold<SumMonoid>(cyc, vector<lng>(3)); }
        else if (name == "fold-range") { ContourFold<SumMonoid>(g, vector<lng>(3)).set(3, 1); }
        else if (name == "apply-range") { ContourApply<AddMax>(g).pointGet(-1); }
        else if (name == "frequency-cycle") { treeDistanceFrequency(cyc); }
        else if (name == "paths-directed") { countPathsWithin(Graph(2, true), 1); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        rng.seed(seed);
        small(); frequencyMedium(); large();
        cout << "PASS centroid seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL centroid seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
