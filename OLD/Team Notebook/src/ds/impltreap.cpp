// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename Mon>
struct ImplicitTreap {
    using S = typename Mon::S;
    using F = typename Mon::F;
    static inline std::mt19937 rng{std::chrono::steady_clock::now().time_since_epoch().count()};

    struct Node {
        int pri, siz;
        S val, acc; F lzy;
        bool to_rev; int lc = -1, rc = -1;
        Node(S v, int p) : pri(p), siz(1), val(v), acc(v), lzy(Mon::idF()), to_rev(false) {}
    };
    int root = -1;
    vector<Node> tree;

    ImplicitTreap() { tree.reserve(500'000); }
    template<typename T>
    ImplicitTreap(const std::span<const T> &a) {
        tree.reserve(2 * std::bit_ceil(static_cast<unsigned>(max<int>(a.size(), 1))));
        for (int i = 0; i < a.size(); i++) {
            root = merge(root, createNode(Mon::init(i, a[i])));}
    }
    int createNode(S v) {
        tree.emplace_back(v, rng());
        return tree.size() - 1;
    }
    int getSiz(int i) const { return i == -1 ? 0 : tree[i].siz; }
    S getAcc(int i) const { return i == -1 ? Mon::idS() : tree[i].acc; }

    void pull(int i) {
        if (i == -1) { return; }
        tree[i].siz = getSiz(tree[i].lc) + getSiz(tree[i].rc) + 1;
        tree[i].acc = Mon::ope(Mon::ope(getAcc(tree[i].lc), tree[i].val), getAcc(tree[i].rc));
    }
    void apply(int i, F f) {
        if (i == -1) { return; }
        tree[i].val = Mon::map(f, tree[i].val);
        tree[i].acc = Mon::map(f, tree[i].acc);
        tree[i].lzy = Mon::cmp(f, tree[i].lzy);
    }
    void applyRev(int i) {
        if (i == -1) { return; }
        tree[i].to_rev ^= true;
        if constexpr (requires { tree[i].acc.rev(); }) {
            tree[i].acc.rev(); tree[i].val.rev();}
    }
    void push(int i) {
        if (i == -1) { return; }
        if (tree[i].to_rev) {
            tree[i].to_rev = false;
            std::swap(tree[i].lc, tree[i].rc);
            applyRev(tree[i].lc); applyRev(tree[i].rc);
        }
        if (tree[i].lzy != Mon::idF()) {
            apply(tree[i].lc, tree[i].lzy); apply(tree[i].rc, tree[i].lzy);
            tree[i].lzy = Mon::idF();
        }
    }

    int merge(int l, int r) {
        if (l == -1 || r == -1) { return l == -1 ? r : l; }
        push(l); push(r);
        if (tree[l].pri > tree[r].pri) {
            tree[l].rc = merge(tree[l].rc, r); pull(l); return l;} 
        else {
            tree[r].lc = merge(l, tree[r].lc); pull(r); return r;}
    }
    void split(int i, int k, int &l, int &r) {
        if (i == -1) { l = -1; r = -1; return; }
        push(i);
        int cur = getSiz(tree[i].lc) + 1;
        if (k >= cur) {
            split(tree[i].rc, k - cur, tree[i].rc, r); l = i;} 
        else {
            split(tree[i].lc, k,       l, tree[i].lc); r = i;}
        pull(i);
    }

    void erase(int k) {
        if (k < 0 || k >= getSiz(root)) { return; }
        int l, md, r;
        split(root, k, l, r); split(r, 1, md, r);
        root = merge(l, r);
    }
    template<typename T>
    void insert(int k, T x) {
        int l, r;
        split(root, k, l, r);
        root = merge(merge(l, createNode(Mon::init(k, x))), r);
    }
    void reverse(int ql, int qr) {
        if (ql > qr || ql < 0 || qr >= getSiz(root)) { return; }
        int l, md, r;
        split(root, ql, l, r); split(r, qr - ql + 1, md, r);
        applyRev(md);
        root = merge(merge(l, md), r);
    }
    void update(int ql, int qr, F f) {
        if (ql > qr || ql < 0 || qr >= getSiz(root)) { return; }
        int l, md, r;
        split(root, ql, l, r); split(r, qr - ql + 1, md, r);
        if (md != -1) { apply(md, f); }
        root = merge(merge(l, md), r);
    }

    S query(int ql, int qr) {
        if (ql > qr || ql < 0 || qr >= getSiz(root)) { return Mon::idS(); }
        int l, md, r;
        split(root, ql, l, r); split(r, qr - ql + 1, md, r);
        S res = getAcc(md); 
        root = merge(merge(l, md), r);
        return res;
    }
};