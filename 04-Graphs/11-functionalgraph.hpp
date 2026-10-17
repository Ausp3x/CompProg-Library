#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// T: O(1), M: O(1); cycle_length=0 means the orbit terminates at entry, its last real vertex.
struct FunctionalOrbit { int entry, tail, cycle_length; };

// Constant-memory Floyd; next[u] is -1 or a vertex (not checked per step), -1 terminates.
// T: O(tail + cycle_length + 1), M: O(1).
inline FunctionalOrbit functionalOrbit(const vector<int> &next, int start) {
    assert(next.size() <= size_t(INT_MAX)); int n = int(next.size());
    assert(0 <= start && start < n);
    auto step = [&](int u) { return u == -1 ? -1 : next[u]; };
    int a = step(start), b = step(step(start));
    while (a != -1 && b != -1 && a != b) { a = step(a); b = step(step(b)); }
    if (a == -1 || b == -1) {
        int u = start, d = 0;
        while (step(u) != -1) { u = step(u); ++d; }
        return {u, d, 0};}
    int tail = 0; a = start;
    while (a != b) { a = step(a); b = step(b); ++tail; }
    int length = 1; b = step(a);
    while (b != a) { b = step(b); ++length; }
    return {a, tail, length};}

// Partial successor function, -1 terminates; component IDs list cycles by minimum vertex, then terminals.
// S: O(n * log(n + 1)), Q: O(log(n + 1)), M: O(n * log(n + 1)).
struct FunctionalGraph {
    int n;
    vector<int> successor, component, cycle, entry, depth, position, tin, tout;
    vector<vector<int>> cycles, up;

    explicit FunctionalGraph(vector<int> next = {}) : n(0), successor(std::move(next)) {
        assert(successor.size() <= size_t(INT_MAX)); n = int(successor.size());
        component.assign(n, -1); cycle.assign(n, -1); entry.resize(n); depth.resize(n);
        position.assign(n, -1); tin.resize(n); tout.resize(n);
        vector<int> degree(n), order;
        for (int v : successor) { assert(-1 <= v && v < n); if (v != -1) { ++degree[v]; }}
        for (int u = 0; u < n; ++u) { if (!degree[u]) { order.push_back(u); }}
        for (int i = 0; i < int(order.size()); ++i) {
            int v = successor[order[i]];
            if (v != -1 && !--degree[v]) { order.push_back(v); }}
        for (int u = 0; u < n; ++u) {
            if (!degree[u] || cycle[u] != -1) { continue; }
            int c = int(cycles.size()), v = u; cycles.emplace_back();
            do {
                component[v] = cycle[v] = c; entry[v] = v;
                position[v] = int(cycles.back().size()); cycles.back().push_back(v); v = successor[v];} while (v != u);}
        int components = int(cycles.size());
        for (int u = 0; u < n; ++u) {
            if (successor[u] == -1) { component[u] = components++; entry[u] = u; }}
        for (int i = int(order.size()) - 1; i >= 0; --i) {
            int u = order[i], v = successor[u];
            if (v == -1) { continue; }
            component[u] = component[v]; cycle[u] = cycle[v]; entry[u] = entry[v]; depth[u] = depth[v] + 1;}
        up.push_back(successor);
        for (int j = 1; j < int(std::bit_width(uint(n))); ++j) {
            up.push_back(vector<int>(n, -1));
            for (int u = 0; u < n; ++u) {
                int v = up[j - 1][u]; if (v != -1) { up[j][u] = up[j - 1][v]; }}}
        vector<vector<int>> children(n);
        for (int u = 0; u < n; ++u) { if (depth[u]) { children[successor[u]].push_back(u); }}
        int timer = 0; vector<pair<int, int>> path;
        for (int root = 0; root < n; ++root) {
            if (depth[root]) { continue; }
            tin[root] = timer++; path.push_back({root, 0});
            while (!path.empty()) {
                auto &[u, i] = path.back();
                if (i == int(children[u].size())) { tout[u] = timer; path.pop_back(); }
                else { int v = children[u][i++]; tin[v] = timer++; path.push_back({v, 0}); }}}}

