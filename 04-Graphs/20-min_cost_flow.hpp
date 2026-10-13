#pragma once

#include "../01-Core/01-template.hpp"

// S: O(n), U: O(1) amortized addEdge, O(p) addConvexEdge, Q: O(1) getEdge, O(m) edges, M: O(n + m + q); p pieces, q all convex pieces.
template<class Cap = lng, class Cost = lng>
struct MinCostFlow {
    static_assert(std::numeric_limits<Cap>::is_integer && std::numeric_limits<Cap>::is_signed);
    static_assert(std::numeric_limits<Cost>::is_integer && std::numeric_limits<Cost>::is_signed);
    static constexpr Cap MAX = std::numeric_limits<Cap>::max();
    static constexpr Cost INF = std::numeric_limits<Cost>::max(), LIM = INF / 16;
    struct Edge { int from, to; Cap cap, flow; Cost cost; };
    struct Arc { int to, rev; Cap cap; Cost cost; };
    struct Convex { int id, l, r, j; };
    int n, sink = -1;
    bool fresh = true;
    vector<vector<Arc>> g;
    vector<pair<int, int>> pos;
    vector<int> cv, pe, low;
    vector<Convex> conv;
    vector<pair<Cap, Cost>> pc;
    vector<Cost> h, dist;
    vector<char> vis;
    vector<pair<Cost, int>> heap;

    explicit MinCostFlow(int N = 0) : n(N), g(std::max(N, 0)), h(std::max(N, 0)) { assert(n >= 0); }

    int addEdge(int u, int v, Cap cap, Cost cost) {
        assert(0 <= u && u < n && 0 <= v && v < n && 0 <= cap && -LIM <= cost && cost <= LIM);
        assert(pos.size() < size_t(INT_MAX));
        int k = int(g[u].size()), r = int(g[v].size()) + (u == v);
        cap = max(cap, Cap(0)); cost = std::clamp(cost, -LIM, LIM);
        if (cap > 0 && cost + h[u] - h[v] < 0) { fresh = false; }
        pos.emplace_back(u, k); cv.push_back(-1);
        g[u].push_back({v, r, cap, cost}); g[v].push_back({u, k, 0, -cost});
        return int(pos.size()) - 1;}
    // pieces (length > 0, unit cost) with nondecreasing unit costs: x units cost the sum of the cheapest x.
    int addConvexEdge(int u, int v, const vector<pair<Cap, Cost>> &pieces) {
        int k = int(pieces.size()), l = int(pc.size());
        if (k == 0) { return addEdge(u, v, 0, 0); }
        Cap x = 0;
        Cost c = -LIM;
        for (auto [len, w] : pieces) {
            assert(0 < len && len <= MAX - x && c <= w && w <= LIM);
            c = std::clamp(w, c, LIM);
            pc.emplace_back(x, c); x += std::clamp(len, Cap(0), MAX - x);}
        pc.emplace_back(x, c);
        int id = addEdge(u, v, pc[l + 1].first, pc[l].second);
        cv[id] = int(conv.size()); conv.push_back({id, l, l + k, l});
        return id;}

    Edge getEdge(int i) const {
        assert(0 <= i && i < int(pos.size()));
        auto [u, k] = pos[i]; auto &a = g[u][k]; auto &b = g[a.to][a.rev];
        if (cv[i] < 0) { return {u, a.to, a.cap + b.cap, b.cap, -b.cost}; }
        auto &c = conv[cv[i]];
        return {u, a.to, pc[c.r].first, pc[min(c.j + 1, c.r)].first - a.cap, -b.cost};}
    vector<Edge> edges() const {
        vector<Edge> res;
        for (int i = 0; i < int(pos.size()); ++i) { res.push_back(getEdge(i)); }
        return res;}
    // T: O(n) when valid, else O(n * m); p with cost + p[u] - p[v] >= 0 on every arc of positive residual capacity.
    vector<Cost> potentials() {
        if (!fresh) { assert(bounded()); initPotentials(); }
        return h;}

    // T: O(n * m + (K + 1) * (n + m) * log(n + m)), M: O(n + m + K); K augmentations; n * m only after a negative reduced-cost arc.
    vector<pair<Cap, lll>> slope(int s, int t, Cap limit = MAX, Cost below = INF) {
        bool ok = bounded();
        assert(0 <= s && s < n && 0 <= t && t < n && s != t && limit >= 0 && ok);
        vector<pair<Cap, lll>> res{{0, 0}};
        if (!fresh) { initPotentials(); }
        else if (ok && sink >= 0 && sink != t) { dijkstra(-1, -1); }
        if (!ok || !fresh) { return res; }
        sink = t;
        Cap flow = 0;
        Cost last = 0;
        while (flow < limit && dijkstra(s, t)) {
            Cost unit = h[t] - h[s];
            if (unit >= below) { break; }
            Cap f = limit - flow;
            for (int v = t; v != s; v = g[v][pe[v]].to) { auto &b = g[v][pe[v]]; f = min(f, g[b.to][b.rev].cap); }
            for (int v = t; v != s; v = g[v][pe[v]].to) { auto &b = g[v][pe[v]]; b.cap += f; g[b.to][b.rev].cap -= f; }
            for (auto &c : conv) { refresh(c); }
            lll cost = res.back().second + lll(f) * unit;
            if (res.size() > 1 && unit == last) { res.pop_back(); }
            flow += f; last = unit;
            res.emplace_back(flow, cost);}
        return res;}
    // T: O(n * m + (K + 1) * (n + m) * log(n + m)), M: O(n + m + K); the last breakpoint of slope.
    pair<Cap, lll> flow(int s, int t, Cap limit = MAX, Cost below = INF) { return slope(s, t, limit, below).back(); }

