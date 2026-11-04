#pragma once

#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/31-radixheap.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "05-shortest_path.hpp"
#include "08-scc.hpp"

// Sentinel above every exact path total and bottleneck of this header and 12-all_pairs_shortest_path.hpp.
inline constexpr lll PATH_INF = lll(1) << 120;

// T: O(1), M: O(n); exists is false when t is unreachable.
struct StPath {
    lll dist = 0;
    PathWitness path;
};

// T: O(1) query, O(out) walk, M: O(n); state 2 * v + r holds rank r in {0, 1} of v, source = -1 when absent.
struct TopTwoPaths {
    vector<lll> dist;
    vector<int> source, parent, parent_arc;

    bool exists(int v, int r) const {
        assert(0 <= v && 2 * v < int(source.size()) && (r == 0 || r == 1)); return source[2 * v + r] != -1;}
    template<class G>
    PathWitness walk([[maybe_unused]] const G &g, int v, int r) const {
        assert(2 * g.n == int(source.size()));
        if (!exists(v, r)) { return {}; }
        PathWitness out; out.exists = true;
        for (int x = 2 * v + r; x != -1; x = parent[x]) {
            out.vertices.push_back(x / 2);
            if (parent_arc[x] != -1) { out.arcs.push_back(parent_arc[x]); }}
        reverse(out.vertices.begin(), out.vertices.end()); reverse(out.arcs.begin(), out.arcs.end());
        return out;}
};

// T: O(1), M: O(n + m); arcs on some shortest walk, order a topological order of the vertices not reachable from a tight cycle.
struct ShortestPathDag {
    bool acyclic;
    vector<int> arcs, order;
};

// T: O(1), M: O(n); count[v] modulo mod is meaningful where paths.finite(v) and !infinite[v].
struct PathCountResult {
    ShortestPathResult paths;
    vector<ulng> count;
    vector<char> infinite;
};

namespace shortest_path_advanced_detail {
    // T: O(m), M: O(1); asserts every weight >= 0 and reports it for the bounded NDEBUG return.
    template<class G>
    bool nonnegative(const G &g) {
        bool ok = std::all_of(g.arcs.begin(), g.arcs.end(), [](const GraphArc &e) { return e.w >= 0; });
        assert(ok); return ok;}

