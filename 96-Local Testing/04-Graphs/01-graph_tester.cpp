#include "../../04-Graphs/01-graph.hpp"
#include "../../02-Data Structures/07-ordered_set.hpp"
#include <fcntl.h>
#include <unistd.h>

static ulng seed = 20260927;
static lng checks = 0;
static string context;
static int factor_rounds = 2;
using Edges = vector<tuple<int, int, lng>>;

void require(bool ok, string_view operation, string_view want = "true", string_view got = "false") {
    ++checks; if (ok) { return; }
    throw std::runtime_error(context + " operation=" + string(operation) + " expected=" + string(want) + " actual=" + string(got));}
void describe(int n, bool directed, const Edges &edges) {
    context = "n=" + std::to_string(n) + " directed=" + std::to_string(directed) + " edges=";
    for (auto [u, v, w] : edges) {
        context += "(" + std::to_string(u) + "," + std::to_string(v) + "," + std::to_string(w) + ")";}}
template<class G>
void storage(const G &g, int n, bool directed, const Edges &edges) {
    require(g.n == n && g.directed == directed, "vertex count/direction");
    int m = int(edges.size()), multiple = directed ? 1 : 2;
    require(int(g.edges.size()) == m && int(g.arcs.size()) == multiple * m, "logical edge and arc counts");
    vector<int> seen(multiple * m), per_edge(m);
    for (int id = 0; id < m; ++id) {
        auto [u, v, w] = edges[id]; auto e = g.edges[id];
        if (e.u != u || e.v != v || e.w != w) { require(false, "logical edge insertion identity=" + std::to_string(id)); }}
    for (int u = 0; u < n; ++u) {
        int previous = -1;
        for (int a : g[u]) {
            require(0 <= a && a < int(g.arcs.size()), "adjacency arc bounds");
            require(a > previous, "adjacency insertion order"); previous = a;
            ++seen[a]; auto e = g.arcs[a];
            require(e.from == u && 0 <= e.id && e.id < m, "arc origin/logical ID");
            auto [x, y, w] = edges[e.id]; ++per_edge[e.id];
            require(e.w == w && ((e.from == x && e.to == y) || (!directed && e.from == y && e.to == x)),
                "arc endpoint/weight fidelity");
            if (directed) { require(e.rev == -1, "directed arc has no reciprocal"); }
            else {
                require(0 <= e.rev && e.rev < int(g.arcs.size()) && e.rev != a, "distinct reverse including loop");
                auto r = g.arcs[e.rev];
                require(r.rev == a && r.id == e.id && r.from == e.to && r.to == e.from && r.w == e.w,
                    "reciprocal arc involution and edge ID");}}}
    for (int count : seen) { require(count == 1, "each arc occurs exactly once in adjacency"); }
    for (int count : per_edge) { require(count == multiple, "each logical edge has required multiplicity"); }}
Edges listed(const Graph &g) {
    Edges res;
    for (auto e : g.edges) { res.emplace_back(e.u, e.v, e.w); }
    return res;}
// Independent oracles: incidence counting, filtered edge lists, relabelled endpoints and a pair map.
template<class G>
void lineOracle(const G &g, bool directed, const Edges &edges) {
    // Exact sequence: directed, (i, j) for each i, then each j leaving head(i), both by ID; undirected, per vertex x
    // its incident IDs (a loop once) in ID order, and (inc[j], inc[i]) for each i, then each j < i.
    int m = int(edges.size()); vector<pair<int, int>> want, got;
    for (int i = 0; i < m && directed; ++i) {
        for (int j = 0; j < m; ++j) { if (get<1>(edges[i]) == get<0>(edges[j])) { want.push_back({i, j}); }}}
    for (int x = 0; x < g.n && !directed; ++x) {
        vector<int> inc;
        for (int id = 0; id < m; ++id) { if (get<0>(edges[id]) == x || get<1>(edges[id]) == x) { inc.push_back(id); }}
        for (int i = 0; i < int(inc.size()); ++i) {
            for (int j = 0; j < i; ++j) { want.push_back({inc[j], inc[i]}); }}}
    auto line = lineGraph(g);
    require(line.n == m && line.directed == directed, "line graph vertex per edge");
    storage(line, m, directed, listed(line));
    for (auto e : line.edges) { require(e.w == 1, "line graph unit weights"); got.push_back({e.u, e.v}); }
    require(got == want, "line graph exact edge sequence", std::to_string(want.size()), std::to_string(got.size()));}
template<class G>
void inducedOracle(const G &g, int n, bool directed, const Edges &edges, const vector<int> &vs) {
    vector<int> pos(n, -1), want;
    for (int i = 0; i < int(vs.size()); ++i) { pos[vs[i]] = i; }
    // Kept edges grouped by the position of their u in vs, IDs increasing within a group.
    for (int x : vs) {
        for (int id = 0; id < int(edges.size()); ++id) {
            if (get<0>(edges[id]) == x && pos[get<1>(edges[id])] != -1) { want.push_back(id); }}}
    auto sub = inducedSubgraph(g, vs);
    require(sub.graph.n == int(vs.size()) && sub.graph.directed == directed, "induced vertex count/direction");
    require(sub.vertex == vs, "induced vertex map is vs");
    require(sub.edge == want && sub.edge.size() == sub.graph.edges.size(), "induced exact edge sequence");
    storage(sub.graph, sub.graph.n, directed, listed(sub.graph));
    for (int i = 0; i < int(sub.edge.size()); ++i) {
        auto [u, v, w] = edges[sub.edge[i]]; auto e = sub.graph.edges[i];
        require(e.u == pos[u] && e.v == pos[v] && e.w == w, "induced edge keeps orientation/weight under relabelling");}}
template<class G>
void simplifyOracle(const G &g, int n, bool directed, const Edges &edges) {
    map<pair<int, int>, pair<lng, int>> best;
    for (int id = 0; id < int(edges.size()); ++id) {
        auto [u, v, w] = edges[id];
        if (u == v) { continue; }
        auto key = directed ? pair(u, v) : pair(min(u, v), max(u, v));
        if (!best.count(key) || pair(w, id) < best[key]) { best[key] = {w, id}; }}
    vector<int> want;
    for (auto [key, value] : best) { want.push_back(value.second); }
    sort(want.begin(), want.end());
    auto sub = simplify(g);
    require(sub.graph.n == n && sub.graph.directed == directed && sub.edge == want, "simplify keeps lightest earliest edge per pair");
    vector<int> all(n); iota(all.begin(), all.end(), 0); require(sub.vertex == all, "simplify vertex map is the identity");
    Edges kept;
    for (int id : want) { kept.push_back(edges[id]); }
    storage(sub.graph, n, directed, kept);}