    // T: O(n); directed outdegree at most one.
    template<class G>
    static vector<int> successors(const G &g) {
        assert(g.directed); vector<int> next(g.n, -1);
        for (int u = 0; u < g.n; ++u) {
            assert(g[u].size() <= 1); if (!g[u].empty()) { next[u] = g.arcs[g[u][0]].to; }}
        return next;}
    template<class G>
    explicit FunctionalGraph(const G &g) : FunctionalGraph(successors(g)) {}

    void check(int u) const { assert(0 <= u && u < n); (void)u; }
    int jump(int u, ulng k) const {
        check(u);
        if (k <= ulng(depth[u])) {
            for (int j = 0; k; ++j, k >>= 1) { if (k & 1) { u = up[j][u]; }}
            return u;}
        int c = cycle[u]; if (c == -1) { return -1; }
        int len = int(cycles[c].size()); k -= depth[u];
        return cycles[c][(ulng(position[entry[u]]) + k % len) % len];}
    // Q: O(1).
    int distance(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        if (depth[v] || cycle[v] == -1) {
            return tin[v] <= tin[u] && tin[u] < tout[v] ? depth[u] - depth[v] : -1;}
        int len = int(cycles[cycle[v]].size());
        int d = int((lng(position[v]) - position[entry[u]] + len) % len);
        return depth[u] + d;}
    bool reachable(int u, int v) const { return distance(u, v) != -1; }
    // Q: O(n); res[u] = jump(u, k) for every vertex.
    vector<int> jumpAll(ulng k) const {
        vector<int> pre(n), path(n), res(n);
        for (int u = 0; u < n; ++u) { pre[tin[u]] = u; }
        for (int u : pre) {
            path[depth[u]] = u; res[u] = k <= ulng(depth[u]) ? path[depth[u] - k] : jump(u, k);}
        return res;}

    int firstMeeting(int u, int v) const {
        check(u); check(v);
        if (u == v) { return 0; }
        if (component[u] != component[v]) { return -1; }
        if (depth[u] != depth[v]) {
            int c = cycle[u]; if (c == -1) { return -1; }
            int len = int(cycles[c].size());
            lng phase = lng(position[entry[u]]) - depth[u] - position[entry[v]] + depth[v];
            return phase % len == 0 ? max(depth[u], depth[v]) : -1;}
        if (entry[u] != entry[v]) { return -1; }
        int time = 0;
        for (int j = int(up.size()) - 1; j >= 0; --j) {
            if (up[j][u] != up[j][v]) { u = up[j][u]; v = up[j][v]; time += 1 << j; }}
        return time + 1;}

    // Q: O(n), M: O(number of cycles).
    template<class M>
    vector<typename M::S> cycleAggregates(const vector<typename M::S> &values, M mon = {}) const {
        assert(values.size() == size_t(n)); vector<typename M::S> out(cycles.size(), mon.e());
        for (int c = 0; c < int(cycles.size()); ++c) {
            for (int u : cycles[c]) { out[c] = mon.op(out[c], values[u]); }}
        return out;}
    // Q: O(cycle length).
    template<class M>
    pair<bool, typename M::S> cycleAggregate(int u, const vector<typename M::S> &values, M mon = {}) const {
        check(u); assert(values.size() == size_t(n));
        if (cycle[u] == -1) { return {false, mon.e()}; }
        int start = entry[u], v = start; typename M::S out = mon.e();
        do { out = mon.op(out, values[v]); v = successor[v]; } while (v != start);
        return {true, std::move(out)};}
};

// T: O(1), M: O(1); count vertices folded from u, next follows them (-1 after a terminal).
template<class T>
struct FunctionalFoldResult { int next; ulng count; T value; };

