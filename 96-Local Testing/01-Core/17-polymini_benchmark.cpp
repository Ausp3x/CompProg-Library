// PolyMini benchmark: medians after doubling warmup against Poly<mint>; every result is checked against the full type's result before timing.
#include "../../01-Core/06-modintmini.hpp"
#include "../../01-Core/17-polymini.hpp"
#include "../../01-Core/16-poly.hpp"
#include <functional>

using T = ModIntMini<998244353>;
using P = PolyMini<T>;
using Clock = std::chrono::steady_clock;
static std::mt19937_64 rng(1);
static int reps = 5;
static double targetMs = 3;
static ulng sink = 0;

template<typename V> static ulng fold(const V &v) { ulng h = 0; for (const auto &x : v) { h = h * 1000003 + ulng(x.val()); } return h; }
static ulng fold(const Poly<mint> &p) { return fold(p.v); }
static double measure(const std::function<ulng()> &work, ulng expected, const std::string &label) {
    if (work() != expected) { std::fprintf(stderr, "VERIFY FAIL %s\n", label.c_str()); std::exit(1); }
    int iters = 1;
    for (;;) {
        Clock::time_point t = Clock::now();
        for (int i = 0; i < iters; ++i) { sink += work(); }
        if (std::chrono::duration<double, std::milli>(Clock::now() - t).count() >= targetMs) { break; }
        iters *= 2;}
    vector<double> samples;
    for (int r = 0; r < reps; ++r) {
        Clock::time_point t = Clock::now();
        for (int i = 0; i < iters; ++i) { sink += work(); }
        samples.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t).count() / iters);}
    sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];}
static void emit(const std::string &op, lng n, double mini, double full) {
    std::printf("{\"op\": \"%s\", \"n\": %lld, \"mini_ms\": %.6f, \"full_ms\": %.6f}\n", op.c_str(), (long long)n, mini, full);}
static P rnd(int n) { P a(n); for (T &x : a) { x = T(rng()); } return a; }
static Poly<mint> toFull(const P &a) { Poly<mint> r(int(a.size())); for (size_t i = 0; i < a.size(); ++i) { r.v[i] = mint(a[i].val()); } return r; }
static P ntt(P a, P b) {
    int n = int(a.size()) + int(b.size()) - 1, len = int(std::bit_ceil(uint(n)));
    a.resize(len); b.resize(len); P::ntt(a); P::ntt(b);
    for (int i = 0; i < len; ++i) { a[i] *= b[i]; }
    P::ntt(a, true); a.resize(n);
    return a;}
static P school(const P &a, const P &b) {
    P r(a.size() + b.size() - 1);
    for (size_t i = 0; i < a.size(); ++i) { for (size_t j = 0; j < b.size(); ++j) { r[i + j] += a[i] * b[j]; } }
    return r;}