string show(const Edges &edges) {
    string res;
    for (auto [u, v, w] : edges) { res += "(" + std::to_string(u) + "," + std::to_string(v) + "," + std::to_string(w) + ")"; }
    return res;}
// Exact (u, v, w) sequence, vertex count and direction, then every storage invariant of the result.
void same(const string &operation, const Graph &got, int n, bool directed, const Edges &want) {
    require(got.n == n && got.directed == directed, operation + " vertex count/direction",
        std::to_string(n) + "/" + std::to_string(directed), std::to_string(got.n) + "/" + std::to_string(got.directed));
    ++checks;
    if (listed(got) != want) { require(false, operation + " exact edge sequence", show(want), show(listed(got))); }
    storage(got, n, directed, want);}
void sameSubgraph(const string &operation, const Subgraph &sub, int n, bool directed, const Edges &edges, const vector<int> &ids) {
    vector<int> all(n); iota(all.begin(), all.end(), 0); Edges want;
    for (int id : ids) { want.push_back(edges[id]); }
    require(sub.vertex == all && sub.edge == ids, operation + " identity vertex map and source edge IDs");
    same(operation, sub.graph, n, directed, want);}
using Matrix = vector<vector<lng>>;
// Arc counts u -> v: an undirected edge counts both ways, its loop once.
Matrix multiplicity(int n, bool directed, const Edges &edges) {
    Matrix res(n, vector<lng>(n));
    for (auto [u, v, w] : edges) {
        ++res[u][v];
        if (!directed && u != v) { ++res[v][u]; }}
    return res;}
// Product adjacency identity: count((u, x) -> (v, y)) = f(u, x, v, y) from the factors' multiplicity matrices.
template<class F>
void productIdentity(const string &operation, const Graph &got, int nb, F f) {
    auto have = multiplicity(got.n, got.directed, listed(got));
    for (int p = 0; p < got.n; ++p) {
        for (int q = 0; q < got.n; ++q) {
            lng want = f(p / nb, p % nb, q / nb, q % nb);
            if (have[p][q] != want) {
                require(false, operation + " adjacency identity at (" + std::to_string(p) + "," + std::to_string(q) + ")",
                    std::to_string(want), std::to_string(have[p][q]));}}}
    ++checks;}
// Unary operations against adjacency matrices, Floyd hop distances and filtered edge lists.
template<class G>
void transforms(const G &g, int n, bool directed, const Edges &edges, std::mt19937_64 &rng) {
    int m = int(edges.size()); auto adjacent = multiplicity(n, directed, edges);
    vector<int> in(n);
    for (auto [u, v, w] : edges) { ++in[v]; if (!directed) { ++in[u]; } }
    require(inDegrees(g) == in, "inDegrees counts entering arcs, undirected loop twice");
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            int want = -1;
            for (int id = 0; id < m && want == -1; ++id) {
                auto [x, y, w] = edges[id];
                if ((x == u && y == v) || (!directed && x == v && y == u)) { want = id; }}
            int got = edgeId(g, u, v);
            if (got != want) {
                require(false, "edgeId smallest matching ID u=" + std::to_string(u) + " v=" + std::to_string(v),
                    std::to_string(want), std::to_string(got));}}}
    Edges want;
    for (int u = 0; u < n; ++u) {
        for (int v = directed ? 0 : u + 1; v < n; ++v) {
            if (u != v && !adjacent[u][v]) { want.emplace_back(u, v, 1); }}}
    same("complementGraph", complementGraph(g), n, directed, want);
    const int inf = INT_MAX / 4; vector<vector<int>> hops(n, vector<int>(n, inf));
    for (int u = 0; u < n; ++u) { hops[u][u] = 0; }
    for (auto [u, v, w] : edges) {
        if (u != v) { hops[u][v] = 1; if (!directed) { hops[v][u] = 1; } }}
    for (int k = 0; k < n; ++k) { for (int u = 0; u < n; ++u) { for (int v = 0; v < n; ++v) {
        hops[u][v] = min(hops[u][v], hops[u][k] + hops[k][v]);}}}
    for (int k : {0, 1, 2, 3, n, INT_MAX}) {
        want.clear();
        for (int u = 0; u < n; ++u) {
            for (int v = directed ? 0 : u + 1; v < n; ++v) {
                if (0 < hops[u][v] && hops[u][v] < inf && hops[u][v] <= k) { want.emplace_back(u, v, 1); }}}
        same("graphPower k=" + std::to_string(k), graphPower(g, k), n, directed, want);}
    want.clear();
    for (int id = 0; id < m; ++id) { auto [u, v, w] = edges[id]; want.emplace_back(u, n + id, w); want.emplace_back(n + id, v, 0); }
    same("subdivideEdges", subdivideEdges(g), n + m, directed, want);
    same("toUndirected", toUndirected(g), n, false, edges);
    want.clear();
    for (auto [u, v, w] : edges) {
        want.emplace_back(u, v, w);
        if (!directed) { want.emplace_back(v, u, w); }}
    same("toDirected", toDirected(g), n, true, want);
    vector<int> all(m); iota(all.begin(), all.end(), 0); vector<int> ids = all;
    sameSubgraph("edgeSubgraph all", edgeSubgraph(g, all), n, directed, edges, all);
    sameSubgraph("edgeSubgraph empty", edgeSubgraph(g, {}), n, directed, edges, {});
    std::shuffle(ids.begin(), ids.end(), rng); ids.resize(m ? rng() % uint(m + 1) : 0);
    sameSubgraph("edgeSubgraph shuffled subset", edgeSubgraph(g, ids), n, directed, edges, ids);
    for (int round = 0; round < 3; ++round) {
        vector<int> vs, drop; set<int> hidden, gone; vector<int> keep;
        for (int i = n ? int(rng() % uint(n + 2)) : 0; i > 0; --i) { vs.push_back(int(rng() % uint(n))); hidden.insert(vs.back()); }
        if (round == 0) { vs.clear(); hidden.clear(); }
        for (int i = m ? int(rng() % uint(m / 2 + 2)) : 0; i > 0; --i) { drop.push_back(int(rng() % uint(m))); gone.insert(drop.back()); }
        for (int id = 0; id < m; ++id) {
            auto [u, v, w] = edges[id];
            if (!gone.count(id) && !hidden.count(u) && !hidden.count(v)) { keep.push_back(id); }}
        sameSubgraph("restrictedView", restrictedView(g, vs, drop), n, directed, edges, keep);}
    if (!directed) {
        want = edges;
        for (auto [u, v, w] : edges) { want.emplace_back(n + u, v, w); want.emplace_back(u, n + v, w); }
        for (int v = 0; v < n; ++v) { want.emplace_back(n + v, 2 * n, 1); }
        same("mycielskian", mycielskian(g), 2 * n + 1, false, want);}
    if (!n) { return; }
    // Builders: appended edges after a copy of the source, returned first new ID.
    Graph h(n, directed);
    for (auto [u, v, w] : edges) { h.addEdge(u, v, w); }
    want = edges;
    for (int round = 0; round < 3; ++round) {
        vector<int> vs(rng() % 5); lng w = lng(rng() % 7) - 3;
        for (int &v : vs) { v = int(rng() % uint(n)); }
        require(h.addPath(vs, w) == int(want.size()), "addPath returns first new edge ID");
        for (int i = 0; i + 1 < int(vs.size()); ++i) { want.emplace_back(vs[i], vs[i + 1], w); }
        require(h.addStar(vs.empty() ? 0 : vs[0], vs, w) == int(want.size()), "addStar returns first new edge ID");
        for (int v : vs) { want.emplace_back(vs.empty() ? 0 : vs[0], v, w); }
        if (vs.empty()) { vs.push_back(int(rng() % uint(n))); }
        require(h.addCycle(vs, w) == int(want.size()), "addCycle returns first new edge ID");
        for (int i = 0; i < int(vs.size()); ++i) { want.emplace_back(vs[i], vs[(i + 1) % vs.size()], w); }
        same("addPath/addStar/addCycle", h, n, directed, want);}}
