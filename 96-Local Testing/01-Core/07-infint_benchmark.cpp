// Timing driver for 07-infint.hpp backends: multiplication, squaring, division, gcd, powMod and radix conversion across sizes; every result is verified.
#include "../../01-Core/07-infint.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using W = infint_detail::W;
static std::mt19937_64 rng(1);
static ulng seedValue = 1;
static int repetitions = 5;
static double minMs = 2;
static std::string only;

static vector<W> randWords(int n, bool ones = false) { vector<W> v(size_t(n), 0); for (W &x : v) { x = ones ? ~W(0) : rng(); } v[size_t(n) - 1] |= W(1) << 63; return v; }
static InfInt ofWords(const vector<W> &w) { return InfInt::fromLimbs(w); }
static ulng digest(const vector<W> &v) { ulng h = 0; for (W x : v) { h = h * 0x9E3779B97F4A7C15ULL + x; } return h; }

// Doubles the iteration count until a pass takes at least minMs, then reports the median of repetitions in nanoseconds per call.
static void measure(const std::string &family, const std::string &backend, const std::string &shape, int n, int m, const std::function<ulng()> &call, ulng expected) {
    if (!only.empty() && family.find(only) == std::string::npos) { return; }
    int iters = 1; double ms = 0;
    while (true) {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) { if (call() != expected) { std::fprintf(stderr, "verification failed in %s/%s %s\n", family.c_str(), backend.c_str(), shape.c_str()); std::exit(1); } }
        ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        if (ms >= minMs || iters >= (1 << 24)) { break; }
        iters *= 2;}
    vector<double> samples;
    for (int r = 0; r < repetitions; ++r) {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) { if (call() != expected) { std::exit(1); } }
        samples.push_back(std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / iters);}
    std::sort(samples.begin(), samples.end());
    std::printf("{\"family\": \"%s\", \"backend\": \"%s\", \"shape\": \"%s\", \"n\": %d, \"m\": %d, \"ns\": %.1f, \"iters\": %d, \"reps\": %d, \"seed\": %llu}\n", family.c_str(), backend.c_str(), shape.c_str(), n, m, samples[size_t(repetitions / 2)], iters, repetitions, static_cast<unsigned long long>(seedValue));
    std::fflush(stdout);}

