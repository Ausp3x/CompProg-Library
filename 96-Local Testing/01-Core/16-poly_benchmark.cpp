// Poly/FPS benchmark: medians after doubling warmup, every result folded into a checksum that is verified.
#include "../../01-Core/16-poly.hpp"
#include <chrono>
#include <cstdio>
#include <functional>
#include <random>
#include <string>

using M = mint;
using Clock = std::chrono::steady_clock;
static std::mt19937_64 rng(1);
static int reps = 5;
static double targetMs = 3;
static ulng sink = 0;

static Poly<M> rnd(int n) { Poly<M> a(n); for (M &x : a.v) { x = M(rng()); } return a; }
static ulng fold(const vector<M> &v) { ulng h = 0; for (const M &x : v) { h = h * 1000003 + x.val(); } return h; }
static void record(const std::string &op, const std::string &reference, lng n, const std::function<ulng()> &work, ulng expected) {
    int iters = 1;
    for (;;) {
        Clock::time_point t = Clock::now();
        for (int i = 0; i < iters; ++i) { sink += work(); }
        double ms = std::chrono::duration<double, std::milli>(Clock::now() - t).count();
        if (ms >= targetMs) { break; }
        iters *= 2;}
    vector<double> samples;
    for (int r = 0; r < reps; ++r) {
        Clock::time_point t = Clock::now();
        ulng h = 0;
        for (int i = 0; i < iters; ++i) { h = work(); }
        samples.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t).count() / iters);
        if (h != expected) { std::fprintf(stderr, "VERIFY FAIL %s n=%lld\n", op.c_str(), (long long)n); std::exit(1); }}
    sort(samples.begin(), samples.end());
    std::printf("{\"op\": \"%s\", \"reference\": \"%s\", \"n\": %lld, \"median_ms\": %.6f, \"iterations\": %d}\n", op.c_str(), reference.c_str(), (long long)n, samples[samples.size() / 2], iters);}
template<typename T> static Poly<T> naive(const Poly<T> &a, const Poly<T> &b) {
    Poly<T> r(a.size() + b.size() - 1);
    for (int i = 0; i < a.size(); ++i) { for (int j = 0; j < b.size(); ++j) { r.v[i + j] += a.v[i] * b.v[j]; } }
    return r;}
