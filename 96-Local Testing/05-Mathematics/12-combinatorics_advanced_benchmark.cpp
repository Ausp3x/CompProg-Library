#include "../../05-Mathematics/12-combinatorics_advanced.hpp"

// Library Checker sizes for the FPS rows and query structures; bellNumberLarge against right-to-left binary powering (one more product per set bit).
template<typename M>
M bellRightToLeft(ulng n) {
    int p = int(M::mod());
    auto b = bellTable<M>(p - 1);
    auto reduce = [&](vector<M> a) {
        for (int d = int(a.size()) - 1; d >= p; --d) { a[d - p + 1] += a[d]; a[d - p] += a[d]; }
        a.resize(p); return a;};
    vector<M> r{1}, x{0, 1};
    for (; n; n >>= 1, x = reduce(convolution(x, x))) { if (n & 1) { r = reduce(convolution(r, x)); } }
    M res = 0;
    for (int i = 0; i < p; ++i) { res += r[i] * b[i]; }
    return res;}

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
    int reps = argc > 1 ? std::atoi(argv[1]) : 5, N = 500000;
    std::mt19937_64 rng(1);
    vector<int> s(N);
    for (auto &x : s) { x = 1 + int(rng() % ulng(N)); }
    vector<mint> tr(100000);
    for (auto &x : tr) { x = mint(rng()); }
    vector<pair<int, int>> qs(100000);
    for (auto &[n, k] : qs) { n = int(rng() % 100001); k = int(rng() % 100001); }
    vector<pair<string, std::function<ulng()>>> work = {
        {"stirling1Row n=5e5", [&] { return ulng(stirling1Row(N)[N / 2].val()); }},
        {"stirling2Row n=5e5", [&] { return ulng(stirling2Row(N)[N / 2].val()); }},
        {"stirling1Column n=5e5 k=1000", [&] { return ulng(stirling1Column(N, 1000)[N].val()); }},
        {"stirling2Column n=5e5 k=1000", [&] { return ulng(stirling2Column(N, 1000)[N].val()); }},
        {"bellTable n=5e5", [&] { return ulng(bellTable(N)[N].val()); }},
        {"partitionCount n=5e5", [&] { return ulng(partitionCount(N)[N].val()); }},
        {"partitionTable n=1e5", [&] { return ulng(partitionTable<mint>(100000)[100000].val()); }},
        {"subsetSumCount n=t=5e5", [&] { return ulng(subsetSumCount(s, N)[N].val()); }},
        {"stirlingTransform n=1e5 (random)", [&] { return ulng(stirlingTransform(tr)[99999].val()); }},
        {"inverseStirlingTransform n=1e5 (random)", [&] { return ulng(inverseStirlingTransform(tr)[99999].val()); }},
        {"BinomialCompositeMod m=1e6, 2e5 queries", [&] {
            BinomialCompositeMod b(1000000); std::mt19937_64 g(2); ulng h = 0;
            for (int i = 0; i < 200000; ++i) { ulng n = g() % ulng(1e18); h += b(n, g() % (n + 1)); }
            return h;}},
        {"binomialRowCompositeMod N=1e7 m=1e18+9", [&] { return binomialRowCompositeMod(10000000, 1000000000000000009ULL)[5000000]; }},
        {"binomialPrefixSumQueries N=q=1e5", [&] { return ulng(binomialPrefixSumQueries(qs, ModFac(100000))[0].val()); }},
        {"StirlingSmallPrime p=4999, 2e5 queries", [&] {
            StirlingSmallPrime sp(4999); std::mt19937_64 g(3); ulng h = 0;
            for (int i = 0; i < 200000; ++i) { ulng n = g() % ulng(1e18), k = g() % (n + 1); h += sp.stirling1(n, k) + sp.stirling2(n, k); }
            return h;}},
        {"QCombinatorics N=1e7, 1e6 queries", [&] {
            QCombinatorics<mint> qc(mint(3), 10000000); std::mt19937_64 g(4); ulng h = 0;
            for (int i = 0; i < 1000000; ++i) { int n = int(g() % 10000001); h += qc.qBinomial(n, int(g() % ulng(n + 1))).val(); }
            return h;}},
        {"bellNumberLarge p=10007 n=1e18 (shipped)", [&] { return ulng(bellNumberLarge<ModInt<10007>>(1000000000000000000ULL).val()); }},
        {"bellNumberLarge p=10007 n=1e18 (right-to-left)", [&] { return ulng(bellRightToLeft<ModInt<10007>>(1000000000000000000ULL).val()); }},
    };
    cout << "[";
    for (size_t i = 0; i < work.size(); ++i) {
        ulng check = 0;
        double ms = median(reps, work[i].second, check);
        cout << (i ? ",\n" : "\n") << "{\"workload\": \"" << work[i].first << "\", \"median_ms\": " << ms << ", \"check\": " << check << "}";}
    cout << "\n]\n";
    if (bellNumberLarge<ModInt<10007>>(123456789123456789ULL) != bellRightToLeft<ModInt<10007>>(123456789123456789ULL)) {
        cerr << "bellNumberLarge kernels disagree\n"; return 1;}}
