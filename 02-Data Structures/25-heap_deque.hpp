#pragma once

#include "../01-Core/01-template.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"

namespace heap_detail {
    // T: O(1), M: O(1); reverses the strict weak order C.
    template<typename T, typename C>
    struct Flip {
        C c;
        bool operator()(const T &a, const T &b) const { return c(b, a); }
    };
} // namespace heap_detail

// C is a strict weak order and top() the id of a C-minimum key; ids in [0, n), n < 2^30, hold at most one key; push asserts an absent id, key/decreaseKey/erase a present one.
// S: O(n), U: O(log(n) / log(D)) push and decreaseKey, O(D * log(n) / log(D)) pop and erase, Q: O(1), M: O(n)
template<typename T, int D = 4, typename C = std::less<T>>
struct DAryHeap {
    static_assert(D >= 2);
    vector<pair<T, int>> h;
    vector<int> pos;
    C cmp;

    explicit DAryHeap(int n = 0, C cmp = C()) : pos(max(n, 0), -1), cmp(cmp) { assert(0 <= n && n < (1 << 30)); }
    explicit DAryHeap(const vector<T> &a, C cmp = C()) : pos(a.size()), cmp(cmp) {
        assert(a.size() < (1u << 30));
        for (int i = 0; i < int(a.size()); ++i) { h.emplace_back(a[i], i); pos[i] = i; }
        for (int k = size() - 1; k >= 0; --k) { down(k); }}

    int size() const { return int(h.size()); }
    bool empty() const { return h.empty(); }
    bool contains(int i) const { assert(0 <= i && i < int(pos.size())); return pos[i] >= 0; }
    int top() const { assert(!empty()); return h[0].second; }
    const T &key(int i) const { assert(contains(i)); return h[pos[i]].first; }

    void up(int k) {
        auto e = std::move(h[k]);
        for (int p; k > 0 && cmp(e.first, h[p = (k - 1) / D].first); k = p) { h[k] = std::move(h[p]); pos[h[k].second] = k; }
        pos[e.second] = k; h[k] = std::move(e);}
    void down(int k) {
        auto e = std::move(h[k]);
        int n = size(), inner = (n + D - 2) / D;
        while (k < inner) {
            int c = D * k + 1, end = min(c + D, n);
            for (int j = c + 1; j < end; ++j) { if (cmp(h[j].first, h[c].first)) { c = j; }}
            if (!cmp(h[c].first, e.first)) { break; }
            h[k] = std::move(h[c]); pos[h[k].second] = k; k = c;}
        pos[e.second] = k; h[k] = std::move(e);}
    void push(int i, const T &x) { assert(!contains(i)); h.emplace_back(x, i); up(size() - 1); }
    void pop() { assert(!empty()); if (!empty()) { erase(h[0].second); }}
    void decreaseKey(int i, const T &x) {
        assert(contains(i) && !cmp(key(i), x));
        if (pos[i] < 0) { return; }
        h[pos[i]].first = x; up(pos[i]);}
    void erase(int i) {
        assert(contains(i));
        if (pos[i] < 0) { return; }
        int k = pos[i];
        pos[i] = -1;
        auto e = std::move(h.back());
        h.pop_back();
        if (k < size()) { int j = e.second; h[k] = std::move(e); pos[j] = k; up(k); down(pos[j]); }}
};

template<typename T, typename C = std::less<T>>
using BinaryHeap = DAryHeap<T, 2, C>;

// C is a strict weak order and top() a C-minimum; erase(x) needs a present copy of x, and C-equivalent values are interchangeable.
// S: O(n), U: O(log(n)) amortized, Q: O(1), M: O(n)
template<typename T, typename C = std::less<T>>
struct ErasableHeap {
    using F = heap_detail::Flip<T, C>;
    C cmp;
    std::priority_queue<T, vector<T>, F> q, d;

    explicit ErasableHeap(const vector<T> &a = {}, C cmp = C()) : cmp(cmp), q(F{cmp}, a), d(F{cmp}) {}

