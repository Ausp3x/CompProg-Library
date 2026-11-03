#include "../../04-Graphs/13-shortest_path_advanced.hpp"

static ulng seed = 20260927;
static lng checks = 0, cases = 0;
static string context;
static const Graph *current = nullptr;
static vector<int> current_sources;
static std::mt19937_64 rng;
static map<string, lng> calls;
static const lng LMIN = std::numeric_limits<lng>::min(), LMAX = std::numeric_limits<lng>::max();
static const array<ulng, 8> MODS = {1, 2, 3, 7, 998244353, (ulng(1) << 61) - 1, (ulng(1) << 63) - 1, ulng(1) << 63};

string decimal(lll x) {
    if (x == PATH_INF) { return "INF"; }
    if (x == -PATH_INF) { return "-INF"; }
    if (!x) { return "0"; }
    bool negative = x < 0; if (negative) { x = -x; }
    string s;
    while (x) { s.push_back(char('0' + int(x % 10))); x /= 10; }
    if (negative) { s.push_back('-'); }
    reverse(s.begin(), s.end()); return s;}
// The message is a string or, on hot paths, a callable built only on failure; the graph and sources are printed only on failure.
template<class F>
void require(bool ok, const F &message) {
    ++checks;
    if (ok) { return; }
    cerr << "FAIL advanced shortest paths seed=" << seed << " case=" << cases << " ";
    if constexpr (std::is_invocable_v<F>) { cerr << message(); } else { cerr << message; }
    cerr << '\n' << context;
    if (current) {
        cerr << " n=" << current->n << " directed=" << current->directed << " sources=";
        for (int s : current_sources) { cerr << s << ' '; }
        cerr << "edges=";
        for (auto e : current->edges) { cerr << '(' << e.u << ',' << e.v << ',' << e.w << ')'; }}
    cerr << '\n'; std::exit(1);}
void describe(const Graph &g, const vector<int> &sources, const string &label) { ++cases; context = label; current = &g; current_sources = sources; }
string at(int v) { return " v=" + std::to_string(v); }

// state 0 unreachable, 1 finite, 2 reachable from a negative cycle; dist exact where state is 1, PATH_INF where 0.
struct Oracle {
    vector<lll> dist;
    vector<char> state;
};

// n - 1 Bellman-Ford passes, then every arc still relaxable seeds an explicit closure of negative-infinite vertices.
Oracle bellmanOracle(const Graph &g, const vector<int> &sources) {
    int n = g.n; Oracle res{vector<lll>(n, PATH_INF), vector<char>(n)};
    for (int s : sources) { res.dist[s] = 0; }
    for (int pass = 0; pass + 1 < n; ++pass) {
        for (auto e : g.arcs) { if (res.dist[e.from] != PATH_INF) { res.dist[e.to] = min(res.dist[e.to], res.dist[e.from] + e.w); }}}
    for (int v = 0; v < n; ++v) { res.state[v] = res.dist[v] != PATH_INF; }
    vector<int> stack;
    for (auto e : g.arcs) {
        if (res.dist[e.from] != PATH_INF && res.dist[e.from] + e.w < res.dist[e.to] && res.state[e.to] != 2) { res.state[e.to] = 2; stack.push_back(e.to); }}
    while (!stack.empty()) {
        int u = stack.back(); stack.pop_back();
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (res.state[v] != 2) { res.state[v] = 2; stack.push_back(v); }}}
    return res;}

// Validates the Bellman-Ford oracle by walk-length DP when every |w| <= big: kmax arcs expose any reachable negative cycle.
void validateOracle(const Graph &g, const vector<int> &sources, const Oracle &o, lng big) {
    int n = g.n;
    if (n == 0) { return; }
    int kmax = 2 * (n - 1) + n * int(3 * (n - 1) * big + 1);
    vector<lll> cur(n, PATH_INF), any(n, PATH_INF), simple(n, PATH_INF);
    for (int s : sources) { cur[s] = 0; }
    for (int k = 0; k <= kmax; ++k) {
        for (int v = 0; v < n; ++v) {
            any[v] = min(any[v], cur[v]);
            if (k < n) { simple[v] = any[v]; }}
        vector<lll> next(n, PATH_INF);
        for (auto e : g.arcs) { if (cur[e.from] != PATH_INF) { next[e.to] = min(next[e.to], cur[e.from] + e.w); }}
        cur = std::move(next);}
    for (int v = 0; v < n; ++v) {
        char want = simple[v] == PATH_INF ? 0 : any[v] < simple[v] ? 2 : 1;
        require(o.state[v] == want && (want != 1 || o.dist[v] == simple[v]), [&] { return "oracle self-check: walk-length DP" + at(v) + " bellman=" + decimal(o.dist[v]) + " walks=" + decimal(simple[v]); });}}

// Arcs join in weight order (descending when maximum); a vertex first reached records the joining weight, its bottleneck.
vector<lll> thresholdOracle(const Graph &g, const vector<int> &sources, bool maximum) {
    int n = g.n; vector<lll> res(n, maximum ? -PATH_INF : PATH_INF); vector<char> seen(n); vector<vector<int>> joined(n);
    vector<int> order(g.arcs.size()); iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return maximum ? g.arcs[a].w > g.arcs[b].w : g.arcs[a].w < g.arcs[b].w; });
    for (int s : sources) { seen[s] = true; res[s] = maximum ? PATH_INF : -PATH_INF; }
    for (int a : order) {
        auto e = g.arcs[a]; joined[e.from].push_back(a);
        if (!seen[e.from] || seen[e.to]) { continue; }
        vector<int> stack = {e.to}; seen[e.to] = true; res[e.to] = e.w;
        while (!stack.empty()) {
            int u = stack.back(); stack.pop_back();
            for (int b : joined[u]) {
                int v = g.arcs[b].to;
                if (!seen[v]) { seen[v] = true; res[v] = e.w; stack.push_back(v); }}}}
    return res;}

// Second bottleneck oracle: max/min label relaxation repeated until nothing changes.
vector<lll> fixpointOracle(const Graph &g, const vector<int> &sources, bool maximum) {
    int n = g.n; vector<lll> res(n, maximum ? -PATH_INF : PATH_INF);
    for (int s : sources) { res[s] = maximum ? PATH_INF : -PATH_INF; }
    for (bool changed = true; changed;) {
        changed = false;
        for (auto e : g.arcs) {
            if (res[e.from] == (maximum ? -PATH_INF : PATH_INF)) { continue; }
            lll c = maximum ? min(res[e.from], lll(e.w)) : max(res[e.from], lll(e.w));
            if (maximum ? c > res[e.to] : c < res[e.to]) { res[e.to] = c; changed = true; }}}
    return res;}

// Everything the checks compare against, computed once per graph and source vector from the oracles above.
struct Expected {
    const Graph *g;
    vector<int> sources, uniq;
    vector<char> mask;
    Oracle sp;
    vector<char> tight, bad;
    vector<int> tight_arcs;
    vector<lll> low, high;
    bool nonnegative = true;
    lng maxw = 0;
    bool second = false;
    vector<array<lll, 2>> loose, strict;
    vector<vector<lll>> from;
    vector<array<int, 2>> pairs;
    map<int, vector<lll>> from_dist, to_dist;
};

vector<lll> finiteDist(const Oracle &o) {
    vector<lll> res = o.dist;
    for (int v = 0; v < int(res.size()); ++v) { if (o.state[v] != 1) { res[v] = PATH_INF; }}
    return res;}
Graph reversed(const Graph &g) {
    Graph res(g.n, true);
    for (auto e : g.arcs) { res.addEdge(e.to, e.from, e.w); }
    return res;}