// Products, joins and unions of a and b (independent spec sequences plus adjacency identities).
template<class A, class B>
void productOracles(const A &a, int na, const Edges &ea, const B &b, int nb, const Edges &eb, bool directed) {
    auto id = [&](int u, int x) { return u * nb + x; };
    auto ma = multiplicity(na, directed, ea), mb = multiplicity(nb, directed, eb);
    Edges want = ea;
    for (auto [u, v, w] : eb) { want.emplace_back(na + u, na + v, w); }
    same("disjointUnion", disjointUnion(a, b), na + nb, directed, want);
    for (int u = 0; u < na; ++u) {
        for (int x = 0; x < nb; ++x) {
            want.emplace_back(u, na + x, 1);
            if (directed) { want.emplace_back(na + x, u, 1); }}}
    same("graphJoin", graphJoin(a, b), na + nb, directed, want);
    Edges cart, tens, lex;
    for (auto [u, v, w] : ea) { for (int x = 0; x < nb; ++x) { cart.emplace_back(id(u, x), id(v, x), w); }}
    Edges layers;
    for (int u = 0; u < na; ++u) { for (auto [x, y, w] : eb) { layers.emplace_back(id(u, x), id(u, y), w); }}
    cart.insert(cart.end(), layers.begin(), layers.end());
    for (auto [u, v, w] : ea) {
        for (auto [x, y, f] : eb) {
            tens.emplace_back(id(u, x), id(v, y), w);
            if (!directed && u != v && x != y) { tens.emplace_back(id(u, y), id(v, x), w); }}}
    for (auto [u, v, w] : ea) {
        for (int x = 0; x < nb; ++x) {
            for (int y = 0; y < nb; ++y) {
                if (directed || u != v || y >= x) { lex.emplace_back(id(u, x), id(v, y), w); }}}}
    lex.insert(lex.end(), layers.begin(), layers.end());
    auto strongWant = cart; strongWant.insert(strongWant.end(), tens.begin(), tens.end());
    auto c = cartesianProduct(a, b), t = tensorProduct(a, b), s = strongProduct(a, b), l = lexicographicProduct(a, b);
    same("cartesianProduct", c, na * nb, directed, cart); same("tensorProduct", t, na * nb, directed, tens);
    same("strongProduct", s, na * nb, directed, strongWant); same("lexicographicProduct", l, na * nb, directed, lex);
    auto cartesian = [&](int u, int x, int v, int y) { return (x == y ? ma[u][v] : 0) + (u == v ? mb[x][y] : 0); };
    auto tensor = [&](int u, int x, int v, int y) { return ma[u][v] * mb[x][y]; };
    productIdentity("cartesianProduct", c, nb, cartesian); productIdentity("tensorProduct", t, nb, tensor);
    productIdentity("strongProduct", s, nb, [&](int u, int x, int v, int y) { return cartesian(u, x, v, y) + tensor(u, x, v, y); });
    productIdentity("lexicographicProduct", l, nb, [&](int u, int x, int v, int y) { return ma[u][v] + (u == v ? mb[x][y] : 0); });
    for (int root = 0; root < nb; ++root) {
        want.clear();
        for (auto [u, v, w] : ea) { want.emplace_back(id(u, root), id(v, root), w); }
        want.insert(want.end(), layers.begin(), layers.end());
        auto r = rootedProduct(a, b, root); same("rootedProduct root=" + std::to_string(root), r, na * nb, directed, want);
        auto rooted = [&](int u, int x, int v, int y) { return (x == root && y == root ? ma[u][v] : 0) + (u == v ? mb[x][y] : 0); };
        productIdentity("rootedProduct", r, nb, rooted);}
    want = ea;
    for (int u = 0; u < na; ++u) {
        int base = na + u * nb;
        for (auto [x, y, w] : eb) { want.emplace_back(base + x, base + y, w); }
        for (int x = 0; x < nb; ++x) {
            want.emplace_back(u, base + x, 1);
            if (directed) { want.emplace_back(base + x, u, 1); }}}
    auto corona = coronaProduct(a, b); same("coronaProduct", corona, na * (nb + 1), directed, want);
    auto have = multiplicity(corona.n, directed, listed(corona));
    for (int p = 0; p < corona.n; ++p) {
        for (int q = 0; q < corona.n; ++q) {
            int u = p < na ? -1 : (p - na) / nb, v = q < na ? -1 : (q - na) / nb; lng expect = 0;
            if (p < na && q < na) { expect = ma[p][q]; }
            else if (u >= 0 && u == v) { expect = mb[(p - na) % nb][(q - na) % nb]; }
            else if ((p < na && v == p) || (q < na && u == q)) { expect = 1; }
            require(have[p][q] == expect, "coronaProduct adjacency identity");}}
    if (directed) { return; }
    want.clear();
    for (int p = 0; p < na * nb; ++p) {
        for (int q = p + 1; q < na * nb; ++q) {
            int u = p / nb, x = p % nb, v = q / nb, y = q % nb;
            if (u != v && x != y && (ma[u][v] > 0) == (mb[x][y] > 0)) { want.emplace_back(p, q, 1); }}}
    same("modularProduct", modularProduct(a, b), na * nb, false, want);}
