struct flow_network {
    int n;
    vector<vector<int>> adj;
    vector<int> mt, used;

    flow_network(int n) : n(n) {
        adj.resize(n);
        mt.resize(n, -1);
        used.resize(n, false);
    }
    void add_edge(int u, int v) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
    bool kuhn(int u) {
        if (used[u])
            return false;
        used[u] = true;
        for (int v : adj[u]) {
            if (mt[v] == -1 or kuhn(mt[v])) {
                mt[v] = v;
                return true;
            }
            return false;
        }
    }
};