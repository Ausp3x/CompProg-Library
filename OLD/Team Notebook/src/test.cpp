#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi     first
#define se     second
#define pb     push_back
#define all(x) (x).begin(), (x).end()
#define ral(x) (x).rbegin(), (x).rend()
#define sze(x) int((x).size())
using lng = long long;
using lll = __int128;
template<typename T> 
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

int const INF32 = 0x3f3f3f3f;
lng const INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T> 
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T> 
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }

/* 
STYLE GUIDE
    1) Always use {} if applicable to avoid bugs. Never do the example below:
    EX: if (...)
            something;
    3) Nested lambda functions should always use the auto = [](...) -> datatype format. Recursive nested 
       lambda functions should have their names placed at the start of the arguments as auto &&name.
    EX: auto foo = [&](auto &&foo, ...) -> int {
    4) Never use the shortcuts in the #defines. Always follow the usings however.
    5) Add std:: prefix to all applicable except for:
        - common datatypes, istream, ostream
        - pair, tuple, array, bitset, vector, deque, priority_queue, queue, stack, maps, sets
        - max, min, gcd, lcm, reverse, sort.
    6) pointers (*) and references (&) should always be attached to the variable and function names.
    8) Always use std::midpoint(l, r) over (l + r) / 2.
*/

// T: O(V + E), M: O(V)
vector<vector<int>> getConnCmps(int n, const vector<vector<int>> &adjl) {
    vector<bool> vst(n, false);
    vector<int> st;
    vector<vector<int>> res;
    for (int i = 0; i < n; i++) {
        if (vst[i]) { continue; } vst[i] = true;
        vector<int> cur;
        st.push_back(i);
        while (!st.empty()) {
            int u = st.back(); st.pop_back();
            cur.push_back(u);
            for (int v : adjl[u]) { 
                if (vst[v]) { continue; } vst[v] = true; 
                st.push_back(v); 
            }
        }
        res.push_back(cur);
    }
    return res;
}

// T: O(V + E), M: O(V)
vector<int> getTopoSort(int n, const vector<vector<int>> &adjl) {
    vector<bool> vst(n, false); 
    vector<int> res;
    
    auto dfs = [&](auto &&dfs, int u) -> void {
        vst[u] = true;
        for (int v : adjl[u]) { if (!vst[v]) { dfs(dfs, v); } }
        res.push_back(u);
    };
    for (int i = 0; i < n; i++) { if (!vst[i]) { dfs(dfs, i); } }
    std::reverse(res.begin(), res.end());
    
    return res;
}

// T: O((V + E) * log(V)), M: O(V)
struct Dijkstra {
    int n;
    vector<int> par; vector<lng> dis; 
    
    void solve(const vector<int> &src, const vector<vector<pair<int, lng>>> &adjl) {
        n = adjl.size();
        par.assign(n, -1); dis.assign(n, INF64); 
        priority_queue<pair<lng, int>, vector<pair<lng, int>>, std::greater<pair<lng, int>>> pq;
        for (int u : src) { dis[u] = 0; pq.push({0, u}); }
        while (!pq.empty()) {
            auto [d, u] = pq.top(); pq.pop();
            if (d > dis[u]) { continue; }
            for (auto [v, w] : adjl[u]) {
                if (dis[u] + w >= dis[v]) { continue; }
                par[v] = u; dis[v] = dis[u] + w; pq.push({dis[v], v}); 
            }
        }
    }
};

// T: O(V * E) avg., M: O(V)
struct DEsopoPape {
    int n; vector<int> M;
    vector<int> par; vector<lng> dis; 
    
