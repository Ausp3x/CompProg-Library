#include "../../03-Geometry/13-segment_union.hpp"

// Medians of five timed calls after one warmup. Both boundary versions must return the same segment multiset, the rings must use every
// piece, and the two coverage paths must agree.
template<typename F> lng median(F f) {
    f(); vector<lng> t;
    for (int r = 0; r < 5; ++r) {
        auto s = std::chrono::steady_clock::now(); f();
        t.push_back(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - s).count());}
    sort(t.begin(), t.end());
    return t[2];}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927, checksum = 0;
    std::mt19937_64 rng(seed);
    auto fail = [](const string &what) { std::cerr << what << '\n'; std::exit(1); };
    auto run = [&](const string &name, const vector<array<lng, 4>> &a) {
        vector<array<point, 2>> x, y; vector<vector<point>> z;
        lng sweep = median([&] { x = rectangleUnionBoundary(a); }), guting = median([&] { y = rectangleUnionBoundaryGuting(a); });
        lng rings = median([&] { z = rectangleUnionContours(a); });
        auto key = [](vector<array<point, 2>> v) {
            vector<array<lng, 4>> k;
            for (auto &s : v) { k.push_back({s[0].x, s[0].y, s[1].x, s[1].y}); }
            sort(k.begin(), k.end());
            return k;};
        if (key(x) != key(y)) { fail("boundary mismatch on " + name); }
        size_t corners = 0;
        for (auto &r : z) { corners += r.size(); }
        if (corners != x.size()) { fail("rings do not use every piece on " + name); }
        std::cout << "{\"case\": \"" << name << "\", \"n\": " << a.size() << ", \"out\": " << x.size() << ", \"sweep_us\": " << sweep << ", \"guting_us\": " << guting
                  << ", \"contours_us\": " << rings << "}\n";
        checksum += x.size() + z.size();};
    for (int n : {1000, 10000, 100000}) {
        vector<array<lng, 4>> a(n);
        for (auto &r : a) {
            lng x = lng(rng() % 1000000000), y = lng(rng() % 1000000000);
            r = {x, y, x + 1 + lng(rng() % 30000000), y + 1 + lng(rng() % 30000000)};}
        run("random", a);}
    for (int g : {100, 300, 1000}) {
        vector<array<lng, 4>> a;
        for (int i = 0; i < g; ++i) { a.push_back({0, 4 * lng(i), 4 * lng(g), 4 * lng(i) + 1}); a.push_back({4 * lng(i), 0, 4 * lng(i) + 1, 4 * lng(g)}); }
        run("bar lattice", a);}
    // Coverage at depth k: short rectangles (side about 6 / sqrt(n) of the square) and tall ones (side up to the whole square).
    for (int n : {10000, 100000}) {
        for (bool tall : {false, true}) {
            vector<array<lng, 4>> a(n); lng c = 1000000000, side = tall ? c : lng(6 * double(c) / std::sqrt(double(n)));
            for (auto &r : a) {
                lng x = lng(rng() % ulng(c)), y = lng(rng() % ulng(c));
                r = {x, y, x + 1 + lng(rng() % ulng(side)), y + 1 + lng(rng() % ulng(side))};}
            for (int k : {2, 8, 16, 17, 32, 64}) {
                pair<lll, lll> p, q;
                lng tree = median([&] { p = rectangle_detail::coverageTree(a, k); }), blocks = median([&] { q = rectangle_detail::coverageBlocks(a, k); });
                if (p != q) { fail("coverage paths disagree"); }
                std::cout << "{\"case\": \"coverage " << (tall ? "tall" : "short") << "\", \"n\": " << n << ", \"k\": " << k << ", \"tree_us\": " << tree
                          << ", \"blocks_us\": " << blocks << "}\n";
                checksum += ulng(p.second % 1000003);}}}
    std::cerr << checksum << '\n';}
