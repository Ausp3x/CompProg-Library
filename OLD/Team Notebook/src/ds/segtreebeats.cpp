// S: O(n), U: O(log(n)^2), Q: O(log(n)), M: O(n)
struct SegTreeBeats {
    struct Node {
        lng sum;
        lng mx1, mx2; int mxc;
        lng mn1, mn2; int mnc;
        bool to_set; lng add_upd, set_upd;
    };
    int n;
    vector<Node> tree;

    template<typename T>
    SegTreeBeats(std::span<const T> &a) : n(a.size()), 
        tree(2 * std::bit_ceil(static_cast<unsigned>(max(n, 1)))) {
        if (n > 0) { build(1, 0, n - 1, a); }
    }
    template<typename T>
    void build(int i, int l, int r, std::span<const T> &a) {
        if (l == r) {
            tree[i].sum = a[l];
            tree[i].mx1 = a[l]; tree[i].mx2 = -INF64; tree[i].mxc = 1;
            tree[i].mn1 = a[l]; tree[i].mn2 = INF64;  tree[i].mnc = 1;
            tree[i].to_set = false; tree[i].add_upd = 0; tree[i].set_upd = 0;
            return;
        }
        int md = std::midpoint(l, r);
        build(2 * i,     l,      md, a);
        build(2 * i + 1, md + 1, r,  a);
        pull(i);
        tree[i].to_set = false; tree[i].add_upd = 0; tree[i].set_upd = 0;
    }

    inline void pull(int i) {
        int lc = 2 * i, rc = 2 * i + 1;
        tree[i].sum = tree[lc].sum + tree[rc].sum;
        if (tree[lc].mx1 == tree[rc].mx1) {
            tree[i].mx1 = tree[lc].mx1; 
            tree[i].mx2 = max(tree[lc].mx2, tree[rc].mx2);
            tree[i].mxc = tree[lc].mxc + tree[rc].mxc;
        } else if (tree[lc].mx1 > tree[rc].mx1) {
            tree[i].mx1 = tree[lc].mx1;
            tree[i].mx2 = max(tree[lc].mx2, tree[rc].mx1);
            tree[i].mxc = tree[lc].mxc;
        } else {
            tree[i].mx1 = tree[rc].mx1;
            tree[i].mx2 = max(tree[lc].mx1, tree[rc].mx2);
            tree[i].mxc = tree[rc].mxc;
        }
        if (tree[lc].mn1 == tree[rc].mn1) {
            tree[i].mn1 = tree[lc].mn1;
            tree[i].mn2 = min(tree[lc].mn2, tree[rc].mn2);
            tree[i].mnc = tree[lc].mnc + tree[rc].mnc;
        } else if (tree[lc].mn1 < tree[rc].mn1) {
            tree[i].mn1 = tree[lc].mn1;
            tree[i].mn2 = min(tree[lc].mn2, tree[rc].mn1);
            tree[i].mnc = tree[lc].mnc;
        } else {
            tree[i].mn1 = tree[rc].mn1;
            tree[i].mn2 = min(tree[lc].mn1, tree[rc].mn2);
            tree[i].mnc = tree[rc].mnc;
        }
    }
    inline void addApply(int i, int len, lng x) {        
        tree[i].sum += x * len;
        tree[i].mx1 += x; if (tree[i].mx2 != -INF64) { tree[i].mx2 += x; }
        tree[i].mn1 += x; if (tree[i].mn2 != INF64)  { tree[i].mn2 += x; }
        if (tree[i].to_set) { tree[i].set_upd += x; } 
        else { tree[i].add_upd += x; }
    }
    inline void chmaxApply(int i, lng x) {
        if (x <= tree[i].mn1) { return; }
        tree[i].sum += (x - tree[i].mn1) * tree[i].mnc;
        if (tree[i].mx1 == tree[i].mn1) { tree[i].mx1 = x; }
        if (tree[i].mx2 == tree[i].mn1) { tree[i].mx2 = x; }
        tree[i].mn1 = x;
    }
    inline void chminApply(int i, lng x) {
        if (x >= tree[i].mx1) { return; }
        tree[i].sum -= (tree[i].mx1 - x) * tree[i].mxc;
        if (tree[i].mn1 == tree[i].mx1) { tree[i].mn1 = x; }
        if (tree[i].mn2 == tree[i].mx1) { tree[i].mn2 = x; }
        tree[i].mx1 = x;
    }
    inline void setApply(int i, int len, lng x) {
        tree[i].sum = x * len;
        tree[i].mx1 = x; tree[i].mx2 = -INF64; tree[i].mxc = len;
        tree[i].mn1 = x; tree[i].mn2 = INF64;  tree[i].mnc = len;
        tree[i].to_set = true; tree[i].add_upd = 0; tree[i].set_upd = x;
    }
    inline void push(int i, int l, int r) {
        int md = std::midpoint(l, r);
        int lc = 2 * i, rc = 2 * i + 1;
        if (tree[i].to_set) {
            setApply(lc, md - l + 1, tree[i].set_upd);
            setApply(rc, r - md,     tree[i].set_upd);
            tree[i].to_set = false;
        }
        if (tree[i].add_upd != 0) {
            addApply(lc, md - l + 1, tree[i].add_upd);
            addApply(rc, r - md,     tree[i].add_upd);
            tree[i].add_upd = 0;
        }
        if (tree[i].mx1 < tree[lc].mx1) { chminApply(lc, tree[i].mx1); }
        if (tree[i].mx1 < tree[rc].mx1) { chminApply(rc, tree[i].mx1); }
        if (tree[i].mn1 > tree[lc].mn1) { chmaxApply(lc, tree[i].mn1); }
        if (tree[i].mn1 > tree[rc].mn1) { chmaxApply(rc, tree[i].mn1); }
    }

