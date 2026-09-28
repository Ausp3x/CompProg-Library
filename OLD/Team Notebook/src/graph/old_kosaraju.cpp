struct graph {
    int n;
    vector<bool> vis;
    vector<vector<vector<int>>> adj;
    vector<vector<int>> comps;

    graph(int n) : n(n) {
        vis.resize(n);
        adj.resize(2);
        for (int dir = 0; dir < 2; dir++)
            adj[dir] = vector<vector<int>> (n);
    }

    void add_edge(int u, int v) {
        adj[0][u].push_back(v);
        adj[1][v].push_back(u);
    }

    void dfs(int u, int p, int dir, vector<int> &topo) {
        vis[u] = true;
        for (int v : adj[dir][u])
            if (!vis[v] and v != p)
                dfs(v, u, dir, topo);
        topo.push_back(u);
    }
    void kosaraju() {
        vector<int> topo;
        vis.assign(n, false);
        for (int u = 0; u < n; u++)
            if (!vis[u])
                dfs(u, -1, 0, topo);
        vis.assign(n, false);
        for (int i = n-1; i >= 0; i--) {
            if (!vis[topo[i]]) {
                comps.push_back({});
                dfs(topo[i], -1, 1, comps.back());
            }
        }
    }
}