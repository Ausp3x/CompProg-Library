#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"

namespace subgraph_enumeration_detail {
    // Dense triangle counting runs when DENSE_RATIO * bitset words < sparse probes and the rows fit in DENSE_WORDS words.
    inline constexpr lng DENSE_RATIO = 2;
    inline constexpr lng DENSE_WORDS = lng(1) << 22;

    // S: O(n + m), M: O(n + m); simple graph relabelled by a degeneracy order (d the degeneracy), lists sorted by rank, out(u) = [split[u], start[u + 1]) has at most d entries; full adds arc, pid, mult, pair_of, dir.
    struct Ranked {
        int n;
        vector<int> vertex, start, split, to, arc, pid, mult, pair_of;
        vector<uint8_t> dir;

        template<class G>
        explicit Ranked(const G &g, bool full = false) : n(g.n), vertex(n), start(size_t(n) + 1) {
            assert(!g.directed || g.arcs.size() <= size_t(INT_MAX / 2));
            vector<int> off(size_t(n) + 1), seen(n, -1), deg(n), pos(n), bin(size_t(n) + 1), where(full ? n : 0), ka, kp;
            vector<uint8_t> kd;
            for (const auto &a : g.arcs) {
                if (a.from != a.to) { ++off[a.from + 1]; off[a.to + 1] += g.directed; }}
            for (int u = 0; u < n; ++u) { off[u + 1] += off[u]; }
            vector<int> raw(off[n]), code(full ? off[n] : 0), cur(off.begin(), off.end() - 1);
            // code is the arc listed; dir bit 1 is owner -> neighbour, bit 2 neighbour -> owner.
            for (int a = 0; a < int(g.arcs.size()); ++a) {
                int x = g.arcs[a].from, y = g.arcs[a].to;
                if (x == y) { continue; }
                if (full) { code[cur[x]] = a; }
                raw[cur[x]++] = y;
                if (g.directed) {
                    if (full) { code[cur[y]] = a; }
                    raw[cur[y]++] = x;}}
            if (full) { pair_of.assign(g.edges.size(), -1); ka.reserve(off[n]); kp.reserve(off[n]); kd.reserve(off[n]); }
            int t = 0;
            for (int u = 0; u < n; ++u) {
                int first = t;
                for (int k = off[u]; k < off[u + 1]; ++k) {
                    int v = raw[k], id = full ? g.arcs[code[k]].id : 0;
                    if (seen[v] != u) {
                        seen[v] = u; raw[t++] = v;
                        if (full) {
                            where[v] = int(ka.size()); ka.push_back(code[k]); kd.push_back(0);
                            kp.push_back(v > u ? int(mult.size()) : pair_of[id]);
                            if (v > u) { mult.push_back(0); }}}
                    if (!full) { continue; }
                    kd[where[v]] |= uint8_t(1 + (g.arcs[code[k]].from != u));
                    if (v > u) { pair_of[id] = kp[where[v]]; ++mult[kp[where[v]]]; }}
                deg[u] = t - first; off[u] = first;}
            off[n] = t;
            // Batagelj-Zaversnik core order: deg ends as the core number, an upper bound on the later neighbours.
            for (int u = 0; u < n; ++u) { ++bin[deg[u]]; }
            for (int d = 0, s = 0; d <= n; ++d) { s += std::exchange(bin[d], s); }
            for (int u = 0; u < n; ++u) { pos[u] = bin[deg[u]]++; vertex[pos[u]] = u; }
            for (int d = n; d > 0; --d) { bin[d] = bin[d - 1]; }
            if (n) { bin[0] = 0; }
            for (int i = 0; i < n; ++i) {
                int v = vertex[i];
                for (int k = off[v]; k < off[v + 1]; ++k) {
                    int u = raw[k];
                    if (deg[u] <= deg[v]) { continue; }
                    int pw = bin[deg[u]], w = vertex[pw];
                    if (u != w) { swap(vertex[pos[u]], vertex[pw]); swap(pos[u], pos[w]); }
                    ++bin[deg[u]]; --deg[u];}}
            for (int r = 0; r < n; ++r) { start[r + 1] = start[r] + off[vertex[r] + 1] - off[vertex[r]]; }
            to.resize(start[n]);
            if (full) { arc.resize(start[n]); pid.resize(start[n]); dir.resize(start[n]); }
            vector<int> fill(start.begin(), start.end() - 1);
            split = fill;
            for (int r = 0; r < n; ++r) {
                for (int k = off[vertex[r]]; k < off[vertex[r] + 1]; ++k) {
                    int s = pos[raw[k]], p = fill[s]++;
                    to[p] = r;
                    if (r < s) { ++split[s]; }
                    if (full) { arc[p] = ka[k]; pid[p] = kp[k]; dir[p] = uint8_t((kd[k] & 1) << 1 | kd[k] >> 1); }}}}
    };