// Endpoint-pair matching by a count map: an edge of a matches while b still has an unused edge of its pair.
vector<char> matchedIn(const Edges &a, const Edges &b, bool directed) {
    map<pair<int, int>, int> left; vector<char> res;
    auto key = [&](int u, int v) { return directed ? pair(u, v) : pair(min(u, v), max(u, v)); };
    for (auto [u, v, w] : b) { ++left[key(u, v)]; }
    for (auto [u, v, w] : a) {
        int &count = left[key(u, v)]; res.push_back(count > 0);
        if (count > 0) { --count; }}
    return res;}
template<class A, class B>
void setOracles(const A &a, const Edges &ea, const B &b, const Edges &eb, int n, bool directed) {
    auto ka = matchedIn(ea, eb, directed), kb = matchedIn(eb, ea, directed);
    auto pick = [](const Edges &es, const vector<char> &matched, bool want) {
        Edges res;
        for (int i = 0; i < int(es.size()); ++i) { if (bool(matched[i]) == want) { res.push_back(es[i]); } }
        return res;};
    auto joined = [](Edges x, const Edges &y) { x.insert(x.end(), y.begin(), y.end()); return x;};
    auto u = graphUnion(a, b), i = graphIntersection(a, b), d = graphDifference(a, b), s = graphSymmetricDifference(a, b);
    same("graphUnion", u, n, directed, joined(ea, pick(eb, kb, false)));
    same("graphIntersection", i, n, directed, pick(ea, ka, true));
    same("graphDifference", d, n, directed, pick(ea, ka, false));
    same("graphSymmetricDifference", s, n, directed, joined(pick(ea, ka, false), pick(eb, kb, false)));
    auto ma = multiplicity(n, directed, ea), mb = multiplicity(n, directed, eb);
    auto mu = multiplicity(n, directed, listed(u)), mi = multiplicity(n, directed, listed(i));
    auto md = multiplicity(n, directed, listed(d)), ms = multiplicity(n, directed, listed(s));
    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            lng p = ma[x][y], q = mb[x][y];
            require(mu[x][y] == max(p, q) && mi[x][y] == min(p, q) && md[x][y] == max(p - q, lng(0))
                && ms[x][y] == abs(p - q), "set operation pair multiplicities max/min/excess/difference");}}}
Edges randomEdges(int n, int m, std::mt19937_64 &rng) {
    Edges res;
    for (int i = 0; n && i < m; ++i) { res.emplace_back(int(rng() % uint(n)), int(rng() % uint(n)), lng(rng() % 9) - 4); }
    return res;}
Graph build(int n, bool directed, const Edges &edges) {
    Graph res(n, directed);
    for (auto [u, v, w] : edges) { res.addEdge(u, v, w); }
    return res;}
template<class G>
void binaries(const G &g, int n, bool directed, const Edges &edges, std::mt19937_64 &rng) {
    for (int round = 0; round < factor_rounds; ++round) {
        int nh = int(rng() % 4); auto eh = randomEdges(nh, int(rng() % 5), rng); Graph h = build(nh, directed, eh);
        productOracles(g, n, edges, h, nh, eh, directed); productOracles(h, nh, eh, g, n, edges, directed);}
    // A second graph on the same vertices sharing some pairs, partly reversed, for the matching set operations.
    Edges eh;
    for (int i = n ? int(rng() % uint(edges.size() + 4)) : 0; i > 0; --i) {
        if (!edges.empty() && rng() % 3) {
            auto [u, v, w] = edges[rng() % edges.size()];
            if (rng() & 1) { swap(u, v); }
            eh.emplace_back(u, v, lng(rng() % 9) - 4);}
        else { eh.emplace_back(int(rng() % uint(n)), int(rng() % uint(n)), lng(rng() % 9) - 4); }}
    Graph h = build(n, directed, eh);
    setOracles(g, edges, h, eh, n, directed); setOracles(h, eh, g, edges, n, directed); setOracles(g, edges, g, edges, n, directed);}
template<class G>
void utilities(const G &g, int n, bool directed, const Edges &edges, std::mt19937_64 &rng) {
    lineOracle(g, directed, edges); simplifyOracle(g, n, directed, edges);
    transforms(g, n, directed, edges, rng); binaries(g, n, directed, edges, rng);
    vector<int> all(n); iota(all.begin(), all.end(), 0);
    inducedOracle(g, n, directed, edges, {}); inducedOracle(g, n, directed, edges, all);
    for (int round = 0; round < 3; ++round) {
        vector<int> vs;
        for (int u = 0; u < n; ++u) { if (rng() & 1) { vs.push_back(u); }}
        std::shuffle(vs.begin(), vs.end(), rng); inducedOracle(g, n, directed, edges, vs);}
    // Labels: identity, all merged into one vertex, and random with spare unused labels.
    for (int round = 0; round < 3; ++round) {
        int k = round == 0 ? n : round == 1 ? (n ? 1 : 0) : n + 2;
        vector<int> label(n);
        for (int u = 0; u < n; ++u) { label[u] = round == 0 ? u : round == 1 ? 0 : int(rng() % uint(k)); }
        auto c = contract(g, label, k); Edges want;
        for (auto [u, v, w] : edges) { want.emplace_back(label[u], label[v], w); }
        require(c.n == k, "contract vertex count"); storage(c, k, directed, want);
        simplifyOracle(c, k, directed, want);}}
// Builders: shuffled lists; directed keeps list order, undirected must equal the sorted (min, max, w) edge multiset.
void builders(int n, bool directed, const Edges &edges, std::mt19937_64 &rng) {
    vector<vector<pair<int, lng>>> adj(n);
    for (auto [u, v, w] : edges) {
        adj[u].push_back({v, w});
        if (!directed) { adj[v].push_back({u, w}); }}
    for (auto &list : adj) { std::shuffle(list.begin(), list.end(), rng); }
    vector<vector<int>> unit(n); Edges want, ones;
    for (int u = 0; u < n; ++u) {
        for (auto [v, w] : adj[u]) {
            unit[u].push_back(v);
            if (directed) { want.emplace_back(u, v, w); }}}
    if (!directed) {
        for (auto [u, v, w] : edges) { want.emplace_back(min(u, v), max(u, v), w); }
        sort(want.begin(), want.end());}
    for (auto [u, v, w] : want) { ones.emplace_back(u, v, 1); }
    storage(Graph::fromAdjacency(adj, directed), n, directed, want);
    storage(Graph::fromAdjacency(unit, directed), n, directed, ones);}