// Least two walk weights per vertex by saturated walk counts per (weight, vertex); weights up to (2n - 1) * C suffice.
bool secondOracle(Expected &x) {
    const Graph &g = *x.g; int n = g.n;
    lll bound = lll(max(2 * n - 1, 0)) * x.maxw;
    if ((bound + 1) * (n + lll(g.arcs.size()) + 1) > 4000000) { return false; }
    int b = int(bound);
    vector<vector<uint8_t>> cnt(size_t(b) + 1, vector<uint8_t>(n));
    for (int w = 0; w <= b; ++w) {
        auto &cur = cnt[w];
        if (w == 0) { for (int s : x.uniq) { cur[s] = 1; }}
        for (auto e : g.arcs) {
            if (e.w > 0 && e.w <= w) { cur[e.to] = uint8_t(min(2, cur[e.to] + cnt[size_t(w - e.w)][e.from])); }}
        // Zero arcs: push count increments until the saturated fixed point.
        vector<pair<int, int>> work;
        for (int v = 0; v < n; ++v) { if (cur[v]) { work.push_back({v, cur[v]}); }}
        while (!work.empty()) {
            auto [u, d] = work.back(); work.pop_back();
            for (int a : g[u]) {
                const auto &e = g.arcs[a];
                if (e.w != 0) { continue; }
                int nv = min(2, cur[e.to] + d);
                if (nv > cur[e.to]) { work.push_back({e.to, nv - cur[e.to]}); cur[e.to] = uint8_t(nv); }}}}
    x.loose.assign(n, {PATH_INF, PATH_INF}); x.strict.assign(n, {PATH_INF, PATH_INF});
    for (int v = 0; v < n; ++v) {
        for (int w = 0; w <= b; ++w) {
            if (!cnt[w][v]) { continue; }
            if (x.loose[v][0] == PATH_INF) {
                x.loose[v][0] = x.strict[v][0] = w;
                if (cnt[w][v] >= 2) { x.loose[v][1] = w; }
                continue;}
            if (x.loose[v][1] == PATH_INF) { x.loose[v][1] = w; }
            x.strict[v][1] = w; break;}}
    return true;}

// Tight arcs among finite vertices; bad marks vertices tight-reachable from a tight cycle, cross-checked by walk lengths.
void tightOracle(Expected &x) {
    const Graph &g = *x.g; int n = g.n, m = int(g.arcs.size()); const auto &o = x.sp;
    x.tight.assign(m, 0); x.bad.assign(n, 0);
    for (int a = 0; a < m; ++a) {
        auto e = g.arcs[a];
        if (o.state[e.from] == 1 && o.state[e.to] == 1 && o.dist[e.from] + e.w == o.dist[e.to]) { x.tight[a] = 1; x.tight_arcs.push_back(a); }}
    auto closure = [&](vector<int> stack, vector<char> &seen) {
        for (int v : stack) { seen[v] = true; }
        while (!stack.empty()) {
            int u = stack.back(); stack.pop_back();
            for (int a : g[u]) {
                if (x.tight[a] && !seen[g.arcs[a].to]) { seen[g.arcs[a].to] = true; stack.push_back(g.arcs[a].to); }}}};
    vector<int> cyclic;
    for (int u = 0; u < n; ++u) {
        vector<int> heads; vector<char> seen(n);
        for (int a : g[u]) { if (x.tight[a]) { heads.push_back(g.arcs[a].to); }}
        closure(heads, seen);
        if (seen[u]) { cyclic.push_back(u); }}
    closure(cyclic, x.bad);
    vector<char> cur(n), reach(n), longer(n);
    for (int s : x.uniq) { if (o.state[s] == 1 && o.dist[s] == 0) { cur[s] = 1; }}
    for (int k = 0; k < 2 * n; ++k) {
        vector<char> next(n);
        for (int v = 0; v < n; ++v) { reach[v] |= cur[v]; if (k >= n) { longer[v] |= cur[v]; }}
        for (int a : x.tight_arcs) { if (cur[g.arcs[a].from]) { next[g.arcs[a].to] = 1; }}
        cur = std::move(next);}
    for (int v = 0; v < n; ++v) {
        require(longer[v] == x.bad[v], "oracle self-check: tight cycle closure vs walks of n..2n-1 tight arcs" + at(v));
        require(bool(reach[v]) == (o.state[v] == 1), "oracle self-check: finite vertices are tight-reachable from a zero source" + at(v));}}

// Shortest walk counts modulo mod: tight walks of fewer than n arcs from sources at distance 0 (exact off the bad set).
vector<ulng> countOracle(const Expected &x, ulng mod) {
    const Graph &g = *x.g; int n = g.n; vector<ulng> cur(n), total(n);
    for (int s : x.uniq) { if (x.sp.state[s] == 1 && x.sp.dist[s] == 0) { cur[s] = 1 % mod; }}
    for (int k = 0; k < n; ++k) {
        vector<ulng> next(n);
        for (int v = 0; v < n; ++v) { total[v] = ulng((ulll(total[v]) + cur[v]) % mod); }
        for (int a : x.tight_arcs) { auto e = g.arcs[a]; next[e.to] = ulng((ulll(next[e.to]) + cur[e.from]) % mod); }
        cur = std::move(next);}
    return total;}

// Walks from distinct sources with at most 2n - 1 arcs: shortest counts and infinity, and the two least walk weights.
bool walkEnumeration(Expected &x, lng budget) {
    const Graph &g = *x.g; int n = g.n, cap = 2 * n - 1;
    vector<ulng> shortest(n); vector<char> longer(n);
    vector<array<lll, 2>> loose(n, {PATH_INF, PATH_INF}), strict(n, {PATH_INF, PATH_INF});
    auto dfs = [&](auto &&self, int u, lll w, int k) -> bool {
        if (--budget < 0) { return false; }
        if (x.sp.state[u] == 1 && w == x.sp.dist[u]) {
            if (k < n) { ++shortest[u]; } else { longer[u] = true; }}
        if (w < loose[u][0]) { loose[u][1] = loose[u][0]; loose[u][0] = w; }
        else if (w < loose[u][1]) { loose[u][1] = w; }
        if (w < strict[u][0]) { strict[u][1] = strict[u][0]; strict[u][0] = w; }
        else if (w > strict[u][0] && w < strict[u][1]) { strict[u][1] = w; }
        if (k == cap) { return true; }
        for (int a : g[u]) { if (!self(self, g.arcs[a].to, w + g.arcs[a].w, k + 1)) { return false; }}
        return true;};
    for (int s : x.uniq) { if (!dfs(dfs, s, 0, 0)) { return false; }}
    auto counts = countOracle(x, ulng(1) << 63);
    for (int v = 0; v < n; ++v) {
        if (x.sp.state[v] != 1) { continue; }
        require(bool(longer[v]) == bool(x.bad[v]), "oracle self-check: infinite count vs enumerated walks" + at(v));
        require(x.bad[v] || shortest[v] == counts[v], [&] { return "oracle self-check: count DP=" + std::to_string(counts[v]) + " enumerated=" + std::to_string(shortest[v]) + at(v); });}
    if (!x.nonnegative) { return true; }
    if (!x.second) { x.loose = loose; x.strict = strict; x.second = true; return true; }
    for (int v = 0; v < n; ++v) {
        require(x.loose[v] == loose[v] && x.strict[v] == strict[v], [&] { return "oracle self-check: second-shortest DP vs enumeration" + at(v)
            + " dp=" + decimal(x.loose[v][1]) + "/" + decimal(x.strict[v][1]) + " walks=" + decimal(loose[v][1]) + "/" + decimal(strict[v][1]);});}
    return true;}

