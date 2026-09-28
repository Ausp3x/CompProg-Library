bool dfs(int u) {
    color[u] = 1;
    for (int v : adj[u]) {
        if (color[v] == 0) {
            if (dfs(v))
                return true;
        } else if (color[v] == 1) {
            cycles.push_back({v, u});
            return true;
        }
    }
    color[u] = 2;
    return false;
}
void find_cycles() {
    for (int u = 0; u < n; u++) {
        if (color[u] == 0)
            dfs(u);
    }

    for (int i = 0; i < cycles.size(); i++) {
        auto& [start, end] = cycles[i];
        for (int v = end; v != start; v = prev[v]) {
            cycle_grp[v] = i+1;
        }
        auto it = find(adj[end].begin(), adj[end].end(), start);
        adj[end].erase(it);
        cycle_grp[start] = i+1;
    }
}