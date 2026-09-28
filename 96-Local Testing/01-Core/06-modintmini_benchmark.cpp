#include "../../01-Core/05-modint.hpp"
#include "../../01-Core/06-modintmini.hpp"

using Clock = std::chrono::steady_clock;
volatile ulng sink;
int repetitions;
double target_ms;
std::mt19937_64 rng;

template<typename F> double measure(F f) {
    auto run = [&] (int count) -> double {
        auto start = Clock::now();
        for (int i = 0; i < count; ++i) { f(); asm volatile("" : : : "memory"); }
        return std::chrono::duration<double, std::nano>(Clock::now() - start).count();
    };
    int count = 1;
    while (run(count) < target_ms * 1e6 && count < (1 << 22)) { count *= 2; }
    vector<double> samples;
    for (int i = 0; i < repetitions; ++i) { samples.push_back(run(count) / count); }
    sort(samples.begin(), samples.end()); return samples[repetitions / 2];}

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

template<typename W, typename D, W FIXED = 0> W nativeProduct(W a, W b, W m) {
    if constexpr (FIXED) { return W(D(a) * b % FIXED); }
    return W(D(a) * b % m);}

template<typename W, typename D, W FIXED = 0> W nativePower(W a, ulll e, W m) {
    W r = W(m != 1);
    for (; e > 1; e >>= 1, a = nativeProduct<W, D, FIXED>(a, a, m)) {
        if (e & 1) { r = nativeProduct<W, D, FIXED>(r, a, m); }}
    return e ? nativeProduct<W, D, FIXED>(r, a, m) : r;}

template<typename Mini, typename Full, typename W, typename D, W FIXED = 0>
[[gnu::noinline]] void bench(W input) {
    constexpr bool STATIC = FIXED != 0;
    constexpr int BITS = 8 * sizeof(W);
    W mod = FIXED;
    if constexpr (!STATIC) {
        mod = input; asm volatile("" : "+r"(mod)); Mini::setMod(mod); Full::setMod(mod, 0); }
    for (const char *distribution : {"uniform", "near-modulus"}) {
        for (int n : {1, 8, 256, 4096}) {
            vector<W> a(n), b(n), out(n), expected(n);
            for (int i = 0; i < n; ++i) {
                a[i] = distribution[0] == 'u' ? W(rng() % mod) : W(mod - 1 - min<ulng>(mod - 1, rng() % 4));
                b[i] = distribution[0] == 'u' ? W(rng() % mod) : W(mod - 1 - min<ulng>(mod - 1, rng() % 4));
                expected[i] = W(D(a[i]) * b[i] % mod); }
            auto report = [&] (const char *method, auto f) -> void {
                f();
                if (out != expected) {
                    cerr << "FAIL product " << BITS << ' ' << mod << ' ' << n << ' ' << method << '\n'; std::exit(1); }
                emit(BITS, STATIC, mod, "ordinary-products", distribution, n, 0, method,
                     measure([&]() { f(); asm volatile("" : : "g"(out.data()) : "memory"); }));
            };
            report("native", [&]() { for (int i = 0; i < n; ++i) { out[i] = nativeProduct<W, D, FIXED>(a[i], b[i], mod); } });
            report("mini", [&]() { for (int i = 0; i < n; ++i) { out[i] = (Mini::raw(a[i]) * Mini::raw(b[i])).val(); } });
            report("full", [&]() { for (int i = 0; i < n; ++i) { out[i] = (Full::raw(a[i]) * Full::raw(b[i])).val(); } });}}
    for (int n : {8, 256}) {
        vector<W> factors(n);
        for (W &a : factors) { do { a = W(rng() % mod); } while (gcd(a, mod) != 1); }
        W expected = W(mod != 1);
        for (W a : factors) { expected = W(D(expected) * a % mod); }
        auto report = [&] (const char *method, auto f) -> void {
            if (f() != expected) { cerr << "FAIL chain " << BITS << ' ' << mod << ' ' << method << '\n'; std::exit(1); }
            emit(BITS, STATIC, mod, "dependent-products", "unit-factors", n, 0, method, measure([&]() { sink = f(); }));
        };
        report("native", [&]() { W r = W(mod != 1); for (W a : factors) { r = nativeProduct<W, D, FIXED>(r, a, mod); } return r; });
        report("mini", [&]() { Mini r = 1; for (W a : factors) { r *= Mini::raw(a); } return r.val(); });
        report("full", [&]() { Full r = 1; for (W a : factors) { r *= Full::raw(a); } return r.val(); });}
    array<W, 8> bases;
    for (W &a : bases) { a = W(rng() % mod); }
    for (ulll exponent : {ulll(0), ulll(1), ulll(7), ulll(511), ulll(512), ulll(513), ulll(65535), ulll(65536), ulll(65537), ulll(1) << 127, ~ulll(0)}) {
        auto report = [&] (const char *method, auto f) -> void {
            for (W a : bases) {
                if (f(a) != nativePower<W, D>(a, exponent, mod)) {
                    cerr << "FAIL power " << BITS << ' ' << mod << ' ' << decimal(exponent) << ' ' << method << '\n'; std::exit(1); }}
            emit(BITS, STATIC, mod, "powers", "uniform-canonical", 8, exponent, method,
                 measure([&]() { ulng r = 0; for (W a : bases) { r ^= f(a); } sink = r; }));
        };
        report("native", [&](W a) { return nativePower<W, D, FIXED>(a, exponent, mod); });
        report("mini-multiply-power", [&](W a) {
            Mini r = Mini::raw(W(mod != 1)), x = Mini::raw(a); ulll e = exponent;
            for (; e > 1; e >>= 1, x *= x) { if (e & 1) { r *= x; } }
            return (e ? r * x : r).val(); });
        report("mini", [&](W a) { return pow(Mini::raw(a), exponent).val(); });
        report("full", [&](W a) { return pow(Full::raw(a), exponent).val(); });}}

template<uint MOD> void fixed32() { bench<ModIntMini<MOD>, ModInt<MOD>, uint, ulng, MOD>(MOD); }
template<ulng MOD> void fixed64() { bench<ModInt64Mini<MOD>, ModInt64<MOD>, ulng, ulll, MOD>(MOD); }

int main(int argc, char **argv) {
    if (argc != 4) { return 2; }
    rng.seed(std::stoull(argv[1])); repetitions = std::stoi(argv[2]); target_ms = std::stod(argv[3]);
    fixed32<1>(); fixed32<1024>(); fixed32<998244353>(); fixed32<4294967291U>(); fixed32<4294967295U>();
    fixed64<1>(); fixed64<998244353>(); fixed64<2305843009213693951ULL>(); fixed64<1ULL << 63>();
    fixed64<18446744073709551557ULL>(); fixed64<18446744073709551614ULL>(); fixed64<18446744073709551615ULL>();
    for (uint mod : {1U, 1024U, 998244353U, 4294967291U, 4294967295U}) {
        bench<DynModIntMini<891>, DynModInt<891>, uint, ulng>(mod); }
    for (ulng mod : {1ULL, 998244353ULL, 2305843009213693951ULL, 1ULL << 63,
                     18446744073709551557ULL, 18446744073709551614ULL, 18446744073709551615ULL}) {
        bench<DynModInt64Mini<892>, DynModInt64<892>, ulng, ulll>(mod); }
    cerr << "PASS all mini/full/native benchmark outputs\n";}
