#include "../../04-Graphs/29-planar_graph.hpp"
#include "../../04-Graphs/19-max_flow.hpp"

namespace {
    using Pt = pair<lng, lng>;
    using Rot = vector<vector<int>>;
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0, cut_queries = 0, genus0_rotations = 0, mixed_rotations = 0;
    // libstdc++ debug mode re-validates the whole heap on every push and pop, so the checked build keeps Dijkstra-heavy sections at quick size.
#ifdef _GLIBCXX_DEBUG
    const bool DEBUG_HEAP = true;
#else
    const bool DEBUG_HEAP = false;
#endif
    // Stress multiplies rounds by 5 in the optimized build and by 2 in the checked and sanitizer builds, which also keep full-size large shapes.
#if defined(_GLIBCXX_DEBUG) || defined(__SANITIZE_ADDRESS__)
    const bool CHECKED = true;
#else
    const bool CHECKED = false;
#endif
    int roundsFor(int quick, int full) { return mode == "quick" ? quick : mode == "full" ? full : (CHECKED ? 2 : 5) * full; }
    bool stressShapes() { return mode == "stress" && !CHECKED; }
    string str(lll x) {
        if (x == 0) { return "0"; }
        bool neg = x < 0; ulll u = neg ? ulll(-(x + 1)) + 1 : ulll(x); string s;
        while (u) { s += char('0' + int(u % 10)); u /= 10; }
        if (neg) { s += '-'; }
        reverse(s.begin(), s.end());
        return s;}
    string str(lng x) { return std::to_string(x); }
    string str(int x) { return std::to_string(x); }
    template<class T>
    string str(const vector<T> &v) {
        string s = "[";
        for (int i = 0; i < int(v.size()) && i < 64; ++i) { s += (i ? "," : "") + str(v[i]); }
        return s + (v.size() > 64 ? ",...]" : "]");}
    template<class A, class B>
    void eq(const A &actual, const B &expected, const string &operation) {
        ++checks;
        if (!(actual == expected)) { throw std::runtime_error(context + " operation=" + operation + " expected=" + str(expected) + " actual=" + str(actual)); }}
    template<class T>
    void eq(const vector<T> &actual, const vector<T> &expected, const string &operation) {
        ++checks;
        if (actual == expected) { return; }
        int i = 0, k = int(min(actual.size(), expected.size()));
        while (i < k && actual[i] == expected[i]) { ++i; }
        string at = i < k ? " expected[i]=" + str(expected[i]) + " actual[i]=" + str(actual[i]) : "";
        throw std::runtime_error(context + " operation=" + operation + " first difference at i=" + str(i) + at + " sizes expected=" + str(int(expected.size())) + " actual=" + str(int(actual.size())) + " expected=" + str(expected) + " actual=" + str(actual));}
    bool fails(bool ok) {
        ++checks; return !ok;}
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g, const vector<Pt> &xy = {}) {
        std::ostringstream out; out << "n=" << g.n << " edges(u-v:w)=";
        for (auto e : g.edges) { out << e.u << '-' << e.v << ':' << e.w << ' '; }
        out << "rotation=";
        for (int v = 0; v < g.n; ++v) { out << str(g.adj[v]); }
        if (!xy.empty()) {
            out << " xy=";
            for (auto [x, y] : xy) { out << '(' << x << ',' << y << ')'; }}
        return out.str();}
    struct Uf {
        vector<int> p;
        explicit Uf(int n) : p(n) { iota(p.begin(), p.end(), 0); }
        int find(int x) {
            while (p[x] != x) { x = p[x] = p[p[x]]; }
            return x;}
        bool unite(int a, int b) {
            a = find(a); b = find(b);
            if (a == b) { return false; }
            p[a] = b; return true;}
    };
    struct Oracle {
        int components = 0;
        vector<int> first, next, prev, face, component, outer, genus;
        vector<vector<int>> faces;
    };
    // next = cw o rev, cw[b] the arc clockwise before b around its tail; faces and components scanned from vertex 0 upward; genus from per-component V - E + F.
    Oracle orbit(const Graph &g, const Rot &rot) {
        int n = g.n, k = int(g.arcs.size());
        Oracle o; o.first.assign(n, -1); o.next.assign(k, -1); o.prev.assign(k, -1); o.face.assign(k, -1); o.component.assign(n, -1);
        vector<int> cw(k, -1);
        for (int v = 0; v < n; ++v) {
            int d = int(rot[v].size());
            if (d) { o.first[v] = rot[v][0]; }
            for (int i = 0; i < d; ++i) { cw[rot[v][(i + 1) % d]] = rot[v][i]; }}
        for (int a = 0; a < k; ++a) { o.next[a] = cw[g.arcs[a].rev]; o.prev[o.next[a]] = a; }
        for (int s = 0; s < n; ++s) {
            if (o.component[s] != -1) { continue; }
            o.component[s] = o.components; vector<int> q{s};
            for (int i = 0; i < int(q.size()); ++i) {
                for (int a : rot[q[i]]) {
                    int y = g.arcs[a].to;
                    if (o.component[y] == -1) { o.component[y] = o.components; q.push_back(y); }}}
            ++o.components;}
        o.outer.assign(o.components, -1);
        vector<int> chi(o.components);
        for (int v = 0; v < n; ++v) {
            ++chi[o.component[v]];
            if (rot[v].empty()) { o.outer[o.component[v]] = int(o.faces.size()); o.faces.emplace_back(); ++chi[o.component[v]]; }
            for (int a : rot[v]) {
                if (o.face[a] != -1) { continue; }
                o.faces.emplace_back(); ++chi[o.component[v]];
                for (int b = a; o.face[b] == -1; b = o.next[b]) { o.face[b] = int(o.faces.size()) - 1; o.faces.back().push_back(b); }}}
        for (auto e : g.edges) { --chi[o.component[e.u]]; }
        for (int c = 0; c < o.components; ++c) {
            if (fails(chi[c] <= 2 && chi[c] % 2 == 0)) { check(false, "oracle: component " + str(c) + " has V - E + F = " + str(chi[c]) + ", even and at most 2"); }
            o.genus.push_back((2 - chi[c]) / 2);}
        int total = 0;
        for (int a = 0; a < k; ++a) {
            if (fails(o.prev[a] != -1 && o.face[a] != -1)) { check(false, "oracle: next is a permutation covering every arc"); }}
        for (auto &f : o.faces) {
            total += int(f.size());
            for (int i = 0; i < int(f.size()); ++i) {
                if (fails(g.arcs[f[i]].to == g.arcs[f[(i + 1) % f.size()]].from)) { check(false, "oracle: face boundary is a closed walk"); }}}
        eq(total, k, "oracle: face lengths sum to 2m");
        return o;}
    void compare(const PlanarEmbedding &e, const Graph &g, const Oracle &o, const vector<int> &outer) {
        eq(e.n, g.n, "n"); eq(e.m, int(g.edges.size()), "m"); eq(e.components, o.components, "components");
        eq(int(e.arcs.size()), int(g.arcs.size()), "arcs.size()");
        for (int a = 0; a < int(g.arcs.size()); ++a) {
            auto x = e.arcs[a], y = g.arcs[a];
            if (fails(x.from == y.from && x.to == y.to && x.id == y.id && x.rev == y.rev && x.w == y.w)) { check(false, "arcs[" + str(a) + "] copied from the graph"); }}
        eq(e.first, o.first, "first"); eq(e.next, o.next, "next"); eq(e.prev, o.prev, "prev"); eq(e.face, o.face, "face");
        eq(e.component, o.component, "component"); eq(e.faces, o.faces, "faces"); eq(e.outer, outer, "outer");
        int chi = g.n - int(g.edges.size()) + int(o.faces.size());
        eq(e.eulerCharacteristic(), chi, "eulerCharacteristic");
        check(chi <= 2 * o.components && (2 * o.components - chi) % 2 == 0, "eulerCharacteristic = 2 * components - 2 * genus, genus >= 0");
        eq(e.genus, o.genus, "genus per component (oracle orbit counts)");
        eq(2 * std::accumulate(e.genus.begin(), e.genus.end(), 0), 2 * e.components - e.eulerCharacteristic(), "2 * sum(genus) = 2 * components - eulerCharacteristic");}
    void sameEmbedding(const PlanarEmbedding &a, const PlanarEmbedding &b, const string &operation) {
        check(a.n == b.n && a.m == b.m && a.components == b.components && a.first == b.first && a.next == b.next && a.prev == b.prev, operation);
        check(a.face == b.face && a.component == b.component && a.outer == b.outer && a.faces == b.faces && a.genus == b.genus, operation);}
    int commonFaceBrute(const Graph &g, const Oracle &o, int u, int v) {
        for (int f = 0; f < int(o.faces.size()); ++f) {
            bool hu = false, hv = false;
            for (int a : o.faces[f]) { hu |= g.arcs[a].from == u; hv |= g.arcs[a].from == v; }
            if (hu && hv) { return f; }}
        return -1;}
    void verifyDualTree(const PlanarEmbedding &e, const Graph &g, const Oracle &o) {
        int n = g.n, m = int(g.edges.size()), k = int(o.faces.size());
        for (int rep = 0; rep < 2; ++rep) {
            Graph d = e.dualGraph();
            check(d.n == k && !d.directed && int(d.edges.size()) == m, "dualGraph shape");
            for (int i = 0; i < m; ++i) {
                auto x = d.edges[i];
                if (fails(x.u == o.face[2 * i] && x.v == o.face[2 * i + 1] && x.w == g.edges[i].w)) { check(false, "dualGraph edge " + str(i) + " joins face[2e], face[2e+1] with weight w"); }}}
        TreeCotree want; Uf primal(n), dual(k);
        for (int i = 0; i < m; ++i) {
            if (primal.unite(g.edges[i].u, g.edges[i].v)) { want.tree.push_back(i); }}
        vector<char> in(m);
        for (int i : want.tree) { in[i] = 1; }
        for (int i = 0; i < m; ++i) {
            if (!in[i]) { (dual.unite(o.face[2 * i], o.face[2 * i + 1]) ? want.cotree : want.leftover).push_back(i); }}
        for (int rep = 0; rep < 2; ++rep) {
            auto tc = e.treeCotree();
            eq(tc.tree, want.tree, "treeCotree.tree"); eq(tc.cotree, want.cotree, "treeCotree.cotree"); eq(tc.leftover, want.leftover, "treeCotree.leftover");}
        eq(int(want.tree.size()), n - o.components, "|tree| = n - components");
        eq(int(want.cotree.size()), k - o.components, "|cotree| = faces - components");
        eq(int(want.leftover.size()), 2 * o.components - e.eulerCharacteristic(), "|leftover| = 2 * genus");}
    void verifyCommon(const PlanarEmbedding &e, const Graph &g, const Oracle &o) {
        for (int u = 0; u < g.n; ++u) {
            for (int v = 0; v < g.n; ++v) {
                if (u != v) { eq(e.commonFace(u, v), commonFaceBrute(g, o, u, v), "commonFace(" + str(u) + "," + str(v) + ")"); }}}}
    lng capOf(const Graph &g, const vector<lng> &cap, int a) { return cap.empty() ? g.arcs[a].w : cap[a]; }
    // Brute min cuts: cut[mask] (capacity leaving mask) by adding one vertex at a time, best[s][t] the minimum over masks with s in, t out.
    vector<vector<lll>> bruteCuts(const Graph &g, const vector<lng> &cap) {
        int n = g.n;
        vector<lll> cut(size_t(1) << n);
        vector<vector<lll>> best(n, vector<lll>(n, -1));
        for (int mask = 1; mask < (1 << n); ++mask) {
            int v = std::countr_zero(uint(mask)), rest = mask & (mask - 1);
            cut[mask] = cut[rest];
            for (int a : g.adj[v]) {
                int y = g.arcs[a].to;
                if (!(mask >> y & 1)) { cut[mask] += capOf(g, cap, a); }
                else if (y != v) { cut[mask] -= capOf(g, cap, g.arcs[a].rev); }}
            for (int s = 0; s < n; ++s) {
                if (!(mask >> s & 1)) { continue; }
                for (int t = 0; t < n; ++t) {
                    if (!(mask >> t & 1) && (best[s][t] == -1 || cut[mask] < best[s][t])) { best[s][t] = cut[mask]; }}}}
        return best;}
    lng maxFlow(const Graph &g, const vector<lng> &cap, int s, int t) {
        MaxFlow<lng> mf(g.n);
        for (int i = 0; i < int(g.edges.size()); ++i) { mf.addEdge(g.edges[i].u, g.edges[i].v, capOf(g, cap, 2 * i), capOf(g, cap, 2 * i + 1)); }
        return mf.flow(s, t);}
    void verifyCut(const PlanarEmbedding &e, const Graph &g, const vector<lng> &cap, int s, int t, lll expected) {
        int n = g.n, m = int(g.edges.size()), k = int(g.arcs.size());
        auto res = e.planarMinCutDual(s, t, cap); ++cut_queries;
        auto op = [&]() { return "planarMinCutDual(" + str(s) + "," + str(t) + (cap.empty() ? ",{})" : ",cap)") + (cap.empty() || k > 64 ? "" : " cap=" + str(cap)) + " arcs=" + str(res.arcs); };
        if (fails(res.value == expected)) { eq(res.value, expected, op() + " value"); }
        lll sum = 0; vector<char> cut(k);
        for (int i = 0; i < int(res.arcs.size()); ++i) {
            int a = res.arcs[i];
            if (fails(0 <= a && a < k && (i == 0 || res.arcs[i - 1] < a))) { check(false, op() + " arcs ascending in range"); }
            sum += capOf(g, cap, a); cut[a] = 1;}
        if (fails(sum == res.value)) { eq(sum, res.value, op() + " capacity of cut arcs"); }
        vector<char> pos(n), side(n); pos[s] = side[s] = 1;
        vector<int> q{s};
        for (int i = 0; i < int(q.size()); ++i) {
            for (int a : g.adj[q[i]]) {
                int y = g.arcs[a].to;
                if (!cut[a] && capOf(g, cap, a) > 0 && !pos[y]) { pos[y] = 1; q.push_back(y); }}}
        if (fails(!pos[t])) { check(false, op() + " no positive-capacity s->t path avoids the cut arcs"); }
        q = {s};
        for (int i = 0; i < int(q.size()); ++i) {
            for (int a : g.adj[q[i]]) {
                int y = g.arcs[a].to;
                if (!cut[a] && !cut[g.arcs[a].rev] && !side[y]) { side[y] = 1; q.push_back(y); }}}
        if (fails(!side[t])) { check(false, op() + " cut edges separate s from t"); }
        for (int a : res.arcs) {
            if (fails(!side[g.arcs[a].to])) { check(false, op() + " cut arc " + str(a) + " points from the s side to the t side"); }}
        if (fails(int(res.flow.size()) == m)) { eq(int(res.flow.size()), m, op() + " flow size"); }
        vector<lll> net(n);
        for (int i = 0; i < m; ++i) {
            lng f = res.flow[i];
            if (fails(-capOf(g, cap, 2 * i + 1) <= f && f <= capOf(g, cap, 2 * i))) { check(false, op() + " flow[" + str(i) + "]=" + str(f) + " within [-cap[rev], cap[fwd]]"); }
            if (fails(f == 0 || e.component[g.edges[i].u] == e.component[s])) { check(false, op() + " flow[" + str(i) + "]=" + str(f) + " outside the s-t component is zero"); }
            net[g.edges[i].u] += f; net[g.edges[i].v] -= f;}
        for (int v = 0; v < n; ++v) {
            lll want = v == s ? expected : v == t ? -expected : lll(0);
            if (fails(net[v] == want)) { eq(net[v], want, op() + " net flow out of vertex " + str(v) + " flow=" + str(res.flow)); }}}
    void verifyDisconnected(const PlanarEmbedding &e, const Graph &g, const vector<lng> &cap, int s, int t) {
        auto res = e.planarMinCutDual(s, t, cap); ++cut_queries;
        if (fails(res.value == 0 && res.arcs.empty() && res.flow == vector<lng>(g.edges.size(), 0))) { check(false, "planarMinCutDual(" + str(s) + "," + str(t) + ") disconnected gives value 0, no arcs, zero flow"); }}
    vector<vector<lng>> capVariants(std::mt19937_64 &rng, const Graph &g) {
        int k = int(g.arcs.size());
        vector<vector<lng>> res{{}, vector<lng>(k)};
        bool huge = rng() % 2;
        for (int a = 0; a < k; ++a) {
            if (huge) { res[1][a] = rng() % 2 ? (rng() % 4 ? lng(rng() % (ulng(1) << 61)) : LLONG_MAX - lng(rng() % 2)) : lng(rng() % 2); }
            else { res[1][a] = rng() % 3 ? lng(rng() % 6) : 0; }}
        return res;}
    // All ordered pairs: brute min cut when they share a face in a genus-0 component, the zero answer when disconnected.
    void verifyCutsBrute(const PlanarEmbedding &e, const Graph &g, const Oracle &o, std::mt19937_64 &rng) {
        vector<vector<char>> share(g.n, vector<char>(g.n));
        for (int s = 0; s < g.n; ++s) {
            for (int t = 0; t < g.n; ++t) { share[s][t] = s != t && commonFaceBrute(g, o, s, t) != -1; }}
        for (auto &cap : capVariants(rng, g)) {
            auto best = bruteCuts(g, cap);
            for (int s = 0; s < g.n; ++s) {
                for (int t = 0; t < g.n; ++t) {
                    if (s == t) { continue; }
                    if (o.component[s] != o.component[t]) { verifyDisconnected(e, g, cap, s, t); continue; }
                    if (!share[s][t] || o.genus[o.component[s]] != 0) { continue; }
                    verifyCut(e, g, cap, s, t, best[s][t]);
                    if (rng() % 8 == 0 && std::ranges::all_of(cap, [](lng c) { return c <= lng(1) << 61; }) && best[s][t] <= (lll(1) << 62)) { eq(lll(maxFlow(g, cap, s, t)), best[s][t], "oracle: MaxFlow agrees with brute subset cut"); }}}}}
    lll cross(Pt o, Pt a, Pt b) { return lll(a.first - o.first) * (b.second - o.second) - lll(a.second - o.second) * (b.first - o.first); }
    int sgn(lll x) { return (x > 0) - (x < 0); }
    bool strictlyInside(Pt p, Pt a, Pt b) { return cross(a, b, p) == 0 && min(a.first, b.first) <= p.first && p.first <= max(a.first, b.first) && min(a.second, b.second) <= p.second && p.second <= max(a.second, b.second) && p != a && p != b; }
    struct Drawing { Graph g; vector<Pt> xy; };
    // Random crossing-free straight-line drawing: distinct points in [-r, r]^2, each segment kept with probability density% if it crosses no segment and contains no point.
    Drawing randomDrawing(std::mt19937_64 &rng, int n, int r, int density, lng wmax) {
        Drawing d{Graph(n), {}}; set<Pt> used;
        while (int(d.xy.size()) < n) {
            Pt p(lng(rng() % ulng(2 * r + 1)) - r, lng(rng() % ulng(2 * r + 1)) - r);
            if (used.insert(p).second) { d.xy.push_back(p); }}
        vector<pair<int, int>> pairs;
        for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) { pairs.push_back({i, j}); }}
        std::shuffle(pairs.begin(), pairs.end(), rng);
        for (auto [i, j] : pairs) {
            if (int(rng() % 100) >= density) { continue; }
            bool ok = true; Pt a = d.xy[i], b = d.xy[j];
            for (int x = 0; x < n && ok; ++x) { ok = !strictlyInside(d.xy[x], a, b); }
            for (auto ed : d.g.edges) {
                if (!ok) { break; }
                if (ed.u == i || ed.u == j || ed.v == i || ed.v == j) { continue; }
                Pt c = d.xy[ed.u], f = d.xy[ed.v];
                ok = !(sgn(cross(a, b, c)) * sgn(cross(a, b, f)) < 0 && sgn(cross(c, f, a)) * sgn(cross(c, f, b)) < 0);}
            if (ok) {
                lng w = lng(rng() % ulng(wmax + 1));
                if (rng() % 2) { d.g.addEdge(i, j, w); } else { d.g.addEdge(j, i, w); }}}
        for (auto &l : d.g.adj) { std::shuffle(l.begin(), l.end(), rng); }
        return d;}
    // Grid r x c, vertex y * c + x at (x * scale + shift.x, y * scale + shift.y), right and up edges, one random diagonal per cell with probability diag%.
    Drawing grid(std::mt19937_64 &rng, int r, int c, int diag, lng scale, Pt shift, lng wmax) {
        Drawing d{Graph(r * c), {}};
        for (int y = 0; y < r; ++y) { for (int x = 0; x < c; ++x) { d.xy.push_back({x * scale + shift.first, y * scale + shift.second}); }}
        auto w = [&]() { return lng(rng() % ulng(wmax + 1)); };
        for (int y = 0; y < r; ++y) {
            for (int x = 0; x < c; ++x) {
                int v = y * c + x;
                if (x + 1 < c) { d.g.addEdge(v, v + 1, w()); }
                if (y + 1 < r) { d.g.addEdge(v, v + c, w()); }
                if (x + 1 < c && y + 1 < r && int(rng() % 100) < diag) {
                    if (rng() % 2) { d.g.addEdge(v, v + c + 1, w()); } else { d.g.addEdge(v + 1, v + c, w()); }}}}
        for (auto &l : d.g.adj) { std::shuffle(l.begin(), l.end(), rng); }
        return d;}
    // Counter-clockwise from direction (1, 0) inclusive by quarter-plane, then cross product inside a quarter.
    int quarter(Pt p) { return p.first > 0 && p.second >= 0 ? 0 : p.first <= 0 && p.second > 0 ? 1 : p.first < 0 && p.second <= 0 ? 2 : 3; }
    Rot drawingRotation(const Graph &g, const vector<Pt> &xy, bool small) {
        Rot rot = g.adj;
        for (int v = 0; v < g.n; ++v) {
            auto dir = [&](int a) { return Pt(xy[g.arcs[a].to].first - xy[v].first, xy[g.arcs[a].to].second - xy[v].second); };
            sort(rot[v].begin(), rot[v].end(), [&](int a, int b) {
                Pt p = dir(a), q = dir(b);
                if (quarter(p) != quarter(q)) { return quarter(p) < quarter(q); }
                return lll(p.first) * q.second - lll(p.second) * q.first > 0;});
            for (int i = 0; small && i + 1 < int(rot[v].size()); ++i) {
                auto ang = [&](int a) { long double t = std::atan2((long double)dir(a).second, (long double)dir(a).first); return t < 0 ? t + 2 * std::numbers::pi_v<long double> : t; };
                check(ang(rot[v][i]) < ang(rot[v][i + 1]), "oracle: quarter-plane order agrees with atan2");}}
        return rot;}
    // Outer face per component: the face of the last counter-clockwise arc at the lowest-then-leftmost vertex, whose downward sector is unbounded.
    vector<int> drawingOuter(const vector<Pt> &xy, const Rot &rot, const Oracle &o) {
        vector<int> low(o.components, -1), res(o.components);
        for (int v = 0; v < int(xy.size()); ++v) {
            int &l = low[o.component[v]];
            if (l == -1 || pair(xy[v].second, xy[v].first) < pair(xy[l].second, xy[l].first)) { l = v; }}
        for (int c = 0; c < o.components; ++c) { res[c] = rot[low[c]].empty() ? o.outer[c] : o.face[rot[low[c]].back()]; }
        return res;}
    // Full check of a drawing: embedding against the oracle, area signs, Euler, commonFace, dual, tree-cotree, and cuts when brute is set.
    PlanarEmbedding verifyDrawing(const Drawing &d, std::mt19937_64 &rng, bool small, bool brute) {
        const Graph &g = d.g;
        auto e = PlanarEmbedding::fromDrawing(g, d.xy);
        Rot rot = drawingRotation(g, d.xy, small);
        Oracle o = orbit(g, rot);
        compare(e, g, o, drawingOuter(d.xy, rot, o));
        eq(e.eulerCharacteristic(), 2 * o.components, "drawing: eulerCharacteristic = 2 * components");
        eq(e.genus, vector<int>(o.components, 0), "drawing: genus 0 in every component");
        if (small) {
            for (int f = 0; f < int(o.faces.size()); ++f) {
                if (o.faces[f].empty()) { continue; }
                lll area = 0;
                for (int a : o.faces[f]) { area += cross(Pt(0, 0), d.xy[g.arcs[a].from], d.xy[g.arcs[a].to]); }
                check((area <= 0) == (e.outer[o.component[g.arcs[o.faces[f][0]].from]] == f), "outer[c] is the face with nonpositive area, face " + str(f) + " area2=" + str(area));}}
        if (g.n <= 12) {
            Graph h = g;
            for (auto &l : h.adj) { std::shuffle(l.begin(), l.end(), rng); }
            sameEmbedding(PlanarEmbedding::fromDrawing(h, d.xy), e, "fromDrawing ignores the input rotation");
            verifyCommon(e, g, o);}
        verifyDualTree(e, g, o);
        if (brute) { verifyCutsBrute(e, g, o, rng); }
        return e;}
    // Triangle 0-1-2 (genus 0) and K3,3 on 3..8 (default rotation, genus >= 1), unit weights.
    Graph triangleK33() {
        Graph g(9); g.addEdge(0, 1); g.addEdge(1, 2); g.addEdge(2, 0);
        for (int u = 3; u < 6; ++u) { for (int v = 6; v < 9; ++v) { g.addEdge(u, v); }}
        return g;}
    void handCases() {
        context = "hand single edge";
        Graph g(2); g.addEdge(0, 1, 5);
        PlanarEmbedding e(g);
        check(e.next == vector<int>{1, 0} && e.prev == vector<int>{1, 0} && e.faces == vector<vector<int>>{{0, 1}} && e.first == vector<int>{0, 1} && e.eulerCharacteristic() == 2, "single edge: one face [0,1]");
        check(e.outer == vector<int>{-1} && e.commonFace(0, 1) == 0 && e.commonFace(1, 0) == 0 && e.genus == vector<int>{0}, "single edge: outer -1, commonFace 0, genus 0");
        auto cut = e.planarMinCutDual(1, 0);
        check(cut.value == 5 && cut.arcs == vector<int>{1} && cut.flow == vector<lng>{-5}, "single edge: cut 1->0 is arc 1, flow -5");
        context = "hand triangle (0,0),(1,0),(0,1) edges 0-1:1 1-2:2 2-0:3";
        Graph tri(3); tri.addEdge(0, 1, 1); tri.addEdge(1, 2, 2); tri.addEdge(2, 0, 3);
        auto te = PlanarEmbedding::fromDrawing(tri, {{0, 0}, {1, 0}, {0, 1}});
        Oracle to = orbit(tri, {{0, 5}, {2, 1}, {4, 3}});
        check(te.faces == vector<vector<int>>{{0, 2, 4}, {5, 3, 1}} && to.faces == te.faces && te.outer == vector<int>{1}, "triangle faces [[0,2,4],[5,3,1]], outer 1");
        auto tc = te.planarMinCutDual(0, 1);
        check(tc.value == 3 && tc.arcs == vector<int>{0, 3} && tc.flow == vector<lng>{1, -2, -2}, "triangle cut 0->1: value 3, arcs [0,3], flow [1,-2,-2]");
        check(bruteCuts(tri, {})[0][1] == 3 && maxFlow(tri, {}, 0, 1) == 3, "oracle: brute and MaxFlow give 3");
        check(te.treeCotree().tree == vector<int>{0, 1} && te.treeCotree().cotree == vector<int>{2} && te.treeCotree().leftover.empty(), "triangle tree-cotree");
        context = "hand isolated vertex and empty graph";
        PlanarEmbedding one{Graph(1)};
        check(one.faces == vector<vector<int>>{{}} && one.outer == vector<int>{0} && one.first == vector<int>{-1} && one.eulerCharacteristic() == 2 && one.components == 1 && one.genus == vector<int>{0}, "n=1: one empty face, outer 0, chi 2, genus 0");
        auto od = one.dualGraph(); auto ot = one.treeCotree();
        check(od.n == 1 && od.edges.empty() && ot.tree.empty() && ot.cotree.empty() && ot.leftover.empty(), "n=1: dual one vertex, empty tree-cotree");
        auto fd = PlanarEmbedding::fromDrawing(Graph(1), {{7, -3}});
        check(fd.faces == vector<vector<int>>{{}} && fd.outer == vector<int>{0}, "n=1 drawing: outer is the empty face");
        PlanarEmbedding none{Graph(0)};
        check(none.components == 0 && none.faces.empty() && none.eulerCharacteristic() == 0 && none.dualGraph().n == 0 && none.genus.empty(), "n=0: no faces, chi 0, no genus entries");
        context = "hand loop";
        Graph lp(1); lp.addEdge(0, 0, 4);
        PlanarEmbedding le(lp);
        check(le.next == vector<int>{0, 1} && le.faces == vector<vector<int>>{{0}, {1}} && le.eulerCharacteristic() == 2 && le.genus == vector<int>{0}, "loop: faces [[0],[1]], genus 0");
        check(le.dualGraph().edges[0].u == 0 && le.dualGraph().edges[0].v == 1 && le.treeCotree().cotree == vector<int>{0}, "loop: dual edge 0-1, loop in the cotree");
        context = "hand parallel pair";
        Graph pp(2); pp.addEdge(0, 1, 2); pp.addEdge(0, 1, 3);
        PlanarEmbedding pe(pp);
        check(pe.next == vector<int>{3, 2, 1, 0} && pe.faces == vector<vector<int>>{{0, 3}, {2, 1}}, "parallel pair: faces [[0,3],[2,1]]");
        auto pc = pe.planarMinCutDual(0, 1, {4, 1, 0, 6});
        check(pc.value == 4 && pc.arcs == vector<int>{0, 2} && pc.flow == vector<lng>{4, 0}, "parallel pair directed cut 0->1: value 4, arcs [0,2], flow [4,0]");
        context = "hand bridge between triangles";
        Graph bt(2); bt.addEdge(0, 1);
        check(PlanarEmbedding(bt).dualGraph().edges[0].u == PlanarEmbedding(bt).dualGraph().edges[0].v, "bridge becomes a dual loop");
        context = "hand K3,3 (default rotation)";
        Graph k33(6);
        for (int u = 0; u < 3; ++u) { for (int v = 3; v < 6; ++v) { k33.addEdge(u, v); }}
        PlanarEmbedding ke(k33);
        check(ke.eulerCharacteristic() <= 0 && orbit(k33, k33.adj).faces == ke.faces && int(ke.treeCotree().leftover.size()) == 2 - ke.eulerCharacteristic(), "K3,3: genus >= 1, leftover = 2 * genus");
        check(ke.genus == vector<int>{(2 - ke.eulerCharacteristic()) / 2} && ke.genus[0] >= 1, "K3,3 default rotation: genus field (2 - chi) / 2 >= 1");
        context = "hand triangle 0-1-2 plus K3,3 on 3..8 (default rotations)";
        Graph mix = triangleK33();
        PlanarEmbedding me(mix);
        check(me.genus.size() == 2 && me.genus[0] == 0 && me.genus[1] >= 1, "triangle + K3,3: genus [0, >=1]");
        verifyCut(me, mix, {}, 0, 1, 2); verifyCut(me, mix, {}, 2, 1, 2);
        check(me.planarMinCutDual(0, 3).value == 0 && me.planarMinCutDual(4, 2).value == 0, "triangle + K3,3: cross-component cuts are zero");
        context = "hand 4x4 grid: corner 0 and vertex 10 share no face";
        std::mt19937_64 rng(1);
        auto gd = grid(rng, 4, 4, 0, 1, {0, 0}, 1);
        auto ge = PlanarEmbedding::fromDrawing(gd.g, gd.xy);
        check(ge.commonFace(0, 10) == -1 && ge.commonFace(0, 5) != -1 && ge.commonFace(0, 15) == ge.outer[0], "grid commonFace: -1 for (0,10), outer for corners");
        ++cases;
        cout << "PASS hand cases (single edge, triangle, n=0, n=1, loop, parallel pair, bridge, K3,3, triangle + K3,3, 4x4 grid)\n";}
    void randomDrawings() {
        std::mt19937_64 rng(seed);
        int rounds = roundsFor(300, 4000);
        for (int r = 0; r < rounds; ++r) {
            int n = r % 10 == 9 ? 10 + int(rng() % 3) : 1 + int(rng() % 9), rad = 1 + int(rng() % 3);
            while ((2 * rad + 1) * (2 * rad + 1) < n) { ++rad; }
            auto d = randomDrawing(rng, n, rad, 20 + int(rng() % 81), rng() % 4 ? 9 : 0);
            context = "random drawing round=" + str(r) + " " + show(d.g, d.xy);
            verifyDrawing(d, rng, true, true); ++cases;}
        cout << "PASS random crossing-free drawings n<=12 rounds=" << rounds << " (embedding, outer, areas, commonFace, dual, tree-cotree, brute cuts over all face-sharing pairs)\n";}
    void hugeDrawings() {
        std::mt19937_64 rng(seed ^ 0x51);
        int rounds = roundsFor(50, 500);
        const lng B = lng(1) << 61;
        for (int r = 0; r < rounds; ++r) {
            int n = 2 + int(rng() % 9), rad = 2 + int(rng() % 3);
            auto d = randomDrawing(rng, n, rad, 70, 9);
            Drawing big = d; lng scale = B / rad;
            for (auto &[x, y] : big.xy) { x *= scale; y *= scale; }
            bool shift = rng() % 2;
            if (shift) {
                for (auto &[x, y] : big.xy) { x += lng(rng() % 1000); y -= lng(rng() % 1000); x = std::clamp(x, -B, B); y = std::clamp(y, -B, B); }
                set<Pt> uniq(big.xy.begin(), big.xy.end());
                if (int(uniq.size()) != n) { continue; }}
            context = "huge drawing round=" + str(r) + " " + show(big.g, big.xy);
            auto small = PlanarEmbedding::fromDrawing(d.g, d.xy);
            auto e = verifyDrawing(big, rng, false, false);
            // A shift may move a direction off the start ray (1, 0), which rotates the lists but keeps the cyclic order and so next.
            if (shift) { eq(e.next, small.next, "fromDrawing shifted from scaled 2^61/r keeps the unscaled face permutation"); }
            else { sameEmbedding(e, small, "fromDrawing scaled by 2^61/r equals the unscaled embedding"); }
            ++cases;}
        context = "huge extreme corners";
        Drawing c{Graph(6), {{-B, -B}, {B, -B}, {B, B}, {-B, B}, {0, 0}, {B, B - 1}}};
        c.g.addEdge(0, 1); c.g.addEdge(2, 3); c.g.addEdge(3, 0); c.g.addEdge(0, 4); c.g.addEdge(4, 2); c.g.addEdge(1, 5); c.g.addEdge(5, 2); c.g.addEdge(4, 5);
        context += " " + show(c.g, c.xy);
        auto ce = verifyDrawing(c, rng, false, false);
        check(ce.outer[0] == ce.face[1] && ce.faces[ce.outer[0]].size() == 5, "extreme corners: outer is the square, clockwise");
        cout << "PASS huge-coordinate drawings |x|,|y|<=2^61 rounds=" << rounds << " (exact rotation, outer, scaled metamorphic) and extreme corners\n";}
    void randomRotations() {
        std::mt19937_64 rng(seed ^ 0x9e);
        int rounds = roundsFor(400, 5000);
        for (int r = 0; r < rounds; ++r) {
            int n = 1 + int(rng() % 7), m = int(rng() % 11);
            Graph g(n);
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = rng() % 6 ? int(rng() % n) : u;
                if (i && rng() % 5 == 0) { u = g.edges[rng() % ulng(i)].u; v = g.edges[rng() % ulng(i)].v; }
                g.addEdge(u, v, lng(rng() % 10));}
            bool mixed = false;
            for (int tries = 0; tries < 25; ++tries) {
                for (auto &l : g.adj) { std::shuffle(l.begin(), l.end(), rng); }
                context = "random rotation round=" + str(r) + " " + show(g);
                PlanarEmbedding e(g);
                Oracle o = orbit(g, g.adj);
                compare(e, g, o, o.outer);
                sameEmbedding(PlanarEmbedding(CsrGraph(g)), e, "CsrGraph embedding equals Graph embedding");
                verifyCommon(e, g, o); verifyDualTree(e, g, o);
                if (e.eulerCharacteristic() == 2 * o.components) { ++genus0_rotations; verifyCutsBrute(e, g, o, rng); break; }
                vector<int> size(o.components);
                for (int v = 0; v < n; ++v) { size[o.component[v]] += o.genus[o.component[v]] == 0; }
                if (!mixed && std::ranges::any_of(size, [](int k) { return k >= 2; })) { mixed = true; ++mixed_rotations; verifyCutsBrute(e, g, o, rng); }}
            ++cases;}
        cout << "PASS random rotations of multigraphs with loops n<=7 m<=10 rounds=" << rounds << " (orbits, genus, CSR, commonFace, dual, tree-cotree; brute cuts on " << genus0_rotations << " genus-0 rotations and in the genus-0 components of " << mixed_rotations << " mixed-genus rotations)\n";}
    vector<pair<int, int>> facePairs(std::mt19937_64 &rng, const Graph &g, const PlanarEmbedding &e, int count) {
        vector<pair<int, int>> res;
        while (int(res.size()) < count) {
            auto &f = e.faces[rng() % e.faces.size()];
            if (f.size() < 2) { continue; }
            int s = g.arcs[f[rng() % f.size()]].from, t = g.arcs[f[rng() % f.size()]].from;
            if (s != t) { res.push_back({s, t}); }}
        return res;}
    void verifyCutsFlow(const PlanarEmbedding &e, const Graph &g, std::mt19937_64 &rng, int pairs, lng cmax) {
        int k = int(g.arcs.size());
        for (auto [s, t] : facePairs(rng, g, e, pairs)) {
            check(e.commonFace(s, t) != -1, "sampled pair shares a face");
            vector<lng> cap(k);
            for (int a = 0; a < k; ++a) { cap[a] = rng() % 4 ? lng(rng() % ulng(cmax + 1)) : 0; }
            for (auto &c : {vector<lng>{}, cap}) { verifyCut(e, g, c, s, t, maxFlow(g, c, s, t)); }}}
    void midDrawings() {
        std::mt19937_64 rng(seed ^ 0x3c);
        int rounds = DEBUG_HEAP ? 3 : roundsFor(3, 40);
        for (int r = 0; r < rounds; ++r) {
            int n = 60 + int(rng() % 140);
            auto d = randomDrawing(rng, n, 12, 15 + int(rng() % 40), 1000);
            context = "mid drawing round=" + str(r) + " n=" + str(n) + " m=" + str(int(d.g.edges.size()));
            auto e = verifyDrawing(d, rng, true, false);
            verifyCutsFlow(e, d.g, rng, 30, 1000);
            auto o = orbit(d.g, drawingRotation(d.g, d.xy, false));
            for (int i = 0; i < 200; ++i) {
                int u = int(rng() % ulng(n)), v = int(rng() % ulng(n));
                if (u != v) { eq(e.commonFace(u, v), commonFaceBrute(d.g, o, u, v), "commonFace(" + str(u) + "," + str(v) + ")"); }}
            ++cases;}
        cout << "PASS mid drawings n<=200 rounds=" << rounds << " (embedding, sampled commonFace, cuts versus MaxFlow)\n";}
    void large() {
        std::mt19937_64 rng(seed ^ 0x77);
        int side = mode == "quick" || DEBUG_HEAP ? 40 : stressShapes() ? 450 : 300, pairs = mode == "stress" ? 6 : 3;
        auto d = grid(rng, side, side, 100, 1, {-side / 2, -side / 2}, 1000000);
        context = "large grid " + str(side) + "x" + str(side) + " with diagonals";
        auto e = verifyDrawing(d, rng, false, false);
        int n = side * side;
        vector<pair<int, int>> fixed{{0, n - 1}, {n - 1, 0}, {0, 1}, {side + 1, 2 * side + 2}};
        for (auto [s, t] : fixed) {
            if (e.commonFace(s, t) == -1) { continue; }
            verifyCut(e, d.g, {}, s, t, maxFlow(d.g, {}, s, t));}
        verifyCutsFlow(e, d.g, rng, pairs, 1000000);
        auto rep = e.planarMinCutDual(0, n - 1);
        check(rep.value == e.planarMinCutDual(0, n - 1).value && rep.arcs == e.planarMinCutDual(0, n - 1).arcs, "repeated planarMinCutDual identical");
        int big = mode == "quick" ? 20000 : stressShapes() ? 600000 : 200000;
        Graph tree(big), cyc(big);
        for (int v = 1; v < big; ++v) { tree.addEdge(int(rng() % ulng(v)), v, lng(rng() % 1000)); }
        for (int v = 0; v < big; ++v) { cyc.addEdge(v, (v + 1) % big, lng(rng() % 1000)); }
        for (auto &l : tree.adj) { std::shuffle(l.begin(), l.end(), rng); }
        for (int shape = 0; shape < 2; ++shape) {
            Graph &g = shape ? cyc : tree;
            context = string(shape ? "large cycle" : "large random tree") + " n=" + str(big);
            PlanarEmbedding pe(g);
            Oracle o = orbit(g, g.adj);
            compare(pe, g, o, o.outer);
            eq(int(pe.faces.size()), shape ? 2 : 1, "faces count");
            verifyDualTree(pe, g, o);
            for (int q = 0; q < 3; ++q) {
                int s = int(rng() % ulng(big)), t = int(rng() % ulng(big));
                if (s == t) { continue; }
                lll want;
                if (shape) {
                    lng m1 = LLONG_MAX, m2 = LLONG_MAX;
                    for (int v = s; v != t; v = (v + 1) % big) { m1 = min(m1, g.edges[v].w); }
                    for (int v = t; v != s; v = (v + 1) % big) { m2 = min(m2, g.edges[v].w); }
                    want = lll(m1) + m2;}
                else {
                    vector<int> par(big, -1), pw(big); vector<int> qu{s}; par[s] = s;
                    for (int i = 0; i < int(qu.size()); ++i) {
                        for (int a : g.adj[qu[i]]) { int y = g.arcs[a].to; if (par[y] == -1) { par[y] = qu[i]; pw[y] = int(g.arcs[a].w); qu.push_back(y); }}}
                    lng mn = LLONG_MAX;
                    for (int v = t; v != s; v = par[v]) { mn = min(mn, lng(pw[v])); }
                    want = mn;}
                verifyCut(pe, g, {}, s, t, want);}}
        ++cases;
        cout << "PASS large: " << side << "x" << side << " grid drawing with diagonals (embedding, dual, tree-cotree, cuts versus MaxFlow), " << big << "-vertex random tree and cycle (orbits, path-min cuts)\n";}
    struct Mixed { Graph g; vector<int> pv, qv, pe; };
    // Drawing d (genus 0) plus K3,3 or a genus >= 1 rotation of K4 in one graph: vertex blocks in random order, edges interleaved at random, rotations mapped.
    Mixed mixedGraph(std::mt19937_64 &rng, const Drawing &d, bool k33) {
        int n1 = d.g.n, n2 = k33 ? 6 : 4;
        Graph q(n2);
        for (int u = 0; u < n2; ++u) {
            for (int v = u + 1; v < n2; ++v) {
                if (!k33 || (u < 3 && v >= 3)) { q.addEdge(u, v, lng(rng() % 10)); }}}
        if (k33 && rng() % 2) { for (auto &l : q.adj) { std::shuffle(l.begin(), l.end(), rng); }}
        while (!k33 && 4 - 6 + int(orbit(q, q.adj).faces.size()) == 2) { for (auto &l : q.adj) { std::shuffle(l.begin(), l.end(), rng); }}
        Mixed res{Graph(n1 + n2), vector<int>(n1), vector<int>(n2), vector<int>(d.g.edges.size())};
        bool planar_first = rng() % 2;
        for (int v = 0; v < n1; ++v) { res.pv[v] = planar_first ? v : n2 + v; }
        for (int v = 0; v < n2; ++v) { res.qv[v] = planar_first ? n1 + v : v; }
        vector<int> qe(q.edges.size());
        for (size_t i = 0, j = 0; i < res.pe.size() || j < qe.size();) {
            if (j == qe.size() || (i < res.pe.size() && rng() % 2)) { auto e = d.g.edges[i]; res.pe[i++] = res.g.addEdge(res.pv[e.u], res.pv[e.v], e.w); }
            else { auto e = q.edges[j]; qe[j++] = res.g.addEdge(res.qv[e.u], res.qv[e.v], e.w); }}
        Rot pr = drawingRotation(d.g, d.xy, false);
        auto place = [&](const Rot &rot, const vector<int> &vmap, const vector<int> &emap) {
            for (int v = 0; v < int(rot.size()); ++v) {
                auto &l = res.g.adj[vmap[v]]; l.clear();
                for (int a : rot[v]) { l.push_back(2 * emap[a / 2] + a % 2); }}};
        place(pr, res.pv, res.pe); place(q.adj, res.qv, qe);
        return res;}
    // Cuts inside the genus-0 drawing component against brute subset cuts on the drawing alone and MaxFlow on the whole graph; cross-component pairs give the zero cut.
    void mixedComponents() {
        std::mt19937_64 rng(seed ^ 0xa5);
        int rounds = roundsFor(30, 300); lng before = cut_queries;
        for (int r = 0; r < rounds; ++r) {
            int n1 = 2 + int(rng() % 9), rad = 2;
            auto d = randomDrawing(rng, n1, rad, 30 + int(rng() % 71), rng() % 4 ? 9 : 0);
            auto [g, pv, qv, pe] = mixedGraph(rng, d, r % 2);
            context = "mixed genus round=" + str(r) + (r % 2 ? " drawing + K3,3 " : " drawing + K4 genus 1 ") + "planar_vertices=" + str(pv) + " " + show(g) + " drawing " + show(d.g, d.xy);
            PlanarEmbedding e(g);
            Oracle o = orbit(g, g.adj);
            compare(e, g, o, o.outer);
            sameEmbedding(PlanarEmbedding(CsrGraph(g)), e, "CsrGraph embedding equals Graph embedding");
            check(e.genus[e.component[qv[0]]] >= 1, "mixed: K3,3 or K4 component has genus >= 1");
            for (int v : pv) { check(e.genus[e.component[v]] == 0, "mixed: drawing components have genus 0"); }
            verifyDualTree(e, g, o);
            for (auto &cap : capVariants(rng, g)) {
                vector<lng> sub(cap.empty() ? 0 : d.g.arcs.size());
                for (int a = 0; a < int(sub.size()); ++a) { sub[a] = cap[2 * pe[a / 2] + a % 2]; }
                auto best = bruteCuts(d.g, sub);
                bool small = std::ranges::all_of(cap, [](lng c) { return c <= lng(1) << 61; });
                for (int s = 0; s < n1; ++s) {
                    for (int t = 0; t < n1; ++t) {
                        int S = pv[s], T = pv[t];
                        if (s == t) { continue; }
                        if (o.component[S] != o.component[T]) { verifyDisconnected(e, g, cap, S, T); continue; }
                        if (commonFaceBrute(g, o, S, T) == -1) { continue; }
                        verifyCut(e, g, cap, S, T, best[s][t]);
                        if (small && rng() % 4 == 0) { eq(lll(maxFlow(g, cap, S, T)), best[s][t], "oracle: MaxFlow on the whole mixed graph agrees with brute cut on the drawing"); }}}
                for (int i = 0; i < 4; ++i) {
                    int S = pv[rng() % pv.size()], T = qv[rng() % qv.size()];
                    verifyDisconnected(e, g, cap, S, T); verifyDisconnected(e, g, cap, T, S);}}
            ++cases;}
        cout << "PASS mixed-genus graphs (drawing n<=10 plus K3,3 or genus-1 K4) rounds=" << rounds << " (genus per component, orbits, dual, tree-cotree; cuts in the planar component versus brute and MaxFlow; cross-component zero cuts) cut_queries=" << cut_queries - before << "\n";}
    void invalid(const string &name) {
        Graph tri(3); tri.addEdge(0, 1); tri.addEdge(1, 2); tri.addEdge(2, 0);
        PlanarEmbedding te(tri);
        if (name == "directed") { PlanarEmbedding bad{Graph(2, true)}; }
        else if (name == "common-same") { te.commonFace(1, 1); }
        else if (name == "common-range") { te.commonFace(0, 3); }
        else if (name == "cut-same") { te.planarMinCutDual(2, 2); }
        else if (name == "cut-range") { te.planarMinCutDual(0, 3); }
        else if (name == "cut-genus-k33") {
            Graph k33(6);
            for (int u = 0; u < 3; ++u) { for (int v = 3; v < 6; ++v) { k33.addEdge(u, v); }}
            PlanarEmbedding(k33).planarMinCutDual(0, 3);}
        else if (name == "cut-genus-k4") {
            Graph k4(4); std::mt19937_64 rng(5);
            for (int u = 0; u < 4; ++u) { for (int v = u + 1; v < 4; ++v) { k4.addEdge(u, v); }}
            while (PlanarEmbedding(k4).eulerCharacteristic() == 2) { for (auto &l : k4.adj) { std::shuffle(l.begin(), l.end(), rng); }}
            PlanarEmbedding(k4).planarMinCutDual(0, 1);}
        else if (name == "cut-genus-mixed") { PlanarEmbedding(triangleK33()).planarMinCutDual(3, 6); }
        else if (name == "cut-no-common-face") {
            std::mt19937_64 rng(1); auto d = grid(rng, 4, 4, 0, 1, {0, 0}, 1);
            PlanarEmbedding::fromDrawing(d.g, d.xy).planarMinCutDual(0, 10);}
        else if (name == "cut-negative") { te.planarMinCutDual(0, 1, {1, 1, 1, -1, 1, 1}); }
        else if (name == "cut-negative-weight") { Graph h(2); h.addEdge(0, 1, -1); PlanarEmbedding(h).planarMinCutDual(0, 1); }
        else if (name == "cut-cap-size") { te.planarMinCutDual(0, 1, {1, 1, 1, 1, 1}); }
        else if (name == "drawing-loop") { Graph h(2); h.addEdge(0, 1); h.addEdge(1, 1); PlanarEmbedding::fromDrawing(h, {{0, 0}, {1, 0}}); }
        else if (name == "drawing-parallel") { Graph h = tri; h.addEdge(0, 1); PlanarEmbedding::fromDrawing(h, {{0, 0}, {2, 0}, {0, 2}}); }
        else if (name == "drawing-coincident") { PlanarEmbedding::fromDrawing(tri, {{0, 0}, {0, 0}, {1, 1}}); }
        else if (name == "drawing-range") { PlanarEmbedding::fromDrawing(tri, {{0, 0}, {lng(1) << 62, 0}, {0, 1}}); }
        else if (name == "drawing-size") { PlanarEmbedding::fromDrawing(tri, {{0, 0}, {1, 0}}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        handCases(); randomDrawings(); hugeDrawings(); randomRotations(); mixedComponents(); midDrawings(); large();
        cout << "PASS planar_graph seed=" << seed << " mode=" << mode << " cases=" << cases << " cut_queries=" << cut_queries << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL planar_graph seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
