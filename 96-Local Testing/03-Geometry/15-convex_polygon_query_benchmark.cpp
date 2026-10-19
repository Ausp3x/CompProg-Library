#include "../../03-Geometry/15-convex_polygon_query.hpp"
#include "../../03-Geometry/04-convexhull.hpp"
#include <sys/resource.h>

// Medians of five timed calls after one warmup; results are compared outside the timed region.
template<typename F> lng median(F f) {
    f(); vector<lng> t;
    for (int r = 0; r < 5; ++r) {
        auto s = std::chrono::steady_clock::now(); f();
        t.push_back(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - s).count());}
    sort(t.begin(), t.end());
    return t[2];}
vector<point> circleHull(std::mt19937_64 &rng, int n) {
    vector<point> p;
    for (int i = 0; i < n; ++i) {
        long double t = 2 * std::numbers::pi_v<long double> * (long double)(rng() % 1000000007) / 1000000007.L;
        p.push_back({std::llround(9e8L * std::cos(t)), std::llround(9e8L * std::sin(t))});}
    return convexHull(p);}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927, checksum = 0;
    std::mt19937_64 rng(seed);
    auto row = [&](const string &c, const string &size, const string &impl, lng us) {
        std::cout << "{\"case\": \"" << c << "\", \"size\": \"" << size << "\", \"implementation\": \"" << impl << "\", \"median_us\": " << us << "}\n";};
    if (argc > 3 && string(argv[2]) == "--memory") {
        // One untimed-median run of a large k for the peak resident memory of this process.
        int k = std::stoi(argv[3]), smawk = argc > 4 && string(argv[4]) == "smawk";
        auto p = circleHull(rng, 100000);
        auto s = std::chrono::steady_clock::now();
        auto r = smawk ? maxAreaInscribedKgonSmawk(p, k) : maxAreaInscribedKgon(p, k);
        lng us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - s).count();
        rusage u{}; getrusage(RUSAGE_SELF, &u);
        std::cout << "{\"case\": \"large k\", \"size\": \"n=" << p.size() << " k=" << k << "\", \"implementation\": \"" << (smawk ? "maxAreaInscribedKgonSmawk" : "maxAreaInscribedKgon")
                  << "\", \"single_run_us\": " << us << ", \"peak_rss_kb\": " << u.ru_maxrss << "}\n";
        std::cerr << ulng(r.value) << '\n';
        return int(r.vertices.size()) == k ? 0 : 1;}
    for (int n : {1000, 10000, 100000}) {
        auto p = circleHull(rng, n);
        string size = "n=" + std::to_string(p.size());
        for (int k : {3, 10, 100}) {
            InscribedKgon<lll> res, fast;
            row("inscribed k-gon area", size + " k=" + std::to_string(k), "maxAreaInscribedKgon", median([&] { res = maxAreaInscribedKgon(p, k); }));
            row("inscribed k-gon area", size + " k=" + std::to_string(k), "maxAreaInscribedKgonSmawk", median([&] { fast = maxAreaInscribedKgonSmawk(p, k); }));
            if (res.value != fast.value || int(res.vertices.size()) != k) { std::cerr << "k-gon mismatch\n"; return 1; }
            checksum += ulng(res.value);}
        InscribedKgon<lll> tri, quad;
        row("inscribed triangle", size, "maxAreaInscribedTriangle", median([&] { tri = maxAreaInscribedTriangle(p); }));
        row("inscribed quadrilateral", size, "maxAreaInscribedQuadrilateral", median([&] { quad = maxAreaInscribedQuadrilateral(p); }));
        InscribedKgon<lll> g3 = maxAreaInscribedKgon(p, 3), g4 = maxAreaInscribedKgon(p, 4);
        if (tri.value != g3.value || quad.value != g4.value) { std::cerr << "linear k-gon mismatch\n"; return 1; }
        row("inscribed quadrilateral", size, "maxAreaInscribedKgon (k = 4)", median([&] { g4 = maxAreaInscribedKgon(p, 4); }));
        checksum += ulng(tri.value) + ulng(quad.value);
        InscribedKgon<long double> per, pf;
        row("inscribed k-gon perimeter", size + " k=10", "maxPerimeterInscribedKgonApprox", median([&] { per = maxPerimeterInscribedKgonApprox(p, 10); }));
        row("inscribed k-gon perimeter", size + " k=10", "maxPerimeterInscribedKgonSmawkApprox", median([&] { pf = maxPerimeterInscribedKgonSmawkApprox(p, 10); }));
        if (std::abs(per.value - pf.value) > 1e-15L * per.value) { std::cerr << "perimeter mismatch\n"; return 1; }
        checksum += ulng(per.value);}
    // Point location in batches of q sorted queries on a 200,000-point hull, against q calls of contains.
    auto big = circleHull(rng, 200000); ConvexPolygonQuery cq(big); point o = big[0];
    for (int q : {10, 1000, 200000}) {
        vector<point> qs(q);
        for (auto &x : qs) { x = {lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000}; }
        sort(qs.begin(), qs.end(), [&](point x, point y) { return polarLess(x - o, y - o); });
        vector<PolygonLocation> a, b(q);
        string size = "n=" + std::to_string(big.size()) + " q=" + std::to_string(q);
        row("located queries", size, "batchedLocate", median([&] { a = cq.batchedLocate(qs); }));
        row("located queries", size, "contains per query", median([&] { for (int i = 0; i < q; ++i) { b[i] = cq.contains(qs[i]); } }));
        if (a != b) { std::cerr << "location mismatch\n"; return 1; }
        checksum += ulng(std::count(a.begin(), a.end(), PolygonLocation::Inside));}
    // Exact and rounded distances and cut areas, 100,000 calls each.
    vector<point> qs(100000), as(100000), bs(100000);
    for (int i = 0; i < 100000; ++i) {
        qs[i] = {lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000};
        as[i] = {lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000};
        do { bs[i] = {lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000}; } while (bs[i] == as[i]);}
    lll s1 = 0; long double s2 = 0;
    row("point distance", "n=" + std::to_string(big.size()) + " calls=100000", "pointDistance2", median([&] { s1 = 0; for (auto q : qs) { s1 += cq.pointDistance2(q).first; } }));
    row("point distance", "n=" + std::to_string(big.size()) + " calls=100000", "pointDistanceApprox", median([&] { s2 = 0; for (auto q : qs) { s2 += cq.pointDistanceApprox(q); } }));
    row("area left of a line", "n=" + std::to_string(big.size()) + " calls=100000", "areaLeftOfLine2", median([&] { s1 = 0; for (int i = 0; i < 100000; ++i) { s1 += cq.areaLeftOfLine2(as[i], bs[i]).whole; } }));
    row("area left of a line", "n=" + std::to_string(big.size()) + " calls=100000", "areaLeftOfLineApprox", median([&] { s2 = 0; for (int i = 0; i < 100000; ++i) { s2 += cq.areaLeftOfLineApprox(as[i], bs[i]); } }));
    checksum += ulng(s1) + ulng(s2);
    std::cerr << checksum << '\n';}