    // T: O(n * m), M: O(n); Bellman-Ford from a zero-cost virtual source in Kahn order, so an acyclic residual network takes one round.
    void initPotentials() {
        vector<int> deg(n), ord;
        for (auto &adj : g) { for (auto &a : adj) { deg[a.to] += a.cap > 0; }}
        for (int v = 0; v < n; ++v) { if (deg[v] == 0) { ord.push_back(v); }}
        for (int i = 0; i < int(ord.size()); ++i) {
            for (auto &a : g[ord[i]]) { if (a.cap > 0 && --deg[a.to] == 0) { ord.push_back(a.to); }}}
        for (int v = 0; v < n; ++v) { if (deg[v] > 0) { ord.push_back(v); }}
        h.assign(n, 0);
        bool upd = n > 0;
        for (int r = 0; r < n && upd; ++r) {
            upd = false;
            for (int u : ord) {
                for (auto &a : g[u]) {
                    if (a.cap > 0 && h[u] + a.cost < h[a.to]) { h[a.to] = max(h[u] + a.cost, -LIM); upd = true; }}}}
        fresh = !upd; sink = -1;
        assert(fresh);}
    // T: O((n + m) * log(n + m)), M: O(n + m); reduced costs from s, or from every v at -h[v] if s < 0; h += dist - dist[t] (t < 0: dist) where settled.
    bool dijkstra(int s, int t) {
        dist.assign(n, INF); vis.assign(n, 0); pe.resize(n); heap.clear(); low.clear();
        // Raw pointers shadow the member arrays so their addresses stay in registers across heap pushes.
        Cost *dist = this->dist.data(), *h = this->h.data();
        char *vis = this->vis.data();
        int *pe = this->pe.data();
        for (int v = 0; v < n; ++v) {
            if (s < 0 || v == s) { dist[v] = s < 0 ? -h[v] : 0; heap.emplace_back(dist[v], v); }}
        std::make_heap(heap.begin(), heap.end(), std::greater<>());
        while (!low.empty() || !heap.empty()) {
            int u;
            if (!low.empty()) { u = low.back(); low.pop_back(); }
            else { std::pop_heap(heap.begin(), heap.end(), std::greater<>()); u = heap.back().second; heap.pop_back(); }
            if (vis[u]) { continue; }
            vis[u] = 1;
            if (u == t) { break; }
            Cost base = dist[u] + h[u];
            for (auto &a : g[u]) {
                if (a.cap <= 0 || vis[a.to]) { continue; }
                Cost d = base + a.cost - h[a.to];
                if (d >= dist[a.to]) { continue; }
                dist[a.to] = d; pe[a.to] = a.rev;
                // A zero reduced-cost arc keeps the current minimum, so its head is settled next without the heap.
                if (d == dist[u]) { low.push_back(a.to); }
                else { heap.emplace_back(d, a.to); std::push_heap(heap.begin(), heap.end(), std::greater<>()); }}}
        if (t >= 0 && !vis[t]) { return false; }
        Cost dt = t >= 0 ? dist[t] : 0;
        for (int v = 0; v < n; ++v) {
            if (vis[v]) { h[v] += dist[v] - dt; }}
        return true;}
    // T: O(1), M: O(1); re-reads a convex edge's flow x after a push and exposes its pieces around x as the arc pair.
    void refresh(Convex &c) {
        auto [u, k] = pos[c.id]; auto &a = g[u][k]; auto &b = g[a.to][a.rev];
        Cap x = pc[min(c.j + 1, c.r)].first - a.cap;
        if (c.j < c.r && x == pc[c.j + 1].first) { ++c.j; }
        else if (c.j > c.l && x < pc[c.j].first) { --c.j; }
        int p = x > pc[c.j].first || c.j == c.l ? c.j : c.j - 1;
        a.cap = pc[min(c.j + 1, c.r)].first - x; a.cost = pc[c.j].second;
        b.cap = x - pc[p].first; b.cost = -pc[p].second;}
    // T: O(m), M: O(1); every simple residual path costs at most LIM in absolute value, and every total cost fits in lll with room to subtract.
    bool bounded() const {
        lll w = 0, top = 0, sum = 0, prod;
        bool ok = true;
        auto mag = [](Cost c) { return c < 0 ? -lll(c) : lll(c); };
        for (int i = 0; i < int(pos.size()); ++i) {
            auto e = getEdge(i);
            if (e.from == e.to || e.cap == 0) { continue; }
            lll c = cv[i] < 0 ? mag(e.cost) : max(mag(pc[conv[cv[i]].l].second), mag(pc[conv[cv[i]].r].second));
            w = min(w + c, lll(LIM) + 1); top = max(top, c);
            ok = ok && !__builtin_mul_overflow(lll(e.cap), c, &prod) && !__builtin_add_overflow(sum, prod, &sum) && sum <= lll(~ulll(0) >> 2);}
        return ok && (w <= LIM || n <= 1 || top <= lll(LIM) / (n - 1));}
};