template<typename T> static Poly<T> euclidGcd(Poly<T> a, Poly<T> b) {
    a.trim(); b.trim();
    while (!b.isNil()) { Poly<T> r = a % b; a = b; b = r; }
    return a.normalize();}

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 1;
    reps = argc > 2 ? std::atoi(argv[2]) : 5;
    targetMs = argc > 3 ? std::atof(argv[3]) : 3;
    rng.seed(seed);
    for (int n : {8, 16, 24, 32, 40, 48, 64, 96, 128, 256, 1024, 4096, 1 << 14, 1 << 16, 1 << 18, 1 << 20}) {
        Poly<M> a = rnd(n), b = rnd(n);
        ulng expected = n <= 4096 ? fold(naive(a, b).v) : fold((a * b).v);
        record("convolution_mint", "schoolbook", n, [&] { return fold((a * b).v); }, expected);
        if (n <= 4096) { record("schoolbook_mint", "same", n, [&] { return fold(schoolbook(a, b).v); }, expected); }
        if (n >= 16 && n <= 4096) { record("karatsuba_mint", "same", n, [&] { return fold(karatsuba(a, b).v); }, expected); }
        if (n >= 32) { record("ntt_mint", "same", n, [&] { return fold(poly_detail::Ntt<M>::convolve(a.v, b.v)); }, expected); }
        if (n <= (1 << 18)) {
            using D = ModInt<1000000007>;
            Poly<D> x(n), y(n); for (int i = 0; i < n; ++i) { x.v[i] = D(a.v[i].val()); y.v[i] = D(b.v[i].val()); }
            vector<uint> xu(static_cast<size_t>(n)), yu(static_cast<size_t>(n)); for (int i = 0; i < n; ++i) { xu[static_cast<size_t>(i)] = x.v[i].val(); yu[static_cast<size_t>(i)] = y.v[i].val(); }
            vector<uint> fm = PolyCompanion::convolutionFftMod(xu, yu, 1000000007);
            ulng e2 = 0; for (uint v : fm) { e2 = e2 * 1000003 + v; }
            record("arbitrary_mod_1e9+7", "three_prime_ntt", n, [&] { ulng h = 0; for (const D &v : convolutionArbitraryMod(x, y).v) { h = h * 1000003 + v.val(); } return h; }, e2);
            record("dispatch_1e9+7", "operator*", n, [&] { ulng h = 0; for (const D &v : (x * y).v) { h = h * 1000003 + v.val(); } return h; }, e2);
            record("fft_mod_1e9+7", "split_15bit_fft", n, [&] { ulng h = 0; for (uint v : PolyCompanion::convolutionFftMod(xu, yu, 1000000007)) { h = h * 1000003 + v; } return h; }, e2);
            Poly<double> da(n), db(n); for (int i = 0; i < n; ++i) { da.v[i] = double(a.v[i].val() % 1000); db.v[i] = double(b.v[i].val() % 1000); }
            Poly<double> dr = da * db; ulng e3 = 0; for (double v : dr.v) { e3 = e3 * 1000003 + ulng(std::llround(v)); }
            record("convolution_double", "fft", n, [&] { ulng h = 0; for (double v : (da * db).v) { h = h * 1000003 + ulng(std::llround(v)); } return h; }, e3);
            vector<lng> la(static_cast<size_t>(n)), lb(static_cast<size_t>(n)); for (int i = 0; i < n; ++i) { la[static_cast<size_t>(i)] = lng(a.v[i].val() % 1000000); lb[static_cast<size_t>(i)] = lng(b.v[i].val() % 1000000); }
            vector<lng> lr = PolyCompanion::convolutionLng(la, lb); ulng e4 = 0; for (lng v : lr) { e4 = e4 * 1000003 + ulng(v); }
            record("convolution_lng", "three_prime_ntt", n, [&] { ulng h = 0; for (lng v : PolyCompanion::convolutionLng(la, lb)) { h = h * 1000003 + ulng(v); } return h; }, e4);}}
    for (int n : {1 << 10, 1 << 12, 1 << 14, 1 << 16, 1 << 18}) {
        Poly<M> a = rnd(n); a.v[0] = 1; Poly<M> z = a; z.v[0] = 0;
        record("inv", "newton", n, [&] { return fold(inv(a, n).v); }, fold(inv(a, n).v));
        record("log", "inv+integ", n, [&] { return fold(log(a, n).v); }, fold(log(a, n).v));
        record("exp", "newton", n, [&] { return fold(exp(z, n).v); }, fold(exp(z, n).v));
        if (n <= (1 << 16)) { record("exp_semi_relaxed", "linearOde", n, [&] { return fold(linearOde(deriv(z), Poly<M>(), M(1), n).v); }, fold(exp(z, n).v)); }
        record("sqrt", "newton", n, [&] { return fold(sqrt(a, n).v); }, fold(sqrt(a, n).v));
        record("pow_1e9", "exp(log)", n, [&] { return fold(pow(a, 1000000000, n).v); }, fold(pow(a, 1000000000, n).v));
        if (n <= (1 << 16)) {
            vector<M> xs(static_cast<size_t>(n)); for (M &x : xs) { x = M(rng()); }
            record("evalMulti", "transposed_tree", n, [&] { return fold(evalMulti(a, xs)); }, fold(evalMulti(a, xs)));
            vector<M> ys(static_cast<size_t>(n)); for (M &y : ys) { y = M(rng()); }
            record("interpolate", "tree", n, [&] { return fold(Poly<M>::interpolate(xs, ys).v); }, fold(Poly<M>::interpolate(xs, ys).v));
            record("taylorShift", "binomial_convolution", n, [&] { return fold(taylorShift(a, M(7)).v); }, fold(taylorShift(a, M(7)).v));
            Poly<M> g = rnd(n);
            record("compose", "kinoshita_li_above_threshold", n, [&] { return fold(compose(a, g, n).v); }, fold(compose(a, g, n).v));
            if (n <= (1 << 14)) { record("compose_brent_kung", "baby_giant", n, [&] { return fold(composeBrentKung(a, g, n).v); }, fold(compose(a, g, n).v)); }
            Poly<M> h = g; h.v[0] = 0; h.v[1] = 1;
            record("compositionalInverse", "power_projection", n, [&] { return fold(compositionalInverse(h, n).v); }, fold(compositionalInverse(h, n).v));
            Poly<M> q = rnd(n + 1); q.v[0] = 1; Poly<M> p = rnd(n);
            record("coefOfRationalFps_1e18", "bostan_mori", n, [&] { return coefOfRationalFps(p, q, 1000000000000000000LL).val(); }, coefOfRationalFps(p, q, 1000000000000000000LL).val());
            Poly<M> b = rnd(n);
            record("gcd", "half_gcd", n, [&] { return fold(gcd(a, b).v); }, fold(euclidGcd(a, b).v));
            if (n <= (1 << 12)) { record("gcd_euclid", "schoolbook_divmod", n, [&] { return fold(euclidGcd(a, b).v); }, fold(euclidGcd(a, b).v)); }
            record("relaxedMul", "online", n, [&] { return fold(relaxedMul(a, b, n).v); }, fold(truncatedMul(a, b, n).v));
            record("semiRelaxedMul", "online", n, [&] { return fold(semiRelaxedMul(a, b, n).v); }, fold(truncatedMul(a, b, n).v));
            record("resultant", "half_gcd_chain", n, [&] { return resultant(a, b).val(); }, resultant(a, b).val());}}
    for (int n : {128, 256, 512, 1024, 2048, 4096, 8192, 16384}) {
        Poly<M> a = rnd(n), b = rnd(n);
        record("gcd_threshold", "fast_euclid_base_1024", n, [&] { return fold(poly_detail::fastEuclid(a, b, a.deg()).apply(a, b).first.normalize().v); }, fold(euclidGcd(a, b).v));
        record("gcd_threshold_euclid", "schoolbook", n, [&] { return fold(euclidGcd(a, b).v); }, fold(euclidGcd(a, b).v));
        if (n > 2048) { continue; }
        Poly<M> g = rnd(n);
        record("compose_threshold_dispatch", "compose", n, [&] { return fold(compose(a, g, n).v); }, fold(compose(a, g, n).v));
        record("compose_threshold_bk", "brent_kung", n, [&] { return fold(composeBrentKung(a, g, n).v); }, fold(compose(a, g, n).v));}
    { int n = 1 << 10; Poly<M> f = rnd(n); f.v[n - 1] = 1; record("factor", "cantor_zassenhaus", n, [&] { ulng h = 0; for (auto &[g, e] : factor(f)) { h = h * 1000003 + fold(g.v) + ulng(e); } return h; }, [&] { ulng h = 0; for (auto &[g, e] : factor(f)) { h = h * 1000003 + fold(g.v) + ulng(e); } return h; }()); }
    { using D = DynModInt<7>; D::setMod(786433, 1); int n = 1 << 19; Poly<D> a(n), b(n); for (D &x : a.v) { x = D(rng()); } for (D &x : b.v) { x = D(rng()); } auto h = [&](const Poly<D> &p) { ulng s = 0; for (const D &x : p.v) { s = s * 1000003 + x.val(); } return s; }; ulng e = h(convolutionLarge(a, b)); record("convolutionLarge_786433", "block_ntt", n, [&] { return h(convolutionLarge(a, b)); }, e); record("arbitraryMod_786433", "three_prime_ntt", n, [&] { return h(convolutionArbitraryMod(a, b)); }, e); }
    std::fprintf(stderr, "verified sink=%llu\n", (unsigned long long)sink);
    return 0;}
