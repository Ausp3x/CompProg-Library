#pragma once

#include "../01-Core/01-template.hpp"

// S: O(n + m), U: O(1) amortized addEdge, Q: O(1) getEdge, O(m) edges, M: O(n + m); Cap is a signed integer type, capacities >= 0.
template<class Cap = lng>
struct MaxFlow {
    static_assert(std::numeric_limits<Cap>::is_integer && std::numeric_limits<Cap>::is_signed);
    static constexpr Cap MAX = std::numeric_limits<Cap>::max();
    static constexpr lng GLOBAL = 4;
    struct Edge { int from, to; Cap cap, flow; };
    struct Arc { int to, rev; Cap cap; };
    int n;
    vector<vector<Arc>> g;
    vector<pair<int, int>> pos;
    vector<Cap> rcap;
    vector<int> level, it, que;

    explicit MaxFlow(int N = 0) : n(N), g(std::max(N, 0)) { assert(n >= 0); }
    template<class G> requires requires(const G &x) { x.edges; x.directed; }
    explicit MaxFlow(const G &gr) : MaxFlow(gr.n) {
        for (auto e : gr.edges) {
            assert(lll(e.w) <= lll(MAX)); addEdge(e.u, e.v, Cap(e.w), gr.directed ? 0 : Cap(e.w));}}

    int addEdge(int u, int v, Cap cap, Cap rev_cap = 0) {
        assert(0 <= u && u < n && 0 <= v && v < n && 0 <= cap && 0 <= rev_cap && cap <= MAX - rev_cap);
        assert(pos.size() < size_t(INT_MAX));
        int k = int(g[u].size()), r = int(g[v].size()) + (u == v);
        pos.emplace_back(u, k); rcap.push_back(rev_cap);
        g[u].push_back({v, r, cap}); g[v].push_back({u, k, rev_cap});
        return int(pos.size()) - 1;}
    void changeEdge(int i, Cap cap, Cap flow) {
        assert(0 <= i && i < int(pos.size()) && 0 <= cap && cap <= MAX - rcap[i] && -rcap[i] <= flow && flow <= cap);
        auto [u, k] = pos[i]; auto &a = g[u][k];
        a.cap = cap - flow; g[a.to][a.rev].cap = rcap[i] + flow;}

    Edge getEdge(int i) const {
        assert(0 <= i && i < int(pos.size()));
        auto [u, k] = pos[i]; auto &a = g[u][k]; Cap f = g[a.to][a.rev].cap - rcap[i];
        return {u, a.to, a.cap + f, f};}
    vector<Edge> edges() const {
        vector<Edge> res;
        for (int i = 0; i < int(pos.size()); ++i) { res.push_back(getEdge(i)); }
        return res;}

