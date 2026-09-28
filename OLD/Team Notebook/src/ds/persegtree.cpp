// S: O(q * log(n)), U: O(log(n)), Q: O(log(n)), M: O(q * log(n))
template<typename Mon>
struct PerSegTree {
    using S = typename Mon::S;
    using F = typename Mon::F;

    struct Node {
        S val; F lzy; int lc = -1, rc = -1;
        Node(lng l, lng r) : val(Mon::defR(l, r)), lzy(Mon::idF()) {}
    };
    lng n;
    vector<int> roots;
    vector<Node> tree;

    PerSegTree(lng N) : n(N) {
        tree.reserve(10'000'000); 
        roots.push_back(cloneNode(-1, 0, n - 1));
    }
    template<typename T>
    PerSegTree(std::span<const T> &a) : n(a.size()) {
        tree.reserve(10'000'000); 
        roots.push_back(build(0, n - 1, a));
    }
    template<typename T>
    int build(lng l, lng r, std::span<const T> &a) {
        int cur = cloneNode(-1, l, r);
        if (l == r) { tree[cur].val = Mon::init(l, a[l]); return cur; }
        lng md = std::midpoint(l, r);
        int nlc = build(l,      md, a); tree[cur].lc = nlc;
        int nrc = build(md + 1, r,  a); tree[cur].rc = nrc;
        tree[cur].val = Mon::ope(tree[nlc].val, tree[nrc].val);
        return cur;
    }
    int cloneNode(int i, lng l, lng r) {
        if (i == -1) { tree.emplace_back(l, r); } 
        else { Node tmp = tree[i]; tree.push_back(tmp); }
        return tree.size() - 1;
    }

    inline void apply(int i, F f) {
        tree[i].val = Mon::map(f, tree[i].val);
        tree[i].lzy = Mon::cmp(f, tree[i].lzy);
    }
    inline void push(int i, lng l, lng r) {
        if (tree[i].lzy == Mon::idF()) { return; }
        lng md = std::midpoint(l, r);
        int nlc = cloneNode(tree[i].lc, l,      md); tree[i].lc = nlc;
        int nrc = cloneNode(tree[i].rc, md + 1, r);  tree[i].rc = nrc;
        apply(tree[i].lc, tree[i].lzy);
        apply(tree[i].rc, tree[i].lzy);
        tree[i].lzy = Mon::idF();
    }

    int _modify(int i1, int i2, lng l, lng r, lng ql, lng qr, F acc) {
        if (ql <= l && r <= qr) {
            int j = cloneNode(i2, l, r); apply(j, acc);
            return j;
        }
        int j = cloneNode(i1, l, r); push(j, l, r);
        int lc2 = -1, rc2 = -1; F nac = acc;
        if (i2 != -1) { 
            lc2 = tree[i2].lc; rc2 = tree[i2].rc;
            nac = Mon::cmp(acc, tree[i2].lzy);
        }
        lng md = std::midpoint(l, r);
        if (ql <= md) { 
            int nlc = _modify(tree[j].lc, lc2, l,      md, ql, qr, nac); 
            tree[j].lc = nlc; 
        }
        if (qr > md) {
            int nrc = _modify(tree[j].rc, rc2, md + 1, r,  ql, qr, nac);
            tree[j].rc = nrc;
        }
        S l_val = tree[j].lc != -1 ? tree[tree[j].lc].val : Mon::defR(l, md);
        S r_val = tree[j].rc != -1 ? tree[tree[j].rc].val : Mon::defR(md + 1, r);
        tree[j].val = Mon::ope(l_val, r_val);
        return j;
    }
    int modify(int ver1, int ver2, lng l, lng r) {
        if (l < 0 || r > n - 1 || l > r) {
            int cur = roots[ver1]; roots.push_back(cur);
            return roots.size() - 1;
        }
        int cur = _modify(roots[ver1], roots[ver2], 0, n - 1, l, r, Mon::idF());
        roots.push_back(cur);
        return roots.size() - 1;
    }

