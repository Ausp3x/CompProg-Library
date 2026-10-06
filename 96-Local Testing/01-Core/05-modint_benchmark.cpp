#include "../../01-Core/03-barrett.hpp"
#include "../../01-Core/04-montgomery.hpp"
#include "../../01-Core/05-modint.hpp"

using Clock = std::chrono::steady_clock;
volatile ulng sink;
int repetitions;
double target_ms;
std::mt19937_64 rng;

template<class F> double measure(F f) {
    auto run = [&] (int count) -> double {
        auto start = Clock::now();
        for (int i = 0; i < count; ++i) { f(); asm volatile("" : : : "memory"); }
        return std::chrono::duration<double, std::nano>(Clock::now() - start).count();};
    int count = 1;
    while (run(count) < target_ms * 1e6 && count < (1 << 22)) { count *= 2; }
    vector<double> samples;
    for (int i = 0; i < repetitions; ++i) { samples.push_back(run(count) / count); }
    sort(samples.begin(), samples.end());
    return samples[repetitions / 2];}

string decimal(ulll x) {
    string s;
    do { s += char('0' + x % 10); x /= 10; } while (x);
    reverse(s.begin(), s.end()); return s;}

void emit(int width, bool fixed, ulng mod, const char *workload, const char *distribution,
          int n, ulll exponent, const char *method, double ns) {
    cout << "{\"width\":" << width << ",\"fixed\":" << (fixed ? "true" : "false")
         << ",\"modulus\":\"" << mod << "\",\"workload\":\"" << workload
         << "\",\"distribution\":\"" << distribution << "\",\"n\":" << n
         << ",\"exponent\":\"" << decimal(exponent) << "\",\"method\":\"" << method
         << "\",\"ns\":" << ns << "}\n";}

template<class W, class D, W FIXED = 0> W nativeProduct(W a, W b, W m) {
    if constexpr (FIXED) { return W(D(a) * b % FIXED); }
    return W(D(a) * b % m);}

template<class W, class D, W FIXED = 0> W nativePow(W a, ulll e, W m) {
    W r = W(m != 1);
    while (e) {
        if (e & 1) { r = nativeProduct<W, D, FIXED>(r, a, m); }
        e >>= 1;
        if (e) { a = nativeProduct<W, D, FIXED>(a, a, m); }}
    return r;}

template<class M, class W> W montPower(const M &m, W a, ulll e) {
    W r = m.init(1); a = m.init(a);
    while (e) {
        if (e & 1) { r = m.mul(r, a); }
        e >>= 1;
        if (e) { a = m.mul(a, a); }}
    return m.get(r);}

const vector<ulll> exponents = {0, 1, 7, 31, 32, 63, 64, 127, 128, 255, 256, 511, 512, 513, 1023, 1024, 1025,
                               65535, 65536, 65537,
                               0xffffffff, ulll(~ulng(0)), ulll(1) << 127, ~ulll(0)};

