#pragma once

#include "01-graph.hpp"
#include "../02-Data Structures/01-dsu.hpp"

// T: O(n + m * alpha(n)) amortized, M: O(n); weak components when directed, alpha = inverse Ackermann.
template<class G>
DSU graphComponents(const G &g) {
    DSU res(g.n);
    for (auto e : g.edges) { res.uniteSets(e.u, e.v); }
    return res;}