    void solve(const vector<int> &src, const vector<vector<pair<int, lng>>> &adjl) {
        n = adjl.size(); M.assign(n, 2);
        par.assign(n, -1); dis.assign(n, INF64);  
        deque<int> dq;
        for (int u : src) { dis[u] = 0; dq.push_back(u); }
        while (!dq.empty()) {
            int u = dq.front(); dq.pop_front(); M[u] = 0;
            for (auto [v, w] : adjl[u]) {
                if (dis[u] + w >= dis[v]) { continue; }
                if      (M[v] == 0) { M[v] = 1; dq.push_front(v); }
                else if (M[v] == 2) { M[v] = 1; dq.push_back(v); }
                par[v] = u; dis[v] = dis[u] + w; 
            }
        }
    }
};

// T: O(V * E), M: O(V + E)
struct SPFA {
    int n;
    vector<int> par; vector<lng> dis; 
    
    bool solve(const vector<int> &src, const vector<vector<pair<int, lng>>> &adjl) {
        n = adjl.size();
        par.assign(n, -1); dis.assign(n, INF64); 
        queue<int> qu;
        vector<bool> in_qu(n, false); vector<int> cnt(n, 0); 
        for (int u : src) { dis[u] = 0; qu.push(u); in_qu[u] = true; }
        while (!qu.empty()) {
            int u = qu.front(); qu.pop();
            in_qu[u] = false;
            for (auto [v, w] : adjl[u]) {
                if (dis[u] + w >= dis[v]) { continue; }
                par[v] = u; dis[v] = dis[u] + w; 
                if (in_qu[v]) { continue; } 
                qu.push(v); in_qu[v] = true; cnt[v]++; 
                if (cnt[v] > n) { return false; }
            }
        }
        return true;
    }
};

// T: O(V^3), M: O(1)
void floydWarshall(vector<vector<lng>> &dis) {
    int n = dis.size();
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (dis[i][k] < INF64 && dis[k][j] < INF64) {
                    dis[i][j] = min(dis[i][j], dis[i][k] + dis[k][j]);}}}}
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                if (dis[i][k] < INF64 && dis[k][k] < 0 && dis[k][j] < INF64) { 
                    dis[i][j] = -INF64; break;}}}}
}

// T: O(V + E), M: O(V)
bool isBipartite(int n, const vector<vector<int>> &adjl) {
    queue<int> qu;
    vector<int> clr(n, -1);
    for (int i = 0; i < n; i++) {
        if (clr[i] != -1) { continue; }
        qu.push(i); clr[i] = 0;
        while (!qu.empty()) {
            int u = qu.front(); qu.pop();
            for (int v : adjl[u]) {
                if (clr[v] == -1) { clr[v] = clr[u] ^ 1; qu.push(v); } 
                else if (clr[v] == clr[u]) { return false; }
            }
        }
    }
    return true;
}

// T: O(V + E), M: O(V)
struct TarjanBridge {
    int n, tm; 
    vector<bool> vst; vector<int> t_in, t_lo;
    vector<bool> is_art; vector<pair<int, int>> bridges;
    
    TarjanBridge(int n, const vector<vector<int>> &adjl) : 
        n(n), tm(0), vst(n, false), t_in(n, -1), t_lo(n, -1), is_art(n, false) {
        auto dfs = [&](auto &&dfs, int u, int p) -> void {
            vst[u] = true; t_lo[u] = t_in[u] = tm++;
            int cnt = 0; 
            bool p_skip = false;
            for (int v : adjl[u]) {
                if (v == p && !p_skip) { p_skip = true; continue; }
                if (vst[v]) { t_lo[u] = min(t_lo[u], t_in[v]); } 
                else {
                    dfs(dfs, v, u); 
                    t_lo[u] = min(t_lo[u], t_lo[v]);
                    if (t_lo[v] >= t_in[u] && p != -1) { is_art[u] = true; }
                    if (t_lo[v] > t_in[u]) { bridges.push_back({min(u, v), max(u, v)}); }
                    cnt++;
                }
            }
            if (p == -1 && cnt > 1) { is_art[u] = true; }
        };
        for (int i = 0; i < n; i++) { if (!vst[i]) { dfs(dfs, i, -1); } }
    }
};