    int size() const { return int(q.size() - d.size()); }
    bool empty() const { return q.size() == d.size(); }
    const T &top() const { assert(!empty()); return q.top(); }

    // Keeps q.top() live: a pending deletion equal to it cancels both.
    void clean() { while (!d.empty() && !q.empty() && !cmp(q.top(), d.top())) { q.pop(); d.pop(); }}
    void push(const T &x) { q.push(x); }
    void erase(const T &x) { d.push(x); clean(); }
    void pop() { assert(!empty()); if (!q.empty()) { q.pop(); clean(); }}
};

// C is a strict weak order and top() a C-minimum; heaps are root ids in one pool (0 is empty); push returns the new node id; applyAdd needs T += T and C kept by adding one value to every key.
// S: O(1), U: O(log(n)), Q: O(1), M: O(m) for m pushes
template<typename T, typename C = std::less<T>>
struct LeftistHeap {
    static constexpr bool ADD = requires (T x) { x += x; };
    struct Node { T key; int l, r, s, sz; };
    vector<Node> t;
    vector<T> lz;
    C cmp;

    explicit LeftistHeap(C cmp = C()) : t(1, Node{T(), 0, 0, 0, 0}), lz(ADD), cmp(cmp) {}

    int size(int v) const { return t[v].sz; }
    const T &top(int v) const { assert(v); return t[v].key; }

    void add(int v, T x) { t[v].key += x; lz[v] += x; }
    void flush(int v) {
        if constexpr (ADD) {
            if (t[v].l) { add(t[v].l, lz[v]); }
            if (t[v].r) { add(t[v].r, lz[v]); }
            lz[v] = T();}}
    int meld(int a, int b) {
        if (!a || !b) { return a ? a : b; }
        if (cmp(t[b].key, t[a].key)) { swap(a, b); }
        flush(a);
        t[a].r = meld(t[a].r, b);
        if (t[t[a].l].s < t[t[a].r].s) { swap(t[a].l, t[a].r); }
        t[a].s = t[t[a].r].s + 1; t[a].sz = t[t[a].l].sz + t[t[a].r].sz + 1;
        return a;}
    int push(int &v, const T &x) {
        t.push_back({x, 0, 0, 1, 1});
        if constexpr (ADD) { lz.push_back(T()); }
        int u = int(t.size()) - 1;
        v = meld(v, u);
        return u;}
    void pop(int &v) { assert(v); if (v) { flush(v); v = meld(t[v].l, t[v].r); }}
    void applyAdd(int v, T x) requires ADD { if (v) { add(v, x); }}
};

// C is a strict weak order and top() a C-minimum; heaps are root ids in one pool (0 is empty); push returns the new node id; applyAdd needs T += T and C kept by adding one value to every key.
// S: O(1), U: O(log(n)) amortized, Q: O(1), M: O(m) for m pushes
template<typename T, typename C = std::less<T>>
struct SkewHeap {
    static constexpr bool ADD = requires (T x) { x += x; };
    struct Node { T key; int l, r, sz; };
    vector<Node> t;
    vector<T> lz;
    C cmp;

    explicit SkewHeap(C cmp = C()) : t(1, Node{T(), 0, 0, 0}), lz(ADD), cmp(cmp) {}

    int size(int v) const { return t[v].sz; }
    const T &top(int v) const { assert(v); return t[v].key; }

    void add(int v, T x) { t[v].key += x; lz[v] += x; }
    void flush(int v) {
        if constexpr (ADD) {
            if (t[v].l) { add(t[v].l, lz[v]); }
            if (t[v].r) { add(t[v].r, lz[v]); }
            lz[v] = T();}}
    // Top-down skew merge: each winner's old right child merges into its left slot.
    int meld(int a, int b) {
        int root = 0, last = 0;
        while (a && b) {
            if (cmp(t[b].key, t[a].key)) { swap(a, b); }
            flush(a);
            t[a].sz += t[b].sz;
            (last ? t[last].l : root) = a;
            last = a; a = std::exchange(t[a].r, t[a].l);}
        (last ? t[last].l : root) = a ? a : b;
        return root;}
    int push(int &v, const T &x) {
        t.push_back({x, 0, 0, 1});
        if constexpr (ADD) { lz.push_back(T()); }
        int u = int(t.size()) - 1;
        v = meld(v, u);
        return u;}
    void pop(int &v) { assert(v); if (v) { flush(v); v = meld(t[v].l, t[v].r); }}
    void applyAdd(int v, T x) requires ADD { if (v) { add(v, x); }}
};

