#include "../../01-Core/03-barrett.hpp"
#include "../../01-Core/04-montgomery.hpp"

using Clock = std::chrono::steady_clock;
volatile ulng sink;
int repetitions;
double target_ms;
std::mt19937_64 rng;

template<class F> double measure(F f) {
    auto run = [&](int count) -> double {
        auto start = Clock::now();
        for (int i = 0; i < count; ++i) { f(); asm volatile("" : : : "memory"); }
        return std::chrono::duration<double, std::nano>(Clock::now() - start).count();};
    int count = 1;
    while (run(count) < target_ms * 1e6 && count < (1 << 22)) { count *= 2; }
    vector<double> samples;
    for (int i = 0; i < repetitions; ++i) { samples.push_back(run(count) / count); }
    sort(samples.begin(), samples.end());
    return samples[repetitions / 2];}

void emit(const char *width, ulng mod, const char *workload, const char *distribution,
          int n, const char *method, double ns) {
    cout << "{\"width\":\"" << width << "\",\"modulus\":\"" << mod
         << "\",\"workload\":\"" << workload << "\",\"distribution\":\"" << distribution
         << "\",\"n\":" << n << ",\"method\":\"" << method << "\",\"ns\":" << ns << "}\n";}

template<class W, class D> W nativePow(W a, ulng e, W m) {
    W r = W(1 % m); a %= m;
    while (e) {
        if (e & 1) { r = W(D(r) * a % m); }
        e >>= 1;
        if (e) { a = W(D(a) * a % m); }}
    return r;}

