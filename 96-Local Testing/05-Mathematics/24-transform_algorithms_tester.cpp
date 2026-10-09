#include "../../05-Mathematics/24-transform_algorithms.hpp"
#include "../../01-Core/05-modint.hpp"

lng checks = 0;
std::mt19937_64 rng;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
template<typename T> vector<T> rnd(int n) {
    vector<T> v(n);
    for (auto &x : v) {
        if constexpr (std::is_integral_v<T>) { x = T(lng(rng() % 61) - 30); }
        else { x = T(rng()); }}
    return v;}
template<typename T> vector<T> bruteSubset(const vector<T> &a, bool super) {
    int N = int(a.size());
    vector<T> r(N, T(0));
    for (int s = 0; s < N; ++s) { for (int t = 0; t < N; ++t) { if (super ? (s & t) == s : (s & t) == t) { r[s] = r[s] + a[t]; } } }
    return r;}
template<typename T> vector<T> bruteWht(const vector<T> &a) {
    int N = int(a.size());
    vector<T> r(N, T(0));
    for (int s = 0; s < N; ++s) { for (int t = 0; t < N; ++t) { r[s] = std::popcount(uint(s & t)) & 1 ? r[s] - a[t] : r[s] + a[t]; } }
    return r;}
template<typename T, typename F> vector<T> bruteBinary(const vector<T> &a, const vector<T> &b, int N, F op) {
    vector<T> r(N, T(0));
    for (int i = 0; i < int(a.size()); ++i) {
        for (int j = 0; j < int(b.size()); ++j) {
            int k = op(i, j);
            if (k >= 0 && k < N) { r[k] = r[k] + a[i] * b[j]; }}}
    return r;}
template<typename T> vector<T> bruteDivisor(const vector<T> &a, bool multiple) {
    int n = int(a.size()) - 1;
    vector<T> r = a;
    for (int k = 1; k <= n; ++k) {
        r[k] = T(0);
        for (int d = 1; d <= n; ++d) { if (multiple ? d % k == 0 : k % d == 0) { r[k] = r[k] + a[d]; } }}
    return r;}
template<typename T> vector<T> bruteSubsetConv(const vector<T> &a, const vector<T> &b) {
    int N = int(a.size());
    vector<T> r(N, T(0));
    for (int s = 0; s < N; ++s) { for (int t = s;; t = (t - 1) & s) { r[s] = r[s] + a[t] * b[s ^ t]; if (!t) { break; } } }
    return r;}

