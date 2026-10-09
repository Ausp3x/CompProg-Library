#pragma once
#include "../01-Core/01-template.hpp"

namespace assignment_detail {
    // Rows [0, n) and virtual row n, which owns every free column; pc[j] is the row of column j (-1 while it is the search target).
    template<class T>
    struct State {
        vector<T> u, v;
        vector<int> pc, pr;
    };

    // Shortest alternating path from row r on reduced costs; ends at the first free column (target -1) or at column target.
    template<class T, class Ok>
    bool search(int n, int m, const vector<T> &c, State<T> &st, int r, int target, Ok &&ok) {
        const T INF = std::numeric_limits<T>::max();
        vector<T> dist(m, INF);
        vector<int> from(m, -1), settled;
        vector<uint8_t> done(m);
        auto relax = [&](int i, T d, int via) {
            for (int j = 0; j < m; ++j) {
                if (done[j] || !ok(i, j) || (i < n && c[size_t(i) * m + j] == INF)) { continue; }
                T nd = d + (i == n ? T(0) : c[size_t(i) * m + j]) - st.u[i] - st.v[j];
                if (nd < dist[j]) { dist[j] = nd; from[j] = via; }}};
        relax(r, T(0), -1);
        bool virt = false;
        T dv = 0, f = 0;
        int end = -1;
        while (end == -1) {
            int j = -1;
            for (int k = 0; k < m; ++k) {
                if (!done[k] && dist[k] < INF && !(virt && st.pc[k] == n) && (j == -1 || dist[k] < dist[j])) { j = k; }}
            if (j == -1) { return false; }
            done[j] = 1; settled.push_back(j);
            if (target == -1 ? st.pc[j] == n : j == target) { end = j; f = dist[j]; }
            else if (st.pc[j] != n) { relax(st.pc[j], dist[j], j); }
            else if (!virt) { virt = true; dv = dist[j]; relax(n, dv, j); }}
        st.u[r] += f;
        if (virt) { st.u[n] += f - dv; }
        for (int j : settled) {
            if (j != end && st.pc[j] != n) { st.u[st.pc[j]] += f - dist[j]; }}
        for (int j = 0; j < m; ++j) {
            if (dist[j] < f) { st.v[j] -= f - dist[j]; }}
        for (int k = end;;) {
            int p = from[k], i = p == -1 ? r : st.pc[p];
            st.pc[k] = i;
            if (i < n) { st.pr[i] = k; }
            if (p == -1) { break; }
            k = p;}
        return true;}

    // Internal n <= m cost matrix (transposed when rows exceed columns, negated when maximizing), INF marks a forbidden pair.
    template<class T>
    vector<T> internal(const vector<vector<T>> &a, bool maximize, const vector<vector<uint8_t>> &allowed, int &n, int &m, bool &tr) {
        int rows = int(a.size()), cols = rows ? int(a[0].size()) : 0;
        assert(std::all_of(a.begin(), a.end(), [&](const vector<T> &row) { return int(row.size()) == cols; }));
        assert(allowed.empty() || (int(allowed.size()) == rows && std::all_of(allowed.begin(), allowed.end(), [&](const vector<uint8_t> &row) { return int(row.size()) == cols; })));
        tr = rows > cols; n = tr ? cols : rows; m = tr ? rows : cols;
        vector<T> c(size_t(n) * m);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                int x = tr ? j : i, y = tr ? i : j;
                c[size_t(i) * m + j] = !allowed.empty() && !allowed[x][y] ? std::numeric_limits<T>::max() : maximize ? -a[x][y] : a[x][y];}}
        return c;}
} // namespace assignment_detail

// T: O(1), M: O(n + m); row[i] / col[j] partner or -1; potentials u, v certify optimality; other fields meaningless when !feasible.
template<class T>
struct AssignmentResult {
    bool feasible = false;
    T cost = T(0);
    vector<int> row, col;
    vector<T> u, v;
};

