#include "../../02-Data Structures/00-monoids.hpp"
#include "../../02-Data Structures/17-waveletmatrix.hpp"
#include "../../02-Data Structures/18-persistentsegmenttree.hpp"

// Emits one JSON line per (workload, variant); variants of one workload must agree on the checksum.
namespace {
    using Clock = std::chrono::steady_clock;
    constexpr uint MOD = 998244353;

    double since(Clock::time_point t0) { return std::chrono::duration<double>(Clock::now() - t0).count(); }
    void emit(const string &workload, const string &variant, int n, int q, double sec, ulng sum, lng nodes) {
        cout << "{\"workload\":\"" << workload << "\",\"variant\":\"" << variant << "\",\"n\":" << n << ",\"q\":" << q
             << ",\"seconds\":" << sec << ",\"checksum\":" << sum << ",\"nodes\":" << nodes << "}\n";}

    // Affine maps x -> a * x + b mod 998244353 on (sum, len); F is FE (with operator==) or FN (without).
    struct FE { uint a, b; bool operator==(const FE &) const = default; };
    struct FN { uint a, b; };
    template<typename G>
    struct Affine {
        struct S { uint sum, len; };
        using F = G;
        static S op(const S &x, const S &y) { return {(x.sum + y.sum) % MOD, x.len + y.len}; }
        static S e() { return {0, 0}; }
        static S mapping(const F &f, const S &x) { return {uint((ulng(f.a) * x.sum + ulng(f.b) * x.len) % MOD), x.len}; }
        static F composition(const F &f, const F &g) { return {uint(ulng(f.a) * g.a % MOD), uint((ulng(f.a) * g.b + f.b) % MOD)}; }
        static F id() { return {1, 0}; }
    };

    // Q1: range add / range sum; latest picks the newest version, otherwise a random one; applyPct of q are applies, the rest prods.
    template<bool C>
    void lazyAdd(ulng seed, int n, int q, bool latest, int applyPct) {
        using M = RangeAddRangeSum<lng>;
        std::mt19937_64 rng(seed);
        vector<M::S> v(n);
        for (auto &x : v) { x = M::leaf(lng(rng() % 1000000000)); }
        auto t0 = Clock::now();
        PersistentSegmentTree<M, C> st(v);
        ulng sum = 0;
        for (int i = 0; i < q; ++i) {
            int k = latest ? st.versionCount() - 1 : int(rng() % ulng(st.versionCount())), l = int(rng() % ulng(n)), r = int(rng() % ulng(n));
            if (l > r) { swap(l, r); }
            ++r;
            if (int(rng() % 100) < applyPct) { st.apply(k, l, r, lng(rng() % 10001)); }
            else { sum = sum * 31 + ulng(st.prod(k, l, r).sum); }}
        sum = sum * 31 + ulng(st.allProd(st.versionCount() - 1).sum);
        emit(string("q1-") + (applyPct == 100 ? "apply" : "mixed") + (latest ? "-latest" : "-random"), C ? "comm" : "noncomm", n, q, since(t0), sum, st.nodeCount());}

    // Q2: dual tree, range add / point get.
    template<bool C>
    void dualAdd(ulng seed, int n, int q, bool latest, int applyPct) {
        std::mt19937_64 rng(seed);
        auto t0 = Clock::now();
        PersistentDualSegmentTree<MonoidAction<Sum<lng>>, C> st(n);
        ulng sum = 0;
        for (int i = 0; i < q; ++i) {
            int k = latest ? st.versionCount() - 1 : int(rng() % ulng(st.versionCount())), l = int(rng() % ulng(n)), r = int(rng() % ulng(n));
            if (l > r) { swap(l, r); }
            ++r;
            if (int(rng() % 100) < applyPct) { st.apply(k, l, r, lng(rng() % 1000000000)); }
            else { sum = sum * 31 + ulng(st.get(k, l)); }}
        sum = sum * 31 + ulng(st.get(st.versionCount() - 1, n / 2));
        emit(string("q2-") + (applyPct == 100 ? "apply" : "mixed") + (latest ? "-latest" : "-random"), C ? "comm" : "noncomm", n, q, since(t0), sum, st.nodeCount());}

