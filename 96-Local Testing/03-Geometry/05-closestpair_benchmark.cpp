#include "../../03-Geometry/05-closestpair.hpp"
#include "../../OLD/Team Notebook/src/geometry/getnearestpair.cpp"

ClosestPair brute(const vector<point> &p) {
    ClosestPair a;
    for (int i = 0; i < int(p.size()); ++i) { for (int j = i + 1; j < int(p.size()); ++j) {
        lll x = lll(p[i].x) - p[j].x, y = lll(p[i].y) - p[j].y, d = x * x + y * y;
        if (a.ids.first < 0 || d < a.distance2) { a = {{i, j}, d}; }}}
    return a;}
ClosestPair brute3(const vector<Point3<lng>> &p) {
    ClosestPair a;
    for (int i = 0; i < int(p.size()); ++i) {
        for (int j = i + 1; j < int(p.size()); ++j) {
            lll d = dist2(p[i], p[j]);
            if (a.ids.first < 0 || d < a.distance2) { a = {{i, j}, d}; }}}
    return a;}
pair<int, int> approxBrute(const vector<dpoint> &p) {
    pair<int, int> a{-1, -1}; long double best = 0;
    for (int i = 0; i < int(p.size()); ++i) {
        for (int j = i + 1; j < int(p.size()); ++j) {
            long double x = p[i].x - p[j].x, y = p[i].y - p[j].y, d = x * x + y * y;
            if (a.first < 0 || d < best) { a = {i, j}; best = d; }}}
    return a;}
long double side(dpoint a, dpoint b) { long double x = a.x - b.x, y = a.y - b.y; return std::sqrt(x * x + y * y); }
long double perimeter(const vector<dpoint> &q, array<int, 3> t) { return side(q[t[0]], q[t[1]]) + side(q[t[1]], q[t[2]]) + side(q[t[0]], q[t[2]]); }
array<int, 3> triangleBrute(const vector<dpoint> &q) {
    array<int, 3> a{-1, -1, -1}; long double best = 0;
    for (int i = 0; i < int(q.size()); ++i) {
        for (int j = i + 1; j < int(q.size()); ++j) {
            for (int k = j + 1; k < int(q.size()); ++k) {
                long double v = perimeter(q, {i, j, k});
                if (a[0] < 0 || v < best) { a = {i, j, k}; best = v; }}}}
    return a;}
// Every side of a triangle is at most half its perimeter, so an x-sorted window brute force sees every triple that could beat best.
bool windowCheck(const vector<dpoint> &q, array<int, 3> best) {
    int n = int(q.size());
    long double per = perimeter(q, best), lim = per / 2 * (1 + 1e-9L);
    vector<int> ids(n), near; iota(ids.begin(), ids.end(), 0);
    sort(ids.begin(), ids.end(), [&](int i, int j) { return q[i].x < q[j].x; });
    for (int a = 0; a < n; ++a) {
        near.clear();
        for (int b = a + 1; b < n && q[ids[b]].x - q[ids[a]].x <= lim; ++b) {
            if (std::abs(q[ids[b]].y - q[ids[a]].y) <= lim) { near.push_back(ids[b]); }}
        for (int b = 0; b < int(near.size()); ++b) {
            for (int c = b + 1; c < int(near.size()); ++c) {
                array<int, 3> t{ids[a], near[b], near[c]}; sort(t.begin(), t.end());
                long double v = perimeter(q, t);
                if (v < per || (v == per && t < best)) { return false; }}}}
    return true;}
