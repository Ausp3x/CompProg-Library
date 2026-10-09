#include "../../05-Mathematics/13-convolution.hpp"

lng checks = 0;
std::mt19937_64 rng;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
template<typename M> M pw(M a, ulng e) {
    M r = 1;
    for (; e; e >>= 1, a = a * a) { if (e & 1) { r = r * a; } }
    return r;}
template<typename M> vector<M> rnd(int n) {
    vector<M> v(n);
    for (auto &x : v) { x = M(rng()); }
    return v;}
template<typename T> vector<T> brute(const vector<T> &a, const vector<T> &b) {
    if (a.empty() || b.empty()) { return {}; }
    vector<T> c(a.size() + b.size() - 1, T(0));
    for (size_t i = 0; i < a.size(); ++i) { for (size_t j = 0; j < b.size(); ++j) { c[i + j] = c[i + j] + a[i] * b[j]; } }
    return c;}
template<typename T> T coefficient(const vector<T> &a, const vector<T> &b, lng k) {
    T s = T(0);
    for (lng i = max(lng(0), k - lng(b.size()) + 1); i <= min(k, lng(a.size()) - 1); ++i) { s = s + a[i] * b[k - i]; }
    return s;}
template<typename T> void sampled(const vector<T> &a, const vector<T> &b, const vector<T> &c, const string &name) {
    lng L = lng(a.size()) + lng(b.size()) - 1;
    check(lng(c.size()) == L, name + " size");
    vector<lng> ks{0, L - 1, L / 2, min<lng>(L - 1, lng(a.size()) - 1), min<lng>(L - 1, lng(b.size()))};
    for (int i = 0; i < 40; ++i) { ks.pb(lng(rng() % ulng(L))); }
    for (lng k : ks) { check(c[k] == coefficient(a, b, k), name + " coefficient k=" + std::to_string(k)); }}
int rev(int k, int lg) {
    int r = 0;
    for (int i = 0; i < lg; ++i) { r |= ((k >> i) & 1) << (lg - 1 - i); }
    return r;}
template<typename M> ulng smallestRoot(const vector<ulng> &qs) {
    ulng p = M::mod();
    for (ulng g = 2;; ++g) {
        bool ok = true;
        for (ulng q : qs) { ok = ok && pw(M(g), (p - 1) / q) != M(1); }
        if (ok) { return g; }}}
// Brute DFT at w_n^k = g^((p - 1) / n * k), stored at bit-reversed slot.
template<typename M> vector<M> bruteNtt(const vector<M> &a, ulng g, int L) {
    int n = int(a.size()), lg = std::countr_zero(uint(L));
    M w = pw(M(g), (M::mod() - 1) / ulng(L));
    vector<M> res(L);
    for (int k = 0; k < L; ++k) {
        M s = 0, wk = pw(w, ulng(k));
        for (int j = n - 1; j >= 0; --j) { s = s * wk + a[j]; }
        res[rev(k, lg)] = s;}
    return res;}

