#pragma once

#include "../01-Core/01-template.hpp"

namespace lichao_detail {
    // T: O(1), M: O(1)
    struct Line {
        lng m, b;

        lng eval(lng x) const { return m * x + b; }
    };

    template<bool MAX, typename T>
    inline constexpr T NONE = MAX ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();

    // T: O(1), M: O(1)
    template<bool MAX, typename T>
    bool better(const T &a, const T &b) { return MAX ? b < a : a < b; }

    // T: O(log(C)) (C = r - l), M: O(1)
    // Node v of a pointer tree is the offset of {function id, left child, right child} in t; -1 is none.
    template<bool MAX, typename F, typename T>
    T query(const vector<int> &t, const vector<F> &fs, int v, lng l, lng r, lng x) {
        T res = NONE<MAX, T>;
        while (v >= 0) {
            if (t[v] >= 0) {
                T y = fs[t[v]].eval(x);
                if (better<MAX>(y, res)) { res = y; }}
            lng md = std::midpoint(l, r);
            int c = x < md ? 1 : 2;
            (x < md ? r : l) = md;
            v = t[v + c];}
        return res;}
} // namespace lichao_detail

// Integer x in [lo, hi); functions pairwise cross at most once there, values exact and strictly inside the sentinels; query returns NONE when nothing covers x.
// S: O(1), U: O(log(C)) (C = hi - lo), Q: O(log(C)), M: O(n + s * log(C)) (s segments; UNDO adds O(log(C)) per line, O(log(C)^2) per segment)
template<bool MAX = false, bool UNDO = false, typename F = lichao_detail::Line>
struct LiChaoTree {
    using Line = lichao_detail::Line;
    using T = decltype(std::declval<const F &>().eval(lng()));
    static constexpr T NONE = lichao_detail::NONE<MAX, T>;
    lng lo, hi;
    vector<F> fs;
    vector<int> t;
    vector<pair<int, int>> his;
    vector<array<int, 3>> saves;

    LiChaoTree(lng lo, lng hi) : lo(lo), hi(hi), t{-1, -1, -1} { assert(lo < hi); }

    int node(int f) {
        t.insert(t.end(), {f, -1, -1});
        return int(t.size()) - 3;}

    void set(int p, int x) {
        if constexpr (UNDO) { his.emplace_back(p, t[p]); }
        t[p] = x;}

    void save() {
        if constexpr (UNDO) { saves.push_back({int(his.size()), int(t.size()), int(fs.size())}); }}

    bool better(int f, int g, lng x) const { return lichao_detail::better<MAX>(fs[f].eval(x), fs[g].eval(x)); }

    // Each node keeps the better function at its midpoint; the loser can only win on one side.
    void descend(int v, lng l, lng r, int f) {
        while (true) {
            int g = t[v];
            if (g < 0) { set(v, f); return; }
            lng md = std::midpoint(l, r);
            bool lef = better(f, g, l), mid = better(f, g, md);
            if (mid) { set(v, f); f = g; }
            if (md == l) { return; }
            int c = lef != mid ? 1 : 2;
            (lef != mid ? r : l) = md;
            if (t[v + c] < 0) { set(v + c, node(f)); return; }
            v = t[v + c];}}

    void segment(int v, lng l, lng r, lng a, lng b, int f) {
        if (a <= l && r <= b) { descend(v, l, r, f); return; }
        lng md = std::midpoint(l, r);
        for (int c : {1, 2}) {
            if (c == 1 ? a >= md : b <= md) { continue; }
            if (t[v + c] < 0) { set(v + c, node(-1)); }
            segment(t[v + c], c == 1 ? l : md, c == 1 ? md : r, a, b, f);}}

    void absorb(int v, const LiChaoTree &o, int u, lng l, lng r) {
        if (o.t[u] >= 0) {
            fs.push_back(o.fs[o.t[u]]);
            descend(v, l, r, int(fs.size()) - 1);}
        lng md = std::midpoint(l, r);
        for (int c : {1, 2}) {
            if (o.t[u + c] < 0) { continue; }
            if (t[v + c] < 0) { set(v + c, node(-1)); }
            absorb(t[v + c], o, o.t[u + c], c == 1 ? l : md, c == 1 ? md : r);}}

