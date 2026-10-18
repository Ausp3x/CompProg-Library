#include "../../05-Mathematics/67-lattice_paths.hpp"

// Library Checker sizes for the divide and conquer on bounds, the rectangle transfer, Bizley and the obstacle DP.
template<typename F> double median(int reps, F f, ulng &check) {
    vector<double> t;
    check = f();
    for (int i = 0; i < reps; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        ulng c = f();
        t.pb(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
        if (c != check) { cerr << "nondeterministic output\n"; std::exit(1); }}
    sort(t.begin(), t.end());
    return t[t.size() / 2];}

int main(int argc, char **argv) {
    int reps = argc > 1 ? std::atoi(argv[1]) : 5, n = 131072;
    std::mt19937_64 rng(1);
    vector<int> ba(n), bb(n), ra(n), rb(n);
    for (int i = 0; i < n; ++i) {
        ba[i] = max(0, i - 2000); bb[i] = min(n, i + 2000);
        ra[i] = int(rng() % ulng(n / 2)); rb[i] = n / 2 + 1 + int(rng() % ulng(n / 2));}
    vector<mint> left(n), bottom(n);
    for (auto &x : left) { x = mint(rng()); }
    for (auto &x : bottom) { x = mint(rng()); }
    vector<pair<int, int>> pts(3000);
    for (auto &[x, y] : pts) { x = int(rng() % 100000); y = int(rng() % 100000); }
    ModFac pc(400000);
    vector<pair<string, std::function<ulng()>>> work = {
        {"increasingSequences N=M=131072, band 4000", [&] { return ulng(increasingSequencesBetweenBounds(ba, bb).val()); }},
        {"increasingSequences N=M=131072, random bounds", [&] { return ulng(increasingSequencesBetweenBounds(ra, rb).val()); }},
        {"gridPathsBoundaryTransfer h=w=131072", [&] { auto [r, t] = gridPathsBoundaryTransfer(left, bottom, pc); return ulng((r.back() + t.back()).val()); }},
        {"bizleyPaths a=3 b=5 n=1e5", [&] { return ulng(bizleyPaths(3, 5, 100000)[100000].val()); }},
        {"gridPathsAvoidingPoints k=3000", [&] { return ulng(gridPathsAvoidingPoints(100000, 100000, pts, pc).val()); }},
        {"motzkinTable, schroderLarge, delannoyCentral n=1e6", [&] { return ulng((motzkinTable(1000000).back() + schroderLarge(1000000).back() + delannoyCentral(1000000).back()).val()); }},
    };
    cout << "[";
    for (size_t i = 0; i < work.size(); ++i) {
        ulng check = 0;
        double ms = median(reps, work[i].second, check);
        cout << (i ? ",\n" : "\n") << "{\"workload\": \"" << work[i].first << "\", \"median_ms\": " << ms << ", \"check\": " << check << "}";}
    cout << "\n]\n";}