// T: O(V + E), M: O(V)
struct TarjanSCC {
    int n, tm;
    vector<int> t_in, t_lo, st;
    vector<int> roots; vector<vector<int>> sccs, adjl_cond;
    
    TarjanSCC(int n, const vector<vector<int>> &adjl) : 
        n(n), tm(0), t_in(n, -1), t_lo(n, -1), roots(n, -1), adjl_cond(n, vector<int>{})  {
        auto dfs = [&](auto &&dfs, int u) -> void {
            t_lo[u] = t_in[u] = tm++; st.push_back(u);
            for (int v : adjl[u]) {
                if (t_in[v] == -1) { dfs(dfs, v); t_lo[u] = min(t_lo[u], t_lo[v]); } 
                else if (roots[v] == -1) { t_lo[u] = min(t_lo[u], t_in[v]); }
            }
            if (t_lo[u] == t_in[u]) {
                sccs.push_back({u});
                while (true) {
                    int v = st.back(); st.pop_back(); 
                    roots[v] = u;
                    if (v == u) { break; }
                    sccs.back().push_back(v);
                }
            }
        };
        for (int i = 0; i < n; i++) { if (t_in[i] == -1) { dfs(dfs, i); } }
        
        adjl_cond.assign(n, {});
        for (int u = 0; u < n; u++) {
            for (int v : adjl[u]) {
                if (roots[u] != roots[v]) { adjl_cond[roots[u]].push_back(roots[v]); }}}
    }
};

// S: O(V), U: O(log(V)), Q: O(1), M: O(V)
struct OnlineBridge {
    int brg_cnt, lca_ver; 
    vector<int> par, dsu_2e, dsu_cc, siz_cc, vst;
    
    OnlineBridge(int n) : brg_cnt(0), lca_ver(0), par(n, -1), dsu_2e(n), dsu_cc(n), siz_cc(n, 1), vst(n, 0) {
        std::iota(dsu_2e.begin(), dsu_2e.end(), 0); 
        std::iota(dsu_cc.begin(), dsu_cc.end(), 0);
    }
    
    int find2E(int u) {
        if (u == -1) { return -1; } 
        return dsu_2e[u] == u ? u : dsu_2e[u] = find2E(dsu_2e[u]); 
    }
    int findCC(int u) { 
        u = find2E(u);
        return dsu_cc[u] == u ? u : dsu_cc[u] = findCC(dsu_cc[u]); 
    }
    void mergePath(int u, int v) {
        lca_ver++; int lca = -1;
        vector<int> path_u, path_v; 
        while (lca == -1) {
            if (u != -1) {
                u = find2E(u); path_u.push_back(u);
                if (vst[u] == lca_ver) { lca = u; break; }
                vst[u] = lca_ver; u = par[u];
            }
            if (v != -1) {
                v = find2E(v); path_v.push_back(v);
                if (vst[v] == lca_ver) { lca = v; break; }
                vst[v] = lca_ver; v = par[v];
            }
        }
        for (int x : path_u) { 
            dsu_2e[x] = lca; 
            if (x == lca) { break; }
            brg_cnt--; 
        }
        for (int x : path_v) { 
            dsu_2e[x] = lca;
            if (x == lca) { break; } 
            brg_cnt--; 
        }
    }
    void reroot(int u) {
        int root = u, v = -1;
        while (u != -1) {
            int p = find2E(par[u]); 
            par[u] = v; dsu_cc[u] = root; 
            v = u; u = p;
        }
        siz_cc[root] = siz_cc[v];
    }

    void addEdge(int u, int v) {
        u = find2E(u); v = find2E(v);
        if (u == v) { return; }
        int cu = findCC(u), cv = findCC(v);
        if (cu != cv) {
            brg_cnt++;
            if (siz_cc[cu] > siz_cc[cv]) { std::swap(u, v); std::swap(cu, cv); }
            reroot(u);
            par[u] = dsu_cc[u] = v; 
            siz_cc[cv] += siz_cc[u];
        } else {
            mergePath(u, v);
        }
    }
};

