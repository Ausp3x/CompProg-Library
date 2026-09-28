// S: O(q * log(n)), U: O(log(n)), Q: O(log(n)), M: O(q * log(n))
template<typename Mon>
struct DynSegTree {
    using S = typename Mon::S;
    using F = typename Mon::F;

    struct Node {
        S val; F lzy; int lc = -1, rc = -1;
        Node(lng l, lng r) : val(Mon::defR(l, r)), lzy(Mon::idF()) {}
    };
    lng n;
    int root = -1;
    vector<Node> tree;
    
    DynSegTree(lng N) : n(N) {
        tree.reserve(500'000);
        if (n > 0) { root = createNode(0, n - 1); }
    }
    template<typename T>
    DynSegTree(std::span<const T> &a) : n(a.size()) {
        tree.reserve(2 * std::bit_ceil(static_cast<unsigned>(max(n, 1LL))));
        if (n > 0) { root = build(0, n - 1, a); }
    }
    template<typename T>
    int build(lng l, lng r, std::span<const T> &a) {
        int cur = createNode(l, r);
        if (l == r) { tree[cur].val = Mon::init(l, a[l]); return cur; }
        lng md = std::midpoint(l, r);
        int nlc = build(l,      md, a); tree[cur].lc = nlc;
        int nrc = build(md + 1, r,  a); tree[cur].rc = nrc;
        tree[cur].val = Mon::ope(tree[nlc].val, tree[nrc].val);
        return cur;
    }
    int createNode(lng l, lng r) {
        tree.emplace_back(l, r);
        return tree.size() - 1;
    }

    inline void apply(int i, F f) {
        tree[i].val = Mon::map(f, tree[i].val);
        tree[i].lzy = Mon::cmp(f, tree[i].lzy);
    }
    inline void push(int i, lng l, lng r) {
        if (tree[i].lzy == Mon::idF()) { return; }
        lng md = std::midpoint(l, r);
        if (tree[i].lc == -1) { int nlc = createNode(l,      md); tree[i].lc = nlc; }
        if (tree[i].rc == -1) { int nrc = createNode(md + 1, r);  tree[i].rc = nrc; }
        apply(tree[i].lc, tree[i].lzy);
        apply(tree[i].rc, tree[i].lzy);
        tree[i].lzy = Mon::idF();
    }

    void _update(int i, lng l, lng r, lng ql, lng qr, F f) {
        if (ql <= l && r <= qr) { apply(i, f); return; }
        push(i, l, r);
        lng md = std::midpoint(l, r);
        if (ql <= md) {
            if (tree[i].lc == -1) { int nlc = createNode(l, md); tree[i].lc = nlc; }
            _update(tree[i].lc, l,      md, ql, qr, f);
        }
        if (qr > md) {
            if (tree[i].rc == -1) { int nrc = createNode(md + 1, r); tree[i].rc = nrc; }
            _update(tree[i].rc, md + 1, r,  ql, qr, f);
        }
        S l_val = tree[i].lc != -1 ? tree[tree[i].lc].val : Mon::defR(l, md);
        S r_val = tree[i].rc != -1 ? tree[tree[i].rc].val : Mon::defR(md + 1, r);
        tree[i].val = Mon::ope(l_val, r_val);
    }
    void update(lng l, lng r, F f) { 
        l = max(l, 0LL); r = min(r, n - 1);
        if (l <= r) { _update(root, 0, n - 1, l, r, f); }
    }

    template<class G>
    lng _maxR(int i, lng l, lng r, lng ql, G &g, S &acc) {
        if (r < ql) { return -1; }
        if (ql <= l) {
            S nac = Mon::ope(acc, i == -1 ? Mon::defR(l, r) : tree[i].val);
            if (g(nac)) { acc = nac; return -1; }
        }
        if (l == r) { return l - 1; }
        if (i != -1) { push(i, l, r); }
        lng md = std::midpoint(l, r);
        lng res = _maxR(i == -1 ? -1 : tree[i].lc, l, md, ql, g, acc);
        if (res != -1) { return res; }
        return _maxR(i == -1 ? -1 : tree[i].rc, md + 1, r, ql, g, acc);
    }
    template<class G>
    lng maxR(lng l, G g) { // Returns max r in [l, n - 1] where g(query(l, r)) is true.
        if (l < 0 || l > n - 1) { return l - 1; }
        S acc = Mon::idS();
        lng res = _maxR(root, 0, n - 1, l, g, acc);
        return res == -1 ? n - 1 : res;
    }

    template<class G>
    lng _minL(int i, lng l, lng r, lng qr, G &g, S &acc) {
        if (l > qr) { return -1; }
        if (r <= qr) { 
            S nac = Mon::ope(i == -1 ? Mon::defR(l, r) : tree[i].val, acc);
            if (g(nac)) { acc = nac; return -1; }
        }
        if (l == r) { return l + 1; }
        if (i != -1) { push(i, l, r); }
        lng md = std::midpoint(l, r);
        lng res = _minL(i == -1 ? -1 : tree[i].rc, md + 1, r, qr, g, acc);
        if (res != -1) { return res; }
        return _minL(i == -1 ? -1 : tree[i].lc, l, md, qr, g, acc);
    }
    template<class G>
    lng minL(lng r, G g) { // Returns min l in [0, r] where g(query(l, r)) is true.
        if (r < 0 || r > n - 1) { return r + 1; }
        S acc = Mon::idS();
        lng res = _minL(root, 0, n - 1, r, g, acc);
        return res == -1 ? 0 : res;
    }

    S _query(int i, lng l, lng r, lng ql, lng qr) {
        if (i == -1) {
            if (max(l, ql) > min(r, qr)) { return Mon::idS(); }
            return Mon::defR(max(l, ql), min(r, qr));
        }
        if (ql <= l && r <= qr) { return tree[i].val; }
        push(i, l, r);
        lng md = std::midpoint(l, r);
        if (qr <= md) { return _query(tree[i].lc, l,      md, ql, qr); }
        if (ql > md) { return _query(tree[i].rc, md + 1, r,  ql, qr); }  
        return Mon::ope(_query(tree[i].lc, l, md, ql, qr), _query(tree[i].rc, md + 1, r, ql, qr));
    }
    S query(lng l, lng r) { 
        l = max(l, 0LL); r = min(r, n - 1);
        if (l > r) { return Mon::idS(); }
        return _query(root, 0, n - 1, l, r);
    }
    S queryAll() { return root == -1 ? Mon::defR(0, n - 1) : tree[root].val; }
};