template<typename T>
void bitSuite(const string &name, int maxLg) {
    for (int lg = 0; lg <= maxLg; ++lg) {
        int N = 1 << lg;
        auto a = rnd<T>(N), b = rnd<T>(N), x = a;
        string tag = name + " N=" + std::to_string(N);
        subsetZeta(x); check(x == bruteSubset(a, false), "subsetZeta " + tag);
        subsetMobius(x); check(x == a, "subsetMobius " + tag);
        x = bruteSubset(a, false); subsetMobius(x); check(x == a, "subsetMobius brute image " + tag);
        x = a; supersetZeta(x); check(x == bruteSubset(a, true), "supersetZeta " + tag);
        supersetMobius(x); check(x == a, "supersetMobius " + tag);
        x = a; walshHadamard(x); check(x == bruteWht(a), "walshHadamard " + tag);
        walshHadamard(x, true); check(x == a, "walshHadamard inverse " + tag);
        x = a; FastConv<T>::fctOr(x, false); check(x == bruteSubset(a, false), "fctOr " + tag);
        FastConv<T>::fctOr(x, true); check(x == a, "fctOr inverse " + tag);
        x = a; FastConv<T>::fctAnd(x, false); check(x == bruteSubset(a, true), "fctAnd " + tag);
        FastConv<T>::fctAnd(x, true); check(x == a, "fctAnd inverse " + tag);
        x = a; FastConv<T>::fctXor(x, false); check(x == bruteWht(a), "fctXor " + tag);
        FastConv<T>::fctXor(x, true); check(x == a, "fctXor inverse " + tag);
        check(orConvolution(a, b) == bruteBinary(a, b, N, [](int i, int j) { return i | j; }), "orConvolution " + tag);
        check(andConvolution(a, b) == bruteBinary(a, b, N, [](int i, int j) { return i & j; }), "andConvolution " + tag);
        check(xorConvolution(a, b) == bruteBinary(a, b, N, [](int i, int j) { return i ^ j; }), "xorConvolution " + tag);
        check(xorConvolution(a, a) == bruteBinary(a, a, N, [](int i, int j) { return i ^ j; }), "xorConvolution aliasing " + tag);
        check(subsetConvolution(a, b) == bruteSubsetConv(a, b), "subsetConvolution " + tag);
        auto z = rankedZeta(a);
        check(int(z.size()) == lg + 1, "rankedZeta size " + tag);
        for (int k = 0; k <= lg; ++k) {
            for (int s = 0; s < N; ++s) {
                T e = T(0);
                for (int t = 0; t < N; ++t) { if ((s & t) == t && std::popcount(uint(t)) == k) { e = e + a[t]; } }
                check(z[k][s] == e, "rankedZeta " + tag);}}
        check(rankedMobius(z) == a, "rankedMobius " + tag);
        auto c = rnd<T>(N);
        check(disjointUnionConvolution(vector<vector<T>>{a}) == a, "disjointUnionConvolution q=1 " + tag);
        check(disjointUnionConvolution(vector<vector<T>>{a, b}) == bruteSubsetConv(a, b), "disjointUnionConvolution q=2 " + tag);
        check(disjointUnionConvolution(vector<vector<T>>{a, b, c}) == bruteSubsetConv(bruteSubsetConv(a, b), c), "disjointUnionConvolution q=3 " + tag);
        x = a; kroneckerPowerTransform(x, {{T(1), T(0)}, {T(1), T(1)}}); check(x == bruteSubset(a, false), "kronecker subset kernel " + tag);
        x = a; kroneckerPowerTransform(x, {{T(1), T(1)}, {T(1), T(0) - T(1)}}); check(x == bruteWht(a), "kronecker Hadamard kernel " + tag);}
    cout << "PASS " << name << ": subset/superset zeta and Mobius, walshHadamard, or/and/xor/subset/disjoint-union convolutions, ranked transforms, FastConv bitwise\n";}

template<typename T>
void kroneckerSuite(const string &name) {
    for (int D = 1; D <= 4; ++D) {
        for (int k = 0; k <= (D == 1 ? 0 : 9 / D); ++k) {
            int N = 1;
            for (int i = 0; i < k; ++i) { N *= D; }
            vector<vector<T>> K(D);
            for (auto &r : K) { r = rnd<T>(D); }
            auto a = rnd<T>(N), x = a;
            kroneckerPowerTransform(x, K);
            vector<T> e(N, T(0));
            for (int r = 0; r < N; ++r) {
                for (int c = 0; c < N; ++c) {
                    T w = T(1);
                    for (int i = 0, rr = r, cc = c; i < k; ++i, rr /= D, cc /= D) { w = w * K[rr % D][cc % D]; }
                    e[r] = e[r] + w * a[c];}}
            check(x == e, name + " kroneckerPowerTransform D=" + std::to_string(D) + " k=" + std::to_string(k));}}
    cout << "PASS " << name << ": kroneckerPowerTransform D = 1..4 against the dense Kronecker power\n";}

