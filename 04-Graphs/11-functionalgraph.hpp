#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// entry is the first cycle vertex, or the last real vertex before termination.
// tail counts edges to entry; cycle_length=0 means termination, not a self-loop.
// T: O(1), M: O(1).
struct FunctionalOrbit { int entry, tail, cycle_length; };

// T: O(tail + cycle_length + 1), M: O(1). Floyd's single-orbit alternative.
// next.size() fits int, next[u] is -1 or a valid vertex, start is valid.
// Only reached endpoints are checked; -1 terminates and is never a vertex.
inline FunctionalOrbit functionalOrbit(const vector<int> &next, int start) {
    assert(next.size() <= size_t(INT_MAX)); int n = int(next.size());
    assert(0 <= start && start < n);
    auto step = [&](int u) -> int {
        if (u == -1) { return -1; }
        assert(-1 <= next[u] && next[u] < n); return next[u];
    };
    int a = step(start), b = step(step(start));
    while (a != -1 && b != -1 && a != b) { a = step(a); b = step(step(b)); }
    if (a == -1 || b == -1) {
        int u = start, d = 0;
        while (step(u) != -1) { u = step(u); ++d; }
        return {u, d, 0}; }
    int tail = 0; a = start;
    while (a != b) { a = step(a); b = step(b); ++tail; }
    int length = 1; b = step(a);
    while (b != a) { b = step(b); ++length; }
    return {a, tail, length}; }

// Owning immutable snapshot of a partial successor function; all sizes fit int.
// -1 terminates (no absorbing vertex). Graph/CSR adapters require directed
// outdegree <=1, reject even duplicate outgoing arcs, and ignore edge weights.
// cycle[u]=-1 for terminating components; entry[u] is a cycle vertex or terminal;
// depth[u] counts edges to entry, position is -1 off-cycle. Each cycles[c] starts
// at its minimum vertex and follows successors. Component IDs put cycles first,
// in increasing minimum-vertex order, then terminals in increasing vertex order.
// tin/tout are half-open reverse-tree intervals, with each cycle vertex a root.
// Public state is read-only; assignment replaces the snapshot.
// S: O(n * log(n + 1)), Q: O(log(n + 1)), M: O(n * log(n + 1)).
// Decomposition is O(n); the table uses max(1, bit_width(n)) levels.
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
                position[v] = int(cycles.back().size()); cycles.back().push_back(v); v = successor[v];
            } while (v != u); }
        int components = int(cycles.size());
        for (int u = 0; u < n; ++u) {
            if (successor[u] == -1) { component[u] = components++; entry[u] = u; }}
        for (int i = int(order.size()) - 1; i >= 0; --i) {
            int u = order[i], v = successor[u];
            if (v == -1) { continue; }
            component[u] = component[v]; cycle[u] = cycle[v]; entry[u] = entry[v]; depth[u] = depth[v] + 1; }
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
                else { int v = children[u][i++]; tin[v] = timer++; path.push_back({v, 0}); }}} }

    // O(n) adapter on the required outdegree-at-most-one domain.
    template<class G>
    static vector<int> successors(const G &g) {
        assert(g.directed); vector<int> next(g.n, -1);
        for (int u = 0; u < g.n; ++u) {
            assert(g[u].size() <= 1); if (!g[u].empty()) { next[u] = g.arcs[g[u][0]].to; }}
        return next; }
    template<class G>
    explicit FunctionalGraph(const G &g) : FunctionalGraph(successors(g)) {}

    void check(int u) const { assert(0 <= u && u < n); (void)u; }
    // Full unsigned 64-bit k. Returns -1 after leaving a terminal vertex.
    int jump(int u, ulng k) const {
        check(u);
        if (k <= ulng(depth[u])) {
            for (int j = 0; k; ++j, k >>= 1) { if (k & 1) { u = up[j][u]; }}
            return u; }
        int c = cycle[u]; if (c == -1) { return -1; }
        int len = int(cycles[c].size()); k -= depth[u];
        return cycles[c][(ulng(position[entry[u]]) + k % len) % len]; }
    // O(1). Shortest directed walk length; -1 for unreachable, self-distance 0.
    int distance(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        if (depth[v] || cycle[v] == -1) {
            return tin[v] <= tin[u] && tin[u] < tout[v] ? depth[u] - depth[v] : -1; }
        int len = int(cycles[cycle[v]].size());
        int d = int((lng(position[v]) - position[entry[u]] + len) % len);
        return depth[u] + d; }
    bool reachable(int u, int v) const { return distance(u, v) != -1; }

    // Earliest t with jump(u,t)==jump(v,t)>=0, or -1. Both move one edge/tick.
    int firstMeeting(int u, int v) const {
        check(u); check(v);
        if (u == v) { return 0; }
        if (component[u] != component[v]) { return -1; }
        if (depth[u] != depth[v]) {
            int c = cycle[u]; if (c == -1) { return -1; }
            int len = int(cycles[c].size());
            lng phase = lng(position[entry[u]]) - depth[u] - position[entry[v]] + depth[v];
            return phase % len == 0 ? max(depth[u], depth[v]) : -1; }
        if (entry[u] != entry[v]) { return -1; }
        int time = 0;
        for (int j = int(up.size()) - 1; j >= 0; --j) {
            if (up[j][u] != up[j][v]) { u = up[j][u]; v = up[j][v]; time += 1 << j; }}
        return time + 1; }

    // O(n) monoid operations, O(number of cycles) returned values. Labels can
    // represent vertices or their outgoing edges. Order starts at cycles[c][0].
    template<class T, class Op>
    vector<T> cycleAggregates(const vector<T> &values, T identity, Op op) const {
        assert(values.size() == size_t(n)); vector<T> out(cycles.size(), identity);
        for (int c = 0; c < int(cycles.size()); ++c) {
            for (int u : cycles[c]) { out[c] = op(out[c], values[u]); }}
        return out; }
    // O(cycle length) operations, O(1) values; order starts at entry[u].
    // Terminating components return {false,identity}; tail labels are excluded.
    template<class T, class Op>
    pair<bool, T> cycleAggregate(int u, const vector<T> &values, T identity, Op op) const {
        check(u); assert(values.size() == size_t(n));
        if (cycle[u] == -1) { return {false, identity}; }
        int start = entry[u], v = start; T out = identity;
        do { out = op(out, values[v]); v = successor[v]; } while (v != start);
        return {true, std::move(out)}; }
};