// T: O(n^2 * m), M: O(n * m); n = min(rows, cols), m = max; assigns min(rows, cols) pairs; feasible false if forbidden pairs prevent it.
template<class T>
AssignmentResult<T> hungarian(const vector<vector<T>> &a, bool maximize = false, const vector<vector<uint8_t>> &allowed = {}) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
    int n, m; bool tr;
    auto c = assignment_detail::internal(a, maximize, allowed, n, m, tr);
    int rows = int(a.size()), cols = rows ? int(a[0].size()) : 0;
    AssignmentResult<T> res{true, T(0), vector<int>(rows, -1), vector<int>(cols, -1), vector<T>(rows), vector<T>(cols)};
    assignment_detail::State<T> st{vector<T>(n + 1), vector<T>(m), vector<int>(m, n), vector<int>(n, -1)};
    for (int r = 0; r < n; ++r) {
        if (!assignment_detail::search(n, m, c, st, r, -1, [](int, int) { return true; })) {
            res.feasible = false; fill(res.u.begin(), res.u.end(), T(0)); fill(res.v.begin(), res.v.end(), T(0)); return res;}}
    for (int i = 0; i < n; ++i) {
        int x = tr ? st.pr[i] : i, y = tr ? i : st.pr[i];
        res.row[x] = y; res.col[y] = x; res.cost += a[x][y];}
    for (int i = 0; i < n; ++i) { (tr ? res.v : res.u)[i] = maximize ? -st.u[i] : st.u[i]; }
    for (int j = 0; j < m; ++j) { (tr ? res.u : res.v)[j] = maximize ? -st.v[j] : st.v[j]; }
    return res;}

// T: O(k * n^2 * m + k * n * log(k * n)), M: O(k * (n + m) + n * m); up to k (cost, row -> col) pairs in best-first order, fewer if exhausted.
template<class T>
vector<pair<T, vector<int>>> kBestAssignments(const vector<vector<T>> &a, int k, bool maximize = false, const vector<vector<uint8_t>> &allowed = {}) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
    assert(k >= 0);
    int n, m; bool tr;
    auto c = assignment_detail::internal(a, maximize, allowed, n, m, tr);
    int rows = int(a.size());
    struct Node { assignment_detail::State<T> st; int f; vector<int> banned; };
    vector<Node> nodes;
    vector<pair<T, vector<int>>> res;
    auto cost = [&](const assignment_detail::State<T> &st) {
        T s = 0;
        for (int i = 0; i < n; ++i) { s += c[size_t(i) * m + st.pr[i]]; }
        return s;};
    // Child t of node p: rows < t keep their columns, row t may not use banned columns, then row t re-augments to its freed column.
    vector<uint8_t> ban(m);
    auto child = [&](int p, int t, Node &out) {
        const Node &par = nodes[p];
        out = {par.st, t, t == par.f ? par.banned : vector<int>{}};
        int col = par.st.pr[t];
        out.banned.push_back(col);
        out.st.pc[col] = -1; out.st.pr[t] = -1;
        for (int j : out.banned) { ban[j] = 1; }
        const auto &pc = out.st.pc;
        bool ok = assignment_detail::search(n, m, c, out.st, t, col, [&](int i, int j) { return !(pc[j] >= 0 && pc[j] < t) && !(i == t && ban[j]); });
        for (int j : out.banned) { ban[j] = 0; }
        return ok;};
    Node root{{vector<T>(n + 1), vector<T>(m), vector<int>(m, n), vector<int>(n, -1)}, 0, {}};
    for (int r = 0; r < n; ++r) {
        if (!assignment_detail::search(n, m, c, root.st, r, -1, [](int, int) { return true; })) { return res; }}
    std::priority_queue<tuple<T, int, int>, vector<tuple<T, int, int>>, std::greater<>> heap;
    heap.emplace(cost(root.st), -1, 0);
    Node cur;
    while (int(res.size()) < k && !heap.empty()) {
        auto [s, p, t] = heap.top(); heap.pop();
        if (p == -1) { cur = root; }
        else { child(p, t, cur); }
        vector<int> assign(rows, -1);
        for (int i = 0; i < n; ++i) { assign[tr ? cur.st.pr[i] : i] = tr ? i : cur.st.pr[i]; }
        res.emplace_back(maximize ? -s : s, std::move(assign));
        nodes.push_back(std::move(cur));
        int id = int(nodes.size()) - 1;
        Node next;
        for (int r = nodes[id].f; r < n; ++r) {
            if (child(id, r, next)) { heap.emplace(cost(next.st), id, r); }}}
    return res;}

