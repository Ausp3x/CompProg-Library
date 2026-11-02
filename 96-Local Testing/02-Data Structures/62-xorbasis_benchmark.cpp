#include "../../02-Data Structures/62-xorbasis.hpp"
#include "../../04-Graphs/28-cycle_basis.hpp"

// One implementation per binary (IMPL 0 old, 1 XorBasis<true>, 2 XorBasis<false>, 3 row 28 end to end); one JSON line per workload after an untimed warmup.
#ifndef IMPL
#define IMPL 0
#endif

namespace {
    using Clock = std::chrono::steady_clock;

    // The mask-free basis Graphs row 28 used before it moved to XorBasis (cycle_basis_detail at master fc8de13).
    struct OldBasis {
        vector<ulng> b;
        int size() const { return int(b.size()); }
        ulng reduce(ulng x) const {
            for (ulng y : b) { x = min(x, x ^ y); }
            return x;}
        bool insert(ulng x) {
            x = reduce(x);
            if (!x) { return false; }
            for (ulng &y : b) { y = min(y, y ^ x); }
            b.insert(std::ranges::upper_bound(b, x, std::greater<>()), x);
            return true;}
    };
    using X = std::conditional_t<IMPL == 0, OldBasis, XorBasis<IMPL == 1>>;

    template<typename F>
    void timed(const string &workload, F f) {
        f();
        auto t0 = Clock::now();
        ulng sum = f();
        double sec = std::chrono::duration<double>(Clock::now() - t0).count();
        std::cout << "{\"workload\":\"" << workload << "\",\"impl\":" << IMPL << ",\"seconds\":" << sec << ",\"checksum\":" << sum << "}\n";}

    // 5 * 10^6 random 64-bit inserts, a fresh basis every 1000.
    ulng inserts(const vector<ulng> &a) {
        ulng sum = 0;
        for (size_t i = 0; i < a.size(); i += 1000) {
            X x;
            for (size_t j = i; j < min(a.size(), i + 1000); ++j) { sum += x.insert(a[j]); }
            sum = sum * 31 + x.b.back();}
        return sum;}
    // XorWalk-like: 10^6 random 63-bit cycle values into one basis, mostly reductions that fail.
    ulng cycles(const vector<ulng> &a) {
        X x;
        ulng sum = 0;
        for (ulng v : a) { sum += x.insert(v >> 1); }
        return sum * 31 + x.reduce(a[0]);}
    // disconnects-like: 2 * 10^5 fresh bases of 40 random words, stopping at the first dependent one.
    ulng cuts(const vector<ulng> &a) {
        ulng sum = 0;
        for (size_t i = 0; i + 40 <= a.size(); i += 40) {
            X x;
            bool dep = false;
            for (size_t j = i; j < i + 40 && !dep; ++j) { dep = !x.insert(a[j]); }
            sum = sum * 31 + dep + ulng(x.size());}
        return sum;}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 1;
    std::mt19937_64 rng(seed);
    if constexpr (IMPL < 3) {
        vector<ulng> a(5000000), c(1000000), d(8000000);
        for (ulng &x : a) { x = rng(); }
        for (ulng &x : c) { x = rng(); }
        for (ulng &x : d) { x = rng(); }
        timed("inserts", [&] { return inserts(a); });
        timed("cycles", [&] { return cycles(c); });
        timed("cuts", [&] { return cuts(d); });}
    else {
        // XorWalk on n = 2 * 10^5, m = 10^6 random 63-bit weights; disconnects on 2 * 10^5 sets of 40 distinct edges.
        int n = 200000, m = 1000000;
        Graph g(n);
        for (int i = 1; i < n; ++i) { g.addEdge(int(rng() % ulng(i)), i, lng(rng() >> 1)); }
        for (int i = n - 1; i < m; ++i) { g.addEdge(int(rng() % ulng(n)), int(rng() % ulng(n)), lng(rng() >> 1)); }
        timed("XorWalk build", [&] { XorWalk x(g); return ulng(x.basis[0].size()) * 31 + x.maxXorWalk(0, n - 1); });
        vector<vector<int>> sets(200000);
        for (auto &s : sets) {
            while (s.size() < 40) { s.push_back(int(rng() % ulng(m))); sort(s.begin(), s.end()); s.erase(unique(s.begin(), s.end()), s.end()); }}
        CutSpaceHash h(g, seed);
        timed("disconnects", [&] { ulng sum = 0; for (auto &s : sets) { sum = sum * 31 + h.disconnects(s); } return sum; });}}