    void _addUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (ql <= l && r <= qr) { addApply(i, r - l + 1, x); return; }
        push(i, l, r);
        int md = std::midpoint(l, r);
        if (ql <= md) { _addUpdate(2 * i,     l,      md, ql, qr, x); }
        if (qr > md)  { _addUpdate(2 * i + 1, md + 1, r,  ql, qr, x); }
        pull(i);
    }
    void _chmaxUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (x <= tree[i].mn1) { return; }
        if (ql <= l && r <= qr && x < tree[i].mn2) { chmaxApply(i, x); return; }
        push(i, l, r);
        int md = std::midpoint(l, r);
        if (ql <= md) { _chmaxUpdate(2 * i,     l,      md, ql, qr, x); }
        if (qr > md)  { _chmaxUpdate(2 * i + 1, md + 1, r,  ql, qr, x); }
        pull(i);
    }
    void _chminUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (x >= tree[i].mx1) { return; }
        if (ql <= l && r <= qr && x > tree[i].mx2) { chminApply(i, x); return; }
        push(i, l, r);
        int md = std::midpoint(l, r);
        if (ql <= md) { _chminUpdate(2 * i,     l,      md, ql, qr, x); }
        if (qr > md)  { _chminUpdate(2 * i + 1, md + 1, r,  ql, qr, x); }
        pull(i);
    }
    void _setUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (ql <= l && r <= qr) { setApply(i, r - l + 1, x); return; }
        push(i, l, r);
        int md = std::midpoint(l, r);
        if (ql <= md) { _setUpdate(2 * i,     l,      md, ql, qr, x); }
        if (qr > md)  { _setUpdate(2 * i + 1, md + 1, r,  ql, qr, x); }
        pull(i);
    }
    void addUpdate(int l, int r, lng x) {
        l = max(l, 0); r = min(r, n - 1);
        if (l <= r) { _addUpdate(1, 0, n - 1, l, r, x); }
    }
    void chminUpdate(int l, int r, lng x) {
        l = max(l, 0); r = min(r, n - 1);
        if (l <= r) { _chminUpdate(1, 0, n - 1, l, r, x); }
    }
    void chmaxUpdate(int l, int r, lng x) {
        l = max(l, 0); r = min(r, n - 1);
        if (l <= r) { _chmaxUpdate(1, 0, n - 1, l, r, x); }
    }
    void setUpdate(int l, int r, lng x) {
        l = max(l, 0); r = min(r, n - 1);
        if (l <= r) { _setUpdate(1, 0, n - 1, l, r, x); }
    }

    lng _maxQuery(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) { return tree[i].mx1; }
        push(i, l, r);
        int md = std::midpoint(l, r);
        lng res = -INF64;
        if (ql <= md) { res = max(res, _maxQuery(2 * i,     l,      md, ql, qr)); }
        if (qr > md)  { res = max(res, _maxQuery(2 * i + 1, md + 1, r,  ql, qr)); }
        return res;
    }
    lng _minQuery(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) { return tree[i].mn1; }
        push(i, l, r);
        int md = std::midpoint(l, r);
        lng res = INF64;
        if (ql <= md) { res = min(res, _minQuery(2 * i,     l,      md, ql, qr)); }
        if (qr > md)  { res = min(res, _minQuery(2 * i + 1, md + 1, r,  ql, qr)); }
        return res;
    }
    lng _sumQuery(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) { return tree[i].sum; }
        push(i, l, r);
        int md = std::midpoint(l, r);
        lng res = 0;
        if (ql <= md) { res += _sumQuery(2 * i,     l,      md, ql, qr); }
        if (qr > md)  { res += _sumQuery(2 * i + 1, md + 1, r,  ql, qr); }
        return res;
    }
    lng maxQuery(int l, int r) {
        l = max(l, 0); r = min(r, n - 1);
        if (l > r) { return -INF64; }
        return _maxQuery(1, 0, n - 1, l, r);
    }
    lng minQuery(int l, int r) {
        l = max(l, 0); r = min(r, n - 1);
        if (l > r) { return INF64; }
        return _minQuery(1, 0, n - 1, l, r);
    }
    lng sumQuery(int l, int r) {
        l = max(l, 0); r = min(r, n - 1);
        if (l > r) { return 0; }
        return _sumQuery(1, 0, n - 1, l, r);
    }
};