    int _update(int i, lng l, lng r, lng ql, lng qr, F f) {
        int j = cloneNode(i, l, r);
        if (ql <= l && r <= qr) { apply(j, f); return j; }
        push(j, l, r);
        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _update(tree[j].lc, l,      md, ql, qr, f);
            tree[j].lc = nlc;
        }
        if (qr > md)  {
            int nrc = _update(tree[j].rc, md + 1, r,  ql, qr, f);
            tree[j].rc = nrc;
        }
        S l_val = tree[j].lc != -1 ? tree[tree[j].lc].val : Mon::defR(l, md);
        S r_val = tree[j].rc != -1 ? tree[tree[j].rc].val : Mon::defR(md + 1, r);
        tree[j].val = Mon::ope(l_val, r_val);
        return j;
    }
    int update(int ver, lng l, lng r, F f) { 
        if (l < 0 || r > n - 1 || l > r) {
            int cur = roots[ver]; roots.push_back(cur);
            return roots.size() - 1;
        }
        int cur = _update(roots[ver], 0, n - 1, l, r, f);
        roots.push_back(cur);
        return roots.size() - 1;
    }

    template<class G>
    lng _maxR(int i, lng l, lng r, lng ql, G &g, S &acc, F tac) {
        if (r < ql) { return -1; }
        if (ql <= l) {
            S val = i == -1 ? Mon::defR(l, r) : tree[i].val; val = Mon::map(tac, val);
            S nac = Mon::ope(acc, val);
            if (g(nac)) { acc = nac; return -1; }
        }
        if (l == r) { return l - 1; }
        tac = i == -1 ? tac : Mon::cmp(tac, tree[i].lzy);
        lng md = std::midpoint(l, r);
        lng res = _maxR(i == -1 ? -1 : tree[i].lc, l, md, ql, g, acc, tac);
        if (res != -1) { return res; }
        return _maxR(i == -1 ? -1 : tree[i].rc, md + 1, r, ql, g, acc, tac);
    }
    template<class G>
    lng maxR(int ver, lng l, G g) { // Returns max r in [l, n - 1] where g(query(l, r)) is true.
        if (l < 0 || l > n - 1) { return l - 1; }
        S acc = Mon::idS();
        lng res = _maxR(roots[ver], 0, n - 1, l, g, acc, Mon::idF());
        return res == -1 ? n - 1 : res;
    }

    template<class G>
    lng _minL(int i, lng l, lng r, lng qr, G &g, S &acc, F tac) {
        if (l > qr) { return -1; }
        if (r <= qr) {
            S val = i == -1 ? Mon::defR(l, r) : tree[i].val; val = Mon::map(tac, val);
            S nac = Mon::ope(val, acc);
            if (g(nac)) { acc = nac; return -1; }
        }
        if (l == r) { return l + 1; }
        tac = i == -1 ? tac : Mon::cmp(tac, tree[i].lzy);
        lng md = std::midpoint(l, r);
        lng res = _minL(i == -1 ? -1 : tree[i].rc, md + 1, r, qr, g, acc, tac);
        if (res != -1) { return res; }
        return _minL(i == -1 ? -1 : tree[i].lc, l, md, qr, g, acc, tac);
    }
    template<class G>
    lng minL(int ver, lng r, G g) { // Returns min l in [0, r] where g(query(l, r)) is true.
        if (r < 0 || r > n - 1) { return r + 1; }
        S acc = Mon::idS();
        lng res = _minL(roots[ver], 0, n - 1, r, g, acc, Mon::idF());
        return res == -1 ? 0 : res;
    }

    S _query(int i, lng l, lng r, lng ql, lng qr, F acc) {
        if (i == -1) {
            if (max(l, ql) > min(r, qr)) { return Mon::idS(); }
            return Mon::map(acc, Mon::defR(max(l, ql), min(r, qr))); 
        }
        if (ql <= l && r <= qr) { return Mon::map(acc, tree[i].val); }
        F nac = Mon::cmp(acc, tree[i].lzy); 
        lng md = std::midpoint(l, r);
        if (qr <= md) { return _query(tree[i].lc, l,      md, ql, qr, nac); }
        if (ql > md)  { return _query(tree[i].rc, md + 1, r,  ql, qr, nac); }
        return Mon::ope(_query(tree[i].lc, l, md, ql, qr, nac), _query(tree[i].rc, md + 1, r, ql, qr, nac));
    }
    S query(int ver, lng l, lng r) { 
        l = max(l, 0LL); r = min(r, n - 1);
        if (l > r) { return Mon::idS(); }
        return _query(roots[ver], 0, n - 1, l, r, Mon::idF());
    }
    S queryAll(int ver) { return roots[ver] == -1 ? Mon::defR(0, n - 1) : tree[roots[ver]].val; }
};