#include "../../03-Geometry/05-closestpair.hpp"
#include "../../OLD/Team Notebook/src/geometry/getnearestpair.cpp"

ClosestPair brute(const vector<point> &p) {
    ClosestPair a;
    for (int i = 0; i < int(p.size()); ++i) { for (int j = i + 1; j < int(p.size()); ++j) {
        lll x = lll(p[i].x) - p[j].x, y = lll(p[i].y) - p[j].y, d = x * x + y * y;
        if (a.ids.first < 0 || d < a.distance2) { a = {{i, j}, d}; } }}
    return a; }
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927, checksum = 0;
    std::mt19937_64 rng(seed);
    for (int n : {32, 512, 2048, 200000}) { for (string shape : {"random", "grid", "vertical", "duplicates"}) {
        vector<point> p;
        for (int i = 0; i < n; ++i) {
            if (shape == "random") { p.push_back({lng(rng() % 2000001) - 1000000, lng(rng() % 2000001) - 1000000}); }
            if (shape == "grid") { p.push_back({i % 400, i / 400}); }
            if (shape == "vertical") { p.push_back({0, -i}); }
            if (shape == "duplicates") { p.push_back({7, 11}); }}
        auto expected = closestPair(p); auto before = p;
        if (n <= 2048) {
            auto b = brute(p);
            if (b.ids != expected.ids || b.distance2 != expected.distance2) { std::cerr << "brute mismatch\n"; return 1; }}
        for (string method : {"closestPair", "legacy", "brute"}) {
            if (method == "brute" && n > 2048) { continue; }
            vector<double> times;
            for (int rep = -1; rep < 5; ++rep) {
                auto start = std::chrono::steady_clock::now();
                ClosestPair a;
                if (method == "closestPair") { a = closestPair(p); }
                else if (method == "brute") { a = brute(p); }
                else { auto ids = getNearestPair(p); a = {ids, dist2(p[ids.first], p[ids.second])}; }
                auto end = std::chrono::steady_clock::now();
                if (a.distance2 != expected.distance2 || p != before ||
                    (method != "legacy" && a.ids != expected.ids)) { std::cerr << "result mismatch " << method << '\n'; return 1; }
                checksum += ulng(a.distance2) + ulng(a.ids.first + 1);
                if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
            sort(times.begin(), times.end());
            cout << "{\"n\":" << n << ",\"shape\":\"" << shape << "\",\"method\":\"" << method << "\",\"median_us\":" << times[2] << "}\n"; }} }
    std::cerr << "checksum=" << checksum << '\n'; }
