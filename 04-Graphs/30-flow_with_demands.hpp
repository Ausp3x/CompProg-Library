#pragma once

#include "../01-Core/01-template.hpp"
#include "19-max_flow.hpp"
#include "20-min_cost_flow.hpp"

// T: O(1), M: O(n + m); flow[i] is edge i's flow, value the s-t value, cost the total; infeasibleCut only when !ok, stCut and potentials when ok.
template<class Cap>
struct FlowWithDemandsResult { bool ok = false; Cap value = 0; lll cost = 0; vector<Cap> flow; vector<bool> infeasibleCut, stCut; vector<lll> potentials; };

namespace flow_with_demands_detail {
    // T: O(n + m) plus one Net maximum flow (two when dir != 0) on n + 2 vertices and m + n + 2 arcs, M: O(n + m); s < 0 circulation; dir 1 max, -1 min, 0 any s-t value.
    template<class Net, class Cap, class E>
    FlowWithDemandsResult<Cap> solve(int n, const vector<E> &es, const vector<Cap> &b, int s, int t, int dir) {
        constexpr bool costed = std::tuple_size_v<E> == 5;
        ulll w = 0;
        auto add = [&](Cap x) { w += x < 0 ? -ulll(x) : ulll(x); assert(w <= ulll(std::numeric_limits<Cap>::max())); };
        assert(n >= 0 && (b.empty() || int(b.size()) == n));
        vector<Cap> d(n);
        for (int v = 0; v < min(n, int(b.size())); ++v) { add(b[v]); d[v] = b[v]; }
        assert(accumulate(b.begin(), b.end(), Cap(0)) == 0);
        int m = int(es.size());
        Net net(n + 2);
        FlowWithDemandsResult<Cap> res;
        for (auto &e : es) {
            int u = std::get<0>(e), v = std::get<1>(e);
            Cap lo = std::get<2>(e), hi = std::get<3>(e);
            assert(0 <= u && u < n && 0 <= v && v < n && lo <= hi);
            add(lo); add(hi);
            d[u] -= lo; d[v] += lo;
            if constexpr (costed) { net.addEdge(u, v, hi - lo, std::get<4>(e)); }
            else { net.addEdge(u, v, hi - lo); }}
        if constexpr (!costed) {
            if (s >= 0) { net.addEdge(t, s, Cap(w)); net.addEdge(s, t, Cap(w)); }}
        Cap need = 0;
        for (int v = 0; v < n; ++v) {
            if (d[v] > 0) { net.addEdge(n, v, d[v], 0); need += d[v]; }
            else if (d[v] < 0) { net.addEdge(v, n + 1, -d[v], 0); }}
        auto got = net.flow(n, n + 1);
        if constexpr (costed) { res.ok = got.first == need; }
        else { res.ok = got == need; }
        auto reach = [&](int r) {
            vector<vector<int>> adj(n + 2);
            vector<int> q{r};
            vector<bool> in(n + 2);
            for (auto e : net.edges()) {
                if (e.flow < e.cap) { adj[e.from].push_back(e.to); }
                if (e.flow > 0) { adj[e.to].push_back(e.from); }}
            in[r] = true;
            for (int i = 0; i < int(q.size()); ++i) {
                for (int v : adj[q[i]]) {
                    if (!in[v]) { in[v] = true; q.push_back(v); }}}
            in.resize(n);
            return in;};
        if (!res.ok) { res.infeasibleCut = reach(n); return res; }
        if constexpr (costed) {
            auto p = net.potentials();
            res.potentials.assign(p.begin(), p.begin() + n);}
        else if (s >= 0) {
            res.value = net.getEdge(m).flow - net.getEdge(m + 1).flow;
            net.changeEdge(m, 0, 0); net.changeEdge(m + 1, 0, 0);
            if (dir) {
                res.value += dir > 0 ? net.flow(s, t) : -net.flow(t, s);
                res.stCut = reach(dir > 0 ? s : t);
                if (dir < 0) { res.stCut.flip(); }}}
        for (int i = 0; i < m; ++i) { res.flow.push_back(std::get<2>(es[i]) + net.getEdge(i).flow); }
        return res;}
} // namespace flow_with_demands_detail

