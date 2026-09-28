
struct edge {
    int u, v; ld cap, cost, flow;
    edge(int u, int v, ld cap, ld cost) : u(u), v(v), cap(cap), cost(cost), flow(0) {}
    ld res() {
        return cap - flow;
    }
};

struct flow_network {
    int n, s, t, *par, *in_queue, *num_vis;
    ld *cost, *pot;
    vector<edge> edges;
    vector<int> *adj;
    map<pair<int,int>, vector<int>> edge_idx;

    flow_network(int n, int s, int t) : n(n), s(s), t(t) { 
        adj = new vector<int>[n];
        par = new int[n];
        in_queue = new int[n];
        num_vis = new int[n];
        cost = new ld[n];
        pot = new ld[n];
        for (int i = 0; i < n; i++) {
            pot[i] = 0;
        }
    }
    void add_edge(int u, int v, ld cap, ld cost) {
        adj[u].push_back(edges.size());
        edge_idx[{u, v}].push_back(edges.size());
        edges.push_back({u, v, cap, cost});
        
        adj[v].push_back(edges.size());
        edge_idx[{v, u}].push_back(edges.size());
        edges.push_back({v, u, 0, -cost});   
    }
    ld get_flow(int u, int v) {
        ld flow = 0;
        for (int i : edge_idx[{u, v}])
            flow += edges[i].flow;
        return flow;
    }
    void bellman_ford() {
        for (int i = 0; i < n; i++)
            pot[i] = INF;
        pot[s] = 0;
        for (int it = 0; it < n-1; it++) {
            for (auto e : edges) {
                if (e.res() > 0)
                    pot[e.v] = min(pot[e.v], pot[e.u] + e.cost);
            }
        }
    }
    bool spfa() { 
        queue<int> q; q.push(s);
        while (not q.empty()) {
            int u = q.front(); q.pop(); 
            in_queue[u] = 0;
            if (num_vis[u]++ >= n) {
                cost[u] = -INF;
                return false;
            }
            for (int v : adj[u]) {
                edge e = edges[v];
                if (e.res() <= 0)
                    continue;
                ld nd = cost[u] + e.cost + pot[u] - pot[e.v];
                if (cost[e.v] > nd) {
                    cost[e.v] = nd;
                    par[e.v] = v;
                    if (not in_queue[e.v]) {
                        q.push(e.v);
                        in_queue[e.v] = true;
                    }
                }
            }
        }
        return cost[t] != INF;
    }
    bool aug_path() {
        for (int i = 0; i < n; i++) {
            par[i] = -1;
            in_queue[i] = 0;
            num_vis[i] = 0;
            cost[i] = INF;
        }
        cost[s] = 0;
        in_queue[s] = true;
        return spfa();
    }
    pdd max_flow(bool do_bellman_ford=false) {
        ld total_cost = 0, total_flow = 0;
        if (do_bellman_ford)
            bellman_ford();
        while (aug_path()) {
            ld flow = INF;
            for (int i = par[t]; i != -1; i = par[edges[i].u])
                flow = min(flow, edges[i].res());
            for (int i = par[t]; i != -1; i = par[edges[i].u]) {
                edges[i].flow += flow;
                edges[i^1].flow -= flow;
            }
            total_cost += flow * (cost[t] + pot[t] - pot[s]);
            total_flow += flow;
            for (int i = 0; i < n; i++) {
                if (par[i] != -1)
                    pot[i] += cost[i];
            }
        }
        return {total_cost, total_flow};
    }
};
// When updating cost, make sure to update the back edge to use -cost