template<typename M>
void nttSuite(const string &name, const vector<ulng> &qs, int lgBrute, int lgBig) {
    const ulng p = M::mod(), g = smallestRoot<M>(qs);
    check(primitiveRootNtt(p) == g, name + " primitiveRootNtt");
    int lgMax = min(lgBig, std::countr_zero(p - 1));
    for (int lg : {0, 1, 2, 3, lgBrute, lgMax}) {
        if (lg > lgMax) { continue; }
        int n = 1 << lg;
        const auto &w = nttRootTable<M>(n);
        const auto &iw = nttRootTable<M>(n, true);
        check(int(w.size()) >= n && int(iw.size()) >= n, name + " nttRootTable size");
        for (int h = 1; h < n; h *= 2) {
            M z = pw(M(g), (p - 1) / (2 * ulng(h))), x = 1;
            check(pw(z, ulng(h)) == M(p - 1), name + " table primitive");
            for (int j = 0; j < h; ++j, x = x * z) { check(w[h + j] == x && iw[h + j] * x == M(1), name + " nttRootTable entry"); }}}
    for (int lg = 0; lg <= lgBrute; ++lg) {
        int n = 1 << lg;
        auto a = rnd<M>(n), x = a, y = a;
        ntt(x);
        check(x == bruteNtt(a, g, n), name + " ntt brute n=" + std::to_string(n));
        intt(x);
        check(x == a, name + " intt round trip n=" + std::to_string(n));
        transposedNtt(y);
        M w = pw(M(g), (p - 1) / ulng(n));
        for (int k = 0; k < n; ++k) {
            M s = 0;
            for (int j = 0; j < n; ++j) { s = s + pw(w, ulng(j) * ulng(k)) * a[rev(j, lg)]; }
            check(y[k] == s, name + " transposedNtt brute n=" + std::to_string(n));}
        if (lg < std::countr_zero(p - 1)) {
            x = a; ntt(x); nttDoubling(x);
            check(x == bruteNtt(a, g, 2 * n), name + " nttDoubling brute n=" + std::to_string(n));}}
    for (int lg : {lgBrute + 1, lgMax}) {
        if (lg > lgMax) { continue; }
        int n = 1 << lg;
        auto a = rnd<M>(n), b = rnd<M>(n), x = a, y = b;
        ntt(x); transposedNtt(y);
        M l = 0, r = 0;
        for (int i = 0; i < n; ++i) { l = l + x[i] * b[i]; r = r + a[i] * y[i]; }
        check(l == r, name + " transposedNtt duality n=" + std::to_string(n));
        intt(x);
        check(x == a, name + " intt round trip large");
        if (lg < std::countr_zero(p - 1)) {
            x = a; ntt(x); nttDoubling(x);
            y = a; y.resize(2 * n); ntt(y);
            check(x == y, name + " nttDoubling vs padded ntt");}}
    for (int n = 0; n <= 70; n += (n < 8 ? 1 : 7)) {
        for (int m : {0, 1, 2, 59, 60, 61, 62, 100}) {
            if (n + m - 1 > (1 << lgMax)) { continue; }
            auto a = rnd<M>(n), b = rnd<M>(m);
            check(convolutionNtt(a, b) == brute(a, b), name + " convolutionNtt n=" + std::to_string(n) + " m=" + std::to_string(m));
            check(convolutionNtt(b, a) == brute(a, b), name + " convolutionNtt swapped");}}
    for (int n : {61, 64, 65, 127, 300}) {
        if (2 * n - 1 > (1 << lgMax)) { continue; }
        auto a = rnd<M>(n);
        check(convolutionNtt(a, a) == brute(a, a), name + " convolutionNtt square n=" + std::to_string(n));}
    int L = 1 << lgMax, n = L / 2 + int(rng() % ulng(L / 4 + 1));
    auto a = rnd<M>(n), b = rnd<M>(L + 1 - n);
    if (min(a.size(), b.size()) > 0) { sampled(a, b, convolutionNtt(a, b), name + " convolutionNtt at length limit"); }
    cout << "PASS " << name << ": primitiveRootNtt, nttRootTable, ntt, intt, transposedNtt, nttDoubling, convolutionNtt\n";}