template<class W, class D, class B, class M> void bench(W mod) {
    constexpr bool SMALL = sizeof(W) == 4;
    const char *width = SMALL ? "32" : "64";
    B bar(mod);
    bool mask = (mod & (mod - 1)) == 0;
    D reciprocal = mask ? 0 : ~D(0) / mod;
    auto genericReduce = [&](D x) -> W {
        if (mask) { return W(x) & (mod - 1); }
        D q;
        if constexpr (SMALL) { q = D(ulll(x) * reciprocal >> 64); }
        else { q = Barrett64::highProduct(x, reciprocal); }
        D r = x - q * mod;
        return W(r >= mod ? r - mod : r);};
    // A dummy odd context keeps the benchmark structure uniform for even moduli.
    M mont(mod | 1);
    for (const char *distribution : {"uniform", "near-modulus"}) {
        for (int n : {1, 7, 8, 9, 16, 32, 256, 4096}) {
            vector<W> a(n), b(n), am(n), bm(n), out(n), expected(n);
            for (int i = 0; i < n; ++i) {
                a[i] = distribution[0] == 'u' ? W(rng() % mod) : W(mod - 1 - min<ulng>(mod - 1, rng() % 4));
                b[i] = distribution[0] == 'u' ? W(rng() % mod) : W(mod - 1 - min<ulng>(mod - 1, rng() % 4));
                expected[i] = W(D(a[i]) * b[i] % mod);
                if (mod & 1) { am[i] = mont.init(a[i]); bm[i] = mont.init(b[i]); }}
            auto report = [&](const char *method, auto f, int transformed) -> void {
                f();
                for (int i = 0; i < n; ++i) {
                    W got = transformed == 2 ? mont.get(mont.normalize(out[i])) : transformed ? mont.get(out[i]) : out[i];
                    if (got != expected[i]) {
                        cerr << "FAIL batch " << width << ' ' << mod << ' ' << n << ' ' << method << ' ' << i << '\n';
                        std::exit(1);}}
                emit(width, mod, "bulk-product", distribution, n, method, measure([&]() {
                    f(); asm volatile("" : : "g"(out.data()) : "memory");}));};
            report("native-percent", [&]() { for (int i = 0; i < n; ++i) { out[i] = W(D(a[i]) * b[i] % mod); } }, false);
            report("barrett-reciprocal", [&]() { for (int i = 0; i < n; ++i) { out[i] = genericReduce(D(a[i]) * b[i]); } }, false);
            report("barrett-scalar", [&]() { for (int i = 0; i < n; ++i) { out[i] = bar.mul(a[i], b[i]); } }, false);
            report("barrett-bulk", [&]() { bar.mul(a.data(), b.data(), out.data(), n); }, false);
            if (mod & 1) {
                report("montgomery-scalar", [&]() { for (int i = 0; i < n; ++i) { out[i] = mont.mul(am[i], bm[i]); } }, true);
                report("montgomery-bulk", [&]() { mont.mul(am.data(), bm.data(), out.data(), n); }, true);
                if (mod < (W(1) << (8 * sizeof(W) - 2))) {
                    for (int i = 0; i < n; ++i) { if (i & 1) { am[i] += mod; } else { bm[i] += mod; } }
                    report("montgomery-lazy-scalar", [&]() { for (int i = 0; i < n; ++i) { out[i] = mont.mulLazy(am[i], bm[i]); } }, 2);
                    report("montgomery-lazy-bulk", [&]() { mont.mulLazy(am.data(), bm.data(), out.data(), n); }, 2);}
                for (int i = 0; i < n; ++i) { am[i] = mont.init(a[i]); }
                auto conversion = [&](const char *method, auto f, const vector<W> &want) -> void {
                    f();
                    if (out != want) { cerr << "FAIL conversion " << method << '\n'; std::exit(1); }
                    emit(width, mod, "conversion", distribution, n, method, measure([&]() {
                        f(); asm volatile("" : : "g"(out.data()) : "memory");}));};
                conversion("init-scalar", [&]() { for (int i = 0; i < n; ++i) { out[i] = mont.init(a[i]); } }, am);
                conversion("init-bulk", [&]() { mont.init(a.data(), out.data(), n); }, am);
                conversion("get-scalar", [&]() { for (int i = 0; i < n; ++i) { out[i] = mont.get(am[i]); } }, a);
                conversion("get-bulk", [&]() { mont.get(am.data(), out.data(), n); }, a);}}}

    const int n = 256;
    vector<D> wide(n);
    vector<W> out(n), expected(n);
    for (int i = 0; i < n; ++i) {
        if constexpr (SMALL) { wide[i] = rng(); }
        else { wide[i] = (D(rng()) << 64) | rng(); }
        expected[i] = W(wide[i] % mod);}
    auto reduce = [&]() -> void { for (int i = 0; i < n; ++i) { out[i] = bar.reduce(wide[i]); } };
    reduce();
    if (out != expected) { cerr << "FAIL wide-reduce\n"; std::exit(1); }
    emit(width, mod, "wide-reduce", "uniform-full-width", n, "native-percent", measure([&]() {
        for (int i = 0; i < n; ++i) { out[i] = W(wide[i] % mod); }
        asm volatile("" : : "g"(out.data()) : "memory");}));
    emit(width, mod, "wide-reduce", "uniform-full-width", n, "barrett", measure([&]() {
        reduce(); asm volatile("" : : "g"(out.data()) : "memory");}));
    for (int i = 0; i < n; ++i) { out[i] = genericReduce(wide[i]); }
    if (out != expected) { cerr << "FAIL generic-reciprocal\n"; std::exit(1); }
    emit(width, mod, "wide-reduce", "uniform-full-width", n, "barrett-reciprocal", measure([&]() {
        for (int i = 0; i < n; ++i) { out[i] = genericReduce(wide[i]); }
        asm volatile("" : : "g"(out.data()) : "memory");}));
    // Quotient and remainder together; the quotient is folded into the checked word.
    for (int i = 0; i < n; ++i) { expected[i] = W(wide[i] / mod) ^ W(wide[i] % mod); }
    auto divide = [&]() -> void { for (int i = 0; i < n; ++i) { auto [q, r] = bar.divMod(wide[i]); out[i] = W(q) ^ r; } };
    divide();
    if (out != expected) { cerr << "FAIL wide-divmod\n"; std::exit(1); }
    emit(width, mod, "wide-divmod", "uniform-full-width", n, "native-percent", measure([&]() {
        for (int i = 0; i < n; ++i) { out[i] = W(wide[i] / mod) ^ W(wide[i] % mod); }
        asm volatile("" : : "g"(out.data()) : "memory");}));
    emit(width, mod, "wide-divmod", "uniform-full-width", n, "barrett", measure([&]() {
        divide(); asm volatile("" : : "g"(out.data()) : "memory");}));

    W constant = W(rng());
    auto fixed = bar.multiplier(constant);
    for (int count : {1, 7, 8, 9, 16, 32, 256, 4096}) {
        vector<W> words(count);
        out.resize(count); expected.resize(count);
        for (int i = 0; i < count; ++i) { words[i] = W(rng()); expected[i] = W(D(words[i]) * constant % mod); }
        auto fixedReport = [&](const char *method, auto f) -> void {
            f();
            if (out != expected) { cerr << "FAIL fixed-multiplier " << method << '\n'; std::exit(1); }
            emit(width, mod, "fixed-product", "uniform-full-width", count, method, measure([&]() {
                f(); asm volatile("" : : "g"(out.data()) : "memory");}));};
        fixedReport("native-percent", [&]() { for (int i = 0; i < count; ++i) { out[i] = W(D(words[i]) * constant % mod); } });
        fixedReport("barrett", [&]() { for (int i = 0; i < count; ++i) { out[i] = bar.mul(words[i], constant); } });
        fixedReport("shoup-scalar", [&]() { for (int i = 0; i < count; ++i) { out[i] = fixed.mul(words[i]); } });
        fixedReport("shoup-bulk", [&]() { fixed.mul(words.data(), out.data(), count);});}

    for (const char *distribution : {"uniform", "unit-factors"}) {
        vector<W> factors(4096);
        bool units = string_view(distribution) == "unit-factors";
        for (W &a : factors) {
            do { a = W(rng() % mod); } while (units && gcd(a, mod) != 1);}
        W answer = W(1 % mod);
        for (W a : factors) { answer = W(D(answer) * a % mod); }
        auto chain = [&](const char *method, auto f) -> void {
            if (f() != answer) { cerr << "FAIL chain " << method << '\n'; std::exit(1); }
            emit(width, mod, "dependent-product", distribution, int(factors.size()), method, measure([&]() { sink = f(); }));};
        chain("native-percent", [&]() { W r = W(1 % mod); for (W a : factors) { r = W(D(r) * a % mod); } return r; });
        chain("barrett", [&]() { W r = W(1 % mod); for (W a : factors) { r = bar.mul(r, a); } return r; });
        if (mod & 1) {
            for (W &a : factors) { a = mont.init(a); }
            chain("montgomery", [&]() { W r = mont.init(1); for (W a : factors) { r = mont.mul(r, a); } return mont.get(r);});}}

    // End-to-end repeated exponentiation includes each context's setup and both conversions.
    // Volatile modulus forces reconstruction; inputs and exponents are shared by all methods.
    volatile W changing_mod = mod;
    array<W, 8> bases;
    array<ulng, 8> exponents;
    for (int i = 0; i < 8; ++i) { bases[i] = W(rng()); exponents[i] = rng(); }
    auto native = [&]() -> ulng {
        ulng checksum = 0;
        for (int i = 0; i < 8; ++i) { checksum ^= nativePow<W, D>(bases[i], exponents[i], changing_mod); }
        return checksum;};
    ulng checksum = native();
    auto powers = [&](const char *method, auto f) -> void {
        if (f() != checksum) { cerr << "FAIL setup+pow " << method << '\n'; std::exit(1); }
        emit(width, mod, "setup-and-pow", "full-width-base-and-exponent", 8, method, measure([&]() { sink = f(); }));};
    powers("native-percent", native);
    powers("barrett", [&]() { ulng r = 0; for (int i = 0; i < 8; ++i) { B b(changing_mod); r ^= b.pow(bases[i], exponents[i]); } return r; });
    if (mod & 1) {
        powers("montgomery", [&]() { ulng r = 0; for (int i = 0; i < 8; ++i) { M m(changing_mod); r ^= m.pow(bases[i], exponents[i]); } return r;});}}

int main(int argc, char **argv) {
    if (argc != 4) { return 2; }
    rng.seed(std::stoull(argv[1]));
    repetitions = std::stoi(argv[2]); target_ms = std::stod(argv[3]);
    for (uint m : {1U, 65536U, 998244353U, 2147483647U, 4294967291U}) { bench<uint, ulng, Barrett32, Montgomery32>(m); }
    for (ulng m : {ulng(1), ulng(4294967311), (ulng(1) << 61) - 1, ulng(1) << 63, ~ulng(0) - 58, ~ulng(0)}) {
        bench<ulng, ulll, Barrett64, Montgomery64>(m);}
    cerr << "PASS all benchmark outputs checked against native wide %\n";}