template<typename T>
void numberSuite(const string &name, int maxN, int big) {
    for (int n = -1; n <= maxN; ++n) {
        auto a = rnd<T>(n + 1), x = a;
        string tag = name + " n=" + std::to_string(n);
        x = a; divisorZeta(x); check(x == bruteDivisor(a, false), "divisorZeta " + tag);
        divisorMobius(x); check(x == a, "divisorMobius " + tag);
        x = a; multipleZeta(x); check(x == bruteDivisor(a, true), "multipleZeta " + tag);
        multipleMobius(x); check(x == a, "multipleMobius " + tag);
        LinearSieve sv(max(n, 0));
        x = a; FastConv<T>::fctLcm(x, false, sv); check(x == bruteDivisor(a, false), "fctLcm " + tag);
        FastConv<T>::fctLcm(x, true, sv); check(x == a, "fctLcm inverse " + tag);
        x = a; FastConv<T>::fctGcd(x, false, sv); check(x == bruteDivisor(a, true), "fctGcd " + tag);
        FastConv<T>::fctGcd(x, true, sv); check(x == a, "fctGcd inverse " + tag);
        x = a; FastConv<T>::fctLcmSlow(x, false); check(x == bruteDivisor(a, false), "fctLcmSlow " + tag);
        FastConv<T>::fctLcmSlow(x, true); check(x == a, "fctLcmSlow inverse " + tag);
        x = a; FastConv<T>::fctGcdSlow(x, false); check(x == bruteDivisor(a, true), "fctGcdSlow " + tag);
        FastConv<T>::fctGcdSlow(x, true); check(x == a, "fctGcdSlow inverse " + tag);
        for (int m : {0, 1, 2, max(n + 1, 0), n + 7, max(3 * n + 2, 0)}) {
            auto b = rnd<T>(m);
            int sa = int(a.size());
            auto g = bruteBinary(a, b, min(sa, m), [](int i, int j) { return i && j ? std::gcd(i, j) : -1; });
            auto l = bruteBinary(a, b, max(sa, m), [](int i, int j) { return i && j && lng(i) * j / std::gcd(i, j) < 1000000 ? int(lng(i) * j / std::gcd(i, j)) : -1; });
            check(gcdConvolution(a, b) == g, "gcdConvolution " + tag + " m=" + std::to_string(m));
            check(lcmConvolution(a, b) == l, "lcmConvolution " + tag + " m=" + std::to_string(m));}}
    auto a = rnd<T>(big + 1), b = rnd<T>(big + 1), x = a;
    divisorZeta(x); divisorMobius(x); check(x == a, name + " divisor round trip big");
    multipleZeta(x); multipleMobius(x); check(x == a, name + " multiple round trip big");
    x = a; divisorZeta(x);
    for (int k : {1, 2, big, big - 1, big / 2, 720720 % (big + 1)}) {
        if (k < 1) { continue; }
        T e = T(0);
        for (int d = 1; d * d <= k; ++d) { if (k % d == 0) { e = e + a[d]; if (d * d != k) { e = e + a[k / d]; } } }
        check(x[k] == e, name + " divisorZeta big k=" + std::to_string(k));}
    auto g = gcdConvolution(a, b);
    for (int k : {big / 7, big / 3, big / 2, big}) {
        T e = T(0);
        for (int i = k; i <= big; i += k) { for (int j = k; j <= big; j += k) { if (std::gcd(i, j) == k) { e = e + a[i] * b[j]; } } }
        check(g[k] == e, name + " gcdConvolution big k=" + std::to_string(k));}
    cout << "PASS " << name << ": divisor/multiple zeta and Mobius, gcd/lcm convolutions, FastConv gcd/lcm (fast and Slow)\n";}