// T: O(V + E), M: O(V + E)
struct StrongOrientation {
    int n, m, brg_cnt; 
    vector<int> t_in, t_lo; 
    vector<vector<pair<int, int>>> adjl; 
    vector<pair<int, int>> edgl; vector<bool> vst; 
    string orient;
    
    StrongOrientation(int n, int m) : n(n), m(m), brg_cnt(0), t_in(n, -1), t_lo(n, -1), 
                                      adjl(n), edgl(m), vst(m, false), orient(m, ' ') {}
    
    void addEdge(int u, int v, int id) { 
        adjl[u].push_back({v, id}); 
        adjl[v].push_back({u, id}); 
        edgl[id] = {u, v}; 
    }
    
    pair<int, string> solve() {
        int tm = 0, cmp_cnt = 0;
        auto dfs = [&](auto &&dfs, int u) -> void {
            t_lo[u] = t_in[u] = tm++;
            for (auto [v, id] : adjl[u]) {
                if (vst[id]) { continue; }
                vst[id] = true; 
                orient[id] = u == edgl[id].first ? '>' : '<';
                if (t_in[v] == -1) { dfs(dfs, v); brg_cnt += t_lo[v] > t_in[u]; }
                t_lo[u] = min(t_lo[u], t_lo[v]);
            }
        };
        for (int i = 0; i < n; i++) {
            if (t_in[i] == -1) { cmp_cnt++; dfs(dfs, i); }}
        
        return {brg_cnt + cmp_cnt, orient};
    }
};

// T: O(V + E), M: O(V + E)
struct TwoSat {
    int n; vector<vector<int>> adjl, adjl_t; 
    vector<bool> vst; vector<int> ord, scc;
    vector<bool> res; 
    
    TwoSat(int n) : n(n), adjl(2 * n), adjl_t(2 * n), res(n) {}
    
    void addClause(int a, bool na, int b, bool nb) {
        int u = 2 * a ^ na, nu = u ^ 1, v = 2 * b ^ nb, nv = v ^ 1;
        adjl[nu].push_back(v); 
        adjl[nv].push_back(u);
        adjl_t[v].push_back(nu); 
        adjl_t[u].push_back(nv);
    }
    
    bool solve() {
        vst.assign(2 * n, false); ord.clear();
        auto dfs1 = [&](auto &&dfs1, int u) -> void {
            vst[u] = true;
            for (int v : adjl[u]) { if (!vst[v]) { dfs1(dfs1, v); } }
            ord.push_back(u);
        };
        for (int i = 0; i < 2 * n; i++) { if (!vst[i]) { dfs1(dfs1, i); } }
        
        scc.assign(2 * n, -1);
        auto dfs2 = [&](auto &&dfs2, int u, int c) -> void {
            scc[u] = c;
            for (int v : adjl_t[u]) { if (scc[v] == -1) { dfs2(dfs2, v, c); } }
        };
        for (int i = 0, j = 0; i < 2 * n; i++) {
            int u = ord[2 * n - i - 1];
            if (scc[u] == -1) { dfs2(dfs2, u, j++); }
        }
        
        for (int i = 0; i < 2 * n; i += 2) {
            if (scc[i] == scc[i + 1]) { return false; }
            res[i & 1] = scc[i] > scc[i + 1];
        }
        return true;
    }
};

// T: O(V + E), M: O(V + E)
struct EulerPath {
    int n, edg_cnt; 
    vector<vector<pair<int, int>>> adjl; 
    
    EulerPath(int n) : n(n), edg_cnt(0), adjl(n) {}
    
    void addEdge(int u, int v) { 
        adjl[u].push_back({v, edg_cnt}); 
        adjl[v].push_back({u, edg_cnt++}); 
    }
    