// C is a strict weak order and top() a C-minimum; heaps are root ids in one pool (0 is empty); push returns the new node id; Las Vegas: seed affects only time.
// S: O(1), U: O(log(n)) expected, Q: O(1), M: O(m) for m pushes
template<typename T, typename C = std::less<T>>
struct RandomizedHeap {
    struct Node { T key; int l, r, sz; };
    vector<Node> t;
    ulng seed;
    C cmp;

    explicit RandomizedHeap(ulng seed = CustomHash::rnd, C cmp = C()) : t(1, Node{T(), 0, 0, 0}), seed(seed), cmp(cmp) {}

    int size(int v) const { return t[v].sz; }
    const T &top(int v) const { assert(v); return t[v].key; }

    int meld(int a, int b) {
        int root = 0, last = 0;
        bool right = false;
        auto slot = [&]() -> int & { return !last ? root : right ? t[last].r : t[last].l; };
        while (a && b) {
            if (cmp(t[b].key, t[a].key)) { swap(a, b); }
            t[a].sz += t[b].sz;
            slot() = a;
            last = a; right = CustomHash::splitMix64(seed++) & 1;
            a = right ? t[a].r : t[a].l;}
        slot() = a ? a : b;
        return root;}
    int push(int &v, const T &x) { t.push_back({x, 0, 0, 1}); int u = int(t.size()) - 1; v = meld(v, u); return u; }
    void pop(int &v) { assert(v); if (v) { v = meld(t[v].l, t[v].r); }}
};

// C is a strict weak order and top() a C-minimum; heaps are root ids in one pool (0 is empty), the head of a root list; push returns the new node id.
// S: O(1), U: O(log(n)), Q: O(log(n)), M: O(m) for m pushes
template<typename T, typename C = std::less<T>>
struct BinomialHeap {
    struct Node { T key; int child, sib, deg, sz; };
    vector<Node> t;
    C cmp;

    explicit BinomialHeap(C cmp = C()) : t(1, Node{T(), 0, 0, 0, 0}), cmp(cmp) {}

    int size(int v) const { return t[v].sz; }
    // Root before a minimum root, 0 when the head is one.
    int minPrev(int v) const {
        int res = 0;
        for (int p = v, x = t[v].sib; x; p = x, x = t[x].sib) {
            if (cmp(t[x].key, t[res ? t[res].sib : v].key)) { res = p; }}
        return res;}
    const T &top(int v) const { assert(v); int p = minPrev(v); return t[p ? t[p].sib : v].key; }

    void link(int c, int p) { t[c].sib = t[p].child; t[p].child = c; ++t[p].deg; }
    // Merges two root lists by degree, then links equal degrees; size is kept only at the head.
    int join(int a, int b) {
        int head = 0, tail = 0;
        while (a || b) {
            int &src = !b || (a && t[a].deg <= t[b].deg) ? a : b;
            int x = src;
            src = t[x].sib;
            (tail ? t[tail].sib : head) = x; tail = x;}
        for (int prev = 0, x = head, nx; (nx = t[x].sib);) {
            if (t[x].deg != t[nx].deg || (t[nx].sib && t[t[nx].sib].deg == t[x].deg)) { prev = x; x = nx; }
            else if (!cmp(t[nx].key, t[x].key)) { t[x].sib = t[nx].sib; link(nx, x); }
            else { (prev ? t[prev].sib : head) = nx; link(x, nx); x = nx; }}
        return head;}
    int meld(int a, int b) { int n = t[a].sz + t[b].sz, v = join(a, b); t[v].sz = n; return v; }
    int push(int &v, const T &x) { t.push_back({x, 0, 0, 0, 1}); int u = int(t.size()) - 1; v = meld(v, u); return u; }
    void pop(int &v) {
        assert(v);
        if (!v) { return; }
        int n = t[v].sz - 1, p = minPrev(v), m = p ? t[p].sib : v, rev = 0;
        (p ? t[p].sib : v) = t[m].sib;
        for (int c = t[m].child; c;) { int nx = t[c].sib; t[c].sib = rev; rev = c; c = nx; }
        v = join(v, rev); t[v].sz = n;}
};

