#include "../../05-Mathematics/14-polynomial_algorithms.hpp"
#include "../../05-Mathematics/15-linear_recurrence.hpp"

template<typename F>
double timeMs(F f) {
    auto t = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count();}
std::mt19937_64 rng(20261010);
template<typename M> vector<M> rnd(int n) {
    vector<M> v(n);
    for (auto &x : v) { x = M(rng()); }
    if (n && v[0] == M(0)) { v[0] = 1; }
    return v;}
// The two branches of polyDivmod and taylorShift, without their cutoffs.
template<typename M> vector<M> divNaive(vector<M> a, const vector<M> &b) {
    int m = int(b.size()), k = int(a.size()) - m + 1;
    vector<M> q(k);
    M c = inv(b.back());
    for (int i = k - 1; i >= 0; --i) {
        q[i] = a[i + m - 1] * c;
        for (int j = 0; j + 1 < m; ++j) { a[i + j] -= q[i] * b[j]; }}
    return q;}
template<typename M> vector<M> divNewton(const vector<M> &a, const vector<M> &b) {
    int m = int(b.size()), k = int(a.size()) - m + 1;
    auto q = truncatedConvolution(vector<M>(a.rbegin(), a.rbegin() + k), fpsInverse(vector<M>(b.rbegin(), b.rend()), k), k);
    reverse(q.begin(), q.end());
    return q;}
template<typename M> vector<M> shiftNaive(vector<M> a, M c) {
    int n = int(a.size());
    for (int i = 0; i + 1 < n; ++i) { for (int j = n - 2; j >= i; --j) { a[j] += c * a[j + 1]; } }
    return a;}
template<typename M> vector<M> shiftFast(vector<M> a, M c) {
    int n = int(a.size());
    vector<M> f(n, M(1)), g(n), e(n);
    for (int i = 1; i < n; ++i) { f[i] = f[i - 1] * M(i); }
    g[n - 1] = inv(f[n - 1]);
    for (int i = n - 1; i > 0; --i) { g[i - 1] = g[i] * M(i); }
    M w = 1;
    for (int i = 0; i < n; ++i, w *= c) { a[i] *= f[i]; e[i] = w * g[i]; }
    reverse(a.begin(), a.end());
    auto b = truncatedConvolution(a, e, n);
    reverse(b.begin(), b.end());
    for (int i = 0; i < n; ++i) { b[i] *= g[i]; }
    return b;}
vector<string> rows;
int reps = 5;
template<typename FA, typename FB>
void pairRow(const string &name, FA fa, FB fb, const string &la, const string &lb) {
    vector<double> ta, tb;
    for (int r = 0; r <= reps; ++r) {
        decltype(fa()) x;
        decltype(fb()) y;
        double t1 = timeMs([&] { x = fa(); }), t2 = timeMs([&] { y = fb(); });
        if (!(x == y)) { cerr << "MISMATCH " << name << endl; exit(1); }
        if (r) { ta.pb(t1); tb.pb(t2); }}
    sort(ta.begin(), ta.end()); sort(tb.begin(), tb.end());
    rows.pb("{\"workload\": \"" + name + "\", \"" + la + "_ms\": " + std::to_string(ta[reps / 2]) + ", \"" + lb + "_ms\": " + std::to_string(tb[reps / 2]) + "}");}
template<typename F>
void single(const string &name, F f) {
    vector<double> t;
    decltype(f()) first{};
    for (int r = 0; r <= reps; ++r) {
        decltype(f()) x;
        double ms = timeMs([&] { x = f(); });
        if (!r) { first = x; }
        else if (!(x == first)) { cerr << "UNSTABLE " << name << endl; exit(1); }
        if (r) { t.pb(ms); }}
    sort(t.begin(), t.end());
    rows.pb("{\"workload\": \"" + name + "\", \"ms\": " + std::to_string(t[reps / 2]) + "}");}

