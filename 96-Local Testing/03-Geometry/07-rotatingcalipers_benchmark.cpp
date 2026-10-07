#include "../../03-Geometry/07-rotatingcalipers.hpp"

// Reproducible comparison of the O(n) polar sweep against all O(n^2) edge
// pairs, using the same exact scalar objective comparator. Hull setup excluded.
long double brute(const ConvexCalipers &h) {
    ulll num = 0, den = 1; bool found = false;
    for (int i = 0; i < h.n; ++i) { for (int j = i + 1; j < h.n; ++j) {
        lll d = cross(h.e[i], h.e[j]); if (d < 0) { d = -d; } if (!d) { continue; }
        ulll u = ulll(h.width[i]) * h.width[j];
        if (!found || ConvexCalipers::ratioLess(u, d, num, den)) { num = u; den = d; found = true; }}}
    return static_cast<long double>(num) / static_cast<long double>(den);}

int main() {
    using Clock = std::chrono::steady_clock;
    long double checksum = 0;
    std::cout << "n setup_us polar_us brute_us brute_over_polar\n";
    for (int n : {32, 128, 512, 2048, 8192}) {
        vector<point> p;
        for (int x = 0; x < n; ++x) { p.push_back({x - n / 2, lng(x - n / 2) * (x - n / 2)}); }
        auto start = Clock::now(); ConvexCalipers h(p);
        double setup = std::chrono::duration<double, std::micro>(Clock::now() - start).count();
        long double reference = brute(h), actual = h.minimumAreaParallelogramApprox().area;
        if (actual != reference) { std::cerr << "FAIL n=" << n << " reference=" << reference << " actual=" << actual << '\n'; return 1; }
        array<double, 7> fast{}, slow{}; int repeats = max(1, 8192 / n);
        for (int k = 0; k < 7; ++k) {
            start = Clock::now();
            for (int r = 0; r < repeats; ++r) { checksum += h.minimumAreaParallelogramApprox().area; }
            fast[k] = std::chrono::duration<double, std::micro>(Clock::now() - start).count() / repeats;
            start = Clock::now(); checksum += brute(h);
            slow[k] = std::chrono::duration<double, std::micro>(Clock::now() - start).count();}
        sort(fast.begin(), fast.end()); sort(slow.begin(), slow.end());
        std::cout << n << ' ' << setup << ' ' << fast[3] << ' ' << slow[3] << ' ' << slow[3] / fast[3] << '\n';}
    std::cout << "checksum=" << checksum << '\n';}
