// Rational throughput against an unchecked textbook fraction (128-bit cross products, std::gcd); prints JSON lines with medians in ns per operation.
#include "../../01-Core/07-infint.hpp"
#include "../../01-Core/09-rational.hpp"

using Clock = std::chrono::steady_clock;

// The usual contest fraction: full cross products in 128 bits and one gcd, no overflow checks.
struct Naive {
    lng p, q;
    static Naive make(lll p, lll q) {
        if (q < 0) { p = -p; q = -q; }
        lll g = std::gcd(p < 0 ? -p : p, q);
        return {lng(p / g), lng(q / g)};}
    friend Naive operator+(Naive a, Naive b) { return make(lll(a.p) * b.q + lll(b.p) * a.q, lll(a.q) * b.q); }
    friend Naive operator*(Naive a, Naive b) { return make(lll(a.p) * b.p, lll(a.q) * b.q); }
    friend bool operator<(Naive a, Naive b) { return lll(a.p) * b.q < lll(b.p) * a.q; }
    friend bool operator==(Naive a, Naive b) { return a.p == b.p && a.q == b.q; }
};

template<typename F> static double median(int reps, int ops, F f) {
    vector<double> t;
    for (int r = 0; r < reps; r++) {
        auto s = Clock::now(); f();
        t.push_back(std::chrono::duration<double, std::nano>(Clock::now() - s).count() / ops);}
    sort(t.begin(), t.end());
    return t[t.size() / 2];}

static void emit(const string &type, const string &op, int bits, double ours, double naive) {
    printf("{\"T\": \"%s\", \"op\": \"%s\", \"bits\": %d, \"rational_ns\": %.2f, \"naive_ns\": %.2f}\n", type.c_str(), op.c_str(), bits, ours, naive);}

int main(int argc, char **argv) {
    int n = argc > 1 && string(argv[1]) == "quick" ? 20000 : 200000, reps = 9;
    std::mt19937_64 rng(20261017);
    for (int bits : {8, 20, 30}) {
        vector<Frac> a(static_cast<size_t>(n)), b(static_cast<size_t>(n)), c(static_cast<size_t>(n));
        vector<Naive> x(static_cast<size_t>(n)), y(static_cast<size_t>(n)), z(static_cast<size_t>(n));
        for (int i = 0; i < n; i++) {
            lng p = lng(rng() >> (64 - bits)) - (lng(1) << (bits - 1)), q = lng(rng() >> (64 - bits)) | 1, r = lng(rng() >> (64 - bits)), s = lng(rng() >> (64 - bits)) | 1;
            a[size_t(i)] = Frac(p, q); b[size_t(i)] = Frac(r, s); x[size_t(i)] = Naive::make(p, q); y[size_t(i)] = Naive::make(r, s);}
        auto check = [&](const string &op) {
            for (int i = 0; i < n; i++) {
                if (c[size_t(i)].num != z[size_t(i)].p || c[size_t(i)].den != z[size_t(i)].q) { cerr << op << " mismatch at " << i << '\n'; std::exit(1); }}};
        double ta = median(reps, n, [&] { for (int i = 0; i < n; i++) { c[size_t(i)] = a[size_t(i)] + b[size_t(i)]; } });
        double na = median(reps, n, [&] { for (int i = 0; i < n; i++) { z[size_t(i)] = x[size_t(i)] + y[size_t(i)]; } });
        check("add"); emit("lng", "add", bits, ta, na);
        double tm = median(reps, n, [&] { for (int i = 0; i < n; i++) { c[size_t(i)] = a[size_t(i)] * b[size_t(i)]; } });
        double nm = median(reps, n, [&] { for (int i = 0; i < n; i++) { z[size_t(i)] = x[size_t(i)] * y[size_t(i)]; } });
        check("mul"); emit("lng", "mul", bits, tm, nm);
        vector<Frac> sa = a; vector<Naive> sx = x;
        double ts = median(3, n, [&] { sa = a; sort(sa.begin(), sa.end()); }), ns = median(3, n, [&] { sx = x; sort(sx.begin(), sx.end()); });
        for (int i = 0; i < n; i++) { if (sa[size_t(i)].num != sx[size_t(i)].p || sa[size_t(i)].den != sx[size_t(i)].q) { cerr << "sort mismatch\n"; return 1; } }
        emit("lng", "sort", bits, ts, ns);
        double sum = 0, nsum = 0;
        double td = median(reps, n, [&] { sum = 0; for (int i = 0; i < n; i++) { sum += a[size_t(i)].toDouble(); } });
        double nd = median(reps, n, [&] { nsum = 0; for (int i = 0; i < n; i++) { nsum += double(x[size_t(i)].p) / double(x[size_t(i)].q); } });
        if (std::fabs(sum - nsum) > 1e-6 * (1 + std::fabs(sum))) { cerr << "toDouble mismatch\n"; return 1; }
        emit("lng", "toDouble", bits, td, nd);}
    for (int bits : {40, 60, 120}) {
        int m = n / 4;
        vector<Rational<lll>> a(static_cast<size_t>(m)), b(static_cast<size_t>(m)), c(static_cast<size_t>(m));
        auto rnd = [&]() { lll v = lll(rng() >> (bits >= 64 ? 0 : 64 - bits)); if (bits > 64) { v = (v << (bits - 64)) | lll(rng() >> (128 - bits)); } return v; };
        for (int i = 0; i < m; i++) { a[size_t(i)] = Rational<lll>(rnd() - (lll(1) << (bits - 1)), rnd() | 1); b[size_t(i)] = Rational<lll>(rnd(), rnd() | 1); }
        int ok = 0;
        double ta = median(reps, m, [&] { ok = 0; for (int i = 0; i < m; i++) { ok += tryAdd(a[size_t(i)], b[size_t(i)], c[size_t(i)]); } });
        emit("lll", "tryAdd", bits, ta, 0);
        double tc = median(reps, m, [&] { ok = 0; for (int i = 0; i < m; i++) { ok += a[size_t(i)] < b[size_t(i)]; } });
        emit("lll", "compare", bits, tc, 0);
        double sum = 0;
        double td = median(reps, m, [&] { sum = 0; for (int i = 0; i < m; i++) { sum += a[size_t(i)].toDouble(); } });
        if (std::isnan(sum)) { return 1; }
        emit("lll", "toDouble", bits, td, 0);}
    for (int bits : {256, 2048}) {
        int m = bits > 1000 ? 2000 : 20000;
        vector<Rational<InfInt>> a(static_cast<size_t>(m)), b(static_cast<size_t>(m)), c(static_cast<size_t>(m));
        for (int i = 0; i < m; i++) {
            a[size_t(i)] = Rational<InfInt>(InfInt::sampleBits(bits, rng), InfInt::sampleBits(bits, rng));
            b[size_t(i)] = Rational<InfInt>(InfInt::sampleBits(bits, rng), InfInt::sampleBits(bits, rng));}
        double ta = median(5, m, [&] { for (int i = 0; i < m; i++) { c[size_t(i)] = a[size_t(i)] + b[size_t(i)]; } });
        for (int i = 0; i < m; i++) { if (c[size_t(i)] - b[size_t(i)] != a[size_t(i)]) { cerr << "InfInt add mismatch\n"; return 1; } }
        emit("InfInt", "add", bits, ta, 0);}
    return 0;}