Expected expect(const Graph &g, const vector<int> &sources, bool pairs) {
    int n = g.n; Expected x; x.g = &g; x.sources = sources; x.mask.assign(n, 0);
    for (int s : sources) { if (!x.mask[s]) { x.mask[s] = 1; x.uniq.push_back(s); }}
    x.sp = bellmanOracle(g, sources);
    for (auto e : g.arcs) { x.nonnegative &= e.w >= 0; x.maxw = max(x.maxw, e.w); }
    tightOracle(x);
    x.low = thresholdOracle(g, sources, false); x.high = thresholdOracle(g, sources, true);
    if (!x.nonnegative) { return x; }
    x.second = secondOracle(x);
    for (int s : x.uniq) { x.from.push_back(finiteDist(bellmanOracle(g, {s}))); }
    if (!pairs || n == 0) { return x; }
    if (n <= 6) {
        for (int s = 0; s < n; ++s) { for (int t = 0; t < n; ++t) { x.pairs.push_back({s, t}); }}}
    else {
        for (int i = 0; i < 8; ++i) { x.pairs.push_back({int(rng() % ulng(n)), int(rng() % ulng(n))}); }
        int s = int(rng() % ulng(n)); x.pairs.push_back({s, s});}
    Graph rev = reversed(g);
    for (auto [s, t] : x.pairs) {
        if (!x.from_dist.count(s)) { x.from_dist[s] = finiteDist(bellmanOracle(g, {s})); }
        if (!x.to_dist.count(t)) { x.to_dist[t] = finiteDist(bellmanOracle(rev, {t})); }}
    return x;}

// Follows parents with a step bound and checks path(g, v): an oriented walk from a source whose sum, max or min equals dist.
template<class G>
void checkForest(const G &g, const ShortestPathResult &res, const Expected &x, const string &name, int kind) {
    int n = g.n;
    for (int v = 0; v < n; ++v) {
        if (!res.finite(v)) { require(!res.path(g, v).exists, name + " path of a non-finite vertex" + at(v)); continue; }
        vector<int> chain = {v};
        for (int u = v; res.parent[u] != -1; u = res.parent[u]) {
            require(int(chain.size()) <= n, name + " parent cycle" + at(v));
            int a = res.parent_arc[u];
            require(0 <= a && a < int(g.arcs.size()) && g.arcs[a].to == u && g.arcs[a].from == res.parent[u], name + " parent_arc" + at(u));
            chain.push_back(res.parent[u]);}
        require(x.mask[chain.back()] && res.parent_arc[chain.back()] == -1, name + " forest root is not a source" + at(v));
        reverse(chain.begin(), chain.end());
        auto path = res.path(g, v);
        require(path.exists && path.vertices == chain && path.arcs.size() + 1 == chain.size(), name + " path(g, v) differs from the parent chain" + at(v));
        lll value = kind == 0 ? 0 : kind == 1 ? -PATH_INF : PATH_INF;
        for (int i = 0; i < int(path.arcs.size()); ++i) {
            const auto &e = g.arcs[path.arcs[i]];
            require(e.from == path.vertices[i] && e.to == path.vertices[i + 1], name + " path orientation" + at(v));
            value = kind == 0 ? value + e.w : kind == 1 ? max(value, lll(e.w)) : min(value, lll(e.w));}
        require(value == res.dist[v], [&] { return name + " witness value" + at(v) + " dist=" + decimal(res.dist[v]) + " witness=" + decimal(value); });}}

// Sum-weight engine against the oracle; bf, when given, is bellmanFord on the same graph and must agree exactly.
template<class G>
void checkSum(const G &g, const ShortestPathResult &res, const Expected &x, const string &name, const ShortestPathResult *bf) {
    ++calls[name]; int n = g.n; const auto &o = x.sp;
    require(int(res.dist.size()) == n && int(res.reachable.size()) == n && int(res.negative.size()) == n && int(res.parent.size()) == n
            && int(res.parent_arc.size()) == n, name + " dimensions");
    bool negative = false;
    for (int v = 0; v < n; ++v) {
        negative |= o.state[v] == 2;
        require(bool(res.reachable[v]) == (o.state[v] != 0) && bool(res.negative[v]) == (o.state[v] == 2) && res.finite(v) == (o.state[v] == 1),
                [&] { return name + " reachable/negative" + at(v) + " oracle state=" + std::to_string(o.state[v]); });
        require(o.state[v] != 1 || res.dist[v] == o.dist[v], [&] { return name + " dist" + at(v) + " expected=" + decimal(o.dist[v]) + " actual=" + decimal(res.dist[v]); });}
    const auto &c = res.negative_cycle;
    require(c.vertices.empty() == !negative, name + " negative_cycle presence");
    if (negative) {
        require(c.vertices.size() >= 2 && c.vertices.size() == c.arcs.size() + 1 && c.vertices.front() == c.vertices.back(), name + " negative_cycle shape");
        lll w = 0;
        for (int i = 0; i < int(c.arcs.size()); ++i) {
            int a = c.arcs[i];
            require(0 <= a && a < int(g.arcs.size()) && g.arcs[a].from == c.vertices[i] && g.arcs[a].to == c.vertices[i + 1], name + " negative_cycle orientation");
            require(o.state[c.vertices[i]] == 2, name + " negative_cycle vertex not negative-infinite");
            w += g.arcs[a].w;}
        require(w < 0, name + " negative_cycle weight=" + decimal(w));}
    if (bf) {
        for (int v = 0; v < n; ++v) {
            require(res.reachable[v] == bf->reachable[v] && res.negative[v] == bf->negative[v] && (!res.reachable[v] || res.dist[v] == bf->dist[v]),
                    [&] { return name + " differs from bellmanFord" + at(v) + " dist=" + decimal(res.dist[v]) + " bellmanFord=" + decimal(bf->dist[v]); });}
        require(c.vertices.empty() == bf->negative_cycle.vertices.empty(), name + " negative_cycle presence differs from bellmanFord");}
    checkForest(g, res, x, name, 0);}

template<class G>
void checkBottleneck(const G &g, const ShortestPathResult &res, const vector<lll> &want, const Expected &x, bool maximum, const string &name) {
    ++calls[name]; int n = g.n; lll absent = maximum ? -PATH_INF : PATH_INF;
    require(int(res.dist.size()) == n && int(res.reachable.size()) == n && int(res.negative.size()) == n && res.negative_cycle.vertices.empty(), name + " dimensions");
    for (int v = 0; v < n; ++v) {
        require(!res.negative[v] && bool(res.reachable[v]) == (want[v] != absent) && res.finite(v) == bool(res.reachable[v]), name + " reachable/negative" + at(v));
        require(!res.reachable[v] || res.dist[v] == want[v], [&] { return name + " dist" + at(v) + " expected=" + decimal(want[v]) + " actual=" + decimal(res.dist[v]); });}
    checkForest(g, res, x, name, maximum ? 2 : 1);}

template<class G>
void checkDag(const G &g, const ShortestPathDag &dag, const Expected &x, const string &name) {
    ++calls["shortestPathDag"]; int n = g.n, finite = 0, good = 0; vector<int> pos(n, -1);
    for (int v = 0; v < n; ++v) { finite += x.sp.state[v] == 1; good += x.sp.state[v] == 1 && !x.bad[v]; }
    require(dag.arcs == x.tight_arcs, name + " arcs differ from the tight arcs in arc order");
    require(int(dag.order.size()) == good, name + " order size=" + std::to_string(dag.order.size()) + " expected=" + std::to_string(good));
    for (int i = 0; i < int(dag.order.size()); ++i) {
        int v = dag.order[i];
        require(0 <= v && v < n && pos[v] == -1 && x.sp.state[v] == 1 && !x.bad[v], name + " order holds a wrong or repeated vertex" + at(v));
        pos[v] = i;}
    for (int a : x.tight_arcs) {
        auto e = g.arcs[a];
        require(pos[e.from] == -1 || pos[e.to] == -1 || pos[e.from] < pos[e.to], name + " order is not topological on arc " + std::to_string(a));}
    require(dag.acyclic == (good == finite), name + " acyclic");}

