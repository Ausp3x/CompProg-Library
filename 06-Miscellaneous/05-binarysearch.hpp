#pragma once

// Canonical engine and full contracts: Mathematics/02-search_algorithms.hpp.
// binSearch uses known true/false lng endpoints (either order, overflow-safe).
// firstTrue/lastTrue use [l,r) and return {found,position}; absent is {false,r}.
// binSearchRealBracket reports finite bracket, feasible endpoint and convergence.
// Integer T: O(log(n + 1)), M: O(1); real T: O(iterations), M: O(1).
// This inventory-mandated topic facade intentionally reuses the Mathematics API.
#include "../05-Mathematics/02-search_algorithms.hpp"
