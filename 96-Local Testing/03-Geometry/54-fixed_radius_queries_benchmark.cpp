#include "../../03-Geometry/54-fixed_radius_queries.hpp"

// The same points at two scales (lll comparator, then I512), about m candidates per pivot: S^2 = 4 * pi * n * r2 / m.
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
            std::cout << "{\"n\": " << n << ", \"candidates\": " << m << ", \"r2\": " << r2 << ", \"count\": " << a.first
                      << ", \"lll_us\": " << fast << ", \"i512_us\": " << wide << "}\n";
            checksum += ulng(a.first);}}
    std::cerr << checksum << '\n';}
