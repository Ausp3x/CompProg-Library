#pragma once

#include "../01-Core/01-template.hpp"

// Times [0, n), n <= 2^29; x is present on [l, r); insert/erase pair copies of an ordered X; solve needs worst-case add and undo.
// m intervals, c per add and its undo: S: O(n), U: O(log(n)) (insert, erase O(log(m)) more), Q: O(n + m * log(n) * c) for solve, M: O(n + m * log(n))
template<typename X>
struct OfflineDeletion {
    int n, sz;
    vector<pair<int, X>> seg;
    vector<int> ask;
    map<X, vector<int>> open;

    explicit OfflineDeletion(int N) : n(N), sz(int(std::bit_ceil(uint(std::clamp(N, 1, 1 << 29))))) {
        assert(0 <= n && n <= 1 << 29);
        ask.assign(2 * sz, 0);}

    void addInterval(int l, int r, const X &x) {
        assert(0 <= l && l <= r && r <= n);
        for (l += sz, r += sz; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { seg.emplace_back(l++, x); }
            if (r & 1) { seg.emplace_back(--r, x); }}}
    void insert(int t, const X &x) { assert(0 <= t && t <= n); open[x].push_back(t); }
    void erase(int t, const X &x) {
        auto it = open.find(x);
        assert(t <= n && it != open.end() && it->second.back() <= t);
        addInterval(it->second.back(), t, x);
        it->second.pop_back();
        if (it->second.empty()) { open.erase(it); }}
    void addQuery(int t) { assert(0 <= t && t < n); ask[sz + t] = 1; }

    template<typename Add, typename Undo, typename Answer>
    void solve(Add add, Undo undo, Answer answer) {
        for (auto &[x, ts] : open) { for (int t : ts) { addInterval(t, n, x); }}
        open.clear();
        vector<int> start(2 * sz + 1, 0), ord(seg.size());
        for (auto &e : seg) { ++start[e.first]; }
        for (int i = 1; i <= 2 * sz; ++i) { start[i] += start[i - 1]; }
        for (int j = int(seg.size()) - 1; j >= 0; --j) { ord[--start[seg[j].first]] = j; }
        for (int i = sz - 1; i > 0; --i) { ask[i] = ask[2 * i] | ask[2 * i + 1]; }
        auto dfs = [&](auto &&dfs, int i) -> void {
            if (!ask[i]) { return; }
            for (int j = start[i]; j < start[i + 1]; ++j) { add(seg[ord[j]].second); }
            if (i >= sz) { answer(i - sz); }
            else { dfs(dfs, 2 * i); dfs(dfs, 2 * i + 1); }
            for (int j = start[i]; j < start[i + 1]; ++j) { undo(); }};
        dfs(dfs, 1);}
};

// add(x) applies an update, undo() reverts the latest; updates commute; popFront removes the oldest update.
// S: O(1), U: O(log(n)) add and undo calls amortized, Q: NA, M: O(n)
template<typename X, typename Add = std::function<void(const X &)>, typename Undo = std::function<void()>>
struct QueueUndo {
    Add add;
    Undo undo;
    vector<pair<X, bool>> st;
    int na = 0;

    QueueUndo(Add add, Undo undo) : add(std::move(add)), undo(std::move(undo)) {}

    int size() const { return int(st.size()); }
    bool empty() const { return st.empty(); }

    void push(const X &x) { add(x); st.emplace_back(x, false); }
    // Entries marked true (A) are the oldest, oldest on top; take the top segment until it holds as many A as B, reapply B then A.
    void popFront() {
        assert(!st.empty());
        if (na <= 0) {
            for (int j = 0; j < size(); ++j) { undo(); }
            reverse(st.begin(), st.end());
            for (auto &[x, a] : st) { add(x); a = true; }
            na = size();}
        int k = size(), a = 0, b = 0;
        while (a < max(b, 1) && a < na) { if (st[--k].second) { ++a; } else { ++b; } }
        for (int j = k; j < size(); ++j) { undo(); }
        std::stable_partition(st.begin() + k, st.end(), [](const pair<X, bool> &e) { return !e.second; });
        if (a) { st.pop_back(); --na; }
        for (int j = k; j < size(); ++j) { add(st[j].first); }}
};