template<class G, class H>
void identical(const G &g, const H &h) {
    require(g.n == h.n && g.directed == h.directed && g.arcs.size() == h.arcs.size(), "snapshot shape");
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        auto e = g.arcs[a], f = h.arcs[a];
        require(e.from == f.from && e.to == f.to && e.id == f.id && e.rev == f.rev && e.w == f.w,
            "snapshot exact arc identity");}
    for (int u = 0; u < g.n; ++u) {
        vector<int> a(g[u].begin(), g[u].end()), b(h[u].begin(), h[u].end());
        require(a == b, "snapshot exact adjacency order");}}
void verify(int n, bool directed, const Edges &edges) {
    describe(n, directed, edges); Graph g(n, directed);
    for (int i = 0; i < int(edges.size()); ++i) {
        auto [u, v, w] = edges[i]; require(g.addEdge(u, v, w) == i, "returned insertion edge ID");}
    storage(g, n, directed, edges); CsrGraph csr(g); storage(csr, n, directed, edges); identical(g, csr);
    auto back = edges;
    if (directed) { for (auto &[u, v, w] : back) { swap(u, v); } }
    auto reversed = g.reverse(); auto reversed_csr = csr.reverse();
    storage(reversed, n, directed, back); storage(reversed_csr, n, directed, back); identical(reversed, reversed_csr);
    auto twice = reversed.reverse(); auto twice_csr = reversed_csr.reverse();
    storage(twice, n, directed, edges); identical(g, twice); identical(g, twice_csr);
    DenseGraph dense(g), heavy(g, true); storage(dense.graph, n, directed, edges); identical(g, dense.graph);
    require(int(dense.best.size()) == n && !dense.maximum && heavy.maximum, "dense row count/view flag");
    for (int u = 0; u < n; ++u) {
        require(int(dense.best[u].size()) == n && int(heavy.best[u].size()) == n, "dense column count");
        for (int v = 0; v < n; ++v) {
            int low = -1, high = -1;
            for (int a : g[u]) {
                if (g.arcs[a].to != v) { continue; }
                if (low == -1 || g.arcs[a].w < g.arcs[low].w) { low = a; }
                if (high == -1 || g.arcs[a].w > g.arcs[high].w) { high = a; }}
            require(dense.best[u][v] == low, "dense minimum/earliest tie/absent diagonal", std::to_string(low),
                std::to_string(dense.best[u][v]));
            require(heavy.best[u][v] == high, "dense maximum/earliest tie/absent diagonal", std::to_string(high),
                std::to_string(heavy.best[u][v]));}}
    static std::mt19937_64 pick(seed); utilities(g, n, directed, edges, pick); utilities(csr, n, directed, edges, pick);
    builders(n, directed, edges, pick);
    Graph copy = g, moved = std::move(copy); storage(moved, n, directed, edges);
    CsrGraph csr_copy = csr, csr_moved = std::move(csr_copy); identical(g, csr_moved);
    if (n) {
        g.addEdge(n - 1, 0); storage(csr, n, directed, edges); storage(dense.graph, n, directed, edges);
        storage(moved, n, directed, edges); require(g.edges.back().w == 1, "default edge weight");}}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 2 : 3; int cases = 0;
    for (int n = 0; n <= bound; ++n) { for (bool directed : {false, true}) {
        vector<pair<int, int>> slots;
        for (int u = 0; u < n; ++u) { for (int v = directed ? 0 : u; v < n; ++v) { slots.push_back({u, v}); }}
        for (uint mask = 0; mask < (1U << slots.size()); ++mask) {
            Edges edges;
            for (int i = 0; i < int(slots.size()); ++i) {
                if (mask >> i & 1U) { edges.emplace_back(slots[i].first, slots[i].second, i % 3 - 1); }}
            verify(n, directed, edges); ++cases;}}}
    std::cout << "PASS exhaustive storage/reverse/CSR/dense graphs=" << cases << '\n';}
