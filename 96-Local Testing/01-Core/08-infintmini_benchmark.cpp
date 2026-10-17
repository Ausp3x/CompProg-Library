// Measures InfIntMini's two product backends and the dispatcher; prints JSON lines with medians in microseconds.
#include "../../01-Core/08-infintmini.hpp"

using Clock = std::chrono::steady_clock;

static vector<uint> randomLimbs(int n, std::mt19937_64 &rng) {
    auto v = vector<uint>(size_t(n));
    for (uint &x : v) { x = uint(rng() % InfIntMini::BASE); }
    v.back() = max(v.back(), 1u);
    return v;}

// The header's schoolbook loop: the shorter operand outside, the longer inside.
static vector<ulng> schoolbook(const vector<uint> &a, const vector<uint> &b) {
    vector<ulng> r(a.size() + b.size(), 0);
    const vector<uint> &x = a.size() <= b.size() ? a : b, &y = a.size() <= b.size() ? b : a;
    for (size_t i = 0; i < x.size(); i++) { for (size_t j = 0; j < y.size(); j++) { r[i + j] += ulng(x[i]) * y[j]; } }
    return r;}

static vector<ulng> transform(const vector<uint> &a, const vector<uint> &b) {
    size_t len = a.size() + b.size(), L = std::bit_ceil(len);
    vector<ulng> fa(a.begin(), a.end()), fb(b.begin(), b.end());
    fa.resize(L); fb.resize(L);
    InfIntMini::ntt(fa, false); InfIntMini::ntt(fb, false);
    for (size_t i = 0; i < L; i++) { fa[i] = InfIntMini::mulP(fa[i], fb[i]); }
    InfIntMini::ntt(fa, true);
    fa.resize(len);
    return fa;}

template<typename F> static double median(int reps, F f) {
    vector<double> t;
    for (int r = 0; r < reps; r++) {
        auto s = Clock::now(); f();
        t.push_back(std::chrono::duration<double, std::micro>(Clock::now() - s).count());}
    sort(t.begin(), t.end());
    return t[t.size() / 2];}

int main(int argc, char **argv) {
    bool quick = argc > 1 && string(argv[1]) == "quick";
    std::mt19937_64 rng(20261017);
    vector<pair<int, int>> shapes;
    for (int n : {16, 32, 48, 64, 96, 128, 192, 256, 512, 1024, 4096}) { shapes.push_back({n, n}); }
    for (int m : {8, 16, 32, 64, 128, 256, 384, 512, 768}) { shapes.push_back({quick ? 4096 : 65536, m}); }
    for (int m : {64, 128, 256, 512}) { shapes.push_back({quick ? 2048 : 16384, m}); }
    for (auto [n, m] : shapes) {
        vector<uint> a = randomLimbs(n, rng), b = randomLimbs(m, rng);
        int reps = n * m > 1000000 ? 5 : 21;
        vector<ulng> x = schoolbook(a, b), y = transform(a, b);
        y.resize(x.size(), 0);
        if (x != y) { cerr << "backend mismatch at " << n << " x " << m << '\n'; return 1; }
        InfIntMini A, B; A.n = a; B.n = b;
        double ts = median(reps, [&] { x = schoolbook(a, b); }), tn = median(reps, [&] { y = transform(a, b); });
        InfIntMini C;
        double td = median(reps, [&] { C = A * B; });
        size_t L = std::bit_ceil(size_t(n + m));
        double k = double(n) * m / (double(L) * std::bit_width(L));
        printf("{\"n\": %d, \"m\": %d, \"schoolbook_us\": %.2f, \"ntt_us\": %.2f, \"dispatch_us\": %.2f, \"cost_ratio\": %.3f}\n", n, m, ts, tn, td, k);}
    return 0;}