static void multiplication() {
    using namespace infint_detail;
    vector<int> sizes = {2, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 64, 80, 96, 112, 128, 160, 192, 256, 320, 384, 512, 640, 768, 1024, 1536, 2048, 3072, 4096, 8192, 16384, 32768};
    for (int n : sizes) {
        vector<W> a = randWords(n), b = randWords(n), r(size_t(2 * n), 0);
        auto run = [&](const char *name, auto &&f) { measure("mul", name, "balanced", n, n, [&] { f(); return digest(r); }, [&] { f(); return digest(r); }()); };
        if (n <= 1024) { run("basecase", [&] { mulBasecase(r.data(), a.data(), n, b.data(), n); }); }
        if (n >= 2 && n <= 8192) { run("karatsuba", [&] { karatsuba(r.data(), a.data(), n, b.data(), n); }); }
        if (n >= 6 && n <= 8192) { run("toom3", [&] { toom3(r.data(), a.data(), b.data(), n); }); }
        if (n >= 16) { run("transform", [&] { transformMul(r.data(), a.data(), n, b.data(), n); }); }
        if (n >= 1024) { run("ssa", [&] { schonhageStrassen(r.data(), a.data(), n, b.data(), n); }); }
        run("dispatch", [&] { mul(r.data(), a.data(), n, b.data(), n); });
        auto runSq = [&](const char *name, auto &&f) { measure("square", name, "balanced", n, n, [&] { f(); return digest(r); }, [&] { f(); return digest(r); }()); };
        if (n <= 1024) { runSq("basecase", [&] { squareBasecase(r.data(), a.data(), n); }); }
        if (n >= 2 && n <= 8192) { runSq("karatsuba", [&] { karatsubaSquare(r.data(), a.data(), n); }); }
        if (n >= 6 && n <= 8192) { runSq("toom3", [&] { toom3Square(r.data(), a.data(), n); }); }
        if (n >= 16) { runSq("transform", [&] { transformMul(r.data(), a.data(), n, a.data(), n); }); }
        runSq("dispatch", [&] { square(r.data(), a.data(), n); });
        if (n >= 16) {
            vector<uint> la(size_t(2 * n), 0), lb(size_t(2 * n), 0);
            for (int i = 0; i < n; ++i) { la[size_t(2 * i)] = uint(a[size_t(i)]); la[size_t(2 * i) + 1] = uint(a[size_t(i)] >> 32); lb[size_t(2 * i)] = uint(b[size_t(i)]); lb[size_t(2 * i) + 1] = uint(b[size_t(i)] >> 32); }
            int L = int(std::bit_ceil(uint(4 * n))); vector<W> lo, hi;
            auto runT = [&](const char *name, auto &&f) { measure("transform", name, "cyclic", n, n, [&] { f(); return digest(lo) ^ digest(hi); }, [&] { f(); return digest(lo) ^ digest(hi); }()); };
            if (L <= (1 << 26)) { runT("ntt3", [&] { ntt3Cyclic(lo, hi, la.data(), 2 * n, lb.data(), 2 * n, L); }); }
            runT("ntt64", [&] { ntt64Cyclic(lo, hi, la.data(), 2 * n, lb.data(), 2 * n, L); });
            if (n <= FFT_DOUBLE_MAX) { runT("fft", [&] { fftCyclic(lo, hi, la.data(), 2 * n, lb.data(), 2 * n, L); }); }}}
    for (int n : {64, 256, 1024, 4096, 16384}) {
        for (int m : {4, 16, n / 16, n / 4, n / 2}) {
            if (m >= n || m < 1) { continue; }
            vector<W> a = randWords(n), b = randWords(m), r(size_t(n + m), 0);
            auto run = [&](const char *name, auto &&f) { measure("mul", name, "unbalanced", n, m, [&] { f(); return digest(r); }, [&] { f(); return digest(r); }()); };
            run("basecase", [&] { mulBasecase(r.data(), a.data(), n, b.data(), m); });
            run("unbalanced", [&] { unbalancedMul(r.data(), a.data(), n, b.data(), m, m); });
            if (m >= FFT) { run("split", [&] { unbalancedMul(r.data(), a.data(), n, b.data(), m, int(std::bit_ceil(uint(n + m)) / 2) - m); }); }
            if (m > n / 2) { run("karatsuba", [&] { karatsuba(r.data(), a.data(), n, b.data(), m); }); }
            run("transform", [&] { transformMul(r.data(), a.data(), n, b.data(), m); });
            run("dispatch", [&] { mul(r.data(), a.data(), n, b.data(), m); });}}
    for (int m : {16, 32, 48, 64, 128, 256, 512, 1024, 2048}) {
        int h = (m + 1) / 2, l = m - h, cnt = l + 4;
        vector<W> a = randWords(m), b = randWords(h + 1), r(size_t(cnt), 0), f(size_t(2 * m), 0);
        measure("middle", "middleProduct", "newton", m, h + 1, [&] { middleProduct(r.data(), a.data(), m, b.data(), h + 1, h - 2, cnt); return digest(r); }, [&] { middleProduct(r.data(), a.data(), m, b.data(), h + 1, h - 2, cnt); return digest(r); }());
        measure("middle", "fullProduct", "newton", m, h + 1, [&] { mulAny(f.data(), a.data(), m, b.data(), h + 1); return digest(f); }, [&] { mulAny(f.data(), a.data(), m, b.data(), h + 1); return digest(f); }());
        vector<W> t(size_t(m), 0), c = randWords(m);
        measure("truncated", "truncatedMul", "low", m, m, [&] { truncatedMul(t.data(), c.data(), a.data(), m); return digest(t); }, [&] { truncatedMul(t.data(), c.data(), a.data(), m); return digest(t); }());
        measure("truncated", "fullProduct", "low", m, m, [&] { mulAny(f.data(), c.data(), m, a.data(), m); return digest(f); }, [&] { mulAny(f.data(), c.data(), m, a.data(), m); return digest(f); }());}}

