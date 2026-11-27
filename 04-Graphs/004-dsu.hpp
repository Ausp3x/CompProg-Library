#pragma once

#include "001-graph.hpp"
#include "../02-Data Structures/001-dsu.hpp"

// T: O(n + m * alpha(n)) amortized, M: O(n); weak components when directed, alpha = inverse Ackermann.
template<class G = Graph>
DSU graphComponents(const G &g) {
    DSU res(g.n);
    for (auto e : g.edges) { res.uniteSets(e.u, e.v); }
    return res;}