// T: O(1), M: O(out); matched edge IDs ascending and curve[j] = optimal cost of a j-edge matching for j <= edges.size().
template<class T>
struct SparseAssignment {
    T cost;
    vector<int> edges;
    vector<T> curve;
};

// T: O(k * (V + E) * log(V + E)), M: O(V + E); k = result size; maximum cardinality first unless maxCardinality is false (optimal any size).
template<class T>
SparseAssignment<T> linearSumAssignment(int nl, int nr, const vector<tuple<int, int, T>> &edges, bool maximize = false, bool maxCardinality = true) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
    assert(nl >= 0 && nr >= 0);
    const T INF = std::numeric_limits<T>::max();
    int nv = nl + nr, ne = int(edges.size());
    vector<vector<int>> adj(nl);
    vector<T> w(ne), h(nv), dist(nv);
    T low = 0;
    for (int e = 0; e < ne; ++e) {
        auto [u, v, x] = edges[e];
        assert(0 <= u && u < nl && 0 <= v && v < nr);
        adj[u].push_back(e); w[e] = maximize ? -x : x; low = min(low, w[e]);}
    for (int v = 0; v < nr; ++v) { h[nl + v] = low; }
    vector<int> ml(nl, -1), mr(nr, -1), pe(nv);
    SparseAssignment<T> res{T(0), {}, {T(0)}};
    T hr = low, total = 0;
    std::priority_queue<pair<T, int>, vector<pair<T, int>>, std::greater<>> pq;
    while (true) {
        fill(dist.begin(), dist.end(), INF);
        for (int u = 0; u < nl; ++u) {
            if (ml[u] == -1) { dist[u] = -h[u]; pq.emplace(dist[u], u); }}
        int end = -1;
        while (!pq.empty()) {
            auto [d, x] = pq.top(); pq.pop();
            if (d != dist[x]) { continue; }
            if (x >= nl) {
                if (mr[x - nl] == -1) { end = x; break; }
                int e = mr[x - nl], y = std::get<0>(edges[e]);
                T nd = d - w[e] + h[x] - h[y];
                if (nd < dist[y]) { dist[y] = nd; pe[y] = e; pq.emplace(nd, y); }
                continue;}
            for (int e : adj[x]) {
                int y = nl + std::get<1>(edges[e]);
                if (e == ml[x]) { continue; }
                T nd = d + w[e] + h[x] - h[y];
                if (nd < dist[y]) { dist[y] = nd; pe[y] = e; pq.emplace(nd, y); }}}
        pq = {};
        if (end == -1) { break; }
        T len = dist[end], gain = len + hr;
        if (!maxCardinality && gain >= 0) { break; }
        for (int x = 0; x < nv; ++x) { h[x] += min(dist[x], len); }
        hr += len; total += gain;
        res.curve.push_back(maximize ? -total : total);
        for (int y = end - nl;;) {
            int e = pe[nl + y], x = std::get<0>(edges[e]), old = ml[x];
            ml[x] = mr[y] = e;
            if (old == -1) { break; }
            y = std::get<1>(edges[old]);}}
    for (int u = 0; u < nl; ++u) {
        if (ml[u] != -1) { res.edges.push_back(ml[u]); res.cost += std::get<2>(edges[ml[u]]); }}
    sort(res.edges.begin(), res.edges.end());
    return res;}

// S: O(n * m), U: O(1) addEdge, Q: O(n^2 * m) solve, M: O(n * m); legacy adapter: unset pairs cost 0, getAssignment after solve.
template<class T>
struct Hungarian {
    int n, m;
    vector<vector<T>> a;
    AssignmentResult<T> res;

    Hungarian(int N, int M) : n(N), m(M) { assert(n >= 0 && m >= 0); a.assign(n, vector<T>(m)); }

    void addEdge(int i, int j, T w) {
        assert(0 <= i && i < n && 0 <= j && j < m); a[i][j] = w;}
    T solve() { res = hungarian(a); return res.cost; }
    vector<int> getAssignment() const { return res.row; }
};