    vector<int> solve() {
        if (edg_cnt == 0) { return {0}; }
        vector<int> deg(n, 0); 
        for (int i = 0; i < n; i++) { deg[i] = adjl[i].size(); }
        int odd_cnt = 0, s0 = -1, s = -1;
        for (int i = 0; i < n; i++) {
            if (deg[i] & 1) { odd_cnt++; s0 = i; }
            if (deg[i] > 0 && s == -1) { s = i; }
        }
        if (odd_cnt != 0 && odd_cnt != 2) { return {}; }
        if (s0 == -1) { s0 = s; }
        
        vector<bool> vst(edg_cnt, false); 
        vector<int> head(n, 0), st, res;
        st.push_back(s0);
        while (!st.empty()) {
            int u = st.back();
            if (head[u] < adjl[u].size()) {
                auto [v, id] = adjl[u][head[u]++];
                if (!vst[id]) { vst[id] = true; st.push_back(v); }
            } else { 
                st.pop_back(); res.push_back(u);
            }
        }
        if (res.size() != edg_cnt + 1) { return {}; }
        std::reverse(res.begin(), res.end());
        return res;
    }
};

// T: O(V + E), M: O(V)
vector<int> findDirectedCycle(int n, const vector<vector<int>> &adjl) {
    int bgn = -1, end = -1;
    vector<char> clr(n, 0); vector<int> par(n, -1); 
    
    auto dfs = [&](auto &&dfs, int u) -> bool {
        clr[u] = 1;
        for (int v : adjl[u]) {
            if (clr[v] == 0) { par[v] = u; if (dfs(dfs, v)) { return true; } } 
            else if (clr[v] == 1) { bgn = v; end = u; return true; }
        }
        clr[u] = 2; 
        return false;
    };
    for (int i = 0; i < n; i++) { if (clr[i] == 0 && dfs(dfs, i)) { break; } }
    if (bgn == -1) { return {}; }    

    vector<int> cyc; 
    cyc.push_back(bgn);
    for (int v = end; v != bgn; v = par[v]) { cyc.push_back(v); }
    cyc.push_back(bgn); 
    std::reverse(cyc.begin(), cyc.end());
    return cyc;
}

// T: O(V * E), M: O(V)
vector<int> findNegativeCycle(int n, const vector<tuple<int, int, lng>> &edgl) {
    int x; vector<int> par(n, -1); vector<lng> dis(n, 0);     
    for (int i = 0; i < n; i++) {
        x = -1;
        for (const auto &[u, v, w] : edgl) {
            if (dis[u] + w >= dis[v]) { continue; }
            x = v; par[v] = u; dis[v] = max(-INF64, dis[u] + w);  
        }
    }
    if (x == -1) { return {}; }
    for (int i = 0; i < n; i++) { x = par[x]; }
        
    vector<int> cyc;
    for (int v = x; v != x && !cyc.empty(); v = par[v]) { cyc.push_back(v); }
    std::reverse(cyc.begin(), cyc.end());
    return cyc;
}

// T: O(E * log(E)), M: O(V + E)
template<typename T> 
struct Kruskal {
    vector<int> par;
    T ttl; vector<pair<int, int>> mst;

    int findSet(int u) { return u == par[u] ? u : par[u] = findSet(par[u]);}

    void solve(int n, vector<tuple<T, int, int>> edgl) {
        if (n == 0) { ttl = 0; mst.clear(); return; }
        std::sort(edgl.begin(), edgl.end());    
        par.assign(n, 0); std::iota(par.begin(), par.end(), 0);
        ttl = 0; mst.clear();
        for (const auto &[w, u, v] : edgl) {
            int ru = findSet(u), rv = findSet(v);
            if (ru == rv) { continue; }
            par[rv] = ru;
            ttl += w; mst.push_back({u, v});
            if (mst.size() == n - 1) { break; }
        }
    }

}; 

