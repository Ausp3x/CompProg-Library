struct edge {
    int u, v;
    ll c, f;
    ll resid() { return c - f; }
};
struct flow_network {
    vector<edge> edges;
    vector<vector<int>> adj;
    vector<int> level, ptr;
    queue<int> q;
    int n, s, t;
    flow_network(int n, int s, int t) : n(n), s(s), t(t) {
        adj.resize(n);
        level.resize(n);
        ptr.resize(n);
    }
    void add_edge(int u, int v, ll cap) {
        edges.push_back({u, v, cap, 0});
        adj[u].push_back(len(edges)-1);
        edges.push_back({v, u, 0, 0});
        adj[v].push_back(len(edges)-1);
    }
    bool bfs() {
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int id : adj[u]) {
                if (edges[id].c == edges[id].f)
                    continue;
                if (level[edges[id].v] != -1)
                    continue;
                level[edges[id].v] = level[u] + 1;
                q.push(edges[id].v);
            } 
        }
        return level[t] != -1;
    }
    ll dfs(int u, ll pushed) {
        if (pushed == 0)
            return 0;
        if (u == t)
            return pushed;
        for (int& cid = ptr[u]; cid < len(adj[u]); cid++) {
            int id = adj[u][cid];
            int v = edges[id].v;
            if (level[u] + 1 != level[v])
                continue;
            ll tr = dfs(v, min(pushed, edges[id].resid()));
            if (tr == 0)
                continue;
            edges[id].f += tr;
            edges[id ^ 1].f -= tr;
            return tr;
        }
        return 0;
    }
    ll flow() {
        ll f = 0;
        while (true) {
            fill(level.begin(), level.end(), -1);
            level[s] = 0;
            q.push(s);
            if (!bfs()) 
                break;
            fill(ptr.begin(), ptr.end(), 0);
            while (ll pushed = dfs(s, INF)) {
                f += pushed;
            }
        }  
        return f;
    }
};