template<class G>
void checkCount(const G &g, const PathCountResult &res, const Expected &x, ulng mod, const ShortestPathResult &engine) {
    ++calls["countShortestPaths"]; int n = g.n; const auto &p = res.paths, &q = engine;
    require(p.dist == q.dist && p.reachable == q.reachable && p.negative == q.negative && p.parent == q.parent && p.parent_arc == q.parent_arc
            && p.negative_cycle.vertices == q.negative_cycle.vertices && p.negative_cycle.arcs == q.negative_cycle.arcs, "countShortestPaths paths differ from its engine");
    require(int(res.count.size()) == n && int(res.infinite.size()) == n, "countShortestPaths dimensions");
    auto want = countOracle(x, mod);
    for (int v = 0; v < n; ++v) {
        if (x.sp.state[v] != 1) { require(res.count[v] == 0 && !res.infinite[v], "countShortestPaths non-finite vertex" + at(v)); continue; }
        require(bool(res.infinite[v]) == bool(x.bad[v]), "countShortestPaths infinite" + at(v));
        require(x.bad[v] || res.count[v] == want[v], [&] { return "countShortestPaths count mod=" + std::to_string(mod) + at(v)
            + " expected=" + std::to_string(want[v]) + " actual=" + std::to_string(res.count[v]);});}}

// want[v][r] is the rank-r weight or PATH_INF; walks are checked as oriented, from a source, of weight dist; distinct asks rank 1 != rank 0.
template<class G>
void checkTopTwo(const G &g, const TopTwoPaths &res, const vector<array<lll, 2>> &want, const Expected &x, const string &name, bool distinct) {
    ++calls[name]; int n = g.n;
    require(int(res.dist.size()) == 2 * n && int(res.source.size()) == 2 * n && int(res.parent.size()) == 2 * n && int(res.parent_arc.size()) == 2 * n, name + " dimensions");
    for (int v = 0; v < n; ++v) {
        PathWitness walks[2];
        for (int r = 0; r < 2; ++r) {
            int s = 2 * v + r; bool present = want[v][r] != PATH_INF;
            require(res.exists(v, r) == present && (res.source[s] != -1) == present, [&] { return name + " exists" + at(v) + " r=" + std::to_string(r) + " expected=" + decimal(want[v][r]); });
            auto w = res.walk(g, v, r);
            if (!present) { require(!w.exists && w.vertices.empty() && w.arcs.empty(), name + " absent walk" + at(v)); continue; }
            require(res.dist[s] == want[v][r], [&] { return name + " dist" + at(v) + " r=" + std::to_string(r) + " expected=" + decimal(want[v][r]) + " actual=" + decimal(res.dist[s]); });
            require(w.exists && w.vertices.size() == w.arcs.size() + 1 && w.vertices.front() == res.source[s] && x.mask[res.source[s]] && w.vertices.back() == v,
                    name + " walk shape" + at(v) + " r=" + std::to_string(r));
            lll total = 0;
            for (int i = 0; i < int(w.arcs.size()); ++i) {
                int a = w.arcs[i];
                require(0 <= a && a < int(g.arcs.size()) && g.arcs[a].from == w.vertices[i] && g.arcs[a].to == w.vertices[i + 1], name + " walk orientation" + at(v));
                total += g.arcs[a].w;}
            require(total == res.dist[s], name + " walk weight=" + decimal(total) + " dist=" + decimal(res.dist[s]) + at(v));
            walks[r] = w;}
        if (distinct && walks[1].exists) {
            require(walks[0].vertices.front() != walks[1].vertices.front() || walks[0].arcs != walks[1].arcs, name + " rank 1 repeats the rank 0 walk" + at(v));}}}

template<class G>
void checkTop2(const G &g, const TopTwoPaths &res, const Expected &x) {
    int n = g.n; vector<int> index(n, -1); vector<array<lll, 2>> want(n, {PATH_INF, PATH_INF});
    for (int i = 0; i < int(x.uniq.size()); ++i) { index[x.uniq[i]] = i; }
    for (int v = 0; v < n; ++v) {
        lll best = PATH_INF;
        for (const auto &d : x.from) { best = min(best, d[v]); }
        want[v][0] = best;
        if (best == PATH_INF || int(res.source.size()) != 2 * n) { continue; }
        int s = res.source[2 * v];
        require(0 <= s && s < n && index[s] != -1 && x.from[index[s]][v] == best, "top2Dijkstra rank-0 source is not a nearest source" + at(v));
        for (int i = 0; i < int(x.uniq.size()); ++i) { if (x.uniq[i] != s) { want[v][1] = min(want[v][1], x.from[i][v]); }}
        if (want[v][1] == PATH_INF) { continue; }
        int t = res.source[2 * v + 1];
        require(t != s && 0 <= t && t < n && index[t] != -1 && x.from[index[t]][v] == want[v][1], "top2Dijkstra rank-1 source" + at(v));}
    checkTopTwo(g, res, want, x, "top2Dijkstra", false);}

template<class G>
void checkSt(const G &g, const StPath &res, int s, int t, lll want, const string &name) {
    ++calls[name]; string where = " s=" + std::to_string(s) + " t=" + std::to_string(t);
    if (want == PATH_INF) { require(!res.path.exists && res.dist == 0 && res.path.vertices.empty() && res.path.arcs.empty(), name + " unreachable" + where); return; }
    const auto &p = res.path;
    require(p.exists && res.dist == want, [&] { return name + " dist" + where + " expected=" + decimal(want) + " actual=" + decimal(res.dist); });
    require(p.vertices.size() == p.arcs.size() + 1 && p.vertices.front() == s && p.vertices.back() == t && (s != t || p.vertices.size() == 1), name + " path shape" + where);
    lll total = 0;
    for (int i = 0; i < int(p.arcs.size()); ++i) {
        int a = p.arcs[i];
        require(0 <= a && a < int(g.arcs.size()) && g.arcs[a].from == p.vertices[i] && g.arcs[a].to == p.vertices[i + 1], name + " path orientation" + where);
        total += g.arcs[a].w;}
    require(total == want, name + " path weight=" + decimal(total) + where);}

// Arbitrary heuristic values, for t and for vertices that cannot reach t.
lll arbitrary() {
    static const array<lll, 6> pool = {-100, lll(1) << 100, -(lll(1) << 100), 0, lll(LMAX), lll(LMIN)};
    return rng() % 2 ? pool[rng() % pool.size()] : lll(rng() % 41) - 20;}

