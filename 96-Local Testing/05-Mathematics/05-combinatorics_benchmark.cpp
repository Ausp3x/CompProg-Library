#include "../../05-Mathematics/05-combinatorics.hpp"
#include "../../01-Core/07-infint.hpp"
#include <chrono>

using Clock = std::chrono::steady_clock;
volatile long double sink = 0;
// The previous LogCombinatorics table, one lgammal call per entry.
vector<long double> lgammaTable(int n) {
    vector<long double> lf(n + 1);
    for (int i = 0; i <= n; ++i) { lf[i] = std::lgamma(i + 1.0L); }
    return lf;}
int main() {
    for (int n : {100000, 1000000, 10000000}) {
        auto reference = lgammaTable(n);
        long double worst = 0;
        for (int rep = -1; rep < 5; ++rep) {
            auto start = Clock::now(); LogCombinatorics table(n); auto end = Clock::now();
            auto start_ref = Clock::now(); auto lf = lgammaTable(n); auto end_ref = Clock::now();
            for (int i = 0; i <= n; ++i) {
                long double ulp = std::nextafter(reference[i], std::numeric_limits<long double>::infinity()) - reference[i];
                worst = max(worst, std::abs(table.lf[i] - reference[i]) / ulp);}
            if (lf != reference) { throw std::runtime_error("lgammal table unstable"); }
            sink = sink + table.lf[n] + lf[n];
            if (rep >= 0) {
                cout << "log compensated_sum " << n << ' ' << rep << ' ' << std::chrono::duration<double, std::milli>(end - start).count() << ' ' << double(worst) << '\n';
                cout << "log lgammal_table " << n << ' ' << rep << ' ' << std::chrono::duration<double, std::milli>(end_ref - start_ref).count() << ' ' << 0 << '\n';}}}
    for (int n : {100000, 1000000}) {
        string expected;
        for (int rep = -1; rep < 5; ++rep) {
            auto start = Clock::now(); InfInt p = primorial<InfInt>(n); auto end = Clock::now();
            string digits = p.toString(16);
            if (rep == -1) { expected = digits; }
            if (digits != expected) { throw std::runtime_error("primorial unstable"); }
            if (rep >= 0) { cout << "primorial infint " << n << ' ' << rep << ' ' << std::chrono::duration<double, std::milli>(end - start).count() << ' ' << digits.size() << '\n'; }}}}