// T: O(V^2), M: O(V)
template<typename T>
struct PrimDense {
    static constexpr T INF = std::numeric_limits<T>::max();
    
    vector<bool> vst;
    vector<T> mn_w; vector<int> mn_e; 
    T ttl; vector<pair<int, int>> mst;

    void solve(int n, const vector<vector<T>> &adjm) {
        if (n == 0) { ttl = 0; mst.clear(); return; }
        vst.assign(n, false); 
        mn_w.assign(n, INF); mn_w[0] = 0; mn_e.assign(n, -1);
        ttl = 0; mst.clear();
        for (int i = 0; i < n; i++) {
            int u = -1;
            for (int j = 0; j < n; j++) {
                if (!vst[j] && (u == -1 || mn_w[j] < mn_w[u])) { u = j; } }
            if (u == -1 || mn_w[u] == INF) { ttl = -1; mst.clear(); return; }

            vst[u] = true; 
            ttl += mn_w[u]; 
            if (mn_e[u] != -1) { mst.push_back({u, mn_e[u]}); }
            for (int v = 0; v < n; v++) {
                if (vst[v] || adjm[u][v] >= mn_w[v]) { continue; }
                mn_w[v] = adjm[u][v]; mn_e[v] = u;
            }
        }
    }
};

// T: O(V * log(V)), M: O(V)
struct CentroidDecomposition {
    int n; 
    vector<bool> vst;
    vector<int> par, siz; 

    CentroidDecomposition(int n, const vector<vector<int>> &adjl) : 
        n(n), siz(n, 0), par(n, -1), vst(n, false) {
        auto getSiz = [&](auto &&getSiz, int u, int p) -> int {
            siz[u] = 1;
            for (int v : adjl[u]) { 
                if (!vst[v] && v != p) { siz[u] += getSiz(getSiz, v, u); }}
            return siz[u];
        };
        auto getCtd = [&](auto &&getCtd, int u, int p, int usz) -> int {
            for (int v : adjl[u]) {
                if (!vst[v] && v != p && 2 * siz[v] > usz) {
                    return getCtd(getCtd, v, u, usz);}}
            return u;
        };

        auto build = [&](auto &&build, int u, int p) -> void {
            int usz = getSiz(getSiz, u, -1), ctd = getCtd(getCtd, u, -1, usz);
            vst[ctd] = true; par[ctd] = p;
            for (int v : adjl[ctd]) { if (!vst[v]) { build(build, v, ctd); } }
        };
        build(build, 0, -1);
    }
};

// TreeEdgePainting someday

// T: O(V^2 * E), M: O(V + E)
template<typename T>
struct Dinic {
    static constexpr T INF = std::numeric_limits<T>::max();
    
    struct Edge { int to; T cap, flow; int rev; };
    int n, s, t; 
    vector<int> lvl, ptr;
    vector<vector<Edge>> adjl; 
    
    Dinic(int n, int s, int t) : n(n), s(s), t(t), adjl(n), lvl(n), ptr(n) {}
    
    void addEdge(int u, int v, T cap) {
        adjl[u].push_back({v, cap, 0, adjl[v].size()});
        adjl[v].push_back({u, 0, 0, adjl[u].size() - 1});
    }
    
    bool bfs() {
        fill(lvl.begin(), lvl.end(), -1); 
        queue<int> q; 
        lvl[s] = 0; 
        q.push(s);
        
        while (!q.empty()) {
            int u = q.front(); 
            q.pop();
            for (auto &e : adjl[u]) {
                if (e.cap - e.flow > 0 && lvl[e.to] == -1) { 
                    lvl[e.to] = lvl[u] + 1; 
                    q.push(e.to); 
                }
            }
        }
        
        return lvl[t] != -1;
    }
    