// Folds count visited vertices starting at u; next is the following vertex.
// A terminal is consumed once, then next=-1 and count may be below the request.
// T: O(1), M: O(1) label values, excluding label payload cost.
template<class T>
struct FunctionalFoldResult { int next; ulng count; T value; };

// Associative const-callable op with two-sided identity; order is preserved.
// Operation arithmetic/overflow and label storage are the caller's contract.
// Owning graph/label snapshot. Bounds count constant-size monoid operations.
// S: O(n * log(n + 1)), Q: O(log(n + 1) + log(k + 1)), M: O(n * log(n + 1)).
// k is requested vertex count (full ulng); no 64-level table is needed.
template<class T, class Op>
struct FunctionalGraphFold {
    FunctionalGraph graph;
    T identity;
    Op op;
    vector<vector<T>> table;

    FunctionalGraphFold(FunctionalGraph g, vector<T> values, T id, Op operation)
        : graph(std::move(g)), identity(std::move(id)), op(std::move(operation)) {
        assert(values.size() == size_t(graph.n)); table.push_back(std::move(values));
        for (int j = 1; j < int(graph.up.size()); ++j) {
            table.push_back(vector<T>(graph.n, identity));
            for (int u = 0; u < graph.n; ++u) {
                int v = graph.up[j - 1][u];
                table[j][u] = v == -1 ? table[j - 1][u] : op(table[j - 1][u], table[j - 1][v]); }} }

    // O(log(n+1)); helper for 0<=k<=n actually available vertices.
    FunctionalFoldResult<T> segment(int u, int k) const {
        graph.check(u); assert(0 <= k && k <= graph.n);
        assert(graph.cycle[u] != -1 || k <= lng(graph.depth[u]) + 1);
        T value = identity; int count = k;
        for (int j = 0; k; ++j, k >>= 1) {
            if (k & 1) { value = op(value, table[j][u]); u = graph.up[j][u]; }}
        return {u, ulng(count), std::move(value)}; }
    FunctionalFoldResult<T> fold(int u, ulng k) const {
        graph.check(u);
        if (graph.cycle[u] == -1) { return segment(u, int(min(k, ulng(graph.depth[u]) + 1))); }
        ulng count = k; int tail = int(min(k, ulng(graph.depth[u])));
        auto out = segment(u, tail); u = out.next; k -= tail;
        if (k) {
            int len = int(graph.cycles[graph.cycle[u]].size());
            ulng times = k / len;
            if (times) {
                T power = segment(u, len).value;
                while (times) {
                    if (times & 1) { out.value = op(out.value, power); }
                    times >>= 1; if (times) { power = op(power, power); }} }
            auto last = segment(u, int(k % len));
            out.next = last.next; out.value = op(out.value, last.value); }
        out.count = count; return out; }
    // O(log(n+1)) operations; rotated full cycle, excluding the tail.
    pair<bool, T> cycleAggregate(int u) const {
        graph.check(u); int c = graph.cycle[u];
        if (c == -1) { return {false, identity}; }
        return {true, segment(graph.entry[u], int(graph.cycles[c].size())).value}; }
};