template<class G>
void checkPairs(const G &g, const Expected &x) {
    int n = g.n;
    for (auto [s, t] : x.pairs) {
        lll want = x.from_dist.at(s)[t]; const auto &to = x.to_dist.at(t);
        checkSt(g, bidirectionalDijkstra(g, s, t), s, t, want, "bidirectionalDijkstra");
        vector<int> evaluated(n);
        auto once = [&](const string &label) {
            require(evaluated[t] == 0, "aStar evaluated h(t) " + label);
            for (int v = 0; v < n; ++v) { require(evaluated[v] <= 1, "aStar evaluated h twice " + label + at(v)); }
            fill(evaluated.begin(), evaluated.end(), 0);};
        checkSt(g, aStar(g, s, t, [&](int v) -> int { ++evaluated[v]; return 0; }), s, t, want, "aStar"); once("h=0");
        for (int variant = 0; variant < 2; ++variant) {
            vector<lll> h(n);
            for (int v = 0; v < n; ++v) {
                if (v == t || to[v] == PATH_INF) { h[v] = arbitrary(); }
                else if (variant == 0) { h[v] = to[v]; }
                else { h[v] = to[v] - lll(rng() % (ulng(min(to[v], lll(1) << 40)) + 4)); }}
            checkSt(g, aStar(g, s, t, [&](int v) { ++evaluated[v]; return h[v]; }), s, t, want, "aStar"); once(variant ? "random admissible" : "exact");}}}

// Every applicable operation on g (Graph or CsrGraph) against the expectations.
template<class G>
void checkOn(const G &g, const Expected &x, ulng mod) {
    const auto &src = x.sources; int n = g.n;
    auto bf = bellmanFord(g, src);
    checkSum(g, bf, x, "bellmanFord", nullptr);
    checkSum(g, spfa(g, src), x, "spfa", &bf);
    checkSum(g, desopoPape(g, src), x, "desopoPape", &bf);
    auto scaled = negativeSsspScaling(g, src);
    checkSum(g, scaled, x, "negativeSsspScaling", &bf);
    checkBottleneck(g, minimaxPath(g, src), x.low, x, false, "minimaxPath");
    checkBottleneck(g, widestPath(g, src), x.high, x, true, "widestPath");
    checkDag(g, shortestPathDag(g, bf), x, "shortestPathDag(bellmanFord)");
    checkDag(g, shortestPathDag(g, scaled), x, "shortestPathDag(negativeSsspScaling)");
    if (!x.nonnegative) { checkCount(g, countShortestPaths(g, src, mod), x, mod, bf); return; }
    auto dij = dijkstra(g, src);
    checkSum(g, dij, x, "dijkstra", nullptr);
    checkDag(g, shortestPathDag(g, dij), x, "shortestPathDag(dijkstra)");
    checkCount(g, countShortestPaths(g, src, mod), x, mod, dij);
    if (x.maxw < INT_MAX && lll(n + 1) * (x.maxw + 1) <= 4000000) { checkSum(g, dialDijkstra(g, src), x, "dialDijkstra", nullptr); }
    if (lll(max(n - 1, 0)) * x.maxw < lll(1) << 64) { checkSum(g, radixHeapDijkstra(g, src), x, "radixHeapDijkstra", nullptr); }
    if (x.second) {
        checkTopTwo(g, secondShortestPath(g, src), x.strict, x, "secondShortestPath strict", true);
        checkTopTwo(g, secondShortestPath(g, src, false), x.loose, x, "secondShortestPath non-strict", true);}
    checkTop2(g, top2Dijkstra(g, src), x);
    checkPairs(g, x);}

// Builds the expectations, self-checks the oracles when validate, then runs every operation on g and, when csr, on CsrGraph(g).
void checkGraph(const Graph &g, const vector<int> &sources, const string &label, bool csr, bool validate, ulng mod = 0) {
    describe(g, sources, label);
    auto x = expect(g, sources, true);
    if (mod == 0) { mod = MODS[size_t(cases) % MODS.size()]; }
    if (validate) {
        lng big = 0;
        for (auto e : g.arcs) { big = max(big, e.w == LMIN ? LMAX : abs(e.w)); }
        if (big <= 3 && g.n <= 5) { validateOracle(g, sources, x.sp, big); }
        if (g.n <= 5) { walkEnumeration(x, 50000); }
        auto low = fixpointOracle(g, sources, false), high = fixpointOracle(g, sources, true);
        require(low == x.low && high == x.high, "oracle self-check: threshold vs fixpoint bottleneck");}
    checkOn(g, x, mod);
    if (csr) { checkOn(CsrGraph(g), x, mod); }}

void fixtures() {
    for (bool directed : {false, true}) {
        checkGraph(Graph(0, directed), {}, "empty", true, true);
        Graph one(1, directed);
        for (const auto &src : vector<vector<int>>{{}, {0}, {0, 0}}) { checkGraph(one, src, "single", true, true); }
        for (lng w : {LMIN, lng(-1), lng(0), lng(1), LMAX}) {
            Graph loop(1, directed); loop.addEdge(0, 0, w); checkGraph(loop, {0}, "loop", true, true);
            Graph two(2, directed); two.addEdge(0, 0, w); two.addEdge(0, 1, 3);
            for (const auto &src : vector<vector<int>>{{0}, {1}, {1, 0}}) { checkGraph(two, src, "loop then arc", true, true); }}}
    // Hand-computed semantics, beside the oracle comparisons.
    Graph zero(1, true); zero.addEdge(0, 0, 0); describe(zero, {0}, "zero loop semantics");
    auto loose = secondShortestPath(zero, {0}, false), strict = secondShortestPath(zero, {0});
    require(loose.exists(0, 1) && loose.dist[1] == 0 && !strict.exists(0, 1), "zero loop: non-strict rank 1 is the loop, strict absent");
    auto counted = countShortestPaths(zero, {0}, 998244353); auto dag = shortestPathDag(zero, dijkstra(zero, {0}));
    require(counted.infinite[0] && !dag.acyclic && dag.order.empty() && dag.arcs == vector<int>({0}), "zero loop: infinitely many shortest walks");
    Graph und(2, false); und.addEdge(0, 1, 4); describe(und, {0}, "undirected edge semantics");
    auto back = secondShortestPath(und, {0});
    require(back.exists(0, 1) && back.dist[1] == 8 && back.exists(1, 1) && back.dist[3] == 12 && back.walk(und, 0, 1).arcs.size() == 2, "undirected edge: rank 1 goes and returns");
    Graph tie(3, false); tie.addEdge(0, 1, 1); tie.addEdge(1, 2, 1); describe(tie, {0, 2}, "top2 tie semantics");
    auto top = top2Dijkstra(tie, {0, 2});
    require(top.dist[2] == 1 && top.dist[3] == 1 && top.source[2] != top.source[3] && top.dist[1] == 2 && top.source[1] == 2, "top2: tie at the middle, other source at 0");
    require(!top2Dijkstra(tie, {0, 0}).exists(1, 1) && top2Dijkstra(tie, {}).source == vector<int>(6, -1), "top2: duplicate sources give no rank 1, no sources reach nothing");
    Graph parallel(2, true); parallel.addEdge(0, 1, 5); parallel.addEdge(0, 1, 5); parallel.addEdge(0, 1, 7); describe(parallel, {0}, "parallel arcs semantics");
    auto p_loose = secondShortestPath(parallel, {0}, false), p_strict = secondShortestPath(parallel, {0});
    require(p_loose.dist[3] == 5 && p_strict.dist[3] == 7 && p_loose.walk(parallel, 1, 0).arcs != p_loose.walk(parallel, 1, 1).arcs, "parallel arcs count as distinct walks");
    require(countShortestPaths(parallel, {0}, 998244353).count[1] == 2 && countShortestPaths(parallel, {0}, 1).count[1] == 0, "parallel arcs count twice, mod 1 gives 0");
    Graph reach(7, true);
    reach.addEdge(0, 1, 4); reach.addEdge(1, 2, -1); reach.addEdge(2, 1, 0); reach.addEdge(3, 0, 5);
    reach.addEdge(2, 4, 9); reach.addEdge(5, 6, -3); reach.addEdge(6, 5, 3); reach.addEdge(4, 6, 1);
    for (const auto &src : vector<vector<int>>{{0}, {3}, {5}, {6, 5}, {}, {4, 3}}) { checkGraph(reach, src, "negative cycle reachability", true, false); }
    describe(reach, {5}, "negative cycle unreachable from source 5");
    require(spfa(reach, {5}).negative_cycle.vertices.empty() && desopoPape(reach, {5}).dist[6] == -3 && negativeSsspScaling(reach, {5}).dist[6] == -3, "unreachable negative cycle ignored");
    Graph zeros(4, true);
    zeros.addEdge(0, 1, 0); zeros.addEdge(1, 0, 0); zeros.addEdge(1, 2, -1); zeros.addEdge(2, 3, 0); zeros.addEdge(3, 2, 1); zeros.addEdge(1, 1, 0);
    for (const auto &src : vector<vector<int>>{{0}, {2}, {3, 2}}) { checkGraph(zeros, src, "zero cycles", true, true); }
    Graph undirected(3, false);
    for (int w = 20; w >= 0; --w) { undirected.addEdge(0, 1, w); }
    undirected.addEdge(1, 2, 5); undirected.addEdge(2, 1, 5); undirected.addEdge(2, 2, 0);
    checkGraph(undirected, {0}, "parallel undirected", true, true); checkGraph(undirected, {2, 0, 2}, "parallel undirected", true, true);
    Graph split(6, false); split.addEdge(0, 1, 2); split.addEdge(1, 2, 3); split.addEdge(3, 4, 1); split.addEdge(4, 5, 1); split.addEdge(5, 3, 1);
    checkGraph(split, {0}, "disconnected", true, true); checkGraph(split, {4, 1}, "disconnected", true, true);
    auto first = spfa(reach, {0}), again = spfa(reach, {0}); auto copied = first;
    require(first.dist == again.dist && first.parent == again.parent && copied.negative == first.negative, "repeated and copied results");
    cout << "PASS fixtures: n=0/1, full-range loops, zero loops, parallel arcs, undirected edges, top2 ties, negative-cycle reachability, zero cycles, disconnected\n";}

