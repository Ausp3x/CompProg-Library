#include "../../03-Geometry/54-fixed_radius_queries.hpp"

// maxPointsInDisk on the same points at two scales (lll comparator, then 384-bit), about m candidates per pivot: S^2 = 4 * pi * n * r2 / m;
// the same points at r2 = 2^30 (lll) and 2^30 + 1 (wide); GridHash against SortedGrid on uniform and clustered points.
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
    for (int n : {500, 2000, 8000}) {
        for (int m : {10, 50}) {
            lng r2 = 100000000, S = min(lng(1000000), lng(std::sqrt(4 * std::numbers::pi_v<long double> * n * r2 / m))); vector<point> p(n), q(n);
            for (int i = 0; i < n; ++i) { p[i] = {lng(rng() % ulng(S)), lng(rng() % ulng(S))}; q[i] = p[i] * 1000; }
            pair<int, dpoint> a, b;
            lng fast = median([&] { a = maxPointsInDisk(p, r2); }), wide = median([&] { b = maxPointsInDisk(q, r2 * 1000000); });
            if (a.first != b.first) { std::cerr << "scaled counts differ\n"; return 1; }
            std::cout << "{\"case\": \"scaled\", \"n\": " << n << ", \"candidates\": " << m << ", \"count\": " << a.first
                      << ", \"lll_us\": " << fast << ", \"wide_us\": " << wide << "}\n";
            checksum += ulng(a.first);}}
    for (int n : {2000, 8000}) {
        for (int m : {10, 50}) {
            lng r2 = lng(1) << 30, S = lng(std::sqrt(4 * std::numbers::pi_v<long double> * n * r2 / m)); vector<point> p(n);
            for (auto &x : p) { x = {lng(rng() % ulng(S)), lng(rng() % ulng(S))}; }
            pair<int, dpoint> a, b;
            lng fast = median([&] { a = maxPointsInDisk(p, r2); }), wide = median([&] { b = maxPointsInDisk(p, r2 + 1); });
            if (a.first > b.first) { std::cerr << "a larger disk covers fewer points\n"; return 1; }
            std::cout << "{\"case\": \"threshold\", \"n\": " << n << ", \"candidates\": " << m << ", \"count\": " << a.first
                      << ", \"lll_us\": " << fast << ", \"wide_us\": " << wide << "}\n";
            checksum += ulng(a.first + b.first);}}
    for (int n : {100000, 1000000}) {
        for (int clustered : {0, 1}) {
            // Uniform points with about 8 others within r, or clusters of 10 inside a 300 x 300 square.
            lng r = 1000, S = lng(std::sqrt(std::numbers::pi_v<long double> * n * r * r / 8)); vector<point> p(n);
            for (auto &x : p) { x = {lng(rng() % ulng(S)), lng(rng() % ulng(S))}; }
            if (clustered) { for (int i = 0; i < n; ++i) { p[i] = p[i % (n / 10)] + point{lng(rng() % 300), lng(rng() % 300)}; } }
            ulng hp = 0, sp = 0;
            lng hb = median([&] { GridHash g(p, r); hp = g.diskCoverCounts()[0]; }), sb = median([&] { SortedGrid g(p, r); sp = g.diskCoverCounts()[0]; });
            GridHash g(p, r); SortedGrid h(p, r); vector<pair<int, int>> x, y;
            lng hq = median([&] { x = g.pairsWithin(); }), sq = median([&] { y = h.pairsWithin(); });
            if (x != y || hp != sp || g.diskCoverCounts() != h.diskCoverCounts()) { std::cerr << "grids differ\n"; return 1; }
            std::cout << "{\"case\": \"" << (clustered ? "grid clustered" : "grid uniform") << "\", \"n\": " << n << ", \"pairs\": " << x.size()
                      << ", \"gridhash_build_cover_us\": " << hb << ", \"sortedgrid_build_cover_us\": " << sb
                      << ", \"gridhash_pairs_us\": " << hq << ", \"sortedgrid_pairs_us\": " << sq << "}\n";
            checksum += x.size();}}
    std::cerr << checksum << '\n';}