    void addLine(const F &f) {
        save();
        fs.push_back(f);
        descend(0, lo, hi, int(fs.size()) - 1);}

    // U: O(log(C)^2); f covers [l, r) intersected with [lo, hi).
    void addSegment(lng l, lng r, const F &f) {
        save();
        fs.push_back(f);
        l = max(l, lo); r = min(r, hi);
        if (l < r) { segment(0, lo, hi, l, r, int(fs.size()) - 1); }}

    // U: O(nodes of o + levels its functions move down), each function moving at most log(C) levels in all; merge the smaller tree into the larger.
    void merge(const LiChaoTree &o) {
        assert(this != &o && lo == o.lo && hi == o.hi);
        save();
        absorb(0, o, 0, lo, hi);}

    // U: O(changes of the reverted call); reverts the latest addLine, addSegment or merge.
    void undo() {
        static_assert(UNDO, "LiChaoTree::undo needs UNDO = true");
        assert(!saves.empty());
        if (saves.empty()) { return; }
        auto [h, n, k] = saves.back();
        saves.pop_back();
        for (; int(his.size()) > h; his.pop_back()) { t[his.back().first] = his.back().second; }
        t.resize(n);
        fs.erase(fs.begin() + k, fs.end());}

    T query(lng x) const {
        assert(lo <= x && x < hi);
        return lichao_detail::query<MAX, F, T>(t, fs, 0, lo, hi, x);}
};

// Versions from 0 (empty); every update returns a new version. Domain and sentinel as LiChaoTree.
// S: O(1), U: O(log(C)) (C = hi - lo), Q: O(log(C)), M: O(n * log(C) + s * log(C)^2) (s segments)
template<bool MAX = false, typename F = lichao_detail::Line>
struct PerLiChaoTree {
    using Line = lichao_detail::Line;
    using T = decltype(std::declval<const F &>().eval(lng()));
    static constexpr T NONE = lichao_detail::NONE<MAX, T>;
    lng lo, hi;
    vector<F> fs;
    vector<int> t, roots;

    PerLiChaoTree(lng lo, lng hi) : lo(lo), hi(hi), roots{-1} { assert(lo < hi); }

    void reserve(int nodes) { t.reserve(3 * size_t(nodes)); }

    int copy(int v) {
        if (v < 0) { t.insert(t.end(), {-1, -1, -1}); }
        else { t.insert(t.end(), {t[v], t[v + 1], t[v + 2]}); }
        return int(t.size()) - 3;}

    bool better(int f, int g, lng x) const { return lichao_detail::better<MAX>(fs[f].eval(x), fs[g].eval(x)); }

    int descend(int v, lng l, lng r, int f) {
        int res = copy(v);
        for (int u = res;;) {
            int g = t[u];
            if (g < 0) { t[u] = f; return res; }
            lng md = std::midpoint(l, r);
            bool lef = better(f, g, l), mid = better(f, g, md);
            if (mid) { t[u] = f; f = g; }
            if (md == l) { return res; }
            int c = lef != mid ? 1 : 2;
            (lef != mid ? r : l) = md;
            int w = copy(t[u + c]);
            t[u + c] = w;
            u = w;}}

    int segment(int v, lng l, lng r, lng a, lng b, int f) {
        if (a <= l && r <= b) { return descend(v, l, r, f); }
        int res = copy(v);
        lng md = std::midpoint(l, r);
        if (a < md) { t[res + 1] = segment(t[res + 1], l, md, a, b, f); }
        if (md < b) { t[res + 2] = segment(t[res + 2], md, r, a, b, f); }
        return res;}

    int addLine(int ver, const F &f) {
        assert(0 <= ver && ver < int(roots.size()));
        fs.push_back(f);
        roots.push_back(descend(roots[ver], lo, hi, int(fs.size()) - 1));
        return int(roots.size()) - 1;}

    // U: O(log(C)^2); f covers [l, r) intersected with [lo, hi).
    int addSegment(int ver, lng l, lng r, const F &f) {
        assert(0 <= ver && ver < int(roots.size()));
        fs.push_back(f);
        l = max(l, lo); r = min(r, hi);
        roots.push_back(l < r ? segment(roots[ver], lo, hi, l, r, int(fs.size()) - 1) : roots[ver]);
        return int(roots.size()) - 1;}