    // T: O(n + m * d), M: O(n), d the degeneracy; f(u, v, w, i, j, k) on ranks u < v < w of every triangle with u < end, i, j, k the positions of v in u, w in v, w in u.
    template<class F>
    void triangles(const Ranked &h, F f, int end = INT_MAX) {
        vector<int> mark(h.n, -1);
        for (int u = 0; u < min(h.n, end); ++u) {
            for (int i = h.split[u]; i < h.start[u + 1]; ++i) { mark[h.to[i]] = i; }
            for (int i = h.split[u]; i < h.start[u + 1]; ++i) {
                int v = h.to[i];
                for (int j = h.split[v]; j < h.start[v + 1]; ++j) {
                    if (int k = mark[h.to[j]]; k >= h.split[u]) { f(u, v, h.to[j], i, j, k); }}}}}

    // T: O(k^2 / w + m * k / w), M: O(k^2 / w), k = n - r0; triangles whose lowest rank is at least r0, by popcounts of out-neighbour rows.
    inline lng bitsetTriangles(const Ranked &h, int r0 = 0) {
        lng words = (h.n - r0 + 63) / 64, res = 0;
        vector<ulng> row(size_t((h.n - r0) * words));
        for (int u = r0; u < h.n; ++u) {
            for (int i = h.split[u]; i < h.start[u + 1]; ++i) {
                int x = h.to[i] - r0;
                row[(u - r0) * words + x / 64] |= ulng(1) << (x % 64);}}
        for (int u = r0; u < h.n; ++u) {
            for (int i = h.split[u]; i < h.start[u + 1]; ++i) {
                int x = h.to[i] - r0;
                const ulng *a = &row[(u - r0) * words], *b = &row[x * words];
                for (lng k = x / 64; k < words; ++k) { res += std::popcount(a[k] & b[k]); }}}
        return res;}

    // T: O(n + m), M: O(1); first rank r0 of the bitset suffix minimizing the probes below it plus DENSE_RATIO * its words within DENSE_WORDS, n if none pays.
    inline int denseStart(const Ranked &h) {
        int res = h.n;
        lng best = 0, probes = 0, scan = 0;
        for (int r = h.n - 1; r >= 0; --r) {
            lng k = h.n - r, rows = k * ((k + 63) / 64);
            if (rows > DENSE_WORDS) { break; }
            for (int i = h.split[r]; i < h.start[r + 1]; ++i) {
                probes += h.start[h.to[i] + 1] - h.split[h.to[i]]; scan += (h.n - h.to[i]) / 64 + 1;}
            if (lng c = DENSE_RATIO * (scan + rows) - probes; c < best) { best = c; res = r; }}
        return res;}

    // T: O(n + m * d), M: O(n) (bitset workspace at most DENSE_WORDS words); simple-graph triangle count, sparse below denseStart and bitset from it.
    inline lng triangleCount(const Ranked &h) {
        int r0 = denseStart(h);
        lng res = r0 < h.n ? bitsetTriangles(h, r0) : 0;
        triangles(h, [&](int, int, int, int, int, int) { ++res; }, r0);
        return res;}

