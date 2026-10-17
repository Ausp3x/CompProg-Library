#include "../../02-Data Structures/12-lazysegmenttree.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    ulng seed = 20261008;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename... T> string show(const tuple<T...> &t) {
        string s = "(";
        std::apply([&](const auto &...x) { int k = 0; ((s += (k++ ? "," : "") + show(x)), ...); }, t);
        return s + ")";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    lng mulmod(lng a, lng b) { return lng(lll(a) * b % P); }

    // Acted specs: draw(rng, a, l, r) applies the action's meaning to a[l, r) and returns the preset's encoding of it.
    struct AddSum {
        using A = RangeAddRangeSum<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -5, 5); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { lng x = rnd(rng, -3, 3); for (int i = l; i < r; ++i) { a[i] += x; } return A::add(x); }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.sum, s.len); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s += a[i]; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
        using X = lng;
        static X seedX(Rng &rng) { return rnd(rng, -9, 9); }
        static X right(X x, const A::S &s) { return x + s.sum; }
        static X left(const A::S &s, X x) { return s.sum + x; }
        static X slow(const vector<V> &a, int l, int r, X x, bool) { for (int i = l; i < r; ++i) { x += a[i]; } return x; }
        static X keyX(X x) { return x; }
    };
    tuple<lng, lng, lng> composite(const vector<pair<lng, lng>> &a, int l, int r) {
        lng ca = 1, cb = 0;
        for (int k = 0; k < r - l; ++k) {
            auto [x, y] = a[l + k];
            ca = mulmod(x, ca); cb = (mulmod(x, cb) + y) % P;}
        return tuple(ca, cb, lng(r - l));}
    template<typename A>
    typename A::F drawComposite(Rng &rng, vector<pair<lng, lng>> &a, int l, int r) {
        if (rng() % 4 == 0) { return A::id(); }
        lng x = rnd(rng, 0, P - 1), y = rnd(rng, 0, P - 1);
        for (int i = l; i < r; ++i) { a[i] = {x, y}; }
        return A::assign(mint(x), mint(y));}
    struct AssignComposite {
        using A = RangeAssignRangeComposite<mint>; using V = pair<lng, lng>;
        static V value(Rng &rng) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { return drawComposite<A>(rng, a, l, r); }
        static A::S leaf(int, V v) { return A::leaf(mint(v.first), mint(v.second)); }
        static auto key(const A::S &s) { return tuple(lng(s.a.val()), lng(s.b.val()), s.len); }
        static auto expect(const vector<V> &a, int l, int r) { return composite(a, l, r); }
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
        using X = A::S;
        static X seedX(Rng &rng) { return leaf(0, value(rng)); }
        static X right(const X &x, const A::S &s) { return A::op(x, s); }
        static X left(const A::S &s, const X &x) { return A::op(s, x); }
        static auto slow(const vector<V> &a, int l, int r, const X &x, bool fromLeft) {
            vector<V> b(a.begin() + l, a.begin() + r);
            b.insert(fromLeft ? b.begin() : b.end(), V(x.a.val(), x.b.val()));
            return composite(b, 0, int(b.size()));}
        static auto keyX(const X &x) { return key(x); }
    };
    struct SumMinMaxIndex {
        using A = RangeAffineRangeSumMinMaxIndex<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -4, 4); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, -1, 1), y = rnd(rng, -3, 3);
            for (int i = l; i < r; ++i) { a[i] = x * a[i] + y; }
            return A::affine(x, y);}
        static A::S leaf(int i, V v) { return A::leaf(i, v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.sum, s.mx, s.mn, s.len, s.mxi, s.mni) : tuple(lng(0), lng(0), lng(0), lng(0), lng(-1), lng(-1)); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0), lng(0), lng(0), lng(-1), lng(-1)); }
            lng s = 0, mx = a[l], mn = a[l], mxi = l, mni = l;
            for (int i = l; i < r; ++i) {
                s += a[i];
                if (a[i] > mx) { mx = a[i]; mxi = i; }
                if (a[i] < mn) { mn = a[i]; mni = i; }}
            return tuple(s, mx, mn, lng(r - l), mxi, mni);}
        static bool ok(const auto &k, lng t) { return std::get<3>(k) == 0 || (std::get<2>(k) >= t && std::get<1>(k) <= t + 6); }
    };
    // Runtime state in the tree: affine sums modulo a modulus chosen by the seed.
    lng runtimeMod = 1000000007;
    struct ModAffine {
        lng mod;
        struct S { lng sum, len; };
        struct F { lng a, b; };
        S op(const S &x, const S &y) const { return {(x.sum + y.sum) % mod, x.len + y.len}; }
        S e() const { return {0, 0}; }
        S mapping(const F &f, const S &x) const { return {lng((lll(f.a) * x.sum + lll(f.b) * x.len) % mod), x.len}; }
        F composition(const F &f, const F &g) const { return {lng(lll(f.a) * g.a % mod), lng((lll(f.a) * g.b + f.b) % mod)}; }
        F id() const { return {1, 0}; }
    };
    struct StatefulAffine {
        using A = ModAffine; using V = lng;
        static A make() { return {runtimeMod}; }
        static V value(Rng &rng) { return rnd(rng, 0, runtimeMod - 1); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, 0, runtimeMod - 1), y = rnd(rng, 0, runtimeMod - 1);
            for (int i = l; i < r; ++i) { a[i] = lng((lll(x) * a[i] + y) % runtimeMod); }
            return {x, y};}
        static A::S leaf(int, V v) { return {v, 1}; }
        static auto key(const A::S &s) { return tuple(s.sum, s.len); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s = (s + a[i]) % runtimeMod; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    // Specs with X check fold and foldRight, each on a copy so it relies on no earlier push; right and left act on X, slow folds a[l, r) directly.
    template<typename C, typename T>
    void checkFolds(Rng &rng, const T &t, const vector<typename C::V> &a, int l, int r, const string &when) {
        if constexpr (requires { typename C::X; }) {
            using S = typename C::A::S;
            using X = typename C::X;
            int calls = 0;
            auto right = [&](X u, const S &s) { ++calls; return C::right(u, s); };
            auto left = [&](const S &s, X u) { ++calls; return C::left(s, u); };
            X x = C::seedX(rng);
            string at = "(" + show(l) + "," + show(r) + ")";
            T u = t, w = t;
            checkEqual(C::keyX(u.fold(l, r, x, right)), C::slow(a, l, r, x, true), when + " fold" + at);
            checkEqual(calls <= 2 * (t.lg + 1), true, when + " fold act calls" + at);
            calls = 0;
            checkEqual(C::keyX(w.foldRight(l, r, x, left)), C::slow(a, l, r, x, false), when + " foldRight" + at);
            checkEqual(calls <= 2 * (t.lg + 1), true, when + " foldRight act calls" + at);}}
    template<typename C>
    typename C::A makeMonoid() {
        if constexpr (requires { C::make(); }) { return C::make(); }
        else { return typename C::A{}; }}

    template<typename C>
    void history(Rng &rng, const string &name, int n, int steps) {
        using A = typename C::A;
        using S = typename A::S;
        vector<typename C::V> a(n);
        vector<S> leaves;
        for (int i = 0; i < n; ++i) { a[i] = C::value(rng); leaves.push_back(C::leaf(i, a[i])); }
        LazySegmentTree<A> t(leaves, makeMonoid<C>());
        context = name + " n=" + show(n) + " seed=" + show(seed);
        bool all = n <= 17;
        auto verify = [&](const string &when) {
            if (all) {
                for (int l = 0; l <= n; ++l) {
                    for (int r = l; r <= n; ++r) { checkFolds<C>(rng, t, a, l, r, when); }}
                for (int l = 0; l <= n; ++l) {
                    for (int r = l; r <= n; ++r) { checkEqual(C::key(t.prod(l, r)), C::expect(a, l, r), when + " prod(" + show(l) + "," + show(r) + ")"); }}}
            checkEqual(C::key(t.allProd()), C::expect(a, 0, n), when + " allProd");};
        verify("build");
        for (int step = 0; step < steps; ++step) {
            int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
            if (l > r) { swap(l, r); }
            int p = n ? int(rnd(rng, 0, n - 1)) : 0;
            string at = "step=" + show(step);
            switch (rng() % 9) {
                case 0:
                    if (n) { a[p] = C::value(rng); t.set(p, C::leaf(p, a[p])); }
                    break;
                case 1:
                    if (n) { checkEqual(C::key(t.get(p)), C::expect(a, p, p + 1), at + " get(" + show(p) + ")"); }
                    break;
                case 2:
                    if (n) { t.apply(p, C::draw(rng, a, p, p + 1)); }
                    break;
                case 3: case 4:
                    t.apply(l, r, C::draw(rng, a, l, r));
                    break;
                case 5:
                    checkFolds<C>(rng, t, a, l, r, at);
                    checkEqual(C::key(t.prod(l, r)), C::expect(a, l, r), at + " prod(" + show(l) + "," + show(r) + ")");
                    break;
                case 6: {
                    lng th = rnd(rng, -6, n + 6);
                    if (!C::ok(C::expect(a, l, l), th)) { break; }
                    int want = l;
                    while (want < n && C::ok(C::expect(a, l, want + 1), th)) { ++want; }
                    checkEqual(t.maxRight(l, [&](const S &s) { return C::ok(C::key(s), th); }), want, at + " maxRight(" + show(l) + ",t=" + show(th) + ")");
                    break;}
                case 7: {
                    lng th = rnd(rng, -6, n + 6);
                    if (!C::ok(C::expect(a, r, r), th)) { break; }
                    int want = r;
                    while (want > 0 && C::ok(C::expect(a, want - 1, r), th)) { --want; }
                    checkEqual(t.minLeft(r, [&](const S &s) { return C::ok(C::key(s), th); }), want, at + " minLeft(" + show(r) + ",t=" + show(th) + ")");
                    break;}
                default: {
                    auto v = t.values();
                    checkEqual(int(v.size()), n, at + " values size");
                    for (int i = 0; i < n; ++i) { checkEqual(C::key(v[i]), C::expect(a, i, i + 1), at + " values[" + show(i) + "]"); }}}
            if (all) { verify(at); }
            else { checkEqual(C::key(t.allProd()), C::expect(a, 0, n), at + " allProd"); }}}

    vector<int> sizes() {
        if (mode == "quick") { return {0, 1, 2, 3, 5, 8, 9}; }
        vector<int> s{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 31, 32, 33, 100};
        if (mode == "stress") { s.push_back(257); }
        return s;}
    template<typename C>
    void preset(Rng &rng, const string &name) {
        int steps = mode == "quick" ? 60 : mode == "full" ? 300 : 600;
        for (int n : sizes()) { history<C>(rng, name, n, n > 17 ? 4 * steps : steps); }
        std::cout << "PASS lazy " << name << '\n';}

    // Counts op calls: a fold never forms a product, so besides act it only pushes (mapping, composition).
    struct CountingSum : RangeAddRangeSum<lng> {
        int *ops;
        S op(const S &x, const S &y) const { ++*ops; return RangeAddRangeSum<lng>::op(x, y); }
    };
    void foldOps(Rng &rng) {
        int ops = 0;
        for (int n : sizes()) {
            if (n > 33) { break; }
            context = "fold op count n=" + show(n);
            vector<lng> a(n);
            vector<CountingSum::S> leaves;
            for (lng &v : a) { v = rnd(rng, -5, 5); leaves.push_back(CountingSum::leaf(v)); }
            LazySegmentTree<CountingSum> t(leaves, CountingSum{{}, &ops});
            for (int step = 0; step < 4; ++step) {
                int p = int(rnd(rng, 0, n)), q = int(rnd(rng, 0, n));
                if (p > q) { swap(p, q); }
                lng f = rnd(rng, -3, 3);
                t.apply(p, q, f);
                for (int i = p; i < q; ++i) { a[i] += f; }
                for (int l = 0; l <= n; ++l) {
                    for (int r = l; r <= n; ++r) {
                        string at = "step=" + show(step) + " (" + show(l) + "," + show(r) + ")";
                        lng want = std::accumulate(a.begin() + l, a.begin() + r, lng(0));
                        auto u = t, w = t;
                        ops = 0;
                        checkEqual(u.fold(l, r, lng(0), [](lng x, const CountingSum::S &s) { return x + s.sum; }), want, at + " fold");
                        checkEqual(w.foldRight(l, r, lng(0), [](const CountingSum::S &s, lng x) { return s.sum + x; }), want, at + " foldRight");
                        checkEqual(ops, 0, at + " op calls in folds");}}}}
        std::cout << "PASS folds form no product (op never called) over pending range adds\n";}

    void dual(Rng &rng) {
        using A = RangeAffineRangeSum<mint>;
        int steps = mode == "quick" ? 100 : mode == "full" ? 1000 : 3000;
        for (int n : sizes()) {
            context = "DualSegmentTree affine n=" + show(n);
            vector<pair<lng, lng>> a(n);
            vector<A::F> init(n);
            for (int i = 0; i < n; ++i) { a[i] = {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; init[i] = {mint(a[i].first), mint(a[i].second)}; }
            DualSegmentTree<A> t(init);
            DualSegmentTree<A> z(n);
            for (int i = 0; i < n; ++i) { checkEqual(tuple(z.get(i).a.val(), z.get(i).b.val()), tuple(1u, 0u), "size ctor identity"); }
            for (int step = 0; step < steps; ++step) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                if (l > r) { swap(l, r); }
                lng fa = rnd(rng, 0, P - 1), fb = rnd(rng, 0, P - 1);
                switch (rng() % 4) {
                    case 0: case 1:
                        t.apply(l, r, A::affine(mint(fa), mint(fb)));
                        for (int i = l; i < r; ++i) { a[i] = {mulmod(fa, a[i].first), (mulmod(fa, a[i].second) + fb) % P}; }
                        break;
                    case 2:
                        if (n) { int p = l % n; t.set(p, {mint(fa), mint(fb)}); a[p] = {fa, fb}; }
                        break;
                    default: {
                        auto v = t.values();
                        for (int i = 0; i < n; ++i) { checkEqual(pair<lng, lng>(v[i].a.val(), v[i].b.val()), a[i], "values[" + show(i) + "]"); }}}
                for (int i = 0; i < n; ++i) { checkEqual(pair<lng, lng>(t.get(i).a.val(), t.get(i).b.val()), a[i], "step=" + show(step) + " get(" + show(i) + ")"); }}
            context = "CommutativeDualSegmentTree add n=" + show(n);
            using B = RangeAddRangeSum<lng>;
            vector<lng> b(n);
            for (lng &x : b) { x = rnd(rng, -9, 9); }
            CommutativeDualSegmentTree<B> c(b), cz(n);
            for (int i = 0; i < n; ++i) { checkEqual(cz.get(i), lng(0), "size ctor identity"); }
            for (int step = 0; step < steps; ++step) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                if (l > r) { swap(l, r); }
                lng f = rnd(rng, -5, 5);
                c.apply(l, r, f);
                for (int i = l; i < r; ++i) { b[i] += f; }
                if (step % 7 == 0) { checkEqual(c.values() == b, true, "values step=" + show(step)); }
                for (int i = 0; i < n; ++i) { checkEqual(c.get(i), b[i], "step=" + show(step) + " get(" + show(i) + ")"); }}}
        std::cout << "PASS dual and commutative dual trees\n";}

    void edges() {
        context = "edges";
        using A = RangeAddRangeSum<lng>;
        LazySegmentTree<A> e;
        checkEqual(e.n, 0, "default size");
        checkEqual(tuple(e.prod(0, 0).sum, e.allProd().len), tuple(lng(0), lng(0)), "empty prod");
        checkEqual(e.maxRight(0, [](const A::S &) { return true; }), 0, "empty maxRight");
        checkEqual(e.minLeft(0, [](const A::S &) { return true; }), 0, "empty minLeft");
        checkEqual(int(e.values().size()), 0, "empty values");
        checkEqual(e.fold(0, 0, lng(7), [](lng u, const A::S &s) { return u + s.sum; }), lng(7), "empty fold");
        checkEqual(e.foldRight(0, 0, lng(7), [](const A::S &s, lng u) { return s.sum + u; }), lng(7), "empty foldRight");
        for (int side = 0; side < 2; ++side) {
            LazySegmentTree<A> z(vector<A::S>(4, A::leaf(0)));
            z.apply(2 * side, 2 * side + 2, A::add(5));
            auto y = z;
            checkEqual(z.fold(1, 3, lng(0), [](lng u, const A::S &s) { return u + s.sum; }), lng(5), "fold pushes the " + string(side ? "right" : "left") + " boundary");
            checkEqual(y.foldRight(1, 3, lng(0), [](const A::S &s, lng u) { return s.sum + u; }), lng(5), "foldRight pushes the " + string(side ? "right" : "left") + " boundary");}
        LazySegmentTree<A> t(vector<A::S>(5, A::leaf(1)));
        auto u = t;
        u.apply(0, 5, A::add(10));
        checkEqual(t.allProd().sum, lng(5), "copy independence");
        checkEqual(u.allProd().sum, lng(55), "copy apply");
        auto m = std::move(u);
        checkEqual(m.prod(1, 3).sum, lng(22), "moved prod");
        LazySegmentTree<A> sized(4);
        checkEqual(sized.allProd().len, lng(0), "size ctor holds e");
        std::cout << "PASS edges, copies and empty trees\n";}

    void invalid(const string &name) {
        using A = RangeAddRangeSum<lng>;
        using T = LazySegmentTree<A>;
        T t(vector<A::S>(3, A::leaf(1)));
        DualSegmentTree<A> d(3);
        CommutativeDualSegmentTree<A> c(3);
        auto yes = [](const A::S &) { return true; };
        auto no = [](const A::S &) { return false; };
        auto act = [](lng u, const A::S &s) { return u + s.sum; };
        auto actLeft = [](const A::S &s, lng u) { return s.sum + u; };
        if (name == "negative-size") { T x(-1); }
        else if (name == "oversize") { T x(T::MAX_SIZE + 1); }
        else if (name == "vector-size") { (void)T::checkedSize(size_t(T::MAX_SIZE) + 1); }
        else if (name == "get-end") { (void)t.get(3); }
        else if (name == "set-negative") { t.set(-1, A::leaf(0)); }
        else if (name == "apply-point-end") { t.apply(3, lng(1)); }
        else if (name == "apply-reversed") { t.apply(2, 1, lng(1)); }
        else if (name == "apply-end") { t.apply(0, 4, lng(1)); }
        else if (name == "prod-negative") { (void)t.prod(-1, 2); }
        else if (name == "prod-end") { (void)t.prod(0, 4); }
        else if (name == "max-right-index") { (void)t.maxRight(4, yes); }
        else if (name == "max-right-identity") { (void)t.maxRight(0, no); }
        else if (name == "min-left-index") { (void)t.minLeft(-1, yes); }
        else if (name == "min-left-identity") { (void)t.minLeft(3, no); }
        else if (name == "fold-negative") { (void)t.fold(-1, 2, lng(0), act); }
        else if (name == "fold-reversed") { (void)t.fold(2, 1, lng(0), act); }
        else if (name == "fold-end") { (void)t.fold(0, 4, lng(0), act); }
        else if (name == "fold-right-negative") { (void)t.foldRight(-1, 2, lng(0), actLeft); }
        else if (name == "fold-right-reversed") { (void)t.foldRight(2, 1, lng(0), actLeft); }
        else if (name == "fold-right-end") { (void)t.foldRight(0, 4, lng(0), actLeft); }
        else if (name == "dual-negative-size") { DualSegmentTree<A> x(-1); }
        else if (name == "dual-get-end") { (void)d.get(3); }
        else if (name == "dual-set-end") { d.set(3, 1); }
        else if (name == "dual-apply-reversed") { d.apply(2, 1, 1); }
        else if (name == "comm-negative-size") { CommutativeDualSegmentTree<A> x(-1); }
        else if (name == "comm-get-end") { (void)c.get(3); }
        else if (name == "comm-apply-end") { c.apply(0, 4, 1); }
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
        std::cout << "lazysegmenttree seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        runtimeMod = array<lng, 4>{1000000007, 998244353, 10007, 97}[seed % 4];
        edges();
        preset<AddSum>(rng, "RangeAddRangeSum");
        preset<AssignComposite>(rng, "RangeAssignRangeComposite (noncommutative)");
        preset<SumMinMaxIndex>(rng, "RangeAffineRangeSumMinMaxIndex (index-aware leaves)");
        preset<StatefulAffine>(rng, "stateful struct (affine mod " + show(runtimeMod) + ")");
        foldOps(rng);
        dual(rng);
        std::cout << "PASS lazysegmenttree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL lazysegmenttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
