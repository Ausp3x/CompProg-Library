// S: O(n^2), U: O(log(n)^2), Q: O(log(n)^2), M: O(n^2)
template<typename Mon>
struct SegTree2D {
    using S = typename Mon::S;
    using F = typename Mon::F;

    int n, m;
    vector<vector<S>> tree;

    template<typename T>
    SegTree2D(const vector<vector<T>> &a) : n(a.size()), m(a.empty() ? 0 : a[0].size()) {
        if (n == 0 || m == 0) { return; }
        tree.assign(4 * n, vector<S>(4 * m, Mon::idS()));
        build(1, 0, n - 1, a);
    }
    template<typename T>
    void _build(int i, int l1, int r1, int j, int l2, int r2, const vector<vector<T>> &a) {
        if (l2 == r2) {
            if (l1 == r1) { tree[i][j] = Mon::init(l2, a[l1][l2]); }
            else          { tree[i][j] = Mon::ope(tree[2 * i][j], tree[2 * i + 1][j]); }
            return;
        }
        int md = l2 + (r2 - l2) / 2;
        _build(i, l1, r1, 2 * j,     l2,     md, a);
        _build(i, l1, r1, 2 * j + 1, md + 1, r2, a);
        tree[i][j] = Mon::ope(tree[i][2 * j], tree[i][2 * j + 1]);
    }
    template<typename T>
    void build(int i, int l1, int r1, const vector<vector<T>> &a) {
        if (l1 != r1) {
            int md = l1 + (r1 - l1) / 2;
            build(2 * i,     l1,     md, a);
            build(2 * i + 1, md + 1, r1, a);
        }
        _build(i, l1, r1, 1, 0, m - 1, a);
    }
    
    void _update_j(int i, int l1, int r1, int j, int l2, int r2, int qj, F f) {
        if (l2 == r2) {
            if (l1 == r1) { tree[i][j] = Mon::map(f, tree[i][j]); }
            else          { tree[i][j] = Mon::ope(tree[2 * i][j], tree[2 * i + 1][j]); }
            return;
        }
        int md = l2 + (r2 - l2) / 2;
        if (qj <= md) { _update_j(i, l1, r1, 2 * j,     l2,     md, qj, f); }
        else          { _update_j(i, l1, r1, 2 * j + 1, md + 1, r2, qj, f); }    
        if (l1 == r1) { tree[i][j] = Mon::ope(tree[i][2 * j], tree[i][2 * j + 1]); } 
        else          { tree[i][j] = Mon::ope(tree[2 * i][j], tree[2 * i + 1][j]); }
    }
    void _update_i(int i, int l1, int r1, int qi, int qj, F f) {
        if (l1 != r1) {
            int md = l1 + (r1 - l1) / 2;
            if (qi <= md) { _update_i(2 * i,     l1,     md, qi, qj, f); }
            else          { _update_i(2 * i + 1, md + 1, r1, qi, qj, f); }
        }
        _update_j(i, l1, r1, 1, 0, m - 1, qj, f);
    }
    void update(int i, int j, F f) {
        if (i >= 0 && i < n && j >= 0 && j < m) { _update_i(1, 0, n - 1, i, j, f); }
    }

    S _query_j(int i, int j, int l2, int r2, int qj1, int qj2) {
        if (qj1 <= l2 && r2 <= qj2) { return tree[i][j]; }
        int md = l2 + (r2 - l2) / 2;
        if (qj2 <= md) { return _query_j(i, 2 * j,     l2,     md, qj1, qj2); }
        if (qj1 > md)  { return _query_j(i, 2 * j + 1, md + 1, r2, qj1, qj2); }
        return Mon::ope(_query_j(i, 2 * j,     l2,     md, qj1, qj2), 
                        _query_j(i, 2 * j + 1, md + 1, r2, qj1, qj2));
    }
    S _query_i(int i, int l1, int r1, int qi1, int qj1, int qi2, int qj2) {
        if (qi1 <= l1 && r1 <= qi2) { return _query_j(i, 1, 0, m - 1, qj1, qj2); }
        int md = l1 + (r1 - l1) / 2;
        if (qi2 <= md) { return _query_i(2 * i,     l1,     md, qi1, qj1, qi2, qj2); }
        if (qi1 > md)  { return _query_i(2 * i + 1, md + 1, r1, qi1, qj1, qi2, qj2); }
        return Mon::ope(_query_i(2 * i,     l1,     md, qi1, qj1, qi2, qj2), 
                        _query_i(2 * i + 1, md + 1, r1, qi1, qj1, qi2, qj2));
    }
    S query(int i1, int j1, int i2, int j2) {
        i1 = max(i1, 0); i2 = min(i2, n - 1);
        j1 = max(j1, 0); j2 = min(j2, m - 1);
        if (i1 > i2 || j1 > j2) { return Mon::idS(); }
        return _query_i(1, 0, n - 1, i1, j1, i2, j2);
    }
    S queryAll() {
        if (n == 0 || m == 0) { return Mon::idS(); }
        return tree[1][1];
    }
};