    // T: O(n^2 * m), M: O(n); adds min(remaining max flow, limit) to the current flow and returns it.
    Cap flow(int s, int t, Cap limit = MAX) {
        assert(0 <= s && s < n && 0 <= t && t < n && s != t && limit >= 0);
        return dinic(s, t, limit, 1);}
    // T: O(n * m * log(U)), M: O(n); U is the largest residual capacity; same result as flow.
    Cap capacityScalingDinic(int s, int t, Cap limit = MAX) {
        assert(0 <= s && s < n && 0 <= t && t < n && s != t && limit >= 0);
        Cap top = 0, delta = 1, res = 0;
        for (auto &adj : g) { for (auto &a : adj) { top = max(top, a.cap); }}
        while (delta <= top / 2) { delta *= 2; }
        for (; delta > 0 && res < limit; delta /= 2) { res += dinic(s, t, limit - res, delta); }
        return res;}
    // T: O(n^2 * sqrt(m)), M: O(n); adds a maximum flow; non-loop residual capacities leaving s must sum to at most MAX.
    Cap pushRelabel(int s, int t) {
        assert(0 <= s && s < n && 0 <= t && t < n && s != t);
        Cap supply = 0;
        bool over = false;
        for (auto &a : g[s]) { over |= a.to != s && __builtin_add_overflow(supply, a.cap, &supply); }
        assert(!over);
        if (s == t || over) { return 0; }
        vector<int> h(n), head(n), nxt(n), prv(n);
        vector<vector<int>> bucket(2 * n);
        vector<Cap> ex(n);
        int hi = 0, top = 0;
        lng work = 0;
        auto link = [&](int v) {
            nxt[v] = head[h[v]]; prv[v] = -1; head[h[v]] = v; top = max(top, h[v]);
            if (nxt[v] >= 0) { prv[nxt[v]] = v; }};
        auto unlink = [&](int v) {
            (prv[v] >= 0 ? nxt[prv[v]] : head[h[v]]) = nxt[v];
            if (nxt[v] >= 0) { prv[nxt[v]] = prv[v]; }};
        auto push = [&](int u, Arc &a, Cap f) {
            if (ex[a.to] == 0 && a.to != s && a.to != t) { bucket[h[a.to]].push_back(a.to); hi = max(hi, h[a.to]); }
            a.cap -= f; g[a.to][a.rev].cap += f; ex[u] -= f; ex[a.to] += f;};
        auto bfs = [&](int r) {
            que.assign(1, r);
            for (int i = 0; i < int(que.size()); ++i) {
                for (auto &a : g[que[i]]) {
                    if (h[a.to] == 2 * n && g[a.to][a.rev].cap > 0) { h[a.to] = h[que[i]] + 1; que.push_back(a.to); }}}};
        // Global relabel: distance to t over reverse residual arcs, else n + distance to s, else 2n - 1.
        auto relabelAll = [&] {
            fill(h.begin(), h.end(), 2 * n); fill(head.begin(), head.end(), -1); it.assign(n, 0);
            for (auto &b : bucket) { b.clear(); }
            h[t] = 0; h[s] = n; hi = top = 0; work = 0;
            bfs(t);
            for (int v : que) { link(v); }
            bfs(s);
            for (int v = 0; v < n; ++v) {
                if (h[v] == 2 * n) { h[v] = 2 * n - 1; }
                if (ex[v] > 0 && v != s && v != t) { bucket[h[v]].push_back(v); hi = max(hi, h[v]); }}};
        for (auto &a : g[s]) {
            if (a.to != s && a.cap > 0) { push(s, a, a.cap); }}
        relabelAll();
        while (hi >= 0) {
            if (bucket[hi].empty()) { --hi; continue; }
            int u = bucket[hi].back(); bucket[hi].pop_back();
            while (ex[u] > 0) {
                if (it[u] < int(g[u].size())) {
                    auto &a = g[u][it[u]];
                    if (a.cap > 0 && h[u] == h[a.to] + 1) { push(u, a, min(ex[u], a.cap)); }
                    else { ++it[u]; }
                    continue;}
                int old = h[u];
                if (old < n) { unlink(u); }
                h[u] = 2 * n; it[u] = 0; work += int(g[u].size()) + 12;
                for (auto &a : g[u]) {
                    if (a.cap > 0) { h[u] = min(h[u], h[a.to] + 1); }}
                if (h[u] >= 2 * n) { break; }
                // Gap: labels in (old, n) cannot reach t; u is the highest active vertex, so none of them is active.
                if (old < n && head[old] < 0) {
                    for (int l = old + 1; l <= top; ++l) {
                        for (int v = head[l]; v >= 0; v = nxt[v]) { h[v] = n + 1; it[v] = 0; }
                        head[l] = -1;}
                    top = old - 1; h[u] = max(h[u], n + 1);}
                else if (h[u] < n) { link(u); }}
            if (work > GLOBAL * (n + lng(pos.size()))) { relabelAll(); }}
        return ex[t];}
    // T: O(min(n^2 * m, (n + m) * (d + 1))) from a maximum flow, M: O(n); d = |cap - old cap|; keeps the flow maximum.
    Cap changeCapacity(int i, Cap cap, int s, int t) {
        assert(0 <= i && i < int(pos.size()) && 0 <= cap && cap <= MAX - rcap[i]);
        assert(0 <= s && s < n && 0 <= t && t < n && s != t);
        auto [u, k] = pos[i]; auto &a = g[u][k]; auto &b = g[a.to][a.rev];
        Cap f = b.cap - rcap[i], res = 0;
        if (f <= cap) { a.cap = cap - f; }
        else {
            int v = a.to;
            a.cap = 0; b.cap = rcap[i] + cap;
            Cap back = f - cap - dinic(u, v, f - cap, 1);
            dinic(u, s, back, 1); dinic(t, v, back, 1);
            res = -back;}
        return res + dinic(s, t, MAX, 1);}

