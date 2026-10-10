#include "../../02-Data Structures/14-segtreebeats.hpp"

namespace {
    using Rng = std::mt19937_64;
    using Clock = std::chrono::steady_clock;
    constexpr int N = 200000, Q = 200000;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    pair<int, int> range(Rng &rng) {
        int l = int(rnd(rng, 0, N)), r = int(rnd(rng, 0, N));
        if (l > r) { swap(l, r); }
        return {l, r};}

    // Each workload returns a checksum of its query answers so the work cannot be optimized away.
    ulng randomBeats(Rng &rng) {
        vector<lng> a(N);
        for (auto &x : a) { x = rnd(rng, -1000000000, 1000000000); }
        SegTreeBeats t(a); ulng h = 0;
        for (int q = 0; q < Q; ++q) {
            auto [l, r] = range(rng);
            lng x = rnd(rng, -1000000000, 1000000000);
            int k = int(rng() % 5);
            if (k == 0) { t.chminUpdate(l, r, x); }
            else if (k == 1) { t.chmaxUpdate(l, r, x); }
            else if (k == 2) { t.addUpdate(l, r, x / 1000); }
            else if (k == 3) { t.setUpdate(l, r, x); }
            else { h = h * 31 + ulng(t.sumQuery(l, r)); }}
        return h;}
    ulng divideAlternating(Rng &) {
        vector<lng> a(N);
        for (int i = 0; i < N; ++i) { a[i] = 1 + (i & 1); }
        SegTreeBeats t(a); ulng h = 0;
        for (int q = 0; q < Q / 2; ++q) { t.divideUpdate(0, N, 2); t.addUpdate(0, N, 1); h = h * 31 + ulng(t.sumQuery(0, N)); }
        return h;}
    ulng sqrtAlternating(Rng &) {
        lng k = 1000000;
        vector<lng> a(N);
        for (int i = 0; i < N; ++i) { a[i] = k * k - (i & 1); }
        SegTreeBeats t(a); ulng h = 0;
        for (int q = 0; q < Q / 2; ++q) { t.sqrtUpdate(0, N); t.addUpdate(0, N, k * k - k); h = h * 31 + ulng(t.sumQuery(0, N)); }
        return h;}
    ulng randomValueOps(Rng &rng) {
        vector<lng> a(N);
        for (auto &x : a) { x = rnd(rng, 0, 1000000000000); }
        SegTreeBeats t(a); ulng h = 0;
        for (int q = 0; q < Q; ++q) {
            auto [l, r] = range(rng);
            int k = int(rng() % 5);
            if (k == 0) { t.divideUpdate(l, r, rnd(rng, 2, 10)); }
            else if (k == 1) { t.sqrtUpdate(l, r); }
            else if (k == 2) { t.modUpdate(l, r, rnd(rng, 1, 1000000)); }
            else if (k == 3) { t.setUpdate(l, r, rnd(rng, 0, 1000000000000)); }
            else { h = h * 31 + ulng(t.sumQuery(l, r)); }}
        return h;}
    ulng historic(Rng &rng) {
        vector<lng> a(N);
        for (auto &x : a) { x = rnd(rng, -1000000, 1000000); }
        HistoricSegTree t(a); ulng h = 0;
        for (int q = 0; q < Q; ++q) {
            auto [l, r] = range(rng);
            if (q & 1) { t.addUpdate(l, r, rnd(rng, -1000, 1000)); t.tick(); }
            else { h = h * 31 + ulng(t.historicMaxQuery(l, r) ^ t.historicSumQuery(l, r)); }}
        return h;}
    ulng andOr(Rng &rng) {
        using A = RangeAndOrRangeSumMax<uint>;
        vector<A::S> a(N);
        for (auto &x : a) { x = A::leaf(uint(rng())); }
        LazySegmentTreeBeats<A> t(a); ulng h = 0;
        for (int q = 0; q < Q; ++q) {
            auto [l, r] = range(rng);
            uint x = uint(rng());
            int k = int(rng() % 3);
            if (k == 0) { t.apply(l, r, A::andWith(x)); }
            else if (k == 1) { t.apply(l, r, A::orWith(x & uint(rng()))); }
            else { auto s = t.prod(l, r); h = h * 31 + s.sum + s.mx; }}
        return h;}
} // namespace

int main() {
    vector<pair<string, ulng (*)(Rng &)>> work = {{"chmin/chmax/add/set/sum random", randomBeats}, {"divide+add alternating 1,2", divideAlternating},
        {"sqrt+add alternating k^2-1,k^2", sqrtAlternating}, {"divide/sqrt/mod/set random", randomValueOps}, {"historic add+tick random", historic}, {"generic and/or sum+max random", andOr}};
    for (auto &[name, f] : work) {
        vector<double> ms; ulng h0 = 0;
        for (int rep = 0; rep < 5; ++rep) {
            Rng rng(20261010);
            auto start = Clock::now();
            ulng h = f(rng);
            ms.push_back(std::chrono::duration<double, std::milli>(Clock::now() - start).count());
            if (rep && h != h0) { std::cerr << "checksum mismatch " << name << '\n'; return 1; }
            h0 = h;}
        sort(ms.begin(), ms.end());
        std::cout << name << " n=" << N << " q=" << Q << " median_ms=" << std::fixed << std::setprecision(1) << ms[2] << " checksum=" << h0 << '\n';}}
