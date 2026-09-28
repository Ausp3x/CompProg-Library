void bellman_ford(int src) {
    dist[src] = 0;

    for (int k = 0; k < n-1; k++) {
        for (int u = 0; u < n; u++) {
            for (auto e : adj[u]) {
                int v = e.second;
                ll w = e.first;
                if (dist[u] < INF) { // if graph has negative weights
                    dist[v] = min(dist[v], dist[u] + w);
                }
            }
        }
    }
}

bool has_negative_cycle() { // call after running bellman-ford
    for (int u = 0; u < n; u++) {
        for (auto e : adj[u]) {
            int v = e.second;
            ll w = e.first;
            if (dist[v] > dist[u] + w)
                return true;
        }
    }
    return false;
}