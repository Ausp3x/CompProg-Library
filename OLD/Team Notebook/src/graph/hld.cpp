// S: O(n), U: O(log(n)) or O(log(n)^2), Q: O(log(n)) or O(log(n)^2), M: O(n)
struct HLD {
    int n, tm = 0;
    vector<int> dep, par, siz, head, in, out, rev;
    vector<vector<int>> adj;

    HLD(int N) : n(N), dep(N), par(N, -1), siz(N), head(N), in(N), out(N), rev(N), adj(N) {}

    void addEdge(int u, int v) { adj[u].push_back(v); adj[v].push_back(u); }
    void build(int root = 0) { tm = 0; dfsSiz(root, -1, 0); dfsHld(root, -1, root); }
    void dfsSiz(int u, int p, int d) {
        dep[u] = d; par[u] = p; siz[u] = 1;
        if (adj[u].size() > 1 && adj[u][0] == p) { std::swap(adj[u][0], adj[u][1]); }
        for (int &v : adj[u]) {
            if (v == p) { continue; }
            dfsSiz(v, u, d + 1);
            siz[u] += siz[v];
            if (siz[v] > siz[adj[u][0]]) { std::swap(v, adj[u][0]); }
        }
    }
    void dfsHld(int u, int p, int h) {
        head[u] = h; in[u] = tm++; rev[in[u]] = u;
        for (int v : adj[u]) {
            if (v == p) { continue; }
            dfsHld(v, u, (v == adj[u][0] ? h : v));
        }
        out[u] = tm - 1;
    }

    int getDis(int u, int v) { return dep[u] + dep[v] - 2 * dep[getLCA(u, v)]; }
    int getLCA(int u, int v) {
        while (head[u] != head[v]) {
            if (dep[head[u]] < dep[head[v]]) { std::swap(u, v); }
            u = par[head[u]];
        }
        return dep[u] < dep[v] ? u : v;
    }
    int kthAnc(int u, int k) {
        if (dep[u] < k) { return -1; }
        while (dep[u] - dep[head[u]] < k) {
            k -= dep[u] - dep[head[u]] + 1;
            u = par[head[u]];
        }
        return rev[in[u] - k];
    }

    template<typename F>
    void processPath(int u, int v, bool is_edge, F f) {
        while (head[u] != head[v]) {
            if (dep[head[u]] < dep[head[v]]) { std::swap(u, v); }
            f(in[head[u]], in[u]);
            u = par[head[u]];
        }
        if (dep[u] > dep[v]) { std::swap(u, v); }
        if (u == v && is_edge) { return; }
        f(in[u] + is_edge, in[v]);
    }
    template<typename F>
    void processSubt(int u, bool is_edge, F f) {
        if (in[u] == out[u] && is_edge) { return; }
        f(in[u] + is_edge, out[u]);
    }

    template<typename T>
    vector<T> reorderArray(const vector<T> &a) {
        vector<T> res(n);
        for (int i = 0; i < n; i++) { res[in[i]] = a[i]; }
        return res;
    }
};