    // T: O(k + (n + m) * log(n + m + 1)), M: O(n + m); reduced weights w + p[from] - p[to] >= 0 on arcs that avoid blocked heads.
    template<class G>
    ShortestPathResult dijkstraPotential(const G &g, const vector<int> &sources, const vector<lll> &p, const vector<char> &blocked = {}) {
        ShortestPathResult out(g.n, sources);
        priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<pair<lll, int>>> ready;
        for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { ready.push({-p[u], u}); }}
        while (!ready.empty()) {
            auto [d, u] = ready.top(); ready.pop();
            if (d != out.dist[u] - p[u]) { continue; }
            for (int a : g[u]) {
                const auto &e = g.arcs[a];
                if ((blocked.empty() || !blocked[e.to]) && shortest_path_detail::relax(out, e, a)) { ready.push({out.dist[e.to] - p[e.to], e.to}); }}}
        return out;}

    // T: O(1), M: O(1); arc a joins finite vertices and is tight.
    template<class G>
    bool tight(const G &g, const ShortestPathResult &sp, int a) {
        const auto &e = g.arcs[a];
        return sp.reachable[e.from] && !sp.negative[e.from] && sp.reachable[e.to] && !sp.negative[e.to] && sp.dist[e.from] + e.w == sp.dist[e.to];}

    // T: O(n + (k + m) * log(k + m + 1)), M: O(n + m + k); accept(out, x, d, s) decides whether an entry beside rank-0 state x becomes rank 1.
    template<class G, class F>
    TopTwoPaths topTwo(const G &g, const vector<int> &sources, F accept) {
        TopTwoPaths out{vector<lll>(2 * size_t(g.n)), vector<int>(2 * size_t(g.n), -1), vector<int>(2 * size_t(g.n), -1), vector<int>(2 * size_t(g.n), -1)};
        using Entry = tuple<lll, int, int, int, int>;
        priority_queue<Entry, vector<Entry>, std::greater<Entry>> ready;
        for (int s : sources) { assert(0 <= s && s < g.n); ready.push({0, s, -1, -1, s}); }
        while (!ready.empty()) {
            auto [d, v, from, a, s] = ready.top(); ready.pop();
            int x = 2 * v;
            if (out.source[x] != -1) {
                if (out.source[x + 1] != -1 || !accept(out, x, d, s)) { continue; }
                ++x;}
            out.dist[x] = d; out.source[x] = s; out.parent[x] = from; out.parent_arc[x] = a;
            for (int b : g[v]) {
                const auto &e = g.arcs[b];
                if (out.source[2 * e.to + 1] == -1) { ready.push({d + e.w, e.to, x, b, s}); }}}
        return out;}

    // T: O(k + (n + m) * log(n + m + 1)), M: O(n + m); maximum maximizes the minimum arc weight, sources hold the empty-walk sentinel.
    template<class G>
    ShortestPathResult bottleneck(const G &g, const vector<int> &sources, bool maximum) {
        ShortestPathResult out(g.n, sources);
        priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<pair<lll, int>>> ready;
        for (int u = 0; u < g.n; ++u) {
            if (out.reachable[u]) { out.dist[u] = maximum ? PATH_INF : -PATH_INF; ready.push({-PATH_INF, u}); }}
        while (!ready.empty()) {
            auto [d, u] = ready.top(); ready.pop();
            if (d != (maximum ? -out.dist[u] : out.dist[u])) { continue; }
            for (int a : g[u]) {
                const auto &e = g.arcs[a];
                lll c = maximum ? min(out.dist[u], lll(e.w)) : max(out.dist[u], lll(e.w));
                if (!out.reachable[e.to] || (maximum ? c > out.dist[e.to] : c < out.dist[e.to])) {
                    out.reachable[e.to] = true; out.dist[e.to] = c; out.parent[e.to] = u; out.parent_arc[e.to] = a;
                    ready.push({maximum ? -c : c, e.to});}}}
        return out;}

    // T: O(sqrt(n) * (n + m)), M: O(n + m); turns reduced weights >= -1 into >= 0 by changing p, false on a negative cycle.
    template<class G, class W>
    bool refine(const G &g, const vector<char> &live, vector<lll> &p, W w) {
        int n = g.n, need = INT_MAX; Graph h(n, true);
        while (true) {
            h.edges.clear(); h.arcs.clear();
            for (auto &l : h.adj) { l.clear(); }
            for (int a = 0; a < int(g.arcs.size()); ++a) {
                lll c = w(a);
                if (live[g.arcs[a].from] && c <= 0) { h.addEdge(g.arcs[a].from, g.arcs[a].to, lng(c)); }}
            auto scc = tarjanScc(h);
            const auto &comp = scc.component;
            vector<char> improvable(n); int k = 0;
            for (const auto &e : h.arcs) {
                if (e.w < 0) {
                    if (comp[e.from] == comp[e.to]) { return false; }
                    k += !improvable[e.to]; improvable[e.to] = true;}}
            if (k == 0) { return true; }
            if (k > need) { return false; }
            int c = int(scc.groups.size());
            vector<int> d(c), pred(c, -1);
            for (int x = 0; x < c; ++x) {
                for (int u : scc.groups[x]) {
                    for (int a : h[u]) {
                        int y = comp[h.arcs[a].to];
                        if (y != x && d[x] + int(h.arcs[a].w) < d[y]) { d[y] = d[x] + int(h.arcs[a].w); pred[y] = a; }}}}
            vector<int> cnt(size_t(n) + 1);
            for (int v = 0; v < n; ++v) { cnt[-d[comp[v]]] += improvable[v]; }
            int j = int(std::max_element(cnt.begin(), cnt.end()) - cnt.begin());
            if (lng(cnt[j]) * cnt[j] >= k) {
                for (int v = 0; v < n; ++v) { if (d[comp[v]] <= -j) { --p[v]; }}
                need = k - cnt[j]; continue;}
            vector<int> ys;
            for (int x = int(std::min_element(d.begin(), d.end()) - d.begin()); pred[x] != -1; x = comp[h.arcs[pred[x]].from]) {
                if (h.arcs[pred[x]].w < 0) { ys.push_back(h.arcs[pred[x]].to); }}
            reverse(ys.begin(), ys.end());
            int t = int(ys.size());
            vector<int> label(n);
            vector<vector<int>> bucket(size_t(t) + 1);
            for (int i = 0; i < t; ++i) { label[ys[i]] = -(i + 1); bucket[t - i - 1].push_back(ys[i]); }
            // Dial over max(reduced weight, 0): label[v] = min(0, min_i(-i + dist(y_i, v))).
            for (int b = 0; b < t; ++b) {
                while (!bucket[b].empty()) {
                    int u = bucket[b].back(); bucket[b].pop_back();
                    if (label[u] != b - t) { continue; }
                    for (int a : g[u]) {
                        lll nl = label[u] + max(w(a), lll(0));
                        int v = g.arcs[a].to;
                        if (nl < label[v]) { label[v] = int(nl); bucket[size_t(nl + t)].push_back(v); }}}}
            for (int v = 0; v < n; ++v) { p[v] += label[v]; }
            need = k - t;}}
} // namespace shortest_path_advanced_detail

