#pragma once
#include "../01-Core/01-template.hpp"

// T: O(1), M: O(out); closed implication walk x -> ... -> ~x -> ... -> x, clause clauses[i] yields literals[i] -> literals[i + 1].
struct TwoSatWitness { vector<int> literals, clauses; };

// S: O(n), U: O(1) amortized, Q: O(n + m) satisfiable/unsatWitness, M: O(n + m); m clauses.
// Literal x >= 0 is "x true", ~x is "x false"; ans and answer() hold the last satisfiable() result, empty if unsatisfiable.
struct TwoSat {
    int n;
    vector<pair<int, int>> clauses;
    vector<int> start, comp;
    vector<pair<int, int>> adj;
    vector<bool> ans;

    explicit TwoSat(int N = 0) : n(N) { assert(n >= 0); }

    static int node(int x) { return x >= 0 ? 2 * x : 2 * ~x + 1; }
    static int literal(int v) { return v & 1 ? ~(v >> 1) : v >> 1; }

    int addVar() { return n++; }
    void addClause(int a, int b) {
        bool ok = -n <= a && a < n && -n <= b && b < n && clauses.size() < size_t(INT_MAX / 2);
        assert(ok);
        if (ok) { clauses.emplace_back(a, b); }}
    void addClause(int a, bool na, int b, bool nb) { addClause(na ? ~a : a, nb ? ~b : b); }
    void setValue(int a) { addClause(a, a); }
    void addImplication(int a, int b) { addClause(~a, b); }
    void addXor(int a, int b) { addClause(a, b); addClause(~a, ~b); }
    void addEquivalent(int a, int b) { addXor(a, ~b); }
    // T: O(k), M: O(k) clauses; adds k - 2 auxiliary variables (prefix "some earlier literal is true").
    void addAtMostOne(const vector<int> &lits) {
        int k = int(lits.size());
        if (k <= 1) { return; }
        int prev = lits[0];
        for (int i = 1; i < k; ++i) {
            addClause(~prev, ~lits[i]);
            if (i + 1 < k) { int p = addVar(); addImplication(lits[i], p); addImplication(prev, p); prev = p; }}}

    bool satisfiable() {
        int nn = 2 * n, m = int(clauses.size());
        start.assign(nn + 1, 0); adj.resize(2 * size_t(m));
        for (auto [a, b] : clauses) { ++start[node(a) ^ 1]; ++start[node(b) ^ 1]; }
        for (int v = 0; v < nn; ++v) { start[v + 1] += start[v]; }
        for (int i = m - 1; i >= 0; --i) {
            auto [a, b] = clauses[i];
            adj[--start[node(b) ^ 1]] = {node(a), 2 * i + 1}; adj[--start[node(a) ^ 1]] = {node(b), 2 * i};}
        vector<int> ord(nn, -1), low(nn), next(start.begin(), start.end() - 1), path, active;
        comp.assign(nn, -1);
        int timer = 0, k = 0;
        for (int s = 0; s < nn; ++s) {
            if (ord[s] != -1) { continue; }
            ord[s] = low[s] = timer++; path.push_back(s); active.push_back(s);
            while (!path.empty()) {
                int u = path.back();
                if (next[u] < start[u + 1]) {
                    int v = adj[next[u]++].first;
                    if (ord[v] == -1) { ord[v] = low[v] = timer++; path.push_back(v); active.push_back(v); }
                    else if (comp[v] == -1) { low[u] = min(low[u], ord[v]); }
                    continue;}
                path.pop_back();
                if (!path.empty()) { low[path.back()] = min(low[path.back()], low[u]); }
                if (low[u] == ord[u]) {
                    while (true) {
                        int v = active.back(); active.pop_back(); comp[v] = k;
                        if (v == u) { break; }}
                    ++k;}}}
        ans.assign(n, false);
        for (int x = 0; x < n; ++x) {
            if (comp[2 * x] == comp[2 * x + 1]) { ans.clear(); return false; }
            ans[x] = comp[2 * x] < comp[2 * x + 1];}
        return true;}
    bool solve() { return satisfiable(); }
    const vector<bool> &answer() const { return ans; }

    // T: O((n + m) * (1 + n / w)), M: O(n + m); asserts satisfiable; literals true in every solution, by variable.
    vector<int> forcedLiterals() {
        bool ok = satisfiable(); assert(ok);
        if (!ok) { return {}; }
        int nn = 2 * n, kc = 0;
        for (int c : comp) { kc = max(kc, c + 1); }
        vector<int> first(kc + 1), nodes(nn), cand;
        for (int c : comp) { ++first[c + 1]; }
        for (int c = 0; c < kc; ++c) { first[c + 1] += first[c]; }
        vector<int> pos(first.begin(), first.end() - 1);
        for (int v = 0; v < nn; ++v) { nodes[pos[comp[v]]++] = v; }
        for (int c = 0; c < kc; ++c) {
            if (c > comp[nodes[first[c]] ^ 1]) { cand.push_back(c); }}
        vector<ulng> mask(kc);
        vector<int8_t> value(n, -1);
        for (int b = 0; b < int(cand.size()); b += 64) {
            int e = min(int(cand.size()), b + 64);
            fill(mask.begin(), mask.end(), 0);
            for (int j = b; j < e; ++j) { mask[cand[j]] |= 1ULL << (j - b); }
            for (int c = cand[e - 1]; c >= 0; --c) {
                if (!mask[c]) { continue; }
                for (int i = first[c]; i < first[c + 1]; ++i) {
                    for (int a = start[nodes[i]]; a < start[nodes[i] + 1]; ++a) { mask[comp[adj[a].first]] |= mask[c]; }}}
            for (int j = b; j < e; ++j) {
                int c = cand[j];
                if (!(mask[comp[nodes[first[c]] ^ 1]] >> (j - b) & 1)) { continue; }
                for (int i = first[c]; i < first[c + 1]; ++i) { value[nodes[i] >> 1] = int8_t(nodes[i] & 1); }}}
        vector<int> res;
        for (int x = 0; x < n; ++x) {
            if (value[x] != -1) { res.push_back(value[x] ? x : ~x); }}
        return res;}

    // T: O(n + m), M: O(n + m); empty when satisfiable.
    TwoSatWitness unsatWitness() {
        if (satisfiable()) { return {}; }
        int x = 0;
        while (comp[2 * x] != comp[2 * x + 1]) { ++x; }
        TwoSatWitness res{{x}, {}};
        vector<int> from(2 * n), arc(2 * n), queue;
        for (int s : {2 * x, 2 * x + 1}) {
            int t = s ^ 1;
            fill(from.begin(), from.end(), -1); from[s] = s; queue.assign(1, s);
            for (int i = 0; from[t] == -1; ++i) {
                int u = queue[i];
                for (int a = start[u]; a < start[u + 1]; ++a) {
                    auto [v, id] = adj[a];
                    if (from[v] == -1) { from[v] = u; arc[v] = id; queue.push_back(v); }}}
            int pos = int(res.literals.size());
            for (int v = t; v != s; v = from[v]) { res.literals.push_back(literal(v)); res.clauses.push_back(arc[v] >> 1); }
            reverse(res.literals.begin() + pos, res.literals.end()); reverse(res.clauses.begin() + pos - 1, res.clauses.end());}
        return res;}
};
