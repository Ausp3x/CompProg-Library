#include "../../03-Geometry/19-lattice_geometry.hpp"

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
    std::cerr << checksum << '\n';}
