#include "../../04-Graphs/14-tree_algorithms.hpp"

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
#ifdef __SANITIZE_ADDRESS__
    constexpr bool ASAN = true;
#else
    constexpr bool ASAN = false;
#endif
    constexpr lng P = 998244353, NEG = LLONG_MIN / 4;

    string text(lll x) {
        if (x == 0) { return "0"; }
        string res;
        ulll y = x < 0 ? ulll(0) - ulll(x) : ulll(x);
        for (; y; y /= 10) { res += char('0' + int(y % 10)); }
        if (x < 0) { res += '-'; }
        reverse(res.begin(), res.end());
        return res;}
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    void checkEq(lll actual, lll expected, const string &operation) {
        ++checks;
        if (actual != expected) { throw std::runtime_error(context + " operation=" + operation + " expected=" + text(expected) + " actual=" + text(actual)); }}
    template<class T>
    void checkVec(const vector<T> &actual, const vector<T> &expected, const string &operation) {
        ++checks;
        if (actual == expected) { return; }
        size_t i = 0;
        while (i < actual.size() && i < expected.size() && actual[i] == expected[i]) { ++i; }
        string detail = " size expected=" + std::to_string(expected.size()) + " actual=" + std::to_string(actual.size()) + " first difference at " + std::to_string(i);
        if constexpr (std::is_integral_v<T>) {
            if (i < actual.size() && i < expected.size()) { detail += " expected=" + text(lll(expected[i])) + " actual=" + text(lll(actual[i])); }}
        throw std::runtime_error(context + " operation=" + operation + detail);}
    int rnd(int n) { return int(rng() % ulng(n)); }
    lng rndRange(lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    ulng mix(ulng x) {
        x += 0x9e3779b97f4a7c15ULL; x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL; x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);}

    // Weight classes: 0 unit, 1 small signed, 2 nonnegative, 3 nonpositive, 4 full lng range, 5 extremes and +-2^62.
    lng weight(int kind) {
        if (kind == 0) { return 1; }
        if (kind == 1) { return rndRange(-5, 5); }
        if (kind == 2) { return rndRange(0, 1000000000); }
        if (kind == 3) { return rndRange(-1000, 0); }
        if (kind == 4) { return lng(rng()); }
        static const lng EXTREME[] = {LLONG_MIN, LLONG_MAX, lng(1) << 62, -(lng(1) << 62), (lng(1) << 62) - 1, 0, 1, -1};
        return EXTREME[rnd(8)];}

    // Forest of comps trees in a shape (random, path, star, caterpillar, binary, path with random tail), labels, edges and orientations shuffled.
    Graph forest(int n, int shape, int comps, int kind) {
        vector<int> label(n);
        iota(label.begin(), label.end(), 0);
        std::shuffle(label.begin(), label.end(), rng);
        vector<GraphEdge> es;
        for (int b = 0, c = 0; b < n; ++c) {
            int e = c + 1 >= comps ? n : min(n, b + max(1, n / comps)), h = (e - b) / 2;
            for (int i = 1; i < e - b; ++i) {
                int p = shape == 0 ? rnd(i) : shape == 1 ? i - 1 : shape == 2 ? 0 : shape == 3 ? (i % 2 ? i - 1 : max(0, i - 2))
                    : shape == 4 ? (i - 1) / 2 : i < h ? i - 1 : rnd(max(1, h));
                es.push_back({label[b + p], label[b + i], weight(kind)});}
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
    vector<tuple<int, int, lng>> edgeList(const vector<GraphEdge> &es) {
        vector<tuple<int, int, lng>> res;
        for (auto [u, v, w] : es) { res.push_back({u, v, w}); }
        return res;}
    vector<pair<int, int>> edgeSet(const Graph &g) {
        vector<pair<int, int>> res;
        for (auto e : g.edges) { res.push_back({min(e.u, e.v), max(e.u, e.v)}); }
        sort(res.begin(), res.end());
        return res;}

    // Oracle: neighbour lists rebuilt from the edge list (Graph's adjacency order), DFS distances on demand.
    struct Nb { int to, id; lng w; };
    struct Brute {
        int n;
        vector<vector<Nb>> adj;
        explicit Brute(const Graph &g) : n(g.n), adj(n) {
            for (int i = 0; i < int(g.edges.size()); ++i) {
                auto [u, v, w] = g.edges[i];
                adj[u].push_back({v, i, w}); adj[v].push_back({u, i, w});}}
        // Exact weighted distances and hop counts from s; hops -1 outside s's tree.
        pair<vector<lll>, vector<int>> from(int s) const {
            vector<lll> d(n); vector<int> h(n, -1), st{s};
            h[s] = 0;
            while (!st.empty()) {
                int u = st.back(); st.pop_back();
                for (auto e : adj[u]) {
                    if (h[e.to] == -1) { h[e.to] = h[u] + 1; d[e.to] = d[u] + e.w; st.push_back(e.to); }}}
            return {d, h};}
    };

    // Oracle rooting: preorder in adjacency order with the documented roots, half-open tout, BFS order over the roots by preorder.
    struct Rooted { vector<int> tin, tout, order, parent, dep, comp, bfs; };
    Rooted rootedOracle(const Brute &b, int root) {
        int n = b.n;
        Rooted r{vector<int>(n, -1), vector<int>(n), {}, vector<int>(n, -1), vector<int>(n), vector<int>(n), {}};
        vector<pair<int, int>> st;
        auto visit = [&](int u, int p, int c) {
            r.tin[u] = int(r.order.size()); r.order.push_back(u); r.parent[u] = p; r.comp[u] = c;
            r.dep[u] = p == -1 ? 0 : r.dep[p] + 1; st.push_back({u, 0});};
        auto run = [&](int s) {
            visit(s, -1, s);
            while (!st.empty()) {
                auto [u, i] = st.back();
                if (i == int(b.adj[u].size())) { r.tout[u] = int(r.order.size()); st.pop_back(); continue; }
                ++st.back().se;
                if (int v = b.adj[u][i].to; v != r.parent[u]) { visit(v, u, r.comp[u]); }}};
        if (root != -1) { run(root); }
        for (int u = 0; u < n; ++u) {
            if (r.tin[u] == -1) { run(u); }}
        for (int v : r.order) {
            if (r.parent[v] == -1) { r.bfs.push_back(v); }}
        for (int i = 0; i < int(r.bfs.size()); ++i) {
            int u = r.bfs[i];
            for (auto e : b.adj[u]) {
                if (e.to != r.parent[u]) { r.bfs.push_back(e.to); }}}
        return r;}

    // Oracle DP: BFS rooting at root, children folded in adjacency order; works for any root of a tree.
    template<class R>
    vector<typename R::S> naiveDp(const Brute &b, int root, const R &m) {
        vector<int> order{root}, up(b.n, -1);
        vector<char> seen(b.n);
        seen[root] = 1;
        for (int i = 0; i < int(order.size()); ++i) {
            for (auto e : b.adj[order[i]]) {
                if (!seen[e.to]) { seen[e.to] = 1; up[e.to] = e.id; order.push_back(e.to); }}}
        vector<typename R::S> res(b.n, m.e());
        for (int i = int(order.size()) - 1; i >= 0; --i) {
            int u = order[i];
            auto x = m.e();
            for (auto e : b.adj[u]) {
                if (up[e.to] == e.id) { x = m.op(x, m.addEdge(res[e.to], GraphArc{u, e.to, e.id, -1, e.w})); }}
            res[u] = m.addVertex(x, u);}
        return res;}

    // Unit height: get(v) is v's hop eccentricity.
    struct Height {
        using S = int;
        S op(S a, S b) const { return max(a, b); }
        S e() const { return 0; }
        S addEdge(S x, const GraphArc &) const { return x + 1; }
        S addVertex(S x, int) const { return x; }
    };
    // Exact canonical text: a sorted multiset of sibling strings (commutative op), arcs spelled id:from>to and w.
    struct Text {
        using S = vector<string>;
        static string join(const S &x) {
            string res;
            for (const auto &s : x) { res += s; res += ','; }
            return res;}
        S op(const S &a, const S &b) const {
            S res;
            std::ranges::merge(a, b, std::back_inserter(res));
            return res;}
        S e() const { return {}; }
        S addEdge(const S &x, const GraphArc &e) const {
            return {"<" + std::to_string(e.id) + ":" + std::to_string(e.from) + ">" + std::to_string(e.to) + "w" + std::to_string(e.w) + "(" + join(x) + ")"};}
        S addVertex(const S &x, int v) const { return {"v" + std::to_string(v) + "[" + join(x) + "]"}; }
    };
    // Exact ordered text (noncommutative concatenation) for subtreeDp.
    struct Chain {
        using S = string;
        S op(const S &a, const S &b) const { return a + b; }
        S e() const { return ""; }
        S addEdge(const S &x, const GraphArc &e) const {
            return "(" + std::to_string(e.id) + ":" + std::to_string(e.from) + ">" + std::to_string(e.to) + "w" + std::to_string(e.w) + x + ")";}
        S addVertex(const S &x, int v) const { return "v" + std::to_string(v) + "{" + x + "}"; }
    };
    // Polynomial hash of the ordered text (h, B^length), noncommutative, for large trees.
    struct Hash {
        using S = pair<ulng, ulng>;
        static constexpr ulng B = 0x9e3779b97f4a7c15ULL;
        static S token(ulng t) { return {t, B}; }
        S op(S a, S b) const { return {a.fi * b.se + b.fi, a.se * b.se}; }
        S e() const { return {0, 1}; }
        S addEdge(S x, const GraphArc &e) const { return op(op(token(mix(ulng(e.id) << 40 ^ ulng(e.from) << 20 ^ ulng(e.to)) ^ ulng(e.w)), x), token(1)); }
        S addVertex(S x, int v) const { return op(op(token(mix(ulng(v)) + 2), x), token(3)); }
    };
    // Labelled weighted sums: cnt = sum of labels c, sum = sum of c(u) * dist(root, u), h = an orientation-sensitive hash.
    struct Weighted {
        struct S {
            lng cnt; lll sum; ulng h;
            bool operator==(const S &) const = default;
        };
        const vector<lng> *c;
        S op(S a, S b) const { return {a.cnt + b.cnt, a.sum + b.sum, a.h + b.h}; }
        S e() const { return {0, 0, 0}; }
        S addEdge(S x, const GraphArc &e) const {
            return {x.cnt, x.sum + lll(x.cnt) * e.w, mix(x.h ^ mix(ulng(e.id) * 3 + ulng(e.from) * 1000003 + ulng(e.to) * 998244353))};}
        S addVertex(S x, int v) const { return {x.cnt + (*c)[v], x.sum, mix(x.h + ulng((*c)[v]) * 7 + ulng(v))}; }
    };

    // Max-plus: res[j] is the best sum of exactly j chosen vertices.
    struct MaxPlus {
        using S = lng;
        const vector<lng> *a;
        S add(S x, S y) const { return max(x, y); }
        S zero() const { return NEG; }
        S mul(S x, S y) const { return x == NEG || y == NEG ? NEG : x + y; }
        S one() const { return 0; }
        void closeVertex(int v, vector<S> &dp) const {
            dp.push_back(NEG);
            for (int j = int(dp.size()) - 1; j > 0; --j) { dp[j] = max(dp[j], mul(dp[j - 1], (*a)[v])); }}
    };
    // Counting mod P: res[j] for j >= 1 counts connected vertex sets of size j containing the vertex; res[0] = 1 means unchosen.
    struct Connected {
        using S = lng;
        S add(S x, S y) const { return (x + y) % P; }
        S zero() const { return 0; }
        S mul(S x, S y) const { return x * y % P; }
        S one() const { return 1; }
        void closeVertex(int, vector<S> &dp) const { dp.insert(dp.begin(), 1); }
    };
    // Counting mod P: every vertex chosen or not, so the root vector is C(n, j).
    struct Binomial : Connected {
        void closeVertex(int, vector<S> &dp) const {
            dp.push_back(0);
            for (int j = int(dp.size()) - 1; j > 0; --j) { dp[j] = (dp[j] + dp[j - 1]) % P; }}
    };
    // Binomial counts, except that marked vertices return an empty vector, which empties every ancestor's product.
    struct Clearing : Connected {
        const vector<char> *kill;
        void closeVertex(int v, vector<S> &dp) const {
            if ((*kill)[v] || dp.empty()) { dp.clear(); return; }
            dp.push_back(0);
            for (int j = int(dp.size()) - 1; j > 0; --j) { dp[j] = (dp[j] + dp[j - 1]) % P; }}
    };
    vector<lng> binomRow(int n, int m) {
        vector<lng> row{1};
        for (int i = 1; i <= n; ++i) {
            if (int(row.size()) <= m) { row.push_back(0); }
            for (int j = int(row.size()) - 1; j > 0; --j) { row[j] = (row[j] + row[j - 1]) % P; }}
        return row;}

    void checkPath(const Graph &g, const TreeDiameter &res) {
        bool ok = res.vertices.size() == res.arcs.size() + 1;
        lll length = 0;
        vector<char> seen(g.n);
        for (int i = 0; ok && i < int(res.vertices.size()); ++i) {
            int v = res.vertices[i];
            ok = 0 <= v && v < g.n && !seen[v];
            if (!ok) { break; }
            seen[v] = 1;
            if (i + 1 < int(res.vertices.size())) {
                int a = res.arcs[i];
                ok = 0 <= a && a < int(g.arcs.size()) && g.arcs[a].from == v && g.arcs[a].to == res.vertices[i + 1];
                if (ok) {
                    auto e = g.edges[g.arcs[a].id];
                    ok = std::minmax(e.u, e.v) == std::minmax(v, res.vertices[i + 1]) && e.w == g.arcs[a].w; length += e.w;}}}
        check(ok, "treeDiameter witness is a simple path along arcs[i]: vertices[i] -> vertices[i + 1]");
        checkEq(length, res.length, "treeDiameter witness weight");
        check(res.length != 0 || res.arcs.empty(), "treeDiameter length 0 is a single vertex");}

    // Diameter, eccentricities, centroid and distance sums against all-pairs DFS distances.
    void testTree(const Graph &g, const Brute &b) {
        int n = g.n;
        lll best = 0;
        vector<lll> ecc(n), sum(n);
        for (int v = 0; v < n; ++v) {
            for (lll x : b.from(v).fi) { best = max(best, x); ecc[v] = max(ecc[v], x); sum[v] += x; }}
        lll radius = *std::ranges::min_element(ecc);
        vector<int> center, centroid;
        for (int v = 0; v < n; ++v) {
            if (ecc[v] == radius) { center.push_back(v); }}
        for (int v = 0; v < n; ++v) {
            int worst = 0;
            vector<char> seen(n);
            seen[v] = 1;
            for (auto e : b.adj[v]) {
                vector<int> st{e.to};
                int size = 0;
                seen[e.to] = 1;
                while (!st.empty()) {
                    int u = st.back(); st.pop_back(); ++size;
                    for (auto f : b.adj[u]) {
                        if (!seen[f.to]) { seen[f.to] = 1; st.push_back(f.to); }}}
                worst = max(worst, size);}
            if (2 * worst <= n) { centroid.push_back(v); }}
        CsrGraph c(g);
        for (const auto &res : {treeDiameter(g), treeDiameter(c)}) {
            checkEq(res.length, best, "treeDiameter length vs all-pairs max"); checkPath(g, res);}
        for (const auto &res : {treeEccentricities(g), treeEccentricities(c)}) {
            checkVec(res.ecc, ecc, "treeEccentricities ecc"); checkEq(res.radius, radius, "treeEccentricities radius");
            checkVec(res.center, center, "treeEccentricities center");}
        checkVec(treeCentroid(g), centroid, "treeCentroid vs removal BFS"); checkVec(treeCentroid(c), centroid, "treeCentroid CsrGraph");
        checkVec(sumOfDistances(g), sum, "sumOfDistances"); checkVec(sumOfDistances(c), sum, "sumOfDistances CsrGraph");}

    // EulerTour, BfsNumbering (levelRange, diskRanges) and VirtualTree against the oracle rooting; exhaustive queries when small.
    void testForest(const Graph &g, const Brute &b, int root, bool small) {
        int n = g.n;
        auto o = rootedOracle(b, root);
        vector<int> size(n, 1);
        for (int i = n - 1; i >= 0; --i) {
            if (int v = o.order[i]; o.parent[v] != -1) { size[o.parent[v]] += size[v]; }}
        auto inside = [&](int a, int u) { return o.tin[a] <= o.tin[u] && o.tin[u] < o.tout[a]; };
        for (const EulerTour &et : {EulerTour(g, root), EulerTour(CsrGraph(g), root)}) {
            checkVec(et.order, o.order, "EulerTour order"); checkVec(et.tin, o.tin, "EulerTour tin"); checkVec(et.tout, o.tout, "EulerTour tout");
            checkVec(et.parent, o.parent, "EulerTour parent"); checkVec(et.dep, o.dep, "EulerTour dep");
            for (int v = 0; v < n; ++v) { check(et.subtreeRange(v) == pair(et.tin[v], et.tout[v]) && et.tout[v] - et.tin[v] == size[v], "EulerTour subtreeRange size"); }}
        if (small) {
            // Parent walks: every subtree range holds exactly the vertices with v on their root path.
            for (int u = 0; u < n; ++u) {
                for (int a = u; a != -1; a = o.parent[a]) { check(inside(a, u), "EulerTour subtree contains every descendant"); }}}
        if (root == -1) { check(EulerTour(g).order == o.order && BfsNumbering(g).order == o.bfs && VirtualTree(g).lca.root == (n ? 0 : -1), "default root -1"); }
        BfsNumbering bn(g, root);
        checkVec(bn.order, o.bfs, "BfsNumbering order"); checkVec(bn.parent, o.parent, "BfsNumbering parent"); checkVec(bn.dep, o.dep, "BfsNumbering dep");
        checkVec(bn.tin, o.tin, "BfsNumbering tin"); checkVec(bn.tout, o.tout, "BfsNumbering tout (exclusive)");
        check(int(bn.id.size()) == n, "BfsNumbering id size");
        for (int i = 0; i < n; ++i) { check(bn.id[bn.order[i]] == i, "BfsNumbering id[order[i]] = i"); }
        check(BfsNumbering(CsrGraph(g), root).order == o.bfs, "BfsNumbering CsrGraph");
        int deepest = n ? std::ranges::max(o.dep) : 0;
        auto members = [&](pair<int, int> r) {
            check(0 <= r.fi && r.fi <= r.se && r.se <= n, "range inside [0, n]");
            vector<int> res(bn.order.begin() + r.fi, bn.order.begin() + r.se);
            sort(res.begin(), res.end());
            return res;};
        auto level = [&](int v, int d) {
            vector<int> want;
            for (int u = 0; u < n; ++u) {
                if (inside(v, u) && o.dep[u] == d) { want.push_back(u); }}
            auto got = bn.levelRange(v, d);
            if (want.empty()) { check(got == pair(0, 0), "levelRange empty is {0, 0}"); }
            else { checkVec(members(got), want, "levelRange is v's descendants at depth d"); }};
        auto disk = [&](int v, int r, const vector<int> &h) {
            vector<int> want, got;
            for (int u = 0; u < n; ++u) {
                if (h[u] != -1 && h[u] <= r) { want.push_back(u); }}
            int last = -1;
            for (auto range : bn.diskRanges(v, r)) {
                auto part = members(range);
                check(!part.empty(), "diskRanges ranges nonempty");
                int d = o.dep[part[0]];
                for (int u : part) { check(o.dep[u] == d, "diskRanges one depth per range"); }
                check(d > last, "diskRanges increasing depth (disjoint)");
                last = d; got.insert(got.end(), part.begin(), part.end());}
            sort(got.begin(), got.end());
            checkVec(got, want, "diskRanges union is the r-ball by BFS");};
        if (small) {
            for (int v = 0; v < n; ++v) {
                auto h = b.from(v).se;
                for (int d = -1; d <= deepest + 1; ++d) { level(v, d); }
                for (int r = 0; r <= 2 * deepest + 1; ++r) { disk(v, r, h); }
                level(v, INT_MAX - 1); level(v, INT_MAX); disk(v, INT_MAX, h);}}
        else {
            for (int t = 0; t < 40 && n; ++t) { level(rnd(n), rnd(4) ? o.dep[rnd(n)] : rnd(deepest + 3) - 1); }
            for (int t = 0; t < 12 && n; ++t) {
                int v = rnd(n);
                disk(v, t % 4 == 0 ? INT_MAX : rnd(t % 2 ? 6 : 2 * deepest + 2), b.from(v).se);}}
        auto lcaNaive = [&](int u, int v) {
            while (o.dep[u] > o.dep[v]) { u = o.parent[u]; }
            while (o.dep[v] > o.dep[u]) { v = o.parent[v]; }
            while (u != v) { u = o.parent[u]; v = o.parent[v]; }
            return u;};
        VirtualTree vt(g, root), vc(CsrGraph(g), root);
        for (int t = 0; t < (small ? 8 : 5); ++t) {
            vector<int> vs;
            int k = !n || t == 0 ? 0 : t == 1 && small ? n : 1 + rnd(min(n, small ? 12 : 10));
            for (int i = 0; i < k; ++i) { vs.push_back(t == 1 && small ? i : rnd(n)); }
            if (t == 2 && k) { vs.push_back(vs[0]); }
            std::shuffle(vs.begin(), vs.end(), rng);
            vector<char> in(n);
            for (int i = 0; i < int(vs.size()); ++i) {
                in[vs[i]] = 1;
                for (int j = 0; j < i; ++j) {
                    if (o.comp[vs[i]] == o.comp[vs[j]]) { in[lcaNaive(vs[i], vs[j])] = 1; }}}
            vector<int> want, pos(n, -1), parent;
            for (int v : o.order) {
                if (in[v]) { pos[v] = int(want.size()); want.push_back(v); }}
            for (int v : want) {
                int p = o.parent[v];
                while (p != -1 && !in[p]) { p = o.parent[p]; }
                parent.push_back(p == -1 ? -1 : pos[p]);}
            for (const VirtualTree *x : {&vt, &vc}) {
                auto res = x->build(vs);
                checkVec(res.vertices, want, "VirtualTree vertices: LCA closure in preorder"); checkVec(res.parent, parent, "VirtualTree parent: nearest proper ancestor in the set");}}}

    // Rerooting (get(v), get(v, p)) and subtreeDp against the oracle DP from every root.
    void testDp(const Graph &g, const Brute &b) {
        int n = g.n;
        bool small = n <= 60;
        vector<lng> c(n);
        for (auto &x : c) { x = rndRange(0, 100); }
        Weighted wm{&c};
        Rerooting<Height> rh(g);
        Rerooting<Weighted> rw(CsrGraph(g), wm);
        std::optional<Rerooting<Text>> rt;
        if (small) { rt.emplace(g); }
        for (int v = 0; v < n; ++v) {
            auto nh = naiveDp(b, v, Height{});
            auto nw = naiveDp(b, v, wm);
            check(rh.get(v) == nh[v] && rh.get(v) == std::ranges::max(b.from(v).se), "Rerooting get height (unit eccentricity)");
            check(rw.get(v) == nw[v], "Rerooting get weighted labels");
            vector<Text::S> nt;
            if (small) { nt = naiveDp(b, v, Text{}); check(rt->get(v) == nt[v], "Rerooting get exact text"); }
            for (auto e : b.adj[v]) {
                check(rh.get(e.to, v) == nh[e.to] && rw.get(e.to, v) == nw[e.to], "Rerooting get(v, p) height and weighted");
                if (small) { check(rt->get(e.to, v) == nt[e.to], "Rerooting get(v, p) exact text"); }}}
        int root = rnd(n);
        check(subtreeDp(g, root, Hash{}) == naiveDp(b, root, Hash{}), "subtreeDp ordered hash");
        check(subtreeDp(CsrGraph(g), root, wm) == naiveDp(b, root, wm), "subtreeDp weighted labels");
        if (small) { check(subtreeDp(g, root, Chain{}) == naiveDp(b, root, Chain{}), "subtreeDp exact ordered text"); }}

    // subtreeMergeDp: max-plus and connected counts against subset enumeration (n <= 12), binomials and size <= 3 counts otherwise.
    // Empty closeVertex output stays empty upward and allocates nothing, even at k = INT_MAX (audit regression).
    void testEmptyMerge(const Graph &g) {
        int n = g.n, root = rnd(n);
        vector<char> kill(n);
        bool any = false;
        for (auto &x : kill) { x = rnd(3) == 0; any |= x; }
        for (int k : {0, 1000, INT_MAX}) {
            auto res = subtreeMergeDp(g, root, k, Clearing{{}, &kill});
            vector<lng> want = any ? vector<lng>{} : binomRow(n, min(n, k));
            check(res == want, "subtreeMergeDp empty hook output");}}

    void testMerge(const Graph &g, const Brute &b, const vector<int> &ks) {
        int n = g.n, root = rnd(n), deg = int(b.adj[root].size());
        vector<lng> a(n);
        for (auto &x : a) { x = rndRange(-1000000000, 1000000000); }
        vector<lng> best(n + 1, NEG), conn(n + 1, 0);
        if (n <= 12) {
            for (int mask = 0; mask < 1 << n; ++mask) {
                lng s = 0;
                for (int v = 0; v < n; ++v) { s += mask >> v & 1 ? a[v] : 0; }
                int k = std::popcount(uint(mask));
                best[k] = max(best[k], s);
                if (!(mask >> root & 1)) { continue; }
                vector<int> st{root};
                int seen = 1 << root, reach = 1;
                while (!st.empty()) {
                    int u = st.back(); st.pop_back();
                    for (auto e : b.adj[u]) {
                        if ((mask >> e.to & 1) && !(seen >> e.to & 1)) { seen |= 1 << e.to; ++reach; st.push_back(e.to); }}}
                conn[k] += reach == k;}}
        else {
            vector<lng> sorted = a;
            sort(sorted.rbegin(), sorted.rend());
            best[0] = 0;
            for (int j = 0; j < n; ++j) { best[j + 1] = best[j] + sorted[j]; }
            conn[1] = 1; conn[2] = deg; conn[3] = lng(deg) * (deg - 1) / 2;
            for (auto e : b.adj[root]) { conn[3] += lng(b.adj[e.to].size()) - 1; }}
        conn[0] = 1;
        for (int k : ks) {
            int m = min(n, k);
            vector<lng> wb(best.begin(), best.begin() + m + 1), wc(conn.begin(), conn.begin() + m + 1);
            checkVec(subtreeMergeDp(g, root, k, Binomial{}), binomRow(n, m), "subtreeMergeDp binomial counts C(n, j)");
            checkVec(subtreeMergeDp(CsrGraph(g), root, k, MaxPlus{&a}), wb, "subtreeMergeDp max-plus best j vertices");
            if (n <= 12 || k <= 3) { checkVec(subtreeMergeDp(g, root, k, Connected{}), wc, "subtreeMergeDp connected sets containing the root"); }}}

    vector<int> naiveEncode(const Brute &b) {
        int n = b.n;
        vector<int> deg(n), res;
        vector<char> gone(n);
        for (int v = 0; v < n; ++v) { deg[v] = int(b.adj[v].size()); }
        for (int i = 0; i < n - 2; ++i) {
            int leaf = 0;
            while (gone[leaf] || deg[leaf] != 1) { ++leaf; }
            gone[leaf] = 1;
            for (auto e : b.adj[leaf]) {
                if (!gone[e.to]) { res.push_back(e.to); --deg[e.to]; }}}
        return res;}
    vector<GraphEdge> naiveDecode(const vector<int> &code) {
        int n = int(code.size()) + 2;
        vector<int> deg(n, 1);
        vector<GraphEdge> res;
        for (int x : code) { ++deg[x]; }
        for (int x : code) {
            int leaf = 0;
            while (deg[leaf] != 1) { ++leaf; }
            res.push_back({leaf, x, 1}); --deg[leaf]; --deg[x];}
        int u = 0;
        while (deg[u] != 1) { ++u; }
        int v = u + 1;
        while (deg[v] != 1) { ++v; }
        res.push_back({u, v, 1});
        return res;}
    void checkDecode(const vector<int> &code) {
        Graph t = pruferDecode(code);
        check(t.n == int(code.size()) + 2 && !t.directed, "pruferDecode size and undirected");
        check(edgeList(t.edges) == edgeList(naiveDecode(code)), "pruferDecode edges in removal order vs naive");
        checkVec(pruferEncode(t), code, "pruferEncode(pruferDecode(code)) == code");}
    void testPrufer(const Graph &g, const Brute &b) {
        auto code = pruferEncode(g);
        checkVec(code, naiveEncode(b), "pruferEncode vs naive smallest-leaf removal");
        checkVec(pruferEncode(CsrGraph(g)), code, "pruferEncode CsrGraph");
        check(edgeSet(pruferDecode(code)) == edgeSet(g), "pruferDecode(pruferEncode(g)) edge set");
        checkDecode(code);}

    void small() {
        int rounds = mode == "quick" ? 300 : mode == "full" ? (CHECKED ? 1000 : ASAN ? 1500 : 3000) : CHECKED || ASAN ? 2000 : 12000;
        for (int t = 0; t < rounds; ++t) {
            int n = t < 8 ? t + 1 : 1 + rnd(t % 10 == 0 ? 120 : 40), kind = t % 6;
            Graph g = forest(n, rnd(6), 1, kind);
            Brute b(g);
            ++cases; context = "seed=" + std::to_string(seed) + " round=" + std::to_string(t) + " tree " + show(g);
            testTree(g, b); testDp(g, b);
            if (n >= 2) { testPrufer(g, b); }
            testForest(g, b, rnd(2) ? -1 : rnd(n), n <= 30);
            testMerge(g, b, {0, 1, 2, 3, rnd(n + 3), n - 1, n, n + 4, INT_MAX - 1, INT_MAX});
            testEmptyMerge(g);
            int m = t < 8 ? t : rnd(t % 10 == 0 ? 120 : 40), comps = 1 + rnd(4);
            Graph f = forest(m, rnd(6), comps, kind);
            Brute bf(f);
            ++cases; context = "seed=" + std::to_string(seed) + " round=" + std::to_string(t) + " forest " + show(f);
            testForest(f, bf, m && rnd(2) ? rnd(m) : -1, m <= 30);}
        context = "empty graph";
        check(sumOfDistances(Graph(0)).empty() && EulerTour(Graph(0)).order.empty() && BfsNumbering(Graph(0)).order.empty(), "n = 0");
        check(VirtualTree(Graph(0)).build({}).vertices.empty(), "VirtualTree n = 0");
        context = "single vertex";
        auto d = treeDiameter(Graph(1));
        check(d.length == 0 && d.vertices == vector<int>{0} && d.arcs.empty(), "treeDiameter n = 1");
        Graph neg(3);
        neg.addEdge(0, 1, -5); neg.addEdge(1, 2, 0);
        context = "nonpositive edges";
        d = treeDiameter(neg);
        check(d.length == 0 && d.vertices == vector<int>{0} && d.arcs.empty(), "treeDiameter nonpositive edges is vertex 0");
        cout << "PASS random trees n<=120 (6 shapes, unit/signed/nonnegative/nonpositive/full-range/extreme weights) and forests n<=120 (1-4 trees) rounds=" << rounds << '\n';}

    void prufer() {
        int top = mode == "quick" ? 6 : mode == "full" ? 7 : 8;
        for (int n = 2; n <= top; ++n) {
            context = "prufer exhaustive n=" + std::to_string(n);
            lng total = 1;
            for (int i = 0; i < n - 2; ++i) { total *= n; }
            std::set<vector<pair<int, int>>> trees;
            vector<int> code(n - 2);
            for (lng c = 0; c < total; ++c) {
                lng x = c;
                for (int &v : code) { v = int(x % n); x /= n; }
                checkDecode(code);
                trees.insert(edgeSet(pruferDecode(code)));}
            check(lng(trees.size()) == total, "pruferDecode is injective over all n^(n-2) codes");}
        for (int t = 0; t < (mode == "quick" ? 300 : 3000); ++t) {
            int n = 2 + rnd(t % 10 ? 30 : 300);
            vector<int> code(n - 2);
            for (int &v : code) { v = rnd(t % 3 ? n : min(n, 3)); }
            context = "prufer random code n=" + std::to_string(n) + " seed=" + std::to_string(seed) + " round=" + std::to_string(t);
            checkDecode(code);}
        cout << "PASS prufer exhaustive n<=" << top << " and random codes n<=301\n";}

    // Large trees: sampled exact distances, oracle rooting, oracle DPs at sampled roots, closed-form counts.
    void largeCommon(const Graph &g, const Brute &b, bool nonneg) {
        int n = g.n;
        auto dia = treeDiameter(g);
        checkPath(g, dia);
        auto te = treeEccentricities(g);
        checkEq(std::ranges::max(te.ecc), dia.length, "max eccentricity is the diameter");
        if (nonneg) {
            auto d0 = b.from(0).fi;
            auto dx = b.from(int(std::ranges::max_element(d0) - d0.begin())).fi;
            checkEq(dia.length, std::ranges::max(dx), "treeDiameter double sweep (nonnegative weights)");}
        auto sod = sumOfDistances(g);
        vector<lng> one(n, 1);
        Rerooting<Weighted> rw(g, Weighted{&one});
        Rerooting<Height> rh{CsrGraph(g)};
        for (int t = 0; t < 4; ++t) {
            int v = rnd(n);
            auto [d, h] = b.from(v);
            checkEq(te.ecc[v], std::ranges::max(d), "treeEccentricities sampled vertex");
            checkEq(sod[v], accumulate(d.begin(), d.end(), lll(0)), "sumOfDistances sampled vertex");
            auto nw = naiveDp(b, v, Weighted{&one});
            auto nh = naiveDp(b, v, Height{});
            check(rw.get(v) == nw[v] && rh.get(v) == nh[v] && rh.get(v) == std::ranges::max(h), "Rerooting get sampled root");
            for (auto e : b.adj[v]) { check(rw.get(e.to, v) == nw[e.to] && rh.get(e.to, v) == nh[e.to], "Rerooting get(v, p) sampled"); }}
        for (int v = 0; v < n; ++v) { checkEq(rw.get(v).sum, sod[v], "Rerooting distance sums equal sumOfDistances"); }
        lll radius = std::ranges::min(te.ecc);
        vector<int> center;
        for (int v = 0; v < n; ++v) {
            if (te.ecc[v] == radius) { center.push_back(v); }}
        check(te.radius == radius && te.center == center, "treeEccentricities radius and center from ecc");
        auto o = rootedOracle(b, 0);
        vector<int> size(n, 1), worst(n, 0), centroid;
        for (int i = n - 1; i > 0; --i) {
            int v = o.order[i];
            size[o.parent[v]] += size[v]; worst[o.parent[v]] = max(worst[o.parent[v]], size[v]);}
        for (int v = 0; v < n; ++v) {
            if (2 * max(worst[v], n - size[v]) <= n) { centroid.push_back(v); }}
        checkVec(treeCentroid(g), centroid, "treeCentroid vs oracle subtree sizes");
        int root = rnd(n);
        check(subtreeDp(g, root, Hash{}) == naiveDp(b, root, Hash{}), "subtreeDp ordered hash large");
        testForest(g, b, rnd(2) ? -1 : rnd(n), false);
        testMerge(g, b, {0, 3, 4});
        auto code = pruferEncode(g);
        check(edgeSet(pruferDecode(code)) == edgeSet(g), "pruferDecode(pruferEncode(g)) edge set large");
        for (int &v : code) { v = rnd(n); }
        checkVec(pruferEncode(pruferDecode(code)), code, "pruferEncode(pruferDecode(random code)) large");}

    // Deep path: vertex label[i] at position i; closed forms from prefix weights.
    void deepPath(int n, bool shuffled) {
        vector<int> label(n);
        iota(label.begin(), label.end(), 0);
        if (shuffled) { std::shuffle(label.begin(), label.end(), rng); }
        vector<lng> w(n - 1);
        for (auto &x : w) { x = shuffled ? weight(4 + rnd(2)) : 1; }
        vector<int> idx(n - 1);
        iota(idx.begin(), idx.end(), 0);
        if (shuffled) { std::shuffle(idx.begin(), idx.end(), rng); }
        Graph g(n);
        for (int i : idx) {
            if (!shuffled || rng() & 1) { g.addEdge(label[i], label[i + 1], w[i]); } else { g.addEdge(label[i + 1], label[i], w[i]); }}
        ++cases; context = "deep path n=" + std::to_string(n) + (shuffled ? " shuffled labels, full-range weights" : " identity, unit weights");
        vector<lll> p(n), low(n), high(n), ecc(n), sum(n);
        for (int i = 0; i + 1 < n; ++i) { p[i + 1] = p[i] + w[i]; }
        lll best = 0, cur = 0, left = 0, right = accumulate(p.begin(), p.end(), lll(0));
        for (lng x : w) { cur = max(lll(0), cur + x); best = max(best, cur); }
        for (int i = 0; i < n; ++i) { low[i] = i ? min(low[i - 1], p[i]) : p[i]; }
        for (int i = n - 1; i >= 0; --i) { high[i] = i + 1 < n ? max(high[i + 1], p[i]) : p[i]; }
        for (int i = 0; i < n; ++i) {
            right -= p[i];
            ecc[label[i]] = max(high[i] - p[i], p[i] - low[i]);
            sum[label[i]] = right - lll(n - 1 - i) * p[i] + lll(i) * p[i] - left;
            left += p[i];}
        vector<int> centroid{label[(n - 1) / 2], label[n / 2]};
        sort(centroid.begin(), centroid.end());
        centroid.erase(unique(centroid.begin(), centroid.end()), centroid.end());
        checkEq(treeDiameter(g).length, best, "treeDiameter path (max subarray)");
        checkVec(treeEccentricities(g).ecc, ecc, "treeEccentricities path closed form");
        checkVec(treeCentroid(g), centroid, "treeCentroid path middle");
        checkVec(sumOfDistances(g), sum, "sumOfDistances path closed form");
        Rerooting<Height> rh(g);
        for (int i = 0; i < n; ++i) { check(rh.get(label[i]) == max(i, n - 1 - i), "Rerooting height on path"); }
        if (!shuffled) {
            EulerTour et(g);
            BfsNumbering bn(g);
            for (int v = 0; v < n; ++v) { check(et.tin[v] == v && et.tout[v] == n && bn.order[v] == v && et.dep[v] == v, "path rooted at 0 is one chain"); }
            vector<lng> ones(min(n, 7) + 1, 1);
            checkVec(subtreeMergeDp(g, 0, 7, Connected{}), ones, "subtreeMergeDp path from an end: one connected set per size");}
        largeCommon(g, Brute(g), !shuffled);}

    void large() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        int deep = CHECKED ? min(n, 50000) : ASAN ? min(n, 200000) : n, other = CHECKED || ASAN ? min(n, 50000) : n;
        deepPath(deep, false); deepPath(deep, true);
        for (int shape : {0, 2, 3, 4, 5}) {
            Graph g = forest(other, shape, 1, shape == 0 ? 4 : 2);
            ++cases; context = "large shape=" + std::to_string(shape) + " n=" + std::to_string(other);
            largeCommon(g, Brute(g), shape != 0);}
        cout << "PASS large deep paths (identity unit, shuffled full-range) n=" << deep << " and random/star/caterpillar/binary/broom n=" << other << '\n';}

    void invalid(const string &name) {
        Graph g(3), cyc(3), part(3), multi(2), loop(1);
        g.addEdge(0, 1); g.addEdge(1, 2);
        cyc.addEdge(0, 1); cyc.addEdge(1, 2); cyc.addEdge(2, 0);
        part.addEdge(0, 1);
        multi.addEdge(0, 1); multi.addEdge(1, 0);
        loop.addEdge(0, 0);
        if (name == "diameter-directed") { treeDiameter(Graph(2, true)); }
        else if (name == "diameter-cycle") { treeDiameter(cyc); }
        else if (name == "diameter-forest") { treeDiameter(part); }
        else if (name == "diameter-empty") { treeDiameter(Graph(0)); }
        else if (name == "centroid-empty") { treeCentroid(Graph(0)); }
        else if (name == "eccentricity-cycle") { treeEccentricities(cyc); }
        else if (name == "eccentricity-empty") { treeEccentricities(Graph(0)); }
        else if (name == "centroid-forest") { treeCentroid(part); }
        else if (name == "distances-directed") { sumOfDistances(Graph(2, true)); }
        else if (name == "euler-root") { EulerTour(g, 3); }
        else if (name == "euler-cycle") { EulerTour{cyc}; }
        else if (name == "euler-multi-edge") { EulerTour{multi}; }
        else if (name == "subtree-range") { EulerTour(g).subtreeRange(3); }
        else if (name == "bfs-loop") { BfsNumbering{loop}; }
        else if (name == "level-range") { BfsNumbering(g).levelRange(-1, 0); }
        else if (name == "disk-negative") { BfsNumbering(g).diskRanges(0, -1); }
        else if (name == "rerooting-forest") { Rerooting<Height>{part}; }
        else if (name == "rerooting-nonadjacent") { Rerooting<Height>(g).get(0, 2); }
        else if (name == "subtree-dp-root") { subtreeDp(g, 3, Height{}); }
        else if (name == "merge-negative-k") { subtreeMergeDp(g, 0, -1, Binomial{}); }
        else if (name == "virtual-range") { VirtualTree(g).build({3}); }
        else if (name == "virtual-cycle") { VirtualTree{cyc}; }
        else if (name == "prufer-single") { pruferEncode(Graph(1)); }
        else if (name == "prufer-forest") { pruferEncode(part); }
        else if (name == "prufer-value") { pruferDecode({3}); }
        else if (name == "subtree-range-far") { EulerTour(g).subtreeRange(4); }
        else if (name == "level-range-far") { BfsNumbering(g).levelRange(4, 0); }
        else if (name == "disk-range") { BfsNumbering(g).diskRanges(3, 0); }
        else if (name == "rerooting-get-range") { Rerooting<Height>(g).get(3); }
        else if (name == "rerooting-parent-far") { Rerooting<Height>(g).get(0, 4); }
        else if (name == "prufer-value-far") { pruferDecode({4}); }
        else if (name == "prufer-negative") { pruferDecode({0, -1}); }
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
        small(); prufer(); large();
        cout << "PASS tree_algorithms seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL tree_algorithms seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
