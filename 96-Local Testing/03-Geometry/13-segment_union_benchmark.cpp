#include "../../03-Geometry/13-segment_union.hpp"

// Medians of five timed calls after one warmup; both boundary versions must return the same segment multiset.
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
    auto run = [&](const string &name, const vector<array<lng, 4>> &a) {
        vector<array<point, 2>> x, y;
        lng sweep = median([&] { x = rectangleUnionBoundary(a); }), guting = median([&] { y = rectangleUnionBoundaryGuting(a); });
        auto key = [](vector<array<point, 2>> v) {
            vector<array<lng, 4>> k;
            for (auto &s : v) { k.push_back({s[0].x, s[0].y, s[1].x, s[1].y}); }
            sort(k.begin(), k.end());
            return k;};
        if (key(x) != key(y)) { std::cerr << "boundary mismatch on " << name << '\n'; std::exit(1); }
        std::cout << "{\"case\": \"" << name << "\", \"n\": " << a.size() << ", \"out\": " << x.size() << ", \"sweep_us\": " << sweep << ", \"guting_us\": " << guting << "}\n";
        checksum += x.size();};
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
    std::cerr << checksum << '\n';}
