struct tree {
    int n;
    vector<int> is_cut, id;
    vector<vector<int>> adj;

    tree(int _n, vector<int> &cutpoints, vector<vector<int>> &comps) {
        n = cutpoints.size() + comps.size();
        l = ceil(log2(n));
        adj.resize(n);
        id.resize(_n);
        is_cut.resize(_n, false);

        for (int i = 0; i < cutpoints.size(); i++) {
            id[cutpoints[i]] = i;
            is_cut[cutpoints[i]] = true;
        }
        for (int i = 0; i < comps.size(); i++) {
            int _id = i + cutpoints.size();
            int (int u : comps[i]) {
                if (is_cut[u])
                    add_edge(id[u], _id);
                else
                    id[u] = _id;
            }
        }
    }
    void add_edge(int u, int v) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
}