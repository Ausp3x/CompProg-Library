#include "../../03-Geometry/10-halfplaneintersection.hpp"

// Medians of five timed calls after one warmup; outputs are compared outside the timed region.
template<typename F> lng median(F f) {
    f(); vector<lng> t;
    for (int r = 0; r < 5; ++r) {
        auto s = std::chrono::steady_clock::now(); f();
        t.push_back(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - s).count());}
    sort(t.begin(), t.end());
    return t[2];}
vector<point> star(std::mt19937_64 &rng, int n, long double inner) {
    vector<long double> ang(n); vector<point> p;
    for (auto &a : ang) { a = (long double)(rng() % 1000000007) / 1000000007.L * 2 * std::numbers::pi_v<long double>; }
    sort(ang.begin(), ang.end());
    for (int i = 0; i < n; ++i) {
        long double r = (i % 2 ? inner : 0.95L) * 1e9L;
        point q{lng(std::llround(r * std::cos(ang[i]))), lng(std::llround(r * std::sin(ang[i])))};
        if (p.empty() || (q != p.back() && cross(p.back(), q) > 0)) { p.push_back(q); }}
    while (p.size() > 2 && cross(p.back(), p[0]) <= 0) { p.pop_back(); }
    return p;}
vector<point> spiral(int n, long double turns) {
    vector<point> out, in;
    for (int i = 0; i < n; ++i) {
        long double t = turns * 2 * std::numbers::pi_v<long double> * i / (n - 1), r = 10 + 3 * t;
        out.push_back({lng(std::llround(1e4L * r * std::cos(t))), lng(std::llround(1e4L * r * std::sin(t)))});
        in.push_back({lng(std::llround(1e4L * (r - 2) * std::cos(t))), lng(std::llround(1e4L * (r - 2) * std::sin(t)))});}
    reverse(in.begin(), in.end()); out.insert(out.end(), in.begin(), in.end());
    return out;}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927, checksum = 0;
    std::mt19937_64 rng(seed);
    auto row = [&](const string &c, int n, const string &impl, lng us) {
        std::cout << "{\"case\": \"" << c << "\", \"n\": " << n << ", \"implementation\": \"" << impl << "\", \"median_us\": " << us << "}\n";};
    for (int n : {1000, 10000, 100000, 1000000}) {
        vector<Halfplane> tangent, random;
        for (int i = 0; i < n; ++i) {
            long double a = (long double)(rng() % 1000000007) / 1000000007.L * 2 * std::numbers::pi_v<long double>;
            point p{lng(std::llround(9e8L * std::cos(a))), lng(std::llround(9e8L * std::sin(a)))};
            tangent.push_back({p, p + point{lng(std::llround(-1e5L * std::sin(a))), lng(std::llround(1e5L * std::cos(a)))}});
            point q{lng(rng() % 2000001) - 1000000, lng(rng() % 2000001) - 1000000}, r{lng(rng() % 2000001) - 1000000, lng(rng() % 2000001) - 1000000};
            if (q != r) { random.push_back({q, r}); }}
        HalfplaneResult x, y;
        row("tangent lines", n, "halfplaneIntersection", median([&] { x = halfplaneIntersection(tangent); }));
        row("random lines", n, "halfplaneIntersection", median([&] { y = halfplaneIntersection(random); }));
        if (x.kind != HalfplaneKind::Polygon) { std::cerr << "tangent family must be a polygon\n"; return 1; }
        checksum += x.vertices.size() + y.vertices.size() + ulng(y.kind);
        for (auto [name, p] : {pair<string, vector<point>>{"star kernel", star(rng, n, 0.6L)}, {"thin star kernel", star(rng, n, 0.05L)}, {"spiral", spiral(n / 2, 10)}}) {
            HalfplaneResult a, b;
            row(name, int(p.size()), "polygonKernel", median([&] { a = polygonKernel(p); }));
            row(name, int(p.size()), "polygonKernelLinear", median([&] { b = polygonKernelLinear(p); }));
            if (a.kind != b.kind || a.vertices != b.vertices) { std::cerr << "kernel mismatch on " << name << '\n'; return 1; }
            checksum += a.vertices.size() + ulng(a.kind);}}
    std::cerr << checksum << '\n';}