// FIXED=0 deliberately hides the modulus from constant propagation. Every
// Montgomery candidate returns ordinary residues and includes conversions.
template<class W, class D, class B, class M, W FIXED = 0>
[[gnu::noinline]] void bench(W input) {
    constexpr bool STATIC = FIXED != 0;
    constexpr int BITS = 8 * sizeof(W);
    W mod = FIXED;
    if constexpr (!STATIC) { mod = input; asm volatile("" : "+r"(mod)); }
    B bar(mod); M mont(mod | 1);
    for (const char *distribution : {"uniform", "near-modulus"}) {
        for (int n : {1, 7, 8, 9, 32, 256, 4096}) {
            vector<W> a(n), b(n), am(n), bm(n), out(n), expected(n);
            for (int i = 0; i < n; ++i) {
                a[i] = distribution[0] == 'u' ? W(rng() % mod) : W(mod - 1 - min<ulng>(mod - 1, rng() % 4));
                b[i] = distribution[0] == 'u' ? W(rng() % mod) : W(mod - 1 - min<ulng>(mod - 1, rng() % 4));
                expected[i] = W(D(a[i]) * b[i] % mod);}
            auto report = [&] (const char *method, auto f) -> void {
                f();
                if (out != expected) {
                    cerr << "FAIL bulk width=" << BITS << " modulus=" << mod << " n=" << n << " method=" << method << '\n';
                    std::exit(1);}
                emit(BITS, STATIC, mod, "ordinary-bulk-product", distribution, n, 0, method, measure([&]() {
                    f(); asm volatile("" : : "g"(out.data()) : "memory");}));};
            report("native-percent", [&]() { for (int i = 0; i < n; ++i) { out[i] = nativeProduct<W, D, FIXED>(a[i], b[i], mod); } });
            report("barrett", [&]() { bar.mul(a.data(), b.data(), out.data(), n); });
            if constexpr (BITS == 64) {
                if (mod == modint_detail::MERSENNE61) {
                    report("mersenne-fold", [&]() { for (int i = 0; i < n; ++i) { out[i] = W(modint_detail::fold61(D(a[i]) * b[i])); } });}}
            if (mod & 1) {
                report("montgomery-convert-scalar", [&]() {
                    for (int i = 0; i < n; ++i) { out[i] = mont.get(mont.mul(mont.init(a[i]), mont.init(b[i]))); }});
                report("montgomery-convert-bulk", [&]() {
                    mont.init(a.data(), am.data(), n); mont.init(b.data(), bm.data(), n);
                    mont.mul(am.data(), bm.data(), out.data(), n); mont.get(out.data(), out.data(), n);});}}}
    for (int n : {8, 256, 4096}) {
        vector<W> factors(n);
        for (W &a : factors) { do { a = W(rng() % mod); } while (gcd(a, mod) != 1); }
        W expected = W(mod != 1);
        for (W a : factors) { expected = W(D(expected) * a % mod); }
        auto report = [&] (const char *method, auto f) -> void {
            if (f() != expected) { cerr << "FAIL chain " << BITS << ' ' << mod << ' ' << method << '\n'; std::exit(1); }
            emit(BITS, STATIC, mod, "ordinary-dependent-product", "unit-factors", n, 0, method,
                 measure([&]() { sink = f(); }));};
        report("native-percent", [&]() { W r = W(mod != 1); for (W a : factors) { r = nativeProduct<W, D, FIXED>(r, a, mod); } return r; });
        report("barrett", [&]() { W r = W(mod != 1); for (W a : factors) { r = bar.mul(r, a); } return r; });
        if constexpr (BITS == 64) {
            if (mod == modint_detail::MERSENNE61) {
                report("mersenne-fold", [&]() { W r = 1; for (W a : factors) { r = W(modint_detail::fold61(D(r) * a)); } return r; });}}
        if (mod & 1) {
            report("montgomery-convert-each", [&]() {
                W r = W(mod != 1);
                for (W a : factors) { r = mont.get(mont.mul(mont.init(r), mont.init(a))); }
                return r;});
            report("montgomery-chain-domain", [&]() {
                W r = mont.init(1);
                for (W a : factors) { r = mont.mul(r, mont.init(a)); }
                return mont.get(r);});}}
    array<W, 8> bases;
    for (W &a : bases) { a = W(rng() % mod); }
    for (ulll exponent : exponents) {
        auto report = [&] (const char *method, auto f) -> void {
            for (W a : bases) {
                if (f(a) != nativePow<W, D>(a, exponent, mod)) {
                    cerr << "FAIL power " << BITS << ' ' << mod << ' ' << a << ' ' << decimal(exponent) << ' ' << method << '\n';
                    std::exit(1);}}
            emit(BITS, STATIC, mod, "ordinary-powers", "uniform-canonical", 8, exponent, method,
                 measure([&]() { ulng r = 0; for (W a : bases) { r ^= f(a); } sink = r; }));};
        report("native-percent", [&](W a) { return nativePow<W, D, FIXED>(a, exponent, mod); });
        report("barrett", [&](W a) {
            W p = W(mod != 1); ulll e = exponent;
            while (e) { if (e & 1) { p = bar.mul(p, a); } e >>= 1; if (e) { a = bar.mul(a, a); } }
            return p;});
        if constexpr (BITS == 64) {
            if (mod == modint_detail::MERSENNE61) {
                report("mersenne-fold", [&](W a) {
                    W p = 1; ulll e = exponent;
                    while (e) { if (e & 1) { p = W(modint_detail::fold61(D(p) * a)); } e >>= 1; if (e) { a = W(modint_detail::fold61(D(a) * a)); } }
                    return p;});}}
        if (mod & 1) {
            report("montgomery-cached", [&](W a) { return montPower(mont, a, exponent); });
            report("montgomery-setup", [&](W a) {
                if constexpr (STATIC) { M context(FIXED); return montPower(context, a, exponent); }
                else { W m = mod; asm volatile("" : "+r"(m)); M context(m); return montPower(context, a, exponent); }});}}}