template<typename M>
void wrapperSuite(const string &name, int big) {
    for (int n = 0; n <= 70; n += (n < 6 ? 1 : 9)) {
        for (int m : {0, 1, 3, 60, 61, 70}) {
            auto a = rnd<M>(n), b = rnd<M>(m), c = brute(a, b);
            string tag = name + " n=" + std::to_string(n) + " m=" + std::to_string(m);
            check(convolution(a, b) == c, "convolution " + tag);
            for (int k : {0, 1, max(n + m - 1, 0), n + m + 3, int(rng() % 80)}) {
                auto t = truncatedConvolution(a, b, k), e = c;
                e.resize(k);
                check(t == e, "truncatedConvolution " + tag + " k=" + std::to_string(k));}
            auto r = crossCorrelation(a, b);
            check(int(r.size()) == (n && m ? n + m - 1 : 0), "crossCorrelation size " + tag);
            for (int k = 0; k < int(r.size()); ++k) {
                M s = 0;
                for (int j = 0; j < m; ++j) { if (0 <= k - m + 1 + j && k - m + 1 + j < n) { s = s + a[k - m + 1 + j] * b[j]; } }
                check(r[k] == s, "crossCorrelation " + tag);}
            if (m >= 1) {
                auto mp = middleProduct(a, b);
                vector<M> e = m <= n ? vector<M>(c.begin() + m - 1, c.begin() + n) : vector<M>();
                check(mp == e, "middleProduct " + tag);}}}
    for (int n : {1, 2, 3, 5, 8, 16, 61, 64, 100, 128, 1024}) {
        auto a = rnd<M>(n), b = rnd<M>(n), c = brute(a, b);
        vector<M> cy(n), ng(n);
        for (int i = 0; i < int(c.size()); ++i) {
            cy[i % n] = cy[i % n] + c[i];
            ng[i % n] = i < n ? ng[i % n] + c[i] : ng[i % n] - c[i];}
        check(cyclicConvolution(a, b) == cy, name + " cyclicConvolution n=" + std::to_string(n));
        check(negacyclicConvolution(a, b) == ng, name + " negacyclicConvolution n=" + std::to_string(n));}
    check(cyclicConvolution(vector<M>{}, vector<M>{}).empty() && negacyclicConvolution(vector<M>{}, vector<M>{}).empty(), name + " cyclic empty");
    for (int n : {big / 2 + 7, big}) {
        auto a = rnd<M>(n), b = rnd<M>(n / 3 + 61), c = convolution(a, b);
        sampled(a, b, c, name + " convolution large n=" + std::to_string(n));
        auto mp = middleProduct(a, b);
        for (int i = 0; i < 30; ++i) {
            int k = int(rng() % mp.size());
            check(mp[k] == c[k + b.size() - 1], name + " middleProduct large");}}
    // Tensor: brute over every pair of multi-indices.
    for (int d = 0; d <= 3; ++d) {
        for (int rep = 0; rep < 6; ++rep) {
            vector<int> da(d), db(d);
            int sa = 1, sb = 1;
            for (int i = 0; i < d; ++i) { da[i] = int(rng() % 4); db[i] = int(rng() % 4) + (rep == 0); sa *= da[i]; sb *= db[i]; }
            auto a = rnd<M>(sa), b = rnd<M>(sb), c = convolutionTensor(a, da, b, db);
            vector<int> dr(d);
            int R = 1;
            for (int i = 0; i < d; ++i) { dr[i] = da[i] + db[i] - 1; R *= dr[i]; }
            if (!sa || !sb) { check(c.empty(), name + " convolutionTensor empty"); continue; }
            vector<M> e(R);
            auto unflat = [&](int x, const vector<int> &dims) {
                vector<int> idx(d);
                for (int i = d - 1; i >= 0; --i) { idx[i] = x % dims[i]; x /= dims[i]; }
                return idx;};
            for (int x = 0; x < sa; ++x) {
                for (int y = 0; y < sb; ++y) {
                    auto ia = unflat(x, da), ib = unflat(y, db);
                    int pos = 0;
                    for (int i = 0; i < d; ++i) { pos = pos * dr[i] + ia[i] + ib[i]; }
                    e[pos] = e[pos] + a[x] * b[y];}}
            check(c == e, name + " convolutionTensor d=" + std::to_string(d));}}
    for (int rep = 0; rep < 20; ++rep) {
        int ha = int(rng() % 5), wa = int(rng() % 5), hb = int(rng() % 5) + 1, wb = int(rng() % 5) + 1;
        vector<vector<M>> a(ha), b(hb);
        for (auto &r : a) { r = rnd<M>(wa); }
        for (auto &r : b) { r = rnd<M>(wb); }
        auto c = convolution2d(a, b);
        if (!ha || !wa) { check(c.empty(), name + " convolution2d empty"); continue; }
        bool ok = int(c.size()) == ha + hb - 1;
        for (int i = 0; ok && i < ha + hb - 1; ++i) {
            ok = int(c[i].size()) == wa + wb - 1;
            for (int j = 0; ok && j < wa + wb - 1; ++j) {
                M s = 0;
                for (int x = 0; x < ha; ++x) { for (int y = 0; y < wa; ++y) { if (0 <= i - x && i - x < hb && 0 <= j - y && j - y < wb) { s = s + a[x][y] * b[i - x][j - y]; } } }
                ok = c[i][j] == s;}}
        check(ok, name + " convolution2d brute");}
    auto big2 = convolution2d(vector<vector<M>>(40, rnd<M>(90)), vector<vector<M>>(30, rnd<M>(70)));
    check(big2.size() == 69 && big2[0].size() == 159, name + " convolution2d shape");
    cout << "PASS " << name << ": convolution, truncatedConvolution, crossCorrelation, middleProduct, cyclicConvolution, negacyclicConvolution, convolutionTensor, convolution2d\n";}