// C is a strict weak order and top() a C-minimum; heaps are root ids in one pool (0 is empty); push returns a node id that stays a handle for decreaseKey/erase until it leaves the heap.
// S: O(1), U: O(1) push and meld, O(log(n)) amortized pop, erase and decreaseKey, Q: O(1), M: O(m) for m pushes
template<typename T, typename C = std::less<T>>
struct PairingHeap {
    struct Node { T key; int child, sib, prev, sz; };
    vector<Node> t;
    vector<int> buf;
    C cmp;

    explicit PairingHeap(C cmp = C()) : t(1, Node{T(), 0, 0, 0, 0}), cmp(cmp) {}

    int size(int v) const { return t[v].sz; }
    const T &top(int v) const { assert(v); return t[v].key; }

    // prev is the left sibling, or the parent for a first child.
    int join(int a, int b) {
        if (!a || !b) { return a ? a : b; }
        if (cmp(t[b].key, t[a].key)) { swap(a, b); }
        t[b].sib = t[a].child; t[b].prev = a;
        if (t[a].child) { t[t[a].child].prev = b; }
        t[a].child = b;
        return a;}
    int pairUp(int c) {
        buf.clear();
        while (c) {
            int a = c, b = t[c].sib;
            c = b ? t[b].sib : 0;
            t[a].sib = t[a].prev = t[b].sib = t[b].prev = 0;
            buf.push_back(join(a, b));}
        int res = 0;
        for (int k = int(buf.size()) - 1; k >= 0; --k) { res = join(buf[k], res); }
        return res;}
    void detach(int x) {
        int p = t[x].prev;
        (t[p].child == x ? t[p].child : t[p].sib) = t[x].sib;
        if (t[x].sib) { t[t[x].sib].prev = p; }
        t[x].sib = t[x].prev = 0;}
    int meld(int a, int b) { int n = t[a].sz + t[b].sz, v = join(a, b); t[v].sz = n; return v; }
    int push(int &v, const T &x) { t.push_back({x, 0, 0, 0, 1}); int u = int(t.size()) - 1; v = meld(v, u); return u; }
    void pop(int &v) {
        assert(v);
        if (!v) { return; }
        int n = t[v].sz - 1;
        v = pairUp(t[v].child); t[v].sz = n;}
    void decreaseKey(int &v, int x, const T &k) {
        assert(v && x && !cmp(t[x].key, k));
        t[x].key = k;
        if (x != v) { int n = t[v].sz; detach(x); v = join(v, x); t[v].sz = n; }}
    void erase(int &v, int x) {
        assert(v && x);
        if (x == v) { pop(v); return; }
        int n = t[v].sz - 1;
        detach(x);
        v = join(v, pairUp(t[x].child)); t[v].sz = n;}
};

// C is a strict weak order and top() a C-minimum; heaps are root ids in one pool (0 is empty), a minimum root; push returns a node id that stays a handle for decreaseKey/erase until it leaves the heap.
// S: O(1), U: O(1) push and meld, O(1) amortized decreaseKey, O(log(n)) amortized pop and erase, Q: O(1), M: O(m) for m pushes
template<typename T, typename C = std::less<T>>
struct FibonacciHeap {
    struct Node { T key; int par, child, lft, rgt, deg, sz; bool mark; };
    vector<Node> t;
    vector<int> buf;
    C cmp;

    explicit FibonacciHeap(C cmp = C()) : t(1, Node{T(), 0, 0, 0, 0, 0, 0, false}), cmp(cmp) {}

