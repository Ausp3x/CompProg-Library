#include "../../06-Miscellaneous/34-slope_trick.hpp"

// Prints "workload impl n rep ms checksum"; the Python driver checks that all implementations of a workload agree.
constexpr ulng SEED = 20261013;

// Benchmark-only reference: heaps of (position, count) pairs, the weighted-heap design the header does not adopt.
struct PairSlopeTrick {
    lng mn = 0;
    std::priority_queue<pair<lng, lng>> left;
    std::priority_queue<pair<lng, lng>, vector<pair<lng, lng>>, std::greater<pair<lng, lng>>> right;
    void addXMinusA(lng a, lng w) {
        lng rem = w;
        while (rem > 0 && !left.empty() && left.top().fi > a) {
            auto [x, c] = left.top(); left.pop();
            lng take = std::min(c, rem);
            mn += (x - a) * take; right.push({x, take}); rem -= take;
            if (c > take) { left.push({x, c - take}); }}
        if (rem) { right.push({a, rem}); }
        if (w - rem) { left.push({a, w - rem}); }}
    void addAMinusX(lng a, lng w) {
        lng rem = w;
        while (rem > 0 && !right.empty() && right.top().fi < a) {
            auto [x, c] = right.top(); right.pop();
            lng take = std::min(c, rem);
            mn += (a - x) * take; left.push({x, take}); rem -= take;
            if (c > take) { right.push({x, c - take}); }}
        if (rem) { left.push({a, rem}); }
        if (w - rem) { right.push({a, w - rem}); }}
    void addAbs(lng a, lng w) { addXMinusA(a, w); addAMinusX(a, w); }
};

struct Clock {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    double ms() const { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(); }
};

int main() {
    std::mt19937_64 gen(SEED);
    auto rnd = [&](lng l, lng r) { return l + lng(gen() % ulng(r - l + 1)); };
    auto emit = [&](const string &w, const string &impl, int n, int rep, double ms, lll sum) {
        cout << w << ' ' << impl << ' ' << n << ' ' << rep << ' ' << std::fixed << std::setprecision(3) << ms << ' ' << lng(sum) << '\n';};
    for (int rep = -1; rep < 5; ++rep) {
        for (int n : {200000, 1000000}) {
            vector<lng> a(n);
            for (auto &x : a) { x = rnd(0, 1000000000); }
            for (int impl = 0; impl < 2; ++impl) {
                int which = (impl + rep + 2) % 2;
                Clock c; lll sum = 0;
                if (which == 0) { SlopeTrick<lng> s; for (lng x : a) { s.prefixMin(); s.addAbs(x); } sum = s.min(); }
                else { WeightedSlopeTrick<lng> s; for (lng x : a) { s.prefixMin(); s.addAbs(x); } sum = s.min(); }
                if (rep >= 0) { emit("monotone-fit", which ? "weighted" : "unit", n, rep, c.ms(), sum); }}
            for (int impl = 0; impl < 2; ++impl) {
                int which = (impl + rep + 2) % 2;
                Clock c; lll sum = 0;
                if (which == 0) { SlopeTrick<lng> s; for (lng x : a) { s.slide(-1000, 1000); s.addAbs(x); } sum = s.min(); }
                else { WeightedSlopeTrick<lng> s; for (lng x : a) { s.slide(-1000, 1000); s.addAbs(x); } sum = s.min(); }
                if (rep >= 0) { emit("bounded-step", which ? "weighted" : "unit", n, rep, c.ms(), sum); }}}
        {
            int n = 200000;
            vector<lng> a(n), w(n);
            for (int i = 0; i < n; ++i) { a[i] = rnd(0, 1000000000); w[i] = rnd(1, 8); }
            for (int impl = 0; impl < 2; ++impl) {
                int which = (impl + rep + 2) % 2;
                Clock c; lll sum = 0;
                if (which == 0) { SlopeTrick<lng> s; for (int i = 0; i < n; ++i) { s.prefixMin(); for (lng k = 0; k < w[i]; ++k) { s.addAbs(a[i]); } } sum = s.min(); }
                else { WeightedSlopeTrick<lng> s; for (int i = 0; i < n; ++i) { s.prefixMin(); s.addAbs(a[i], w[i]); } sum = s.min(); }
                if (rep >= 0) { emit("weighted-fit", which ? "weighted" : "unit-repeated", n, rep, c.ms(), sum); }}}
        for (int n : {2500, 5000, 10000, 200000}) {
            const lng M = 1000000;
            vector<lng> pts(n);
            for (auto &x : pts) { x = rnd(-M + 1, M - 1); }
            for (int impl = 0; impl < 2; ++impl) {
                int which = (impl + rep + 2) % 2;
                if (which == 0 && n > 10000) { continue; }
                Clock c; lll sum = 0;
                auto run = [&](auto &s) {
                    for (int i = 0; i < n; ++i) { s.addAbs(pts[i], 1); s.addAbs(i % 2 ? -M : M, n); }};
                if (which == 0) { PairSlopeTrick s; run(s); sum = s.mn; }
                else { WeightedSlopeTrick<lng> s; run(s); sum = s.min(); }
                if (rep >= 0) { emit("ping-pong", which ? "weighted" : "pair-heaps", n, rep, c.ms(), sum); }}}
        {
            int n = 200000;
            vector<int> par(n, -1); vector<lng> a(n);
            for (int i = 1; i < n; ++i) { par[i] = int(rnd(std::max(0, i - 64), i - 1)); }
            for (auto &x : a) { x = rnd(0, 1000000000); }
            for (int impl = 0; impl < 2; ++impl) {
                int which = (impl + rep + 2) % 2;
                Clock c; lll sum = 0;
                auto run = [&](auto &fs) {
                    for (int v = n - 1; v >= 0; --v) {
                        fs[v].prefixMin(); fs[v].addAbs(a[v]);
                        if (v) { fs[par[v]].merge(std::move(fs[v])); }}
                    return fs[0].min();};
                if (which == 0) { vector<SlopeTrick<lng>> fs(n); sum = run(fs); }
                else { vector<WeightedSlopeTrick<lng>> fs(n); sum = run(fs); }
                if (rep >= 0) { emit("tree-merge", which ? "weighted" : "unit", n, rep, c.ms(), sum); }}}
        {
            int n = 200000;
            vector<lng> p(n), q(n);
            for (int i = 0; i < n; ++i) { p[i] = rnd(-1000000000, 1000000000); q[i] = rnd(-1000000000, 1000000000); }
            for (int impl = 0; impl < 2; ++impl) {
                int which = (impl + rep + 2) % 2;
                Clock c; lll sum = 0;
                auto run = [&](auto &s) {
                    for (int i = 0; i < n; ++i) { lng d = q[i] - p[i]; s.addLinear(-d); s.slide(0, 1); s.addLinear(d); s.addConst(p[i]); }};
                if (which == 0) { DualSlopeTrick<lng> s; run(s); s.shift(-n / 2); sum = s.atZero(); }
                else { WeightedSlopeTrick<lng> s; s.restrictDomain(0, 0); run(s); sum = s.evaluate(n / 2); }
                if (rep >= 0) { emit("choose-half", which ? "weighted" : "dual", n, rep, c.ms(), sum); }}}}}