    // Q3: p affine applies chained on the latest version, then q ops on random versions, half point set, half prod.
    template<typename G>
    void affineSkip(ulng seed, int n, int p, int q, const string &variant) {
        using M = Affine<G>;
        std::mt19937_64 rng(seed);
        vector<typename M::S> v(n);
        for (auto &x : v) { x = {uint(rng() % MOD), 1}; }
        PersistentSegmentTree<M> st(v);
        auto range = [&](int &l, int &r) {
            l = int(rng() % ulng(n)); r = int(rng() % ulng(n));
            if (l > r) { swap(l, r); }
            ++r;};
        auto t0 = Clock::now();
        int l, r, built = st.nodeCount();
        for (int i = 0; i < p; ++i) {
            range(l, r);
            st.apply(st.versionCount() - 1, l, r, G{uint(rng() % MOD), uint(rng() % MOD)});}
        double applySec = since(t0);
        int applied = st.nodeCount();
        ulng sum = st.allProd(st.versionCount() - 1).sum;
        emit("q3-apply", variant, n, p, applySec, sum, applied - built);
        t0 = Clock::now();
        for (int i = 0; i < q; ++i) {
            int k = int(rng() % ulng(st.versionCount()));
            range(l, r);
            if (rng() & 1) { st.set(k, l, {uint(rng() % MOD), 1}); }
            else { sum = sum * 31 + st.prod(k, l, r).sum; }}
        emit("q3-setprod", variant, n, q, since(t0), sum, st.nodeCount() - applied);}

    // Q4: PersistentValueTree against WaveletMatrix on n random values in [0, 10^9).
    void valueTree(ulng seed, int n, int q) {
        std::mt19937_64 rng(seed);
        vector<int> a(n);
        vector<lng> w(n);
        for (int i = 0; i < n; ++i) { a[i] = int(rng() % 1000000000); w[i] = a[i]; }
        vector<std::array<int, 4>> qs(q);
        for (auto &[l, r, k, x] : qs) {
            l = int(rng() % ulng(n)); r = int(rng() % ulng(n));
            if (l > r) { swap(l, r); }
            ++r;
            k = int(rng() % ulng(r - l)); x = int(rng() % 1000000000);}
        auto t0 = Clock::now();
        PersistentValueTree<int> plain(a);
        emit("q4-build", "PVT", n, 0, since(t0), ulng(plain.rangeKth(0, n, n / 2)), plain.t.size() - 1);
        t0 = Clock::now();
        PersistentValueTree<int> pvt(a, w);
        emit("q4-build", "PVT-weighted", n, 0, since(t0), ulng(pvt.rangeKth(0, n, n / 2)), pvt.t.size() - 1);
        t0 = Clock::now();
        WaveletMatrix<int> wm(a);
        emit("q4-build", "WM", n, 0, since(t0), ulng(wm.kth(0, n, n / 2)), 0);
        ulng sum = 0;
        t0 = Clock::now();
        for (auto [l, r, k, x] : qs) { sum = sum * 31 + ulng(pvt.rangeKth(l, r, k)); }
        emit("q4-kth", "PVT", n, q, since(t0), sum, 0);
        sum = 0; t0 = Clock::now();
        for (auto [l, r, k, x] : qs) { sum = sum * 31 + ulng(wm.kth(l, r, k)); }
        emit("q4-kth", "WM", n, q, since(t0), sum, 0);
        sum = 0; t0 = Clock::now();
        for (auto [l, r, k, x] : qs) { sum = sum * 31 + ulng(pvt.countRange(l, r, min(k, x), max(k, x))); }
        emit("q4-countRange", "PVT", n, q, since(t0), sum, 0);
        sum = 0; t0 = Clock::now();
        for (auto [l, r, k, x] : qs) { sum = sum * 31 + ulng(pvt.kthSum(l, r, k)); }
        emit("q4-kthSum", "PVT", n, q, since(t0), sum, 0);}

    // Q4: Library Checker static_range_count_distinct shape, values in [0, n).
    void distinct(ulng seed, int n, int q) {
        std::mt19937_64 rng(seed);
        vector<int> a(n);
        for (auto &x : a) { x = int(rng() % ulng(n)); }
        vector<pair<int, int>> qs(q);
        for (auto &[l, r] : qs) {
            l = int(rng() % ulng(n + 1)); r = int(rng() % ulng(n + 1));
            if (l > r) { swap(l, r); }}
        auto t0 = Clock::now();
        PersistentDistinctCount<int> d(a);
        double buildSec = since(t0);
        ulng sum = 0;
        t0 = Clock::now();
        for (auto [l, r] : qs) { sum = sum * 31 + ulng(d.countDistinct(l, r)); }
        double querySec = since(t0);
        emit("q4-distinct-build", "PDC", n, 0, buildSec, ulng(d.countDistinct(0, n)), d.tree.t.size() - 1);
        emit("q4-distinct-query", "PDC", n, q, querySec, sum, 0);}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261027;
    const int n = 200000, q = 200000;
    for (int latest = 0; latest < 2; ++latest) {
        for (int pct : {50, 100}) {
            ulng s = seed + 2 * ulng(latest) + (pct == 100);
            lazyAdd<false>(s, n, q, latest, pct); lazyAdd<true>(s, n, q, latest, pct);
            dualAdd<false>(s + 10, n, q, latest, pct); dualAdd<true>(s + 10, n, q, latest, pct);}}
    affineSkip<FE>(seed + 20, n, 20000, q, "with=="); affineSkip<FN>(seed + 20, n, 20000, q, "without==");
    valueTree(seed + 30, n, q);
    distinct(seed + 40, 500000, 500000);
    return 0;}