    int size(int v) const { return t[v].sz; }
    const T &top(int v) const { assert(v); return t[v].key; }

    // Siblings form circular lists through lft/rgt; splice joins the lists of a and b.
    void splice(int a, int b) { int ra = t[a].rgt, lb = t[b].lft; t[a].rgt = b; t[b].lft = a; t[lb].rgt = ra; t[ra].lft = lb; }
    void unlink(int x) { t[t[x].lft].rgt = t[x].rgt; t[t[x].rgt].lft = t[x].lft; t[x].lft = t[x].rgt = x; }
    int join(int a, int b) {
        if (!a || !b) { return a ? a : b; }
        splice(a, b);
        return cmp(t[b].key, t[a].key) ? b : a;}
    void cut(int v, int x) {
        int p = t[x].par;
        if (t[p].child == x) { t[p].child = t[x].rgt == x ? 0 : t[x].rgt; }
        unlink(x); --t[p].deg;
        t[x].par = 0; t[x].mark = false;
        splice(v, x);}
    void cascade(int v, int p) {
        while (t[p].par) {
            if (!t[p].mark) { t[p].mark = true; return; }
            int q = t[p].par;
            cut(v, p); p = q;}}
    int meld(int a, int b) { int n = t[a].sz + t[b].sz, v = join(a, b); t[v].sz = n; return v; }
    int push(int &v, const T &x) {
        int u = int(t.size());
        t.push_back({x, 0, 0, u, u, 0, 1, false});
        v = meld(v, u);
        return u;}
    void pop(int &v) {
        assert(v);
        if (!v) { return; }
        int n = t[v].sz - 1, m = v;
        if (int c = t[m].child) {
            int x = c;
            do { t[x].par = 0; x = t[x].rgt; } while (x != c);
            splice(m, c); t[m].child = 0;}
        if (t[m].rgt == m) { v = 0; return; }
        buf.clear();
        for (int x = t[m].rgt; x != m; x = t[x].rgt) { buf.push_back(x); }
        unlink(m);
        array<int, 64> tab{};
        for (int x : buf) {
            for (int d = t[x].deg; tab[d]; ++d) {
                int y = std::exchange(tab[d], 0);
                if (cmp(t[y].key, t[x].key)) { swap(x, y); }
                unlink(y); t[y].par = x; t[y].mark = false;
                if (t[x].child) { splice(t[x].child, y); } else { t[x].child = y; }
                ++t[x].deg;}
            tab[t[x].deg] = x;}
        v = 0;
        for (int y : tab) {
            if (y && (!v || cmp(t[y].key, t[v].key))) { v = y; }}
        t[v].sz = n;}
    void decreaseKey(int &v, int x, const T &k) {
        assert(v && x && !cmp(t[x].key, k));
        t[x].key = k;
        int p = t[x].par;
        if (p && cmp(k, t[p].key)) { cut(v, x); cascade(v, p); }
        if (cmp(k, t[v].key)) { t[x].sz = t[v].sz; v = x; }}
    void erase(int &v, int x) {
        assert(v && x);
        if (int p = t[x].par) { cut(v, x); cascade(v, p); }
        t[x].sz = t[v].sz; v = x;
        pop(v);}
};

// C is a strict weak order; min()/max() are C-extremes and pops assert a nonempty queue.
// S: O(n), U: O(log(n)), Q: O(1), M: O(n)
template<typename T, typename C = std::less<T>>
struct DoubleEndedPriorityQueue {
    vector<T> a;
    C cmp;

    // Node k keeps the interval [a[2k], a[2k + 1]] inside its parent's; a last single slot is both ends.
    explicit DoubleEndedPriorityQueue(const vector<T> &v = {}, C cmp = C()) : a(v), cmp(cmp) {
        for (int k = (size() + 1) / 2 - 1; k >= 0; --k) {
            if (2 * k + 1 < size() && cmp(a[2 * k + 1], a[2 * k])) { swap(a[2 * k], a[2 * k + 1]); }
            downMin(2 * k); downMax(std::min(2 * k + 1, size() - 1));}}