// Vertex labels under a monoid M (S, op, e), folded in walk order; k is a full ulng count.
// S: O(n * log(n + 1)), Q: O(log(n + 1) + log(k + 1)), M: O(n * log(n + 1)).
template<class M>
struct FunctionalGraphFold {
    using S = typename M::S;
    [[no_unique_address]] M m;
    FunctionalGraph graph;
    vector<vector<S>> table;

    FunctionalGraphFold(FunctionalGraph g, vector<S> values, M mon = {}) : m(std::move(mon)), graph(std::move(g)) {
        assert(values.size() == size_t(graph.n)); table.push_back(std::move(values));
        for (int j = 1; j < int(graph.up.size()); ++j) {
            table.push_back(vector<S>(graph.n, m.e()));
            for (int u = 0; u < graph.n; ++u) {
                int v = graph.up[j - 1][u];
                table[j][u] = v == -1 ? table[j - 1][u] : m.op(table[j - 1][u], table[j - 1][v]);}}}

    FunctionalFoldResult<S> segment(int u, int k) const {
        graph.check(u); assert(0 <= k && k <= graph.n);
        assert(graph.cycle[u] != -1 || k <= lng(graph.depth[u]) + 1);
        S value = m.e(); int count = k;
        for (int j = 0; k; ++j, k >>= 1) {
            if (k & 1) { value = m.op(value, table[j][u]); u = graph.up[j][u]; }}
        return {u, ulng(count), std::move(value)};}
    FunctionalFoldResult<S> fold(int u, ulng k) const {
        graph.check(u);
        if (graph.cycle[u] == -1) { return segment(u, int(min(k, ulng(graph.depth[u]) + 1))); }
        ulng count = k; int tail = int(min(k, ulng(graph.depth[u])));
        auto out = segment(u, tail); u = out.next; k -= tail;
        if (k) {
            int len = int(graph.cycles[graph.cycle[u]].size());
            ulng times = k / len;
            if (times) {
                S power = segment(u, len).value;
                while (times) {
                    if (times & 1) { out.value = m.op(out.value, power); }
                    times >>= 1; if (times) { power = m.op(power, power); }}}
            auto last = segment(u, int(k % len));
            out.next = last.next; out.value = m.op(out.value, last.value);}
        out.count = count; return out;}
    pair<bool, S> cycleAggregate(int u) const {
        graph.check(u); int c = graph.cycle[u];
        if (c == -1) { return {false, m.e()}; }
        return {true, segment(graph.entry[u], int(graph.cycles[c].size())).value};}
    template<class Pred>
    ulng maxStep(int u, Pred pred, ulng limit = ULLONG_MAX) const {
        graph.check(u); assert(pred(m.e()));
        S acc = m.e(); ulng k = 0;
        auto descend = [&](ulng bound) {
            for (int j = int(table.size()) - 1; j >= 0; --j) {
                if (u == -1 || bound - k < (1ULL << j)) { continue; }
                if (graph.cycle[u] == -1 && (1 << j) > graph.depth[u] + 1) { continue; }
                S next = m.op(acc, table[j][u]);
                if (pred(next)) { acc = std::move(next); k += 1ULL << j; u = graph.up[j][u]; }}};
        if (graph.cycle[u] == -1) { descend(limit); return k; }
        ulng tail = min(limit, ulng(graph.depth[u]));
        descend(tail);
        if (k < tail) { return k; }
        int len = int(graph.cycles[graph.cycle[u]].size());
        ulng laps = (limit - k) / len, taken = 0;
        vector<S> power{segment(u, len).value};
        while (power.size() < 64 && (2ULL << (power.size() - 1)) <= laps && pred(m.op(acc, power.back()))) {
            power.push_back(m.op(power.back(), power.back()));}
        for (int i = int(power.size()) - 1; i >= 0; --i) {
            if (laps - taken < (1ULL << i)) { continue; }
            S next = m.op(acc, power[i]);
            if (pred(next)) { acc = std::move(next); taken += 1ULL << i; }}
        k += taken * len; descend(limit); return k;}
};