    T query(int ver, lng x) const {
        assert(0 <= ver && ver < int(roots.size()) && lo <= x && x < hi);
        return lichao_detail::query<MAX, F, T>(t, fs, roots[ver], lo, hi, x);}
};

// x among the constructor's points; functions cross at most once on them; query returns {value, line id (insertion index)}, {NONE, -1} when nothing covers x.
// S: O(m * log(m)) (m points), U: O(log(m)), Q: O(log(m)), M: O(n + m) (UNDO adds O(log(m)) per line, O(log(m)^2) per segment)
template<bool MAX = false, bool UNDO = false, typename F = lichao_detail::Line>
struct LiChaoTreeCompressed {
    using Line = lichao_detail::Line;
    using T = decltype(std::declval<const F &>().eval(lng()));
    static constexpr T NONE = lichao_detail::NONE<MAX, T>;
    int n, sz;
    vector<lng> xs;
    vector<F> fs;
    vector<int> t;
    vector<pair<int, int>> his, saves;

    explicit LiChaoTreeCompressed(vector<lng> pts) : xs(std::move(pts)) {
        sort(xs.begin(), xs.end());
        xs.erase(unique(xs.begin(), xs.end()), xs.end());
        n = int(xs.size());
        sz = int(std::bit_ceil(unsigned(max(n, 1))));
        xs.resize(sz, n > 0 ? xs.back() : 0);
        t.assign(2 * sz, -1);}

    void set(int p, int x) {
        if constexpr (UNDO) { his.emplace_back(p, t[p]); }
        t[p] = x;}

    bool better(int f, int g, int i) const { return lichao_detail::better<MAX>(fs[f].eval(xs[i]), fs[g].eval(xs[i])); }

    void descend(int v, int l, int r, int f) {
        while (true) {
            int g = t[v];
            if (g < 0) { set(v, f); return; }
            int md = (l + r) / 2;
            bool lef = better(f, g, l), mid = better(f, g, md);
            if (mid) { set(v, f); f = g; }
            if (md == l) { return; }
            v = 2 * v + (lef != mid ? 0 : 1);
            (lef != mid ? r : l) = md;}}

    void segment(int v, int l, int r, int a, int b, int f) {
        if (b <= l || r <= a) { return; }
        if (a <= l && r <= b) { descend(v, l, r, f); return; }
        int md = (l + r) / 2;
        segment(2 * v, l, md, a, b, f);
        segment(2 * v + 1, md, r, a, b, f);}

    int index(lng x) const { return int(lower_bound(xs.begin(), xs.begin() + n, x) - xs.begin()); }

    void addLine(const F &f) {
        if constexpr (UNDO) { saves.emplace_back(int(his.size()), int(fs.size())); }
        fs.push_back(f);
        if (n > 0) { descend(1, 0, sz, int(fs.size()) - 1); }}

    // U: O(log(m)^2); f covers the points in [l, r).
    void addSegment(lng l, lng r, const F &f) {
        if constexpr (UNDO) { saves.emplace_back(int(his.size()), int(fs.size())); }
        fs.push_back(f);
        segment(1, 0, sz, index(l), index(r), int(fs.size()) - 1);}

    // U: O(changes of the reverted call); reverts the latest addLine or addSegment.
    void undo() {
        static_assert(UNDO, "LiChaoTreeCompressed::undo needs UNDO = true");
        assert(!saves.empty());
        if (saves.empty()) { return; }
        auto [h, k] = saves.back();
        saves.pop_back();
        for (; int(his.size()) > h; his.pop_back()) { t[his.back().first] = his.back().second; }
        fs.erase(fs.begin() + k, fs.end());}

    pair<T, int> query(lng x) const {
        int p = index(x);
        assert(p < n && xs[p] == x);
        pair<T, int> res{NONE, -1};
        for (int v = p + sz; v > 0; v /= 2) {
            if (t[v] < 0) { continue; }
            T y = fs[t[v]].eval(x);
            if (lichao_detail::better<MAX>(y, res.first)) { res = {y, t[v]}; }}
        return res;}
};