    // T: O(n + m), M: O(n); res[v] is true when v is reachable from s in the residual graph.
    vector<bool> minCut(int s) const {
        assert(0 <= s && s < n);
        vector<bool> res(n);
        vector<int> q{s};
        res[s] = true;
        for (int i = 0; i < int(q.size()); ++i) {
            for (auto &a : g[q[i]]) {
                if (a.cap > 0 && !res[a.to]) { res[a.to] = true; q.push_back(a.to); }}}
        return res;}
    // T: O(n + m), M: O(n + out); IDs of edges with positive capacity from the minCut(s) side to the other side.
    vector<int> minCutEdges(int s) const {
        auto side = minCut(s);
        vector<int> res;
        for (int i = 0; i < int(pos.size()); ++i) {
            auto e = getEdge(i);
            if ((side[e.from] && !side[e.to] && e.cap > 0) || (side[e.to] && !side[e.from] && rcap[i] > 0)) { res.push_back(i); }}
        return res;}
    // T: O(n * m), M: O(n + m + out); valid flow as (amount, edge IDs from s to t) paths summing to its value, cycles dropped.
    vector<pair<Cap, vector<int>>> pathDecomposition(int s, int t) const {
        assert(0 <= s && s < n && 0 <= t && t < n && s != t);
        vector<Cap> rem(pos.size());
        vector<vector<int>> out(n);
        vector<int> head(pos.size()), at(n), on(n, -1), vs{s}, es;
        vector<pair<Cap, vector<int>>> res;
        lll need = 0;
        if (s == t) { return res; }
        for (int i = 0; i < int(pos.size()); ++i) {
            auto e = getEdge(i);
            int tail = e.flow < 0 ? e.to : e.from;
            rem[i] = e.flow < 0 ? -e.flow : e.flow;
            head[i] = e.flow < 0 ? e.from : e.to;
            need += (head[i] == t ? lll(rem[i]) : 0) - (tail == t ? lll(rem[i]) : 0);
            if (e.flow != 0) { out[tail].push_back(i); }}
        auto cancel = [&](int from, vector<int> &&ids, lll cap) {
            Cap f = MAX;
            for (int id : ids) { f = min(f, rem[id]); }
            if (lll(f) > cap) { f = Cap(cap); }
            for (int id : ids) { rem[id] -= f; }
            for (int j = from; j < int(vs.size()); ++j) { on[vs[j]] = -1; }
            vs.resize(from); es.resize(from == 0 ? 0 : from - 1);
            return pair{f, std::move(ids)};};
        on[s] = 0;
        int u = s;
        while (true) {
            // Only the net inflow of t ends paths; the rest of its inflow continues as cycles.
            if (u == t && need > 0) {
                res.push_back(cancel(0, vector<int>(es), need)); need -= res.back().first;
                vs.push_back(s); on[s] = 0; u = s;
                continue;}
            while (at[u] < int(out[u].size()) && rem[out[u][at[u]]] == 0) { ++at[u]; }
            if (at[u] == int(out[u].size())) { break; }
            int i = out[u][at[u]], v = head[i];
            es.push_back(i);
            if (on[v] < 0) { on[v] = int(vs.size()); vs.push_back(v); u = v; continue; }
            int j = on[v];
            cancel(j + 1, vector<int>(es.begin() + j, es.end()), lll(MAX));
            u = v;}
        return res;}

