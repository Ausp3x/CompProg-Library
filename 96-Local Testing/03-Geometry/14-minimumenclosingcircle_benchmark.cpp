#include "../../03-Geometry/14-minimumenclosingcircle.hpp"
#include <chrono>

// minimumEnclosingCircle (move-to-front within a 32 n + 64 test budget) against the plain three scans over one shuffle,
// and minimumEnclosingCircleApprox on the same points as double. One JSON row per case; the checksum is printed to stderr.
EnclosingCircle plain(const vector<point> &p, ulng seed) {
    int n = int(p.size()), m = 0; vector<int> o(n); vector<point> q(n); array<point, 3> c; array<int, 3> s;
    iota(o.begin(), o.end(), 0);
    for (int i = n - 1; i > 0; --i) { swap(o[i], o[CustomHash::splitMix64(seed++) % ulng(i + 1)]); }
    for (int i = 0; i < n; ++i) { q[i] = p[o[i]]; }
    auto fit = [&](const array<int, 3> &t, int k) { m = k; for (int i = 0; i < k; ++i) { c[i] = q[t[i]]; } };
    int k = enclosing_detail::welzl(n, s, false, LLONG_MAX, fit, [&](int i) { return enclosing_detail::locate(c.data(), m, q[i]) > 0; });
    EnclosingCircle res;
    for (int t = 0; t < k; ++t) { res.support.push_back(o[s[t]]); res.points.push_back(q[s[t]]); }
    return res;}
template<typename F> double median(F f) {
    f(0); vector<double> t;
    for (int r = 1; r <= 5; ++r) {
        auto a = std::chrono::steady_clock::now(); f(r);
        t.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - a).count());}
    sort(t.begin(), t.end());
    return t[2];}
int main(int argc, char **argv) {
    std::mt19937_64 rng(argc > 1 ? std::stoull(argv[1]) : 20260927); ulng checksum = 0;
    const char *names[] = {"uniform square", "uniform disk", "circle", "gaussian"};
    for (int d = 0; d < 4; ++d) {
        for (int n : {1000, 10000, 100000, 1000000}) {
            vector<point> p(n); std::normal_distribution<long double> g(0, 2e8L);
            for (auto &x : p) {
                if (d == 0) { x = {lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000}; continue; }
                if (d == 3) { x = {lng(std::clamp(g(rng), -1e9L, 1e9L)), lng(std::clamp(g(rng), -1e9L, 1e9L))}; continue; }
                long double a = (long double)(rng() % 1000000007) * 2 * std::numbers::pi_v<long double> / 1000000007.L;
                long double r = d == 1 ? 9e8L * std::sqrt((long double)(rng() % 1000001) / 1e6L) : 9e8L;
                x = {std::llround(r * std::cos(a)), std::llround(r * std::sin(a))};}
            vector<Point2<double>> f(n);
            for (int i = 0; i < n; ++i) { f[size_t(i)] = {double(p[size_t(i)].x), double(p[size_t(i)].y)}; }
            EnclosingCircle a, b; CircleApprox e;
            double tm = median([&](int r) { a = minimumEnclosingCircle(p, ulng(100 + r)); });
            double tp = median([&](int r) { b = plain(p, ulng(100 + r)); });
            double tf = median([&](int r) { e = minimumEnclosingCircleApprox(f, ulng(100 + r)); });
            if (a.center() != b.center() || a.radius2() != b.radius2() || std::abs(e.r - a.circle.r) > 1e-9L * 1e9L) { std::cerr << "mismatch\n"; return 1; }
            checksum += ulng(a.support.size()) + ulng(e.r);
            for (auto [name, t] : {pair{"minimumEnclosingCircle", tm}, pair{"plain three scans", tp}, pair{"minimumEnclosingCircleApprox", tf}}) {
                std::cout << "{\"case\": \"" << names[d] << "\", \"size\": " << n << ", \"implementation\": \"" << name << "\", \"median_us\": " << std::llround(t) << "}\n";}}}
    std::cerr << checksum << '\n';}
