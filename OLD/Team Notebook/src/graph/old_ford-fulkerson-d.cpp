 struct flow_network {
    vector<vector<ll>> f, c;
    vector<vector<int>> adj;
    vector<int> par;
    int n, s, t;

    flow_network(int n, int s, int t) : n(n), s(s), t(t) {
        adj.resize(n);
        par.resize(n);
        f.resize(n, vector<ll>(n, 0));
        c.resize(n, vector<ll>(n, 0));
    }
    void add_edge(int u, int v, ll cap) {
        adj[u].push_back(v);
        c[u][v] = cap;
        adj[v].push_back(u);
        c[v][u] = 0;
    }
    ll bfs() {
        fill(par.begin(), par.end(), -1);
        par[s] = -2;
        queue<pair<int, ll>> q;
        q.push({s, INF});

        while (!q.empty()) {
            int u = q.front().first;
            ll flow = q.front().second; q.pop();

            for (int v : adj[u]) {
                if (c[u][v]-f[u][v] > 0 and par[v] == -1) {
                    par[v] = u;
                    ll new_flow = min(flow, c[u][v]-f[u][v]);
                    if (v == t)
                        return new_flow;
                    q.push({v, new_flow});
                }
            }
        }
        return 0;
    }

    ll max_flow() {
        ll flow = 0;
        while (ll new_flow = bfs()) {
            flow += new_flow;
            int v = t;
            while (v != s) {
                int u = par[v];
                f[u][v] += new_flow;
                f[v][u] -= new_flow;
                v = u;
            }
        }
        return flow;
    }
};