static void division() {
    using namespace infint_detail;
    for (int m : {2, 4, 8, 16, 24, 32, 40, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 4096, 8192}) {
        for (int qn : {m, 2 * m, 8 * m}) {
            if (qn > 65536) { continue; }
            vector<W> b = randWords(m), q = randWords(qn), a(size_t(qn + m), 0); mulAny(a.data(), q.data(), qn, b.data(), m);
            int n = norm(a.data(), qn + m); vector<W> qq(size_t(n - m + 1), 0), rr(size_t(m), 0), iv(size_t(m) + 1);
            auto run = [&](const char *name, auto &&f) { measure("div", name, qn == m ? "equal" : qn == 2 * m ? "double" : "long", n, m, [&] { f(); return digest(qq) ^ digest(rr); }, [&] { f(); return digest(qq) ^ digest(rr); }()); };
            if (m <= 2048) { run("basecase", [&] { divBasecase(qq.data(), rr.data(), a.data(), n, b.data(), m); }); }
            if (m >= 3) { run("burnikelZiegler", [&] { divBurnikelZiegler(qq.data(), rr.data(), a.data(), n, b.data(), m); }); }
            run("newton", [&] { reciprocal(iv.data(), b.data(), m); divNewton(qq.data(), rr.data(), a.data(), n, b.data(), m, iv.data()); });
            reciprocal(iv.data(), b.data(), m);
            run("newtonCached", [&] { divNewton(qq.data(), rr.data(), a.data(), n, b.data(), m, iv.data()); });
            run("dispatch", [&] { divRem(qq.data(), rr.data(), a.data(), n, b.data(), m); });
            if (qn == 2 * m) { measure("reciprocal", "reciprocal", "normalized", m, m, [&] { reciprocal(iv.data(), b.data(), m); return digest(iv); }, [&] { reciprocal(iv.data(), b.data(), m); return digest(iv); }()); }
            if (qn == m) {
                vector<W> bo = b; bo[0] |= 1; vector<W> ao(size_t(qn + m), 0); mulAny(ao.data(), q.data(), qn, bo.data(), m); int no = norm(ao.data(), qn + m); vector<W> qe(size_t(no - m + 1), 0);
                measure("exact", "hensel", "equal", no, m, [&] { divExactHensel(qe.data(), ao.data(), no, bo.data(), m); return digest(qe); }, [&] { divExactHensel(qe.data(), ao.data(), no, bo.data(), m); return digest(qe); }());
                measure("exact", "divRem", "equal", no, m, [&] { divRem(qe.data(), rr.data(), ao.data(), no, bo.data(), m); return digest(qe); }, [&] { divRem(qe.data(), rr.data(), ao.data(), no, bo.data(), m); return digest(qe); }());}}}}

static void gcdFamily() {
    for (int limbs : {2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 320, 384, 512, 768, 1024, 2048, 4096, 8192}) {
        InfInt a = ofWords(randWords((limbs + 1) / 2)), b = ofWords(randWords((limbs + 1) / 2));
        if (limbs & 1) { a >>= 32; b >>= 32; }
        InfInt g = gcd(a, b);
        auto run = [&](const char *name, auto &&f) { measure("gcd", name, "balanced", limbs, limbs, [&] { return ulng(f() % 1000000007ULL); }, ulng(g % 1000000007ULL)); };
        run("dispatch", [&] { return gcd(a, b); });
        if (limbs <= 512) { run("binary", [&] { vector<W> x = a.toWords(), y = b.toWords(); int len = infint_detail::gcdBinary(x.data(), int(x.size()), y.data(), int(y.size())); x.resize(size_t(len)); return ofWords(x); }); }
        run("lehmer", [&] { InfInt x = a, y = b, A, B, C, D; while (!y.isNil()) { if (x < y) { std::swap(x, y); } if (y.fitsWord()) { ulng u = y.low64(); x %= u; x = std::gcd(u, x.low64()); break; } if (x.n.size() > y.n.size() || !InfInt::lehmerHalfGcd<false>(x, y, A, B, C, D)) { x %= y; } } return x; });
        if (limbs >= 64) { run("halfGcd", [&] { InfInt x = a, y = b, A, B, C, D; while (!y.isNil()) { if (x < y) { std::swap(x, y); } if (y.fitsWord()) { ulng u = y.low64(); x %= u; x = std::gcd(u, x.low64()); break; } if (!InfInt::halfGcd<false>(x, y, A, B, C, D)) { if (x.n.size() > y.n.size() || !InfInt::lehmerHalfGcd<false>(x, y, A, B, C, D)) { x %= y; } } } return x; }); }
        InfInt cx, cy; InfInt eg = exGcd(a, b, cx, cy);
        measure("exgcd", "dispatch", "balanced", limbs, limbs, [&] { InfInt x, y; return ulng(exGcd(a, b, x, y) % 1000000007ULL) ^ ulng(x % 1000000007ULL); }, ulng(eg % 1000000007ULL) ^ ulng(cx % 1000000007ULL));}}

