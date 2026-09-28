struct graph {
    int n, timer;
    vector<vector<int>> adj, comps;
    vector<int> tin, low, vis, stk, cutpoints, bridges;

    graph(int n) : n(n) {
        adj.resize(n);
    }

    void add_edge(int u, int v) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    void dfs(int u, int p = -1) {
        vis[u] = true;
        tin[u] = low[u] = timer++;
        stk.push_back(u);
        int children = 0;
        bool has_low_child = false;
        
        for (int v : adj[u]) {
            if (v == p)
                continue;
            if (vis[v])
                low[u] = min(low[u], tin[v]);
            else {
                dfs(v, u);
                children++;
                low[u] = min(low[u], low[v]);
                if (low[v] > tin[u])
                    bridges.push_back(u);
                if (low[v] >= tin[u]) {
                    has_low_child = true;
                    comps.push_back({u});
                    while (comps.back().back() != v and !stk.empty()) {
                        comps.back().push_back(stk.back());
                        stk.pop_back();
                    }
                }
            }
        }
        if ((p == -1 and children >= 2) or (p != -1 and has_low_child))
            cutpoints.push_back(u);
    }

    void find_cutpoints_bridges() {
        vis.assign(n, false);
        tin.assign(n, -1);
        low.assign(n, -1);
        cutpoints.clear();
        bridges.clear();
        comps.clear();
        stk.clear();
        timer = 0;
        for (int u = 0; u < n; u++)
            if (!vis[u])
                dfs(u);
    }
}