    // T: O(sum of deg(v) over neighbours v < u, deg the simple degree), M: O(1); f(i, j) on each wedge u - to[i] - to[j] with both ranks below u until f returns false.
    template<class F>
    bool wedges(const Ranked &h, int u, F f) {
        for (int i = h.start[u]; i < h.split[u]; ++i) {
            for (int j = h.start[h.to[i]]; j < h.start[h.to[i] + 1] && h.to[j] < u; ++j) {
                if (!f(i, j)) { return false; }}}
        return true;}

    // T: O(n + m * d), M: O(n); per-rank triangle and 4-cycle counts of the simple graph, d the degeneracy.
    inline pair<vector<lng>, vector<lng>> c3c4(const Ranked &h) {
        vector<lng> c3(h.n), c4(h.n), cnt(h.n);
        vector<int> seen;
        triangles(h, [&](int u, int v, int w, int, int, int) { ++c3[u]; ++c3[v]; ++c3[w]; });
        for (int u = 0; u < h.n; ++u) {
            wedges(h, u, [&](int, int j) {
                if (!cnt[h.to[j]]++) { seen.push_back(h.to[j]); }
                return true;});
            wedges(h, u, [&](int i, int j) { c4[h.to[i]] += cnt[h.to[j]] - 1; return true; });
            for (int w : seen) {
                lng p = cnt[w] * (cnt[w] - 1) / 2;
                c4[u] += p; c4[w] += p; cnt[w] = 0;}
            seen.clear();}
        return {std::move(c3), std::move(c4)};}

    // T: O(n + m * d + m * d^2 / w), M: O(n + d^2 / w), d the degeneracy; f(u, a, b, t, x) for each edge a < b inside out(u) (positions split[u] + a, split[u] + b) and each nonzero word x = t of their common later neighbours.
    template<class F>
    void k4(const Ranked &h, F f) {
        vector<int> mark(h.n, -1);
        vector<ulng> row;
        for (int u = 0; u < h.n; ++u) {
            int s = h.split[u], k = h.start[u + 1] - s, words = (k + 63) / 64;
            for (int i = s; i < s + k; ++i) { mark[h.to[i]] = i; }
            row.assign(size_t(k) * words, 0);
            for (int i = s; i < s + k; ++i) {
                for (int j = h.split[h.to[i]]; j < h.start[h.to[i] + 1]; ++j) {
                    if (int b = mark[h.to[j]] - s; b >= 0) { row[size_t(i - s) * words + b / 64] |= ulng(1) << (b % 64); }}}
            for (int a = 0; a < k; ++a) {
                const ulng *ra = &row[size_t(a) * words];
                for (int t = 0; t < words; ++t) {
                    for (ulng bits = ra[t]; bits; bits &= bits - 1) {
                        int b = 64 * t + std::countr_zero(bits);
                        const ulng *rb = &row[size_t(b) * words];
                        for (int q = b / 64; q < words; ++q) {
                            if (ulng x = ra[q] & rb[q]) { f(u, a, b, q, x); }}}}}}}

    // T: O(m), M: O(m); per pair of a full Ranked, the sum of cbrt(w / max w) over its arcs (max over every edge, loops included, 1 without edges).
    template<class G>
    vector<double> cubeRoots(const G &g, const Ranked &h) {
        lng top = g.edges.empty() ? 1 : std::ranges::max(g.edges, {}, &GraphEdge::w).w;
        assert(top != 0);
        vector<double> res(h.mult.size());
        vector<uint8_t> seen(h.mult.size());
        for (int e = 0; e < int(g.edges.size()); ++e) {
            if (int p = h.pair_of[e]; p >= 0) {
                uint8_t bit = uint8_t(!g.directed || g.edges[e].u < g.edges[e].v ? 1 : 2);
                assert(!(seen[p] & bit));
                seen[p] |= bit; res[p] += std::cbrt(double(g.edges[e].w) / double(top));}}
        return res;}

} // namespace subgraph_enumeration_detail

