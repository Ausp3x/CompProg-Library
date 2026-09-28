// S: O(n * log(n)), U: NA, Q: O(log(n)), M: O(n * log(n))
struct LCA {
    int n, l, root, tm; 
    vector<int> dep, tin, tout; 
    vector<vector<int>> up;
    
    LCA(int n, int rt, const vector<vector<int>> &adj) : 
        n(n), root(rt), tm(0), dep(n, 0), tin(n, 0), tout(n, 0) {
        l = std::ceil(std::log2(max(n, 1))); 
        up.assign(n, vector<int>(l + 1));
        
        auto dfs = [&](auto &&dfs, int u, int p, int d) -> void {
            dep[u] = d; tin[u] = ++tm; 
            up[u][0] = p;
            for (int i = 1; i <= l; i++) { up[u][i] = up[up[u][i - 1]][i - 1]; }
            for (int v : adj[u]) {
                if (v != p) { dfs(dfs, v, u, d + 1); }}
            tout[u] = ++tm;
        };
        
        dfs(dfs, root, root, 0); 
    }

    bool isAnc(int u, int v) const { return tin[u] <= tin[v] && tout[u] >= tout[v]; }
    int getDis(int u, int v) const { return dep[u] + dep[v] - 2 * dep[getLCA(u, v)]; }
    int getKthAncestor(int u, int k) const {
        if (k <= 0) { return u; }
        if (k > dep[u]) { return -1; }
        for (int i = l; i >= 0; i--) {
            if (k & (1 << i)) { u = up[u][i]; }}
        return u;
    }
    int getLCA(int u, int v) const {
        if (isAnc(u, v)) { return u; } 
        if (isAnc(v, u)) { return v; }
        for (int i = l; i >= 0; i--) {
            if (!isAnc(up[u][i], v)) { u = up[u][i]; }}
        return up[u][0];
    }
};