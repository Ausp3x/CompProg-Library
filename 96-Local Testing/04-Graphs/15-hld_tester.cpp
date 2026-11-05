#include "../../04-Graphs/15-hld.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    int qu = -1, qv = -1;
    std::mt19937_64 rng;
#ifdef _GLIBCXX_DEBUG
    constexpr bool CHECKED = true;
#else
    constexpr bool CHECKED = false;
#endif
    void check(bool ok, const char *operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " u=" + std::to_string(qu) + " v=" + std::to_string(qv) + " operation=" + operation + " expected=true actual=false"); }}
    void checkEq(lng actual, lng expected, const char *operation) {
        ++checks;
        if (actual != expected) { throw std::runtime_error(context + " u=" + std::to_string(qu) + " v=" + std::to_string(qv) + " operation=" + operation + " expected=" + std::to_string(expected) + " actual=" + std::to_string(actual)); }}
    int rnd(int n) { return int(rng() % ulng(n)); }
    lng rndRange(lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }

    // Forest of comps trees of random sizes in a shape (0 random, 1 path, 2 star, 3 caterpillar, 4 binary, 5 broom); labels, edges and orientations shuffled.
    Graph forest(int n, int shape, int comps, bool shuffle = true) {
        vector<int> label(n);
        iota(label.begin(), label.end(), 0);
        if (shuffle) { std::shuffle(label.begin(), label.end(), rng); }
        vector<pair<int, int>> es;
        for (int b = 0, c = 0; b < n; ++c) {
            int e = c + 1 >= comps ? n : min(n, b + 1 + rnd(max(1, 2 * n / comps))), h = (e - b) / 2;
            for (int i = 1; i < e - b; ++i) {
                int p = shape == 0 ? rnd(i) : shape == 1 ? i - 1 : shape == 2 ? 0 : shape == 3 ? (i % 2 ? i - 1 : max(0, i - 2))
                    : shape == 4 ? (i - 1) / 2 : i < h ? i - 1 : rnd(max(1, h));
                es.push_back({label[b + p], label[b + i]});}
            b = e;}
        if (shuffle) { std::shuffle(es.begin(), es.end(), rng); }
        Graph g(n);
        for (auto [u, v] : es) {
            if (!shuffle || (rng() & 1)) { g.addEdge(u, v); } else { g.addEdge(v, u); }}
        return g;}
    string show(const Graph &g, int root) {
        std::ostringstream out; out << "n=" << g.n << " root=" << root << " edges=";
        if (g.n > 200) { out << "(n>200 omitted)"; return out.str(); }
        for (auto e : g.edges) { out << e.u << '-' << e.v << ' '; }
        return out.str();}

    // Oracle: rooting by DFS (a tree's parent pointers do not depend on the traversal), paths by explicit parent walks.
    struct Oracle {
        int n, root;
        vector<int> parent, pedge, comp, dep, size, child;
        vector<vector<pair<int, int>>> adj;
        vector<vector<int>> members;
        Oracle(const Graph &g, int root) : n(g.n), root(root), parent(n, -1), pedge(n, -1), comp(n, -1), dep(n, 0), size(n, 1),
            child(g.edges.size(), -1), adj(n), members(n) {
            for (int id = 0; id < int(g.edges.size()); ++id) {
                int u = g.edges[id].u, v = g.edges[id].v;
                adj[u].push_back({v, id}); adj[v].push_back({u, id});}
            vector<int> pre, st;
            auto grow = [&](int s) {
                comp[s] = s; st.push_back(s);
                while (!st.empty()) {
                    int u = st.back(); st.pop_back();
                    pre.push_back(u); members[s].push_back(u);
                    for (auto [v, id] : adj[u]) {
                        if (comp[v] == -1) { comp[v] = s; parent[v] = u; pedge[v] = id; dep[v] = dep[u] + 1; child[id] = v; st.push_back(v); }}}};
            if (root != -1) { grow(root); }
            for (int s = 0; s < n; ++s) {
                if (comp[s] == -1) { grow(s); }}
            for (int i = n - 1; i >= 0; --i) {
                if (parent[pre[i]] != -1) { size[parent[pre[i]]] += size[pre[i]]; }}}
        vector<int> path(int u, int v) const {
            if (comp[u] != comp[v]) { return {}; }
            vector<int> left, right;
            while (u != v) {
                if (dep[u] >= dep[v]) { left.push_back(u); u = parent[u]; }
                else { right.push_back(v); v = parent[v]; }}
            left.push_back(u);
            left.insert(left.end(), right.rbegin(), right.rend());
            return left;}
        int lca(int u, int v) const {
            if (comp[u] != comp[v]) { return -1; }
            while (u != v) {
                if (dep[u] >= dep[v]) { u = parent[u]; } else { v = parent[v]; }}
            return u;}
        int ancestor(int u, int k) const {
            if (k > dep[u]) { return -1; }
            while (k--) { u = parent[u]; }
            return u;}
        // Path u -> v as (value index, vertex reached): vertices, or edge ids with the endpoint each edge leads to.
        vector<pair<int, int>> steps(int u, int v, bool edges) const {
            auto p = path(u, v);
            vector<pair<int, int>> res;
            if (!edges) {
                for (int x : p) { res.push_back({x, x}); }
                return res;}
            for (int i = 1; i < int(p.size()); ++i) { res.push_back({parent[p[i - 1]] == p[i] ? pedge[p[i - 1]] : pedge[p[i]], p[i]}); }
            return res;}
        int pick(int u) const {
            if (rng() % 4 == 0) { return rnd(n); }
            const auto &c = members[comp[u]];
            return c[rnd(int(c.size()))];}
    };

    // Rooting fields equal the oracle; order is a preorder whose subtrees are [tin, tout); heavy child is first and largest; heads chain heavy edges.
    void verifyStructure(const HLD &h, const Oracle &o) {
        int n = o.n;
        check(h.n == n && h.parent == o.parent && h.parent_edge == o.pedge && h.component == o.comp && h.dep == o.dep && h.size == o.size, "rooting fields");
        check(h.edge_vertex == o.child && int(h.order.size()) == n && int(h.tin.size()) == n && int(h.tout.size()) == n && int(h.head.size()) == n, "edge_vertex and sizes");
        for (int id = 0; id < int(o.child.size()); ++id) { check(h.edgeVertex(id) == o.child[id], "edgeVertex"); }
        vector<char> seen(n);
        for (int i = 0; i < n; ++i) {
            int v = h.order[i];
            check(0 <= v && v < n && !seen[v] && h.tin[v] == i, "order[tin[v]] = v permutation");
            seen[v] = 1;}
        vector<int> big(n, 0);
        for (int v = 0; v < n; ++v) {
            if (o.parent[v] != -1) { big[o.parent[v]] = max(big[o.parent[v]], o.size[v]); }}
        vector<int> first(n, -1);
        for (int u = 0; u < n; ++u) {
            for (auto [v, id] : o.adj[u]) {
                if (first[u] == -1 && o.pedge[v] == id && o.parent[v] == u && o.size[v] == big[u]) { first[u] = v; }}}
        for (int v = 0; v < n; ++v) {
            int p = o.parent[v], hd = h.head[v];
            check(h.tout[v] == h.tin[v] + o.size[v], "tout = tin + size");
            check(0 <= hd && hd < n && h.tin[v] - h.tin[hd] == o.dep[v] - o.dep[hd], "heavy path contiguous");
            if (p == -1) {
                check(hd == v, "root heads its path");
                continue;}
            check(h.tin[p] < h.tin[v] && h.tout[v] <= h.tout[p], "subtree nesting");
            bool heavy = h.tin[v] == h.tin[p] + 1;
            check(hd == (heavy ? h.head[p] : v), "head chain");
            check(heavy == (v == first[p]), "heavy child is the first child of maximum size in adjacency order");}}

    void checkPath(const HLD &h, const Oracle &o, int u, int v, bool edges) {
        int n = o.n, calls = 0;
        bool sane = true;
        vector<int> got;
        bool ok = h.processPath(u, v, edges, [&](int l, int r, bool up) {
            ++calls;
            if (!(0 <= l && l < r && r <= n)) { sane = false; return; }
            if (up) {
                for (int i = r - 1; i >= l; --i) { got.push_back(h.order[i]); }}
            else {
                for (int i = l; i < r; ++i) { got.push_back(h.order[i]); }}});
        auto p = o.path(u, v);
        if (p.empty()) {
            check(!ok && calls == 0, "processPath across components");
            return;}
        if (edges) { p.erase(std::find(p.begin(), p.end(), o.lca(u, v))); }
        check(ok && sane && got == p, edges ? "processPath edges concatenation" : "processPath vertices concatenation");
        check(calls <= 2 * int(std::bit_width(uint(n))) + 2, "processPath O(log n) calls");}

    // Brute rerooting: BFS from r inside the component, v's rerooted subtree; edges keep x whose original parent is inside.
    void checkReroot(const HLD &h, const Oracle &o, int v, int r, bool edges) {
        int n = o.n;
        vector<int> par(n, -2), queue(1, r);
        vector<char> in(n, 0), want(n, 0);
        par[r] = -1;
        for (int i = 0; i < int(queue.size()); ++i) {
            int x = queue[i];
            in[x] = x == v || (par[x] >= 0 && in[par[x]]);
            for (auto a : o.adj[x]) {
                if (par[a.first] == -2) { par[a.first] = x; queue.push_back(a.first); }}}
        int total = 0;
        for (int x : queue) {
            if (in[x] && (!edges || (o.parent[x] != -1 && in[o.parent[x]]))) { want[h.tin[x]] = 1; ++total; }}
        int covered = 0;
        bool ok = true;
        for (auto [l, rr] : h.rerootedSubtreeIntervals(v, r, edges)) {
            ok = ok && 0 <= l && l < rr && rr <= n;
            if (!ok) { break; }
            covered += rr - l;
            for (int s = l; s < rr; ++s) { ok = ok && want[s]; }}
        check(ok && covered == total, edges ? "rerootedSubtreeIntervals edges" : "rerootedSubtreeIntervals vertices");}

    void structureQueries(const HLD &h, const Oracle &o, int q, int reroots, bool allPairs) {
        int n = o.n;
        if (!n) { return; }
        vector<lng> a(n);
        for (auto &x : a) { x = lng(rng()); }
        auto ra = h.reorderArray(a);
        for (int v = 0; v < n; ++v) { check(ra[h.tin[v]] == a[v], "reorderArray"); }
        auto one = [&](int u, int v, int t, bool reroot) {
            qu = u; qv = v;
            int c = o.lca(u, v), k = rnd(o.dep[u] + 2);
            bool e = t & 1;
            checkEq(h.getLCA(u, v), c, "getLCA");
            checkEq(h.distance(u, v), c == -1 ? -1 : o.dep[u] + o.dep[v] - 2 * o.dep[c], "distance");
            checkEq(h.kthAncestor(u, k), o.ancestor(u, k), "kthAncestor"); checkEq(h.kthAncestor(u, o.dep[u]), o.comp[u], "kthAncestor to the root");
            check(h.kthAncestor(u, o.dep[u] + 1) == -1 && h.kthAncestor(u, INT_MAX) == -1 && h.kthAncestor(u, INT64_MAX) == -1, "kthAncestor beyond the root");
            checkPath(h, o, u, v, false); checkPath(h, o, u, v, true);
            check(h.subtreeRange(v, e) == pair<int, int>{h.tin[v] + e, h.tout[v]}, "subtreeRange");
            if (c != -1 && reroot) { checkReroot(h, o, v, u, e); checkReroot(h, o, v, u, !e); }};
        if (allPairs) {
            for (int u = 0; u < n; ++u) {
                for (int v = 0; v < n; ++v) { one(u, v, u + v, true); }}}
        for (int t = 0; t < q; ++t) {
            int u = rnd(n);
            one(u, o.pick(u), t, t < reroots);}}

    template<class A>
    concept Acted = requires { typename A::F; };

    // TreeMonoid against naive values: ops random set/get/pathFold (both directions)/subtreeFold (tin order)/maxPath/pathApply/subtreeApply.
    template<bool C, class A, class G, class Gen, class GenF, class Eq, class Mk>
    void suite(const char *name, const G &g, const Oracle &o, bool edges, A mon, Gen gen, GenF genF, Eq eq, Mk mk, int ops) {
        using S = typename A::S;
        constexpr bool SEARCH = !std::is_same_v<Mk, std::nullptr_t>;
        int n = o.n, m = edges ? int(o.child.size()) : n;
        string base = context;
        context += string(" suite=") + name + " commutative=" + (C ? "1" : "0") + " edges=" + (edges ? "1" : "0");
        vector<S> val(m);
        for (auto &x : val) { x = gen(); }
        TreeMonoid<A, C> tm(g, val, edges, o.root, mon);
        ++cases;
        check(tm.parent == o.parent && int(tm.order.size()) == n, "TreeMonoid rooting");
        S e = mon.e();
        auto fold = [&](int u, int v) {
            S res = e;
            for (auto [i, x] : o.steps(u, v, edges)) { res = mon.op(res, val[i]); }
            return res;};
        auto slotValue = [&](int s) -> const S & { int x = tm.order[s]; return edges ? val[o.pedge[x]] : val[x]; };
        for (int t = 0; t < ops && n; ++t) {
            int u = rnd(n), v = o.pick(u), type = rnd(Acted<A> ? 9 : 7);
            bool same = o.comp[u] == o.comp[v];
            qu = u; qv = v;
            if (type == 0 && m) {
                int i = rnd(m);
                val[i] = gen(); tm.set(i, val[i]);}
            else if (type == 1 && m) {
                int i = rnd(m);
                check(eq(tm.get(i), val[i]), "get");}
            else if (type <= 3) {
                auto [ok, x] = tm.pathFold(u, v);
                auto [ok2, y] = tm.pathFold(v, u);
                check(ok == same && ok2 == same && eq(x, same ? fold(u, v) : e) && eq(y, same ? fold(v, u) : e), "pathFold both directions");}
            else if (type == 4) {
                auto [l, r] = tm.subtreeRange(v, edges);
                S res = e;
                for (int s = l; s < r; ++s) { res = mon.op(res, slotValue(s)); }
                check(l == tm.tin[v] + edges && r == tm.tin[v] + o.size[v], "subtree slots");
                check(eq(tm.subtreeFold(v), res), "subtreeFold");}
            else if (type <= 6) {
                if constexpr (SEARCH) {
                    auto pred = mk();
                    int res = same ? (edges ? u : -1) : -1;
                    bool failed = false, monotone = true;
                    S acc = e;
                    // Only paths whose prefix predicate is monotone are in maxPath's domain; signed sums often leave it.
                    for (auto [i, x] : o.steps(u, v, edges)) {
                        acc = mon.op(acc, val[i]);
                        if (!pred(acc)) { failed = true; }
                        else if (failed) { monotone = false; }
                        else { res = x; }}
                    if (monotone) { checkEq(tm.maxPath(u, v, pred), res, "maxPath"); }}
                else {
                    check(eq(tm.pathFold(u, v).second, same ? fold(u, v) : e), "pathFold");}}
            else if constexpr (Acted<A>) {
                auto f = genF();
                if (type == 7) {
                    check(tm.pathApply(u, v, f) == same, "pathApply status");
                    for (auto [i, x] : o.steps(u, v, edges)) { val[i] = mon.mapping(f, val[i]); }}
                else {
                    tm.subtreeApply(v, f);
                    for (int s = tm.tin[v] + edges; s < tm.tout[v]; ++s) {
                        int x = tm.order[s], i = edges ? o.pedge[x] : x;
                        val[i] = mon.mapping(f, val[i]);}}}}
        for (int i = 0; i < m; ++i) { check(eq(tm.get(i), val[i]), "final get"); }
        context = base;}

    // Legacy TreeEdgePainting (paint/unpaint an edge by id, count painted edges from u up to an ancestor) as edge-mode Sum set + pathFold.
    template<class G>
    void treeEdgePainting(const G &g, const Oracle &o, int ops) {
        int n = o.n, m = int(o.child.size());
        if (!m) { return; }
        TreeMonoid<Sum<int>, true> paint(g, vector<int>(m, 0), true, o.root);
        vector<int> painted(m, 0);
        for (int t = 0; t < ops; ++t) {
            if (rng() & 1) {
                int id = rnd(m);
                painted[id] = rnd(2); paint.set(id, painted[id]);
                continue;}
            int u = rnd(n), a = paint.kthAncestor(u, rnd(o.dep[u] + 1)), cnt = 0;
            qu = u; qv = a;
            for (int x = u; x != a; x = o.parent[x]) { cnt += painted[o.pedge[x]]; }
            check(paint.pathFold(u, a) == pair<bool, int>{true, cnt} && paint.pathFold(a, u).second == cnt, "treeEdgePainting count");}}

    using Aff = AffineComposite<mint>;
    using Rac = RangeAssignRangeComposite<mint>;
    using Gain = MaxGain<lng>;
    using Rars = RangeAddRangeSum<lng>;
    using Raff = RangeAffineRangeSum<mint>;
    mint rmint() { return mint(rng()); }
    auto eqAny = [](const auto &x, const auto &y) { return x == y; };
    auto eqRac = [](const Rac::S &x, const Rac::S &y) { return x.a == y.a && x.b == y.b && x.len == y.len; };
    auto eqSum = [](const auto &x, const auto &y) { return x.sum == y.sum && x.len == y.len; };
    auto eqGain = [](const Gain::S &x, const Gain::S &y) { return x.mn == y.mn && x.mx == y.mx && x.best == y.best; };

    // Every TreeMonoid configuration on one forest in one mode; value bound V keeps sums and prefix sums below 2^62.
    template<class G>
    void monoidSuites(const G &g, const Oracle &o, bool edges, lng V, int ops) {
        int n = max(1, o.n);
        lng A = min(V, lng(1000000000));
        suite<false>("AffineComposite<mint>", g, o, edges, Aff{}, [] { return Aff::S{rmint(), rmint()}; }, nullptr, eqAny, nullptr, ops);
        ulng mod = rng() & 1 ? 1000000007 : 998244353, b = 2 + rng() % 1000;
        auto hop = [mod](const pair<ulng, ulng> &x, const pair<ulng, ulng> &y) { return pair<ulng, ulng>{(x.first * y.second + y.first) % mod, x.second * y.second % mod}; };
        using Hash = Monoid<pair<ulng, ulng>, decltype(hop)>;
        suite<false>("stateful string-hash composite", g, o, edges, Hash{{0, 1}, hop}, [&] { return pair<ulng, ulng>{rng() % 26, b}; }, nullptr, eqAny, nullptr, ops);
        auto sumPred = [&] { lng k = rndRange(0, V * (n / 2 + 1)); return [k](lng s) { return s <= k; }; };
        auto nonneg = [&] { return rndRange(0, V); };
        suite<true>("Sum<lng>", g, o, edges, Sum<lng>{}, nonneg, nullptr, eqAny, sumPred, ops);
        suite<false>("Sum<lng>", g, o, edges, Sum<lng>{}, nonneg, nullptr, eqAny, sumPred, ops);
        auto signedValue = [&] { return rndRange(-V, V); };
        suite<true>("signed Sum<lng>", g, o, edges, Sum<lng>{}, signedValue, nullptr, eqAny, sumPred, ops);
        suite<false>("signed Sum<lng>", g, o, edges, Sum<lng>{}, signedValue, nullptr, eqAny, sumPred, ops);
        auto minPred = [&] { lng k = rndRange(-V, V); return [k](lng s) { return s >= k; }; };
        auto maxPred = [&] { lng k = rndRange(-V, V); return [k](lng s) { return s <= k; }; };
        suite<true>("Min<lng>", g, o, edges, Min<lng>{}, signedValue, nullptr, eqAny, minPred, ops);
        suite<false>("Min<lng>", g, o, edges, Min<lng>{}, signedValue, nullptr, eqAny, minPred, ops);
        suite<true>("Max<lng>", g, o, edges, Max<lng>{}, signedValue, nullptr, eqAny, maxPred, ops);
        suite<false>("Max<lng>", g, o, edges, Max<lng>{}, signedValue, nullptr, eqAny, maxPred, ops);
        suite<false>("MaxGain<lng>", g, o, edges, Gain{}, [&] { return Gain::leaf(rndRange(-A, A)); }, nullptr, eqGain,
            [&] { lng k = rndRange(0, 2 * A); return [k](const Gain::S &s) { return s.best <= k; }; }, ops);
        suite<false>("RangeAssignRangeComposite<mint>", g, o, edges, Rac{}, [] { return Rac::leaf(rmint(), rmint()); },
            [] { return rng() % 5 ? Rac::assign(rmint(), rmint()) : Rac::id(); }, eqRac, nullptr, ops);
        auto rarsPred = [&] { lng k = rndRange(0, 4 * A * (n / 2 + 1)); return [k](const Rars::S &s) { return s.sum <= k; }; };
        suite<true>("RangeAddRangeSum<lng>", g, o, edges, Rars{}, [&] { return Rars::leaf(rndRange(0, A)); }, [&] { return Rars::add(rndRange(0, A / 64)); }, eqSum, rarsPred, ops);
        suite<false>("RangeAddRangeSum<lng>", g, o, edges, Rars{}, [&] { return Rars::leaf(rndRange(0, A)); }, [&] { return Rars::add(rndRange(0, A / 64)); }, eqSum, rarsPred, ops);
        suite<true>("RangeAffineRangeSum<mint>", g, o, edges, Raff{}, [] { return Raff::leaf(rmint()); }, [] { return Raff::affine(rmint(), rmint()); }, eqSum, nullptr, ops);}

    // Reviewer reproducers: monotone along the path, not over the off-path slots a whole-array search would read.
    void maxPathRegressions() {
        Graph g(4);
        for (int v = 1; v < 4; ++v) { g.addEdge(v - 1, v); }
        auto pred = [](lng s) { return s <= 3; };
        context = "maxPath regressions on the path 0-1-2-3";
        TreeMonoid<Sum<lng>, true> a(g, {-100, 0, 0, 5});
        TreeMonoid<Sum<lng>> b(g, {-100, 0, 0, 5});
        checkEq(a.maxPath(3, 2, pred), -1, "maxPath upward, commutative");
        checkEq(b.maxPath(3, 2, pred), -1, "maxPath upward, reversible");
        TreeMonoid<Sum<lng>, true> c(g, {0, 5, -100, 0});
        checkEq(c.maxPath(0, 1, pred), 0, "maxPath downward");
        TreeMonoid<Sum<lng>, true> d(g, {-100, 0, 5}, true);
        checkEq(d.maxPath(3, 1, pred), 3, "maxPath edge mode");
        Graph h(2);
        TreeMonoid<Min<lng>> f(h, {5, 7});
        checkEq(f.maxPath(1, 1, [](lng s) { return s >= 4; }), 1, "maxPath isolated vertex");
        cases += 5;}

    template<class G>
    void forestCase(const Graph &raw, const G &g, int root, lng V, int ops, int q) {
        Oracle o(raw, root);
        HLD h(g, root);
        verifyStructure(h, o);
        structureQueries(h, o, q, q, raw.n <= 12);
        for (bool edges : {false, true}) { monoidSuites(g, o, edges, V, ops); }
        treeEdgePainting(g, o, ops);}

    void small() {
        maxPathRegressions();
        int rounds = mode == "quick" ? 150 : mode == "full" ? 1500 : CHECKED ? 3000 : 8000;
        for (int t = 0; t < rounds; ++t) {
            int n = t < 6 ? t : 1 + rnd(t % 10 == 0 ? 120 : 40), shape = rnd(6), comps = rng() % 3 ? 1 : 1 + rnd(6);
            int root = n && rng() % 2 ? rnd(n) : -1;
            lng V = t % 3 == 0 ? 3 : t % 3 == 1 ? 1000000000 : (lng(1) << 62) / (2 * (n + 1));
            Graph g = forest(n, shape, comps);
            ++cases; context = "seed=" + std::to_string(seed) + " round=" + std::to_string(t) + " " + show(g, root);
            if (t % 2) { forestCase(g, CsrGraph(g), root, V, 3 * n + 10, 2 * n + 5); }
            else { forestCase(g, g, root, V, 3 * n + 10, 2 * n + 5); }}
        cout << "PASS random forests n<=120 (6 shapes, 1-6 trees, isolated vertices, root=-1 and explicit, Graph and CsrGraph) rounds=" << rounds << '\n';}

    void large() {
#ifdef __SANITIZE_ADDRESS__
        int cap = 200000;
#else
        int cap = INT_MAX;
#endif
        int n = min(cap, mode == "quick" ? 20000 : CHECKED ? 50000 : mode == "full" ? 200000 : 1000000);
        struct Shape { const char *name; int shape, comps; bool shuffle; int root; };
        for (auto [name, shape, comps, shuffle, root] : vector<Shape>{{"deep path rooted at an end", 1, 1, false, n - 1}, {"shuffled path", 1, 1, true, -1},
                {"star", 2, 1, true, rnd(n)}, {"caterpillar", 3, 1, true, -1}, {"binary", 4, 1, true, -1}, {"random", 0, 1, true, rnd(n)},
                {"broom forest", 5, 40, true, -1}}) {
            Graph g = forest(n, shape, comps, shuffle);
            context = "seed=" + std::to_string(seed) + " large " + name + " n=" + std::to_string(n) + " root=" + std::to_string(root);
            ++cases;
            Oracle o(g, root);
            CsrGraph csr(g);
            HLD h(csr, root);
            verifyStructure(h, o);
            structureQueries(h, o, 20, 3, false);
            auto sumPred = [] { lng k = rndRange(0, lng(1) << 40); return [k](lng s) { return s <= k; }; };
            suite<false>("Sum<lng>", g, o, false, Sum<lng>{}, [] { return rndRange(0, 1000000000); }, nullptr, eqAny, sumPred, 20);
            suite<true>("Max<lng>", csr, o, true, Max<lng>{}, [] { return rndRange(-1000, 1000); }, nullptr, eqAny,
                [] { lng k = rndRange(-1000, 1000); return [k](lng s) { return s <= k; }; }, 20);
            suite<false>("AffineComposite<mint>", g, o, true, Aff{}, [] { return Aff::S{rmint(), rmint()}; }, nullptr, eqAny, nullptr, 20);
            suite<false>("RangeAssignRangeComposite<mint>", csr, o, false, Rac{}, [] { return Rac::leaf(rmint(), rmint()); },
                [] { return Rac::assign(rmint(), rmint()); }, eqRac, nullptr, 20);
            treeEdgePainting(g, o, 30);}
        // 10^6 isolated vertices: each component's BFS starts at its own root, or the build rescans the earlier ones in O(n^2).
        context = "seed=" + std::to_string(seed) + " large isolated vertices n=1000000";
        ++cases;
        Graph lone(1000000);
        HLD h(lone);
        bool ok = true;
        for (int v = 0; v < lone.n; ++v) { ok &= h.component[v] == v && h.head[v] == v && h.tin[v] == v && h.tout[v] == v + 1 && h.parent[v] == -1; }
        check(ok, "isolated vertices: own component, head and slot");
        cout << "PASS large deep/shuffled path, star, caterpillar, binary, random, broom forest n=" << n << ", isolated vertices n=1000000\n";}

    void invalid(const string &name) {
        Graph g(3); g.addEdge(0, 1); g.addEdge(1, 2);
        Graph h(4); h.addEdge(0, 1); h.addEdge(2, 3);
        if (name == "directed") { (void)HLD(Graph(2, true)); }
        else if (name == "self-loop") { Graph s(2); s.addEdge(0, 1); s.addEdge(1, 1); (void)HLD(s); }
        else if (name == "cycle") { Graph c = g; c.addEdge(2, 0); (void)HLD(c); }
        else if (name == "multi-edge") { Graph c = g; c.addEdge(1, 0); (void)HLD(CsrGraph(c)); }
        else if (name == "root-range") { (void)HLD(g, 3); }
        else if (name == "vertex-range") { (void)HLD(g).getLCA(0, 3); }
        else if (name == "kth-negative") { (void)HLD(g).kthAncestor(2, -1); }
        else if (name == "edge-range") { (void)HLD(g).edgeVertex(2); }
        else if (name == "path-range") { (void)TreeMonoid<Sum<lng>>(g, vector<lng>(3)).pathFold(-1, 0); }
        else if (name == "reroot-components") { (void)HLD(h).rerootedSubtreeIntervals(0, 2); }
        else if (name == "root-far") { (void)HLD(g, 4); }
        else if (name == "vertex-far") { (void)HLD(g).getLCA(0, 4); }
        else if (name == "vertex-first") { (void)HLD(g).getLCA(3, 0); }
        else if (name == "edge-far") { (void)HLD(g).edgeVertex(3); }
        else if (name == "path-second") { (void)TreeMonoid<Sum<lng>>(g, vector<lng>(3)).pathFold(0, 3); }
        else if (name == "subtree-range") { (void)HLD(g).subtreeRange(3); }
        else if (name == "reroot-vertex") { (void)HLD(g).rerootedSubtreeIntervals(3, 0); }
        else if (name == "reroot-target") { (void)HLD(g).rerootedSubtreeIntervals(0, 3); }
        else if (name == "reorder-short") { (void)HLD(g).reorderArray(vector<int>(2)); }
        else if (name == "reorder-long") { (void)HLD(g).reorderArray(vector<int>(4)); }
        else if (name == "values-short") { (void)TreeMonoid<Sum<lng>>(g, vector<lng>(2)); }
        else if (name == "values-size") { (void)TreeMonoid<Sum<lng>>(g, vector<lng>(3), true); }
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
        small(); large();
        cout << "PASS hld seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL hld seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