// T: O(n * m + n + k), M: O(n); FIFO label correcting, signed weights; a vertex queued more than n times reruns bellmanFord.
template<class G>
ShortestPathResult spfa(const G &g, const vector<int> &sources) {
    ShortestPathResult out(g.n, sources); queue<int> ready; vector<char> queued(g.n); vector<int> cnt(g.n);
    for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { ready.push(u); queued[u] = true; cnt[u] = 1; }}
    while (!ready.empty()) {
        int u = ready.front(); ready.pop(); queued[u] = false;
        for (int a : g[u]) {
            const auto &e = g.arcs[a];
            if (shortest_path_detail::relax(out, e, a) && !queued[e.to]) {
                if (++cnt[e.to] > g.n) { return bellmanFord(g, sources); }
                ready.push(e.to); queued[e.to] = true;}}}
    return out;}

// T: O(n * m + n + k), M: O(n); D'Esopo-Pape deque order, signed weights; past n * (m + 1) arc scans it reruns bellmanFord.
template<class G>
ShortestPathResult desopoPape(const G &g, const vector<int> &sources) {
    ShortestPathResult out(g.n, sources); deque<int> ready; vector<char> state(g.n);
    lng budget = lng(g.n) * (lng(g.arcs.size()) + 1);
    for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { ready.push_back(u); state[u] = 1; }}
    while (!ready.empty()) {
        int u = ready.front(); ready.pop_front(); state[u] = 2;
        if ((budget -= lng(g[u].size()) + 1) < 0) { return bellmanFord(g, sources); }
        for (int a : g[u]) {
            const auto &e = g.arcs[a];
            if (shortest_path_detail::relax(out, e, a)) {
                if (state[e.to] == 0) { ready.push_back(e.to); }
                else if (state[e.to] == 2) { ready.push_front(e.to); }
                state[e.to] = 1;}}}
    return out;}

// T: O(k + m + n * C) for maximum weight C, M: O(n + m + C); weights in [0, 2^31 - 1).
template<class G>
ShortestPathResult dialDijkstra(const G &g, const vector<int> &sources) {
    lng c = 0; bool ok = true;
    for (const auto &e : g.arcs) { ok &= 0 <= e.w && e.w < INT_MAX; c = max(c, e.w); }
    assert(ok);
    if (!ok) { return ShortestPathResult(g.n, sources); }
    ShortestPathResult out(g.n, sources); vector<vector<int>> bucket(size_t(c) + 1); vector<char> done(g.n); lng pending = 0;
    for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { bucket[0].push_back(u); ++pending; }}
    for (lng cur = 0; pending > 0; ++cur) {
        auto &b = bucket[size_t(cur % (c + 1))];
        while (!b.empty()) {
            int u = b.back(); b.pop_back(); --pending;
            if (done[u] || out.dist[u] != cur) { continue; }
            done[u] = true;
            for (int a : g[u]) {
                const auto &e = g.arcs[a];
                if (shortest_path_detail::relax(out, e, a)) { bucket[size_t((cur + e.w) % (c + 1))].push_back(e.to); ++pending; }}}}
    return out;}

// T: O(k + m + n * b), b = 64, M: O(n + b); nonnegative weights with (n - 1) * max weight < 2^64.
template<class G>
ShortestPathResult radixHeapDijkstra(const G &g, const vector<int> &sources) {
    lng c = 0; bool ok = true;
    for (const auto &e : g.arcs) { ok &= e.w >= 0; c = max(c, e.w); }
    ok &= lll(max(g.n - 1, 0)) * c < lll(1) << 64;
    assert(ok);
    if (!ok) { return ShortestPathResult(g.n, sources); }
    ShortestPathResult out(g.n, sources); RadixHeapArray<ulng> ready(g.n);
    for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { ready.push(u, 0); }}
    while (!ready.empty()) {
        int u = ready.top(); ready.pop();
        for (int a : g[u]) {
            const auto &e = g.arcs[a];
            if (shortest_path_detail::relax(out, e, a)) {
                if (ready.contains(e.to)) { ready.decreaseKey(e.to, ulng(out.dist[e.to])); }
                else { ready.push(e.to, ulng(out.dist[e.to])); }}}}
    return out;}

