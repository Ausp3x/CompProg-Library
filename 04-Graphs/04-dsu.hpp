#pragma once

#include "01-graph.hpp"
#include "../02-Data Structures/01-dsu.hpp"

// Canonical DSU is re-exported by inclusion, with no duplicate implementation.
// Undirected components, or weak components when g is directed. Loops allowed.
// T: O(n + m * alpha(n)) amortized, M: O(n) including returned DSU.
template<class G>
DSU graphComponents(const G &g) {
    DSU res(g.n);
    for (auto e : g.edges) { res.uniteSets(e.u, e.v); }
    return res; }