// T: O(n + m * d + out), M: O(n + m), d the degeneracy (d <= sqrt(2 * m)); undirected, loops and parallel edges ignored; f(a, b, c) once per triangle.
template<class G, class F>
void enumerateTriangles(const G &g, F f) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    subgraph_enumeration_detail::triangles(h, [&](int u, int v, int w, int, int, int) { f(h.vertex[u], h.vertex[v], h.vertex[w]); });}

// T: O(n + m * d + out), M: O(n + m), d the degeneracy; undirected multigraph, f(a, b, c, x, y, z) once per triangle of edges x = ab, y = bc, z = ca.
template<class G, class F>
void enumerateTriangleEdges(const G &g, F f) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<int> off(h.mult.size() + 1), ids(g.edges.size());
    for (int p : h.pair_of) {
        if (p >= 0) { ++off[p + 1]; }}
    for (int p = 0; p < int(h.mult.size()); ++p) { off[p + 1] += off[p]; }
    vector<int> cur(off.begin(), off.end() - 1);
    for (int e = 0; e < int(g.edges.size()); ++e) {
        if (h.pair_of[e] >= 0) { ids[cur[h.pair_of[e]]++] = e; }}
    subgraph_enumeration_detail::triangles(h, [&](int u, int v, int w, int i, int j, int k) {
        for (int x = off[h.pid[i]]; x < off[h.pid[i] + 1]; ++x) {
            for (int y = off[h.pid[j]]; y < off[h.pid[j] + 1]; ++y) {
                for (int z = off[h.pid[k]]; z < off[h.pid[k] + 1]; ++z) { f(h.vertex[u], h.vertex[v], h.vertex[w], ids[x], ids[y], ids[z]); }}}});}