// T: O(n^2 * (n + m)), M: O(n + m); edges (u, v, lo, hi) with lo <= hi; supplies b[v] = outflow - inflow summing to 0, empty for none.
template<class Cap>
FlowWithDemandsResult<Cap> feasibleCirculation(int n, const vector<tuple<int, int, Cap, Cap>> &es, const vector<Cap> &b = {}) {
    return flow_with_demands_detail::solve<MaxFlow<Cap>>(n, es, b, -1, -1, 0);}
// T: O(n^2 * (n + m)), M: O(n + m); as feasibleCirculation, except that s and t absorb a value (s's net outflow beyond b[s]) of any sign.
template<class Cap>
FlowWithDemandsResult<Cap> feasibleFlow(int n, const vector<tuple<int, int, Cap, Cap>> &es, int s, int t, const vector<Cap> &b = {}) {
    assert(0 <= s && s < n && 0 <= t && t < n && s != t);
    return flow_with_demands_detail::solve<MaxFlow<Cap>>(n, es, b, s, t, 0);}
// T: O(n^2 * (n + m)), M: O(n + m); as feasibleFlow with the maximum value; stCut is the residual side of s.
template<class Cap>
FlowWithDemandsResult<Cap> maxFlowWithLowerBounds(int n, const vector<tuple<int, int, Cap, Cap>> &es, int s, int t, const vector<Cap> &b = {}) {
    assert(0 <= s && s < n && 0 <= t && t < n && s != t);
    return flow_with_demands_detail::solve<MaxFlow<Cap>>(n, es, b, s, t, 1);}
// T: O(n^2 * (n + m)), M: O(n + m); as feasibleFlow with the minimum value, possibly negative; stCut excludes the residual side of t.
template<class Cap>
FlowWithDemandsResult<Cap> minFlowWithLowerBounds(int n, const vector<tuple<int, int, Cap, Cap>> &es, int s, int t, const vector<Cap> &b = {}) {
    assert(0 <= s && s < n && 0 <= t && t < n && s != t);
    return flow_with_demands_detail::solve<MaxFlow<Cap>>(n, es, b, s, t, -1);}

// T: O(n + m) plus one Backend min-cost max flow on n + 2 vertices, m + n arcs, costs >= 0; M: O(n + m) plus the backend's; edges (u, v, lo, hi, cost).
template<class Cap, class Cost, class Backend = MinCostFlow<Cap, Cost>>
FlowWithDemandsResult<Cap> minCostCirculationAdapter(int n, const vector<tuple<int, int, Cap, Cap, Cost>> &es, const vector<Cap> &b = {}) {
    auto rev = es;
    auto mag = [](auto x) { return x < 0 ? -ulll(x) : ulll(x); };
    ulll top = 0, p, sum = 0;
    bool over = false;
    // A negative-cost edge runs reversed with negated bounds and cost, so the backend sees costs >= 0 only.
    for (auto &[u, v, lo, hi, c] : rev) {
        assert(c > std::numeric_limits<Cost>::min() && lo > std::numeric_limits<Cap>::min());
        over |= __builtin_mul_overflow(mag(c), max(mag(lo), mag(hi)), &p) || __builtin_add_overflow(top, p, &top);
        if (c < 0) { std::tie(u, v, lo, hi, c) = tuple{v, u, -hi, -lo, -c}; }}
    assert(!over && top <= ulll(std::numeric_limits<lll>::max()));
    auto res = flow_with_demands_detail::solve<Backend>(n, rev, b, -1, -1, 0);
    for (int i = 0; i < int(res.flow.size()); ++i) {
        Cost c = std::get<4>(es[i]);
        if (c < 0) { res.flow[i] = -res.flow[i]; }
        sum += ulll(c) * ulll(res.flow[i]);}
    res.cost = lll(sum);
    return res;}
