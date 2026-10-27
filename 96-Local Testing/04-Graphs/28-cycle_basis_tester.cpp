#include "../../04-Graphs/28-cycle_basis.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context, at;
    lng checks = 0, cases = 0;
    // libstdc++ debug mode at -O0 is about 20x slower, so its stress run keeps full-mode rounds and shapes.
#ifdef _GLIBCXX_DEBUG
    const bool CHECKED = true;
#else
    const bool CHECKED = false;
#endif
    auto last = std::chrono::steady_clock::now();
    // Seconds since the previous section ended.
    string elapsed() {
        auto now = std::chrono::steady_clock::now();
        double s = std::chrono::duration<double>(now - std::exchange(last, now)).count();
        return " time=" + std::to_string(int(s * 100) / 100.0).substr(0, 5) + "s";}
    template<class T>
    string str(const T &x) {
        if constexpr (std::is_same_v<T, ulll>) {
            string s; ulll y = x;
            do { s += char('0' + int(y % 10)); y /= 10; } while (y);
            reverse(s.begin(), s.end());
            return s;}
        else if constexpr (std::is_integral_v<T>) { return std::to_string(x); }
        else {
            string s = "[";
            for (const auto &y : x) { s += str(y) + ' '; }
            return s + "]";}}
    template<class A, class B>
    void same(const A &actual, const B &expected, const char *op) {
        ++checks;
        if (!(actual == expected)) { throw std::runtime_error(context + at + " operation=" + op + " expected=" + str(expected) + " actual=" + str(actual)); }}
    // Failure context built only when the check fails.
    template<class A, class B, class F>
    void same(const A &actual, const B &expected, const char *op, F where) {
        if (!(actual == expected)) { at += where(); }
        same(actual, expected, op);}
    void check(bool ok, const char *op) { same(ok, true, op); }
    string show(const Graph &g) {
        std::ostringstream out; out << "graph n=" << g.n << " edges(u-v:w)=";
        for (auto e : g.edges) { out << e.u << '-' << e.v << ':' << e.w << ' '; }
        bool sorted = std::ranges::all_of(g.adj, [](const vector<int> &l) { return std::ranges::is_sorted(l); });
        if (!sorted) { out << "adjacency(arcs)="; }
        for (int v = 0; v < g.n && !sorted; ++v) { out << str(g.adj[v]); }
        return out.str();}
    // Adjacency order then differs from edge-ID order, which the documented forest must follow.
    void shuffleAdj(Graph &g, std::mt19937_64 &rng) {
        for (auto &l : g.adj) { std::shuffle(l.begin(), l.end(), rng); }}
    struct Dsu {
        vector<int> p;
        explicit Dsu(int n) : p(n) { iota(p.begin(), p.end(), 0); }
        int find(int x) {
            while (p[x] != x) { x = p[x] = p[p[x]]; }
            return x;}
        bool unite(int a, int b) {
            a = find(a); b = find(b);
            if (a == b) { return false; }
            p[a] = b; return true;}
    };
    // Component ids numbered by increasing minimum vertex, through a DSU.
    vector<int> componentIds(const Graph &g, int &count) {
        Dsu d(g.n);
        for (auto e : g.edges) { d.unite(e.u, e.v); }
        vector<int> id(g.n), root(g.n, -1); count = 0;
        for (int v = 0; v < g.n; ++v) {
            int r = d.find(v);
            if (root[r] == -1) { root[r] = count++; }
            id[v] = root[r];}
        return id;}
    int componentsWithout(const Graph &g, const vector<int> &removed) {
        vector<char> gone(g.edges.size());
        for (int e : removed) { gone[e] = 1; }
        Dsu d(g.n); int res = g.n;
        for (int e = 0; e < int(g.edges.size()); ++e) {
            if (!gone[e] && d.unite(g.edges[e].u, g.edges[e].v)) { --res; }}
        return res;}
    struct Forest { vector<int> parent_edge, depth, order; };
    // The documented forest: queue BFS from each unreached vertex in increasing order, arcs in adjacency order.
    Forest refForest(const Graph &g) {
        Forest f{vector<int>(g.n, -1), vector<int>(g.n, -1), {}};
        for (int s = 0; s < g.n; ++s) {
            if (f.depth[s] != -1) { continue; }
            queue<int> q; q.push(s); f.depth[s] = 0;
            while (!q.empty()) {
                int u = q.front(); q.pop(); f.order.push_back(u);
                for (int a : g[u]) {
                    int v = g.arcs[a].to;
                    if (f.depth[v] == -1) { f.depth[v] = f.depth[u] + 1; f.parent_edge[v] = g.arcs[a].id; q.push(v); }}}}
        return f;}
    // An unrelated spanning forest: stack search from the largest unreached vertex; tree flags and root XOR distances.
    vector<char> stackForest(const Graph &g, vector<ulng> &dist) {
        vector<char> tree(g.edges.size()), seen(g.n);
        dist.assign(g.n, 0);
        for (int s = g.n - 1; s >= 0; --s) {
            if (seen[s]) { continue; }
            vector<int> st{s}; seen[s] = 1;
            while (!st.empty()) {
                int u = st.back(); st.pop_back();
                for (int a : g[u]) {
                    int v = g.arcs[a].to, id = g.arcs[a].id;
                    if (!seen[v]) { seen[v] = 1; tree[id] = 1; dist[v] = dist[u] ^ ulng(g.arcs[a].w); st.push_back(v); }}}}
        return tree;}
    // Cycle values of the stack forest in a pivot-by-top-bit basis per component.
    struct XorOracle {
        vector<int> comp, rank;
        vector<ulng> dist;
        vector<array<ulng, 64>> piv;

        explicit XorOracle(const Graph &g) {
            int c; comp = componentIds(g, c); rank.assign(c, 0); piv.assign(c, array<ulng, 64>{});
            auto tree = stackForest(g, dist);
            for (int e = 0; e < int(g.edges.size()); ++e) {
                if (tree[e]) { continue; }
                int k = comp[g.edges[e].u];
                ulng x = dist[g.edges[e].u] ^ dist[g.edges[e].v] ^ ulng(g.edges[e].w);
                for (int b = 63; b >= 0 && x; --b) {
                    if (!(x >> b & 1)) { continue; }
                    if (!piv[k][b]) { piv[k][b] = x; ++rank[k]; x = 0; }
                    else { x ^= piv[k][b]; }}}}
        ulng low(int k, ulng x) const {
            for (int b = 63; b >= 0; --b) {
                if ((x >> b & 1) && piv[k][b]) { x ^= piv[k][b]; }}
            return x;}
        ulng high(int k, ulng x) const {
            for (int b = 63; b >= 0; --b) {
                if (!(x >> b & 1) && piv[k][b]) { x ^= piv[k][b]; }}
            return x;}
        vector<ulng> rref(int k) const {
            auto p = piv[k]; vector<ulng> res;
            for (int b = 63; b >= 0; --b) {
                for (int c = b + 1; c < 64 && p[b]; ++c) {
                    if (p[c] >> b & 1) { p[c] ^= p[b]; }}}
            for (int b = 63; b >= 0; --b) {
                if (p[b]) { res.push_back(p[b]); }}
            return res;}
        vector<ulng> coset(int k, ulng x0) const {
            vector<ulng> res{x0};
            for (ulng p : piv[k]) {
                int s = p ? int(res.size()) : 0;
                for (int i = 0; i < s; ++i) { res.push_back(res[i] ^ p); }}
            sort(res.begin(), res.end());
            return res;}
    };
    vector<ulng> toRow(const vector<int> &set, int m) {
        vector<ulng> x(size_t(m + 63) / 64);
        for (int e : set) { x[size_t(e) / 64] ^= ulng(1) << (e & 63); }
        return x;}
    // Column-wise Gauss-Jordan rank over GF(2).
    int rankF2(const vector<vector<int>> &sets, int m) {
        vector<vector<ulng>> rows;
        for (const auto &s : sets) { rows.push_back(toRow(s, m)); }
        int r = 0, k = int(rows.size());
        for (int col = 0; col < m && r < k; ++col) {
            int p = r;
            while (p < k && !(rows[p][size_t(col) / 64] >> (col & 63) & 1)) { ++p; }
            if (p == k) { continue; }
            swap(rows[p], rows[r]);
            for (int i = 0; i < k; ++i) {
                if (i != r && (rows[i][size_t(col) / 64] >> (col & 63) & 1)) {
                    for (size_t j = 0; j < rows[i].size(); ++j) { rows[i][j] ^= rows[r][j]; }}}
            ++r;}
        return r;}
    // No nonempty subset of the sets XORs to zero (m <= 64, k <= 10).
    bool bruteIndependent(const vector<vector<int>> &sets) {
        int k = int(sets.size());
        vector<ulng> x(k);
        for (int i = 0; i < k; ++i) {
            for (int e : sets[i]) { x[i] ^= ulng(1) << e; }}
        for (int mask = 1; mask < (1 << k); ++mask) {
            ulng s = 0;
            for (int i = 0; i < k; ++i) {
                if (mask >> i & 1) { s ^= x[i]; }}
            if (!s) { return false; }}
        return true;}
    // Edge subsets (m <= 12) in which every vertex has even degree; loops add two.
    int evenSubsets(const Graph &g) {
        int m = int(g.edges.size()), res = 0;
        vector<int> inc(g.n);
        for (int e = 0; e < m; ++e) {
            if (g.edges[e].u != g.edges[e].v) { inc[g.edges[e].u] |= 1 << e; inc[g.edges[e].v] |= 1 << e; }}
        for (int mask = 0; mask < (1 << m); ++mask) {
            bool even = true;
            for (int v = 0; v < g.n && even; ++v) { even = std::popcount(uint(mask & inc[v])) % 2 == 0; }
            res += even;}
        return res;}
    // Exact u-v walk value sets by BFS over (vertex, xor) states; every weight is below 2^B.
    vector<vector<ulng>> walkValues(const Graph &g, int u, int B) {
        vector<vector<char>> seen(g.n, vector<char>(size_t(1) << B));
        vector<pair<int, int>> q{{u, 0}}; seen[u][0] = 1;
        for (int i = 0; i < int(q.size()); ++i) {
            auto [x, s] = q[i];
            for (int a : g[x]) {
                int y = g.arcs[a].to, t = s ^ int(g.arcs[a].w);
                if (!seen[y][t]) { seen[y][t] = 1; q.push_back({y, t}); }}}
        vector<vector<ulng>> res(g.n);
        for (int v = 0; v < g.n; ++v) {
            for (int t = 0; t < (1 << B); ++t) {
                if (seen[v][t]) { res[v].push_back(ulng(t)); }}}
        return res;}
    vector<vector<pair<int, int>>> forestAdj(const Graph &g, const vector<int> &parent_edge) {
        vector<vector<pair<int, int>>> adj(g.n);
        for (int e : parent_edge) {
            if (e != -1) { adj[g.edges[e].u].push_back({g.edges[e].v, e}); adj[g.edges[e].v].push_back({g.edges[e].u, e}); }}
        return adj;}
    // Edges crossing the split made by deleting forest edge t: sides labelled by a forest-only BFS.
    vector<int> bruteCut(const Graph &g, const vector<vector<pair<int, int>>> &fadj, int t) {
        vector<int> side(g.n, -1), res;
        for (int s = 0; s < g.n; ++s) {
            if (side[s] != -1) { continue; }
            side[s] = s; vector<int> q{s};
            for (int i = 0; i < int(q.size()); ++i) {
                for (auto [y, e] : fadj[q[i]]) {
                    if (e != t && side[y] == -1) { side[y] = s; q.push_back(y); }}}}
        for (int e = 0; e < int(g.edges.size()); ++e) {
            if (side[g.edges[e].u] != side[g.edges[e].v]) { res.push_back(e); }}
        return res;}
    void sameCycleBasis(const CycleBasis &a, const CycleBasis &b, const char *op) {
        same(a.parent_edge, b.parent_edge, op); same(a.depth, b.depth, op); same(a.chords, b.chords, op); same(a.cycles, b.cycles, op);}
    void sameCutBasis(const CutBasis &a, const CutBasis &b, const char *op) {
        same(a.parent_edge, b.parent_edge, op); same(a.depth, b.depth, op); same(a.tree, b.tree, op); same(a.cuts, b.cuts, op);}
    // Forest, cycles and cuts; samples < 0 checks every cut by brute force and the cycle rank.
    void verifyBases(const Graph &g, int samples, CycleBasis &cb, CutBasis &cu, std::mt19937_64 &rng) {
        at = "";
        int n = g.n, m = int(g.edges.size()), c;
        auto ids = componentIds(g, c);
        auto f = refForest(g);
        cb = fundamentalCycleBasis(g); cu = fundamentalCuts(g);
        CsrGraph h(g);
        sameCycleBasis(fundamentalCycleBasis(g), cb, "fundamentalCycleBasis repeated call");
        sameCycleBasis(fundamentalCycleBasis(h), cb, "fundamentalCycleBasis CsrGraph agrees with Graph");
        sameCutBasis(fundamentalCuts(h), cu, "fundamentalCuts CsrGraph agrees with Graph");
        same(cb.parent_edge, f.parent_edge, "fundamentalCycleBasis parent_edge (reference BFS forest)");
        same(cb.depth, f.depth, "fundamentalCycleBasis depth (reference BFS forest)");
        vector<int> first(c, -1);
        for (int v = n - 1; v >= 0; --v) { first[ids[v]] = v; }
        vector<char> tree(m);
        int forest = 0;
        for (int v = 0; v < n; ++v) {
            int e = cb.parent_edge[v];
            check((e == -1) == (v == first[ids[v]]), "forest roots are the component minima");
            if (e == -1) { same(cb.depth[v], 0, "root depth"); continue; }
            check(0 <= e && e < m, "parent_edge in range");
            auto [a, b, w] = g.edges[e];
            check(a != b && (a == v || b == v), "parent edge is incident and not a loop");
            same(cb.depth[a ^ b ^ v], cb.depth[v] - 1, "parent at depth - 1");
            check(!tree[e], "parent edges distinct"); tree[e] = 1; ++forest;}
        same(forest, n - c, "forest has n - c edges");
        for (auto e : g.edges) { check(abs(cb.depth[e.u] - cb.depth[e.v]) <= 1, "depth is BFS distance (every edge spans at most one level)"); }
        vector<int> chords, trees;
        for (int e = 0; e < m; ++e) { (tree[e] ? trees : chords).push_back(e); }
        same(cb.chords, chords, "chords are the non-forest edges ascending");
        same(int(cb.cycles.size()), m - n + c, "cycle count m - n + c");
        vector<int> mark(n, -1);
        vector<int> treeIndex(m, -1);
        for (int i = 0; i < int(trees.size()); ++i) { treeIndex[trees[i]] = i; }
        // dual[t] lists, ascending, the chords whose cycle uses tree[t].
        vector<vector<int>> dual(trees.size());
        for (int i = 0; i < int(cb.cycles.size()) && i < int(chords.size()); ++i) {
            const auto &cyc = cb.cycles[i];
            check(!cyc.empty() && cyc[0] == chords[i], "cycles[i][0] is chords[i]");
            int s = g.edges[chords[i]].u, x = g.edges[chords[i]].v;
            bool ok = true; mark[s] = i;
            for (int j = 1; j < int(cyc.size()) && ok; ++j) {
                int e = cyc[j];
                ok = mark[x] != i && 0 <= e && e < m && tree[e] && (g.edges[e].u == x || g.edges[e].v == x);
                mark[x] = i;
                if (ok) { x ^= g.edges[e].u ^ g.edges[e].v; dual[treeIndex[e]].push_back(chords[i]); }}
            same(ok && x == s, true, "cycle is a simple closed walk u -> v by the chord, back along forest edges", [&] { return " cycle=" + str(cyc); });}
        if (samples < 0 && m <= 400) { same(rankF2(cb.cycles, m), int(cb.cycles.size()), "fundamental cycles independent (brute rank)"); }
        same(cu.parent_edge, cb.parent_edge, "fundamentalCuts parent_edge equals the cycle forest");
        same(cu.depth, cb.depth, "fundamentalCuts depth equals the cycle forest");
        same(cu.tree, trees, "fundamentalCuts tree = forest edges ascending");
        same(cu.cuts.size(), trees.size(), "one cut per forest edge");
        auto fadj = forestAdj(g, f.parent_edge);
        bool duality = true;
        for (int i = 0; i < int(cu.cuts.size()); ++i) {
            const auto &cut = cu.cuts[i];
            bool ok = std::is_sorted(cut.begin(), cut.end()) && std::adjacent_find(cut.begin(), cut.end()) == cut.end() && std::binary_search(cut.begin(), cut.end(), trees[i]);
            int j = 0;
            for (int e : cut) {
                ok = ok && 0 <= e && e < m && g.edges[e].u != g.edges[e].v;
                if (ok && !tree[e]) { duality = duality && j < int(dual[i].size()) && dual[i][j++] == e; }}
            duality = duality && j == int(dual[i].size());
            same(ok, true, "cut ascending, distinct, contains its forest edge, no loops", [&] { return " tree_edge=" + std::to_string(trees[i]); });}
        int brute = samples < 0 ? int(trees.size()) : min(samples, int(trees.size()));
        for (int t = 0; t < brute; ++t) {
            int i = samples < 0 ? t : int(rng() % trees.size());
            same(cu.cuts[i], bruteCut(g, fadj, trees[i]), "fundamentalCuts cut versus forest-split brute force", [&] { return " tree_edge=" + std::to_string(trees[i]); });}
        check(duality, "duality: chord c in cuts[t] iff tree[t] in the cycle of c");}
    void verifyDimension(const Graph &g) {
        int c; componentIds(g, c);
        int dim = cycleSpaceDimension(g), m = int(g.edges.size());
        same(dim, m - g.n + c, "cycleSpaceDimension = m - n + DSU components");
        same(cycleSpaceDimension(CsrGraph(g)), dim, "cycleSpaceDimension CsrGraph");
        if (m <= 12) { same(1 << dim, evenSubsets(g), "2^cycleSpaceDimension = even-degree edge subsets"); }}
    void verifyIndependence(const Graph &g, std::mt19937_64 &rng, const CycleBasis &cb, const CutBasis &cu) {
        int m = int(g.edges.size());
        at = "";
        same(isIndependentF2(g, vector<vector<int>>{}), true, "isIndependentF2 zero sets");
        same(isIndependentF2(g, vector<vector<int>>(1)), false, "isIndependentF2 one empty set");
        for (int t = 0; t < 24; ++t) {
            int k = int(rng() % ulng(min(10, m + 3) + 1));
            vector<vector<int>> sets(k);
            for (auto &s : sets) {
                int len = m ? int(rng() % 5) : 0;
                for (int i = 0; i < len; ++i) { s.push_back(int(rng() % ulng(m))); }
                if (len && rng() % 3 == 0) { s.push_back(s[0]); }}
            if (k >= 3 && rng() % 3 == 0) {
                sets[2] = sets[0]; sets[2].insert(sets[2].end(), sets[1].begin(), sets[1].end());}
            bool expected = m <= 64 ? bruteIndependent(sets) : rankF2(sets, m) == k;
            auto where = [&] { return " sets=" + str(sets); };
            same(isIndependentF2(g, sets), expected, "isIndependentF2 random sets with duplicates (brute subsets)", where);
            if (t == 0) { same(isIndependentF2(CsrGraph(g), sets), expected, "isIndependentF2 CsrGraph", where); }}
        same(isIndependentF2(g, cb.cycles), true, "isIndependentF2 fundamental cycles");
        if (cb.cycles.size() >= 2) {
            auto sets = cb.cycles; sets.push_back(sets[0]); sets.back().insert(sets.back().end(), sets[1].begin(), sets[1].end());
            same(isIndependentF2(g, sets), false, "isIndependentF2 cycles plus the sum of two (concatenation cancels)");}
        same(rankF2(cu.cuts, m), int(cu.cuts.size()), "fundamental cuts independent (brute rank)");
        same(isIndependentF2(g, cu.cuts), true, "isIndependentF2 fundamental cuts");
        auto both = cb.cycles; both.insert(both.end(), cu.cuts.begin(), cu.cuts.end());
        same(isIndependentF2(g, both), rankF2(both, m) == int(both.size()), "isIndependentF2 cycles and cuts (brute rank)");
        if (m >= 1) {
            vector<vector<int>> over(m + 1, vector<int>{0});
            for (int i = 0; i <= m; ++i) { over[i][0] = i % m; }
            same(isIndependentF2(g, over), false, "isIndependentF2 k > m");}}
    // Chord labels of seeds s and s + 1 must not be one edge apart (the old splitMix64(seed + e) labels were).
    void checkNoShift(const CutSpaceHash &a, const CutSpaceHash &b, const vector<int> &chords) {
        bool shifted = true; int pairs = 0;
        for (int i = 0; i + 1 < int(chords.size()); ++i) {
            if (chords[i + 1] == chords[i] + 1) { ++pairs; shifted = shifted && a.label[chords[i] + 1] == b.label[chords[i]]; }}
        if (chords.size() >= 3 && pairs) { check(!shifted, "CutSpaceHash adjacent seeds s, s + 1 give no one-edge label shift"); }}
    void verifyHash(const Graph &g, std::mt19937_64 &rng, const vector<int> &chords) {
        int m = int(g.edges.size()), c = componentsWithout(g, {});
        ulng s1 = seed * 3 + rng() % 1000, s2 = s1 + 1 + rng() % 1000;
        CutSpaceHash h1(g, s1), h2(g, s2), hd(g);
        at = "";
        same(CutSpaceHash(g, s1).label, h1.label, "CutSpaceHash same seed same label (repeat)");
        same(CutSpaceHash(CsrGraph(g), s1).label, h1.label, "CutSpaceHash same seed same label (CsrGraph)");
        same(int(h1.label.size()), m, "CutSpaceHash label size");
        if (!chords.empty()) { check(h1.label != h2.label, "CutSpaceHash different seeds give different chord labels"); }
        checkNoShift(h1, CutSpaceHash(g, s1 + 1), chords);
        auto test = [&](vector<int> es, bool all) {
            bool expected = componentsWithout(g, es) > c;
            std::shuffle(es.begin(), es.end(), rng);
            auto where = [&] { return " removed=" + str(es); };
            same(h1.disconnects(es), expected, "CutSpaceHash::disconnects versus deletion DSU", where);
            same(h2.disconnects(es), expected, "CutSpaceHash::disconnects versus deletion DSU (second seed)", where);
            if (all) { same(hd.disconnects(es), expected, "CutSpaceHash::disconnects default seed", [&] { return where() + " default_seed=" + std::to_string(CustomHash::rnd); }); }};
        if (m <= 8) {
            for (int mask = 0; mask < (1 << m); ++mask) {
                vector<int> es;
                for (int e = 0; e < m; ++e) {
                    if (mask >> e & 1) { es.push_back(e); }}
                test(es, mask % 7 == 0);}}
        else {
            vector<int> ids(m); iota(ids.begin(), ids.end(), 0);
            for (int t = 0; t < 100; ++t) {
                std::shuffle(ids.begin(), ids.end(), rng);
                test(vector<int>(ids.begin(), ids.begin() + int(rng() % ulng(min(6, m) + 1))), t % 7 == 0);}}
        at = "";}
    void verifyXorSmall(const Graph &g, int B) {
        XorWalk xw(g);
        for (int u = 0; u < g.n; ++u) {
            auto vals = walkValues(g, u, B);
            for (int v = 0; v < g.n; ++v) {
                const auto &s = vals[v];
                auto where = [&] { return " u=" + std::to_string(u) + " v=" + std::to_string(v) + " walk_values=" + str(s); };
                same(xw.count(u, v), ulll(s.size()), "XorWalk::count versus (vertex, xor) BFS", where);
                for (ulng x = 0; x <= ulng(1) << B; ++x) { same(xw.contains(u, v, x), std::binary_search(s.begin(), s.end(), x), "XorWalk::contains versus (vertex, xor) BFS", [&] { return where() + " x=" + std::to_string(x); }); }
                same(xw.contains(u, v, ~ulng(0)), false, "XorWalk::contains all-ones", where);
                if (s.empty()) { continue; }
                same(xw.minXorWalk(u, v), s.front(), "XorWalk::minXorWalk versus (vertex, xor) BFS", where);
                same(xw.maxXorWalk(u, v), s.back(), "XorWalk::maxXorWalk versus (vertex, xor) BFS", where);
                for (int k = 0; k < int(s.size()); ++k) { same(xw.kth(u, v, ulng(k)), s[k], "XorWalk::kth versus (vertex, xor) BFS", [&] { return where() + " k=" + std::to_string(k); }); }}}
        at = "";}
    // Full-range weights against the stack-forest oracle; queries < 0 checks every vertex pair.
    void verifyXorFull(const Graph &g, std::mt19937_64 &rng, int queries) {
        at = "";
        CsrGraph h(g);
        XorWalk xw(g), xc(h);
        same(xc.component, xw.component, "XorWalk CsrGraph component"); same(xc.dist, xw.dist, "XorWalk CsrGraph dist"); same(xc.basis, xw.basis, "XorWalk CsrGraph basis");
        XorOracle o(g); auto f = refForest(g);
        int c; auto ids = componentIds(g, c);
        same(xw.component, ids, "XorWalk::component ids by increasing minimum vertex");
        vector<ulng> d(g.n);
        for (int v : f.order) {
            if (f.parent_edge[v] != -1) { auto e = g.edges[f.parent_edge[v]]; d[v] = d[e.u ^ e.v ^ v] ^ ulng(e.w); }}
        same(xw.dist, d, "XorWalk::dist along the BFS forest");
        same(int(xw.basis.size()), c, "XorWalk::basis one per component");
        for (int k = 0; k < c && k < int(xw.basis.size()); ++k) { same(xw.basis[k], o.rref(k), "XorWalk::basis fully reduced decreasing (stack-forest RREF)"); }
        auto query = [&](int u, int v) {
            at = " u=" + std::to_string(u) + " v=" + std::to_string(v);
            int k = ids[u]; ulll cnt = xw.count(u, v);
            if (ids[v] != k) {
                same(cnt, ulll(0), "XorWalk::count disconnected");
                same(xw.contains(u, v, rng()), false, "XorWalk::contains disconnected");
                same(xw.contains(u, v, o.dist[u] ^ o.dist[v]), false, "XorWalk::contains disconnected (tree value)");
                return;}
            same(cnt, ulll(1) << o.rank[k], "XorWalk::count = 2^rank");
            ulng x0 = o.dist[u] ^ o.dist[v], lo = o.low(k, x0), hi = o.high(k, x0), known = x0;
            same(xw.minXorWalk(u, v), lo, "XorWalk::minXorWalk versus pivot basis");
            same(xw.maxXorWalk(u, v), hi, "XorWalk::maxXorWalk versus pivot basis");
            for (ulng p : o.piv[k]) {
                if (rng() & 1) { known ^= p; }}
            same(xw.contains(u, v, known), true, "XorWalk::contains a known walk value");
            ulng r = rng();
            same(xw.contains(u, v, r), o.low(k, r ^ x0) == 0, "XorWalk::contains random value");
            if (o.rank[k] <= 7) {
                auto all = o.coset(k, x0);
                for (int i = 0; i < int(all.size()); ++i) { same(xw.kth(u, v, ulng(i)), all[i], "XorWalk::kth versus enumerated walk values"); }
                return;}
            same(xw.kth(u, v, 0), lo, "XorWalk::kth(0) = min");
            same(xw.kth(u, v, ulng(cnt - 1)), hi, "XorWalk::kth(count - 1) = max");
            vector<ulng> ks(8);
            for (auto &x : ks) { x = rng() & ulng(cnt - 1); }
            sort(ks.begin(), ks.end()); ks.erase(unique(ks.begin(), ks.end()), ks.end());
            ulng prev = 0;
            for (int i = 0; i < int(ks.size()); ++i) {
                ulng val = xw.kth(u, v, ks[i]);
                check(o.low(k, val ^ x0) == 0, "XorWalk::kth is a walk value");
                check(i == 0 || val > prev, "XorWalk::kth strictly increasing");
                prev = val;}};
        if (queries < 0) {
            for (int u = 0; u < g.n; ++u) {
                for (int v = 0; v < g.n; ++v) { query(u, v); }}}
        else {
            for (int t = 0; t < queries && g.n; ++t) {
                int u = int(rng() % ulng(g.n)), v = t % 2 ? int(rng() % ulng(g.n)) : min(g.n - 1, u + int(rng() % 4));
                query(u, v);}}
        at = "";}
    // Same edges and adjacency order (arc IDs depend only on the edge order), new weights.
    Graph reweight(const Graph &g, std::mt19937_64 &rng) {
        Graph h(g.n);
        for (auto e : g.edges) { h.addEdge(e.u, e.v, rng() % 5 ? lng(rng()) : lng(rng() % 3)); }
        h.adj = g.adj;
        return h;}
    // Every check on a small or medium graph whose weights are below 2^B, optionally with shuffled adjacency.
    void verifyAll(Graph g, std::mt19937_64 &rng, int B, bool shuffle = false) {
        if (shuffle) { shuffleAdj(g, rng); }
        context = show(g);
        CycleBasis cb; CutBasis cu;
        verifyBases(g, -1, cb, cu, rng); verifyDimension(g); verifyIndependence(g, rng, cb, cu);
        verifyHash(g, rng, cb.chords); verifyXorSmall(g, B);
        Graph h = reweight(g, rng); context = show(h);
        verifyXorFull(h, rng, g.n <= 12 ? -1 : 3000); ++cases;}
    Graph randomGraph(std::mt19937_64 &rng, int n, int m, int B) {
        Graph g(n);
        for (int i = 0; i < m; ++i) {
            int u = int(rng() % ulng(n)), v = rng() % 6 ? int(rng() % ulng(n)) : u;
            if (i && rng() % 5 == 0) { auto e = g.edges[rng() % ulng(i)]; u = e.v; v = e.u; }
            g.addEdge(u, v, lng(rng() % (ulng(1) << B)));}
        return g;}
    // Square 0-1-2-3-0 (edges 0..3); reversed lists vertex 0's arc to 3 first, so BFS reaches 2 through edge 2, not edge 1.
    Graph square(bool reversed) {
        Graph g(4); g.addEdge(0, 1); g.addEdge(1, 2); g.addEdge(2, 3); g.addEdge(3, 0);
        if (reversed) { reverse(g.adj[0].begin(), g.adj[0].end()); }
        return g;}
    void validateOracles() {
        context = "oracle self-test";
        Graph g(4); g.addEdge(0, 2); g.addEdge(2, 1); g.addEdge(0, 1); g.addEdge(3, 3);
        auto f = refForest(g);
        same(f.parent_edge, vector<int>{-1, 2, 0, -1}, "refForest parents"); same(f.depth, vector<int>{0, 1, 1, 0}, "refForest depth");
        int c; same(componentIds(g, c), vector<int>{0, 0, 0, 1}, "componentIds"); same(c, 2, "component count");
        vector<ulng> dist;
        auto tree = stackForest(g, dist);
        same(int(std::count(tree.begin(), tree.end(), 1)), 2, "stackForest spans");
        same(evenSubsets(g), 4, "even subsets: triangle and loop");
        Graph tri(3); tri.addEdge(0, 1, 1); tri.addEdge(1, 2, 2); tri.addEdge(2, 0, 4);
        auto w = walkValues(tri, 0, 3);
        same(w[0], vector<ulng>{0, 7}, "walkValues closed"); same(w[1], vector<ulng>{1, 6}, "walkValues 0-1");
        XorOracle o(tri);
        same(o.rref(0), vector<ulng>{7}, "XorOracle rref"); same(o.low(0, o.dist[0] ^ o.dist[1]), ulng(1), "XorOracle low"); same(o.high(0, o.dist[0] ^ o.dist[1]), ulng(6), "XorOracle high");
        same(o.coset(0, o.dist[0] ^ o.dist[1]), vector<ulng>{1, 6}, "XorOracle coset");
        Graph p(3); p.addEdge(0, 1); p.addEdge(1, 2);
        same(evenSubsets(p), 1, "even subsets: path"); same(evenSubsets(tri), 2, "even subsets: triangle");
        check(bruteIndependent({{0}, {1}}) && !bruteIndependent({{0}, {1}, {0, 1}}) && !bruteIndependent({{0, 0}}) && bruteIndependent({}), "bruteIndependent hand cases");
        same(rankF2({{0}, {1}, {0, 1}, {2, 2}}, 3), 2, "rankF2 hand case");
        same(componentsWithout(p, {0}), 2, "deletion DSU path"); same(componentsWithout(tri, {0}), 1, "deletion DSU triangle one edge");
        same(componentsWithout(tri, {0, 1}), 2, "deletion DSU triangle two edges"); same(componentsWithout(g, {3}), 2, "deletion DSU loop");
        Graph q(3); q.addEdge(0, 1); q.addEdge(1, 2); q.addEdge(0, 2);
        auto fq = refForest(q); auto adj = forestAdj(q, fq.parent_edge);
        same(bruteCut(q, adj, 0), vector<int>{0, 1}, "bruteCut forest edge 0"); same(bruteCut(q, adj, 2), vector<int>{1, 2}, "bruteCut forest edge 2");
        same(refForest(square(true)).parent_edge, vector<int>{-1, 0, 2, 3}, "refForest follows adjacency order, not edge IDs");
        cout << "PASS oracle self-tests (reference BFS, stack forest, XOR BFS, pivot basis, even subsets, rank, deletion DSU, forest cut)" << elapsed() << '\n';}
    void special() {
        std::mt19937_64 rng(seed ^ 0x51);
        verifyAll(Graph(0), rng, 1); verifyAll(Graph(1), rng, 1);
        Graph one(1); one.addEdge(0, 0, 0); one.addEdge(0, 0, 5); one.addEdge(0, 0, 5);
        verifyAll(one, rng, 3);
        context = show(one);
        same(fundamentalCycleBasis(one).cycles, vector<vector<int>>{{0}, {1}, {2}}, "loops are chords with one-edge cycles");
        Graph par(2); par.addEdge(0, 1, 3); par.addEdge(1, 0, 3);
        context = show(par);
        same(fundamentalCycleBasis(par).cycles, vector<vector<int>>{{1, 0}}, "parallel chord gives a two-edge cycle");
        same(XorWalk(par).count(0, 0), ulll(1), "parallel equal weights cancel");
        verifyAll(par, rng, 2);
        Graph iso(6); iso.addEdge(1, 3, 1); iso.addEdge(3, 1, 2); iso.addEdge(4, 4, 3);
        verifyAll(iso, rng, 2);
        for (int n = 1; n <= 9; ++n) {
            Graph k(n);
            for (int u = 0; u < n; ++u) {
                for (int v = u + 1; v < n; ++v) { k.addEdge(u, v, lng(rng() % 16)); }}
            verifyAll(k, rng, 4);}
        Graph tree(9);
        for (int v = 1; v < 9; ++v) { tree.addEdge(int(rng() % ulng(v)), v, lng(rng() % 16)); }
        verifyAll(tree, rng, 4);
        check(fundamentalCycleBasis(tree).chords.empty() && cycleSpaceDimension(tree) == 0, "tree has no chords");
        for (int t = 0; t < 6; ++t) { verifyAll(randomGraph(rng, 20 + int(rng() % 30), 60 + int(rng() % 90), 4), rng, 4); }
        Graph full(2);
        for (int i = 0; i < 64; ++i) { full.addEdge(0, 0, lng(ulng(1) << i)); }
        full.addEdge(0, 1, lng(rng())); full.addEdge(1, 1, lng(rng()));
        context = show(full);
        CycleBasis cb; CutBasis cu;
        verifyBases(full, -1, cb, cu, rng); verifyDimension(full); verifyIndependence(full, rng, cb, cu); verifyHash(full, rng, cb.chords); verifyXorFull(full, rng, -1);
        XorWalk xw(full);
        same(xw.count(0, 1), ulll(1) << 64, "rank 64: count = 2^64");
        same(xw.minXorWalk(0, 0), ulng(0), "rank 64: min"); same(xw.maxXorWalk(0, 1), ~ulng(0), "rank 64: max");
        same(xw.kth(0, 1, ~ulng(0)), ~ulng(0), "rank 64: kth(2^64 - 1)");
        for (int t = 0; t < 1000; ++t) {
            ulng k = rng();
            same(xw.kth(0, 1, k), k, "rank 64: kth(k) = k"); same(xw.kth(0, 0, k), k, "rank 64: closed kth(k) = k");}
        context = "square with vertex 0's adjacency reversed";
        same(fundamentalCycleBasis(square(true)).parent_edge, vector<int>{-1, 0, 2, 3}, "forest follows adjacency order, not edge IDs");
        same(fundamentalCuts(square(true)).tree, vector<int>{0, 2, 3}, "fundamentalCuts forest follows adjacency order");
        same(fundamentalCycleBasis(square(false)).parent_edge, vector<int>{-1, 0, 1, 3}, "forest of the unreversed square");
        verifyAll(square(true), rng, 1);
        Graph loops(1);
        for (int i = 0; i < 8; ++i) { loops.addEdge(0, 0); }
        context = show(loops);
        vector<int> all(8); iota(all.begin(), all.end(), 0);
        for (int t = 0; t < 64; ++t) {
            ulng s = t == 0 ? 0 : t == 1 ? ~ulng(0) : t == 2 ? seed : rng();
            CutSpaceHash a(loops, s), b(loops, s + 1);
            at = " seed=" + std::to_string(s);
            same(CutSpaceHash(loops, s).label, a.label, "CutSpaceHash seed determinism (all chords)");
            checkNoShift(a, b, all);
            auto sorted = a.label; sort(sorted.begin(), sorted.end());
            check(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end(), "CutSpaceHash chord labels distinct");}
        at = "";
        cout << "PASS special cases (n=0, n=1, loops, parallel, isolated, K1..K9, tree, medium m>64, rank 64, adjacency-order forest, adjacent seeds)" << elapsed() << '\n';}
    void exhaustive() {
        std::mt19937_64 rng(seed);
        int bound = mode == "quick" ? 4 : 5; lng before = cases;
        for (int n = 0; n <= bound; ++n) {
            int pairs = n * (n - 1) / 2;
            for (int mask = 0; mask < (1 << pairs); ++mask) {
                Graph g(n); int i = 0;
                for (int u = 0; u < n; ++u) {
                    for (int v = u + 1; v < n; ++v) {
                        if (mask >> i++ & 1) { g.addEdge(u, v, lng(rng() % 16)); }}}
                verifyAll(g, rng, 4);
                if (n >= 3 && mask % 3 == 2) { verifyAll(g, rng, 4, true); }}}
        cout << "PASS every simple graph n<=" << bound << " (a third again with shuffled adjacency) cases=" << cases - before << elapsed() << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed ^ 0x9e);
        int rounds = mode == "quick" ? 300 : mode == "full" || CHECKED ? 3000 : 15000;
        for (int r = 0; r < rounds; ++r) {
            int n = 1 + int(rng() % 7), m = int(rng() % 13), B = 1 + int(rng() % 4);
            if (r % 100 == 99) { n = 15 + int(rng() % 30); m = 40 + int(rng() % 80); }
            verifyAll(randomGraph(rng, n, m, B), rng, B, r % 3 == 1);}
        cout << "PASS random multigraphs with loops n<=7 m<=12 (1% medium n<=44 m<=119, a third with shuffled adjacency) rounds=" << rounds << elapsed() << '\n';}
    struct Tally { int exact = 0, yes = 0, onesided = 0; };
    // Sizes up to 40 (44 for fixed seeds, false-true odds below 2^-20 per set, deterministic) compare exactly; above, a false true has probability up to 2^(k - 64), so only true answers are checked.
    void hashSet(const CutSpaceHash &h, const Graph &g, int c, vector<int> es, std::mt19937_64 &rng, Tally &tally, const char *op, bool fixed = true) {
        bool expected = componentsWithout(g, es) > c;
        std::shuffle(es.begin(), es.end(), rng);
        bool got = h.disconnects(es);
        auto where = [&] { return " k=" + std::to_string(es.size()) + " removed=" + str(es); };
        if (es.size() <= (fixed ? 44u : 40u)) { same(got, expected, op, where); ++tally.exact; tally.yes += expected; }
        else if (expected) { same(got, true, op, where); ++tally.onesided; }
        at = "";}
    // Adds distinct elements of pool not yet in es until es has k (fewer if the pool runs out).
    void pad(vector<int> &es, const vector<int> &pool, int k, std::mt19937_64 &rng) {
        set<int> in(es.begin(), es.end());
        for (int tries = 0; int(es.size()) < k && tries < 40 * k; ++tries) {
            int e = pool[rng() % pool.size()];
            if (in.insert(e).second) { es.push_back(e); }}}
    // Distinct sets of 7..64 edges: random, exactly 64, BFS-forest chords (never disconnecting), a fundamental cut whole or minus one edge padded with chords, a vertex star padded at random.
    vector<vector<int>> midSets(const Graph &g, std::mt19937_64 &rng, const CycleBasis &cb, const CutBasis &cu, int count) {
        int m = int(g.edges.size());
        vector<int> ids(m); iota(ids.begin(), ids.end(), 0);
        vector<vector<int>> res;
        for (int t = 0; m >= 7 && t < count; ++t) {
            vector<int> es;
            int type = t % 5, k = 7 + int(rng() % ulng(min(64, m) - 6));
            if (type == 1 && m >= 64) { k = 64; }
            if (type == 2 && cb.chords.size() >= 7) { pad(es, cb.chords, 7 + int(rng() % ulng(min<lng>(40, lng(cb.chords.size())) - 6)), rng); }
            else if (type == 3 && !cu.cuts.empty()) {
                const auto &cut = cu.cuts[rng() % cu.cuts.size()];
                if (cut.size() > 40) { continue; }
                es = cut;
                if (es.size() >= 2 && rng() % 2) { es.erase(es.begin() + int(rng() % es.size())); }
                if (!cb.chords.empty()) { pad(es, cb.chords, max(7, int(rng() % 41)), rng); }}
            else if (type == 4) {
                int v = g.edges[rng() % ulng(m)].u;
                for (int a : g[v]) {
                    int e = g.arcs[a].id;
                    if (g.edges[e].u != g.edges[e].v && int(es.size()) < 64) { es.push_back(e); }}
                pad(es, ids, max(int(es.size()), k), rng);}
            else { pad(es, ids, k, rng); }
            if (es.size() >= 7) { res.push_back(es); }}
        return res;}
    void midHash() {
        std::mt19937_64 rng(seed ^ 0x2f);
        int rounds = mode == "quick" ? 40 : mode == "full" || CHECKED ? 400 : 1500;
        Tally tally;
        for (int r = 0; r < rounds; ++r) {
            int n = 2 + int(rng() % 11), m = 20 + int(rng() % 61);
            Graph g = randomGraph(rng, n, m, 1);
            if (r % 2) { shuffleAdj(g, rng); }
            context = show(g) + " hash_seed=" + std::to_string(seed + ulng(r));
            CutSpaceHash h(g, seed + ulng(r));
            auto cb = fundamentalCycleBasis(g); auto cu = fundamentalCuts(g);
            int c = componentsWithout(g, {});
            for (auto &es : midSets(g, rng, cb, cu, 15)) { hashSet(h, g, c, es, rng, tally, "CutSpaceHash::disconnects 7..64 edges versus deletion DSU"); }
            if (r % 10 == 0) {
                CutSpaceHash hd(g); context = show(g) + " default_seed=" + std::to_string(CustomHash::rnd);
                for (auto &es : midSets(g, rng, cb, cu, 5)) { hashSet(hd, g, c, es, rng, tally, "CutSpaceHash::disconnects 7..64 edges default seed", false); }}
            ++cases;}
        cout << "PASS CutSpaceHash 7..64-edge sets on dense multigraphs n<=12 m<=80 rounds=" << rounds << " exact(k<=44, default seed k<=40)=" << tally.exact << " (disconnecting " << tally.yes << ") one-sided(k>44)=" << tally.onesided << elapsed() << '\n';}
    Graph largeShape(std::mt19937_64 &rng, int shape, int n) {
        Graph g(n);
        if (shape == 0) {
            for (int v = 1; v < n; ++v) { g.addEdge(v - 1, v, lng(rng())); }}
        else if (shape == 1) {
            for (int v = 0; v < n; ++v) { g.addEdge(v, (v + 1) % n, lng(rng())); }}
        else if (shape == 2) {
            for (int v = 1; v < n; ++v) { g.addEdge(int(rng() % ulng(v)), v, lng(rng())); }
            for (int i = 0; i < n / 4; ++i) { int u = int(rng() % ulng(n)); g.addEdge(u, min(n - 1, u + 1 + int(rng() % 6)), lng(rng())); }}
        else {
            for (int i = 0; i < n / 2; ++i) { g.addEdge(int(rng() % ulng(n)), int(rng() % ulng(n)), lng(rng())); }
            for (int i = 0; i < n / 50; ++i) { int u = int(rng() % ulng(n)); g.addEdge(u, u, lng(rng())); }
            for (int i = 0; i < n / 50; ++i) { auto e = g.edges[rng() % g.edges.size()]; g.addEdge(e.v, e.u, lng(rng())); }}
        return g;}
    void large() {
        std::mt19937_64 rng(seed ^ 0x6d);
        int n = mode == "quick" ? 3000 : mode == "full" || CHECKED ? 200000 : 300000;
        int queries = mode == "quick" ? 2000 : 6000, sets = mode == "quick" ? 60 : 40;
        const char *names[] = {"path", "cycle", "random tree + local chords", "random sparse disconnected with loops and multiedges (shuffled adjacency)"};
        for (int shape = 0; shape < 4; ++shape) {
            Graph g = largeShape(rng, shape, n);
            if (shape == 3) { shuffleAdj(g, rng); }
            int m = int(g.edges.size());
            context = string("large shape=") + names[shape] + " n=" + std::to_string(n) + " m=" + std::to_string(m) + " seed=" + std::to_string(seed);
            CycleBasis cb; CutBasis cu;
            verifyBases(g, 8, cb, cu, rng); verifyDimension(g); verifyXorFull(g, rng, queries);
            vector<vector<int>> part(cb.cycles.begin(), cb.cycles.begin() + min<lng>(64, lng(cb.cycles.size())));
            same(isIndependentF2(g, part), true, "large isIndependentF2 64 fundamental cycles");
            if (part.size() >= 2) {
                part.push_back(part[0]); part.back().insert(part.back().end(), part[1].begin(), part[1].end());
                same(isIndependentF2(g, part), false, "large isIndependentF2 cycles plus a sum");}
            CutSpaceHash h(g, seed + ulng(shape));
            int c = componentsWithout(g, {}), yes = 0;
            for (int t = 0; t < sets; ++t) {
                int k = 1 + int(rng() % 6), s = int(rng() % ulng(m));
                set<int> es;
                while (int(es.size()) < k) {
                    int i = int(es.size());
                    es.insert(t % 2 && s + i < m ? s + i : int(rng() % ulng(m)));}
                vector<int> list(es.begin(), es.end());
                bool expected = componentsWithout(g, list) > c;
                at = " removed=" + str(list); yes += expected;
                same(h.disconnects(list), expected, "large CutSpaceHash::disconnects versus deletion DSU");}
            if (shape <= 1) {
                for (int t = 0; t < 5000; ++t) {
                    int a = int(rng() % ulng(m)), b = int(rng() % ulng(m));
                    at = " removed=" + std::to_string(a) + "," + std::to_string(b);
                    same(h.disconnects({a}), shape == 0, "large CutSpaceHash path/cycle single edge");
                    if (a != b) { same(h.disconnects({a, b}), true, "large CutSpaceHash path/cycle two edges"); }}}
            Tally tally;
            for (auto &es : midSets(g, rng, cb, cu, mode == "quick" ? 15 : 30)) { hashSet(h, g, c, es, rng, tally, "large CutSpaceHash::disconnects 7..64 edges versus deletion DSU"); }
            if (m > 64) {
                vector<int> many(65); iota(many.begin(), many.end(), 0);
                at = " removed=[0,65)"; same(h.disconnects(many), true, "large CutSpaceHash more than 64 edges report true");}
            at = "";
            cout << "PASS large " << names[shape] << " n=" << n << " m=" << m << " chords=" << cb.chords.size() << " disconnecting_sets=" << yes << "/" << sets << " mid_exact=" << tally.exact << " (disconnecting " << tally.yes << ") mid_one_sided=" << tally.onesided << elapsed() << '\n';}}
    void invalid(const string &name) {
        Graph g(4); g.addEdge(0, 1, 1); g.addEdge(1, 2, 2); g.addEdge(2, 0, 4);
        if (name == "directed") { fundamentalCycleBasis(Graph(2, true)); }
        else if (name == "independent-range") { isIndependentF2(g, {{0, 3}}); }
        else if (name == "max-disconnected") { XorWalk(g).maxXorWalk(0, 3); }
        else if (name == "kth-range") { XorWalk xw(g); xw.kth(0, 1, ulng(xw.count(0, 1))); }
        else if (name == "disconnects-duplicate") { CutSpaceHash(g, 1).disconnects({1, 1}); }
        else if (name == "disconnects-range") { CutSpaceHash(g, 1).disconnects({3}); }
        else if (name == "count-range") { XorWalk(g).count(0, 4); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        validateOracles(); special(); exhaustive(); randomCases(); midHash(); large();
        cout << "PASS cycle_basis seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL cycle_basis seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
