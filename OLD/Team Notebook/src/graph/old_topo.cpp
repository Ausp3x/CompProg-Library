void toposort(int u) {
    visited[u] = true;
    
    for (auto v : graph[u]) 
        if (!visited[v]) {
            toposort(v);
        }
    stack.push(u); // the key difference between dfs and topological sort
}
	