// T: O((n + m) * log(n + m + 1)) for consistent h, exponential for admissible h, M: O(n + m); weights >= 0, h(v) <= dist(v, t).
template<class G, class H>
StPath aStar(const G &g, int s, int t, H h) {
    assert(0 <= s && s < g.n && 0 <= t && t < g.n);
    if (!shortest_path_advanced_detail::nonnegative(g)) { return {}; }
    ShortestPathResult out(g.n, {s}); vector<lll> hv(g.n); vector<char> known(g.n);
    auto f = [&](int v) {
        if (!known[v]) { known[v] = true; hv[v] = v == t ? 0 : lll(h(v)); }
        return out.dist[v] + hv[v];};
    priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<pair<lll, int>>> ready; ready.push({f(s), s});
    while (!ready.empty()) {
        auto [d, u] = ready.top(); ready.pop();
        if (d != f(u)) { continue; }
        if (u == t) { return {out.dist[t], out.path(g, t)}; }
        for (int a : g[u]) {
            const auto &e = g.arcs[a];
            if (shortest_path_detail::relax(out, e, a)) { ready.push({f(e.to), e.to}); }}}
    return {};}

// T: O((n + m) * log(n + m + 1)), M: O(n + m); weights >= 0, searches from s on g and from t on the reversed arcs.
template<class G>
StPath bidirectionalDijkstra(const G &g, int s, int t) {
    assert(0 <= s && s < g.n && 0 <= t && t < g.n);
    if (!shortest_path_advanced_detail::nonnegative(g)) { return {}; }
    vector<vector<int>> in(g.n);
    for (int a = 0; a < int(g.arcs.size()); ++a) { in[g.arcs[a].to].push_back(a); }
    ShortestPathResult side[2] = {ShortestPathResult(g.n, {s}), ShortestPathResult(g.n, {t})};
    priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<pair<lll, int>>> ready[2];
    ready[0].push({0, s}); ready[1].push({0, t});
    lll best = s == t ? 0 : PATH_INF; int meet = s == t ? s : -1;
    while (!ready[0].empty() && !ready[1].empty() && ready[0].top().first + ready[1].top().first < best) {
        int i = ready[0].top().first <= ready[1].top().first ? 0 : 1;
        auto [d, u] = ready[i].top(); ready[i].pop();
        if (d != side[i].dist[u]) { continue; }
        auto scan = [&](int a) {
            const auto &e = g.arcs[a];
            int v = i ? e.from : e.to;
            if (side[i].reachable[v] && d + e.w >= side[i].dist[v]) { return; }
            side[i].reachable[v] = true; side[i].dist[v] = d + e.w; side[i].parent[v] = u; side[i].parent_arc[v] = a;
            ready[i].push({side[i].dist[v], v});
            if (side[!i].reachable[v] && side[0].dist[v] + side[1].dist[v] < best) { best = side[0].dist[v] + side[1].dist[v]; meet = v; }};
        if (i) { for (int a : in[u]) { scan(a); }}
        else { for (int a : g[u]) { scan(a); }}}
    if (meet == -1) { return {}; }
    StPath out{best, side[0].path(g, meet)};
    for (int v = meet; v != t; v = side[1].parent[v]) {
        out.path.arcs.push_back(side[1].parent_arc[v]); out.path.vertices.push_back(side[1].parent[v]);}
    return out;}

// T: O(n + (k + m) * log(k + m + 1)), M: O(n + m); weights >= 0; rank 1 is the second shortest walk, strictly longer when strict.
template<class G>
TopTwoPaths secondShortestPath(const G &g, const vector<int> &sources, bool strict = true) {
    if (!shortest_path_advanced_detail::nonnegative(g)) { return {}; }
    vector<int> unique; vector<char> seen(g.n);
    for (int s : sources) {
        assert(0 <= s && s < g.n);
        if (!seen[s]) { seen[s] = true; unique.push_back(s); }}
    return shortest_path_advanced_detail::topTwo(g, unique, [&](const TopTwoPaths &r, int x, lll d, int) { return !strict || d > r.dist[x]; });}

// T: O(n + (k + m) * log(k + m + 1)), M: O(n + m + k); weights >= 0; rank 1 is the nearest source distinct from the rank-0 source.
template<class G>
TopTwoPaths top2Dijkstra(const G &g, const vector<int> &sources) {
    if (!shortest_path_advanced_detail::nonnegative(g)) { return {}; }
    return shortest_path_advanced_detail::topTwo(g, sources, [](const TopTwoPaths &r, int x, lll, int s) { return s != r.source[x]; });}

