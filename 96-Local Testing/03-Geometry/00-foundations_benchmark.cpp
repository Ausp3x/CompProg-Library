#include "../../03-Geometry/02-line_segment.hpp"
#include "../../03-Geometry/04-convexhull.hpp"
#include <chrono>

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    ulng checksum = 0;
    const vector<string> algorithms{"monotone", "graham", "sorted", "jarvis", "melkman"};
    for (int n : {32, 2048, 32768}) {
        for (string distribution : {"random", "grid", "collinear", "parabola", "square", "star"}) {
            vector<point> p(n);
            for (int i = 0; i < n; ++i) {
                lng x = lng(rng() % 2000001) - 1000000, y = lng(rng() % 2000001) - 1000000;
                if (distribution == "grid") { x = lng(rng() % 128); y = lng(rng() % 128); }
                if (distribution == "collinear") { x = i - n / 2; y = 3 * x + 7; }
                if (distribution == "parabola") { x = i - n / 2; y = x * x; }
                // Four corners and interior points: the hull has four vertices.
                if (distribution == "square" && i < 4) { x = i & 1 ? 1000001 : -1000001; y = i & 2 ? 1000001 : -1000001; }
                if (distribution == "star") {
                    long double a = 2 * std::numbers::pi_v<long double> * i / n, r = 500000 + lng(rng() % 500001);
                    x = std::llround(r * std::cos(a)); y = std::llround(r * std::sin(a));}
                p[i] = {x, y};}
            // The star is a simple polygon in exact angular order around the origin, one vertex per direction.
            if (distribution == "star") {
                sort(p.begin(), p.end(), [](point a, point b) { return polarLess(a, b); });
                p.erase(unique(p.begin(), p.end(), [](point a, point b) { return cross(a, b) == 0 && dot(a, b) > 0; }), p.end());}
            else { std::shuffle(p.begin(), p.end(), rng); }
            auto sorted = p; sort(sorted.begin(), sorted.end());
            for (bool keep : {false, true}) {
                auto expected = convexHull(p, keep);
                for (const string &algorithm : algorithms) {
                    if (algorithm == "melkman" && distribution != "star") { continue; }
                    // Gift wrapping is O(n * h); skip the quadratic all-extreme cases.
                    if (algorithm == "jarvis" && lng(expected.size()) * n > 50000000) { continue; }
                    auto call = [&] {
                        if (algorithm == "graham") { return convexHullGraham(p, keep); }
                        if (algorithm == "sorted") { return convexHullSorted(sorted, keep); }
                        if (algorithm == "jarvis") { return convexHullJarvis(p, keep); }
                        if (algorithm == "melkman") { return convexHullMelkman(p, keep); }
                        return convexHull(p, keep);};
                    vector<double> times;
                    // One warmup and five measurements; input copy and output allocation included.
                    for (int rep = -1; rep < 5; ++rep) {
                        auto start = std::chrono::steady_clock::now();
                        auto h = call();
                        auto end = std::chrono::steady_clock::now();
                        if (h != expected) { cerr << "timed output mismatch " << algorithm << ' ' << distribution << '\n'; return 1; }
                        for (point a : h) { checksum = checksum * 1000003 + ulng(a.x) + 31 * ulng(a.y); }
                        if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
                    sort(times.begin(), times.end());
                    cout << "{\"n\":" << p.size() << ",\"distribution\":\"" << distribution
                         << "\",\"keep_collinear\":" << (keep ? "true" : "false")
                         << ",\"algorithm\":\"" << algorithm
                         << "\",\"hull_size\":" << expected.size() << ",\"median_us\":" << times[2] << "}\n";}}}}
    // Segment pairs: the long double segment distance against the exact rational one (ruling 4 of duplicates.md).
    for (int n : {2048, 32768}) {
        vector<array<point, 4>> q(n);
        for (auto &s : q) {
            for (point &a : s) { a = {lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000}; }}
        vector<long double> exact;
        for (const auto &s : q) { exact.push_back(linearDistanceApprox(Linear2::segment(s[0], s[1]), Linear2::segment(s[2], s[3]))); }
        for (string algorithm : {"segmentDistanceApprox", "linearDistanceApprox"}) {
            vector<double> times;
            for (int rep = -1; rep < 5; ++rep) {
                vector<long double> d(n);
                auto start = std::chrono::steady_clock::now();
                if (algorithm == "linearDistanceApprox") {
                    for (int i = 0; i < n; ++i) { d[i] = linearDistanceApprox(Linear2::segment(q[i][0], q[i][1]), Linear2::segment(q[i][2], q[i][3])); }}
                else {
                    for (int i = 0; i < n; ++i) { d[i] = segmentDistanceApprox(q[i][0], q[i][1], q[i][2], q[i][3]); }}
                auto end = std::chrono::steady_clock::now();
                long double sum = 0;
                for (int i = 0; i < n; ++i) {
                    if (abs(d[i] - exact[i]) > 1e-9L * max(1.L, exact[i])) { cerr << "distance mismatch " << algorithm << '\n'; return 1; }
                    sum += d[i];}
                checksum = checksum * 1000003 + ulng(sum);
                if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
            sort(times.begin(), times.end());
            cout << "{\"n\":" << n << ",\"distribution\":\"segment pairs\",\"keep_collinear\":false,\"algorithm\":\"" << algorithm
                 << "\",\"hull_size\":0,\"median_us\":" << times[2] << "}\n";}}
    cerr << "checksum=" << checksum << '\n';}