    int size() const { return int(a.size()); }
    bool empty() const { return a.empty(); }
    const T &min() const { assert(!empty()); return a[0]; }
    const T &max() const { assert(!empty()); return a[size() == 1 ? 0 : 1]; }

    void upMin(int i) {
        for (int p; i > 1 && cmp(a[i], a[p = (i / 2 - 1) / 2 * 2]); i = p) { swap(a[i], a[p]); }}
    void upMax(int i) {
        for (int p; i > 1 && cmp(a[p = (i / 2 - 1) / 2 * 2 + 1], a[i]); i = p) { swap(a[i], a[p]); }}
    void downMin(int i) {
        for (int n = size(), c; (c = 2 * i + 2) < n; i = c) {
            if (c + 2 < n && cmp(a[c + 2], a[c])) { c += 2; }
            if (!cmp(a[c], a[i])) { break; }
            swap(a[i], a[c]);
            if (c + 1 < n && cmp(a[c + 1], a[c])) { swap(a[c], a[c + 1]); }}}
    void downMax(int i) {
        for (int n = size(), c; (c = i / 2 * 4 + 2) < n; i = c) {
            int m = std::min(c + 1, n - 1);
            if (c + 2 < n && cmp(a[m], a[std::min(c + 3, n - 1)])) { m = std::min(c + 3, n - 1); }
            if (!cmp(a[i], a[m])) { break; }
            swap(a[i], a[m]); c = m;
            if ((c & 1) && cmp(a[c], a[c - 1])) { swap(a[c], a[c - 1]); }}}
    void push(const T &x) {
        a.push_back(x);
        int i = size() - 1;
        if (i & 1) {
            if (cmp(a[i], a[i - 1])) { swap(a[i], a[i - 1]); upMin(i - 1); } else { upMax(i); }}
        else if (i > 0) {
            int p = (i / 2 - 1) / 2 * 2;
            if (cmp(a[i], a[p])) { upMin(i); } else if (cmp(a[p + 1], a[i])) { upMax(i); }}}
    void popMin() {
        assert(!empty());
        if (empty()) { return; }
        a[0] = std::move(a.back()); a.pop_back(); downMin(0);}
    void popMax() {
        assert(!empty());
        if (size() <= 2) { if (!empty()) { a.pop_back(); } return; }
        a[1] = std::move(a.back()); a.pop_back(); downMax(1);}
};

// C is a strict weak order; sum() adds the min(k, size) C-greatest values and sumRest() the others; erase(x) needs a present copy; boundary() asserts a kept value.
// S: O(1), U: O(log(n)) amortized per insert, erase and unit change of k, Q: O(1), M: O(n)
template<typename T, typename C = std::less<T>>
struct PrioritySum {
    using F = heap_detail::Flip<T, C>;
    int k;
    T in_sum = T(), out_sum = T();
    C cmp;
    ErasableHeap<T, C> in;
    ErasableHeap<T, F> out;

    explicit PrioritySum(int k = 0, C cmp = C()) : k(max(k, 0)), cmp(cmp), in({}, cmp), out({}, F{cmp}) { assert(k >= 0); }

    int size() const { return in.size() + out.size(); }
    const T &sum() const { return in_sum; }
    const T &sumRest() const { return out_sum; }
    const T &boundary() const { assert(!in.empty()); return in.top(); }

    void moveIn() { T x = out.top(); out.pop(); out_sum -= x; in.push(x); in_sum += x; }
    void moveOut() { T x = in.top(); in.pop(); in_sum -= x; out.push(x); out_sum += x; }
    void fix() {
        while (in.size() > k) { moveOut(); }
        while (in.size() < k && !out.empty()) { moveIn(); }}
    void insert(T x) { in.push(x); in_sum += x; fix(); }
    void erase(T x) {
        if (!in.empty() && !cmp(x, in.top())) { in.erase(x); in_sum -= x; } else { out.erase(x); out_sum -= x; }
        fix();}
    void setK(int k) { assert(k >= 0); this->k = max(k, 0); fix(); }
};
