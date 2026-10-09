#include "../../05-Mathematics/13-convolution.hpp"
#include "../../05-Mathematics/24-transform_algorithms.hpp"

template<typename F>
double timeMs(F f) {
    auto t = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count();}
template<typename M> vector<M> rnd(std::mt19937_64 &rng, int n) {
    vector<M> v(n);
    for (auto &x : v) { x = M(rng()); }
    return v;}
// The transform path of convolutionNtt without its naive cutoff.
vector<mint> nttOnly(const vector<mint> &a, const vector<mint> &b) {
    int L = int(std::bit_ceil(uint(a.size() + b.size() - 1)));
    vector<mint> x(L), y(L);
    std::copy(a.begin(), a.end(), x.begin()); std::copy(b.begin(), b.end(), y.begin());
    ntt(x); ntt(y);
    for (int i = 0; i < L; ++i) { x[i] *= y[i]; }
    intt(x);
    x.resize(a.size() + b.size() - 1);
    return x;}

int main(int argc, char **argv) {
    int reps = argc > 1 ? std::stoi(argv[1]) : 5;
    std::mt19937_64 rng(20261009);
    vector<string> rows;
    auto run = [&](const string &name, auto fa, auto fb, const string &la, const string &lb) {
        vector<double> ta, tb;
        for (int r = 0; r <= reps; ++r) {
            decltype(fa()) x;
            decltype(fb()) y;
            double t1 = timeMs([&] { x = fa(); }), t2 = timeMs([&] { y = fb(); });
            if (!(x == y)) { cerr << "MISMATCH " << name << endl; exit(1); }
            if (r) { ta.pb(t1); tb.pb(t2); }}
        sort(ta.begin(), ta.end()); sort(tb.begin(), tb.end());
        rows.pb("{\"workload\": \"" + name + "\", \"" + la + "_ms\": " + std::to_string(ta[reps / 2]) + ", \"" + lb + "_ms\": " + std::to_string(tb[reps / 2]) + "}");};
    auto big = rnd<mint>(rng, 1 << 16);
    for (int s : {16, 32, 48, 60, 64, 96, 128}) {
        auto b = rnd<mint>(rng, s);
        run("mint 2^16 x " + std::to_string(s), [&] { return convolutionNaive(big, b); }, [&] { return nttOnly(big, b); }, "naive", "ntt");}
    for (int s : {16, 32, 64, 128, 256, 1024}) {
        vector<ulng> a(s), b(s);
        for (auto &x : a) { x = rng(); }
        for (auto &x : b) { x = rng(); }
        int k = max(1, 4096 / s);
        run("ulng " + std::to_string(s) + " x " + std::to_string(s) + " (x" + std::to_string(k) + ")", [&] { vector<ulng> c; for (int i = 0; i < k; ++i) { c = convolutionNaive(a, b); } return c; },
            [&] { vector<ulng> c; for (int i = 0; i < k; ++i) { c = convolutionKaratsuba(a, b); } return c; }, "naive", "karatsuba");}
    {
        auto a = rnd<mint>(rng, 1 << 19), b = rnd<mint>(rng, 1 << 19);
        run("mint 2^19 x 2^19", [&] { return convolutionNtt(a, b); }, [&] { return convolution(a, b); }, "convolutionNtt", "convolution");
        auto c = rnd<ModInt<1000000007>>(rng, 1 << 19), d = rnd<ModInt<1000000007>>(rng, 1 << 19);
        run("1e9+7 2^19 x 2^19", [&] { return convolutionArbitraryMod(c, d); }, [&] { return convolution(c, d); }, "convolutionArbitraryMod", "convolution");
        vector<lng> e(1 << 19), f(1 << 19);
        vector<double> g(1 << 19), h(1 << 19);
        for (int i = 0; i < (1 << 19); ++i) { e[i] = lng(rng() % 2001) - 1000; f[i] = lng(rng() % 2001) - 1000; g[i] = double(e[i]); h[i] = double(f[i]); }
        run("int 2^19 x 2^19, |v| <= 1000", [&] { return convolutionLong(e, f); },
            [&] { auto r = convolutionFft(g, h); vector<lng> o(r.size()); for (int i = 0; i < int(r.size()); ++i) { o[i] = std::llround(r[i]); } return o; }, "convolutionLong", "convolutionFft");
        vector<ulng> u(100000), v(100000);
        for (auto &x : u) { x = rng(); }
        for (auto &x : v) { x = rng(); }
        run("ulng 1e5 x 1e5", [&] { return convolutionKaratsuba(u, v); }, [&] { return convolutionKaratsuba(u, v); }, "karatsuba", "karatsuba_again");}
    {
        auto a = rnd<mint>(rng, 1 << 24), b = rnd<mint>(rng, 1 << 24);
        run("mint 2^24 x 2^24 (blocks of 2^22)", [&] { return convolutionLarge(a, b); }, [&] { return convolution(a, b); }, "convolutionLarge", "convolution");}
    {
        auto a = rnd<mint>(rng, 1 << 20), b = rnd<mint>(rng, 1 << 20);
        run("subset convolution n = 20", [&] { return subsetConvolution(a, b); }, [&] { return disjointUnionConvolution(vector<vector<mint>>{a, b}); }, "subsetConvolution", "disjointUnionConvolution");
        run("xor/or convolution n = 20", [&] { return xorConvolution(a, b); }, [&] { return xorConvolution(a, b); }, "xorConvolution", "xorConvolution_again");
        vector<mint> x(1000001), y(1000001);
        for (int i = 1; i <= 1000000; ++i) { x[i] = mint(rng()); y[i] = mint(rng()); }
        run("gcd/lcm convolution n = 10^6", [&] { return gcdConvolution(x, y); }, [&] { return gcdConvolution(x, y); }, "gcdConvolution", "gcdConvolution_again");}
    cout << "[\n";
    for (int i = 0; i < int(rows.size()); ++i) { cout << "  " << rows[i] << (i + 1 < int(rows.size()) ? "," : "") << "\n"; }
    cout << "]\n";}