    // T: O(n^2 * m), M: O(n); Dinic phases over arcs with residual >= delta, blocking flow by iterative multi-push DFS from t.
    Cap dinic(int s, int t, Cap limit, Cap delta) {
        struct Frame { int v; Cap up, got; };
        vector<Frame> st;
        Cap res = 0;
        while (res < limit) {
            level.assign(n, -1); level[s] = 0; que.assign(1, s);
            for (int i = 0; i < int(que.size()) && level[t] < 0; ++i) {
                for (auto &a : g[que[i]]) {
                    if (a.cap >= delta && level[a.to] < 0) { level[a.to] = level[que[i]] + 1; que.push_back(a.to); }}}
            if (level[t] < 0) { return res; }
            it.assign(n, 0); st.assign(1, {t, limit - res, 0});
            Cap ret = -1;
            while (!st.empty()) {
                auto &[v, up, got] = st.back();
                if (ret >= 0) {
                    auto &a = g[v][it[v]];
                    a.cap += ret; g[a.to][a.rev].cap -= ret; got += ret; ret = -1;
                    if (got == up) { ret = got; st.pop_back(); continue; }
                    ++it[v];}
                if (v == s) { ret = up; st.pop_back(); continue; }
                int &i = it[v];
                while (i < int(g[v].size()) && (level[g[v][i].to] != level[v] - 1 || g[g[v][i].to][g[v][i].rev].cap < delta)) { ++i; }
                if (i == int(g[v].size())) { level[v] = n; ret = got; st.pop_back(); continue; }
                auto &a = g[v][i];
                st.push_back({a.to, min(up - got, g[a.to][a.rev].cap), 0});}
            if (ret == 0) { return res; }
            res += ret;}
        return res;}
};

// Legacy adapter: vertices 0..n, cap[u][v] read once per listed pair; getMaxFlow runs Dinic.
// S: O(n^2 + m), Q: O(n^2 * (m + 1)) getMaxFlow/getMinCut, O(n + m) augmentFlow, M: O(n^2); exact when the max flow fits lng.
struct EdmondsKarp {
    int n;
    vector<vector<bool>> adjm;
    vector<vector<int>> adjl;
    vector<vector<lng>> ocap, ncap;
    lng max_flow = 0, add_flow = 0;
    vector<int> par;
    vector<bool> in_S;
    vector<pair<int, int>> cut_set;

    EdmondsKarp(int N, const vector<vector<int>> &adj, const vector<vector<lng>> &cap) : n(N) {
        assert(n >= 0 && int(adj.size()) == n + 1 && int(cap.size()) == n + 1);
        adjm.assign(n + 1, vector<bool>(n + 1)); ocap.assign(n + 1, vector<lng>(n + 1)); adjl.resize(n + 1);
        for (int u = 0; u <= n; ++u) {
            assert(int(cap[u].size()) == n + 1);
            for (int v : adj[u]) {
                assert(0 <= v && v <= n && cap[u][v] >= 0);
                adjm[u][v] = true; ocap[u][v] = cap[u][v];}}
        for (int u = 0; u <= n; ++u) {
            for (int v = 0; v <= n; ++v) {
                if (adjm[u][v] || adjm[v][u]) {
                    assert(ocap[u][v] <= std::numeric_limits<lng>::max() - ocap[v][u]); adjl[u].push_back(v);}}}
        ncap = ocap; par.assign(n + 1, -1); in_S.assign(n + 1, false);}

    lng augmentFlow(int s, int t) {
        assert(0 <= s && s <= n && 0 <= t && t <= n);
        fill(par.begin(), par.end(), -1); par[s] = s;
        vector<pair<int, lng>> q{{s, std::numeric_limits<lng>::max()}};
        for (int i = 0; i < int(q.size()); ++i) {
            auto [u, f] = q[i];
            for (int v : adjl[u]) {
                if (par[v] == -1 && ncap[u][v] > 0) {
                    par[v] = u;
                    if (v == t) { return min(f, ncap[u][v]); }
                    q.emplace_back(v, min(f, ncap[u][v]));}}}
        return 0;}
    lng getMaxFlow(int s, int t) {
        assert(0 <= s && s <= n && 0 <= t && t <= n);
        MaxFlow<lng> mf(n + 1);
        for (int u = 0; u <= n; ++u) {
            for (int v : adjl[u]) {
                if (adjm[u][v]) { mf.addEdge(u, v, ocap[u][v]); }}}
        max_flow = s == t ? 0 : mf.flow(s, t);
        ncap = ocap;
        for (auto e : mf.edges()) { ncap[e.from][e.to] -= e.flow; ncap[e.to][e.from] += e.flow; }
        add_flow = augmentFlow(s, t);
        return max_flow;}
    void getMinCut(int s, int t) {
        getMaxFlow(s, t); cut_set.clear();
        for (int u = 0; u <= n; ++u) { in_S[u] = par[u] != -1; }
        for (int u = 0; u <= n; ++u) {
            for (int v : adjl[u]) {
                if (in_S[u] && adjm[u][v] && !in_S[v]) { cut_set.emplace_back(u, v); }}}}
};