static void modularPower() {
    for (int bits : {64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384}) {
        InfInt m = ofWords(randWords(bits / 64)); m.n[0] |= 1; InfInt e = ofWords(randWords(bits / 64)), a = ofWords(randWords(bits / 64)) % m;
        InfInt expected = powMod(a, e, m);
        auto run = [&](const char *name, bool montgomery) {
            measure("powMod", name, "odd", bits / 64, bits / 64, [&] {
                InfInt::ModCtx ctx(m, montgomery); vector<W> x = ctx.enter(a), r = ctx.enter(InfInt(1)), tmp;
                for (int i = bitLength(e) - 1; i >= 0; --i) { ctx.mul(r, r, tmp); if (testBit(e, i)) { ctx.mul(r, x, tmp); } }
                return ulng(ctx.leave(r) % 1000000007ULL);}, ulng(expected % 1000000007ULL));};
        run("montgomery", true); run("reciprocal", false);
        measure("powMod", "windowed", "odd", bits / 64, bits / 64, [&] { return ulng(powMod(a, e, m) % 1000000007ULL); }, ulng(expected % 1000000007ULL));
        InfInt me = m - 1; InfInt expectedEven = powMod(a, e, me);
        measure("powMod", "windowed", "even", bits / 64, bits / 64, [&] { return ulng(powMod(a, e, me) % 1000000007ULL); }, ulng(expectedEven % 1000000007ULL));}}

static void radix() {
    for (int words : {1, 4, 16, 64, 256, 1024, 4096, 16384, 65536}) {
        InfInt a = ofWords(randWords(words));
        std::string dec = a.toString(10), hex = a.toString(16), b36 = a.toString(36);
        measure("radix", "print10", "decimal", words, int(dec.size()), [&] { return std::hash<std::string>{}(a.toString(10)); }, std::hash<std::string>{}(dec));
        measure("radix", "parse10", "decimal", words, int(dec.size()), [&] { InfInt x(dec); return x.n[0]; }, a.n[0]);
        measure("radix", "print16", "hex", words, int(hex.size()), [&] { return std::hash<std::string>{}(a.toString(16)); }, std::hash<std::string>{}(hex));
        measure("radix", "parse16", "hex", words, int(hex.size()), [&] { InfInt x(hex, 16); return x.n[0]; }, a.n[0]);
        if (words <= 4096) {
            measure("radix", "print36", "base36", words, int(b36.size()), [&] { return std::hash<std::string>{}(a.toString(36)); }, std::hash<std::string>{}(b36));
            measure("radix", "parse36", "base36", words, int(b36.size()), [&] { InfInt x(b36, 36); return x.n[0]; }, a.n[0]);}
        if (words <= 1024) {
            vector<uint> d = a.toDigits(1000000000u);
            measure("radix", "toDigits1e9", "digits", words, int(d.size()), [&] { return ulng(a.toDigits(1000000000u).size()); }, ulng(d.size()));
            measure("radix", "fromDigits1e9", "digits", words, int(d.size()), [&] { return InfInt(d, 1000000000u).n[0]; }, a.n[0]);}}
    for (int words : {4, 64, 1024, 16384}) {
        InfInt a = ofWords(randWords(words));
        measure("roots", "isqrtRem", "root", words, words, [&] { return isqrtRem(a).first.n[0]; }, isqrtRem(a).first.n[0]);
        measure("roots", "kthRoot3", "root", words, words, [&] { return kthRootRem(a, 3).first.n[0]; }, kthRootRem(a, 3).first.n[0]);
        measure("bits", "popcount", "bits", words, words, [&] { return ulng(popcount(a)); }, ulng(popcount(a)));
        measure("bits", "shiftLeft", "bits", words, words, [&] { return (a << 13).n[1]; }, (a << 13).n[1]);
        InfInt p = ofWords(randWords(words)); p.n[0] |= 1;
        if (words <= 64) { measure("prime", "isProbablePrime", "odd", words, words, [&] { return ulng(isProbablePrime(p)); }, ulng(isProbablePrime(p))); }}}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--seed" && i + 1 < argc) { seedValue = std::strtoull(argv[++i], nullptr, 10); }
        else if (arg == "--repetitions" && i + 1 < argc) { repetitions = std::atoi(argv[++i]); }
        else if (arg == "--milliseconds" && i + 1 < argc) { minMs = std::atof(argv[++i]); }
        else if (arg == "--only" && i + 1 < argc) { only = argv[++i]; }
        else { std::fprintf(stderr, "usage: --seed N --repetitions R --milliseconds MS --only family\n"); return 2; }}
    rng.seed(seedValue);
    multiplication(); division(); gcdFamily(); modularPower(); radix();
    return 0;}
