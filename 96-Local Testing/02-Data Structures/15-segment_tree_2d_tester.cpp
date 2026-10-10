#include "../../02-Data Structures/15-segment_tree_2d.hpp"
#include "../../02-Data Structures/00-monoids.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    ulng seed = 20261010;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    struct Sum { using S = lng; static S op(S a, S b) { return a + b; } static S e() { return 0; } static S unit(lng x) { return x; } };
    struct Min { using S = lng; static S op(S a, S b) { return min(a, b); } static S e() { return std::numeric_limits<lng>::max(); } static S unit(lng x) { return x; } };
    struct AddAct { using F = lng; static F composition(F f, F g) { return f + g; } static F id() { return 0; } };
    struct MaxAct { using F = lng; static F composition(F f, F g) { return max(f, g); } static F id() { return std::numeric_limits<lng>::min(); } };
    pair<int, int> range(Rng &rng, int n) {
        int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
        if (l > r) { swap(l, r); }
        return {l, r};}

    template<typename M>
    void denseRun(Rng &rng, int n, int m, int steps, const string &name) {
        vector<vector<lng>> a(n, vector<lng>(m));
        for (auto &row : a) { for (auto &x : row) { x = rnd(rng, -9, 9); }}
        bool built = n && rng() % 2;
        SegmentTree2DDense<M> t = built ? SegmentTree2DDense<M>(a) : SegmentTree2DDense<M>(n, m);
        if (!built) {
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < m; ++j) { if (rng() % 2) { t.set(i, j, a[i][j]); } else { t.set(i, j, M::e()); t.apply(i, j, a[i][j]); }}}}
        for (int s = 0; s < steps; ++s) {
            context = name + " dense n=" + show(n) + " m=" + show(m) + " step=" + show(s);
            if (n && m) {
                int i = int(rnd(rng, 0, n - 1)), j = int(rnd(rng, 0, m - 1));
                lng x = rnd(rng, -9, 9);
                if (rng() % 2) { t.set(i, j, x); a[i][j] = x; } else { t.apply(i, j, x); a[i][j] = M::op(a[i][j], x); }
                checkEqual(t.get(i, j), a[i][j], "get");}
            auto [xl, xr] = range(rng, n); auto [yl, yr] = range(rng, m);
            lng want = M::e();
            for (int i = xl; i < xr; ++i) { for (int j = yl; j < yr; ++j) { want = M::op(want, a[i][j]); }}
            checkEqual(t.prod(xl, xr, yl, yr), want, "prod " + show(xl) + "," + show(xr) + "," + show(yl) + "," + show(yr));
            lng all = M::e();
            for (auto &row : a) { for (lng x : row) { all = M::op(all, x); }}
            checkEqual(t.allProd(), all, "allProd");}}

    template<typename A>
    void dualRun(Rng &rng, int n, int m, int steps, const string &name) {
        vector<vector<lng>> a(n, vector<lng>(m, A::id()));
        DualSegmentTree2DDense<A> t(n, m);
        for (int s = 0; s < steps; ++s) {
            context = name + " dual n=" + show(n) + " m=" + show(m) + " step=" + show(s);
            auto [xl, xr] = range(rng, n); auto [yl, yr] = range(rng, m);
            lng f = rnd(rng, -9, 9);
            t.apply(xl, xr, yl, yr, f);
            for (int i = xl; i < xr; ++i) { for (int j = yl; j < yr; ++j) { a[i][j] = A::composition(f, a[i][j]); }}
            if (n && m) { int i = int(rnd(rng, 0, n - 1)), j = int(rnd(rng, 0, m - 1)); checkEqual(t.get(i, j), a[i][j], "get"); }
            if (s % 16 == 0) { checkEqual(t.values() == a, true, "values"); }}}

    template<typename M>
    void sparseRun(Rng &rng, int k, lng span, int steps, const string &name) {
        vector<pair<lng, lng>> p(k);
        vector<lng> w(k);
        for (int i = 0; i < k; ++i) { p[i] = {rnd(rng, -span, span), rnd(rng, -span, span)}; w[i] = rnd(rng, -9, 9); }
        bool weighted = rng() % 2;
        SegmentTree2DSparse<M> t(p, weighted ? w : vector<lng>{});
        map<pair<lng, lng>, lng> a;
        for (int i = 0; i < k; ++i) { a.emplace(p[i], M::e()); }
        if (weighted) { for (int i = 0; i < k; ++i) { a[p[i]] = M::op(a[p[i]], w[i]); }}
        for (int s = 0; s < steps; ++s) {
            context = name + " sparse k=" + show(k) + " span=" + show(span) + " step=" + show(s);
            if (k) {
                auto pt = p[rng() % k];
                lng x = rnd(rng, -9, 9);
                if (rng() % 2) { t.set(pt.first, pt.second, x); a[pt] = x; } else { t.apply(pt.first, pt.second, x); a[pt] = M::op(a[pt], x); }
                checkEqual(t.get(pt.first, pt.second), a[pt], "get");}
            lng xl = rnd(rng, -span - 2, span + 2), xr = rnd(rng, -span - 2, span + 2), yl = rnd(rng, -span - 2, span + 2), yr = rnd(rng, -span - 2, span + 2);
            if (rng() % 8 == 0) { xl = std::numeric_limits<lng>::min(); xr = std::numeric_limits<lng>::max(); }
            lng want = M::e(), all = M::e();
            for (auto &[q, v] : a) {
                all = M::op(all, v);
                if (xl <= q.first && q.first < xr && yl <= q.second && q.second < yr) { want = M::op(want, v); }}
            checkEqual(t.prod(xl, xr, yl, yr), want, "prod " + show(xl) + "," + show(xr) + "," + show(yl) + "," + show(yr));
            checkEqual(t.allProd(), all, "allProd");}}

    using AS = RangeAffineRangeSum<mint>;
    void kdRun(Rng &rng, int k, lng span, int steps, bool degenerate) {
        vector<pair<lng, lng>> p(k);
        for (auto &q : p) { q = {degenerate && rng() % 2 ? 7 : rnd(rng, -span, span), degenerate && rng() % 3 == 0 ? -3 : rnd(rng, -span, span)}; }
        vector<lng> val(k, 0);
        vector<bool> on(k, false);
        vector<AS::S> w;
        if (rng() % 2) { for (int i = 0; i < k; ++i) { val[i] = rnd(rng, 0, P - 1); on[i] = true; w.push_back(AS::leaf(mint(val[i]))); }}
        LazyKdTree<AS> t(p, w);
        auto inside = [&](int i, lng xl, lng xr, lng yl, lng yr) { return xl <= p[i].first && p[i].first < xr && yl <= p[i].second && p[i].second < yr; };
        for (int s = 0; s < steps; ++s) {
            context = "kd k=" + show(k) + " span=" + show(span) + " degenerate=" + show(degenerate) + " step=" + show(s);
            lng xl = rnd(rng, -span - 1, span + 1), xr = rnd(rng, -span - 1, span + 2), yl = rnd(rng, -span - 1, span + 1), yr = rnd(rng, -span - 1, span + 2);
            int kind = int(rng() % 4);
            if (kind == 0 && k) { int i = int(rng() % k); val[i] = rnd(rng, 0, P - 1); on[i] = true; t.set(i, AS::leaf(mint(val[i]))); }
            else if (kind == 1) {
                lng a = rnd(rng, 0, P - 1), b = rnd(rng, 0, P - 1);
                t.apply(xl, xr, yl, yr, {mint(a), mint(b)});
                for (int i = 0; i < k; ++i) { if (on[i] && inside(i, xl, xr, yl, yr)) { val[i] = (a * val[i] + b) % P; }}}
            lng sum = 0, cnt = 0, all = 0;
            for (int i = 0; i < k; ++i) {
                if (!on[i]) { continue; }
                all = (all + val[i]) % P;
                if (inside(i, xl, xr, yl, yr)) { sum = (sum + val[i]) % P; ++cnt; }}
            auto got = t.prod(xl, xr, yl, yr);
            checkEqual(lng(got.sum.val()), sum, "prod.sum"); checkEqual(lng(got.len.val()), cnt, "prod.len");
            checkEqual(lng(t.allProd().sum.val()), all, "allProd");
            if (k) { int i = int(rng() % k); auto g = t.get(i); checkEqual(lng(g.sum.val()), on[i] ? val[i] : 0, "get"); }}}

    void edges() {
        context = "edges";
        SegmentTree2DDense<Sum> z(0, 5), z2(4, 0), z3;
        checkEqual(z.allProd(), 0, "dense empty rows"); checkEqual(z2.prod(0, 4, 0, 0), 0, "dense empty cols"); checkEqual(z3.allProd(), 0, "dense default");
        SegmentTree2DDense<Min> one(vector<vector<lng>>{{5}});
        checkEqual(one.allProd(), 5, "dense 1x1"); one.apply(0, 0, 3); checkEqual(one.get(0, 0), 3, "dense 1x1 apply");
        SegmentTree2DDense<Sum> c(vector<vector<lng>>{{1, 2, 3}, {4, 5, 6}});
        auto copy = c;
        copy.set(1, 2, 100);
        checkEqual(c.prod(0, 2, 0, 3), 21, "dense copy independent"); checkEqual(copy.prod(1, 2, 2, 3), 100, "dense copy");
        DualSegmentTree2DDense<AddAct> d0(0, 0);
        checkEqual(d0.values().size(), size_t(0), "dual empty values");
        SegmentTree2DSparse<Sum> s0;
        checkEqual(s0.prod(-5, 5, -5, 5), 0, "sparse empty"); checkEqual(s0.allProd(), 0, "sparse empty all");
        lng big = std::numeric_limits<lng>::max();
        SegmentTree2DSparse<Sum> s1({{big - 1, -big}, {big - 1, -big}, {-big, big - 1}}, {2, 3, 4});
        checkEqual(s1.get(big - 1, -big), 5, "sparse duplicate weights"); checkEqual(s1.prod(-big, big, -big, big), 9, "sparse extreme coordinates");
        checkEqual(s1.prod(0, 0, -big, big), 0, "sparse empty x");
        LazyKdTree<AS> k0;
        checkEqual(lng(k0.prod(-5, 5, -5, 5).len.val()), lng(0), "kd empty"); k0.apply(-5, 5, -5, 5, {mint(2), mint(3)});
        LazyKdTree<AS> k1({{1, 1}, {1, 1}}, {AS::leaf(mint(4)), AS::leaf(mint(6))});
        k1.apply(1, 2, 1, 2, {mint(2), mint(1)});
        checkEqual(lng(k1.get(0).sum.val()), lng(9), "kd duplicate point"); checkEqual(lng(k1.prod(0, 5, 0, 5).sum.val()), lng(22), "kd duplicate sum");}

    void invalid(const string &name) {
        SegmentTree2DDense<Sum> t(3, 4);
        DualSegmentTree2DDense<AddAct> d(3, 4);
        SegmentTree2DSparse<Sum> s({{0, 0}, {1, 2}});
        LazyKdTree<AS> k({{0, 0}, {1, 2}});
        if (name == "dense-negative") { SegmentTree2DDense<Sum> x(-1, 2); }
        else if (name == "dense-oversize") { SegmentTree2DDense<Sum> x(1 << 15, 1 << 14); }
        else if (name == "dense-ragged") { SegmentTree2DDense<Sum> x(vector<vector<lng>>{{1, 2}, {3}}); }
        else if (name == "dense-get-end") { (void)t.get(3, 0); }
        else if (name == "dense-set-negative") { t.set(0, -1, 1); }
        else if (name == "dense-apply-end") { t.apply(0, 4, 1); }
        else if (name == "dense-prod-reversed") { (void)t.prod(2, 1, 0, 4); }
        else if (name == "dense-prod-end") { (void)t.prod(0, 3, 0, 5); }
        else if (name == "dual-negative") { DualSegmentTree2DDense<AddAct> x(2, -1); }
        else if (name == "dual-get-end") { (void)d.get(0, 4); }
        else if (name == "dual-apply-reversed") { d.apply(0, 3, 3, 2, 1); }
        else if (name == "sparse-weights") { SegmentTree2DSparse<Sum> x({{0, 0}}, {1, 2}); }
        else if (name == "sparse-set-missing") { s.set(1, 1, 5); }
        else if (name == "sparse-get-missing") { (void)s.get(2, 2); }
        else if (name == "kd-weights") { LazyKdTree<AS> x({{0, 0}}, {AS::leaf(mint(1)), AS::leaf(mint(2))}); }
        else if (name == "kd-get-end") { (void)k.get(2); }
        else if (name == "kd-set-negative") { k.set(-1, AS::leaf(mint(1))); }
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
        std::cout << "segment_tree_2d seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        vector<int> sizes = mode == "quick" ? vector<int>{0, 1, 2, 3, 5} : vector<int>{0, 1, 2, 3, 4, 5, 7, 8, 9, 13, 16, 17};
        if (mode == "stress") { sizes.push_back(33); sizes.push_back(64); }
        int steps = mode == "quick" ? 60 : mode == "full" ? 400 : 1500;
        for (int n : sizes) {
            for (int m : sizes) {
                denseRun<Sum>(rng, n, m, steps / 4, "sum"); denseRun<Min>(rng, n, m, steps / 4, "min");
                dualRun<AddAct>(rng, n, m, steps / 4, "add"); dualRun<MaxAct>(rng, n, m, steps / 4, "max");}}
        vector<int> counts = mode == "quick" ? vector<int>{0, 1, 2, 5, 17} : vector<int>{0, 1, 2, 3, 4, 7, 8, 9, 16, 33, 100, 257};
        if (mode == "stress") { counts.push_back(1000); }
        for (int k : counts) {
            for (lng span : {lng(2), lng(6), lng(1000000000000000000)}) {
                sparseRun<Sum>(rng, k, span, steps, "sum"); sparseRun<Min>(rng, k, span, steps, "min");
                kdRun(rng, k, span, steps, false); kdRun(rng, k, span, steps, true);}}
        std::cout << "PASS segment_tree_2d checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL segment_tree_2d seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
