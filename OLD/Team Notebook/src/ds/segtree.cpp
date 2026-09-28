// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename Mon>
struct SegTree {    
    using S = typename Mon::S;
    using F = typename Mon::F;

    int n;
    vector<S> tree;
    vector<F> lazy;

    template<typename T>
    SegTree(std::span<const T> &a) : n(a.size()), 
        tree(2 * std::bit_ceil(static_cast<unsigned>(max(n, 1))), Mon::idS()), 
        lazy(2 * std::bit_ceil(static_cast<unsigned>(max(n, 1))), Mon::idF()) {
        if (n > 0) { build(1, 0, n - 1, a); }
    }
    template<typename T>
    void build(int i, int l, int r, std::span<const T> &a) {
        if (l == r) { tree[i] = Mon::init(l, a[l]); return; }
        int md = std::midpoint(l, r);
        build(2 * i,     l,      md, a);
        build(2 * i + 1, md + 1, r,  a);
        tree[i] = Mon::ope(tree[2 * i], tree[2 * i + 1]);
    }

    inline void apply(int i, F f) {
        tree[i] = Mon::map(f, tree[i]);
        lazy[i] = Mon::cmp(f, lazy[i]);
    }
    inline void push(int i) {
        if (lazy[i] == Mon::idF()) { return; }
        apply(2 * i,     lazy[i]);
        apply(2 * i + 1, lazy[i]);
        lazy[i] = Mon::idF();
    }

    void _update(int i, int l, int r, int ql, int qr, F f) {
        if (ql <= l && r <= qr) { apply(i, f); return; }
        push(i);
        int md = std::midpoint(l, r);
        if (ql <= md) { _update(2 * i,     l,      md, ql, qr, f); }
        if (qr > md)  { _update(2 * i + 1, md + 1, r,  ql, qr, f); }
        tree[i] = Mon::ope(tree[2 * i], tree[2 * i + 1]);
    }
    void update(int l, int r, F f) { 
        l = max(l, 0); r = min(r, n - 1);
        if (l <= r) { _update(1, 0, n - 1, l, r, f); }
    }

    template<class G>
    int _maxR(int i, int l, int r, int ql, G &g, S &acc) {
        if (r < ql) { return -1; }
        if (ql <= l) {
            S nac = Mon::ope(acc, tree[i]);
            if (g(nac)) { acc = nac; return -1; }
        }
        if (l == r) { return l - 1; } 
        push(i);
        int md = std::midpoint(l, r);
        int res = _maxR(2 * i, l, md, ql, g, acc);
        if (res != -1) { return res; }
        return _maxR(2 * i + 1, md + 1, r, ql, g, acc);
    }
    template<class G>
    int maxR(int l, G g) { // Returns max r in [l, n - 1] where g(query(l, r)) is true.
        l = max(l, 0);
        if (l > n - 1) { return l - 1; }
        S acc = Mon::idS();
        int res = _maxR(1, 0, n - 1, l, g, acc);
        return res == -1 ? n - 1 : res;
    }

    template<class G>
    int _minL(int i, int l, int r, int qr, G &g, S &acc) {
        if (l > qr) { return -1; }
        if (r <= qr) {
            S nac = Mon::ope(tree[i], acc);
            if (g(nac)) { acc = nac; return -1; }
        }
        if (l == r) { return l + 1; }
        push(i);
        int md = std::midpoint(l, r);
        int res = _minL(2 * i + 1, md + 1, r, qr, g, acc);
        if (res != -1) { return res; }
        return _minL(2 * i, l, md, qr, g, acc);
    }
    template<class G>
    int minL(int r, G g) { // Returns min l in [0, r] where g(query(l, r)) is true.
        r = min(r, n - 1);
        if (r < 0) { return 0; }
        S acc = Mon::idS();
        int res = _minL(1, 0, n - 1, r, g, acc);
        return res == -1 ? 0 : res;
    }

    S _query(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) { return tree[i]; }
        push(i);
        int md = std::midpoint(l, r);
        if (qr <= md) { return _query(2 * i,     l,      md, ql, qr); }
        if (ql > md)  { return _query(2 * i + 1, md + 1, r,  ql, qr); }
        return Mon::ope(_query(2 * i, l, md, ql, qr), _query(2 * i + 1, md + 1, r, ql, qr));
    }
    S query(int l, int r) { 
        l = max(l, 0); r = min(r, n - 1);
        if (l > r) { return Mon::idS(); }
        return _query(1, 0, n - 1, l, r); 
    }
    S queryAll() { return tree[1]; }
};