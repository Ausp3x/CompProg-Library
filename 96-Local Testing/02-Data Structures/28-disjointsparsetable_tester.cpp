#include "../../02-Data Structures/28-disjointsparsetable.hpp"

namespace {
    ulng seed = 20261014;
    string mode = "full", context;
    lng checks = 0, ops = 0;
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    lng power(lng a, lng e) {
        lng r = 1;
        for (a %= P; e; e >>= 1, a = a * a % P) { if (e & 1) { r = r * a % P; } }
        return r;}

    // Affine maps x -> a x + b mod P; op(p, q) applies p first, so products are noncommutative; ops counts calls.
    struct Affine {
        using S = pair<lng, lng>;
        static S op(const S &p, const S &q) { ++ops; return {q.first * p.first % P, (q.first * p.second + q.second) % P}; }
        static S e() { return {1, 0}; }
    };
    struct Concat {
        using S = string;
        static S op(const S &p, const S &q) { return p + q; }
        static S e() { return ""; }
    };
    struct Min {
        using S = int;
        static S op(S a, S b) { return min(a, b); }
        static S e() { return INT_MAX; }
    };
    // A semigroup without an identity: prod on nonempty ranges and the table need only op.
    struct MaxSemi {
        using S = lng;
        static S op(S a, S b) { return max(a, b); }
    };
    Affine::S inverse(const Affine::S &f) { lng ia = power(f.first, P - 2); return {ia, (P - ia * f.second % P) % P}; }
    Affine::S value(Rng &rng, Affine *) { return {rnd(rng, 1, P - 1), rnd(rng, 0, P - 1)}; }
    string value(Rng &rng, Concat *) { return string(1, char('a' + rnd(rng, 0, 25))) + (rng() % 4 ? "" : "z"); }
    int value(Rng &rng, Min *) { return int(rnd(rng, -5, 5)); }

    template<typename M> typename M::S naive(const vector<typename M::S> &a, int l, int r) {
        typename M::S res = M::e();
        for (int i = l; i < r; ++i) { res = M::op(res, a[i]); }
        return res;}

    // All (l, r) for small n: prod and the offline products, against a left fold.
    template<typename M>
    void exhaustive(Rng &rng, int maxN, const string &name) {
        for (int n = 0; n <= maxN; ++n) {
            vector<typename M::S> a(n);
            for (auto &x : a) { x = value(rng, (M *)nullptr); }
            DisjointSparseTable<M> t(a);
            vector<pair<int, int>> qs;
            for (int l = 0; l <= n; ++l) {
                for (int r = l; r <= n; ++r) {
                    context = name + " exhaustive n=" + show(n) + " l=" + show(l) + " r=" + show(r);
                    auto want = naive<M>(a, l, r);
                    checkEqual(t.prod(l, r), want, "prod");
                    qs.push_back({l, r});}}
            std::shuffle(qs.begin(), qs.end(), rng);
            auto res = DisjointSparseTable<M>::staticRangeProducts(a, qs);
            checkEqual(res.size(), qs.size(), "result count");
            for (size_t j = 0; j < qs.size(); ++j) {
                context = name + " offline n=" + show(n) + " query=" + show(qs[j]);
                checkEqual(res[j], naive<M>(a, qs[j].first, qs[j].second), "staticRangeProducts");}}}

