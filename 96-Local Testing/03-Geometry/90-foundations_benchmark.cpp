#include "../../03-Geometry/04-convexhull.hpp"
#include <chrono>

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    ulng checksum = 0;
    for (int n : {32, 2048, 32768}) {
        for (string distribution : {"random", "grid", "collinear", "parabola"}) {
            vector<point> p(n);
            for (int i = 0; i < n; ++i) {
                lng x = lng(rng() % 2000001) - 1000000, y = lng(rng() % 2000001) - 1000000;
                if (distribution == "grid") { x = lng(rng() % 128); y = lng(rng() % 128); }
                if (distribution == "collinear") { x = i - n / 2; y = 3 * x + 7; }
                if (distribution == "parabola") { x = i - n / 2; y = x * x; }
                p[i] = {x, y}; }
            std::shuffle(p.begin(), p.end(), rng);
            for (bool keep : {false, true}) {
                auto expected = convexHull(p, keep);
                if (convexHullGraham(p, keep) != expected) { cerr << "hull mismatch\n"; return 1; }
                for (bool graham : {false, true}) {
                    vector<double> times;
                    // One warmup and five measurements; input copy and output allocation included.
                    for (int rep = -1; rep < 5; ++rep) {
                        auto start = std::chrono::steady_clock::now();
                        auto h = graham ? convexHullGraham(p, keep) : convexHull(p, keep);
                        auto end = std::chrono::steady_clock::now();
                        if (h != expected) { cerr << "timed output mismatch\n"; return 1; }
                        for (point a : h) { checksum = checksum * 1000003 + ulng(a.x) + 31 * ulng(a.y); }
                        if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
                    sort(times.begin(), times.end());
                    cout << "{\"n\":" << n << ",\"distribution\":\"" << distribution
                         << "\",\"keep_collinear\":" << (keep ? "true" : "false")
                         << ",\"algorithm\":\"" << (graham ? "graham" : "monotone")
                         << "\",\"hull_size\":" << expected.size() << ",\"median_us\":" << times[2] << "}\n"; }}}
    }
    cerr << "checksum=" << checksum << '\n';
}
