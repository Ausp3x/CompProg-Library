#pragma once

#include "../01-Core/01-template.hpp"

// M is a monoid (S, op, e); fold is the product from the oldest to the newest element, e() when empty.
// S: O(1), U: O(1) amortized, Q: O(1), M: O(n)
template<typename M>
struct SWAGQueue {
    using S = typename M::S;
    vector<pair<S, S>> fr;
    vector<S> bk;
    S acc = M::e();

    int size() const { return int(fr.size() + bk.size()); }
    bool empty() const { return fr.empty() && bk.empty(); }
    const S &front() const { assert(!empty()); return fr.empty() ? bk.front() : fr.back().first; }
    const S &back() const { assert(!empty()); return bk.empty() ? fr.front().first : bk.back(); }

    void push(const S &x) { bk.push_back(x); acc = M::op(acc, x); }
    void pop() {
        assert(!empty());
        if (fr.empty()) {
            for (int k = int(bk.size()) - 1; k >= 0; --k) { fr.emplace_back(bk[k], fr.empty() ? bk[k] : M::op(bk[k], fr.back().second)); }
            bk.clear(); acc = M::e();}
        if (!fr.empty()) { fr.pop_back(); }}
    void clear() { fr.clear(); bk.clear(); acc = M::e(); }

    S fold() const { return fr.empty() ? acc : M::op(fr.back().second, acc); }
};

// M is a monoid (S, op, e); fold is the product from the front to the back, e() when empty.
// S: O(1), U: O(1) amortized, Q: O(1), M: O(n)
template<typename M>
struct SWAGDeque {
    using S = typename M::S;
    vector<pair<S, S>> fr, bk;

    int size() const { return int(fr.size() + bk.size()); }
    bool empty() const { return fr.empty() && bk.empty(); }
    const S &front() const { assert(!empty()); return fr.empty() ? bk.front().first : fr.back().first; }
    const S &back() const { assert(!empty()); return bk.empty() ? fr.front().first : bk.back().first; }

    S frFold() const { return fr.empty() ? M::e() : fr.back().second; }
    S bkFold() const { return bk.empty() ? M::e() : bk.back().second; }
    void pushFront(const S &x) { fr.emplace_back(x, M::op(x, frFold())); }
    void pushBack(const S &x) { bk.emplace_back(x, M::op(bkFold(), x)); }
    void rebalance() {
        vector<S> a;
        for (int k = int(fr.size()) - 1; k >= 0; --k) { a.push_back(fr[k].first); }
        for (auto &p : bk) { a.push_back(p.first); }
        int h = (int(a.size()) + fr.empty()) / 2;
        fr.clear(); bk.clear();
        for (int k = h - 1; k >= 0; --k) { pushFront(a[k]); }
        for (int k = h; k < int(a.size()); ++k) { pushBack(a[k]); }}
    void popFront() {
        assert(!empty());
        if (fr.empty()) { rebalance(); }
        if (!fr.empty()) { fr.pop_back(); }}
    void popBack() {
        assert(!empty());
        if (bk.empty()) { rebalance(); }
        if (!bk.empty()) { bk.pop_back(); }}
    void clear() { fr.clear(); bk.clear(); }

    S fold() const { return M::op(frFold(), bkFold()); }
};

// S: O(1), U: O(1), Q: O(1), M: O(n)
template<typename M>
struct SWAGStack {
    using S = typename M::S;
    vector<pair<S, S>> st;

    int size() const { return int(st.size()); }
    bool empty() const { return st.empty(); }
    const S &top() const { assert(!empty()); return st.back().first; }

    void push(const S &x) { st.emplace_back(x, M::op(fold(), x)); }
    void pop() { assert(!empty()); if (!st.empty()) { st.pop_back(); }}
    void clear() { st.clear(); }

    S fold() const { return st.empty() ? M::e() : st.back().second; }
};