// T: O(n + m), M: O(n + m); sp from a sum-weight engine on g, tight arcs join finite vertices.
template<class G>
ShortestPathDag shortestPathDag(const G &g, const ShortestPathResult &sp) {
    assert(int(sp.dist.size()) == g.n);
    ShortestPathDag out{true, {}, {}}; vector<int> indeg(g.n); int finite = 0;
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        if (shortest_path_advanced_detail::tight(g, sp, a)) { out.arcs.push_back(a); ++indeg[g.arcs[a].to]; }}
    for (int v = 0; v < g.n; ++v) {
        finite += sp.finite(v);
        if (sp.finite(v) && indeg[v] == 0) { out.order.push_back(v); }}
    for (int i = 0; i < int(out.order.size()); ++i) {
        for (int a : g[out.order[i]]) {
            if (shortest_path_advanced_detail::tight(g, sp, a) && --indeg[g.arcs[a].to] == 0) { out.order.push_back(g.arcs[a].to); }}}
    out.acyclic = int(out.order.size()) == finite;
    return out;}

// T: O(k + (n + m) * log(n + m + 1)) for weights >= 0, O(n * m + n + k) otherwise, M: O(n + m); mod in [1, 2^63].
template<class G>
PathCountResult countShortestPaths(const G &g, const vector<int> &sources, ulng mod) {
    assert(1 <= mod && mod <= ulng(1) << 63);
    bool nonnegative = std::all_of(g.arcs.begin(), g.arcs.end(), [](const GraphArc &e) { return e.w >= 0; });
    PathCountResult out{nonnegative ? dijkstra(g, sources) : bellmanFord(g, sources), vector<ulng>(g.n), vector<char>(g.n)};
    auto dag = shortestPathDag(g, out.paths); vector<char> source(g.n);
    for (int s : sources) { source[s] = true; }
    for (int v = 0; v < g.n; ++v) { out.infinite[v] = out.paths.finite(v); }
    for (int u : dag.order) {
        out.infinite[u] = false;
        if (source[u] && out.paths.dist[u] == 0) { out.count[u] = (out.count[u] + 1) % mod; }
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (shortest_path_advanced_detail::tight(g, out.paths, a)) { out.count[v] = (out.count[v] + out.count[u]) % mod; }}}
    return out;}

// T: O(k + (n + m) * log(n + m + 1)), M: O(n + m); signed weights, dist is the least maximum arc weight, -PATH_INF at sources.
template<class G>
ShortestPathResult minimaxPath(const G &g, const vector<int> &sources) {
    return shortest_path_advanced_detail::bottleneck(g, sources, false);}

// T: O(k + (n + m) * log(n + m + 1)), M: O(n + m); signed weights, dist is the greatest minimum arc weight, PATH_INF at sources.
template<class G>
ShortestPathResult widestPath(const G &g, const vector<int> &sources) {
    return shortest_path_advanced_detail::bottleneck(g, sources, true);}

// T: O(sqrt(n) * (n + m) * log(N) + (n + m) * log(n + m + 1) + k), plus O(n * m) with a reachable negative cycle, N = 2 + max |w| of a reachable negative arc, M: O(n + m).
template<class G>
ShortestPathResult negativeSsspScaling(const G &g, const vector<int> &sources) {
    auto walk = bfs(g, sources); vector<char> live(g.n); ulng big = 0;
    for (int v : walk.order) { live[v] = true; }
    for (const auto &e : g.arcs) { if (live[e.from] && e.w < 0) { big = max(big, ulng(-(e.w + 1)) + 1); }}
    vector<lll> p(g.n);
    for (int t = std::bit_width(big) - 1; t >= 0; --t) {
        for (auto &x : p) { x *= 2; }
        // Scaled weight ceil(w / 2^t): rounding up never turns a cycle negative.
        auto w = [&](int a) {
            const auto &e = g.arcs[a];
            lll c = e.w >= 0 ? (lll(e.w) + (lll(1) << t) - 1) >> t : -lll((ulng(-(e.w + 1)) + 1) >> t);
            return c + p[e.from] - p[e.to];};
        if (!shortest_path_advanced_detail::refine(g, live, p, w)) { return bellmanFord(g, sources); }}
    return shortest_path_advanced_detail::dijkstraPotential(g, sources, p);}