    template<typename M>
    void randomMedium(Rng &rng, int rounds, int maxN, const string &name) {
        for (int s = 0; s < rounds; ++s) {
            int n = int(rnd(rng, 0, maxN));
            vector<typename M::S> a(n);
            for (auto &x : a) { x = value(rng, (M *)nullptr); }
            DisjointSparseTable<M> t(a);
            vector<pair<int, int>> qs;
            for (int j = 0; j < 200; ++j) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                if (l > r) { swap(l, r); }
                if (rng() % 3 == 0 && n) { r = min(n, l + int(rnd(rng, 0, 40))); }
                qs.push_back({l, r});
                context = name + " medium n=" + show(n) + " l=" + show(l) + " r=" + show(r);
                checkEqual(t.prod(l, r), naive<M>(a, l, r), "prod");}
            auto res = DisjointSparseTable<M>::staticRangeProducts(a, qs);
            for (size_t j = 0; j < qs.size(); ++j) { checkEqual(res[j], naive<M>(a, qs[j].first, qs[j].second), "staticRangeProducts " + show(qs[j])); }}}

    // Affine maps with a != 0 form a group: the product of [l, r) is inverse(pre[l]) * pre[r], and op calls stay within the bounds.
    void largeGroup(Rng &rng, int n, int q) {
        vector<Affine::S> a(n), pre(n + 1, Affine::e());
        for (auto &x : a) { x = value(rng, (Affine *)nullptr); }
        lng saved = ops;
        for (int i = 0; i < n; ++i) { pre[i + 1] = Affine::op(pre[i], a[i]); }
        auto want = [&](int l, int r) { return Affine::op(inverse(pre[l]), pre[r]); };
        int sb = 1;
        while (sb < int(std::bit_width(uint(n)))) { sb *= 2; }
        int lev = 0;
        while ((1 << lev) < sb) { ++lev; }
        ops = saved;
        DisjointSparseTable<Affine> t(a);
        context = "large build n=" + show(n);
        checkEqual(ops - saved <= lng(n) * max(0, t.h - 1), true, "build op calls " + show(ops - saved));
        vector<vector<pair<int, int>>> sets(6);
        for (int j = 0; j < q; ++j) {
            int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
            if (l > r) { swap(l, r); }
            sets[0].push_back({l, r});
            int b = int(rnd(rng, 0, (n - 1) / sb)) * sb, e = min(n, b + sb);
            int x = int(rnd(rng, b, e - 1)), y = int(rnd(rng, x + 1, e));
            sets[1].push_back({x, y});
            int c = min(n, b + 2 * sb);
            sets[2].push_back({int(rnd(rng, b, e - 1)), int(rnd(rng, min(e + 1, c), c))});
            sets[3].push_back({b, min(n, int(rnd(rng, b / sb + 1, max(b / sb + 1, n / sb))) * sb)});
            sets[4].push_back({0, n});
            int m = int(rnd(rng, 0, n));
            sets[5].push_back({m, min(n, m + int(rnd(rng, 0, 1)))});}
        for (auto &qs : sets) {
            for (auto &[l, r] : qs) { if (l > r) { swap(l, r); } }}
        for (int k = 0; k < 6; ++k) {
            context = "large n=" + show(n) + " set=" + show(k);
            for (int j = 0; j < q; j += 1 + q / 2000) {
                auto [l, r] = sets[k][j];
                checkEqual(t.prod(l, r), want(l, r), "prod " + show(sets[k][j]));}
            saved = ops;
            auto res = DisjointSparseTable<Affine>::staticRangeProducts(a, sets[k]);
            lng used = ops - saved, blocks = (n + sb - 1) / sb;
            lng bound = 2 * lng(n) + lng(n) * lev + blocks * int(std::bit_width(ulng(blocks))) + 3 * lng(q);
            checkEqual(used <= bound, true, "offline op calls " + show(used) + " bound " + show(bound));
            for (int j = 0; j < q; ++j) { checkEqual(res[j], want(sets[k][j].first, sets[k][j].second), "staticRangeProducts " + show(sets[k][j])); }}}

    void semigroup(Rng &rng) {
        for (int n = 1; n <= 70; ++n) {
            vector<lng> a(n);
            for (lng &x : a) { x = rnd(rng, -9, 9); }
            DisjointSparseTable<MaxSemi> t(a);
            for (int l = 0; l < n; ++l) {
                for (int r = l + 1; r <= n; ++r) {
                    context = "semigroup n=" + show(n) + " l=" + show(l) + " r=" + show(r);
                    checkEqual(t.prod(l, r), *std::max_element(a.begin() + l, a.begin() + r), "semigroup prod");}}}}

    void edges() {
        context = "edges";
        DisjointSparseTable<Min> e(vector<int>{});
        checkEqual(e.prod(0, 0), INT_MAX, "empty prod");
        checkEqual(DisjointSparseTable<Min>::staticRangeProducts({}, {{0, 0}, {0, 0}}), vector<int>{INT_MAX, INT_MAX}, "empty offline");
        checkEqual(DisjointSparseTable<Min>::staticRangeProducts({4, 2, 9}, {}).size(), size_t(0), "no queries");
        DisjointSparseTable<Concat> c(vector<string>{"a", "b", "c", "d", "e"});
        checkEqual(c.prod(1, 4), string("bcd"), "concat order"); checkEqual(c.prod(3, 3), string(), "prod empty");
        DisjointSparseTable<Concat> copy = c;
        checkEqual(copy.prod(0, 5), string("abcde"), "copy");
        checkEqual(DisjointSparseTable<Concat>::staticRangeProducts({"x", "y"}, {{0, 2}, {1, 1}, {1, 2}, {0, 2}}), vector<string>{"xy", "", "y", "xy"}, "repeated queries");
        // Runtime state: sums modulo a member modulus, in the table and in the offline products.
        struct ModSum { lng mod; using S = lng; S op(S a, S b) const { return (a + b) % mod; } S e() const { return 0; } };
        for (lng mod : {lng(5), lng(1000000007)}) {
            vector<lng> a{4, 9, 2, 7, 3, 8, 1};
            for (lng &x : a) { x %= mod; }
            DisjointSparseTable st(a, ModSum{mod});
            vector<pair<int, int>> qs;
            vector<lng> want;
            for (int l = 0; l <= 7; ++l) {
                for (int r = l; r <= 7; ++r) {
                    lng s = 0;
                    for (int i = l; i < r; ++i) { s = (s + a[i]) % mod; }
                    checkEqual(st.prod(l, r), s, "stateful prod"); qs.push_back({l, r}); want.push_back(s);}}
            checkEqual(DisjointSparseTable<ModSum>::staticRangeProducts(a, qs, ModSum{mod}), want, "stateful staticRangeProducts");}}

    void invalid(const string &name) {
        DisjointSparseTable<Min> t(vector<int>{3, 1, 4});
        DisjointSparseTable<MaxSemi> g(vector<MaxSemi::S>(3, MaxSemi::S(1)));
        if (name == "semigroup-prod-empty") { (void)g.prod(1, 1); }
        else if (name == "prod-range") { (void)t.prod(0, 4); }
        else if (name == "prod-negative") { (void)t.prod(-1, 2); }
        else if (name == "prod-reversed") { (void)t.prod(2, 1); }
        else if (name == "static-reversed") { (void)DisjointSparseTable<Min>::staticRangeProducts({3, 1, 4}, {{2, 1}}); }
        else if (name == "static-range") { (void)DisjointSparseTable<Min>::staticRangeProducts({3, 1, 4}, {{0, 4}}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else if (arg == "--invalid" && i + 1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "disjointsparsetable seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        semigroup(rng);
        int small = mode == "quick" ? 20 : mode == "full" ? 70 : 140;
        exhaustive<Affine>(rng, small, "affine"); exhaustive<Concat>(rng, small / 2, "concat"); exhaustive<Min>(rng, small, "min");
        int rounds = mode == "quick" ? 20 : mode == "full" ? 300 : 2000;
        randomMedium<Affine>(rng, rounds, 3000, "affine"); randomMedium<Concat>(rng, rounds / 4, 600, "concat");
        for (int n : {1, 2, 3, 31, 32, 33, 255, 256, 257, 4095, 4096, 4097}) { largeGroup(rng, n, 500); }
        if (mode != "quick") { largeGroup(rng, 1 << 17, 1 << 17); largeGroup(rng, 100003, 50000); }
        if (mode == "stress") { largeGroup(rng, 1 << 20, 1 << 19); largeGroup(rng, (1 << 20) + 1, 1 << 18); }
        std::cout << "PASS disjointsparsetable checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL disjointsparsetable seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
