#include "../../06-Miscellaneous/05-binarysearch.hpp"

// Compile these BEFORE including the shared suite: an empty/broken facade fails.
static_assert(std::is_same_v<decltype(firstTrue(0, 1, [](lng x) { return x == 0; })), pair<bool, lng>>);
static_assert(std::is_same_v<decltype(lastTrue(0, 1, [](lng x) { return x == 0; })), pair<bool, lng>>);
static_assert(std::is_same_v<decltype(binSearch(0, 1, [](lng x) { return x == 0; })), lng>);
static_assert(std::is_same_v<decltype(binSearchRealBracket(0, 1, [](double x) { return x < .5; })), RealSearchResult>);

// Reuse the canonical exhaustive/analytic suite, including every exposed API.
#include "../05-Mathematics/02-search_algorithms_tester.cpp"