struct OrderedLabel {
    int key;
    friend bool operator<(OrderedLabel a, OrderedLabel b) { return a.key < b.key; }
};
// Hand-computed results of named small graphs, independent of the brute oracles above.
void fixtures() {
    context = "named fixtures";
    Graph k2 = build(2, false, {{0, 1, 1}}), p3 = build(3, false, {{0, 1, 1}, {1, 2, 1}}), empty2(2), d2 = build(2, true, {});
    same("K2 x K2 Cartesian is C4", cartesianProduct(k2, k2), 4, false, {{0, 2, 1}, {1, 3, 1}, {0, 1, 1}, {2, 3, 1}});
    same("K2 x K2 tensor is 2K2", tensorProduct(k2, k2), 4, false, {{0, 3, 1}, {1, 2, 1}});
    same("K2 x K2 strong is K4", strongProduct(k2, k2), 4, false, {{0, 2, 1}, {1, 3, 1}, {0, 1, 1}, {2, 3, 1}, {0, 3, 1}, {1, 2, 1}});
    same("K2 modular K2", modularProduct(k2, k2), 4, false, {{0, 3, 1}, {1, 2, 1}});
    same("K2 modular 2K1", modularProduct(k2, empty2), 4, false, {});
    same("2K1 modular 2K1", modularProduct(empty2, empty2), 4, false, {{0, 3, 1}, {1, 2, 1}});
    same("Mycielskian of K2 is C5", mycielskian(build(2, false, {{0, 1, 5}})), 5, false,
        {{0, 1, 5}, {2, 1, 5}, {0, 3, 5}, {2, 4, 1}, {3, 4, 1}});
    same("Mycielskian of K1 is 2K1 plus K1-edge", mycielskian(Graph(1)), 3, false, {{1, 2, 1}});
    same("complement of P3", complementGraph(p3), 3, false, {{0, 2, 1}});
    same("complement of directed P3", complementGraph(build(3, true, {{0, 1, 1}, {1, 2, 1}})), 3, true,
        {{0, 2, 1}, {1, 0, 1}, {2, 0, 1}, {2, 1, 1}});
    same("complement ignores loops and multiplicity", complementGraph(build(2, false, {{0, 0, 3}, {1, 0, 2}, {0, 1, 2}})), 2, false, {});
    same("square of P4", graphPower(build(4, false, {{0, 1, 9}, {2, 1, 9}, {3, 2, 9}}), 2), 4, false,
        {{0, 1, 1}, {0, 2, 1}, {1, 2, 1}, {1, 3, 1}, {2, 3, 1}});
    same("directed cube of C3 reaches all", graphPower(build(3, true, {{0, 1, 1}, {1, 2, 1}, {2, 0, 1}}), 2), 3, true,
        {{0, 1, 1}, {0, 2, 1}, {1, 0, 1}, {1, 2, 1}, {2, 0, 1}, {2, 1, 1}});
    same("undirected loop layer of K1[2K1]", lexicographicProduct(build(1, false, {{0, 0, 4}}), empty2), 2, false,
        {{0, 0, 4}, {0, 1, 4}, {1, 1, 4}});
    same("K2[2K1] is C4", lexicographicProduct(k2, empty2), 4, false, {{0, 2, 1}, {0, 3, 1}, {1, 2, 1}, {1, 3, 1}});
    same("K1 corona K2 is a triangle", coronaProduct(Graph(1), build(2, false, {{0, 1, 3}})), 3, false, {{1, 2, 3}, {0, 1, 1}, {0, 2, 1}});
    same("rooted K2 by P3 at 1", rootedProduct(k2, p3, 1), 6, false, {{1, 4, 1}, {0, 1, 1}, {1, 2, 1}, {3, 4, 1}, {4, 5, 1}});
    same("directed K1 + K1 join", graphJoin(build(1, true, {}), build(1, true, {})), 2, true, {{0, 1, 1}, {1, 0, 1}});
    same("toDirected splits an undirected edge", toDirected(build(2, false, {{0, 1, 3}, {1, 1, 2}})), 2, true,
        {{0, 1, 3}, {1, 0, 3}, {1, 1, 2}, {1, 1, 2}});
    same("subdivided loop and parallel pair", subdivideEdges(build(2, true, {{1, 1, 7}, {0, 1, -2}, {0, 1, 3}})), 5, true,
        {{1, 2, 7}, {2, 1, 0}, {0, 3, -2}, {3, 1, 0}, {0, 4, 3}, {4, 1, 0}});
    require(edgeId(d2, 0, 1) == -1 && inDegrees(build(2, false, {{1, 1, 1}, {0, 1, 1}})) == vector<int>({1, 3}), "edgeId/inDegrees fixtures");
    for (bool directed : {false, true}) {
        Graph g = build(2, directed, {{0, 1, 5}, {1, 0, 6}, {0, 1, 7}}), h = build(2, directed, {{1, 0, 9}, {1, 1, 2}});
        Edges inter = directed ? Edges{{1, 0, 6}} : Edges{{0, 1, 5}};
        Edges diff = directed ? Edges{{0, 1, 5}, {0, 1, 7}} : Edges{{1, 0, 6}, {0, 1, 7}};
        Edges sym = diff; sym.emplace_back(1, 1, 2);
        same("union fixture", graphUnion(g, h), 2, directed, {{0, 1, 5}, {1, 0, 6}, {0, 1, 7}, {1, 1, 2}});
        same("intersection fixture", graphIntersection(g, h), 2, directed, inter);
        same("difference fixture", graphDifference(g, h), 2, directed, diff);
        same("symmetric difference fixture", graphSymmetricDifference(g, h), 2, directed, sym);}
    Graph c(3);
    require(c.addCycle({2}) == 0 && c.addPath({}) == 1 && c.addPath({1}) == 1 && c.addStar(1, {}) == 1 && c.addCycle({0, 1}) == 1,
        "builder return values on degenerate lists");
    same("one-vertex cycle is a loop, two-vertex cycle doubles", c, 3, false, {{2, 2, 1}, {0, 1, 1}, {1, 0, 1}});
    auto view = restrictedView(p3, {1, 1}, {0, 0});
    require(view.vertex == vector<int>({0, 1, 2}) && view.edge.empty() && view.graph.n == 3, "restrictedView duplicates");
    std::cout << "PASS named fixtures: subdivision, products, complement, power, Mycielskian, corona, rooted, join, set operations\n";}
void adapters() {
    fixtures();
    context = "input and label adapters";
    Edges weighted{{0, 0, std::numeric_limits<lng>::min()}, {0, 2, std::numeric_limits<lng>::max()}, {0, 2, -7}, {2, 1, 0}};
    for (bool directed : {false, true}) { for (bool weights : {false, true}) { for (int base : {0, 1}) {
        std::stringstream in; Edges want = weighted;
        for (auto &[u, v, w] : want) {
            in << u + base << ' ' << v + base;
            if (weights) { in << ' ' << w; } else { w = 1; }
            in << '\n';}
        in << "12345"; auto g = Graph::read(in, 3, int(want.size()), directed, weights, base);
        storage(g, 3, directed, want); int tail = 0; in >> tail;
        require(tail == 12345, "input adapter consumes exactly m records");}}}
    std::istringstream empty; auto g = Graph::read(empty, 0, 0); storage(g, 0, false, {});
    Graph default_graph; storage(default_graph, 0, false, {});
    // Vertex labels on CoordinateCompression; absent labels give -1.
    CoordinateCompression<lng> labels({7, -3, 7, std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), 0});
    vector<lng> expected{std::numeric_limits<lng>::min(), -3, 0, 7, std::numeric_limits<lng>::max()};
    require(labels.v == expected, "sorted deduplicated integer labels");
    for (int i = 0; i < int(expected.size()); ++i) {
        require(labels.index(expected[i]) == i && labels.decode(i) == expected[i], "integer label roundtrip");}
    require(labels.index(-4) == -1 && labels.index(8) == -1, "absent integer label");
    CoordinateCompression<string> names({"z", "", "a", "a"});
    require(names.v == vector<string>({"", "a", "z"}) && names.index("z") == 2 && names.decode(0).empty(),
        "string labels including empty label");
    CoordinateCompression<OrderedLabel> order({{9}, {3}, {9}, {-1}});
    require(order.v.size() == 3 && order.index({3}) == 1 && order.decode(2).key == 9 && order.index({4}) == -1,
        "strict ordering only, no equality operator required");
    CoordinateCompression<int> zero(vector<int>{}); require(zero.v.empty() && zero.index(0) == -1, "empty label set");
    auto copied = names; auto moved = std::move(copied);
    require(moved.v == names.v && moved.index("a") == 1, "label copy/move");
    // Builder fixtures: defaults, a loop listed twice, parallel edges listed in different orders, empty inputs.
    vector<vector<pair<int, lng>>> lists{{{1, 7}, {0, -2}, {1, 5}, {0, -2}, {2, 0}}, {{0, 5}, {0, 7}}, {{0, 0}}};
    storage(Graph::fromAdjacency(lists), 3, false, {{0, 0, -2}, {0, 1, 5}, {0, 1, 7}, {0, 2, 0}});
    storage(Graph::fromAdjacency(lists, true), 3, true, {{0, 1, 7}, {0, 0, -2}, {0, 1, 5}, {0, 0, -2}, {0, 2, 0},
        {1, 0, 5}, {1, 0, 7}, {2, 0, 0}});
    storage(Graph::fromAdjacency(vector<vector<int>>{{1, 1}, {0, 0, 1, 1}}), 2, false, {{0, 1, 1}, {0, 1, 1}, {1, 1, 1}});
    storage(Graph::fromAdjacency(vector<vector<int>>{}), 0, false, {});
    vector<vector<lng>> matrix{{0, 4, 0}, {4, -1, std::numeric_limits<lng>::min()}, {0, std::numeric_limits<lng>::min(), 0}};
    storage(Graph::fromMatrix(matrix), 3, true, {{0, 1, 4}, {1, 0, 4}, {1, 1, -1}, {1, 2, std::numeric_limits<lng>::min()},
        {2, 1, std::numeric_limits<lng>::min()}});
    storage(Graph::fromMatrix(matrix, false), 3, false, {{0, 1, 4}, {1, 1, -1}, {1, 2, std::numeric_limits<lng>::min()}});
    storage(Graph::fromMatrix(vector<vector<lng>>{}), 0, true, {});
    verify(3, false, {{0, 0, -5}, {0, 0, -5}, {0, 1, 3}, {0, 1, 3}, {1, 0, -9},
        {1, 2, std::numeric_limits<lng>::min()}, {2, 2, std::numeric_limits<lng>::max()}});
    verify(3, true, weighted);
    std::cout << "PASS defaults/copy/move/input/labels/builders/extreme weights/multiedges/loops\n";}
void randomCases(const string &mode) {
    std::mt19937_64 rng(seed); int count = mode == "quick" ? 70 : mode == "full" ? 500 : 4000;
    for (int test = 0; test < count; ++test) {
        int n = int(rng() % 25), m = n ? int(rng() % 120) : 0; Edges edges;
        for (int i = 0; i < m; ++i) {
            lng w = lng(rng() % 15) - 7;
            if (i % 19 == 0) { w = std::numeric_limits<lng>::min(); }
            if (i % 23 == 0) { w = std::numeric_limits<lng>::max(); }
            edges.emplace_back(int(rng() % n), int(rng() % n), w);}
        verify(n, bool(rng() & 1), edges);}
    std::cout << "PASS deterministic multigraph storage cases=" << count << '\n';}
void builderCases(const string &mode) {
    std::mt19937_64 rng(seed + 1); int count = mode == "quick" ? 100 : mode == "full" ? 1000 : 8000;
    for (int test = 0; test < count; ++test) {
        // Full-range weights that differ in one random byte exercise every radix pass and its skip.
        lng x = lng(rng()); int b = int(rng() % 8);
        vector<lng> pool{x, lng(ulng(x) ^ (rng() % 255 + 1) << (8 * b)), std::numeric_limits<lng>::min(),
            std::numeric_limits<lng>::max(), 0};
        int n = 1 + int(rng() % 8), m = int(rng() % 40); bool directed = bool(rng() & 1); Edges edges;
        for (int i = 0; i < m; ++i) { edges.emplace_back(int(rng() % n), int(rng() % n), pool[rng() % pool.size()]); }
        describe(n, directed, edges); builders(n, directed, edges, rng);
        vector<vector<lng>> w(n, vector<lng>(n));
        for (int u = 0; u < n; ++u) {
            for (int v = directed ? 0 : u; v < n; ++v) {
                w[u][v] = rng() % 3 ? 0 : pool[rng() % pool.size()];
                if (!directed) { w[v][u] = w[u][v]; }}}
        Edges want; vector<vector<pair<int, lng>>> adj(n);
        for (int u = 0; u < n; ++u) {
            for (int v = directed ? 0 : u; v < n; ++v) {
                if (!w[u][v]) { continue; }
                want.emplace_back(u, v, w[u][v]); adj[u].push_back({v, w[u][v]});
                if (!directed) { adj[v].push_back({u, w[u][v]}); }}}
        auto g = Graph::fromMatrix(w, directed); storage(g, n, directed, want);
        identical(g, Graph::fromAdjacency(adj, directed));}
    std::cout << "PASS builders: shuffled full-range adjacency lists and matrices cases=" << count << '\n';}
// A death probe that also names its assertion: stderr goes to a pipe, and abort passes on only when it holds want.
static int probe_err = -1, probe_pipe = -1;
static const char *probe_want = "";
void probeAbort(int) {
    char buf[4096]; ssize_t len = read(probe_pipe, buf, sizeof buf - 1);
    buf[len < 0 ? 0 : len] = 0;
    if (write(probe_err, buf, std::strlen(buf)) < 0 || !std::strstr(buf, probe_want)) { _exit(3); }}
void expectAssertion(const char *want) {
    int fds[2];
    if (pipe(fds) != 0 || fcntl(fds[0], F_SETFL, O_NONBLOCK) != 0) { throw std::runtime_error("probe pipe"); }
    probe_err = dup(2); dup2(fds[1], 2); probe_pipe = fds[0]; probe_want = want;
    std::signal(SIGABRT, probeAbort);}