// aStar reproducer (h(t) = -100 used to return 10), negative admissible h elsewhere, arbitrary h off the vertices reaching t.
void aStarFixtures() {
    Graph g(3, true); g.addEdge(0, 2, 10); g.addEdge(0, 1, 1); g.addEdge(1, 2, 1); describe(g, {0}, "aStar h(t) = -100 reproducer");
    auto res = aStar(g, 0, 2, [](int v) { return v == 2 ? lng(-100) : lng(0); });
    require(res.dist == 2 && res.path.vertices == vector<int>({0, 1, 2}), "aStar h(t)=-100 dist=" + decimal(res.dist) + " expected=2");
    checkSt(g, res, 0, 2, 2, "aStar");
    auto negative = aStar(g, 0, 2, [](int v) { return v == 0 ? lll(-(lll(1) << 100)) : v == 1 ? lll(-5) : lll(1) << 100; });
    checkSt(g, negative, 0, 2, 2, "aStar");
    Graph dead(5, true); dead.addEdge(0, 3, 1); dead.addEdge(3, 4, 0); dead.addEdge(0, 1, 4); dead.addEdge(1, 2, 4); dead.addEdge(0, 2, 9);
    describe(dead, {0}, "aStar dead ends with arbitrary h");
    for (lll big : {-(lll(1) << 100), lll(1) << 100, lll(LMIN)}) {
        auto r = aStar(dead, 0, 2, [&](int v) { return v == 3 || v == 4 ? big : v == 2 ? -big : lll(0); });
        checkSt(dead, r, 0, 2, 8, "aStar");}
    checkSt(dead, aStar(dead, 4, 0, [](int) { return 0; }), 4, 0, PATH_INF, "aStar");
    checkSt(dead, aStar(dead, 2, 2, [](int) { return -7; }), 2, 2, 0, "aStar");
    checkSt(dead, bidirectionalDijkstra(dead, 4, 0), 4, 0, PATH_INF, "bidirectionalDijkstra");
    checkSt(CsrGraph(dead), bidirectionalDijkstra(CsrGraph(dead), 3, 3), 3, 3, 0, "bidirectionalDijkstra");
    cout << "PASS aStar fixtures: h(t)=-100 reproducer, negative admissible h, arbitrary h at dead ends, unreachable t, s == t\n";}

// Diamond chains (2^k walks) and triple parallel arcs (3^k walks): closed-form counts for every modulus.
ulng powMod(ulng b, int e, ulng mod) {
    ulll res = 1 % mod, x = b % mod;
    for (; e > 0; e >>= 1, x = x * x % mod) { if (e & 1) { res = res * x % mod; }}
    return ulng(res);}
void countFixtures() {
    for (bool directed : {true, false}) {
        int k = 100; Graph g(3 * k + 1, directed);
        for (int i = 0; i < k; ++i) {
            g.addEdge(3 * i, 3 * i + 1, 1); g.addEdge(3 * i, 3 * i + 2, 1); g.addEdge(3 * i + 1, 3 * i + 3, 1); g.addEdge(3 * i + 2, 3 * i + 3, 1);}
        Graph chain(41, directed);
        for (int i = 0; i < 40; ++i) { for (int j = 0; j < 3; ++j) { chain.addEdge(i, i + 1, 5); }}
        auto x = expect(g, {0}, false), y = expect(chain, {0, 0}, false);
        for (ulng mod : MODS) {
            describe(g, {0}, "diamond chain closed form");
            auto res = countShortestPaths(g, {0}, mod);
            for (int i = 0; i <= k; ++i) { require(res.count[3 * i] == powMod(2, i, mod) && !res.infinite[3 * i], [&] { return "diamond count 2^" + std::to_string(i) + " mod=" + std::to_string(mod); }); }
            checkCount(g, res, x, mod, dijkstra(g, {0}));
            describe(chain, {0, 0}, "parallel chain closed form");
            auto par = countShortestPaths(chain, {0, 0}, mod);
            for (int i = 0; i <= 40; ++i) { require(par.count[i] == powMod(3, i, mod), [&] { return "parallel count 3^" + std::to_string(i) + " mod=" + std::to_string(mod); }); }
            checkCount(chain, par, y, mod, dijkstra(chain, {0, 0}));}
        checkGraph(g, {0}, "diamond chain", false, false, MODS.back());
        checkGraph(chain, {0}, "parallel chain", true, false, MODS.back());}
    cout << "PASS count fixtures: 2^100 diamond walks, 3^40 parallel walks, mods 1, 2, 3, 7, 998244353, 2^61-1, 2^63-1, 2^63, directed and undirected\n";}

// Pure D'Esopo-Pape arc scans (out-degree + 1 per pop), stopped once past limit.
lng papeScans(const Graph &g, int s, lng limit) {
    int n = g.n; vector<lll> d(n, PATH_INF); vector<char> state(n); deque<int> ready = {s}; d[s] = 0; state[s] = 1; lng scans = 0;
    while (!ready.empty() && scans <= limit) {
        int u = ready.front(); ready.pop_front(); state[u] = 2; scans += lng(g[u].size()) + 1;
        for (int a : g[u]) {
            auto e = g.arcs[a];
            if (d[u] + e.w < d[e.to]) {
                d[e.to] = d[u] + e.w;
                if (state[e.to] == 0) { ready.push_back(e.to); }
                else if (state[e.to] == 2) { ready.push_front(e.to); }
                state[e.to] = 1;}}}
    return scans;}