// T: O(n + m * d), M: O(n + m) (bitset workspace at most 2^22 words), d the degeneracy; undirected simple-graph count, bitset over the densest rank suffix when cheaper.
template<class G>
lng countTriangles(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    return subgraph_enumeration_detail::triangleCount(h);}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected simple graph, sum over triangles {a, b, c} of x[a] * x[b] * x[c]; T() is zero, x.size() == n.
template<class G, class T>
T triangleProductSum(const G &g, const vector<T> &x) {
    assert(!g.directed && int(x.size()) == g.n);
    subgraph_enumeration_detail::Ranked h(g);
    vector<T> xr(h.n);
    for (int r = 0; r < h.n; ++r) {
        if (h.vertex[r] < int(x.size())) { xr[r] = x[h.vertex[r]]; }}
    T res{};
    subgraph_enumeration_detail::triangles(h, [&](int u, int v, int w, int, int, int) { res += xr[u] * xr[v] * xr[w]; });
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected multigraph, res[e] = triangles (edge sets) through edge e, 0 for loops.
template<class G>
vector<lng> countTrianglesPerEdge(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<lng> acc(h.mult.size()), res(g.edges.size());
    subgraph_enumeration_detail::triangles(h, [&](int, int, int, int i, int j, int k) {
        lng a = h.mult[h.pid[i]], b = h.mult[h.pid[j]], c = h.mult[h.pid[k]];
        acc[h.pid[i]] += b * c; acc[h.pid[j]] += a * c; acc[h.pid[k]] += a * b;});
    for (int e = 0; e < int(res.size()); ++e) { res[e] = h.pair_of[e] < 0 ? 0 : acc[h.pair_of[e]]; }
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected simple graph, res[v][k] = neighbours u of v whose pair {u, v} lies in exactly k triangles, size 1 + max k (empty if isolated).
template<class G>
vector<vector<int>> generalizedDegree(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<int> tri(h.mult.size());
    vector<vector<int>> res(h.n);
    subgraph_enumeration_detail::triangles(h, [&](int, int, int, int i, int j, int k) { ++tri[h.pid[i]]; ++tri[h.pid[j]]; ++tri[h.pid[k]]; });
    for (int r = 0; r < h.n; ++r) {
        auto &row = res[h.vertex[r]];
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) {
            int k = tri[h.pid[i]];
            if (k >= int(row.size())) { row.resize(size_t(k) + 1); }
            ++row[k];}}
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected simple graph, 4-cycles (vertex sets with a cyclic order), each counted once.
template<class G>
lng countC4(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    vector<int> cnt(h.n), seen;
    lng res = 0;
    for (int u = 0; u < h.n; ++u) {
        subgraph_enumeration_detail::wedges(h, u, [&](int, int j) {
            if (!cnt[h.to[j]]) { seen.push_back(h.to[j]); }
            res += cnt[h.to[j]]++;
            return true;});
        for (int w : seen) { cnt[w] = 0; }
        seen.clear();}
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected multigraph, res[e] = 4-cycles (edge sets) through edge e, 0 for loops; exact for simple graphs and m <= 6 * 10^6, else mod 2^64.
template<class G>
vector<lng> countC4PerEdge(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<ulng> sum(h.n), acc(h.mult.size());
    vector<lng> res(g.edges.size());
    vector<int> seen;
    for (int u = 0; u < h.n; ++u) {
        subgraph_enumeration_detail::wedges(h, u, [&](int i, int j) {
            if (!sum[h.to[j]]) { seen.push_back(h.to[j]); }
            sum[h.to[j]] += ulng(h.mult[h.pid[i]]) * ulng(h.mult[h.pid[j]]);
            return true;});
        subgraph_enumeration_detail::wedges(h, u, [&](int i, int j) {
            ulng a = ulng(h.mult[h.pid[i]]), b = ulng(h.mult[h.pid[j]]), other = sum[h.to[j]] - a * b;
            acc[h.pid[i]] += b * other; acc[h.pid[j]] += a * other;
            return true;});
        for (int w : seen) { sum[w] = 0; }
        seen.clear();}
    for (int e = 0; e < int(res.size()); ++e) { res[e] = h.pair_of[e] < 0 ? 0 : lng(acc[h.pair_of[e]]); }
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected simple graph, {triangles, 4-cycles} containing each vertex.
template<class G>
pair<vector<lng>, vector<lng>> countC3C4PerVertex(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    auto [c3, c4] = subgraph_enumeration_detail::c3c4(h);
    pair<vector<lng>, vector<lng>> res{vector<lng>(h.n), vector<lng>(h.n)};
    for (int r = 0; r < h.n; ++r) { res.first[h.vertex[r]] = c3[r]; res.second[h.vertex[r]] = c4[r]; }
    return res;}

// T: O(n + m + min(n^2, m * d)), M: O(n + m), d the degeneracy; undirected, closed walk over 4 distinct vertices back to the first with arc IDs of g, empty when the simple graph has no 4-cycle.
template<class G>
CycleWitness findC4(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<int> first(h.n, -1), via(h.n), seen;
    CycleWitness res;
    for (int u = 0; u < h.n && res.arcs.empty(); ++u) {
        subgraph_enumeration_detail::wedges(h, u, [&](int i, int j) {
            int w = h.to[j];
            if (first[w] == -1) { first[w] = i; via[w] = j; seen.push_back(w); return true; }
            // Entry p of list x holds an arc from to[p] into x.
            res.vertices = {h.vertex[u], h.vertex[h.to[i]], h.vertex[w], h.vertex[h.to[first[w]]], h.vertex[u]};
            res.arcs = {g.arcs[h.arc[i]].rev, g.arcs[h.arc[j]].rev, h.arc[via[w]], h.arc[first[w]]};
            return false;});
        for (int w : seen) { first[w] = -1; }
        seen.clear();}
    return res;}

// T: O(n + m * d + out), M: O(n + m), d the degeneracy; undirected simple graph, f(a, b, c, e) once per 4-cycle a-b-c-e-a.
template<class G, class F>
void enumerateC4(const G &g, F f) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    vector<int> cnt(h.n), at(h.n), mid, seen;
    for (int u = 0; u < h.n; ++u) {
        subgraph_enumeration_detail::wedges(h, u, [&](int, int j) {
            if (!cnt[h.to[j]]++) { seen.push_back(h.to[j]); }
            return true;});
        int total = 0;
        for (int w : seen) { at[w] = total; total += cnt[w]; }
        mid.resize(total);
        subgraph_enumeration_detail::wedges(h, u, [&](int i, int j) { mid[at[h.to[j]]++] = h.to[i]; return true; });
        for (int w : seen) {
            for (int p = at[w] - cnt[w]; p < at[w]; ++p) {
                for (int q = at[w] - cnt[w]; q < p; ++q) { f(h.vertex[u], h.vertex[mid[q]], h.vertex[w], h.vertex[mid[p]]); }}
            cnt[w] = 0;}
        seen.clear();}}

// T: O(n + m * d + m * d^2 / w), M: O(n + m), d the degeneracy; undirected simple graph, 4-cliques.
template<class G>
lng countK4(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    lng res = 0;
    subgraph_enumeration_detail::k4(h, [&](int, int, int, int, ulng x) { res += std::popcount(x); });
    return res;}

// T: O(n + m * d + m * d^2 / w + out), M: O(n + m), d the degeneracy; undirected simple graph, f(a, b, c, e) once per 4-clique.
template<class G, class F>
void enumerateK4(const G &g, F f) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    subgraph_enumeration_detail::k4(h, [&](int u, int a, int b, int t, ulng x) {
        int s = h.split[u];
        for (; x; x &= x - 1) { f(h.vertex[u], h.vertex[h.to[s + a]], h.vertex[h.to[s + b]], h.vertex[h.to[s + 64 * t + std::countr_zero(x)]]); }});}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected simple graph, res[k][v] = simple paths with k + 3 vertices starting at v.
template<class G>
array<vector<lng>, 3> countPathsP3P4P5(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    auto [c3, c4] = subgraph_enumeration_detail::c3c4(h);
    vector<lng> deg(h.n), p3(h.n), p4(h.n), p5(h.n);
    for (int r = 0; r < h.n; ++r) { deg[r] = h.start[r + 1] - h.start[r]; }
    for (int r = 0; r < h.n; ++r) {
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) { p3[r] += deg[h.to[i]] - 1; }}
    for (int r = 0; r < h.n; ++r) {
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) { p4[r] += p3[h.to[i]]; }
        p4[r] -= deg[r] * (deg[r] - 1) + 2 * c3[r];}
    for (int r = 0; r < h.n; ++r) {
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) { p5[r] += p4[h.to[i]]; }
        p5[r] -= (deg[r] - 1) * p3[r] + 2 * c3[r] * (deg[r] - 3) + 2 * c4[r];}
    array<vector<lng>, 3> res;
    res.fill(vector<lng>(h.n));
    for (int r = 0; r < h.n; ++r) { res[0][h.vertex[r]] = p3[r]; res[1][h.vertex[r]] = p4[r]; res[2][h.vertex[r]] = p5[r]; }
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected simple graph, 2 t(v) / (deg(v) (deg(v) - 1)) with t(v) the triangles at v, 0 when that is 0 / 0.
template<class G>
vector<double> localClustering(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    vector<lng> t(h.n);
    vector<double> res(h.n);
    subgraph_enumeration_detail::triangles(h, [&](int u, int v, int w, int, int, int) { ++t[u]; ++t[v]; ++t[w]; });
    for (int r = 0; r < h.n; ++r) {
        lng d = h.start[r + 1] - h.start[r];
        if (t[r]) { res[h.vertex[r]] = double(2 * t[r]) / double(d * (d - 1)); }}
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected, no parallel edges, max weight != 0; NetworkX weighted clustering 2 * sum over triangles at v of cbrt(product of w / max w) / (deg(v) (deg(v) - 1)).
template<class G>
vector<double> weightedClustering(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<double> c = subgraph_enumeration_detail::cubeRoots(g, h), t(h.n), res(h.n);
    subgraph_enumeration_detail::triangles(h, [&](int u, int v, int w, int i, int j, int k) {
        double p = 2 * c[h.pid[i]] * c[h.pid[j]] * c[h.pid[k]];
        t[u] += p; t[v] += p; t[w] += p;});
    for (int r = 0; r < h.n; ++r) {
        lng d = h.start[r + 1] - h.start[r];
        if (t[r] != 0) { res[h.vertex[r]] = t[r] / double(d * (d - 1)); }}
    return res;}

// T: O(n + m * d), M: O(n + m) (bitset workspace at most 2^22 words), d the degeneracy; undirected simple graph, 3 * triangles / 2-paths, 0 without triangles.
template<class G>
double transitivity(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    lng t = subgraph_enumeration_detail::triangleCount(h), paths = 0;
    for (int r = 0; r < h.n; ++r) { paths += lng(h.start[r + 1] - h.start[r]) * (h.start[r + 1] - h.start[r] - 1) / 2; }
    return t ? double(3 * t) / double(paths) : 0;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; undirected simple graph, NetworkX square clustering q(v) / ((deg(v) - 1) P3(v) - 2 t(v) - q(v)), 0 when the denominator is not positive.
template<class G>
vector<double> squareClustering(const G &g) {
    assert(!g.directed);
    subgraph_enumeration_detail::Ranked h(g);
    auto [c3, c4] = subgraph_enumeration_detail::c3c4(h);
    vector<double> res(h.n);
    for (int r = 0; r < h.n; ++r) {
        lng d = h.start[r + 1] - h.start[r], p3 = 0;
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) { p3 += h.start[h.to[i] + 1] - h.start[h.to[i]] - 1; }
        if (lng potential = (d - 1) * p3 - 2 * c3[r] - c4[r]; potential > 0) { res[h.vertex[r]] = double(c4[r]) / double(potential); }}
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; directed, loops and parallel arcs ignored, C(n, 3) < 2^63; counts in NetworkX order 003, 012, 102, 021D, 021U, 021C, 111D, 111U, 030T, 030C, 201, 120D, 120U, 120C, 210, 300.
template<class G>
array<lng, 16> triadicCensus(const G &g) {
    assert(g.directed && lll(g.n) * (g.n - 1) * (g.n - 2) / 6 <= LLONG_MAX);
    subgraph_enumeration_detail::Ranked h(g, true);
    array<lng, 16> res{};
    // Dyad codes seen from the centre: 1 out, 2 in, 3 mutual; WEDGE[x][y] is the open-wedge type.
    static constexpr int WEDGE[4][4] = {{}, {0, 3, 5, 7}, {0, 5, 4, 6}, {0, 7, 6, 10}};
    auto flip = [](int x) { return (x & 1) << 1 | x >> 1; };
    for (int r = 0; r < h.n; ++r) {
        lng c[4] = {};
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) { ++c[h.dir[i]]; }
        res[3] += c[1] * (c[1] - 1) / 2; res[4] += c[2] * (c[2] - 1) / 2; res[5] += c[1] * c[2];
        res[6] += c[3] * c[2]; res[7] += c[3] * c[1]; res[10] += c[3] * (c[3] - 1) / 2;
        for (int i = h.split[r]; i < h.start[r + 1]; ++i) {
            res[h.dir[i] == 3 ? 2 : 1] += h.n - (h.start[r + 1] - h.start[r]) - (h.start[h.to[i] + 1] - h.start[h.to[i]]);}}
    subgraph_enumeration_detail::triangles(h, [&](int, int, int, int i, int j, int k) {
        int uv = h.dir[i], vw = h.dir[j], uw = h.dir[k], mutual = (uv == 3) + (vw == 3) + (uw == 3);
        for (int x : {uv, vw, uw}) { ++res[x == 3 ? 2 : 1]; }
        --res[WEDGE[uv][uw]]; --res[WEDGE[flip(uv)][vw]]; --res[WEDGE[flip(uw)][flip(vw)]];
        int ou = (uv & 1) + (uw & 1), ov = (uv >> 1) + (vw & 1), ow = (vw >> 1) + (uw >> 1);
        if (mutual == 0) { ++res[ou == 1 && ov == 1 ? 9 : 8]; }
        else if (mutual == 1) {
            int o = uv == 3 ? ow : vw == 3 ? ou : ov;
            ++res[o == 2 ? 11 : o == 0 ? 12 : 13];}
        else { ++res[12 + mutual]; }});
    res[0] = lng(lll(h.n) * (h.n - 1) * (h.n - 2) / 6);
    for (int t = 1; t < 16; ++t) { res[0] -= res[t]; }
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy of the underlying graph; directed, loops and parallel arcs ignored, NetworkX directed clustering T(v) / (2 (dt (dt - 1) - 2 db)), 0 without directed triangles.
template<class G>
vector<double> directedClustering(const G &g) {
    assert(g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<lng> t(h.n);
    vector<double> res(h.n);
    subgraph_enumeration_detail::triangles(h, [&](int u, int v, int w, int i, int j, int k) {
        lng p = 2 * std::popcount(uint(h.dir[i])) * std::popcount(uint(h.dir[j])) * std::popcount(uint(h.dir[k]));
        t[u] += p; t[v] += p; t[w] += p;});
    for (int r = 0; r < h.n; ++r) {
        lng dt = 0, db = 0;
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) { dt += std::popcount(uint(h.dir[i])); db += h.dir[i] == 3; }
        if (t[r]) { res[h.vertex[r]] = double(t[r]) / double(2 * (dt * (dt - 1) - 2 * db)); }}
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy of the underlying graph; directed, no parallel arcs, max weight != 0; NetworkX weighted directed clustering, cube roots of w / max w in place of arc counts.
template<class G>
vector<double> weightedDirectedClustering(const G &g) {
    assert(g.directed);
    subgraph_enumeration_detail::Ranked h(g, true);
    vector<double> c = subgraph_enumeration_detail::cubeRoots(g, h), t(h.n), res(h.n);
    subgraph_enumeration_detail::triangles(h, [&](int u, int v, int w, int i, int j, int k) {
        double p = 2 * c[h.pid[i]] * c[h.pid[j]] * c[h.pid[k]];
        t[u] += p; t[v] += p; t[w] += p;});
    for (int r = 0; r < h.n; ++r) {
        lng dt = 0, db = 0;
        for (int i = h.start[r]; i < h.start[r + 1]; ++i) { dt += std::popcount(uint(h.dir[i])); db += h.dir[i] == 3; }
        if (t[r] != 0) { res[h.vertex[r]] = t[r] / double(2 * (dt * (dt - 1) - 2 * db)); }}
    return res;}

// T: O(n + m * d), M: O(n + m), d the degeneracy; mean of localClustering (directedClustering when directed) over all vertices, or over the nonzero ones if !count_zeros; 0 when nothing is averaged.
template<class G>
double averageClustering(const G &g, bool count_zeros = true) {
    vector<double> c = g.directed ? directedClustering(g) : localClustering(g);
    double sum = 0;
    int k = 0;
    for (double x : c) {
        if (count_zeros || x != 0) { sum += x; ++k; }}
    return k ? sum / k : 0;}
