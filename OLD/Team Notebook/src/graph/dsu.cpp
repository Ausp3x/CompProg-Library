// S: O(n), U: O(a(n)), Q: O(a(n)), M: O(n)
struct DSU {
    int n, ncon;
    vector<int> par, siz, esz;
    vector<lng> dis;
    vector<bool> is_bip;

    DSU(int N) : n(N), ncon(N), par(n, 0), siz(n, 1), esz(n, 0), dis(n, 0), is_bip(n, true) {
        iota(par.begin(), par.end(), 0);
    }

    bool uniteSets(int u, int v, lng w = 1) {
        int ru = findSet(u); int rv = findSet(v);
        if (ru == rv) {
            esz[ru]++;
            if ((dis[u] - dis[v] - w) & 1) { is_bip[ru] = false; }
            return false;
        }
        if (siz[ru] < siz[rv]) { std::swap(ru, rv); std::swap(u, v); w = -w; }
        ncon--;
        par[rv] = ru; siz[ru] += siz[rv]; esz[ru] += esz[rv] + 1;
        dis[rv] = dis[u] - dis[v] - w;
        is_bip[ru] = is_bip[ru] && is_bip[rv];
        return true;
    }

    int findSet(int u) {
        if (u == par[u]) { return u; }
        int p = par[u];
        par[u] = findSet(p);
        dis[u] += dis[p];
        return par[u];
    }
    bool isSameSet(int u, int v) { return findSet(u) == findSet(v); }
};