vector<dpoint> widen(const vector<point> &p) {
    vector<dpoint> q; for (auto a : p) { q.push_back(a.cast<long double>()); }
    return q;}
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
        for (string method : {"closestPair", "closestPairRandomized", "legacy", "brute"}) {
            if (method == "brute" && n > 2048) { continue; }
            vector<double> times;
            for (int rep = -1; rep < 5; ++rep) {
                auto start = std::chrono::steady_clock::now();
                ClosestPair a;
                if (method == "closestPair") { a = closestPair(p); }
                else if (method == "closestPairRandomized") { a = closestPairRandomized(p, seed + ulng(rep + 2) * 0x9e3779b97f4a7c15ULL); }
                else if (method == "brute") { a = brute(p); }
                else { auto ids = getNearestPair(p); a = {ids, dist2(p[ids.first], p[ids.second])}; }
                auto end = std::chrono::steady_clock::now();
                if (a.distance2 != expected.distance2 || p != before ||
                    (method != "legacy" && a.ids != expected.ids)) { std::cerr << "result mismatch " << method << '\n'; return 1; }
                checksum += ulng(a.distance2) + ulng(a.ids.first + 1);
                if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
            sort(times.begin(), times.end());
            cout << "{\"n\":" << n << ",\"shape\":\"" << shape << "\",\"method\":\"" << method << "\",\"median_us\":" << times[2] << "}\n";}
        // Integer coordinates below 2^21 make every long double squared distance exact, so closestPair's ids are the expected answer.
        if (shape != "random") { continue; }
        auto q = widen(p); auto qbefore = q;
        if (n <= 2048 && approxBrute(q) != expected.ids) { std::cerr << "approx brute mismatch\n"; return 1; }
        for (string method : {"closestPairApprox", "approxBrute"}) {
            if (method == "approxBrute" && n > 2048) { continue; }
            vector<double> times;
            for (int rep = -1; rep < 5; ++rep) {
                auto start = std::chrono::steady_clock::now();
                auto ids = method == "approxBrute" ? approxBrute(q) : closestPairApprox(q);
                auto end = std::chrono::steady_clock::now();
                if (ids != expected.ids || q != qbefore) { std::cerr << "result mismatch " << method << '\n'; return 1; }
                checksum += ulng(ids.first + 1);
                if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
            sort(times.begin(), times.end());
            cout << "{\"n\":" << n << ",\"shape\":\"random-long-double\",\"method\":\"" << method << "\",\"median_us\":" << times[2] << "}\n";}}}
    for (int n : {32, 512, 2048, 200000}) {
        for (string shape : {"random", "grid", "duplicate-pairs"}) {
            vector<point> p;
            for (int i = 0; i < n; ++i) {
                if (shape == "random") { p.push_back({lng(rng() % 2000001) - 1000000, lng(rng() % 2000001) - 1000000}); }
                if (shape == "grid") { p.push_back({i % 400, i / 400}); }
                if (shape == "duplicate-pairs") { p.push_back({10 * (i / 2 % 400), 10 * (i / 2 / 400)}); }}
            auto q = widen(p); auto before = p;
            auto expected = minimumPerimeterTriangleApprox(p);
            if (n <= 512 ? triangleBrute(q) != expected : !windowCheck(q, expected)) { std::cerr << "triangle check mismatch " << shape << '\n'; return 1; }
            for (string method : {"minimumPerimeterTriangleApprox", "triangleBrute"}) {
                if (method == "triangleBrute" && n > 512) { continue; }
                vector<double> times;
                for (int rep = -1; rep < 5; ++rep) {
                    auto start = std::chrono::steady_clock::now();
                    auto t = method == "triangleBrute" ? triangleBrute(q) : minimumPerimeterTriangleApprox(p);
                    auto end = std::chrono::steady_clock::now();
                    if (t != expected || p != before) { std::cerr << "result mismatch " << method << '\n'; return 1; }
                    checksum += ulng(t[0] + t[1] + t[2]);
                    if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
                sort(times.begin(), times.end());
                cout << "{\"n\":" << n << ",\"shape\":\"triangle-" << shape << "\",\"method\":\"" << method << "\",\"median_us\":" << times[2] << "}\n";}}}
    for (int n : {32, 512, 2048, 200000}) {
        for (string shape : {"random3", "lattice3", "plane3", "duplicates3"}) {
            vector<Point3<lng>> p;
            for (int i = 0; i < n; ++i) {
                if (shape == "random3") { p.push_back({lng(rng() % 2000001) - 1000000, lng(rng() % 2000001) - 1000000, lng(rng() % 2000001) - 1000000}); }
                if (shape == "lattice3") { p.push_back({i % 60, i / 60 % 60, i / 3600}); }
                if (shape == "plane3") { p.push_back({0, i % 450, i / 450}); }
                if (shape == "duplicates3") { p.push_back({7, 11, 13}); }}
            auto expected = closestPair3(p); auto before = p;
            if (n <= 2048) {
                auto b = brute3(p);
                if (b.ids != expected.ids || b.distance2 != expected.distance2) { std::cerr << "brute3 mismatch\n"; return 1; }}
            for (string method : {"closestPair3", "brute3"}) {
                if (method == "brute3" && n > 2048) { continue; }
                vector<double> times;
                for (int rep = -1; rep < 5; ++rep) {
                    auto start = std::chrono::steady_clock::now();
                    ClosestPair a = method == "brute3" ? brute3(p) : closestPair3(p);
                    auto end = std::chrono::steady_clock::now();
                    if (a.distance2 != expected.distance2 || a.ids != expected.ids || p != before) { std::cerr << "result mismatch " << method << '\n'; return 1; }
                    checksum += ulng(a.distance2) + ulng(a.ids.first + 1);
                    if (rep >= 0) { times.push_back(std::chrono::duration<double, std::micro>(end - start).count()); }}
                sort(times.begin(), times.end());
                cout << "{\"n\":" << n << ",\"shape\":\"" << shape << "\",\"method\":\"" << method << "\",\"median_us\":" << times[2] << "}\n";}}}
    std::cerr << "checksum=" << checksum << '\n';}