template<typename M>
void largeSuite(const string &name, int maxLen) {
    for (int n : {1, 2, 3, 5, 17, 100}) {
        for (int m : {1, 2, 9, 64, 300}) {
            if (n + m - 1 > maxLen) { continue; }
            auto a = rnd<M>(n), b = rnd<M>(m);
            check(convolutionLarge(a, b) == brute(a, b), name + " convolutionLarge n=" + std::to_string(n) + " m=" + std::to_string(m));}}
    cout << "PASS " << name << ": convolutionLarge block path against brute\n";}

int main(int argc, char **argv) {
    using M = mint;
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        vector<M> three(3), one(1);
        if (s == "root-one") { primitiveRootNtt(1); }
        if (s == "table-rank") { nttRootTable<ModInt<12289>>(8192); }
        if (s == "ntt-size") { ntt(three); }
        if (s == "intt-size") { intt(three); }
        if (s == "transposed-size") { transposedNtt(three); }
        if (s == "ntt-rank") { vector<ModInt<12289>> v(8192); ntt(v); }
        if (s == "doubling-rank") { vector<ModInt<12289>> v(4096); nttDoubling(v); }
        if (s == "fft-size") { vector<std::complex<double>> v(3); fft(v); }
        if (s == "ifft-size") { vector<std::complex<double>> v(6); ifft(v); }
        if (s == "arbitrary-bound") { using B = ModInt64<1000000000039ULL>; convolutionArbitraryMod(vector<B>(100, 1), vector<B>(100, 1)); }
        if (s == "middle-zero") { middleProduct(three, vector<M>{}); }
        if (s == "cyclic-size") { cyclicConvolution(three, one); }
        if (s == "negacyclic-size") { negacyclicConvolution(three, one); }
        if (s == "truncated-negative") { truncatedConvolution(three, one, -1); }
        if (s == "tensor-rank") { convolutionTensor(three, {3}, one, {1, 1}); }
        if (s == "tensor-size") { convolutionTensor(three, {2}, one, {1}); }
        if (s == "tensor-negative") { convolutionTensor(three, {-3}, one, {1}); }
        if (s == "2d-ragged") { convolution2d(vector<vector<M>>{three, one}, vector<vector<M>>{one}); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        // Lines: op n m then a then b; op L (signed), U (u128), A <mod> (arbitrary modulus, dynamic modint).
        string op;
        while (cin >> op) {
            ulng mod = 0;
            if (op == "A") { cin >> mod; }
            int n, m;
            cin >> n >> m;
            if (op == "L") {
                vector<lng> a(n), b(m);
                for (auto &x : a) { cin >> x; }
                for (auto &x : b) { cin >> x; }
                for (lng x : convolutionLong(a, b)) { cout << x << ' '; }}
            else if (op == "U") {
                vector<ulng> a(n), b(m);
                for (auto &x : a) { cin >> x; }
                for (auto &x : b) { cin >> x; }
                for (ulll x : convolutionU128(a, b)) {
                    string s;
                    do { s += char('0' + int(x % 10)); x /= 10; } while (x);
                    std::reverse(s.begin(), s.end());
                    cout << s << ' ';}}
            else {
                using D = DynModInt64<7>;
                D::setMod(mod);
                vector<D> a(n), b(m);
                for (auto &x : a) { ulng v; cin >> v; x = D(v); }
                for (auto &x : b) { ulng v; cin >> v; x = D(v); }
                for (D x : convolutionArbitraryMod(a, b)) { cout << x.val() << ' '; }}
            cout << '\n';}
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    rng.seed(seed);
    bool quick = mode == "quick", stress = mode == "stress";
#ifdef NDEBUG
    bool fast = true;
#else
    bool fast = false;
#endif
    int lgBig = quick ? 12 : !fast ? 16 : stress ? 23 : 20;

    for (ulng p = 1; p <= (quick ? 2000ULL : 6000ULL); ++p) {
        ulng want = 0;
        bool prime = p >= 2;
        for (ulng d = 2; d * d <= p; ++d) { prime = prime && p % d; }
        if (prime) {
            for (ulng g = 1; g < p && !want; ++g) {
                ulng x = 1, ord = 0;
                do { x = x * g % p; ++ord; } while (x != 1);
                if (ord == p - 1) { want = g; }}}
        if (p >= 2) { check(primitiveRootNtt(p) == want, "primitiveRootNtt brute p=" + std::to_string(p)); }}
    for (ulng p : {561ULL, 1105ULL, 41041ULL, 4294967297ULL, 1000000007ULL * 998244353ULL}) { check(primitiveRootNtt(p) == 0, "primitiveRootNtt composite"); }
    check(primitiveRootNtt(1000000007) == 5 && primitiveRootNtt(2305843009213693951ULL) == 37, "primitiveRootNtt 1e9+7 and 2^61-1");
    static_assert(primitiveRootNtt(998244353) == 3 && primitiveRootNtt(754974721) == 11);
    cout << "PASS primitiveRootNtt exhaustive through " << (quick ? 2000 : 6000) << "\n";

    nttSuite<M>("998244353", {2, 7, 17}, quick ? 6 : 9, lgBig);
    nttSuite<ModInt<12289>>("12289", {2, 3}, 7, 12);
    nttSuite<ModInt<13>>("13", {2, 3}, 2, 2);
    nttSuite<ModInt<7340033>>("7340033", {2, 7}, quick ? 5 : 8, min(lgBig, 20));
    nttSuite<ModInt<754974721>>("754974721", {2, 3, 5}, 5, min(lgBig, 16));
    nttSuite<ModInt64<4179340454199820289ULL>>("4179340454199820289", {2, 29}, quick ? 4 : 7, min(lgBig, 14));

    for (int lg = 0; lg <= 9; ++lg) {
        int n = 1 << lg;
        vector<std::complex<double>> a(n), x;
        for (auto &v : a) { v = {double(rng() % 2001) - 1000, double(rng() % 2001) - 1000}; }
        x = a; fft(x);
        for (int k = 0; k < n; ++k) {
            std::complex<long double> s = 0;
            for (int j = 0; j < n; ++j) { s += std::complex<long double>(a[j]) * std::polar(1.0L, -2 * std::numbers::pi_v<long double> * ((lng(j) * k) % n) / n); }
            check(std::abs(std::complex<long double>(x[rev(k, lg)]) - s) <= 1e-9L * n * 1000, "fft brute n=" + std::to_string(n));}
        ifft(x);
        for (int k = 0; k < n; ++k) { check(std::abs(x[k] - a[k]) <= 1e-9, "ifft round trip n=" + std::to_string(n)); }}
    {
        vector<std::complex<double>> x(1 << 18), a;
        for (auto &v : x) { v = {double(rng() % 2001) - 1000, 0}; }
        a = x; fft(x); ifft(x);
        double e = 0;
        for (int i = 0; i < int(x.size()); ++i) { e = max(e, std::abs(x[i] - a[i])); }
        check(e < 1e-6, "fft/ifft round trip 2^18");}
    for (int n : {0, 1, 2, 7, 64, 200}) {
        for (int m : {0, 1, 5, 64, 130}) {
            vector<double> a(n), b(m);
            vector<lng> ia(n), ib(m);
            for (int i = 0; i < n; ++i) { ia[i] = lng(rng() % 200001) - 100000; a[i] = double(ia[i]); }
            for (int i = 0; i < m; ++i) { ib[i] = lng(rng() % 200001) - 100000; b[i] = double(ib[i]); }
            auto c = convolutionFft(a, b);
            auto e = brute(ia, ib);
            check(c.size() == e.size(), "convolutionFft size");
            for (int i = 0; i < int(e.size()); ++i) { check(std::llround(c[i]) == e[i] && std::abs(c[i] - double(e[i])) < 0.1, "convolutionFft brute n=" + std::to_string(n) + " m=" + std::to_string(m)); }}}
    {
        // Near the stated bound: (sum a^2 + sum b^2) * log2(L) ~ 8.0e14 (full) or 8.6e14 (quick) < 9e14.
        int n = quick ? 1 << 12 : 1 << 16;
        lng V = quick ? 90000 : 19000;
        double worst = 0;
        for (int rep = 0; rep < 2; ++rep) {
            vector<double> a(n), b(n);
            vector<lng> ia(n), ib(n);
            for (int i = 0; i < n; ++i) {
                ia[i] = (rep || i % 3 ? V : -V) - lng(rng() % 7); ib[i] = (rep || i % 5 ? V : -V) - lng(rng() % 7);
                a[i] = double(ia[i]); b[i] = double(ib[i]);}
            auto c = convolutionFft(a, b);
            for (int k = 0; k < 2 * n - 1; k += (k < 300 ? 1 : 97)) {
                lng e = coefficient(ia, ib, k);
                worst = max(worst, std::abs(c[k] - double(e)));
                check(std::llround(c[k]) == e, "convolutionFft rounding at bound k=" + std::to_string(k));}}
        cout << "PASS fft, ifft, convolutionFft (worst error at bound " << worst << ")\n";}

    for (int n : {0, 1, 2, 31, 32, 33, 64, 65, 100, 257}) {
        for (int m : {0, 1, 3, 32, 33, 70, 257, 1000}) {
            vector<ulng> a(n), b(m);
            for (auto &x : a) { x = rng(); }
            for (auto &x : b) { x = rng(); }
            check(convolutionKaratsuba(a, b) == brute(a, b) && convolutionNaive(a, b) == brute(a, b), "Karatsuba/naive ulng n=" + std::to_string(n) + " m=" + std::to_string(m));
            auto x = rnd<ModInt<1000000007>>(n), y = rnd<ModInt<1000000007>>(m);
            check(convolutionKaratsuba(x, y) == brute(x, y), "Karatsuba modint n=" + std::to_string(n) + " m=" + std::to_string(m));}}
    {
        vector<ulng> a(quick ? 3000 : 20000), b(quick ? 2500 : 15001);
        for (auto &x : a) { x = rng(); }
        for (auto &x : b) { x = rng(); }
        sampled(a, b, convolutionKaratsuba(a, b), "Karatsuba large");
        vector<double> d{1.5, -2}, e{4, 0.25};
        check(convolutionNaive(d, e) == vector<double>{6, -7.625, -0.5}, "naive double");}
    cout << "PASS convolutionNaive, convolutionKaratsuba\n";

    for (int rep = 0; rep < (quick ? 20 : 200); ++rep) {
        int n = int(rng() % 90) + 1, m = int(rng() % 90) + 1;
        int sh = int(rng() % 4);
        vector<lng> a(n), b(m);
        for (auto &x : a) { x = lng(rng() >> (2 + 20 * sh)) * (rng() & 1 ? 1 : -1); }
        lng bound = (lng(1) << 62) / ((lng(1) << (62 - 20 * sh)) + 1) / min(n, m);
        for (auto &x : b) { x = lng(rng() % ulng(2 * bound + 1)) - bound; }
        auto c = convolutionLong(a, b);
        vector<lll> wa(a.begin(), a.end()), wb(b.begin(), b.end());
        auto e = brute(wa, wb);
        bool ok = c.size() == e.size();
        for (int i = 0; ok && i < int(e.size()); ++i) { ok = lll(c[i]) == e[i]; }
        check(ok, "convolutionLong brute rep=" + std::to_string(rep));
        vector<ulng> ua(n), ub(m);
        for (auto &x : ua) { x = rng() >> 26; }
        for (auto &x : ub) { x = rng() >> 26; }
        auto u = convolutionU128(ua, ub);
        auto f = brute(vector<ulll>(ua.begin(), ua.end()), vector<ulll>(ub.begin(), ub.end()));
        check(u == f, "convolutionU128 brute rep=" + std::to_string(rep));}
    check(convolutionLong({LLONG_MIN}, {1}) == vector<lng>{LLONG_MIN} && convolutionLong({LLONG_MAX, LLONG_MIN}, {1, 1}) == vector<lng>{LLONG_MAX, -1, LLONG_MIN}, "convolutionLong extremes");
    check(convolutionLong({-1}, {LLONG_MIN + 1}) == vector<lng>{LLONG_MAX} && convolutionLong({}, {1}).empty(), "convolutionLong extremes 2");
    {
        const ulll P = ulll(754974721) * 167772161 * 469762049;
        ulng x = (1ULL << 42) - 1, y = ulng((P - 1) / x);
        check(convolutionU128({x, 0, 1}, {y}) == vector<ulll>{ulll(x) * y, 0, y} && ulll(x) * y < P && ulll(x) * (y + 1) >= P, "convolutionU128 below P1 * P2 * P3");}
    {
        int n = quick ? 5000 : fast ? (stress ? (1 << 23) + 3 : 1 << 20) : 1 << 16;
        vector<lng> a(n), b((1 << (quick ? 13 : fast ? (stress ? 24 : 21) : 17)) + 1 - n);
        for (auto &x : a) { x = lng(rng() % 2000001) - 1000000; }
        for (auto &x : b) { x = lng(rng() % 2000001) - 1000000; }
        sampled(a, b, convolutionLong(a, b), "convolutionLong long length");}
    cout << "PASS convolutionLong, convolutionU128\n";

    {
        using A = ModInt<1000000007>;
        using D = DynModInt<3>;
        using E = DynModInt64<4>;
        for (int n : {0, 1, 59, 60, 61, 200}) {
            for (int m : {0, 1, 60, 61, 333}) {
                auto a = rnd<A>(n), b = rnd<A>(m);
                check(convolutionArbitraryMod(a, b) == brute(a, b), "convolutionArbitraryMod 1e9+7");
                for (ulng mod : {1ULL, 2ULL, 998244353ULL, 2147483647ULL, 4294967291ULL}) {
                    D::setMod(uint(mod));
                    auto x = rnd<D>(n), y = rnd<D>(m);
                    check(convolutionArbitraryMod(x, y) == brute(x, y), "convolutionArbitraryMod dynamic mod=" + std::to_string(mod));}
                E::setMod(1000000000039ULL);
                if (min(n, m) <= 60) {
                    auto x = rnd<E>(n), y = rnd<E>(m);
                    check(convolutionArbitraryMod(x, y) == brute(x, y), "convolutionArbitraryMod 64-bit naive range");}}}
        auto a = rnd<A>(quick ? 4000 : 300000), b = rnd<A>(quick ? 3000 : 200000);
        sampled(a, b, convolutionArbitraryMod(a, b), "convolutionArbitraryMod large");
        cout << "PASS convolutionArbitraryMod\n";}

    largeSuite<ModInt<3>>("p=3 (L=2)", 400);
    largeSuite<ModInt<13>>("p=13 (L=4)", 400);
    largeSuite<ModInt<12289>>("p=12289 (L=4096)", 400);
    {
        using P = ModInt<12289>;
        for (auto [n, m] : vector<pair<int, int>>{{4096, 1}, {4000, 97}, {4097, 4096}, {9000, 7001}, {2048, 2049}, {20000, 61}}) {
            auto a = rnd<P>(n), b = rnd<P>(m);
            auto c = convolutionLarge(a, b);
            if (lng(n) * m <= 40000000) { check(c == brute(a, b), "convolutionLarge 12289 brute n=" + std::to_string(n) + " m=" + std::to_string(m)); }
            else { sampled(a, b, c, "convolutionLarge 12289"); }}
        using Q = ModInt<7340033>;
        int n = quick ? 1000 : fast ? (1 << 20) - 5 : 600000;
        auto a = rnd<Q>(n), b = rnd<Q>(quick ? 900 : 700000);
        sampled(a, b, convolutionLarge(a, b), "convolutionLarge 7340033 beyond 2^20");
        if (stress && fast) {
            auto x = rnd<M>((1 << 24) - 3), y = rnd<M>(1 << 24);
            sampled(x, y, convolution(x, y), "convolution mint length 2^25 - 4");}
        cout << "PASS convolutionLarge\n";}

    wrapperSuite<M>("998244353", quick ? 3000 : 200000);
    wrapperSuite<ModInt<1000000007>>("1000000007", quick ? 2000 : 50000);
    DynModInt<5>::setMod(1000003);
    wrapperSuite<DynModInt<5>>("dynamic 1000003", quick ? 2000 : 30000);
    wrapperSuite<ModInt64<4179340454199820289ULL>>("4179340454199820289", quick ? 1000 : 20000);
    cout << "PASS all, " << checks << " checks\n";}