int main(int argc, char **argv) {
    using M = ModInt<998244353>;
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        vector<lng> three(3), four(4), two(2);
        if (s == "zeta-size") { subsetZeta(three); }
        if (s == "wht-empty") { vector<lng> e; walshHadamard(e); }
        if (s == "or-size") { orConvolution(four, two); }
        if (s == "xor-size") { xorConvolution(two, four); }
        if (s == "kronecker-square") { kroneckerPowerTransform(four, {{1, 0}, {1}}); }
        if (s == "kronecker-power") { kroneckerPowerTransform(three, {{1, 0}, {1, 1}}); }
        if (s == "kronecker-empty") { kroneckerPowerTransform(three, {}); }
        if (s == "ranked-size") { rankedZeta(three); }
        if (s == "ranked-mobius") { rankedMobius(vector<vector<lng>>{four, four}); }
        if (s == "disjoint-empty") { disjointUnionConvolution(vector<vector<lng>>{}); }
        if (s == "disjoint-size") { disjointUnionConvolution(vector<vector<lng>>{four, two}); }
        if (s == "subset-size") { subsetConvolution(four, two); }
        if (s == "gcd-sieve") { LinearSieve sv(5); vector<lng> v(10); FastConv<lng>::fctGcd(v, false, sv); }
        if (s == "lcm-sieve") { LinearSieve sv(5); vector<lng> v(10); FastConv<lng>::fctLcm(v, false, sv); }
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    rng.seed(seed);
    bool quick = mode == "quick", stress = mode == "stress";
    int maxLg = quick ? 6 : stress ? 10 : 9, maxN = quick ? 40 : stress ? 300 : 150, big = quick ? 20000 : stress ? 3000000 : 1000000;

    bitSuite<lng>("lng", maxLg);
    bitSuite<M>("mint", maxLg);
    bitSuite<ModInt<1000000007>>("1e9+7", maxLg - 2);
    {
        vector<double> a{1.5, -2, 0.25, 8}, x = a;
        walshHadamard(x); walshHadamard(x, true);
        for (int i = 0; i < 4; ++i) { check(std::abs(x[i] - a[i]) < 1e-12, "walshHadamard double"); }
        vector<lll> w{lll(1) << 100, -(lll(1) << 99), 3, -5};
        auto y = w;
        walshHadamard(y); walshHadamard(y, true);
        check(y == w, "walshHadamard __int128 exact");
        vector<ulng> uw{0, ~0ULL, 5, ulng(-7), ulng(1) << 59, 3, ulng(-(1LL << 58)), 0}, uy = uw;
        walshHadamard(uy); walshHadamard(uy, true);
        check(uy == uw, "walshHadamard ulng two's complement inverse");
        check(xorConvolution(vector<ulng>{0, ~0ULL}, vector<ulng>{1, 0}) == vector<ulng>{0, ~0ULL}, "xorConvolution ulng negative result");
        vector<uint> vw{7, ~0U, 0, 1U << 20}, vy = vw;
        walshHadamard(vy); walshHadamard(vy, true);
        check(vy == vw, "walshHadamard uint two's complement inverse");
        vector<ulng> u(1 << 10);
        for (auto &v : u) { v = rng(); }
        auto c = u;
        subsetZeta(c); subsetMobius(c);
        check(c == u, "subset round trip with unsigned wraparound");
        cout << "PASS walshHadamard over double and __int128, unsigned wraparound\n";}
    if (!quick) {
        int lg = stress ? 20 : 16, N = 1 << lg;
        auto a = rnd<M>(N), b = rnd<M>(N);
        auto c = subsetConvolution(a, b), d = orConvolution(a, b), e = xorConvolution(a, b);
        for (int rep = 0; rep < 12; ++rep) {
            int s = rep < 2 ? (rep ? N - 1 : 0) : int(rng() % ulng(N));
            if (std::popcount(uint(s)) > 14) { s &= (1 << 14) - 1; }
            M cs = 0;
            for (int t = s;; t = (t - 1) & s) {
                cs += a[t] * b[s ^ t];
                if (!t) { break; }}
            check(c[s] == cs, "subsetConvolution large N=" + std::to_string(N) + " s=" + std::to_string(s));
            M sa = 0, sb = 0;
            for (int t = s;; t = (t - 1) & s) {
                sa += a[t]; sb += b[t];
                if (!t) { break; }}
            M sd = 0;
            for (int t = s;; t = (t - 1) & s) {
                sd += d[t];
                if (!t) { break; }}
            check(sd == sa * sb, "orConvolution large zeta identity s=" + std::to_string(s));}
        M sa = 0, sb = 0, se = 0;
        for (int i = 0; i < N; ++i) { sa += a[i]; sb += b[i]; se += e[i]; }
        check(se == sa * sb, "xorConvolution large sum identity");
        cout << "PASS large N = 2^" << lg << " subset/or/xor convolutions against submask brute and identities\n";}

    kroneckerSuite<lng>("lng");
    kroneckerSuite<M>("mint");
    numberSuite<lng>("lng", maxN, big / 10);
    numberSuite<M>("mint", maxN, big);
    cout << "PASS all, " << checks << " checks\n";}
