#pragma once

// GNU C++20 / GCC 14+, Linux and Windows (MinGW) x86-64. No global ISA/FP flags or I/O setup.
// Width aliases are exact; INF32/INF64 are finite sentinels, not overflow guards.
// indexed_set/indexed_map store unique keys; use (key, id) pairs for repeated values.
// chmin/chmax compare once, assign only on improvement, return whether assigned;
// T supplies ordinary comparison/copy-assignment semantics (NaN never improves).
// Each helper is O(1) comparisons/assignments, plus the costs of T.
// 知彼知己，百战不殆
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi    first
#define se    second
#define pb    push_back
using uint = uint32_t;
using lng = int64_t;    using ulng = uint64_t;
using lll = __int128_t; using ulll = __uint128_t;
template<typename T>
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;
template<typename K, typename V>
using indexed_map = tree<K, V, std::less<K>, rb_tree_tag, tree_order_statistics_node_update>;

constexpr int INF32 = 0x3f3f3f3f;
constexpr lng INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T>
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T>
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }
