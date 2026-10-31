// Timing driver for 07-infint.hpp: backends of multiplication, squaring, division, gcd, powMod and radix conversion, small values, text I/O, add/sub and a cold maximal product; every result is verified.
#include "../../01-Core/07-infint.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <sys/resource.h>
#include <string>
#include <vector>

using W = infint_detail::W;
static std::mt19937_64 rng(1);
static ulng seedValue = 1;
static int repetitions = 5;
static double minMs = 2;
static std::string only;

static vector<W> randWords(int n, bool ones = false, std::mt19937_64 &g = rng) { vector<W> v(size_t(n), 0); for (W &x : v) { x = ones ? ~W(0) : g(); } v[size_t(n) - 1] |= W(1) << 63; return v; }
static InfInt ofWords(const vector<W> &w) { return InfInt::fromLimbs(w); }
static constexpr ulng K = 0x9E3779B97F4A7C15ULL;
static ulng digest(const vector<W> &v) { ulng h = 0; for (W x : v) { h = h * K + x; } return h; }
static ulng digest(const InfInt &x) { ulng h = ulng(x.sgn); for (W w : x.n) { h = h * K + w; } return h; }
// Digest of the length, about cnt evenly spaced entries and the last one: verifies outputs too long to hash on every timed call.
template<typename T> static ulng sampled(const T *p, size_t n, size_t cnt = 512) { ulng h = n; size_t step = max<size_t>(1, n / cnt); for (size_t i = 0; i < n; i += step) { h = h * K + ulng(p[i]); } return n ? h * K + ulng(p[n - 1]) : h; }
static ulng sampled(const InfInt &x, size_t cnt = 512) { return sampled(x.n.data(), size_t(x.n.size()), cnt) * K + ulng(x.sgn); }
static ulng sampled(string_view s) { return sampled(s.data(), s.size()); }
static void require(bool ok, const char *what) { if (!ok) { std::fprintf(stderr, "verification failed: %s\n", what); std::exit(1); } }
static bool wanted(const char *family) { return only.empty() || std::string(family).find(only) != std::string::npos; }

// Doubles the iteration count until a pass takes at least minMs, then reports the median of repetitions in nanoseconds per call; extra: further JSON fields.
static void measure(const std::string &family, const std::string &backend, const std::string &shape, int n, int m, const std::function<ulng()> &call, ulng expected, int per = 1, const std::string &extra = "") {
    if (!wanted(family.c_str())) { return; }
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
        samples.push_back(std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / iters / per);}
    std::sort(samples.begin(), samples.end());
    std::printf("{\"family\": \"%s\", \"backend\": \"%s\", \"shape\": \"%s\", \"n\": %d, \"m\": %d, \"ns\": %.3f, \"iters\": %d, \"reps\": %d, \"per\": %d, \"seed\": %llu%s}\n", family.c_str(), backend.c_str(), shape.c_str(), n, m, samples[size_t(repetitions / 2)], iters, repetitions, per, static_cast<unsigned long long>(seedValue), extra.c_str());
    std::fflush(stdout);}
static std::string planJson(const infint_detail::FftPlan &p) { char s[96]; std::snprintf(s, sizeof s, ", \"plan\": {\"lg\": %d, \"r\": %d, \"k\": %d}", p.lg, p.r, p.k); return s; }

// 32-bit limb image of words, and the words of a transform's limb coefficients lo + hi * 2^64 (carried as transformMul does).
static vector<uint> limbs(const vector<W> &a) { vector<uint> l(2 * a.size()); for (size_t i = 0; i < a.size(); ++i) { l[2 * i] = uint(a[i]); l[2 * i + 1] = uint(a[i] >> 32); } return l; }
static void carryLimbs(W *r, int words, const W *lo, const W *hi) {
    unsigned __int128 carry = 0;
    for (int i = 0; i < words; ++i) {
        unsigned __int128 c0 = lo[2 * i] + ((unsigned __int128)hi[2 * i] << 64) + carry;
        W low = uint(c0); carry = c0 >> 32;
        unsigned __int128 c1 = lo[2 * i + 1] + ((unsigned __int128)hi[2 * i + 1] << 64) + carry;
        r[i] = low | (W(uint(c1)) << 32); carry = c1 >> 32;}}
static constexpr ulng P61 = (ulng(1) << 61) - 1;
static ulng modP(const vector<W> &v) { return (ofWords(v) % P61).low64(); }
// Reference product a * b: the exact three-prime NTT's limb convolution carried into words, checked mod 2^61 - 1 against the operands.
static vector<W> refProduct(const vector<W> &a, const vector<W> &b) {
    int n = int(a.size()), m = int(b.size()), L = int(std::bit_ceil(uint(2 * (n + m))));
    vector<uint> la = limbs(a), lb = limbs(b); vector<W> lo(size_t(L), 0), hi(size_t(L), 0), r(size_t(n + m), 0);
    infint_detail::ntt3Cyclic(lo.data(), hi.data(), la.data(), 2 * n, lb.data(), 2 * m, L);
    carryLimbs(r.data(), n + m, lo.data(), hi.data());
    require(modP(r) == ulng((unsigned __int128)modP(a) * modP(b) % P61), "reference product mod 2^61 - 1");
    return r;}

static void multiplication() {
    using namespace infint_detail;
    vector<int> sizes = {2, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 64, 80, 96, 112, 128, 160, 192, 256, 320, 384, 512, 640, 768, 1024, 1536, 2048, 3072, 4096, 8192, 16384, 32768, 65536, 100000, 131072, 262144};
    for (int n : sizes) {
        vector<W> a = randWords(n), b = randWords(n), r(size_t(2 * n), 0);
        ulng want = digest(refProduct(a, b)), wantSq = digest(refProduct(a, a));
        auto run = [&](const char *name, auto &&f) { measure("mul", name, "balanced", n, n, [&] { f(); return digest(r); }, want); };
        if (n <= 1024) { run("basecase", [&] { mulBasecase(r.data(), a.data(), n, b.data(), n); }); }
        if (n >= 2 && n <= 8192) { run("karatsuba", [&] { karatsuba(r.data(), a.data(), n, b.data(), n); }); }
        if (n >= 6 && n <= 8192) { run("toom3", [&] { toom3(r.data(), a.data(), b.data(), n); }); }
        if (n >= 16) { run("transform", [&] { transformMul(r.data(), a.data(), n, b.data(), n); }); }
        if (n >= 1024) { run("ssa", [&] { schonhageStrassen(r.data(), a.data(), n, b.data(), n); }); }
        run("dispatch", [&] { mul(r.data(), a.data(), n, b.data(), n); });
        auto runSq = [&](const char *name, auto &&f) { measure("square", name, "balanced", n, n, [&] { f(); return digest(r); }, wantSq); };
        if (n <= 1024) { runSq("basecase", [&] { squareBasecase(r.data(), a.data(), n); }); }
        if (n >= 2 && n <= 8192) { runSq("karatsuba", [&] { karatsubaSquare(r.data(), a.data(), n); }); }
        if (n >= 6 && n <= 8192) { runSq("toom3", [&] { toom3Square(r.data(), a.data(), n); }); }
        if (n >= 16) { runSq("transform", [&] { transformMul(r.data(), a.data(), n, a.data(), n); }); }
        runSq("dispatch", [&] { square(r.data(), a.data(), n); });
        // The full 2n-word product through each transform: the NTTs' cyclic limb convolution of bit_ceil(4n) limbs (checked once against the reference after the carry) and the planned double FFT.
        if (n >= 16) {
            vector<uint> la = limbs(a), lb = limbs(b);
            int L = int(std::bit_ceil(uint(4 * n))); vector<W> lo(static_cast<size_t>(L)), hi(static_cast<size_t>(L)), w(size_t(2 * n));
            auto runT = [&](const char *name, auto &&f) {
                f(); carryLimbs(w.data(), 2 * n, lo.data(), hi.data()); require(digest(w) == want, name);
                measure("transform", name, "cyclic", n, n, [&] { f(); return digest(lo) ^ digest(hi); }, digest(lo) ^ digest(hi));};
            if (L <= (1 << 26)) { runT("ntt3", [&] { ntt3Cyclic(lo.data(), hi.data(), la.data(), 2 * n, lb.data(), 2 * n, L); }); }
            runT("ntt64", [&] { ntt64Cyclic(lo.data(), hi.data(), la.data(), 2 * n, lb.data(), 2 * n, L); });
            FftPlan p = fftPlan(n, n, 2 * n, false);
            if (p.lg) { measure("transform", "fft", "cyclic", n, n, [&] { fftRun(r.data(), p, a.data(), n, b.data(), n, false); return digest(r); }, want, 1, planJson(p)); }}}
    for (int n : {64, 256, 1024, 4096, 16384, 65536, 131072}) {
        for (int m : {4, 8, 16, 24, 32, 48, 64, 80, 128, 256, 512, n / 16, n / 4, n / 2}) {
            if (m >= n || m < 1 || (n >= 65536 && m >= n / 16)) { continue; }
            vector<W> a = randWords(n), b = randWords(m), r(size_t(n + m), 0);
            ulng want = digest(refProduct(a, b));
            auto run = [&](const char *name, auto &&f) { measure("mul", name, "unbalanced", n, m, [&] { f(); return digest(r); }, want); };
            run("basecase", [&] { mulBasecase(r.data(), a.data(), n, b.data(), m); });
            run("unbalanced", [&] { unbalancedMul(r.data(), a.data(), n, b.data(), m, m); });
            if (m >= FFT) { run("split", [&] { unbalancedMul(r.data(), a.data(), n, b.data(), m, int(std::bit_ceil(uint(n + m)) / 2) - m); }); }
            if (m > n / 2) { run("karatsuba", [&] { karatsuba(r.data(), a.data(), n, b.data(), m); }); }
            run("transform", [&] { transformMul(r.data(), a.data(), n, b.data(), m); });
            run("dispatch", [&] { mul(r.data(), a.data(), n, b.data(), m); });}}
    // The one-sided inverse (B^m + v within one unit below floor(B^2m / d), checked exactly), the exact inverse, the cyclic residue of an m-word square at want = m + 1 against the
    // linear transform and the dispatcher's full product, then the truncated product against the low words of the reference.
    for (int m : {16, 32, 48, 64, 96, 128, 160, 192, 224, 256, 320, 384, 448, 512, 640, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 16384, 32768, 65536, 131072}) {
        vector<W> d = randWords(m), v(size_t(m) + 1, 0), x(size_t(m) + 1, 0);
        invertExact(x.data(), d.data(), m); InfInt X = ofWords(x), B2 = InfInt(1) << (128 * m), D = ofWords(d);
        require(D * X <= B2 && D * (X + 1) > B2, "invertExact floor");
        invert(v.data(), d.data(), m); v[size_t(m)] = 1; InfInt V = ofWords(v);
        require(V == X || V + 1 == X, "invert within one unit below the floor");
        measure("invert", "invert", "normalized", m, m, [&] { invert(v.data(), d.data(), m); return digest(v); }, digest(v));
        if (m <= 65536) { measure("invert", "invertExact", "normalized", m, m, [&] { invertExact(x.data(), d.data(), m); return digest(x); }, digest(x)); }
        vector<W> a = randWords(m), b = randWords(m), full = refProduct(a, b), f2(size_t(2 * m), 0);
        CyclicPlan c = cyclicPlan(m, m, m + 1); vector<W> s(size_t(c.len), 0);
        mulCyclic(s.data(), c, a.data(), m, b.data(), m);
        require(ofWords(s) == ofWords(full) % ((InfInt(1) << (64 * c.len)) - 1), "mulCyclic residue");
        measure("cyclic", "mulCyclic", "residue", m, m, [&] { mulCyclic(s.data(), c, a.data(), m, b.data(), m); return digest(s); }, digest(s), 1, c.p.lg ? planJson(c.p) : "");
        if (m >= 16) { FftPlan p = fftPlan(m, m, 2 * m, false); if (p.lg) { measure("cyclic", "transform", "residue", m, m, [&] { fftRun(f2.data(), p, a.data(), m, b.data(), m, false); return digest(f2); }, digest(full), 1, planJson(p)); } }
        measure("cyclic", "dispatch", "residue", m, m, [&] { mulAny(f2.data(), a.data(), m, b.data(), m); return digest(f2); }, digest(full));
        if (m <= 2048) {
            vector<W> t(size_t(m), 0);
            measure("truncated", "truncatedMul", "low", m, m, [&] { truncatedMul(t.data(), a.data(), b.data(), m); return digest(t); }, digest(vector<W>(full.begin(), full.begin() + m)));
            measure("truncated", "fullProduct", "low", m, m, [&] { mulAny(f2.data(), a.data(), m, b.data(), m); return digest(f2); }, digest(full));}}}

// Division: Knuth D, Burnikel-Ziegler, blocks with a fresh context (the dispatcher's block choice and the alternatives) and with a cached one, the dispatcher; then the exact division
// backends, the short-quotient grid (Knuth D and Burnikel-Ziegler against one block of the quotient's size, fresh and cached, with the cyclic and the linear remainder product) and the inverse alone.
static void division() {
    using namespace infint_detail;
    auto blocksOf = [](int qn, int m) { int nb = qn > m ? (qn + m - 1) / m : 3 * qn > m ? 2 : 1; return (qn + nb - 1) / nb; };
    for (int m : {2, 4, 8, 16, 24, 32, 40, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 4096, 8192, 16384, 32768, 65536, 131072}) {
        for (int qn : {m, 2 * m, 8 * m}) {
            if (qn > 262144 || (m > 8192 && qn > 2 * m)) { continue; }
            vector<W> b = randWords(m), q = randWords(qn), a(size_t(qn + m), 0); mulAny(a.data(), q.data(), qn, b.data(), m);
            int n = norm(a.data(), qn + m); vector<W> qq(size_t(n - m + 1), 0), rr(size_t(m), 0);
            const char *shape = qn == m ? "equal" : qn == 2 * m ? "double" : "long";
            auto run = [&](const std::string &name, auto &&f) {
                f(); require(ofWords(qq) * ofWords(b) + ofWords(rr) == ofWords(a) && ofWords(rr) < ofWords(b), "division q * b + r == a, r < b");
                measure("div", name, shape, n, m, [&] { f(); return digest(qq) ^ digest(rr); }, digest(qq) ^ digest(rr));};
            if (m <= 2048 && qn <= 16384) { run("basecase", [&] { divBasecase(qq.data(), rr.data(), a.data(), n, b.data(), m); }); }
            if (m >= 3 && m <= 16384) { run("burnikelZiegler", [&] { divBurnikelZiegler(qq.data(), rr.data(), a.data(), n, b.data(), m); }); }
            int blk = blocksOf(n - m, m);
            auto fresh = [&](int bl) { Divisor dv; Scratch ctx(Divisor::words(m, bl)); dv.prepare(ctx, b.data(), m, bl); divBlocks(qq.data(), rr.data(), a.data(), n, dv); };
            if (m >= 2) {
                run("blocks", [&] { fresh(blk); });
                for (int bl : {m, (m + 1) / 2, (m + 2) / 3}) { if (bl != blk && bl >= 1 && bl <= n - m) { run("blocks" + std::to_string(bl == m ? 1 : bl == (m + 1) / 2 ? 2 : 3), [&] { fresh(bl); }); } }
                Divisor dv; Scratch ctx(Divisor::words(m, blk)); dv.prepare(ctx, b.data(), m, blk);
                run("blocksCached", [&] { divBlocks(qq.data(), rr.data(), a.data(), n, dv); });
                if (blk != m) { Divisor dm; Scratch cm(Divisor::words(m, m)); dm.prepare(cm, b.data(), m, m); run("blocksCachedFull", [&] { divBlocks(qq.data(), rr.data(), a.data(), n, dm); }); }}
            run("dispatch", [&] { divRem(qq.data(), rr.data(), a.data(), n, b.data(), m); });
            if (qn == m && m <= 65536) {
                vector<W> bo = b; bo[0] |= 1; vector<W> ao(size_t(qn + m), 0); mulAny(ao.data(), q.data(), qn, bo.data(), m); int no = norm(ao.data(), qn + m); vector<W> qe(size_t(no - m + 1), 0);
                auto runExact = [&](const char *name, auto &&f) {
                    f(); require(ofWords(qe) * ofWords(bo) == ofWords(ao), "exact division q * b == a");
                    measure("exact", name, "equal", no, m, [&] { f(); return digest(qe); }, digest(qe));};
                if (m <= 4096) { runExact("hensel", [&] { divExactHensel(qe.data(), ao.data(), no, bo.data(), m); }); }
                runExact("divRem", [&] { divRem(qe.data(), rr.data(), ao.data(), no, bo.data(), m); });
                runExact("divRemExact", [&] { divRem(qe.data(), nullptr, ao.data(), no, bo.data(), m, nullptr, true); });}}}
    for (int m : {64, 128, 256, 512, 1024, 4096, 16384, 65536}) {
        for (int qn : {2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 512}) {
            if (2 * qn > m) { continue; }
            vector<W> b = randWords(m), q = randWords(qn), a(size_t(qn + m), 0); mulAny(a.data(), q.data(), qn, b.data(), m);
            int n = norm(a.data(), qn + m); vector<W> qq(size_t(n - m + 1), 0), rr(size_t(m), 0);
            auto run = [&](const char *name, auto &&f) {
                f(); require(ofWords(qq) * ofWords(b) + ofWords(rr) == ofWords(a) && ofWords(rr) < ofWords(b), "short division q * b + r == a, r < b");
                measure("divShort", name, "short", n, m, [&] { f(); return digest(qq) ^ digest(rr); }, digest(qq) ^ digest(rr));};
            run("basecase", [&] { divBasecase(qq.data(), rr.data(), a.data(), n, b.data(), m); });
            if (m <= 16384) { run("burnikelZiegler", [&] { divBurnikelZiegler(qq.data(), rr.data(), a.data(), n, b.data(), m); }); }
            int blk = n - m;
            run("blocks", [&] { Divisor dv; Scratch ctx(Divisor::words(m, blk)); dv.prepare(ctx, b.data(), m, blk); divBlocks(qq.data(), rr.data(), a.data(), n, dv); });
            Divisor dv; Scratch ctx(Divisor::words(m, blk)); dv.prepare(ctx, b.data(), m, blk);
            run("blocksCached", [&] { divBlocks(qq.data(), rr.data(), a.data(), n, dv); });
            CyclicPlan cyc = cyclicPlan(m, blk, m + 1); Scratch sp(size_t(Spectrum::words(cyc.p)));
            Divisor dc = dv; dc.cr = cyc; if (cyc.p.lg) { dc.sd.take(sp, cyc.p, dv.d, m, blk); }
            run("blocksCyclic", [&] { divBlocks(qq.data(), rr.data(), a.data(), n, dc); });
            Divisor dl; Scratch cl(Divisor::words(m, blk)); dl.prepare(cl, b.data(), m, blk, false);
            run("blocksLinear", [&] { divBlocks(qq.data(), rr.data(), a.data(), n, dl); });
            run("dispatch", [&] { divRem(qq.data(), rr.data(), a.data(), n, b.data(), m); });}}}

static void gcdFamily() {
    for (int limbs : {2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 320, 384, 512, 768, 1024, 2048, 4096, 8192}) {
        InfInt a = ofWords(randWords((limbs + 1) / 2)), b = ofWords(randWords((limbs + 1) / 2));
        if (limbs & 1) { a >>= 32; b >>= 32; }
        InfInt g = gcd(a, b);
        auto run = [&](const char *name, auto &&f) { measure("gcd", name, "balanced", limbs, limbs, [&] { return ulng(f() % 1000000007ULL); }, ulng(g % 1000000007ULL)); };
        run("dispatch", [&] { return gcd(a, b); });
        if (limbs <= 512) { run("binary", [&] { vector<W> x(a.n.begin(), a.n.end()), y(b.n.begin(), b.n.end()); int len = infint_detail::gcdBinary(x.data(), int(x.size()), y.data(), int(y.size())); x.resize(size_t(len)); return ofWords(x); }); }
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
                InfInt::ModCtx ctx(m, montgomery); InfInt::Words x = ctx.enter(a), r = ctx.enter(InfInt(1));
                for (int i = bitLength(e) - 1; i >= 0; --i) { ctx.mul(r, r); if (testBit(e, i)) { ctx.mul(r, x); } }
                return ulng(ctx.leave(std::move(r)) % 1000000007ULL);}, ulng(expected % 1000000007ULL));};
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

// Independent decimal text of little-endian words: repeated division by 10^19 in 128-bit arithmetic.
static std::string refDecimal(vector<W> w) {
    constexpr ulng E19 = 10000000000000000000ULL;
    std::string out;
    while (!w.empty()) {
        unsigned __int128 rem = 0;
        for (size_t i = w.size(); i-- > 0;) { unsigned __int128 cur = (rem << 64) | w[i]; w[i] = W(cur / E19); rem = cur % E19; }
        while (!w.empty() && w.back() == 0) { w.pop_back(); }
        for (int d = 0; d < 19; ++d) { out += char('0' + int(rem % 10)); rem /= 10; }}
    while (out.size() > 1 && out.back() == '0') { out.pop_back(); }
    std::reverse(out.begin(), out.end());
    return out.empty() ? "0" : out;}
static ulng textDigest(const char *p, size_t len) { ulng lo = 0, hi = 0; std::memcpy(&lo, p, 8); std::memcpy(&hi, p + len - 8, 8); return (len * K + lo) * K + hi; }

// Small values: passes of 10^6 operations over 1024 operands of k words, nanoseconds per operation.
static void smallValues() {
    if (!wanted("small")) { return; }
    constexpr int OPS = 1000000, POOL = 1024, MASK = POOL - 1;
    std::mt19937_64 g(seedValue + 1);
    for (int k = 1; k <= 4; ++k) {
        vector<InfInt> x, y, add, den; vector<std::string> dec, d19; vector<ulng> v19;
        for (int i = 0; i < POOL; ++i) {
            vector<W> w = randWords(k, false, g);
            x.push_back(ofWords(w)); dec.push_back(refDecimal(w)); y.push_back(ofWords(randWords(k, false, g)));
            vector<W> s = randWords(k, false, g); s.back() >>= 24;   // top word in [2^39, 2^40): 10^6 additions stay within k words
            add.push_back(ofWords(s));
            vector<W> d = randWords((k + 1) / 2, false, g); if (k % 2) { d.back() >>= 32; }   // a 32k-bit divisor
            den.push_back(ofWords(d));
            v19.push_back(1000000000000000000ULL + g() % 9000000000000000000ULL); d19.push_back(std::to_string(v19.back()));}
        // +=: the final sum against add[0] + (OPS / POOL) * sum(add) + sum(add[0, OPS % POOL)).
        InfInt total, expectSum = add[0];
        for (int i = 0; i < POOL; ++i) { total += add[size_t(i)]; }
        expectSum += total * (OPS / POOL);
        for (int i = 0; i < OPS % POOL; ++i) { expectSum += add[size_t(i)]; }
        measure("small", "add", "+=", k, k, [&] { InfInt acc = add[0]; for (int i = 0; i < OPS; ++i) { acc += add[size_t(i & MASK)]; } return digest(acc); }, digest(expectSum), OPS);
        // *=: against a 128-bit schoolbook product.
        ulng expectMul = 0;
        for (int i = 0; i < OPS; ++i) {
            const InfInt &a = x[size_t(i & MASK)], &b = y[size_t((7 * i + 1) & MASK)];
            W r[8] = {};
            for (int p = 0; p < k; ++p) { W c = 0; for (int q = 0; q < k; ++q) { unsigned __int128 t = (unsigned __int128)a.n[p] * b.n[q] + r[p + q] + c; r[p + q] = W(t); c = W(t >> 64); } r[p + k] = c; }
            int len = infint_detail::norm(r, 2 * k);
            expectMul = expectMul * K + r[0] + r[len - 1] + ulng(len);}
        measure("small", "mul", "*=", k, k, [&] {
            ulng h = 0;
            for (int i = 0; i < OPS; ++i) { InfInt z = x[size_t(i & MASK)]; z *= y[size_t((7 * i + 1) & MASK)]; h = h * K + z.n[0] + z.n.back() + ulng(z.n.size()); }
            return h;}, expectMul, OPS);
        // divMod: q * d + r == x and 0 <= r < d checked once, then the digest of (q, r).
        ulng expectDiv = 0;
        for (int i = 0; i < OPS; ++i) {
            const InfInt &a = x[size_t(i & MASK)], &d = den[size_t((7 * i + 1) & MASK)];
            auto [q, r] = divMod(a, d);
            if (i < POOL) { require(q * d + r == a && r < d && r >= 0, "small divMod"); }
            expectDiv = expectDiv * K + q.low64() + 3 * r.low64();}
        measure("small", "divMod", "32k-bit divisor", k, (k + 1) / 2, [&] {
            ulng h = 0;
            for (int i = 0; i < OPS; ++i) { auto [q, r] = divMod(x[size_t(i & MASK)], den[size_t((7 * i + 1) & MASK)]); h = h * K + q.low64() + 3 * r.low64(); }
            return h;}, expectDiv, OPS);
        // read: 19-digit decimals (k = 1, values from std::to_string) or the reference decimal text of the k-word operands.
        const vector<std::string> &texts = k == 1 ? d19 : dec;
        ulng expectRead = 0;
        for (int i = 0; i < OPS; ++i) { int j = i & MASK; expectRead = expectRead * K + (k == 1 ? v19[size_t(j)] + v19[size_t(j)] : x[size_t(j)].n[0] + x[size_t(j)].n.back()); }
        measure("small", "read", "decimal", k, int(texts[0].size()), [&] {
            ulng h = 0; InfInt v;
            for (int i = 0; i < OPS; ++i) { if (!v.read(texts[size_t(i & MASK)])) { return ulng(0); } h = h * K + v.n[0] + v.n.back(); }
            return h;}, expectRead, OPS);
        // toString and toChars into a stack buffer against the reference decimal text.
        ulng expectText = 0;
        for (int i = 0; i < OPS; ++i) { const std::string &t = dec[size_t(i & MASK)]; expectText = expectText * K + textDigest(t.data(), t.size()); }
        measure("small", "toString", "decimal", k, int(dec[0].size()), [&] {
            ulng h = 0;
            for (int i = 0; i < OPS; ++i) { std::string t = x[size_t(i & MASK)].toString(); h = h * K + textDigest(t.data(), t.size()); }
            return h;}, expectText, OPS);
        measure("small", "toChars", "decimal", k, int(dec[0].size()), [&] {
            ulng h = 0; char buf[96];
            for (int i = 0; i < OPS; ++i) { auto res = x[size_t(i & MASK)].toChars(buf, buf + 96); h = h * K + textDigest(buf, size_t(res.ptr - buf)); }
            return h;}, expectText, OPS);}}

// Long decimal and hex texts through read, toString, toChars and the stream operators (one reused stream positioned at its start).
static void largeText() {
    if (!wanted("text")) { return; }
    constexpr ulng P = (ulng(1) << 61) - 1;
    std::mt19937_64 g(seedValue + 2);
    for (uint base : {10u, 16u}) {
        for (int len : {100000, 1000000, 3200000}) {
            std::string s(size_t(len), '0');
            for (char &c : s) { c = InfInt::DIGITS[g() % base]; }
            s[0] = InfInt::DIGITS[1 + g() % (base - 1)];
            InfInt v; require(v.read(s, base), "text read");
            // Independent checks: the value mod 2^61 - 1 by Horner over the digits, and for hex the words assembled from the digits.
            ulng horner = 0;
            for (char c : s) { horner = ulng(((unsigned __int128)horner * base + InfInt::DIGIT_VALUE[size_t(uint8_t(c))]) % P); }
            require((v % P).low64() == horner, "text value mod 2^61 - 1");
            if (base == 16) {
                vector<W> w(size_t((len + 15) / 16), 0);
                for (int i = 0; i < len; ++i) { w[size_t(i / 16)] |= W(InfInt::DIGIT_VALUE[size_t(uint8_t(s[size_t(len - 1 - i)]))]) << (4 * (i % 16)); }
                require(v == ofWords(w), "text hex words");}
            require(v.toString(base) == s, "text toString");
            const char *shape = base == 10 ? "decimal" : "hex";
            int words = v.n.size(); ulng expectValue = sampled(v), expectText = sampled(string_view(s));
            measure("text", "read", shape, words, len, [&] { InfInt x; return x.read(s, base) ? sampled(x) : 0; }, expectValue);
            measure("text", "toString", shape, words, len, [&] { return sampled(string_view(v.toString(base))); }, expectText);
            vector<char> buf(size_t(v.charsBound(base)));
            measure("text", "toChars", shape, words, len, [&] { auto res = v.toChars(buf.data(), buf.data() + buf.size(), base); return sampled(string_view(buf.data(), size_t(res.ptr - buf.data()))); }, expectText);
            std::istringstream is(s); std::ostringstream os;
            if (base == 16) { is.setf(std::ios_base::hex, std::ios_base::basefield); os.setf(std::ios_base::hex, std::ios_base::basefield); }
            { InfInt x; is >> x; require(x == v && is.eof() && !is.fail(), "text operator>>"); os << v; require(os.view() == s, "text operator<<"); }
            measure("text", "istream", shape, words, len, [&] { is.clear(); is.seekg(0); InfInt x; is >> x; return is.fail() ? 0 : sampled(x); }, expectValue);
            measure("text", "ostream", shape, words, len, [&] { os.seekp(0); os << v; return sampled(os.view()); }, expectText);}}}

// a + b and a - b of n-word operands against 128-bit reference loops, nanoseconds per word: InfInt operators (with and without a carry
// out of the top word) and the add/sub kernels into a preallocated buffer.
static void addSub() {
    if (!wanted("addsub")) { return; }
    std::mt19937_64 g(seedValue + 3);
    for (int lg : {10, 16, 20}) {
        int n = 1 << lg;
        vector<W> wa = randWords(n, false, g), wb = randWords(n, false, g), wc = wb, sum(static_cast<size_t>(n) + 1), diff(static_cast<size_t>(n)), r(static_cast<size_t>(n));
        wc.back() >>= 1;   // wa > wc
        auto reference = [&](const vector<W> &a, const vector<W> &b, int sign) {
            W c = 0;
            for (size_t i = 0; i < a.size(); ++i) {
                unsigned __int128 t = sign > 0 ? (unsigned __int128)a[i] + b[i] + c : (unsigned __int128)a[i] - b[i] - c;
                (sign > 0 ? sum : diff)[i] = W(t); c = sign > 0 ? W(t >> 64) : W(t >> 64) != 0;}
            if (sign > 0) { sum[a.size()] = c; }};
        InfInt a = ofWords(wa), b = ofWords(wb), c = ofWords(wc);
        reference(wa, wb, 1); require(a + b == ofWords(sum), "addsub a + b");
        ulng expectAdd = sampled(ofWords(sum), 8), expectAddKernel = sampled(sum.data(), size_t(n), 8) + sum[size_t(n)];
        measure("addsub", "a+b", "carry", n, n, [&] { InfInt s = a + b; return sampled(s, 8); }, expectAdd, n);
        measure("addsub", "add", "kernel", n, n, [&] { W cy = infint_detail::add(r.data(), wa.data(), n, wb.data(), n); return sampled(r.data(), size_t(n), 8) + cy; }, expectAddKernel, n);
        vector<W> na = wa, nb = wb; na.back() >>= 2; nb.back() >>= 2;
        InfInt a2 = ofWords(na), b2 = ofWords(nb);
        reference(na, nb, 1); require(sum[size_t(n)] == 0 && a2 + b2 == ofWords(sum), "addsub a + b without carry");
        ulng expectAdd2 = sampled(ofWords(sum), 8);
        measure("addsub", "a+b", "nocarry", n, n, [&] { InfInt s = a2 + b2; return sampled(s, 8); }, expectAdd2, n);
        reference(wa, wc, -1); require(a - c == ofWords(diff), "addsub a - b");
        ulng expectSub = sampled(ofWords(diff), 8), expectSubKernel = sampled(diff.data(), size_t(n), 8);
        measure("addsub", "a-b", "positive", n, n, [&] { InfInt s = a - c; return sampled(s, 8); }, expectSub, n);
        measure("addsub", "sub", "kernel", n, n, [&] { W cy = infint_detail::sub(r.data(), wa.data(), n, wc.data(), n); return sampled(r.data(), size_t(n), 8) + cy; }, expectSubKernel, n);}}