// Complete DAG with arcs i -> j by decreasing j and weight 2^(j - i), shifted by h[j] - h[i]; the shift keeps every pure-rule decision.
Graph papeFamily(int n, bool shifted) {
    Graph g(n, true); vector<lng> h(n);
    for (auto &x : h) { x = shifted ? lng(rng() % (1 << 21)) - (1 << 20) : 0; }
    for (int i = 0; i < n; ++i) { for (int j = n - 1; j > i; --j) { g.addEdge(i, j, (lng(1) << (j - i)) + h[j] - h[i]); }}
    return g;}
void papeFixtures(const string &mode) {
    vector<int> sizes = mode == "quick" ? vector<int>{32} : mode == "full" ? vector<int>{32, 40, 48} : vector<int>{32, 40, 48, 56, 62};
    for (int n : sizes) {
        for (bool shifted : {false, true}) {
            Graph g = papeFamily(n, shifted); lng budget = lng(n) * (lng(g.arcs.size()) + 1);
            describe(g, {0}, "D'Esopo-Pape exponential family");
            require(papeScans(g, 0, budget) > budget, "pure rule stays within the budget n=" + std::to_string(n));
            auto res = desopoPape(g, {0}), bf = bellmanFord(g, {0});
            require(res.dist == bf.dist && res.parent == bf.parent && res.parent_arc == bf.parent_arc, "desopoPape budget fallback is not bellmanFord");
            checkGraph(g, {0}, "D'Esopo-Pape exponential family", shifted, false);}}
    Graph cycle(3, true); cycle.addEdge(0, 1, 1); cycle.addEdge(1, 2, -3); cycle.addEdge(2, 0, 1); describe(cycle, {0}, "pure rule on a negative cycle");
    require(papeScans(cycle, 0, 3 * 4) > 3 * 4, "pure rule stops on a negative cycle");
    cout << "PASS D'Esopo-Pape fixtures: pure rule past n*(m+1) scans on n=" << sizes.front() << ".." << sizes.back() << ", plain and potential-shifted, negative cycle\n";}

// Dial at the largest bucket count that fits the cap comfortably, C = 0, and radix keys up to 2^64 - 1.
void dialRadixFixtures(const string &mode) {
    lng c = mode == "quick" ? (lng(1) << 20) - 1 : (lng(1) << 22) - 1;
    for (bool directed : {true, false}) {
        Graph g(4, directed); g.addEdge(0, 1, c); g.addEdge(1, 2, c); g.addEdge(0, 2, c - 1); g.addEdge(2, 3, 0); g.addEdge(3, 2, 0); g.addEdge(2, 2, c);
        describe(g, {0}, "dial large C"); auto x = expect(g, {0}, false);
        checkSum(g, dialDijkstra(g, {0}), x, "dialDijkstra", nullptr); checkSum(CsrGraph(g), dialDijkstra(CsrGraph(g), {0}), x, "dialDijkstra", nullptr);
        require(dialDijkstra(g, {1, 1}).dist[3] == c && dialDijkstra(g, {0}).dist[3] == c - 1, "dial large C hand-computed distances");
        Graph flat(5, directed); flat.addEdge(0, 1, 0); flat.addEdge(1, 0, 0); flat.addEdge(1, 2, 0); flat.addEdge(3, 3, 0);
        describe(flat, {0}, "dial C = 0"); auto y = expect(flat, {0}, false);
        checkSum(flat, dialDijkstra(flat, {0}), y, "dialDijkstra", nullptr);}
    lng third = lng(((ulll(1) << 64) - 1) / 3);
    vector<pair<Graph, vector<int>>> radix;
    Graph r1(1, true); r1.addEdge(0, 0, LMAX); radix.push_back({r1, {0}});
    Graph r2(2, false); r2.addEdge(0, 1, LMAX); radix.push_back({r2, {1}});
    Graph r3(3, true); r3.addEdge(0, 1, LMAX); r3.addEdge(1, 2, LMAX); r3.addEdge(0, 2, LMAX); radix.push_back({r3, {0}});
    Graph r4(4, true); r4.addEdge(0, 1, third); r4.addEdge(1, 2, third); r4.addEdge(2, 3, third); r4.addEdge(3, 3, 0); radix.push_back({r4, {0}});
    for (auto &[g, src] : radix) {
        describe(g, src, "radix near 2^64"); auto x = expect(g, src, false);
        checkSum(g, radixHeapDijkstra(g, src), x, "radixHeapDijkstra", nullptr);
        checkGraph(g, src, "radix near 2^64", true, true);}
    describe(r4, {0}, "radix key 2^64 - 1");
    require(radixHeapDijkstra(r4, {0}).dist[3] == lll(3) * third && lll(3) * third == lll((ulll(1) << 64) - 1), "radix distance 2^64 - 1");
    cout << "PASS dial/radix fixtures: C=" << c << ", C=0 with zero cycles, radix distances LMAX at n=3 and 2^64-1 at n=4\n";}

// INT64_MIN arcs, negative cycles of the form LMIN + LMAX, and unreachable negative parts for the scaling engine.
void scalingFixtures() {
    Graph chain(5, true);
    for (int i = 0; i < 4; ++i) { chain.addEdge(i, i + 1, LMIN); }
    chain.addEdge(4, 0, LMAX); chain.addEdge(4, 0, LMAX);
    for (const auto &src : vector<vector<int>>{{0}, {2, 4}}) { checkGraph(chain, src, "INT64_MIN chain", true, false); }
    Graph minus(2, true); minus.addEdge(0, 1, LMIN); minus.addEdge(1, 0, LMAX); checkGraph(minus, {0}, "LMIN + LMAX = -1 cycle", true, false);
    Graph plus(3, true); plus.addEdge(0, 1, LMIN); plus.addEdge(1, 2, LMAX); plus.addEdge(2, 0, LMAX); checkGraph(plus, {1, 0}, "LMIN + 2 LMAX cycle", true, false);
    Graph cut(6, true);
    cut.addEdge(0, 1, 3); cut.addEdge(1, 2, -2); cut.addEdge(3, 4, LMIN); cut.addEdge(4, 3, LMAX - 1); cut.addEdge(5, 1, LMIN); cut.addEdge(4, 0, 1);
    for (const auto &src : vector<vector<int>>{{0}, {3}, {5}, {}}) { checkGraph(cut, src, "unreachable negative part", true, false); }
    // Regression: scaling nonnegative arcs to 0 made the 0-1 cycle negative at t = 1 and returned bellmanFord's tie-breaking parent.
    Graph tie(4, true); tie.addEdge(0, 1, 5); tie.addEdge(1, 0, -2); tie.addEdge(0, 2, 5); tie.addEdge(2, 3, 1); tie.addEdge(1, 3, 1);
    checkGraph(tie, {0}, "scaling without fallback", true, false);
    auto scaled = negativeSsspScaling(tie, {0}); auto reference = bellmanFord(tie, {0});
    require(scaled.dist[3] == 6 && scaled.parent[3] == 1 && reference.parent[3] == 2, "scaling took the bellmanFord fallback on a feasible graph");
    cout << "PASS scaling fixtures: INT64_MIN chains, LMIN+LMAX cycles, unreachable negative parts, no-fallback regression\n";}