// The evalTree descent with a configurable Horner leaf size, for the cutoff measurement.
static vector<T> evalCut(const P &a, const vector<T> &xs, int cut) {
    vector<P> t = P::tree(xs); vector<T> res(xs.size());
    auto go = [&](auto &&f, P p, int k, int l, int r) -> void {
        p = divMod(p, t[k]).second;
        if (r - l <= cut) { for (int i = l; i < r; ++i) { res[i] = eval(p, xs[i]); } return; }
        int m = std::midpoint(l, r); f(f, p, 2 * k, l, m); f(f, p, 2 * k + 1, m, r);};
    go(go, a, 1, 0, int(xs.size()));
    return res;}

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 1;
    reps = argc > 2 ? std::atoi(argv[2]) : 5;
    targetMs = argc > 3 ? std::atof(argv[3]) : 3;
    rng.seed(seed);
    for (int n : {8, 16, 24, 32, 40, 48, 64, 96}) {
        P a = rnd(n), b = rnd(n); ulng h = fold(toFull(a) * toFull(b));
        emit("threshold schoolbook vs ntt (mini_ms schoolbook, full_ms ntt)", n,
             measure([&] { return fold(school(a, b)); }, h, "school"), measure([&] { return fold(ntt(a, b)); }, h, "ntt"));}
    for (int cut : {1, 8, 16, 32, 64, 128}) {
        int n = 1 << 14; P a = rnd(n); vector<T> xs(n);
        for (T &x : xs) { x = T(rng()); }
        vector<mint> fx(n); for (int i = 0; i < n; ++i) { fx[i] = mint(xs[i].val()); }
        ulng h = fold(evalMulti(toFull(a), fx));
        emit("evalMulti leaf cutoff (mini_ms at cut = n, full_ms Poly)", cut, measure([&] { return fold(evalCut(a, xs, cut)); }, h, "cut"), measure([&] { return fold(evalMulti(toFull(a), fx)); }, h, "full eval"));}
    for (int n : {1 << 10, 1 << 14, 1 << 17, 500000}) {
        P a = rnd(n), b = rnd(n / 2 + 1); Poly<mint> fa = toFull(a), fb = toFull(b);
        emit("convolution n x n", n, measure([&] { return fold(a * a); }, fold(fa * fa), "mul"), measure([&] { return fold(fa * fa); }, fold(fa * fa), "fmul"));
        ulng hd = fold(divMod(fa, fb).first) ^ fold(divMod(fa, fb).second);
        emit("divMod n by n/2", n, measure([&] { auto [q, r] = divMod(a, b); return fold(q) ^ fold(r); }, hd, "div"), measure([&] { auto [q, r] = divMod(fa, fb); return fold(q.v) ^ fold(r.v); }, hd, "fdiv"));
        a[0] = 1; fa.v[0] = 1;
        emit("inv", n, measure([&] { return fold(inv(a, n)); }, fold(inv(fa, n)), "inv"), measure([&] { return fold(inv(fa, n)); }, fold(inv(fa, n)), "finv"));
        emit("log", n, measure([&] { return fold(log(a, n)); }, fold(log(fa, n)), "log"), measure([&] { return fold(log(fa, n)); }, fold(log(fa, n)), "flog"));
        a[0] = 0; fa.v[0] = 0;
        emit("exp", n, measure([&] { return fold(exp(a, n)); }, fold(exp(fa, n)), "exp"), measure([&] { return fold(exp(fa, n)); }, fold(exp(fa, n)), "fexp"));
        a[0] = 1; fa.v[0] = 1;
        emit("pow k = 10^18", n, measure([&] { return fold(pow(a, lng(1e18), n)); }, fold(pow(fa, lng(1e18), n)), "pow"), measure([&] { return fold(pow(fa, lng(1e18), n)); }, fold(pow(fa, lng(1e18), n)), "fpow"));
        emit("taylorShift", n, measure([&] { return fold(taylorShift(a, T(3))); }, fold(taylorShift(fa, mint(3))), "shift"), measure([&] { return fold(taylorShift(fa, mint(3))); }, fold(taylorShift(fa, mint(3))), "fshift"));
        if (n > 1 << 17) { continue; }
        vector<T> xs(a.begin(), a.end()); vector<mint> fx = fa.v;
        for (int i = 0; i < n; ++i) { xs[i] = T(i + 7); fx[i] = mint(i + 7); }
        ulng he = fold(evalMulti(fa, fx));
        emit("evalMulti n points", n, measure([&] { return fold(evalMulti(a, xs)); }, he, "evalMulti"), measure([&] { return fold(evalMulti(fa, fx)); }, he, "fevalMulti"));
        vector<mint> fy = evalMulti(fa, fx); vector<T> ys(n);
        for (int i = 0; i < n; ++i) { ys[i] = T(fy[i].val()); }
        emit("interpolate n points", n, measure([&] { return fold(P::interpolate(xs, ys)); }, fold(fa), "interpolate"), measure([&] { return fold(Poly<mint>::interpolate(fx, fy)); }, fold(fa), "finterpolate"));
        if (n > 1 << 14) { continue; }
        vector<T> s(n); vector<mint> fs(n);
        for (int i = 0; i < n; ++i) { s[i] = T(rng()); fs[i] = mint(s[i].val()); }
        vector<mint> fc = Poly<mint>::findLinearRecurrence(fs); vector<T> c(fc.size());
        for (size_t i = 0; i < fc.size(); ++i) { c[i] = T(fc[i].val()); }
        emit("findLinearRecurrence", n, measure([&] { return fold(P::findLinearRecurrence(s)); }, fold(fc), "bm"), measure([&] { return fold(Poly<mint>::findLinearRecurrence(fs)); }, fold(fc), "fbm"));
        ulng hk = Poly<mint>::linearRecurrenceKth(fs, fc, lng(1e18)).val();
        emit("linearRecurrenceKth d = n/2, k = 10^18", n, measure([&] { return ulng(P::linearRecurrenceKth(s, c, lng(1e18)).val()); }, hk, "kth"),
             measure([&] { return ulng(Poly<mint>::linearRecurrenceKth(fs, fc, lng(1e18)).val()); }, hk, "fkth"));}
    std::fprintf(stderr, "verified every measured result against Poly<mint>; sink=%llu\n", (unsigned long long)sink);}