// Repeated maximal product of two 2^17-word operands (the Library Checker max_max shape), first in the process: the first call with its
// page faults and arena and table growth, then the median of the next five; each product checked mod 2^61 - 1 and by its digest.
static void maxProduct() {
    if (!wanted("maxmax")) { return; }
    constexpr int N = 1 << 17, CALLS = 5; constexpr ulng P = (ulng(1) << 61) - 1;
    std::mt19937_64 g(seedValue + 4);
    InfInt a = ofWords(randWords(N, false, g)), b = ofWords(randWords(N, false, g));
    auto timed = [&](ulng &faults) {
        rusage r0, r1; getrusage(RUSAGE_SELF, &r0);
        auto t0 = std::chrono::steady_clock::now();
        InfInt c = a * b;
        double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count();
        getrusage(RUSAGE_SELF, &r1); faults = ulng(r1.ru_minflt - r0.ru_minflt);
        return pair<double, InfInt>{ns, std::move(c)};};
    ulng firstFaults = 0;
    auto [first, c] = timed(firstFaults);
    rusage ru; getrusage(RUSAGE_SELF, &ru);
    require((c % P).low64() == ulng((unsigned __int128)(a % P).low64() * (b % P).low64() % P) && c.n.size() == 2 * N, "maxmax product");
    ulng expect = digest(c);
    vector<double> samples; vector<ulng> faults;
    for (int i = 0; i < CALLS; ++i) { ulng f = 0; auto [ns, d] = timed(f); require(digest(d) == expect, "maxmax repeat"); samples.push_back(ns); faults.push_back(f); }
    std::sort(samples.begin(), samples.end()); std::sort(faults.begin(), faults.end());
    std::printf("{\"family\": \"maxmax\", \"backend\": \"first\", \"shape\": \"balanced\", \"n\": %d, \"m\": %d, \"ns\": %.3f, \"iters\": 1, \"reps\": 1, \"per\": 1, \"seed\": %llu, \"minflt\": %llu, \"maxrss_kb\": %ld}\n", N, N, first, static_cast<unsigned long long>(seedValue), static_cast<unsigned long long>(firstFaults), ru.ru_maxrss);
    std::printf("{\"family\": \"maxmax\", \"backend\": \"warm\", \"shape\": \"balanced\", \"n\": %d, \"m\": %d, \"ns\": %.3f, \"iters\": 1, \"reps\": %d, \"per\": 1, \"seed\": %llu, \"minflt\": %llu}\n", N, N, samples[CALLS / 2], CALLS, static_cast<unsigned long long>(seedValue), static_cast<unsigned long long>(faults[CALLS / 2]));
    std::fflush(stdout);}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--seed" && i + 1 < argc) { seedValue = std::strtoull(argv[++i], nullptr, 10); }
        else if (arg == "--repetitions" && i + 1 < argc) { repetitions = std::atoi(argv[++i]); }
        else if (arg == "--milliseconds" && i + 1 < argc) { minMs = std::atof(argv[++i]); }
        else if (arg == "--only" && i + 1 < argc) { only = argv[++i]; }
        else { std::fprintf(stderr, "usage: --seed N --repetitions R --milliseconds MS --only family\n"); return 2; }}
    rng.seed(seedValue);
    maxProduct();   // first, so its first call meets a cold process
    multiplication(); division(); gcdFamily(); modularPower(); radix(); smallValues(); largeText(); addSub();
    return 0;}