void invalid(const string &probe) {
    Graph g(2); CsrGraph csr(g);
    using Lists = vector<vector<int>>; using Weighted = vector<vector<pair<int, lng>>>;
    if (probe == "negative-size") { Graph bad(-1); }
    if (probe == "add-negative") { g.addEdge(-1, 0); }
    if (probe == "add-end") { g.addEdge(0, 2); }
    if (probe == "add-past-end") { g.addEdge(3, 0); }
    if (probe == "row-negative") { (void)g[-1]; }
    if (probe == "row-end") { (void)g[2]; }
    if (probe == "row-past-end") { (void)g[3]; }
    if (probe == "csr-row-negative") { (void)csr[-1]; }
    if (probe == "csr-row-end") { (void)csr[2]; }
    if (probe == "csr-row-past-end") { (void)csr[3]; }
    if (probe == "adjacency-negative") { (void)Graph::fromAdjacency(Lists{{-1}}, true); }
    if (probe == "adjacency-end") { (void)Graph::fromAdjacency(Lists{{1}}, true); }
    if (probe == "adjacency-asymmetric") { (void)Graph::fromAdjacency(Lists{{1}, {}}); }
    if (probe == "adjacency-asymmetric-reverse") { (void)Graph::fromAdjacency(Lists{{}, {0}}); }
    if (probe == "adjacency-multiplicity") { (void)Graph::fromAdjacency(Lists{{1, 1}, {0}}); }
    if (probe == "adjacency-loop-once") { (void)Graph::fromAdjacency(Lists{{0}}); }
    if (probe == "adjacency-weight") { (void)Graph::fromAdjacency(Weighted{{{1, 2}}, {{0, 3}}}); }
    if (probe == "adjacency-loop-weight") { (void)Graph::fromAdjacency(Weighted{{{0, 5}, {0, 7}}}); }
    if (probe == "matrix-ragged") { (void)Graph::fromMatrix({{0, 1}, {0}}); }
    if (probe == "matrix-asymmetric") { (void)Graph::fromMatrix({{0, 1}, {0, 0}}, false); }
    if (probe == "matrix-asymmetric-lower") { (void)Graph::fromMatrix({{0, 0}, {1, 0}}, false); }
    if (probe == "induced-negative") { (void)inducedSubgraph(g, {-1}); }
    if (probe == "induced-end") { (void)inducedSubgraph(csr, {2}); }
    if (probe == "induced-past-end") { (void)inducedSubgraph(g, {3}); }
    if (probe == "induced-duplicate") { (void)inducedSubgraph(g, {1, 0, 1}); }
    if (probe == "contract-size") { (void)contract(g, {0}, 1); }
    if (probe == "contract-count") { (void)contract(Graph(0), {}, -1); }
    if (probe == "contract-negative") { (void)contract(g, {0, -1}, 2); }
    if (probe == "contract-end") { (void)contract(csr, {0, 2}, 2); }
    if (probe == "contract-past-end") { (void)contract(g, {0, 3}, 2); }
    Graph d(2, true), three(3);
    if (probe == "cycle-empty") { g.addCycle({}); }
    if (probe == "path-end") { g.addPath({0, 2}); }
    if (probe == "star-end") { g.addStar(2, {0}); }
    g.addEdge(0, 1);
    if (probe == "edge-subgraph-negative") { (void)edgeSubgraph(g, {-1}); }
    if (probe == "edge-subgraph-end") { (void)edgeSubgraph(g, {1}); }
    if (probe == "edge-subgraph-past-end") { (void)edgeSubgraph(g, {2}); }
    if (probe == "edge-subgraph-duplicate") { (void)edgeSubgraph(g, {0, 0}); }
    if (probe == "restricted-vertex-negative") { (void)restrictedView(g, {-1}, {}); }
    if (probe == "restricted-vertex-end") { (void)restrictedView(g, {2}, {}); }
    if (probe == "restricted-vertex-past-end") { (void)restrictedView(g, {3}, {}); }
    if (probe == "restricted-edge-negative") { (void)restrictedView(g, {}, {-1}); }
    if (probe == "restricted-edge-end") { (void)restrictedView(g, {}, {1}); }
    if (probe == "restricted-edge-past-end") { (void)restrictedView(g, {}, {2}); }
    if (probe == "edge-id-negative") { (void)edgeId(g, -1, 0); }
    if (probe == "edge-id-end") { (void)edgeId(g, 0, 2); }
    if (probe == "edge-id-past-end") { (void)edgeId(g, 0, 3); }
    if (probe == "power-negative") { (void)graphPower(g, -1); }
    if (probe == "union-direction") { (void)disjointUnion(g, d); }
    if (probe == "join-direction") { (void)graphJoin(d, g); }
    if (probe == "cartesian-direction") { (void)cartesianProduct(g, d); }
    if (probe == "tensor-direction") { (void)tensorProduct(d, g); }
    if (probe == "strong-direction") { (void)strongProduct(g, d); }
    if (probe == "lexicographic-direction") { (void)lexicographicProduct(d, g); }
    if (probe == "rooted-direction") { (void)rootedProduct(g, d, 0); }
    if (probe == "rooted-negative") { (void)rootedProduct(Graph(1), Graph(1), -1); }
    if (probe == "rooted-end") { (void)rootedProduct(Graph(1), Graph(1), 1); }
    if (probe == "corona-direction") { (void)coronaProduct(d, g); }
    if (probe == "corona-direction-reverse") { (void)coronaProduct(Graph(1), Graph(1, true)); }
    if (probe == "modular-directed") { (void)modularProduct(d, d); }
    if (probe == "modular-mixed") { (void)modularProduct(g, d); }
    if (probe == "product-overflow") { (void)cartesianProduct(Graph(65536), Graph(65537)); }
    if (probe == "corona-overflow") { (void)coronaProduct(Graph(50000), Graph(50000)); }
    // 65535 * 32768 <= INT_MAX < 65535 * 32769: only the corona size assertion may stop it.
    if (probe == "corona-overflow-window") { expectAssertion("coronaProduct"); (void)coronaProduct(Graph(65535), Graph(32768)); }
    if (probe == "mycielskian-directed") { (void)mycielskian(d); }
    if (probe == "set-union-size") { (void)graphUnion(g, three); }
    if (probe == "set-union-direction") { (void)graphUnion(g, d); }
    if (probe == "set-intersection-size") { (void)graphIntersection(three, g); }
    if (probe == "set-intersection-direction") { (void)graphIntersection(d, g); }
    if (probe == "set-difference-size") { (void)graphDifference(g, three); }
    if (probe == "set-difference-direction") { (void)graphDifference(g, d); }
    if (probe == "set-symmetric-size") { (void)graphSymmetricDifference(three, g); }
    if (probe == "set-symmetric-direction") { (void)graphSymmetricDifference(d, g); }
    std::istringstream empty;
    if (probe == "input-negative-size") { (void)Graph::read(empty, -1, 0); }
    if (probe == "input-negative-count") { (void)Graph::read(empty, 2, -1); }
    if (probe == "input-base") { (void)Graph::read(empty, 2, 0, false, false, 2); }
    if (probe == "input-base-negative") { (void)Graph::read(empty, 2, 0, false, false, -1); }
    map<string, string> input{{"input-truncated", "1"}, {"input-weight-truncated", "1 2"},
        {"input-low", "0 2"}, {"input-high", "1 3"}, {"input-extreme-low", "-9223372036854775808 1"},
        {"input-extreme-high", "1 9223372036854775807"}};
    if (input.count(probe)) {
        std::istringstream in(input[probe]); (void)Graph::read(in, 2, 1, false, probe == "input-weight-truncated");}
    if (probe_err != -1) { dup2(probe_err, 2); }
    throw std::runtime_error("unknown or unasserted precondition=" + probe);}
int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); }}
        // Quick pairs each graph with one random second factor instead of two (10 s quick bound).
        factor_rounds = mode == "quick" ? 1 : 2; adapters(); exhaustive(mode); randomCases(mode); builderCases(mode);
        std::cout << "PASS graph checks=" << checks << " seed=" << seed << '\n'; return 0;} catch (const std::exception &error) {
        std::cerr << "FAIL graph seed=" << seed << " mode=" << mode << ' ' << error.what() << '\n'; return 1;}}