    T dfs(int u, T pushed) {
        if (pushed == 0 || u == t) {
            return pushed;
        }
        for (int &i = ptr[u]; i < adj[u].size(); i++) {
            auto &e = adj[u][i];
            if (lvl[u] + 1 != lvl[e.to] || e.cap - e.flow == 0) {
                continue;
            }
            T tr = dfs(e.to, min(pushed, e.cap - e.flow));
            if (tr == 0) {
                continue;
            }
            e.flow += tr; 
            adj[e.to][e.rev].flow -= tr;
            
            return tr;
        }
        
        return 0;
    }
    
    T maxFlow() {
        T flow = 0;
        while (bfs()) {
            fill(ptr.begin(), ptr.end(), 0);
            while (T pushed = dfs(s, std::numeric_limits<T>::max())) {
                flow += pushed;
            }
        }
        
        return flow;
    }
};

// S: O(V + E), U: NA, Q: O(V^2 * E), M: O(V + E)
template<typename T>
struct FlowWithDemands {
    int n, s_prime, t_prime; 
    Dinic<T> dinic; 
    vector<T> d_in, d_out; 
    T sum_demands;
    
    FlowWithDemands(int n) : n(n), s_prime(n), t_prime(n + 1), dinic(n + 2, n, n + 1), d_in(n, 0), d_out(n, 0), sum_demands(0) {}
    
    void addEdge(int u, int v, T cap, T demand) {
        d_in[v] += demand; 
        d_out[u] += demand; 
        dinic.addEdge(u, v, cap - demand);
    }
    
    bool hasValidFlow(int s, int t) {
        dinic.addEdge(t, s, std::numeric_limits<T>::max());
        for (int i = 0; i < n; i++) {
            if (d_in[i] > 0) { 
                dinic.addEdge(s_prime, i, d_in[i]); 
                sum_demands += d_in[i]; 
            }
            if (d_out[i] > 0) { 
                dinic.addEdge(i, t_prime, d_out[i]); 
            }
        }
        
        return dinic.maxFlow() == sum_demands;
    }
};

// S: O(F * V * E), U: NA, Q: O(1), M: O(V^2)
template<typename T, typename C>
struct MaxFlowMinCut {
    struct MCFEdge { int u, v; T cap; C cost; };
    int n; 
    vector<MCFEdge> edges; 
    vector<vector<int>> adj; 
    vector<vector<C>> cost; 
    vector<vector<T>> cap;
    
    MaxFlowMinCut(int n, const vector<MCFEdge> &E) : n(n), edges(E), adj(n), cost(n, vector<C>(n, 0)), cap(n, vector<T>(n, 0)) {
        for (const auto &e : edges) {
            adj[e.u].push_back(e.v); 
            adj[e.v].push_back(e.u);
            cost[e.u][e.v] = e.cost; 
            cost[e.v][e.u] = -e.cost;
            cap[e.u][e.v] = e.cap;
        }
    }
    
    pair<T, C> solve(int s, int t, T k) {
        T flow = 0; 
        C min_cost = 0; 
        vector<C> d(n); 
        vector<int> p(n);
        
        auto spfa = [&]() -> bool {
            d.assign(n, std::numeric_limits<C>::max()); 
            p.assign(n, -1); 
            d[s] = 0;
            vector<bool> in_q(n, false); 
            queue<int> q; 
            q.push(s); 
            in_q[s] = true;
            
            while (!q.empty()) {
                int u = q.front(); 
                q.pop(); 
                in_q[u] = false;
                for (int v : adj[u]) {
                    if (cap[u][v] > 0 && d[u] + cost[u][v] < d[v]) {
                        d[v] = d[u] + cost[u][v]; 
                        p[v] = u;
                        if (!in_q[v]) { 
                            in_q[v] = true; 
                            q.push(v); 
                        }
                    }
                }
            }
            
            return d[t] != std::numeric_limits<C>::max();
        };
        
        while (flow < k) {
            if (!spfa()) {
                break;
            }
            T f = k - flow; 
            int cur = t;
            while (cur != s) { 
                f = min(f, cap[p[cur]][cur]); 
                cur = p[cur]; 
            }
            
            flow += f; 
            min_cost += f * d[t]; 
            cur = t;
            
            while (cur != s) { 
                cap[p[cur]][cur] -= f; 
                cap[cur][p[cur]] += f; 
                cur = p[cur]; 
            }
        }
        
        return {flow, min_cost};
    }
};