template<class W, class D, class T, W FIXED = 0> [[gnu::noinline]] void benchType(W input) {
    constexpr bool STATIC = FIXED != 0;
    constexpr int BITS = 8 * sizeof(W);
    W mod = FIXED;
    if constexpr (!STATIC) { mod = input; asm volatile("" : "+r"(mod)); T::setMod(mod, 0); }
    for (int n : {8, 256, 4096}) {
        vector<T> a(n), b(n), out(n), expected(n);
        for (int i = 0; i < n; ++i) {
            W x; do { x = W(rng() % mod); } while (gcd(x, mod) != 1);
            a[i] = x; b[i] = rng(); expected[i] = W(D(a[i].n) * b[i].n % mod);}
        auto bulk = [&] () { for (int i = 0; i < n; ++i) { out[i] = a[i] * b[i]; } };
        bulk(); if (out != expected) { cerr << "FAIL actual bulk " << BITS << ' ' << mod << '\n'; std::exit(1); }
        emit(BITS, STATIC, mod, "actual-bulk-product", "unit-left-uniform-right", n, 0, "modular-type", measure([&]() {
            bulk(); asm volatile("" : : "g"(out.data()) : "memory");}));
        emit(BITS, STATIC, mod, "actual-bulk-product", "unit-left-uniform-right", n, 0, "native-percent", measure([&]() {
            for (int i = 0; i < n; ++i) { out[i].n = nativeProduct<W, D, FIXED>(a[i].n, b[i].n, mod); }
            asm volatile("" : : "g"(out.data()) : "memory");}));
        W want = W(mod != 1);
        for (T x : a) { want = W(D(want) * x.n % mod); }
        auto chain = [&] () { T r = 1; for (T x : a) { r *= x; } return r.n; };
        if (chain() != want) { cerr << "FAIL actual chain " << BITS << ' ' << mod << '\n'; std::exit(1); }
        emit(BITS, STATIC, mod, "actual-dependent-product", "unit-factors", n, 0, "modular-type", measure([&]() { sink = chain(); }));
        emit(BITS, STATIC, mod, "actual-dependent-product", "unit-factors", n, 0, "native-percent", measure([&]() {
            W r = W(mod != 1); for (T x : a) { r = nativeProduct<W, D, FIXED>(r, x.n, mod); } sink = r;}));}
    array<T, 8> bases;
    for (T &a : bases) { a = rng(); }
    for (ulll exponent : exponents) {
        for (T a : bases) {
            if (pow(a, exponent).n != nativePow<W, D>(a.n, exponent, mod)) {
                cerr << "FAIL actual power " << BITS << ' ' << mod << ' ' << a.n << ' ' << decimal(exponent) << '\n'; std::exit(1);}}
        auto powers = [&] () { ulng r = 0; for (T a : bases) { r ^= pow(a, exponent).n; } return r; };
        emit(BITS, STATIC, mod, "actual-powers", "uniform-canonical", 8, exponent, "modular-type", measure([&]() { sink = powers(); }));
        emit(BITS, STATIC, mod, "actual-powers", "uniform-canonical", 8, exponent, "native-percent", measure([&]() {
            ulng r = 0; for (T a : bases) { r ^= nativePow<W, D, FIXED>(a.n, exponent, mod); } sink = r;}));}}

