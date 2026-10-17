#include "../../02-Data Structures/34-sqrttree.hpp"

// Emits one JSON line per workload; VARIANT names the layering threshold the header was built with.
#ifndef VARIANT
#define VARIANT "library"
#endif

namespace {
    using Clock = std::chrono::steady_clock;
    constexpr lng P = 998244353;
    struct Min {
        using S = int;
        static S op(S a, S b) { return min(a, b); }
        static S e() { return INT_MAX; }
    };
    struct Affine {
        using S = pair<lng, lng>;
        static S op(const S &p, const S &q) { return {q.first * p.first % P, (q.first * p.second + q.second) % P}; }
        static S e() { return {1, 0}; }
    };
    ulng digest(int x) { return ulng(x); }
    ulng digest(const pair<lng, lng> &x) { return ulng(x.first) * 1000003 + ulng(x.second); }

    // Build, q random ranges, q short ranges (length <= 16), q / 200 point sets; cells counts stored values per element.
    template<typename M, typename G>
    void run(const string &name, int n, int q, ulng seed, G gen) {
        std::mt19937_64 rng(seed);
        vector<typename M::S> a(n);
        for (auto &x : a) { x = gen(rng); }
        vector<pair<int, int>> lr(q), sh(q);
        for (auto &[l, r] : lr) { l = int(rng() % ulng(n)); r = int(rng() % ulng(n)); if (l > r) { swap(l, r); } ++r; }
        for (auto &[l, r] : sh) { l = int(rng() % ulng(n - 16)); r = l + 1 + int(rng() % 16); }
        auto t0 = Clock::now();
        SqrtTree<M> t(a);
        double build = std::chrono::duration<double>(Clock::now() - t0).count();
        lng cells = lng(t.v.size());
        for (auto *g : {&t.pre, &t.suf, &t.btw}) { for (auto &v : *g) { cells += lng(v.size()); } }
        ulng sum = 0;
        t0 = Clock::now();
        for (auto [l, r] : lr) { sum = sum * 31 + digest(t.query(l, r)); }
        double random = std::chrono::duration<double>(Clock::now() - t0).count();
        t0 = Clock::now();
        for (auto [l, r] : sh) { sum = sum * 31 + digest(t.query(l, r)); }
        double shortq = std::chrono::duration<double>(Clock::now() - t0).count();
        t0 = Clock::now();
        for (int k = 0; k < q / 200; ++k) { t.set(int(rng() % ulng(n)), gen(rng)); }
        double sets = std::chrono::duration<double>(Clock::now() - t0).count();
        sum = sum * 31 + digest(t.query(0, n));
        std::cout << "{\"workload\":\"" << name << "\",\"variant\":\"" << VARIANT << "\",\"n\":" << n << ",\"q\":" << q << ",\"layers\":" << t.lay.size()
                  << ",\"cells_per_n\":" << double(cells) / n << ",\"build\":" << build << ",\"random\":" << random << ",\"short\":" << shortq
                  << ",\"sets\":" << sets << ",\"checksum\":" << sum << "}\n";}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261017;
    run<Min>("min", 1 << 20, 4000000, seed, [](std::mt19937_64 &g) { return int(g() % 1000000000); });
    run<Min>("min", 1000003, 4000000, seed + 1, [](std::mt19937_64 &g) { return int(g() % 1000000000); });
    run<Affine>("affine", 1 << 18, 2000000, seed + 2, [](std::mt19937_64 &g) { return pair<lng, lng>(lng(g() % P), lng(g() % P)); });
    return 0;}