// S: O(V^2 * sqrt(E)) to O(V^3), U: NA, Q: O(1), M: O(V^2)
template<typename T>
struct PushRelabel {
    int n; 
    vector<vector<T>> cap, flow; 
    vector<int> h; 
    vector<T> ex;
    
    PushRelabel(int n) : n(n), cap(n, vector<T>(n, 0)), flow(n, vector<T>(n, 0)), h(n, 0), ex(n, 0) {}
    
    void addEdge(int u, int v, T c) { 
        cap[u][v] += c; 
    }
    
    T solve(int s, int t) {
        auto push = [&](int u, int v) -> void {
            T d = min(ex[u], cap[u][v] - flow[u][v]);
            flow[u][v] += d; 
            flow[v][u] -= d; 
            ex[u] -= d; 
            ex[v] += d;
        };
        
        auto relabel = [&](int u) -> void {
            int d = INF32;
            for (int i = 0; i < n; i++) {
                if (cap[u][i] - flow[u][i] > 0) {
                    d = min(d, h[i]);
                }
            }
            if (d < INF32) {
                h[u] = d + 1;
            }
        };
        
        auto getMaxH = [&]() -> vector<int> {
            vector<int> max_h;
            for (int i = 0; i < n; i++) {
                if (i != s && i != t && ex[i] > 0) {
                    if (!max_h.empty() && h[i] > h[max_h[0]]) {
                        max_h.clear();
                    }
                    if (max_h.empty() || h[i] == h[max_h[0]]) {
                        max_h.push_back(i);
                    }
                }
            }
            
            return max_h;
        };
        
        h[s] = n; 
        ex[s] = std::numeric_limits<T>::max();
        for (int i = 0; i < n; i++) {
            if (i != s) {
                push(s, i);
            }
        }
            
        vector<int> cur;
        while (!(cur = getMaxH()).empty()) {
            for (int i : cur) {
                bool pushed = false;
                for (int j = 0; j < n && ex[i]; j++) {
                    if (cap[i][j] - flow[i][j] > 0 && h[i] == h[j] + 1) { 
                        push(i, j); 
                        pushed = true; 
                    }
                }
                if (!pushed) { 
                    relabel(i); 
                    break; 
                }
            }
        }
        
        return ex[t];
    }
};

// T: O(V * E), M: O(V + E)
struct Kuhn {
    int n, m; 
    vector<vector<int>> adj; 
    vector<int> mt; 
    vector<bool> used;
    
    Kuhn(int n, int m) : n(n), m(m), adj(n), mt(m, -1) {}
    
    void addEdge(int u, int v) { 
        adj[u].push_back(v); 
    }
    
    bool dfs(int u) {
        if (used[u]) {
            return false;
        }
        used[u] = true;
        for (int v : adj[u]) {
            if (mt[v] == -1 || dfs(mt[v])) { 
                mt[v] = u; 
                return true; 
            }
        }
        
        return false;
    }
    
    int solve() {
        vector<bool> used1(n, false);
        for (int u = 0; u < n; u++) {
            for (int v : adj[u]) {
                if (mt[v] == -1) { 
                    mt[v] = u; 
                    used1[u] = true; 
                    break; 
                }
            }
        }
        
        int matches = 0;
        for (int u = 0; u < n; u++) {
            if (used1[u]) { 
                matches++; 
                continue; 
            }
            used.assign(n, false); 
            if (dfs(u)) {
                matches++;
            }
        }
        
        return matches;
    }
};


int main() {
    return 0;
}