// Signed native inputs: the type's word-width normalization against a 128-bit remainder.
template<class W, class D, class T, W FIXED = 0> [[gnu::noinline]] void benchConstruct(W input) {
    constexpr bool STATIC = FIXED != 0;
    constexpr int BITS = 8 * sizeof(W);
    W mod = FIXED;
    if constexpr (!STATIC) { mod = input; asm volatile("" : "+r"(mod)); T::setMod(mod, 0); }
    int n = 4096;
    vector<T> out(n); vector<W> expected(n);
    auto check = [&](const char *kind) -> void {
        for (int i = 0; i < n; ++i) {
            if (out[i].n != expected[i]) { cerr << "FAIL construction " << BITS << ' ' << mod << ' ' << kind << ' ' << i << '\n'; std::exit(1); }}};
    auto wide = [&](lng x) -> W { lll r = lll(x) % lll(mod); return W(r < 0 ? r + mod : r); };
    for (const char *kind : {"int", "lng"}) {
        vector<lng> x(n);
        for (lng &v : x) { v = kind[0] == 'i' ? lng(int(rng())) : lng(rng()); }
        for (int i = 0; i < n; ++i) { expected[i] = wide(x[i]); }
        auto build = [&]() -> void {
            if (kind[0] == 'i') { for (int i = 0; i < n; ++i) { out[i] = T(int(x[i])); } }
            else { for (int i = 0; i < n; ++i) { out[i] = T(x[i]); } }
            asm volatile("" : : "g"(out.data()) : "memory");};
        build(); check(kind);
        emit(BITS, STATIC, mod, "construction", kind, n, 0, "modular-type", measure(build));
        auto former = [&]() -> void {
            for (int i = 0; i < n; ++i) { out[i].n = wide(x[i]); }
            asm volatile("" : : "g"(out.data()) : "memory");};
        former(); check(kind);
        emit(BITS, STATIC, mod, "construction", kind, n, 0, "wide-percent", measure(former));}}

template<class W, class D, class T, W FIXED = 0> [[gnu::noinline]] void benchInverse(W input) {
    constexpr bool STATIC = FIXED != 0;
    constexpr int BITS = 8 * sizeof(W);
    W mod = FIXED;
    if constexpr (!STATIC) { mod = input; asm volatile("" : "+r"(mod)); T::setMod(mod, 0); }
    bool prime = T::isPrime();
    for (int n : {1, 8, 256}) {
        vector<T> a(n), out(n), expected(n);
        for (int i = 0; i < n; ++i) {
            W x; do { x = W(rng() % mod); } while (gcd(x, mod) != 1);
            a[i] = x; expected[i] = inv(a[i]);
            if (W(D(a[i].n) * expected[i].n % mod) != W(mod != 1)) {
                cerr << "FAIL inverse reference " << BITS << ' ' << mod << ' ' << x << '\n'; std::exit(1);}}
        auto report = [&] (const char *method, auto f) -> void {
            f();
            if (out != expected) { cerr << "FAIL inverse " << BITS << ' ' << mod << ' ' << n << ' ' << method << '\n'; std::exit(1); }
            emit(BITS, STATIC, mod, "unit-inverses", prime ? "uniform-prime-units" : "uniform-composite-units", n, 0,
                 method, measure([&]() { f(); asm volatile("" : : "g"(out.data()) : "memory"); }));};
        report("individual-eea", [&]() { for (int i = 0; i < n; ++i) { out[i] = inv(a[i]); } });
        report("batch-one-eea", [&]() { if (!batchInv(a, out)) { std::exit(1); } });
        if (prime) { report("individual-fermat", [&]() { for (int i = 0; i < n; ++i) { out[i] = pow(a[i], mod - 2); } }); }}}