// Every graph on n vertices whose slots (ordered pairs, or unordered for undirected, loops when asked) take a weight or are absent.
void exhaustive(int n, bool directed, bool loops, const vector<lng> &weights, int csr_every) {
    vector<pair<int, int>> slots;
    for (int u = 0; u < n; ++u) {
        for (int v = directed ? 0 : u; v < n; ++v) { if (u != v || loops) { slots.push_back({u, v}); }}}
    int base = int(weights.size()) + 1, graphs = 1;
    for (int i = 0; i < int(slots.size()); ++i) { graphs *= base; }
    const vector<vector<int>> patterns = {{0}, {n - 1, 0}, {}, {0, 0}, {n - 1}};
    for (int code = 0; code < graphs; ++code) {
        Graph g(n, directed);
        for (int i = 0, x = code; i < int(slots.size()); ++i, x /= base) {
            if (x % base) { g.addEdge(slots[i].first, slots[i].second, weights[x % base - 1]); }}
        checkGraph(g, patterns[size_t(code) % patterns.size()], "exhaustive", code % csr_every == 0, true);}
    cout << "PASS exhaustive n=" << n << " directed=" << directed << " loops=" << loops << " weights=" << weights.size() << " graphs=" << graphs << '\n';}

lng randomWeight(int domain) {
    static const array<lng, 8> extremes = {LMIN, LMAX, -(lng(1) << 62), lng(1) << 62, -1, 0, 1, LMIN + 1};
    static const array<lng, 4> wide = {0, 1, lng(1) << 62, LMAX};
    if (domain == 0) { return lng(rng() % 7) - 3; }
    if (domain == 1) { return lng(rng() % 21); }
    if (domain == 2) { return extremes[rng() % extremes.size()]; }
    if (domain == 3) { return wide[rng() % wide.size()]; }
    return lng(rng() % 3);}
// Domains 0-5 as randomWeight; 6 and 7 take w = h[v] - h[u] + r >= h[v] - h[u] (negative arcs, no negative cycle), |h| up to 50 or 2^61.
Graph randomGraph(int n, bool directed, int m, int domain) {
    Graph g(n, directed);
    if (n == 0) { return g; }
    vector<lng> h(n);
    for (auto &x : h) { x = domain == 7 ? lng(rng() % (ulng(1) << 62)) - (lng(1) << 61) : lng(rng() % 101) - 50; }
    for (int j = 0; j < m; ++j) {
        int u = int(rng() % ulng(n)), v = int(rng() % ulng(n));
        g.addEdge(u, v, domain >= 6 ? h[v] - h[u] + lng(rng() % 4) : randomWeight(domain));}
    return g;}
vector<int> randomSources(int n) {
    vector<int> res;
    if (n == 0) { return res; }
    for (int k = int(rng() % 4); k > 0; --k) { res.push_back(int(rng() % ulng(n))); }
    return res;}

void randomTests(const string &mode) {
    int count = mode == "quick" ? 150 : mode == "full" ? 2000 : 10000;
    for (int it = 0; it < count; ++it) {
        int n = int(rng() % 9), domain = it % 8; bool directed = domain >= 6 || rng() % 2;
        Graph g = randomGraph(n, directed, int(rng() % ulng(3 * n + 3)), domain);
        checkGraph(g, randomSources(n), "random", it % 2 == 0, true);}
    int medium = mode == "quick" ? 8 : mode == "full" ? 80 : 400;
    for (int it = 0; it < medium; ++it) {
        int n = 10 + int(rng() % 31), domain = it % 8; bool directed = domain >= 6 || rng() % 2;
        Graph g = randomGraph(n, directed, int(rng() % ulng(4 * n)), domain);
        checkGraph(g, randomSources(n), "random medium", it % 4 == 0, false);}
    cout << "PASS random graphs n<=8: " << count << ", n in [10,40]: " << medium << '\n';}

// Larger graphs: potential-based signed weights (with an occasional extra negative arc that may close a negative cycle) and small nonnegative weights.
void largeTests(const string &mode) {
    int count = mode == "quick" ? 4 : mode == "full" ? 30 : 120, low = mode == "quick" ? 20 : 50, span = mode == "quick" ? 41 : 251;
    for (int it = 0; it < count; ++it) {
        int n = low + int(rng() % ulng(span)), kind = it % 5;
        int domain = kind == 0 ? 7 : kind == 1 ? 6 : kind == 2 ? 2 : kind == 3 ? 4 : 1;
        Graph g = randomGraph(n, kind <= 1 || rng() % 2, n + int(rng() % ulng(3 * n)), domain);
        if (kind <= 1 && it % 3 == 0) { g.addEdge(int(rng() % ulng(n)), int(rng() % ulng(n)), -lng(rng() % 200)); }
        vector<int> src = randomSources(n);
        if (src.empty()) { src.push_back(0); }
        checkGraph(g, src, "large", it % 3 == 0, false);}
    cout << "PASS large graphs: " << count << " with n in [" << low << "," << low + span - 1 << "]\n";}

void invalid(const string &which) {
    Graph g(2, true); g.addEdge(0, 1, 2);
    Graph bad = g; bad.addEdge(1, 0, -1);
    auto zero = [](int) { return 0; };
    if (which == "dial-negative") { (void)dialDijkstra(bad, {0}); }
    if (which == "dial-int-max") { g.addEdge(1, 0, INT_MAX); (void)dialDijkstra(g, {0}); }
    if (which == "radix-negative") { (void)radixHeapDijkstra(bad, {0}); }
    if (which == "radix-bound") { Graph h(4, true); h.addEdge(0, 1, lng(((ulll(1) << 64) - 1) / 3) + 1); (void)radixHeapDijkstra(h, {0}); }
    if (which == "astar-negative") { (void)aStar(bad, 0, 1, zero); }
    if (which == "astar-range") { (void)aStar(g, 0, 2, zero); }
    if (which == "bidirectional-negative") { (void)bidirectionalDijkstra(bad, 0, 1); }
    if (which == "bidirectional-range") { (void)bidirectionalDijkstra(g, -1, 1); }
    if (which == "second-negative") { (void)secondShortestPath(bad, {0}); }
    if (which == "second-source-range") { (void)secondShortestPath(g, {2}); }
    if (which == "top2-negative") { (void)top2Dijkstra(bad, {0}); }
    if (which == "top2-source-range") { (void)top2Dijkstra(g, {0, 2}); }
    if (which == "count-mod-zero") { (void)countShortestPaths(g, {0}, 0); }
    if (which == "count-mod-big") { (void)countShortestPaths(g, {0}, (ulng(1) << 63) + 1); }
    if (which == "exists-rank") { (void)secondShortestPath(g, {0}).exists(0, 2); }
    if (which == "walk-size") { (void)secondShortestPath(g, {0}).walk(Graph(3, true), 1, 0); }
    if (which == "dag-size") { (void)shortestPathDag(g, ShortestPathResult(3)); }
    std::exit(2);}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--invalid") { invalid(argv[++i]); }
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { seed = std::stoull(argv[++i]); }}
    rng.seed(seed);
    fixtures(); aStarFixtures(); countFixtures(); papeFixtures(mode); dialRadixFixtures(mode); scalingFixtures();
    const vector<lng> signed3 = {-1, 0, 1}, signed2 = {-1, 1}, small3 = {0, 1, 2};
    if (mode == "quick") { exhaustive(2, true, true, signed3, 3); exhaustive(2, false, true, signed3, 3); }
    else {
        exhaustive(2, true, true, signed3, 1); exhaustive(2, false, true, signed3, 1); exhaustive(2, true, true, small3, 1);
        exhaustive(3, true, false, signed3, 5); exhaustive(3, false, true, signed3, 5); exhaustive(3, true, false, small3, 5);
        if (mode == "stress") { exhaustive(3, true, true, signed2, 7); exhaustive(3, true, true, {0, 1}, 7); }}
    randomTests(mode); largeTests(mode);
    cout << "PASS operations:";
    for (const auto &[name, k] : calls) { cout << ' ' << name << '=' << k; }
    cout << "\nPASS advanced shortest paths mode=" << mode << " seed=" << seed << " cases=" << cases << " checks=" << checks << '\n';}