template<typename M>
void thresholds(const string &mod, bool ntt) {
    auto pick = [&](vector<int> a, vector<int> b) { return ntt ? a : b; };
    for (int m : pick({128, 192, 256, 320, 384}, {256, 512, 768, 1024, 1536, 2048})) {
        auto a = rnd<M>(2 * m), b = rnd<M>(m);
        int k = max(1, 4096 / m);
        pairRow(mod + " divmod 2m / m, m=" + std::to_string(m) + " (x" + std::to_string(k) + ")", [&] { vector<M> q; for (int i = 0; i < k; ++i) { q = divNaive(a, b); } return q; },
            [&] { vector<M> q; for (int i = 0; i < k; ++i) { q = divNewton(a, b); } return q; }, "naive", "newton");}
    for (int n : pick({32, 64, 96, 128, 160, 192, 256, 320, 384}, {256, 384, 512, 768, 1024, 1536, 2048})) {
        auto a = rnd<M>(n);
        int k = max(1, 8192 / n);
        a[0] = 1;
        pairRow(mod + " inverse n=" + std::to_string(n) + " (x" + std::to_string(k) + ")", [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = fpsSparseInverse(a, n); } return r; },
            [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = polynomial_detail::invNewton(a, n); } return r; }, "naive", "newton");
        pairRow(mod + " log n=" + std::to_string(n) + " (x" + std::to_string(k) + ")", [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = fpsSparseLog(a, n); } return r; },
            [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = polynomial_detail::logNewton(a, n); } return r; }, "naive", "newton");
        pairRow(mod + " sqrt n=" + std::to_string(n) + " (x" + std::to_string(k) + ")", [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = fpsSparseSqrt(a, n); } return r; },
            [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = polynomial_detail::sqrtNewton(a, M(1), n); } return r; }, "naive", "newton");
        pairRow(mod + " pow k=1e18 n=" + std::to_string(n) + " (x" + std::to_string(k) + ")", [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = fpsSparsePow(a, lng(1e18), n); } return r; },
            [&] { vector<M> r; for (int i = 0; i < k; ++i) { auto l = fpsLog(a, n); for (auto &x : l) { x *= M(lng(1e18)); } r = fpsExp(l, n); } return r; }, "naive", "logexp");
        a[0] = 0;
        pairRow(mod + " exp n=" + std::to_string(n) + " (x" + std::to_string(k) + ")", [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = fpsSparseExp(a, n); } return r; },
            [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = polynomial_detail::expNewton(a, n); } return r; }, "naive", "newton");}
    for (int n : pick({16, 32, 48, 64, 96, 128}, {64, 96, 128, 192, 256})) {
        auto a = rnd<M>(n);
        M c = M(rng());
        int k = max(1, 8192 / n);
        pairRow(mod + " taylorShift n=" + std::to_string(n) + " (x" + std::to_string(k) + ")", [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = shiftNaive(a, c); } return r; },
            [&] { vector<M> r; for (int i = 0; i < k; ++i) { r = shiftFast(a, c); } return r; }, "naive", "fast");}
    {
        // Half-GCD recursion base at a large degree (half-GCD from the top), then plain Euclid against half-GCD by degree.
        int n = ntt ? 32768 : 16384;
        auto a = rnd<M>(n + 1), b = rnd<M>(n);
        a.back() = 1; b.back() = 1;
        for (int base : pick({32, 64, 128, 256, 512, 1024}, {128, 256, 512, 1024, 2048, 4096})) {
            single(mod + " gcd deg " + std::to_string(n) + " half-GCD base " + std::to_string(base), [&] { return polynomial_detail::euclid<M>(a, b, nullptr, nullptr, 0, base); });}}
    for (int n : pick({256, 512, 1024, 2048, 3072, 4096, 6144}, {1024, 2048, 4096, 8192, 12288, 16384})) {
        auto a = rnd<M>(n + 1), b = rnd<M>(n);
        a.back() = 1; b.back() = 1;
        int base = polynomial_detail::cut<M>(polynomial_detail::HGCD);
        pairRow(mod + " gcd deg n, n - 1, n=" + std::to_string(n), [&] { return polynomial_detail::euclid<M>(a, b, nullptr, nullptr, 1 << 30, base); },
            [&] { return polynomial_detail::euclid<M>(a, b, nullptr, nullptr, 0, base); }, "euclid", "halfgcd");}
    for (int n : pick({512, 1024, 1536, 2048, 4096}, {2048, 4096, 8192, 12288, 16384})) {
        auto s = rnd<M>(n);
        pairRow(mod + " find recurrence n=" + std::to_string(n), [&] { return berlekampMassey(s).size(); }, [&] { return findLinearRecurrenceHalfGcd(s).size(); }, "bm", "halfgcd");}}

int main(int argc, char **argv) {
    reps = argc > 1 ? std::stoi(argv[1]) : 5;
    using M = mint;
    thresholds<M>("mint", true);
    thresholds<ModInt<1000000007>>("1e9+7", false);
    const int N = 500000;
    auto a = rnd<M>(N);
    a[0] = 1;
    single("mint fpsInverse 5e5", [&] { return fpsInverse(a, N); });
    single("mint fpsLog 5e5", [&] { return fpsLog(a, N); });
    single("mint fpsSqrt 5e5", [&] { return fpsSqrt(a, N); });
    single("mint fpsPow k=1e18 5e5", [&] { return fpsPow(a, lng(1e18), N); });
    a[0] = 0;
    single("mint fpsExp 5e5", [&] { return fpsExp(a, N); });
    auto b = rnd<M>(N / 2);
    b.back() = 1;
    single("mint polyDivmod 5e5 / 2.5e5", [&] { return polyDivmod(a, b); });
    single("mint taylorShift 5e5", [&] { return taylorShift(a, M(12345)); });
    single("mint polyFromRoots 1e5", [&] { return polyFromRoots(vector<M>(a.begin(), a.begin() + 100000)); });
    auto g = rnd<M>(50001), h = rnd<M>(50000);
    g.back() = 1;
    single("mint polyXgcd deg 5e4", [&] { auto r = polyXgcd(g, h); return r.x; });
    single("mint polyResultant deg 5e4", [&] { return polyResultant(g, h); });
    single("mint polyPowMod e=1e18 deg 1e5", [&] { return polyPowMod(vector<M>{0, 1}, ulng(1e18), vector<M>(a.begin(), a.begin() + 100001)); });
    auto f = rnd<M>(8000), u = rnd<M>(8000);
    u[0] = 0;
    single("mint fpsComposition 8000", [&] { return fpsComposition(f, u, 8000); });
    u[1] = 1;
    single("mint fpsCompositionalInverse 8000", [&] { return fpsCompositionalInverse(u, 8000); });
    auto c = rnd<M>(100000), init = rnd<M>(100000);
    single("mint nthTermLinearRecurrence d=1e5 k=1e18", [&] { return nthTermLinearRecurrence(c, init, ulng(1e18)); });
    single("mint consecutiveTerms d=1e5 k=1e18 m=1e5", [&] { return consecutiveTerms(c, init, ulng(1e18), 100000); });
    auto s = rnd<M>(10000);
    single("mint berlekampMassey n=1e4", [&] { return berlekampMassey(s); });
    auto t = rnd<M>(200000);
    single("mint findLinearRecurrenceHalfGcd n=2e5", [&] { return findLinearRecurrenceHalfGcd(t); });
    cout << "[\n";
    for (int i = 0; i < int(rows.size()); ++i) { cout << "  " << rows[i] << (i + 1 < int(rows.size()) ? ",\n" : "\n"); }
    cout << "]\n";}
