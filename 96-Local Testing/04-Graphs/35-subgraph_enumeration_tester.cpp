#include "../../04-Graphs/35-subgraph_enumeration.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    namespace sd = subgraph_enumeration_detail;
    ulng seed = 20260927;
    string mode = "full", context, at;
    lng checks = 0, cases = 0;
    // Weights for the weighted clustering checks, seeded from --seed in main.
    std::mt19937_64 wrng;
    // libstdc++ debug mode at -O0 is about 50x and ASan/UBSan about 10x slower: both keep full-mode rounds in stress and shrink the large shapes.
#ifdef _GLIBCXX_DEBUG
    const bool CHECKED = true;
#else
    const bool CHECKED = false;
#endif
#ifdef __SANITIZE_ADDRESS__
    const bool ASAN = true;
#else
    const bool ASAN = false;
#endif
    // Divisor of the large shapes.
    const int SHRINK = CHECKED ? 10 : ASAN ? 8 : 1;
    const ulng MOD = 998244353;
    using Edge = array<int, 2>;
    using Triple = array<int, 3>;
    using Quad = array<int, 4>;
    using Census = array<lng, 16>;
    using Arcs = array<array<bool, 3>, 3>;
    // NetworkX triads.py: TRICODES[_tricode(G, v, u, w)] is the 1-based census index; bits v->u 1, u->v 2, v->w 4, w->v 8, u->w 16, w->u 32.
    constexpr int TRICODES[64] = {1, 2, 2, 3, 2, 4, 6, 8, 2, 6, 5, 7, 3, 8, 7, 11, 2, 6, 4, 8, 5, 9, 9, 13, 6, 10, 9, 14, 7, 14, 12, 15,
        2, 5, 6, 7, 6, 9, 10, 14, 4, 9, 9, 12, 8, 13, 14, 15, 3, 7, 8, 11, 7, 12, 14, 15, 8, 14, 13, 15, 11, 15, 15, 16};
    const array<string, 16> NAMES = {"003", "012", "102", "021D", "021U", "021C", "111D", "111U", "030T", "030C", "201", "120D", "120U", "120C", "210", "300"};
    // Labelled triads per type, 64 in total.
    const array<int, 16> ORBITS = {1, 6, 3, 3, 3, 6, 6, 6, 6, 2, 3, 3, 3, 6, 6, 1};
    // NetworkX shapes on A = 0, B = 1, C = 2 in census order: 021D = A<-B->C, 111D = A<->B<-C, 120D = A<-B->C with A<->C.
    const vector<vector<Edge>> SHAPES = {{}, {{0, 1}}, {{0, 1}, {1, 0}}, {{1, 0}, {1, 2}}, {{0, 1}, {2, 1}}, {{0, 1}, {1, 2}},
        {{0, 1}, {1, 0}, {2, 1}}, {{0, 1}, {1, 0}, {1, 2}}, {{0, 1}, {1, 2}, {0, 2}}, {{0, 1}, {1, 2}, {2, 0}},
        {{0, 1}, {1, 0}, {1, 2}, {2, 1}}, {{1, 0}, {1, 2}, {0, 2}, {2, 0}}, {{0, 1}, {2, 1}, {0, 2}, {2, 0}}, {{0, 1}, {1, 2}, {0, 2}, {2, 0}},
        {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {0, 2}}, {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {0, 2}, {2, 0}}};
    const int PERMS[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};

    auto last = std::chrono::steady_clock::now();
    // Seconds since the previous section ended.
    string elapsed() {
        auto now = std::chrono::steady_clock::now();
        double s = std::chrono::duration<double>(now - std::exchange(last, now)).count();
        return " time=" + std::to_string(int(s * 100) / 100.0).substr(0, 5) + "s";}
    template<class T>
    string str(const T &x) {
        if constexpr (std::is_convertible_v<T, string>) { return string(x); }
        else if constexpr (std::is_integral_v<T>) { return std::to_string(x); }
        else if constexpr (std::is_floating_point_v<T>) {
            std::ostringstream out;
            out << std::setprecision(17) << x;
            return out.str();}
        else if constexpr (requires { x.val(); }) { return std::to_string(x.val()); }
        else if constexpr (requires { x.first; }) { return "(" + str(x.first) + ", " + str(x.second) + ")"; }
        else {
            string s = "[";
            int k = 0;
            for (const auto &y : x) {
                if (++k > 20) { return s + "... size=" + std::to_string(std::size(x)) + "]"; }
                s += str(y) + ' ';}
            return s + "]";}}
    // First mismatch of two ranges, so long vectors stay readable.
    template<class A, class B>
    string diff(const A &a, const B &b) {
        if constexpr (requires { a.begin(); b.begin(); } && !std::is_convertible_v<A, string>) {
            auto i = a.begin();
            auto j = b.begin();
            for (int k = 0; i != a.end() && j != b.end(); ++i, ++j, ++k) {
                if (!(*i == *j)) { return " first_mismatch=" + std::to_string(k) + " expected=" + str(*j) + " actual=" + str(*i); }}
            return " size expected=" + std::to_string(std::size(b)) + " actual=" + std::to_string(std::size(a));}
        else { return ""; }}
    template<class A, class B>
    void same(const A &actual, const B &expected, const char *op) {
        ++checks;
        if (!(actual == expected)) { throw std::runtime_error(context + at + " operation=" + op + " expected=" + str(expected) + " actual=" + str(actual) + diff(actual, expected)); }}
    void check(bool ok, const char *op) { same(ok, true, op); }
    int rnd(std::mt19937_64 &rng, int k) { return int(rng() % ulng(k)); }
    bool quick() { return mode == "quick"; }
    // Size for quick, full and stress; the checked build runs stress at full size.
    int pick(int q, int f, int s) { return quick() ? q : mode == "stress" && SHRINK == 1 ? s : f; }
    lng choose(lng n, int k) {
        if (n < k) { return 0; }
        lll r = 1;
        for (int i = 0; i < k; ++i) { r = r * (n - i) / (i + 1); }
        return lng(r);}
    // n * (n - 1) * ... with k factors, zero once a factor reaches zero.
    lng falling(lng n, int k) {
        lng r = 1;
        for (int i = 0; i < k; ++i) { r *= max<lng>(n - i, 0); }
        return r;}
    lll c3(lll n) { return n * (n - 1) * (n - 2) / 6; }
    Graph build(int n, const vector<Edge> &es, bool directed = false) {
        Graph g(n, directed);
        for (auto [u, v] : es) { g.addEdge(u, v); }
        return g;}
    string show(const Graph &g) {
        string s = " n=" + std::to_string(g.n) + " m=" + std::to_string(g.edges.size()) + (g.directed ? " directed" : "");
        if (g.edges.size() > 80) { return s; }
        s += " edges=";
        for (auto e : g.edges) { s += std::to_string(e.u) + "-" + std::to_string(e.v) + " "; }
        return s;}

    // Random multigraph: loops kept with probability loops%, an extra parallel or reversed copy with probability dups%.
    vector<Edge> randomEdges(std::mt19937_64 &rng, int n, lng m, int loops, int dups) {
        vector<Edge> es;
        while (n && lng(es.size()) < m) {
            int u = rnd(rng, n), v = rnd(rng, n);
            if (u == v && n > 1 && rnd(rng, 100) >= loops) { continue; }
            es.push_back({u, v});
            if (rnd(rng, 100) < dups) { es.push_back(rng() & 1 ? Edge{u, v} : Edge{v, u}); }}
        es.resize(size_t(m * (n > 0)));
        return es;}
    // Each pair with probability p, random orientation.
    vector<Edge> densityEdges(std::mt19937_64 &rng, int n, double p) {
        vector<Edge> es;
        ulng cut = p >= 1 ? 0 : ulng(p * 0x1p64);
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) {
                if (p >= 1 || rng() < cut) { es.push_back(rng() & 1 ? Edge{u, v} : Edge{v, u}); }}}
        return es;}
    // Disjoint random blocks of 6..16 vertices with internal density p plus extra uniform edges.
    vector<Edge> blockEdges(std::mt19937_64 &rng, int n, double p, lng extra, bool directed) {
        vector<int> perm(n);
        iota(perm.begin(), perm.end(), 0);
        std::shuffle(perm.begin(), perm.end(), rng);
        vector<Edge> es = randomEdges(rng, n, extra, 1, 3);
        ulng cut = ulng(p * 0x1p64);
        for (int s = 0; s < n;) {
            int k = min(n - s, 6 + rnd(rng, 11));
            for (int i = 0; i < k; ++i) {
                for (int j = directed ? 0 : i + 1; j < k; ++j) {
                    if (i != j && rng() < cut) { es.push_back({perm[s + i], perm[s + j]}); }}}
            s += k;}
        std::shuffle(es.begin(), es.end(), rng);
        return es;}
    vector<Edge> completeEdges(std::mt19937_64 &rng, int n) {
        auto es = densityEdges(rng, n, 1);
        std::shuffle(es.begin(), es.end(), rng);
        return es;}

    // Index of the first element of sorted a whose key is >= x; hand-written because libstdc++ debug mode makes std::lower_bound check the whole range.
    template<class T, class K>
    int lowerIndex(const vector<T> &a, K x, K key(const T &)) {
        int l = 0, r = int(a.size());
        while (l < r) {
            int mid = std::midpoint(l, r);
            if (key(a[mid]) < x) { l = mid + 1; } else { r = mid; }}
        return l;}
    int self(const int &y) { return y; }
    int head(const pair<int, double> &y) { return y.first; }
    bool contains(const vector<int> &a, int x) {
        int i = lowerIndex(a, x, self);
        return i < int(a.size()) && a[i] == x;}
    // Sorted distinct non-loop neighbours (both directions for a digraph) with multiplicities.
    struct Lists {
        vector<vector<int>> nb;
        vector<vector<lng>> mu;
        bool simple = true;

        explicit Lists(const Graph &g) : nb(g.n), mu(g.n) {
            vector<vector<int>> raw(g.n);
            for (auto e : g.edges) {
                if (e.u == e.v) { simple = false; continue; }
                raw[e.u].push_back(e.v); raw[e.v].push_back(e.u);}
            for (int u = 0; u < g.n; ++u) {
                sort(raw[u].begin(), raw[u].end());
                for (int v : raw[u]) {
                    if (nb[u].empty() || nb[u].back() != v) { nb[u].push_back(v); mu[u].push_back(0); }
                    ++mu[u].back();
                    simple = simple && mu[u].back() == 1;}}}
        lng mult(int u, int v) const {
            int i = lowerIndex(nb[u], v, self);
            return i < int(nb[u].size()) && nb[u][i] == v ? mu[u][i] : 0;}
    };
    // Expected results; paths[k][s] belongs to start vertex sample[s]; tri is listed when listed, cyc (canonical) and k4s when also quads.
    struct Expect {
        bool listed = true, simple = true, quads = true;
        lng t = 0, c4 = 0, k4 = 0;
        vector<Triple> tri;
        vector<Quad> cyc, k4s;
        vector<Triple> tedges;
        vector<lng> tri_edge, c4_edge, c3, c4v;
        vector<int> sample, deg;
        array<vector<lng>, 3> paths;
        vector<double> lc, sq;
        double tr = 0;
        vector<vector<int>> gd;
    };
    // Sorted edge-ID triples of every edge triangle over the vertex triangles tri (each parallel copy combination), empty past limit.
    vector<Triple> edgeTriples(const Graph &g, const vector<Triple> &tri, size_t limit) {
        map<pair<int, int>, vector<int>> ids;
        for (int e = 0; e < int(g.edges.size()); ++e) {
            auto [u, v, w] = g.edges[e];
            if (u != v) { ids[{min(u, v), max(u, v)}].push_back(e); }}
        vector<Triple> res;
        for (auto [a, b, c] : tri) {
            for (int x : ids[{a, b}]) {
                for (int y : ids[{b, c}]) {
                    for (int z : ids[{a, c}]) {
                        if (res.size() >= limit) { return {}; }
                        Triple t{x, y, z};
                        sort(t.begin(), t.end()); res.push_back(t);}}}}
        sort(res.begin(), res.end());
        return res;}
    // Rotation and direction of a 4-cycle a-b-c-e-a with the smallest vertex first and the smaller of its neighbours second.
    Quad canon(Quad q) {
        std::rotate(q.begin(), std::min_element(q.begin(), q.end()), q.end());
        if (q[3] < q[1]) { swap(q[1], q[3]); }
        return q;}
    // NetworkX cluster.py transliterated on the simple graph: clustering, transitivity, square_clustering and generalized_degree.
    void addNx(Expect &x, const Lists &l) {
        int n = int(l.nb.size());
        auto common = [](const vector<int> &a, const vector<int> &b) {
            const auto &s = a.size() < b.size() ? a : b;
            const auto &t = a.size() < b.size() ? b : a;
            lng c = 0;
            for (int y : s) { c += contains(t, y); }
            return c;};
        x.deg.assign(n, 0); x.lc.assign(n, 0); x.sq.assign(n, 0); x.gd.assign(n, {});
        lng tri = 0, contri = 0;
        vector<lng> p2(n);
        vector<int> touched;
        vector<char> in(n);
        for (int v = 0; v < n; ++v) {
            const auto &vs = l.nb[v];
            lng d = lng(vs.size()), t = 0;
            x.deg[v] = int(d);
            // _triangles_and_degree_iter: gen_degree counts len(vs & adj[w]) over w in vs, t their sum (twice the triangles).
            for (int w : vs) {
                int k = int(common(vs, l.nb[w]));
                if (k >= int(x.gd[v].size())) { x.gd[v].resize(size_t(k) + 1); }
                ++x.gd[v][k]; t += k;}
            if (t) { x.lc[v] = double(t) / double(d * (d - 1)); }
            tri += t; contri += d * (d - 1);
            if (d - 1 <= 0) { continue; }
            lng uw_degrees = 0, uw_count = d * (d - 1), triangles = 0, squares = 0;
            for (int u : vs) {
                uw_degrees += lng(l.nb[u].size()) * (d - 1);
                lng q = common(l.nb[u], vs);
                triangles += q; squares += q * (q - 1);}
            // two_hop_neighbors minus N(v) and v, with p2 = |N(v) & N(x)| counted through the middle vertices.
            for (int u : vs) { in[u] = 1; }
            for (int u : vs) {
                for (int y : l.nb[u]) {
                    if (y == v || in[y]) { continue; }
                    if (!p2[y]++) { touched.push_back(y); }}}
            for (int y : touched) { squares += p2[y] * (p2[y] - 1); p2[y] = 0; }
            for (int u : vs) { in[u] = 0; }
            touched.clear();
            squares /= 2;
            if (lng potential = uw_degrees - uw_count - triangles - squares; potential > 0) { x.sq[v] = double(squares) / double(potential); }}
        x.tr = tri ? double(tri) / double(contri) : 0;}
    // Simple paths with 3, 4 and 5 vertices from each sampled start by depth-first enumeration.
    void dfsPaths(const Lists &l, Expect &x) {
        vector<char> on(l.nb.size());
        for (auto &p : x.paths) { p.assign(x.sample.size(), 0); }
        auto go = [&](auto &&f, int s, int v, int len) -> void {
            if (len >= 3) { ++x.paths[len - 3][s]; }
            if (len == 5) { return; }
            for (int w : l.nb[v]) {
                if (!on[w]) { on[w] = 1; f(f, s, w, len + 1); on[w] = 0; }}};
        for (int s = 0; s < int(x.sample.size()); ++s) {
            int v = x.sample[s];
            on[v] = 1; go(go, s, v, 1); on[v] = 0;}}
    // Adjacency-matrix brute force over vertex triples, quadruples (three cyclic orders each) and per-edge vertex choices.
    Expect brute(const Graph &g) {
        int n = g.n, m = int(g.edges.size());
        Lists l(g);
        vector<vector<lng>> mu(n, vector<lng>(n));
        for (auto e : g.edges) {
            if (e.u != e.v) { ++mu[e.u][e.v]; ++mu[e.v][e.u]; }}
        auto adj = [&](int a, int b) { return mu[a][b] > 0; };
        Expect x;
        x.simple = l.simple; x.c3.assign(n, 0); x.c4v.assign(n, 0); x.tri_edge.assign(m, 0); x.c4_edge.assign(m, 0);
        for (int a = 0; a < n; ++a) {
            for (int b = a + 1; b < n; ++b) {
                for (int c = b + 1; c < n; ++c) {
                    if (adj(a, b) && adj(b, c) && adj(a, c)) { x.tri.push_back({a, b, c}); ++x.c3[a]; ++x.c3[b]; ++x.c3[c]; }
                    for (int d = c + 1; d < n; ++d) {
                        const int q[3][4] = {{a, b, c, d}, {a, b, d, c}, {a, c, b, d}};
                        for (const auto &o : q) {
                            bool cyc = adj(o[0], o[1]) && adj(o[1], o[2]) && adj(o[2], o[3]) && adj(o[3], o[0]);
                            x.c4 += cyc; x.c4v[a] += cyc; x.c4v[b] += cyc; x.c4v[c] += cyc; x.c4v[d] += cyc;
                            if (cyc) { x.cyc.push_back(canon({o[0], o[1], o[2], o[3]})); }}
                        if (adj(a, b) && adj(a, c) && adj(a, d) && adj(b, c) && adj(b, d) && adj(c, d)) { ++x.k4; x.k4s.push_back({a, b, c, d}); }}}}}
        sort(x.cyc.begin(), x.cyc.end());
        x.t = lng(x.tri.size());
        for (int e = 0; e < m; ++e) {
            int u = g.edges[e].u, v = g.edges[e].v;
            for (int w = 0; w < n && u != v; ++w) {
                if (w == u || w == v) { continue; }
                x.tri_edge[e] += mu[u][w] * mu[w][v];
                for (int y = 0; y < n; ++y) {
                    if (y != u && y != v && y != w) { x.c4_edge[e] += mu[v][w] * mu[w][y] * mu[y][u]; }}}}
        x.sample.resize(n);
        iota(x.sample.begin(), x.sample.end(), 0);
        dfsPaths(l, x);
        addNx(x, l);
        x.tedges = edgeTriples(g, x.tri, SIZE_MAX);
        return x;}
    // Codegree oracle in O(sum of deg^2 * log): weighted two-paths p[w] and common neighbours c[w] from each u.
    Expect fast(const Graph &g, const vector<int> &sample) {
        int n = g.n, m = int(g.edges.size());
        Lists l(g);
        Expect x;
        x.simple = l.simple; x.c3.assign(n, 0); x.c4v.assign(n, 0); x.tri_edge.assign(m, 0); x.c4_edge.assign(m, 0);
        vector<lng> p(n), c(n);
        vector<int> touched;
        vector<char> mark(n);
        vector<vector<int>> owned(n), mids(n);
        vector<int> ends;
        // 4-cycles and 4-cliques are listed up to 2 * 10^6 each, otherwise only counted.
        auto keep = [&](vector<Quad> &list, Quad q) {
            if (x.quads && list.size() >= 2000000) {
                x.quads = false;
                vector<Quad>().swap(x.cyc); vector<Quad>().swap(x.k4s);}
            if (x.quads) { list.push_back(q); }};
        for (int e = 0; e < m; ++e) {
            if (g.edges[e].u != g.edges[e].v) { owned[g.edges[e].u].push_back(e); }}
        lng quads = 0;
        for (int u = 0; u < n; ++u) {
            const auto &nu = l.nb[u];
            for (int i = 0; i < int(nu.size()); ++i) {
                int v = nu[i];
                for (int j = 0; j < int(l.nb[v].size()); ++j) {
                    int w = l.nb[v][j];
                    if (w == u) { continue; }
                    if (!c[w]++) { touched.push_back(w); }
                    p[w] += l.mu[u][i] * l.mu[v][j];}}
            for (int v : nu) { x.c3[u] += c[v]; mark[v] = 1; }
            x.c3[u] /= 2;
            for (int w : touched) { x.c4v[u] += c[w] * (c[w] - 1) / 2; }
            quads += x.c4v[u];
            for (int e : owned[u]) {
                int v = g.edges[e].v;
                lng a = l.mult(u, v), s = 0;
                x.tri_edge[e] = p[v];
                for (int j = 0; j < int(l.nb[v].size()); ++j) {
                    int w = l.nb[v][j];
                    if (w != u) { s += l.mu[v][j] * (p[w] - a * l.mu[v][j]); }}
                x.c4_edge[e] = s;}
            for (int v : nu) {
                if (v < u) { continue; }
                vector<int> common;
                for (int w : l.nb[v]) {
                    if (w > v && mark[w]) { x.tri.push_back({u, v, w}); common.push_back(w); }}
                for (int i = 0; i < int(common.size()); ++i) {
                    for (int j = i + 1; j < int(common.size()); ++j) {
                        if (l.mult(common[i], common[j])) { ++x.k4; keep(x.k4s, {u, v, common[i], common[j]}); }}}}
            // 4-cycles u-y-w-z with u the smallest vertex: middles y < z of the two-paths u-y-w above u.
            for (int y : nu) {
                if (y < u) { continue; }
                for (int w : l.nb[y]) {
                    if (w <= u) { continue; }
                    if (mids[w].empty()) { ends.push_back(w); }
                    mids[w].push_back(y);}}
            for (int w : ends) {
                for (int i = 0; i < int(mids[w].size()); ++i) {
                    for (int j = i + 1; j < int(mids[w].size()); ++j) { keep(x.cyc, {u, mids[w][i], w, mids[w][j]}); }}
                mids[w].clear();}
            ends.clear();
            for (int v : nu) { mark[v] = 0; }
            for (int w : touched) { p[w] = 0; c[w] = 0; }
            touched.clear();}
        check(quads % 4 == 0, "oracle self-check: per-vertex 4-cycles sum to a multiple of 4");
        sort(x.tri.begin(), x.tri.end()); sort(x.cyc.begin(), x.cyc.end()); sort(x.k4s.begin(), x.k4s.end());
        x.t = lng(x.tri.size()); x.c4 = quads / 4; x.sample = sample;
        same(x.quads ? lng(x.cyc.size()) : x.c4, x.c4, "oracle self-check: listed 4-cycles match the codegree count");
        dfsPaths(l, x);
        addNx(x, l);
        lng et = accumulate(x.tri_edge.begin(), x.tri_edge.end(), lng(0)) / 3;
        x.tedges = edgeTriples(g, x.tri, et <= 2000000 ? SIZE_MAX : 0);
        return x;}
    // Edge-set semantics by enumerating 3- and 4-edge subsets: non-loop edges on as many vertices, each of degree 2, connected.
    pair<vector<lng>, vector<lng>> subsets(const Graph &g, vector<Triple> *tris = nullptr) {
        int m = int(g.edges.size());
        vector<lng> t(m), q(m);
        auto cycle = [&](const vector<int> &ids) {
            vector<int> vs;
            for (int id : ids) {
                if (g.edges[id].u == g.edges[id].v) { return false; }
                vs.push_back(g.edges[id].u); vs.push_back(g.edges[id].v);}
            vector<int> all = vs;
            sort(vs.begin(), vs.end());
            vs.erase(unique(vs.begin(), vs.end()), vs.end());
            if (vs.size() != ids.size()) { return false; }
            for (int v : vs) {
                if (std::count(all.begin(), all.end(), v) != 2) { return false; }}
            vector<int> comp{g.edges[ids[0]].u, g.edges[ids[0]].v};
            for (int pass = 0; pass < int(ids.size()); ++pass) {
                for (int id : ids) {
                    bool a = std::ranges::count(comp, g.edges[id].u) > 0, b = std::ranges::count(comp, g.edges[id].v) > 0;
                    if (a != b) { comp.push_back(a ? g.edges[id].v : g.edges[id].u); }}}
            return comp.size() == vs.size();};
        for (int i = 0; i < m; ++i) {
            for (int j = i + 1; j < m; ++j) {
                for (int k = j + 1; k < m; ++k) {
                    if (cycle({i, j, k})) {
                        ++t[i]; ++t[j]; ++t[k];
                        if (tris) { tris->push_back({i, j, k}); }}
                    for (int l = k + 1; l < m; ++l) {
                        if (cycle({i, j, k, l})) { ++q[i]; ++q[j]; ++q[k]; ++q[l]; }}}}}
        return {t, q};}
    void sameExpect(const Expect &a, const Expect &b, const char *op) {
        at = string(" check=") + op;
        same(a.tri, b.tri, "triangles"); same(a.t, b.t, "triangle count"); same(a.c4, b.c4, "4-cycles"); same(a.k4, b.k4, "4-cliques");
        same(a.tri_edge, b.tri_edge, "per-edge triangles"); same(a.c4_edge, b.c4_edge, "per-edge 4-cycles");
        same(a.c3, b.c3, "per-vertex triangles"); same(a.c4v, b.c4v, "per-vertex 4-cycles"); same(a.paths, b.paths, "paths");
        same(a.cyc, b.cyc, "4-cycle list"); same(a.k4s, b.k4s, "4-clique list"); same(a.lc, b.lc, "local clustering");
        same(a.sq, b.sq, "square clustering"); same(a.tr, b.tr, "transitivity"); same(a.gd, b.gd, "generalized degree");
        same(a.tedges, b.tedges, "edge-triangle list");
        at = "";}

    // Independent witness check: closed walk on 4 distinct vertices whose arcs exist with matching endpoints.
    template<class G>
    void witness(const G &g, const CycleWitness &w, bool exists) {
        if (!exists) {
            same(w.vertices.size() + w.arcs.size(), size_t(0), "findC4 empty without a 4-cycle");
            return;}
        same(w.vertices.size(), size_t(5), "findC4 closed walk of 5 vertices");
        same(w.arcs.size(), size_t(4), "findC4 4 arcs");
        same(w.vertices[0], w.vertices[4], "findC4 walk is closed");
        vector<int> d(w.vertices.begin(), w.vertices.begin() + 4);
        sort(d.begin(), d.end());
        check(std::adjacent_find(d.begin(), d.end()) == d.end() && d[0] >= 0 && d[3] < g.n, "findC4 four distinct vertices");
        for (int i = 0; i < 4; ++i) {
            int a = w.arcs[i];
            check(0 <= a && a < int(g.arcs.size()), "findC4 arc id in range");
            same(g.arcs[a].from, w.vertices[i], "findC4 arc tail"); same(g.arcs[a].to, w.vertices[i + 1], "findC4 arc head");}}
    // Every undirected operation against the expectation, plus identities between the operations.
    template<class G>
    void verify(const G &g, const Graph &src, const Expect &x, std::mt19937_64 &rng) {
        int n = g.n;
        ++cases;
        vector<Triple> got;
        enumerateTriangles(g, [&](int a, int b, int c) {
            Triple t{a, b, c};
            sort(t.begin(), t.end()); got.push_back(t);});
        sort(got.begin(), got.end());
        if (x.listed) { same(got, x.tri, "enumerateTriangles triples, each once"); }
        else {
            same(lng(got.size()), x.t, "enumerateTriangles call count");
            check(std::adjacent_find(got.begin(), got.end()) == got.end(), "enumerateTriangles reports no triangle twice");
            Lists l(src);
            for (auto [a, b, c] : got) {
                check(0 <= a && a < b && b < c && c < n && l.mult(a, b) && l.mult(b, c) && l.mult(a, c), "enumerateTriangles reports triangles of distinct vertices");}}
        lng t = countTriangles(g);
        same(t, x.t, "countTriangles");
        ulng k = rng();
        same(triangleProductSum(g, vector<ulng>(n, k)), ulng(x.t) * k * k * k, "triangleProductSum ulng constant (wraparound)");
        same(triangleProductSum(g, vector<lng>(n, 1)), x.t, "triangleProductSum x = 1 equals countTriangles");
        if (x.listed) {
            vector<lng> xl(n);
            vector<ulng> xu(n), xr(n);
            vector<mint> xm(n);
            for (int v = 0; v < n; ++v) { xl[v] = lng(rng() % 2001) - 1000; xu[v] = rng(); xr[v] = rng() % MOD; xm[v] = mint(xr[v]); }
            lng el = 0;
            ulng eu = 0, em = 0;
            for (auto [a, b, c] : x.tri) { el += xl[a] * xl[b] * xl[c]; eu += xu[a] * xu[b] * xu[c]; em = (em + xr[a] * xr[b] % MOD * xr[c]) % MOD; }
            same(triangleProductSum(g, xl), el, "triangleProductSum lng");
            same(triangleProductSum(g, xu), eu, "triangleProductSum ulng random (wraparound)");
            same(ulng(triangleProductSum(g, xm).val()), em, "triangleProductSum ModInt<998244353>");}
        auto te = countTrianglesPerEdge(g);
        same(te, x.tri_edge, "countTrianglesPerEdge");
        same(countC4(g), x.c4, "countC4");
        auto qe = countC4PerEdge(g);
        same(qe, x.c4_edge, "countC4PerEdge");
        auto [v3, v4] = countC3C4PerVertex(g);
        same(v3, x.c3, "countC3C4PerVertex triangles"); same(v4, x.c4v, "countC3C4PerVertex 4-cycles");
        witness(g, findC4(g), x.c4 > 0);
        same(countK4(g), x.k4, "countK4");
        auto p = countPathsP3P4P5(g);
        array<vector<lng>, 3> sub;
        for (int i = 0; i < 3; ++i) {
            same(int(p[i].size()), n, "countPathsP3P4P5 size");
            for (int v : x.sample) { sub[i].push_back(p[i][v]); }}
        same(sub, x.paths, "countPathsP3P4P5 at sampled starts");
        same(accumulate(v3.begin(), v3.end(), lng(0)), 3 * t, "identity: per-vertex triangles sum to 3 * countTriangles");
        same(accumulate(v4.begin(), v4.end(), lng(0)), 4 * x.c4, "identity: per-vertex 4-cycles sum to 4 * countC4");
        if (x.simple) {
            same(accumulate(te.begin(), te.end(), lng(0)), 3 * t, "identity: simple per-edge triangles sum to 3 * countTriangles");
            same(accumulate(qe.begin(), qe.end(), lng(0)), 4 * x.c4, "identity: simple per-edge 4-cycles sum to 4 * countC4");}
        // enumerateC4 and enumerateK4: exact lists when listed, else count plus validity and no repeats among the first 10^6.
        bool exact = x.listed && x.quads;
        size_t cap = exact ? SIZE_MAX : 1000000;
        vector<Quad> cs, ks;
        lng nc = 0, nk = 0;
        enumerateC4(g, [&](int a, int b, int c, int e) {
            if (cs.size() < cap) { cs.push_back(canon({a, b, c, e})); }
            ++nc;});
        enumerateK4(g, [&](int a, int b, int c, int e) {
            Quad q{a, b, c, e};
            sort(q.begin(), q.end());
            if (ks.size() < cap) { ks.push_back(q); }
            ++nk;});
        sort(cs.begin(), cs.end()); sort(ks.begin(), ks.end());
        same(nc, x.c4, "enumerateC4 calls equal countC4"); same(nk, x.k4, "enumerateK4 calls equal countK4");
        if (exact) { same(cs, x.cyc, "enumerateC4 cycles, each once"); same(ks, x.k4s, "enumerateK4 cliques, each once"); }
        else {
            const Lists l(src);
            check(std::adjacent_find(cs.begin(), cs.end()) == cs.end() && std::adjacent_find(ks.begin(), ks.end()) == ks.end(), "enumerateC4 / enumerateK4 report nothing twice");
            for (auto q : cs) {
                bool ok = 0 <= q[0] && q[0] < q[1] && q[0] < q[2] && q[1] < q[3] && q[2] != q[1] && q[2] != q[3] && q[2] < n && q[3] < n;
                for (int i = 0; i < 4 && ok; ++i) { ok = l.mult(q[i], q[(i + 1) % 4]) > 0; }
                check(ok, "enumerateC4 reports a 4-cycle on 4 distinct vertices");}
            for (auto q : ks) {
                bool ok = q[0] >= 0 && q[0] < q[1] && q[1] < q[2] && q[2] < q[3] && q[3] < n;
                for (int i = 0; i < 4 && ok; ++i) {
                    for (int j = i + 1; j < 4 && ok; ++j) { ok = l.mult(q[i], q[j]) > 0; }}
                check(ok, "enumerateK4 reports a 4-clique");}}
        // enumerateTriangleEdges: edge IDs joining a-b, b-c, c-a; exact list when listed, else count and no repeats among the first 2 * 10^6.
        lng nt = 0;
        bool joined = true;
        vector<Triple> te3;
        auto joins = [&](int e, int a, int b) {
            if (e < 0 || e >= int(g.edges.size())) { return false; }
            auto ed = g.edges[e];
            return (ed.u == a && ed.v == b) || (ed.u == b && ed.v == a);};
        enumerateTriangleEdges(g, [&](int a, int b, int c, int ex, int ey, int ez) {
            ++nt;
            joined = joined && a != b && b != c && a != c && joins(ex, a, b) && joins(ey, b, c) && joins(ez, c, a);
            Triple q{ex, ey, ez};
            sort(q.begin(), q.end());
            if (te3.size() < 2000000) { te3.push_back(q); }});
        sort(te3.begin(), te3.end());
        check(joined, "enumerateTriangleEdges edges join a-b, b-c, c-a on three distinct vertices");
        same(nt, accumulate(x.tri_edge.begin(), x.tri_edge.end(), lng(0)) / 3, "enumerateTriangleEdges calls = sum of per-edge triangles / 3");
        if (x.listed && (!x.tedges.empty() || nt == 0)) { same(te3, x.tedges, "enumerateTriangleEdges edge triangles, each once"); }
        else { check(std::adjacent_find(te3.begin(), te3.end()) == te3.end(), "enumerateTriangleEdges reports no edge triangle twice"); }
        double avg = 0, nz = 0;
        int kz = 0;
        for (double y : x.lc) { avg += y; }
        for (double y : x.lc) {
            if (std::abs(y) > 0) { nz += y; ++kz; }}
        same(averageClustering(g), n ? avg / double(n) : 0.0, "averageClustering (NetworkX average_clustering)");
        same(averageClustering(g, false), kz ? nz / double(kz) : 0.0, "averageClustering count_zeros = false");
        same(localClustering(g), x.lc, "localClustering (NetworkX clustering)");
        same(transitivity(g), x.tr, "transitivity (NetworkX transitivity)");
        same(squareClustering(g), x.sq, "squareClustering (NetworkX square_clustering)");
        auto gd = generalizedDegree(g);
        same(gd, x.gd, "generalizedDegree (NetworkX generalized_degree)");
        for (int v = 0; v < n; ++v) {
            lng deg = 0, twice = 0;
            for (int k = 0; k < int(gd[v].size()); ++k) { deg += gd[v][k]; twice += lng(k) * gd[v][k]; }
            same(deg, lng(x.deg[v]), "identity: generalizedDegree row sums to the simple degree");
            same(twice, 2 * v3[v], "identity: sum of k * generalizedDegree[v][k] = 2 * triangles at v");}}
    // Normalised weights w / max_weight per arc (both directions when undirected; into[v] lists arcs entering v), max over all edges, loops included, 1 without edges.
    struct Weights {
        vector<vector<pair<int, double>>> out, into;

        explicit Weights(const Graph &g) : out(g.n), into(g.n) {
            double top = g.edges.empty() ? 1 : double(std::ranges::max(g.edges, {}, &GraphEdge::w).w);
            for (auto e : g.edges) {
                if (e.u == e.v) { continue; }
                out[e.u].push_back({e.v, double(e.w) / top}); into[e.v].push_back({e.u, double(e.w) / top});
                if (!g.directed) { out[e.v].push_back({e.u, double(e.w) / top}); into[e.u].push_back({e.v, double(e.w) / top}); }}
            for (auto *l : {&out, &into}) {
                for (auto &o : *l) { sort(o.begin(), o.end()); }}}
        // wt(u, v), NaN when there is no arc u -> v.
        double wt(int u, int v) const {
            int i = lowerIndex(out[u], v, head);
            return i < int(out[u].size()) && out[u][i].first == v ? out[u][i].second : NAN;}
    };
    // NetworkX _weighted_triangles_and_degree_iter and clustering transliterated; second the same sum over |terms|, the error scale.
    // inbrs & jnbrs - seen is scanned from the smaller side, so a star stays linear.
    pair<vector<double>, vector<double>> nxWeighted(const Graph &g) {
        Weights w(g);
        int n = g.n;
        vector<double> res(n), scale(n), mine(n, NAN);
        vector<char> seen(n);
        for (int i = 0; i < n; ++i) {
            const auto &inbrs = w.out[i];
            double t = 0, a = 0;
            for (auto [k, x] : inbrs) { mine[k] = x; }
            for (auto [j, wij] : inbrs) {
                seen[j] = 1;
                auto term = [&](int k, double wjk) {
                    if (seen[k] || std::isnan(mine[k])) { return; }
                    double c = std::cbrt(wij * wjk * mine[k]);
                    t += c; a += std::abs(c);};
                if (w.out[j].size() <= inbrs.size()) {
                    for (auto [k, wjk] : w.out[j]) { term(k, wjk); }}
                else {
                    for (auto [k, x] : inbrs) {
                        if (double wjk = w.wt(j, k); !std::isnan(wjk)) { term(k, wjk); }}}}
            for (auto [k, x] : inbrs) { mine[k] = NAN; seen[k] = 0; }
            double d = double(inbrs.size());
            t *= 2; a *= 2;
            if (t != 0) { res[i] = t / (d * (d - 1)); }
            if (a != 0) { scale[i] = a / (d * (d - 1)); }}
        return {res, scale};}
    // NetworkX _directed_weighted_triangles_and_degree_iter and directed clustering transliterated, with the |terms| scale:
    // for j in ipreds (first factor wt(j, i)) then isuccs (wt(i, j)), k over jpreds (wt(k, j)) and jsuccs (wt(j, k)) meeting ipreds (wt(k, i)) or isuccs (wt(i, k)).
    pair<vector<double>, vector<double>> nxWeightedDirected(const Graph &g) {
        int n = g.n;
        Weights w(g);
        vector<double> res(n), scale(n), in_w(n, NAN), out_w(n, NAN);
        for (int i = 0; i < n; ++i) {
            double t = 0, a = 0, db = 0;
            for (auto [k, x] : w.into[i]) { in_w[k] = x; }
            for (auto [k, x] : w.out[i]) { out_w[k] = x; db += !std::isnan(in_w[k]); }
            auto add = [&](double c) {
                c = std::cbrt(c);
                t += c; a += std::abs(c);};
            for (int side = 0; side < 2; ++side) {
                for (auto [j, first] : side ? w.out[i] : w.into[i]) {
                    for (auto [k, wkj] : w.into[j]) {
                        if (!std::isnan(in_w[k])) { add(first * in_w[k] * wkj); }
                        if (!std::isnan(out_w[k])) { add(first * out_w[k] * wkj); }}
                    for (auto [k, wjk] : w.out[j]) {
                        if (!std::isnan(in_w[k])) { add(first * in_w[k] * wjk); }
                        if (!std::isnan(out_w[k])) { add(first * out_w[k] * wjk); }}}}
            for (auto [k, x] : w.into[i]) { in_w[k] = NAN; }
            for (auto [k, x] : w.out[i]) { out_w[k] = NAN; }
            double dt = double(w.into[i].size() + w.out[i].size());
            if (t != 0) { res[i] = t / ((dt * (dt - 1) - 2 * db) * 2); }
            if (a != 0) { scale[i] = a / ((dt * (dt - 1) - 2 * db) * 2); }}
        return {res, scale};}
    // |actual - expected| <= 1e-10 * scale: per term at most ~10 rounding units, summation of at most 4.5 * 10^4 terms adds (T - 1) u sum |terms| < 5e-12.
    void closeTo(const vector<double> &actual, const pair<vector<double>, vector<double>> &e, const char *op) {
        same(actual.size(), e.first.size(), op);
        for (int v = 0; v < int(actual.size()); ++v) {
            if (!(std::abs(actual[v] - e.first[v]) <= 1e-10 * e.second[v])) {
                at = " vertex=" + std::to_string(v) + " scale=" + str(e.second[v]);
                same(actual[v], e.first[v], op);}}
        ++checks;}
    // A copy of g without repeated (same-direction) arcs, weighted by mode: 0 all 1, 1 random in [1, 10^6], 2 random in [-5, 5] with max != 0.
    Graph weighted(const Graph &g, int mode) {
        Graph h(g.n, g.directed);
        set<pair<int, int>> seen;
        for (auto e : g.edges) {
            pair<int, int> key = g.directed || e.u <= e.v ? pair{e.u, e.v} : pair{e.v, e.u};
            if (!seen.insert(key).second) { continue; }
            lng w = mode == 0 ? 1 : mode == 1 ? 1 + lng(wrng() % 1000000) : lng(wrng() % 11) - 5;
            h.addEdge(e.u, e.v, w);}
        if (mode == 2 && !h.edges.empty() && std::ranges::max(h.edges, {}, &GraphEdge::w).w == 0) {
            h.edges[0].w = 3;
            for (auto &a : h.arcs) {
                if (a.id == 0) { a.w = 3; }}}
        return h;}
    // weightedClustering and weightedDirectedClustering on all three weight modes, Graph and CsrGraph; unit weights exactly equal the unweighted values.
    // The checked and ASan builds run one rotating weight mode per graph.
    void weightedChecks(const Graph &g) {
        for (int t = 0, first = int(wrng() % 3); t < (SHRINK > 1 ? 1 : 3); ++t) {
            int wm = (first + t) % 3;
            Graph h = weighted(g, wm);
            auto e = h.directed ? nxWeightedDirected(h) : nxWeighted(h);
            if (h.directed) {
                closeTo(weightedDirectedClustering(h), e, "weightedDirectedClustering (NetworkX weighted directed clustering)");
                closeTo(weightedDirectedClustering(CsrGraph(h)), e, "weightedDirectedClustering CsrGraph");
                if (wm == 0) { same(weightedDirectedClustering(h), directedClustering(h), "unit weights: weightedDirectedClustering = directedClustering"); }}
            else {
                closeTo(weightedClustering(h), e, "weightedClustering (NetworkX weighted clustering)");
                closeTo(weightedClustering(CsrGraph(h)), e, "weightedClustering CsrGraph");
                if (wm == 0) { same(weightedClustering(h), localClustering(h), "unit weights: weightedClustering = localClustering"); }}}}
    void verifyBoth(const Graph &g, const Expect &x, std::mt19937_64 &rng) {
        verify(g, g, x, rng);
        verify(CsrGraph(g), g, x, rng);
        weightedChecks(g);}

    // NetworkX triad_type rules: mutual, asymmetric and null dyad counts, then D/U/C/T from the asymmetric arcs.
    string structural(const Arcs &arc) {
        int mutual = 0, asym = 0, out[3] = {}, in[3] = {}, third = 0;
        for (int x = 0; x < 3; ++x) {
            for (int y = x + 1; y < 3; ++y) {
                if (arc[x][y] && arc[y][x]) { ++mutual; third = 3 - x - y; }
                else if (arc[x][y] || arc[y][x]) {
                    int a = arc[x][y] ? x : y;
                    ++asym; ++out[a]; ++in[x + y - a];}}}
        string res = std::to_string(mutual) + std::to_string(asym) + std::to_string(3 - mutual - asym);
        bool two_out = std::ranges::count(out, 2) > 0, two_in = std::ranges::count(in, 2) > 0;
        if (res == "021") { res += two_out ? "D" : two_in ? "U" : "C"; }
        else if (res == "111") { res += out[third] ? "D" : "U"; }
        else if (res == "030") { res += std::ranges::count(out, 1) == 3 ? "C" : "T"; }
        else if (res == "120") { res += out[third] == 2 ? "D" : in[third] == 2 ? "U" : "C"; }
        return res;}
    template<class F>
    int tricode(F arc, int a, int b, int c) { return arc(a, b) + 2 * arc(b, a) + 4 * arc(a, c) + 8 * arc(c, a) + 16 * arc(b, c) + 32 * arc(c, b); }
    // Census by classifying every vertex triple through TRICODES.
    Census censusBrute(const Graph &g) {
        int n = g.n;
        vector<vector<int>> arc(n, vector<int>(n));
        for (auto e : g.edges) {
            if (e.u != e.v) { arc[e.u][e.v] = 1; }}
        auto has = [&](int a, int b) { return arc[a][b]; };
        Census res{};
        for (int a = 0; a < n; ++a) {
            for (int b = a + 1; b < n; ++b) {
                for (int c = b + 1; c < n; ++c) { ++res[TRICODES[tricode(has, a, b, c)] - 1]; }}}
        return res;}
    // Census in O(sum of deg^2 * log): one-dyad triads per dyad from the neighbourhood union, others by centre and neighbour pairs.
    Census censusFast(const Graph &g) {
        int n = g.n;
        Lists l(g);
        vector<vector<int>> out(n);
        for (auto e : g.edges) {
            if (e.u != e.v) { out[e.u].push_back(e.v); }}
        for (auto &o : out) { sort(o.begin(), o.end()); }
        auto has = [&](int a, int b) { return int(contains(out[a], b)); };
        Census res{};
        for (int u = 0; u < n; ++u) {
            for (int v : l.nb[u]) {
                if (v < u) { continue; }
                lng common = 0;
                for (size_t i = 0, j = 0; i < l.nb[u].size() && j < l.nb[v].size();) {
                    if (l.nb[u][i] == l.nb[v][j]) { ++common; ++i; ++j; }
                    else if (l.nb[u][i] < l.nb[v][j]) { ++i; }
                    else { ++j; }}
                res[has(u, v) && has(v, u) ? 2 : 1] += n - (lng(l.nb[u].size()) + lng(l.nb[v].size()) - common);}}
        for (int c = 0; c < n; ++c) {
            const auto &nc = l.nb[c];
            for (int i = 0; i < int(nc.size()); ++i) {
                for (int j = i + 1; j < int(nc.size()); ++j) {
                    if (!l.mult(nc[i], nc[j]) || c < nc[i]) { ++res[TRICODES[tricode(has, c, nc[i], nc[j])] - 1]; }}}}
        res[0] = choose(n, 3);
        for (int t = 1; t < 16; ++t) { res[0] -= res[t]; }
        return res;}
    // NetworkX _directed_triangles_and_degree_iter and clustering transliterated: loops dropped, parallel arcs merged.
    vector<double> nxDirected(const Graph &g) {
        int n = g.n;
        vector<vector<int>> pred(n), succ(n);
        for (auto e : g.edges) {
            if (e.u != e.v) { succ[e.u].push_back(e.v); pred[e.v].push_back(e.u); }}
        for (auto *l : {&pred, &succ}) {
            for (auto &v : *l) {
                sort(v.begin(), v.end());
                v.erase(unique(v.begin(), v.end()), v.end());}}
        vector<char> ip(n), is(n);
        vector<double> res(n);
        for (int i = 0; i < n; ++i) {
            for (int j : pred[i]) { ip[j] = 1; }
            for (int j : succ[i]) { is[j] = 1; }
            lng t = 0, db = 0, dt = lng(pred[i].size() + succ[i].size());
            // For j in chain(ipreds, isuccs): |ipreds & jpreds| + |ipreds & jsuccs| + |isuccs & jpreds| + |isuccs & jsuccs|.
            for (const auto *js : {&pred[i], &succ[i]}) {
                for (int j : *js) {
                    for (int k : pred[j]) { t += ip[k] + is[k]; }
                    for (int k : succ[j]) { t += ip[k] + is[k]; }}}
            for (int j : succ[i]) { db += ip[j]; }
            if (t) { res[i] = double(t) / double((dt * (dt - 1) - 2 * db) * 2); }
            for (int j : pred[i]) { ip[j] = 0; }
            for (int j : succ[i]) { is[j] = 0; }}
        return res;}
    void sameDirected(const Graph &g, const vector<double> &e) {
        same(directedClustering(g), e, "directedClustering (NetworkX directed clustering)");
        same(directedClustering(CsrGraph(g)), e, "directedClustering CsrGraph");
        double avg = 0, nz = 0;
        int k = 0;
        for (double y : e) { avg += y; }
        for (double y : e) {
            if (std::abs(y) > 0) { nz += y; ++k; }}
        same(averageClustering(g), g.n ? avg / double(g.n) : 0.0, "averageClustering directed (NetworkX average_clustering)");
        same(averageClustering(CsrGraph(g), false), k ? nz / double(k) : 0.0, "averageClustering directed count_zeros = false");
        weightedChecks(g);}
    void sameCensus(const Graph &g, const Census &e, const char *op) {
        ++cases;
        same(triadicCensus(g), e, op);
        same(triadicCensus(CsrGraph(g)), e, op);
        same(accumulate(e.begin(), e.end(), lng(0)), choose(g.n, 3), "census sums to C(n, 3)");}
    // Both arcs per edge: only 003, 102, 201 and 300 occur, 300 = triangles, 201 = open wedges.
    void symmetrised(const Graph &g, const Expect &x) {
        Graph d(g.n, true);
        for (auto e : g.edges) { d.addEdge(e.u, e.v); d.addEdge(e.v, e.u); }
        Lists l(g);
        Census e = censusBrute(d);
        lng wedges = 0;
        for (const auto &nb : l.nb) { wedges += choose(lng(nb.size()), 2); }
        for (int t : {1, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14}) { same(e[t], 0, "symmetrised census has only 003, 102, 201, 300"); }
        same(e[15], x.t, "symmetrised census 300 = triangles"); same(e[10], wedges - 3 * x.t, "symmetrised census 201 = open wedges");
        sameCensus(d, e, "triadicCensus symmetrised digraph");
        auto dc = nxDirected(d);
        sameDirected(d, dc);
        same(dc, x.lc, "symmetrised directed clustering equals local clustering (NetworkX oracle)");}

    // Header results under a vertex permutation, shuffled edge order and swapped endpoints.
    void relabel(const Graph &g, std::mt19937_64 &rng) {
        int n = g.n, m = int(g.edges.size());
        vector<int> pv(n), pe(m);
        iota(pv.begin(), pv.end(), 0); iota(pe.begin(), pe.end(), 0);
        std::shuffle(pv.begin(), pv.end(), rng); std::shuffle(pe.begin(), pe.end(), rng);
        Graph h(n);
        for (int i = 0; i < m; ++i) {
            auto e = g.edges[pe[i]];
            if (rng() & 1) { std::swap(e.u, e.v); }
            h.addEdge(pv[e.u], pv[e.v]);}
        vector<Triple> tg, th;
        enumerateTriangles(g, [&](int a, int b, int c) {
            Triple t{pv[a], pv[b], pv[c]};
            sort(t.begin(), t.end()); tg.push_back(t);});
        enumerateTriangles(h, [&](int a, int b, int c) {
            Triple t{a, b, c};
            sort(t.begin(), t.end()); th.push_back(t);});
        sort(tg.begin(), tg.end()); sort(th.begin(), th.end());
        same(th, tg, "relabel enumerateTriangles");
        auto te = countTrianglesPerEdge(g), teh = countTrianglesPerEdge(h), qe = countC4PerEdge(g), qeh = countC4PerEdge(h);
        auto vg = countC3C4PerVertex(g), vh = countC3C4PerVertex(h);
        auto pg = countPathsP3P4P5(g), ph = countPathsP3P4P5(h);
        for (int i = 0; i < m; ++i) {
            same(teh[i], te[pe[i]], "relabel countTrianglesPerEdge"); same(qeh[i], qe[pe[i]], "relabel countC4PerEdge");}
        for (int v = 0; v < n; ++v) {
            same(vh.first[pv[v]], vg.first[v], "relabel countC3C4PerVertex triangles"); same(vh.second[pv[v]], vg.second[v], "relabel countC3C4PerVertex 4-cycles");
            for (int i = 0; i < 3; ++i) { same(ph[i][pv[v]], pg[i][v], "relabel countPathsP3P4P5"); }}
        same(countTriangles(h), countTriangles(g), "relabel countTriangles"); same(countC4(h), countC4(g), "relabel countC4");
        same(countK4(h), countK4(g), "relabel countK4"); same(findC4(h).arcs.empty(), findC4(g).arcs.empty(), "relabel findC4 existence");
        vector<Quad> cg, ch, kg, kh;
        enumerateC4(g, [&](int a, int b, int c, int e) { cg.push_back(canon({pv[a], pv[b], pv[c], pv[e]})); });
        enumerateC4(h, [&](int a, int b, int c, int e) { ch.push_back(canon({a, b, c, e})); });
        enumerateK4(g, [&](int a, int b, int c, int e) {
            Quad q{pv[a], pv[b], pv[c], pv[e]};
            sort(q.begin(), q.end()); kg.push_back(q);});
        enumerateK4(h, [&](int a, int b, int c, int e) {
            Quad q{a, b, c, e};
            sort(q.begin(), q.end()); kh.push_back(q);});
        sort(cg.begin(), cg.end()); sort(ch.begin(), ch.end()); sort(kg.begin(), kg.end()); sort(kh.begin(), kh.end());
        same(ch, cg, "relabel enumerateC4"); same(kh, kg, "relabel enumerateK4");
        auto lg = localClustering(g), lh = localClustering(h), sg = squareClustering(g), sh = squareClustering(h);
        auto gg = generalizedDegree(g), gh = generalizedDegree(h);
        for (int v = 0; v < n; ++v) {
            same(lh[pv[v]], lg[v], "relabel localClustering"); same(sh[pv[v]], sg[v], "relabel squareClustering"); same(gh[pv[v]], gg[v], "relabel generalizedDegree");}
        same(transitivity(h), transitivity(g), "relabel transitivity");}
    // k parallel copies of every edge of a simple graph plus loops: simple results stay, per-edge results scale by k^2 and k^3.
    void multiplicity(const Graph &g, std::mt19937_64 &rng) {
        int k = 2 + rnd(rng, 2), m = int(g.edges.size());
        Graph h(g.n);
        for (auto e : g.edges) {
            for (int j = 0; j < k; ++j) { h.addEdge(j % 2 ? e.v : e.u, j % 2 ? e.u : e.v); }}
        for (int j = 0; j < 3 && g.n; ++j) {
            int v = rnd(rng, g.n);
            h.addEdge(v, v);}
        auto te = countTrianglesPerEdge(g), teh = countTrianglesPerEdge(h), qe = countC4PerEdge(g), qeh = countC4PerEdge(h);
        for (int i = 0; i < int(teh.size()); ++i) {
            bool copy = i < m * k;
            same(teh[i], copy ? lng(k) * k * te[i / k] : 0, "multiplicity countTrianglesPerEdge scales by k^2");
            same(qeh[i], copy ? lng(k) * k * k * qe[i / k] : 0, "multiplicity countC4PerEdge scales by k^3");}
        vector<Triple> tg, th;
        enumerateTriangles(g, [&](int a, int b, int c) {
            Triple t{a, b, c};
            sort(t.begin(), t.end()); tg.push_back(t);});
        enumerateTriangles(h, [&](int a, int b, int c) {
            Triple t{a, b, c};
            sort(t.begin(), t.end()); th.push_back(t);});
        sort(tg.begin(), tg.end()); sort(th.begin(), th.end());
        same(th, tg, "multiplicity enumerateTriangles");
        same(countTriangles(h), countTriangles(g), "multiplicity countTriangles"); same(countC4(h), countC4(g), "multiplicity countC4");
        same(countK4(h), countK4(g), "multiplicity countK4"); same(countC3C4PerVertex(h), countC3C4PerVertex(g), "multiplicity countC3C4PerVertex");
        same(countPathsP3P4P5(h), countPathsP3P4P5(g), "multiplicity countPathsP3P4P5");
        same(findC4(h).arcs.empty(), findC4(g).arcs.empty(), "multiplicity findC4 existence");
        vector<Quad> cg, ch, kg, kh;
        enumerateC4(g, [&](int a, int b, int c, int e) { cg.push_back(canon({a, b, c, e})); });
        enumerateC4(h, [&](int a, int b, int c, int e) { ch.push_back(canon({a, b, c, e})); });
        enumerateK4(g, [&](int a, int b, int c, int e) {
            Quad q{a, b, c, e};
            sort(q.begin(), q.end()); kg.push_back(q);});
        enumerateK4(h, [&](int a, int b, int c, int e) {
            Quad q{a, b, c, e};
            sort(q.begin(), q.end()); kh.push_back(q);});
        sort(cg.begin(), cg.end()); sort(ch.begin(), ch.end()); sort(kg.begin(), kg.end()); sort(kh.begin(), kh.end());
        same(ch, cg, "multiplicity enumerateC4"); same(kh, kg, "multiplicity enumerateK4");
        same(localClustering(h), localClustering(g), "multiplicity localClustering"); same(squareClustering(h), squareClustering(g), "multiplicity squareClustering");
        same(transitivity(h), transitivity(g), "multiplicity transitivity"); same(generalizedDegree(h), generalizedDegree(g), "multiplicity generalizedDegree");
        vector<lng> x(g.n);
        for (auto &y : x) { y = lng(rng() % 2001) - 1000; }
        same(triangleProductSum(h, x), triangleProductSum(g, x), "multiplicity triangleProductSum");}

    vector<lng> rep(int n, lng v) { return vector<lng>(n, v); }
    // Hand graphs with values derived on paper: validates brute force, edge-subset enumeration and the codegree oracle.
    void handCases(std::mt19937_64 &rng) {
        struct Hand { const char *name; int n; vector<Edge> es; lng t, c4, k4; vector<lng> te, qe, v3, v4, p3, p4, p5; };
        vector<Edge> petersen;
        for (int i = 0; i < 5; ++i) { petersen.push_back({i, (i + 1) % 5}); petersen.push_back({i, i + 5}); petersen.push_back({i + 5, (i + 2) % 5 + 5}); }
        vector<Edge> k4{{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}}, k4x2;
        for (auto e : k4) { k4x2.push_back(e); k4x2.push_back({e[1], e[0]}); }
        const vector<Hand> hands = {
            {"K4", 4, k4, 4, 3, 1, rep(6, 2), rep(6, 2), rep(4, 3), rep(4, 3), rep(4, 6), rep(4, 6), rep(4, 0)},
            {"K4 doubled", 4, k4x2, 4, 3, 1, rep(12, 8), rep(12, 16), rep(4, 3), rep(4, 3), rep(4, 6), rep(4, 6), rep(4, 0)},
            {"C4", 4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}}, 0, 1, 0, rep(4, 0), rep(4, 1), rep(4, 0), rep(4, 1), rep(4, 2), rep(4, 2), rep(4, 0)},
            {"triangle with a doubled edge", 3, {{0, 1}, {1, 0}, {1, 2}, {2, 0}}, 1, 0, 0, {1, 1, 2, 2}, rep(4, 0), rep(3, 1), rep(3, 0), rep(3, 2), rep(3, 0), rep(3, 0)},
            {"square with a doubled edge", 4, {{0, 1}, {0, 1}, {1, 2}, {2, 3}, {3, 0}}, 0, 1, 0, rep(5, 0), {1, 1, 2, 2, 2}, rep(4, 0), rep(4, 1), rep(4, 2), rep(4, 2), rep(4, 0)},
            {"two digons and a bridge", 4, {{0, 1}, {0, 1}, {2, 3}, {3, 2}, {1, 2}}, 0, 0, 0, rep(5, 0), rep(5, 0), rep(4, 0), rep(4, 0), rep(4, 1), {1, 0, 0, 1}, rep(4, 0)},
            {"K2,3", 5, {{0, 2}, {0, 3}, {0, 4}, {1, 2}, {1, 3}, {1, 4}}, 0, 3, 0, rep(6, 0), rep(6, 2), rep(5, 0), {3, 3, 2, 2, 2}, {3, 3, 4, 4, 4}, {6, 6, 4, 4, 4}, {0, 0, 4, 4, 4}},
            {"bowtie", 5, {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {3, 4}, {4, 0}}, 2, 0, 0, rep(6, 1), rep(6, 0), {2, 1, 1, 1, 1}, rep(5, 0), rep(5, 4), {0, 4, 4, 4, 4}, {0, 2, 2, 2, 2}},
            {"Petersen", 10, petersen, 0, 0, 0, rep(15, 0), rep(15, 0), rep(10, 0), rep(10, 0), rep(10, 6), rep(10, 12), rep(10, 24)},
            {"loops only", 3, {{0, 0}, {1, 1}, {1, 1}}, 0, 0, 0, rep(3, 0), rep(3, 0), rep(3, 0), rep(3, 0), rep(3, 0), rep(3, 0), rep(3, 0)},
            {"empty n=0", 0, {}, 0, 0, 0, {}, {}, {}, {}, {}, {}, {}},
            {"single vertex", 1, {}, 0, 0, 0, {}, {}, {0}, {0}, {0}, {0}, {0}},
            {"single edge", 2, {{1, 0}}, 0, 0, 0, {0}, {0}, rep(2, 0), rep(2, 0), rep(2, 0), rep(2, 0), rep(2, 0)}};
        for (const auto &hc : hands) {
            Graph g = build(hc.n, hc.es);
            context = string("hand case ") + hc.name + show(g);
            Expect b = brute(g);
            same(b.t, hc.t, "brute triangles"); same(b.c4, hc.c4, "brute 4-cycles"); same(b.k4, hc.k4, "brute 4-cliques");
            same(b.tri_edge, hc.te, "brute per-edge triangles"); same(b.c4_edge, hc.qe, "brute per-edge 4-cycles");
            same(b.c3, hc.v3, "brute per-vertex triangles"); same(b.c4v, hc.v4, "brute per-vertex 4-cycles");
            same(b.paths, array<vector<lng>, 3>{hc.p3, hc.p4, hc.p5}, "brute paths");
            auto [t, q] = subsets(g);
            same(t, hc.te, "3-edge subsets per-edge triangles"); same(q, hc.qe, "4-edge subsets per-edge 4-cycles");
            vector<int> all(hc.n);
            iota(all.begin(), all.end(), 0);
            sameExpect(fast(g, all), b, "codegree oracle versus brute force");
            verifyBoth(g, b, rng);}
        // NetworkX values on paper: clustering, square_clustering, transitivity, generalized_degree.
        struct NxHand { int index; vector<double> lc, sq; double tr; vector<vector<int>> gd; };
        const vector<NxHand> nx = {
            {0, vector<double>(4, 1), vector<double>(4, 1), 1, vector<vector<int>>(4, {0, 0, 3})},
            {2, vector<double>(4, 0), vector<double>(4, 1), 0, vector<vector<int>>(4, {2})},
            {6, vector<double>(5, 0), vector<double>(5, 1), 0, {{3}, {3}, {2}, {2}, {2}}},
            {7, {double(4) / double(12), 1, 1, 1, 1}, vector<double>(5, 0), double(12) / double(20), {{0, 4}, {0, 2}, {0, 2}, {0, 2}, {0, 2}}},
            {8, vector<double>(10, 0), vector<double>(10, 0), 0, vector<vector<int>>(10, {3})},
            {9, vector<double>(3, 0), vector<double>(3, 0), 0, vector<vector<int>>(3)}};
        for (const auto &h : nx) {
            Graph g = build(hands[h.index].n, hands[h.index].es);
            context = string("hand case ") + hands[h.index].name + show(g);
            Expect b = brute(g);
            same(b.lc, h.lc, "NetworkX clustering oracle"); same(b.sq, h.sq, "NetworkX square_clustering oracle");
            same(b.tr, h.tr, "NetworkX transitivity oracle"); same(b.gd, h.gd, "NetworkX generalized_degree oracle");}
        context = "";
        cout << "PASS hand cases " << hands.size() << " (brute force, edge subsets and codegree oracle validated)" << elapsed() << '\n';}
    // Triad table: TRICODES against the NetworkX triad_type rules on all 64 labelled triads, then the 16 hand shapes in every labelling.
    void triadCases() {
        context = "triad table";
        array<int, 16> orbit{};
        const int bit[6][2] = {{0, 1}, {1, 0}, {0, 2}, {2, 0}, {1, 2}, {2, 1}};
        for (int code = 0; code < 64; ++code) {
            Arcs arc{};
            for (int b = 0; b < 6; ++b) { arc[bit[b][0]][bit[b][1]] = code >> b & 1; }
            at = " tricode=" + std::to_string(code);
            same(structural(arc), NAMES[TRICODES[code] - 1], "TRICODES versus NetworkX triad_type rules");
            ++orbit[TRICODES[code] - 1];}
        at = "";
        same(orbit, ORBITS, "labelled triads per type");
        for (int t = 0; t < 16; ++t) {
            for (const auto &perm : PERMS) {
                vector<Edge> es;
                Arcs arc{};
                for (auto [a, b] : SHAPES[t]) { es.push_back({perm[a], perm[b]}); arc[perm[a]][perm[b]] = true; }
                Graph g = build(3, es, true);
                context = "triad shape " + NAMES[t] + show(g);
                same(structural(arc), NAMES[t], "hand shape type by triad_type rules");
                Census e{};
                e[t] = 1;
                same(censusBrute(g), e, "brute census of a hand shape"); same(censusFast(g), e, "fast census of a hand shape");
                sameCensus(g, e, "triadicCensus hand shape");
                if (!es.empty()) { es.push_back(es[0]); }
                es.push_back({perm[1], perm[1]});
                sameCensus(build(3, es, true), e, "triadicCensus hand shape with a repeated arc and a loop");}}
        Graph nx = build(4, {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 0}, {3, 1}}, true);
        Census e{};
        e[8] = 2; e[9] = 2;
        context = "NetworkX triadic_census docstring example";
        same(censusBrute(nx), e, "brute census of the docstring example");
        sameCensus(nx, e, "triadicCensus docstring example");
        // Directed clustering on paper: 3-cycle and transitive triangle 0.5 everywhere, complete symmetric triangle 1, mutual edge 0.
        const vector<pair<vector<Edge>, vector<double>>> dc = {{{{0, 1}, {1, 2}, {2, 0}}, {0.5, 0.5, 0.5}}, {{{0, 1}, {0, 2}, {1, 2}}, {0.5, 0.5, 0.5}},
            {{{0, 1}, {1, 0}, {1, 2}, {2, 1}, {0, 2}, {2, 0}}, {1, 1, 1}}, {{{0, 1}, {1, 0}, {1, 1}}, {0, 0, 0}}};
        for (const auto &[es, want] : dc) {
            Graph g = build(3, es, true);
            context = "directed clustering hand case" + show(g);
            same(nxDirected(g), want, "NetworkX directed clustering oracle");
            sameDirected(g, want);}
        context = "";
        cout << "PASS triad table 64 tricodes, 16 shapes x 6 labellings, NetworkX docstring example" << elapsed() << '\n';}

    void exhaustive(std::mt19937_64 &rng) {
        int top = quick() || SHRINK > 1 ? 5 : 6, graphs = 0;
        for (int n = 0; n <= top; ++n) {
            vector<Edge> pairs;
            for (int u = 0; u < n; ++u) {
                for (int v = u + 1; v < n; ++v) { pairs.push_back({u, v}); }}
            for (int mask = 0; mask < 1 << pairs.size(); ++mask) {
                vector<Edge> es;
                for (int i = 0; i < int(pairs.size()); ++i) {
                    if (mask >> i & 1) { es.push_back(rng() & 1 ? pairs[i] : Edge{pairs[i][1], pairs[i][0]}); }}
                Graph g = build(n, es);
                context = "exhaustive" + show(g) + " seed=" + std::to_string(seed);
                verifyBoth(g, brute(g), rng);
                ++graphs;}}
        int dtop = quick() ? 3 : 4, digraphs = 0;
        for (int n = 0; n <= dtop; ++n) {
            vector<Edge> arcs;
            for (int u = 0; u < n; ++u) {
                for (int v = 0; v < n; ++v) {
                    if (u != v) { arcs.push_back({u, v}); }}}
            for (int mask = 0; mask < 1 << arcs.size(); ++mask) {
                vector<Edge> es;
                for (int i = 0; i < int(arcs.size()); ++i) {
                    if (mask >> i & 1) { es.push_back(arcs[i]); }}
                Graph g = build(n, es, true);
                context = "exhaustive digraph" + show(g);
                sameCensus(g, censusBrute(g), "triadicCensus versus brute force");
                sameDirected(g, nxDirected(g));
                ++digraphs;}}
        context = "";
        cout << "PASS exhaustive " << graphs << " simple graphs n<=" << top << ", " << digraphs << " digraphs n<=" << dtop << elapsed() << '\n';}
    // Tiny multigraphs with loops and parallel edges: edge-subset enumeration validates the brute force, which checks the header.
    void randomSmall(std::mt19937_64 &rng) {
        int rounds = pick(300, 3000, 20000);
        for (int r = 0; r < rounds; ++r) {
            int n = rnd(rng, 8), m = n ? rnd(rng, 13) : 0;
            Graph g = build(n, randomEdges(rng, n, m, 15, 25));
            context = "random small round=" + std::to_string(r) + " seed=" + std::to_string(seed) + show(g);
            Expect b = brute(g);
            vector<Triple> tris;
            auto [t, q] = subsets(g, &tris);
            same(tris, b.tedges, "edge-triangle list versus 3-edge subsets");
            same(t, b.tri_edge, "brute per-edge triangles versus 3-edge subsets"); same(q, b.c4_edge, "brute per-edge 4-cycles versus 4-edge subsets");
            if (b.simple) {
                same(accumulate(t.begin(), t.end(), lng(0)), 3 * b.t, "3-edge subsets versus vertex triples");
                same(accumulate(q.begin(), q.end(), lng(0)), 4 * b.c4, "4-edge subsets versus vertex quadruples");}
            verifyBoth(g, b, rng);
            Graph d = build(n, randomEdges(rng, n, m, 15, 40), true);
            context = "random small digraph round=" + std::to_string(r) + " seed=" + std::to_string(seed) + show(d);
            Census e = censusBrute(d);
            same(censusFast(d), e, "fast census versus brute force");
            sameCensus(d, e, "triadicCensus versus brute force");
            sameDirected(d, nxDirected(d));}
        context = "";
        cout << "PASS random small " << rounds << " multigraphs n<=7 m<=12 (loops, parallel edges) and digraphs" << elapsed() << '\n';}
    // Medium graphs of every density: both oracles, both graph types, repeated calls, relabelling, multiplicity and symmetrised census.
    void medium(std::mt19937_64 &rng) {
        int rounds = pick(40, 400, 2000);
        const double P[] = {0.05, 0.15, 0.3, 0.5, 0.8, 1.0};
        for (int r = 0; r < rounds; ++r) {
            double p = P[rnd(rng, 6)];
            int n = rnd(rng, p >= 0.5 ? 19 : 41);
            auto simple = densityEdges(rng, n, p), es = simple;
            for (int j = r % 2 && n ? rnd(rng, 8) : 0; j > 0; --j) {
                int v = rnd(rng, n);
                es.push_back(rng() % 3 || es.empty() ? Edge{v, v} : es[rnd(rng, int(es.size()))]);}
            std::shuffle(es.begin(), es.end(), rng);
            Graph g = build(n, es);
            context = "medium round=" + std::to_string(r) + " p=" + std::to_string(p) + " seed=" + std::to_string(seed) + show(g);
            Expect b = brute(g);
            sameExpect(fast(g, b.sample), b, "codegree oracle versus brute force");
            verifyBoth(g, b, rng);
            if (r % 4 == 0) { verify(g, g, b, rng); }
            relabel(g, rng);
            multiplicity(build(n, simple), rng);
            symmetrised(g, b);
            Graph d = build(n, randomEdges(rng, n, lng(p * n * n), 2, 30), true);
            context = "medium digraph round=" + std::to_string(r) + " seed=" + std::to_string(seed) + show(d);
            Census e = censusBrute(d);
            same(censusFast(d), e, "fast census versus brute force");
            sameCensus(d, e, "triadicCensus versus brute force");
            sameDirected(d, nxDirected(d));}
        context = "";
        cout << "PASS medium " << rounds << " graphs n<=40 density 0.05..1 with relabelling, multiplicity and symmetrised census" << elapsed() << '\n';}

    Expect closedForm(int n, int m) {
        Expect x;
        x.listed = false; x.tri_edge.assign(m, 0); x.c4_edge.assign(m, 0); x.c3.assign(n, 0); x.c4v.assign(n, 0);
        x.sample.resize(n);
        iota(x.sample.begin(), x.sample.end(), 0);
        for (auto &p : x.paths) { p.assign(n, 0); }
        return x;}
    Expect completeForm(int n) {
        Expect x = closedForm(n, int(choose(n, 2)));
        x.t = choose(n, 3); x.c4 = 3 * choose(n, 4); x.k4 = choose(n, 4);
        x.tri_edge.assign(x.tri_edge.size(), n - 2); x.c4_edge.assign(x.c4_edge.size(), lng(n - 2) * (n - 3));
        x.c3.assign(n, choose(n - 1, 2)); x.c4v.assign(n, 3 * choose(n - 1, 3));
        for (int k = 0; k < 3; ++k) { x.paths[k].assign(n, falling(n - 1, k + 2)); }
        x.deg.assign(n, n - 1); x.lc.assign(n, n >= 3); x.sq.assign(n, n >= 4); x.tr = n >= 3;
        vector<int> row(size_t(max(n - 1, 1)));
        row.back() = n - 1;
        x.gd.assign(n, n >= 2 ? row : vector<int>{});
        return x;}
    // Closed forms: complete, complete bipartite, cycles, paths, stars (also with doubled spokes and loops) and grids.
    void closedForms(std::mt19937_64 &rng) {
        vector<int> ks = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 20, 40};
        if (!CHECKED) { ks.insert(ks.end(), {63, 64, 65, 66}); }
        if (SHRINK == 1) { ks.insert(ks.end(), {127, 128, 129, 130}); }
        if (!quick() && SHRINK == 1) { ks.push_back(200); }
        for (int n : ks) {
            Graph g = build(n, completeEdges(rng, n));
            context = "complete K" + std::to_string(n);
            Expect x = completeForm(n), y;
            addNx(y, Lists(g));
            same(y.lc, x.lc, "K_n NetworkX clustering = 1"); same(y.sq, x.sq, "K_n NetworkX square_clustering");
            same(y.tr, x.tr, "K_n NetworkX transitivity = 1"); same(y.gd, x.gd, "K_n NetworkX generalized_degree");
            verifyBoth(g, x, rng);}
        vector<array<int, 2>> bip = {{1, 1}, {2, 2}, {2, 3}, {3, 3}, {1, 50}, {7, 9}, {40, 60}};
        bip.push_back({2, pick(2000, 50000, 100000) / (CHECKED ? 25 : SHRINK)});
        for (auto [a, b] : bip) {
            vector<Edge> es;
            for (int u = 0; u < a; ++u) {
                for (int v = a; v < a + b; ++v) { es.push_back(rng() & 1 ? Edge{u, v} : Edge{v, u}); }}
            std::shuffle(es.begin(), es.end(), rng);
            Graph g = build(a + b, es);
            Expect x = closedForm(a + b, a * b);
            x.c4 = choose(a, 2) * choose(b, 2);
            x.c4_edge.assign(es.size(), lng(a - 1) * (b - 1));
            for (int v = 0; v < a + b; ++v) {
                lng s = v < a ? a : b, o = v < a ? b : a;
                x.c4v[v] = (s - 1) * choose(o, 2);
                x.paths[0][v] = o * (s - 1); x.paths[1][v] = o * (s - 1) * (o - 1); x.paths[2][v] = o * (s - 1) * (o - 1) * max<lng>(s - 2, 0);}
            x.deg.resize(a + b); x.lc.assign(a + b, 0); x.sq.assign(a + b, a >= 2 && b >= 2); x.tr = 0; x.gd.resize(a + b);
            for (int v = 0; v < a + b; ++v) { x.deg[v] = v < a ? b : a; x.gd[v] = {x.deg[v]}; }
            context = "complete bipartite K" + std::to_string(a) + "," + std::to_string(b);
            if (lng(a) * b * (a + b) <= 50000000) {
                Expect y;
                addNx(y, Lists(g));
                same(y.lc, x.lc, "K_a,b NetworkX clustering = 0"); same(y.sq, x.sq, "K_a,b NetworkX square_clustering = 1 for a, b >= 2");
                same(y.tr, x.tr, "K_a,b NetworkX transitivity = 0"); same(y.gd, x.gd, "K_a,b NetworkX generalized_degree");}
            verifyBoth(g, x, rng);}
        int big = pick(3000, 200000, 300000) / (CHECKED ? 50 : SHRINK);
        for (int n : {3, 4, 5, 6, 7, big}) {
            vector<Edge> cyc, path;
            for (int i = 0; i < n; ++i) { cyc.push_back({i, (i + 1) % n}); }
            for (int i = 0; i + 1 < n; ++i) { path.push_back({i + 1, i}); }
            Expect x = closedForm(n, n);
            x.t = n == 3; x.c4 = n == 4; x.tri_edge.assign(n, n == 3); x.c4_edge.assign(n, n == 4); x.c3.assign(n, n == 3); x.c4v.assign(n, n == 4);
            for (int k = 0; k < 3; ++k) { x.paths[k].assign(n, k + 3 <= n ? 2 : 0); }
            context = "cycle C" + std::to_string(n);
            Graph gc = build(n, cyc), gp = build(n, path);
            addNx(x, Lists(gc));
            verifyBoth(gc, x, rng);
            Expect y = closedForm(n, n - 1);
            for (int i = 0; i < n; ++i) {
                for (int k = 0; k < 3; ++k) { y.paths[k][i] = (i >= k + 2) + (n - 1 - i >= k + 2); }}
            context = "path P" + std::to_string(n);
            addNx(y, Lists(gp));
            verifyBoth(gp, y, rng);}
        for (int leaves : {1, 2, 3, 10, big}) {
            int centre = rnd(rng, leaves + 1);
            vector<Edge> es, doubled;
            for (int v = 0; v <= leaves; ++v) {
                if (v != centre) { es.push_back({centre, v}); doubled.push_back({v, centre}); doubled.push_back({centre, v}); doubled.push_back({v, v}); }}
            Expect x = closedForm(leaves + 1, leaves), y = closedForm(leaves + 1, 3 * leaves);
            for (int v = 0; v <= leaves; ++v) { x.paths[0][v] = y.paths[0][v] = v == centre ? 0 : leaves - 1; }
            y.simple = false;
            Graph gs = build(leaves + 1, es), gdbl = build(leaves + 1, doubled);
            addNx(x, Lists(gs)); addNx(y, Lists(gdbl));
            same(x.sq, vector<double>(leaves + 1, 0), "star NetworkX square_clustering = 0"); same(x.tr, 0.0, "star NetworkX transitivity = 0");
            context = "star with " + std::to_string(leaves) + " leaves";
            verifyBoth(gs, x, rng);
            context = "star with doubled spokes and loops, " + std::to_string(leaves) + " leaves";
            verifyBoth(gdbl, y, rng);}
        for (auto [a, b] : vector<array<int, 2>>{{1, 1}, {1, 5}, {2, 2}, {3, 4}, {pick(50, 400, 500) / (CHECKED ? 8 : 1), pick(60, 500, 600) / (CHECKED ? 8 : 1)}}) {
            vector<Edge> es;
            for (int r = 0; r < a; ++r) {
                for (int c = 0; c < b; ++c) {
                    if (c + 1 < b) { es.push_back({r * b + c, r * b + c + 1}); }
                    if (r + 1 < a) { es.push_back({(r + 1) * b + c, r * b + c}); }}}
            Graph g = build(a * b, es);
            vector<int> sample;
            for (int i = 0; i < 50; ++i) { sample.push_back(rnd(rng, a * b)); }
            Expect x = fast(g, sample);
            same(x.c4, lng(a - 1) * (b - 1), "grid oracle 4-cycles = (a - 1)(b - 1)");
            for (int e = 0; e < int(es.size()); ++e) {
                int u = min(es[e][0], es[e][1]), r = u / b, c = u % b;
                same(x.c4_edge[e], es[e][0] < es[e][1] ? lng(r > 0) + (r + 1 < a) : lng(c > 0) + (c + 1 < b), "grid oracle per-edge 4-cycles");}
            context = "grid " + std::to_string(a) + "x" + std::to_string(b);
            verifyBoth(g, x, rng);}
        context = "";
        cout << "PASS closed forms K0..K" << ks.back() << ", K2," << bip.back()[1] << ", cycles, paths and stars up to n=" << big << ", grids" << elapsed() << '\n';}

    // Blocks of parallel edges, count[b] copies of pairs[b] with alternating orientation, storage reserved up front.
    Graph blockGraph(int n, const vector<Edge> &pairs, const vector<lng> &count) {
        Graph g(n);
        lng m = accumulate(count.begin(), count.end(), lng(0));
        vector<lng> deg(n);
        for (int b = 0; b < int(pairs.size()); ++b) { deg[pairs[b][0]] += count[b]; deg[pairs[b][1]] += count[b]; }
        g.edges.reserve(size_t(m)); g.arcs.reserve(size_t(2 * m));
        for (int v = 0; v < n; ++v) { g.adj[v].reserve(size_t(deg[v])); }
        for (int b = 0; b < int(pairs.size()); ++b) {
            for (lng i = 0; i < count[b]; ++i) { g.addEdge(pairs[b][i % 2], pairs[b][1 - i % 2]); }}
        return g;}
    // Per-edge counts against per-block closed forms computed in lll: below 2^63 when exact, else countC4PerEdge modulo 2^64.
    template<class G>
    void blockCheck(const G &g, const vector<lng> &count, const vector<lll> &te, const vector<lll> &qe, lng t, lng c4, bool exact) {
        auto tri = countTrianglesPerEdge(g);
        auto quad = countC4PerEdge(g);
        same(tri.size(), g.edges.size(), "countTrianglesPerEdge size"); same(quad.size(), g.edges.size(), "countC4PerEdge size");
        size_t e = 0;
        for (int b = 0; b < int(count.size()); ++b) {
            check(te[b] <= LLONG_MAX && (!exact || qe[b] <= LLONG_MAX), "closed form below 2^63");
            at = " block=" + std::to_string(b);
            for (lng i = 0; i < count[b]; ++i, ++e) {
                same(tri[e], lng(te[b]), "countTrianglesPerEdge near the domain edge");
                same(quad[e], lng(ulng(qe[b])), exact ? "countC4PerEdge near the domain edge" : "countC4PerEdge past the domain equals the count mod 2^64");}}
        at = "";
        same(countTriangles(g), t, "countTriangles of the block multigraph"); same(countC4(g), c4, "countC4 of the block multigraph");
        ++cases;}
    // Pair multiplicities near and past the documented m <= 6 * 10^6 domain of countC4PerEdge, on Graph then CsrGraph; ASan runs the first two cases at full size.
    void domainEdge() {
        lng s = CHECKED ? 1000 : 1, top = 0;
        auto run = [&](const string &name, int n, const vector<Edge> &pairs, const vector<lng> &count, const vector<lll> &te, const vector<lll> &qe, lng t, lng c4, bool exact = true) {
            Graph g = blockGraph(n, pairs, count);
            context = "domain edge " + name + show(g);
            blockCheck(g, count, te, qe, t, c4, exact);
            blockCheck(CsrGraph(g), count, te, qe, t, c4, exact);
            for (lll x : qe) { top = max(top, exact ? lng(x) : 0); }};
        // 4-cycle 0-1-2-3 with multiplicities a, b, c, d and a chord 0-2: one 4-cycle, triangles 0-1-2 and 0-2-3.
        lll a = 1, b = 1999998 / s, c = 2000000 / s, d = 2000000 / s;
        run("4-cycle (1, 1999998, 2e6, 2e6) plus chord", 4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 2}}, {lng(a), lng(b), lng(c), lng(d), 1},
            {b, a, d, c, a * b + c * d}, {b * c * d, a * c * d, a * b * d, a * b * c, 0}, 2, 1);
        // Past the domain: one test edge plus 2.1 * 10^6 copies of each other pair, m = 6300001, test edge k^3 > 2^63 wraps.
        lll w = 2100000 / s;
        check(CHECKED || w * w * w > LLONG_MAX, "past-the-domain case exceeds 2^63");
        run("4-cycle past the domain (1, 2.1e6, 2.1e6, 2.1e6)", 4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}}, {1, lng(w), lng(w), lng(w)},
            {0, 0, 0, 0}, {w * w * w, w * w, w * w, w * w}, 0, 1, false);
        s *= ASAN ? 4 : 1;
        lll k = (6000000 - 1) / 3 / s;
        run("4-cycle equal split ((m - 1) / 3 per pair, one test edge)", 4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}}, {1, lng(k), lng(k), lng(k)},
            {0, 0, 0, 0}, {k * k * k, k * k, k * k, k * k}, 0, 1);
        lll p = 1, q = 2999999 / s, r = 3000000 / s;
        run("triangle (1, 2999999, 3e6)", 3, {{0, 1}, {1, 2}, {2, 0}}, {lng(p), lng(q), lng(r)}, {q * r, p * r, p * q}, {0, 0, 0}, 1, 0);
        context = "";
        cout << "PASS domain edge m=" << 6000000 / (s / (ASAN ? 4 : 1)) << " per-edge C4 up to " << top << " (2^63 - 1 = " << LLONG_MAX << ")" << elapsed() << '\n';}

    // Triangles from vertex-id bitset rows, each triangle seen once per edge.
    lng bitsetOracle(const Graph &g) {
        int n = g.n;
        size_t words = (size_t(n) + 63) / 64;
        vector<ulng> row(size_t(n) * words);
        for (auto e : g.edges) {
            if (e.u != e.v) { row[e.u * words + size_t(e.v) / 64] |= ulng(1) << (e.v % 64); row[e.v * words + size_t(e.u) / 64] |= ulng(1) << (e.u % 64); }}
        lng res = 0;
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) {
                if (!(row[u * words + size_t(v) / 64] >> (v % 64) & 1)) { continue; }
                for (size_t k = 0; k < words; ++k) { res += std::popcount(row[u * words + k] & row[v * words + k]); }}}
        return res / 3;}
    // Triangles by marking the neighbours of each u and scanning the two-paths u < v < w.
    lng markCount(const Graph &g) {
        Lists l(g);
        vector<char> mark(g.n);
        lng res = 0;
        for (int u = 0; u < g.n; ++u) {
            for (int v : l.nb[u]) { mark[v] = 1; }
            for (int v : l.nb[u]) {
                for (int w : l.nb[v]) { res += v > u && w > v && mark[w]; }}
            for (int v : l.nb[u]) { mark[v] = 0; }}
        return res;}
    lng rows(lng k) { return k * ((k + 63) / 64); }
    int sparse_cases = 0, whole_cases = 0, suffix_cases = 0;
    // countTriangles (both graph types) and triangleCount against e; the sparse loop below denseStart plus the bitset suffix; with
    // sparse_direct, bitsetTriangles(h, r) for several r against the sparse loop's triangles whose lowest rank is at least r.
    int triangleKernels(const Graph &g, lng e, bool sparse_direct, std::mt19937_64 &rng) {
        sd::Ranked h(g);
        int n = h.n, r0 = sd::denseStart(h);
        check(0 <= r0 && r0 <= n && rows(n - r0) <= sd::DENSE_WORDS, "denseStart suffix inside DENSE_WORDS");
        (r0 == n ? sparse_cases : r0 == 0 ? whole_cases : suffix_cases) += 1;
        at = " r0=" + std::to_string(r0);
        same(countTriangles(g), e, "countTriangles");
        same(countTriangles(CsrGraph(g)), e, "countTriangles CsrGraph");
        same(sd::triangleCount(h), e, "triangleCount dispatcher");
        lng below = 0;
        sd::triangles(h, [&](int u, int, int, int, int, int) { below += 1; check(u < r0, "triangles(h, f, end) stops at end"); }, r0);
        same(below + (r0 < n ? sd::bitsetTriangles(h, r0) : 0), e, "sparse loop below denseStart plus bitsetTriangles from it");
        if (sparse_direct) {
            vector<lng> low(size_t(n) + 1);
            sd::triangles(h, [&](int u, int, int, int, int, int) { ++low[u]; });
            for (int u = n - 1; u >= 0; --u) { low[u] += low[u + 1]; }
            same(low[0], e, "sparse triangle loop");
            for (int r : {r0, 0, n / 2, n - 1, n, rnd(rng, n + 1), max(r0 - 1, 0), min(r0 + 1, n)}) {
                r = std::clamp(r, 0, n);
                if (rows(n - r) > sd::DENSE_WORDS) { continue; }
                at = " r0=" + std::to_string(r0) + " r=" + std::to_string(r);
                same(sd::bitsetTriangles(h, r), low[r], "bitsetTriangles(h, r) = triangles with lowest rank >= r");}}
        at = "";
        ++cases;
        return r0;}
    // Dense dispatch: complete and random dense graphs, dense cores among many low-degree vertices (whole graph, proper suffix) and a density sweep.
    void dense(std::mt19937_64 &rng) {
        vector<int> ks = {300};
        if (!quick() && !CHECKED) { ks.push_back(1000); }
        if (!quick() && SHRINK == 1) { ks.push_back(2000); }
        for (int n : ks) {
            context = "dense complete K" + std::to_string(n);
            triangleKernels(build(n, completeEdges(rng, n)), choose(n, 3), n <= 1000, rng);}
        vector<pair<int, double>> rs = {{300, 0.9}};
        if (!CHECKED) { rs.insert(rs.begin(), {300, 0.5}); }
        if (!quick() && !CHECKED) { rs.insert(rs.end(), {{1000, 0.5}, {1000, 0.75}}); }
        if (!quick() && SHRINK == 1) { rs.push_back({3000, 0.5}); }
        for (auto [n, p] : rs) {
            auto es = densityEdges(rng, n, p);
            for (int j = 0; j < int(es.size()) / 100; ++j) { es.push_back(es[rnd(rng, int(es.size()))]); }
            for (int j = 0; j < 10; ++j) {
                int v = rnd(rng, n);
                es.push_back({v, v});}
            std::shuffle(es.begin(), es.end(), rng);
            Graph g = build(n, es);
            context = "dense random n=" + std::to_string(n) + " p=" + std::to_string(p) + " seed=" + std::to_string(seed);
            triangleKernels(g, bitsetOracle(g), n <= 1000, rng);
            if (n == 300 && p == 0.5) { verifyBoth(g, fast(g, {}), rng); }}
        // A clique K_c on random vertices among extra vertices that are isolated, paired or carry 2 random edges each: the suffix must hold the core.
        vector<array<int, 3>> cores;
        for (int fill = 0; fill < 3; ++fill) {
            cores.push_back({200, 5000, fill});
            if (!CHECKED) { cores.insert(cores.end(), {{600, 13384, fill}, {600, 13385, fill}, {640, 15744, fill}, {640, 15745, fill}, {600, 100000, fill}}); }}
        if (!quick() && SHRINK == 1) { cores.push_back({3000, 100000, 0}); }
        const char *FILLS[3] = {"isolated", "paired", "random"};
        for (auto [c, extra, fill] : cores) {
            int n = c + extra;
            vector<int> pv(n);
            iota(pv.begin(), pv.end(), 0);
            std::shuffle(pv.begin(), pv.end(), rng);
            vector<Edge> es;
            if (fill == 2) { es = randomEdges(rng, n, 2 * lng(extra), 1, 5); }
            for (int i = c; fill == 1 && i + 1 < n; i += 2) { es.push_back({pv[i], pv[i + 1]}); }
            for (auto [u, v] : completeEdges(rng, c)) { es.push_back({pv[u], pv[v]}); }
            Graph g = build(n, es);
            context = "dense core K" + std::to_string(c) + " plus " + std::to_string(extra) + " " + FILLS[fill] + " vertices seed=" + std::to_string(seed);
            int r0 = triangleKernels(g, fill < 2 ? choose(c, 3) : markCount(g), c <= 640, rng);
            check(r0 < n, "denseStart picks a bitset suffix for a dense core");
            if (rows(n) > sd::DENSE_WORDS) { check(r0 > 0, "denseStart picks a proper suffix when the whole graph exceeds DENSE_WORDS"); }
            cout << "PASS dense core K" << c << " plus " << extra << " " << FILLS[fill] << " vertices: r0=" << r0 << " of n=" << n << elapsed() << '\n';}
        for (double p : {0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.4}) {
            int n = quick() || CHECKED ? 400 : 800;
            Graph g = build(n, densityEdges(rng, n, p));
            context = "density sweep n=" + std::to_string(n) + " p=" + std::to_string(p) + " seed=" + std::to_string(seed);
            triangleKernels(g, bitsetOracle(g), true, rng);}
        context = "";
        check(sparse_cases > 0 && whole_cases > 0 && suffix_cases > 0, "pure sparse (r0 = n), whole-graph (r0 = 0) and proper-suffix (0 < r0 < n) paths all reached");
        cout << "PASS dense dispatch: complete up to K" << ks.back() << ", random up to n=" << rs.back().first << ", dense cores up to K" << cores.back()[0] << "+" << cores.back()[1]
             << ", sweep; sparse=" << sparse_cases << " whole=" << whole_cases << " suffix=" << suffix_cases << elapsed() << '\n';}

    // Large shapes against the codegree oracle (paths at sampled starts), both graph types.
    void large(std::mt19937_64 &rng) {
        int n = pick(3000, 200000, 300000) / SHRINK, hub = min(n, CHECKED ? 1500 : ASAN ? 3000 : 5000);
        lng m = pick(10000, 500000, 300000) / SHRINK;
        vector<pair<string, Graph>> shapes;
        shapes.push_back({"uniform multigraph (loops, parallel edges)", build(n, randomEdges(rng, n, m, 1, 3))});
        shapes.push_back({"dense blocks plus uniform edges", build(n, blockEdges(rng, n, 0.6, m / 4, false))});
        vector<Edge> hes = randomEdges(rng, hub, 4 * lng(hub), 1, 3);
        int centre = rnd(rng, hub);
        for (int v = 0; v < hub; ++v) {
            if (v != centre) { hes.push_back(rng() & 1 ? Edge{centre, v} : Edge{v, centre}); }}
        std::shuffle(hes.begin(), hes.end(), rng);
        shapes.push_back({"hub adjacent to all plus uniform edges", build(hub, hes)});
        if (mode == "stress" && SHRINK == 1) { shapes.push_back({"uniform multigraph, average degree 10", build(200000, randomEdges(rng, 200000, 1000000, 1, 3))}); }
        for (auto &[name, g] : shapes) {
            context = "large " + name + show(g) + " seed=" + std::to_string(seed);
            vector<int> sample;
            int top = 0;
            for (int v = 0; v < g.n; ++v) { top = g[v].size() > g[top].size() ? v : top; }
            sample.push_back(top);
            for (int i = 0; i < 200; ++i) { sample.push_back(rnd(rng, g.n)); }
            Expect x = fast(g, sample);
            verifyBoth(g, x, rng);
            cout << "PASS large " << name << show(g) << " triangles=" << x.t << " c4=" << x.c4 << " k4=" << x.k4 << elapsed() << '\n';}
        context = "large relabel" + show(shapes[0].second);
        relabel(shapes[0].second, rng);
        context = "";}
    // Largest n with C(n, 3) < 2^63.
    int censusLimit() {
        int lo = 3, hi = 4000000;
        while (hi - lo > 1) {
            int mid = std::midpoint(lo, hi);
            (c3(mid) <= LLONG_MAX ? lo : hi) = mid;}
        return lo;}
    // Census on special digraphs, medium-scale and large digraphs against the fast census, and the C(n, 3) < 2^63 boundary.
    void censusCases(std::mt19937_64 &rng) {
        for (int n = 0; n < 3; ++n) {
            context = "census empty n=" + std::to_string(n);
            sameCensus(Graph(n, true), Census{}, "triadicCensus with fewer than 3 vertices");}
        int n = 60;
        vector<Edge> trans, both;
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) { trans.push_back({u, v}); both.push_back({u, v}); both.push_back({v, u}); }}
        Census e{};
        e[8] = choose(n, 3);
        context = "census transitive tournament n=60";
        sameCensus(build(n, trans, true), e, "transitive tournament is all 030T");
        e = Census{};
        e[15] = choose(n, 3);
        context = "census complete symmetric n=60";
        sameCensus(build(n, both, true), e, "complete symmetric digraph is all 300");
        int big = pick(3000, 200000, 300000) / SHRINK;
        lng m = pick(10000, 500000, 1000000) / SHRINK;
        vector<pair<string, Graph>> shapes;
        shapes.push_back({"uniform (loops, parallel and antiparallel arcs)", build(big, randomEdges(rng, big, m, 1, 20), true)});
        shapes.push_back({"dense directed blocks", build(big, blockEdges(rng, big, 0.4, m / 4, true), true)});
        for (auto &[name, g] : shapes) {
            context = "census large " + name + show(g) + " seed=" + std::to_string(seed);
            Census f = censusFast(g);
            sameCensus(g, f, "triadicCensus versus fast census");
            sameDirected(g, nxDirected(g));
            cout << "PASS census large " << name << show(g) << " 300=" << f[15] << " 030C=" << f[9] << elapsed() << '\n';}
        if (quick()) { return; }
        int lo = censusLimit();
        context = "census boundary n=" + std::to_string(lo) + " (largest n with C(n, 3) < 2^63)";
        e = Census{};
        e[0] = lng(c3(lo));
        sameCensus(Graph(lo, true), e, "triadicCensus at the largest n");
        e[0] = lng(c3(lo) - 1 - 3 * lll(lo - 3)); e[1] = 3 * lng(lo - 3); e[9] = 1;
        sameCensus(build(lo, {{0, 1}, {1, 2}, {2, 0}}, true), e, "triadicCensus 3-cycle at the largest n");
        context = "";
        cout << "PASS census boundary n=" << lo << elapsed() << '\n';}
    void invalid(const string &name) {
        vector<Edge> tri{{0, 1}, {1, 2}, {2, 0}};
        Graph d = build(4, tri, true), u = build(4, tri);
        auto none = [](int, int, int) {};
        if (name == "directed-enumerate") { enumerateTriangles(d, none); }
        else if (name == "directed-count") { (void)countTriangles(d); }
        else if (name == "directed-count-csr") { (void)countTriangles(CsrGraph(d)); }
        else if (name == "directed-product") { (void)triangleProductSum(d, vector<lng>(4, 1)); }
        else if (name == "directed-tri-edge") { (void)countTrianglesPerEdge(d); }
        else if (name == "directed-c4") { (void)countC4(d); }
        else if (name == "directed-c4-edge") { (void)countC4PerEdge(d); }
        else if (name == "directed-c3c4-vertex") { (void)countC3C4PerVertex(d); }
        else if (name == "directed-find-c4") { (void)findC4(d); }
        else if (name == "directed-k4") { (void)countK4(d); }
        else if (name == "directed-paths") { (void)countPathsP3P4P5(d); }
        else if (name == "undirected-census") { (void)triadicCensus(u); }
        else if (name == "undirected-census-csr") { (void)triadicCensus(CsrGraph(u)); }
        else if (name == "product-size-short") { (void)triangleProductSum(u, vector<lng>(3, 1)); }
        else if (name == "product-size-long") { (void)triangleProductSum(u, vector<mint>(5, 1)); }
        else if (name == "census-overflow") { (void)triadicCensus(Graph(censusLimit() + 1, true)); }
        else if (name == "census-overflow-4e6") { (void)triadicCensus(Graph(4000000, true)); }
        else if (name == "directed-enumerate-c4") { enumerateC4(d, [](int, int, int, int) {}); }
        else if (name == "directed-enumerate-k4") { enumerateK4(d, [](int, int, int, int) {}); }
        else if (name == "directed-local-clustering") { (void)localClustering(d); }
        else if (name == "directed-transitivity") { (void)transitivity(d); }
        else if (name == "directed-square-clustering") { (void)squareClustering(d); }
        else if (name == "directed-generalized-degree") { (void)generalizedDegree(d); }
        else if (name == "undirected-directed-clustering") { (void)directedClustering(u); }
        else if (name == "undirected-directed-clustering-csr") { (void)directedClustering(CsrGraph(u)); }
        else if (name == "directed-enumerate-triangle-edges") { enumerateTriangleEdges(d, [](int, int, int, int, int, int) {}); }
        else if (name == "directed-weighted-clustering") { (void)weightedClustering(d); }
        else if (name == "undirected-weighted-directed-clustering") { (void)weightedDirectedClustering(u); }
        else if (name == "weighted-parallel") { u.addEdge(1, 0, 2); (void)weightedClustering(u); }
        else if (name == "weighted-directed-parallel") { d.addEdge(0, 1, 2); (void)weightedDirectedClustering(d); }
        else if (name == "weighted-max-zero") { Graph z(3); z.addEdge(0, 1, 0); z.addEdge(1, 2, -4); (void)weightedClustering(z); }
        else if (name == "weighted-directed-max-zero") { Graph z(3, true); z.addEdge(0, 1, 0); z.addEdge(1, 2, -4); (void)weightedDirectedClustering(z); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        // Unbuffered, so a run killed at the memory cap still shows its last passed section.
        cout << std::unitbuf;
        std::mt19937_64 rng(seed);
        wrng.seed(seed ^ 0x9e3779b97f4a7c15);
        handCases(rng); triadCases(); exhaustive(rng); randomSmall(rng); medium(rng); closedForms(rng); domainEdge(); dense(rng); large(rng); censusCases(rng);
        cout << "PASS subgraph_enumeration seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL subgraph_enumeration seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
