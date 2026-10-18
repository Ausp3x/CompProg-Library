#include "../../03-Geometry/19-lattice_geometry.hpp"
#include "../../03-Geometry/15-convex_polygon_query.hpp"
#include "../../03-Geometry/04-convexhull.hpp"

// Medians of five timed calls after one warmup; results are compared outside the timed region.
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
    auto row = [&](const string &c, const string &size, const string &impl, lng us) {
        std::cout << "{\"case\": \"" << c << "\", \"size\": \"" << size << "\", \"implementation\": \"" << impl << "\", \"median_us\": " << us << "}\n";};
    for (lng r : {10000LL, 100000LL, 1000000LL, 10000000LL, 100000000LL, 1000000000LL}) {
        for (int rational = 0; rational < 2; ++rational) {
            lng q = rational ? 7 : 1, a = rational ? lng(rng() % 1000) - 500 : 0, b = rational ? lng(rng() % 1000) - 500 : 0;
            lng n = r <= 285000000 ? r * r * q * q : 4000000000000000000LL;
            string name = rational ? "centre (a, b) / 7" : "origin", size = "r=" + std::to_string(r);
            lll fast = 0, slow = 0;
            row(name, size, "latticeInDiskFast", median([&] { fast = latticeInDiskFast(a, b, q, n); }));
            if (r <= 10000000) {
                row(name, size, "latticeInDisk", median([&] { slow = latticeInDisk(a, b, q, n); }));
                if (fast != slow) { std::cerr << "disk mismatch at r=" << r << '\n'; return 1; }}
            checksum += ulng(fast);}}
    for (int n : {1000, 10000, 100000}) {
        vector<point> p;
        for (int i = 0; i < n; ++i) {
            long double t = 2 * std::numbers::pi_v<long double> * (long double)(rng() % 1000000007) / 1000000007.L;
            p.push_back({std::llround(9e8L * std::cos(t)), std::llround(9e8L * std::sin(t))});}
        p = convexHull(p);
        for (int k : {3, 10, 100}) {
            InscribedKgon<lll> res;
            row("inscribed k-gon area", "n=" + std::to_string(p.size()) + " k=" + std::to_string(k), "maxAreaInscribedKgon", median([&] { res = maxAreaInscribedKgon(p, k); }));
            if (int(res.vertices.size()) != k) { std::cerr << "k-gon size mismatch\n"; return 1; }
            checksum += ulng(res.value);}
        InscribedKgon<long double> per;
        row("inscribed k-gon perimeter", "n=" + std::to_string(p.size()) + " k=10", "maxPerimeterInscribedKgonApprox", median([&] { per = maxPerimeterInscribedKgonApprox(p, 10); }));
        checksum += ulng(per.value);}
    std::cerr << checksum << '\n';}