int main(int argc, char **argv) {
    if (argc != 5) { return 2; }
    rng.seed(std::stoull(argv[1])); repetitions = std::stoi(argv[2]); target_ms = std::stod(argv[3]);
    string suite = argv[4];
    if (suite == "all" || suite == "candidates") {
        for (uint m : {1U, 65536U, 998244353U, 4294967291U, 4294967295U}) {
            bench<uint, ulng, Barrett32, Montgomery32>(m);}
        for (ulng m : {ulng(998244353), ulng(4294967311), (ulng(1) << 61) - 1, ulng(1) << 63, ~ulng(0) - 58, ~ulng(0)}) {
            bench<ulng, ulll, Barrett64, Montgomery64>(m);}
        bench<uint, ulng, Barrett32, Montgomery32, 998244353U>(0);
        bench<uint, ulng, Barrett32, Montgomery32, 4294967291U>(0);
        bench<ulng, ulll, Barrett64, Montgomery64, 998244353ULL>(0);
        bench<ulng, ulll, Barrett64, Montgomery64, 4294967311ULL>(0);
        bench<ulng, ulll, Barrett64, Montgomery64, modint_detail::MERSENNE61>(0);
        bench<ulng, ulll, Barrett64, Montgomery64, 18446744073709551557ULL>(0);}
    if (suite == "all" || suite == "actual") {
        for (uint m : {1U, 65536U, 998244353U, 4294967291U, 4294967295U}) { benchType<uint, ulng, DynModInt<91>>(m); }
        for (ulng m : {ulng(998244353), ulng(4294967311), (ulng(1) << 61) - 1, ulng(1) << 63, ~ulng(0) - 58, ~ulng(0)}) {
            benchType<ulng, ulll, DynModInt64<91>>(m);}
        benchType<uint, ulng, ModInt<998244353>, 998244353U>(0);
        benchType<uint, ulng, ModInt<4294967291>, 4294967291U>(0);
        benchType<ulng, ulll, ModInt64<998244353ULL>, 998244353ULL>(0);
        benchType<ulng, ulll, ModInt64<4294967311ULL>, 4294967311ULL>(0);
        benchType<ulng, ulll, ModInt61, modint_detail::MERSENNE61>(0);
        benchType<ulng, ulll, ModInt64<18446744073709551557ULL>, 18446744073709551557ULL>(0);}
    if (suite == "all" || suite == "construction") {
        for (uint m : {1U, 65536U, 998244353U, 4294967295U}) { benchConstruct<uint, ulng, DynModInt<93>>(m); }
        for (ulng m : {ulng(998244353), (ulng(1) << 61) - 1, ulng(1) << 63, ~ulng(0) - 58}) { benchConstruct<ulng, ulll, DynModInt64<93>>(m); }
        benchConstruct<uint, ulng, ModInt<998244353>, 998244353U>(0);
        benchConstruct<uint, ulng, ModInt<4294967295>, 4294967295U>(0);
        benchConstruct<ulng, ulll, ModInt64<998244353ULL>, 998244353ULL>(0);
        benchConstruct<ulng, ulll, ModInt61, 2305843009213693951ULL>(0);
        benchConstruct<ulng, ulll, ModInt64<18446744073709551557ULL>, 18446744073709551557ULL>(0);}
    if (suite == "all" || suite == "inverses") {
        benchInverse<uint, ulng, ModInt<998244353>, 998244353U>(0);
        benchInverse<uint, ulng, ModInt<4294967291>, 4294967291U>(0);
        benchInverse<ulng, ulll, ModInt64<4294967311ULL>, 4294967311ULL>(0);
        benchInverse<ulng, ulll, ModInt64<2305843009213693951ULL>, 2305843009213693951ULL>(0);
        benchInverse<ulng, ulll, ModInt64<18446744073709551557ULL>, 18446744073709551557ULL>(0);
        for (uint m : {15U, 998244353U, 4294967291U}) { benchInverse<uint, ulng, DynModInt<92>>(m); }
        for (ulng m : {ulng(4294967311), (ulng(1) << 61) - 1, ~ulng(0) - 58, ~ulng(0)}) {
            benchInverse<ulng, ulll, DynModInt64<92>>(m);}}
    cerr << "PASS all candidate outputs checked against unsigned double-width %\n";}
