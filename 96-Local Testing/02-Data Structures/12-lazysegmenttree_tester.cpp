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

    struct AddSum {
        using A = RangeAddRangeSum<lng>; using V = lng; using F = lng;
        static V value(Rng &rng) { return rnd(rng, -5, 5); }
        static F action(Rng &rng, int, int) { return rnd(rng, -3, 3); }
        static void act(V &v, const F &f, int) { v += f; }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.sum, s.len); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s += a[i]; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    template<bool MAX> struct AddMinMax {
        using A = std::conditional_t<MAX, RangeAddRangeMax<lng>, RangeAddRangeMin<lng>>; using V = lng; using F = lng;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static F action(Rng &rng, int, int) { return rnd(rng, -4, 4); }
        static void act(V &v, const F &f, int) { v += f; }
        static A::S leaf(int, V v) { return v; }
        static auto key(const A::S &s) { return tuple(s); }
        static auto expect(const vector<V> &a, int l, int r) {
            lng s = MAX ? std::numeric_limits<lng>::lowest() : std::numeric_limits<lng>::max();
            for (int i = l; i < r; ++i) { s = MAX ? max(s, a[i]) : min(s, a[i]); }
            return tuple(s);}
        static bool ok(const auto &k, lng t) { return MAX ? std::get<0>(k) <= t : std::get<0>(k) >= t; }
    };
    struct Argmin {
        using A = RangeAddRangeArgmin<lng>; using V = lng; using F = lng;
        static V value(Rng &rng) { return rnd(rng, -3, 3); }
        static F action(Rng &rng, int, int) { return rnd(rng, -2, 2); }
        static void act(V &v, const F &f, int) { v += f; }
        static A::S leaf(int i, V v) { return A::leaf(i, v); }
        static auto key(const A::S &s) { return s.i < 0 ? tuple(lng(0), lng(-1)) : tuple(s.x, s.i); }
        static auto expect(const vector<V> &a, int l, int r) {
            tuple res(lng(0), lng(-1));
            for (int i = l; i < r; ++i) {
                if (std::get<1>(res) < 0 || a[i] < std::get<0>(res)) { res = tuple(a[i], lng(i)); }}
            return res;}
        static bool ok(const auto &k, lng t) { return std::get<1>(k) < 0 || std::get<0>(k) >= t; }
    };
    struct MinCount {
        using A = RangeAddRangeMinCount<lng>; using V = lng; using F = lng;
        static V value(Rng &rng) { return rnd(rng, -2, 2); }
        static F action(Rng &rng, int, int) { return rnd(rng, -2, 2); }
        static void act(V &v, const F &f, int) { v += f; }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.cnt ? s.x : 0, s.cnt); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0)); }
            lng m = *std::min_element(a.begin() + l, a.begin() + r);
            return tuple(m, lng(std::count(a.begin() + l, a.begin() + r, m)));}
        static bool ok(const auto &k, lng t) { return std::get<1>(k) == 0 || std::get<0>(k) >= t; }
    };
    struct AffineSum {
        using A = RangeAffineRangeSum<mint>; using V = lng; using F = A::F;
        static V value(Rng &rng) { return rnd(rng, 0, P - 1); }
        static F action(Rng &rng, int, int) { return {mint(rnd(rng, 0, P - 1)), mint(rnd(rng, 0, P - 1))}; }
        static void act(V &v, const F &f, int) { v = (lng(f.a.val()) * v + lng(f.b.val())) % P; }
        static A::S leaf(int, V v) { return A::leaf(mint(v)); }
        static auto key(const A::S &s) { return tuple(lng(s.sum.val()), lng(s.len.val())); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s = (s + a[i]) % P; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    struct AssignSum {
        using A = RangeAssignRangeSum<lng>; using V = lng; using F = A::F;
        static V value(Rng &rng) { return rnd(rng, 0, 6); }
        static F action(Rng &rng, int, int) { return {rng() % 4 != 0, rnd(rng, 0, 6)}; }
        static void act(V &v, const F &f, int) { if (f.set) { v = f.x; }}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.sum, s.len); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s += a[i]; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<0>(k) <= 2 * t; }
    };
    struct SetComposite {
        using A = RangeSetRangeComposite<mint>; using V = pair<lng, lng>; using F = A::F;
        static V value(Rng &rng) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
        static F action(Rng &rng, int, int) { return {rng() % 4 != 0, mint(rnd(rng, 0, P - 1)), mint(rnd(rng, 0, P - 1))}; }
        static void act(V &v, const F &f, int) { if (f.set) { v = {lng(f.a.val()), lng(f.b.val())}; }}
        static A::S leaf(int, V v) { return A::leaf(mint(v.first), mint(v.second)); }
        static auto key(const A::S &s) { return tuple(lng(s.a.val()), lng(s.b.val()), s.len); }
        static auto expect(const vector<V> &a, int l, int r) {
            lng ca = 1, cb = 0;
            for (int i = l; i < r; ++i) { ca = a[i].first * ca % P; cb = (a[i].first * cb + a[i].second) % P; }
            return tuple(ca, cb, lng(r - l));}
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
    };
    struct Arithmetic {
        using A = RangeArithmeticAddRangeSum<lng>; using V = lng; using F = pair<lng, lng>;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static F action(Rng &rng, int, int) { return {rnd(rng, -5, 5), rnd(rng, -3, 3)}; }
        static A::S leaf(int i, V v) { return A::leaf(i, v); }
        static auto key(const A::S &s) { return tuple(s.sum, s.len, s.idx); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0, x = 0; for (int i = l; i < r; ++i) { s += a[i]; x += i; } return tuple(s, lng(r - l), x); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    struct MinMaxArg {
        using A = RangeAffineRangeMinMaxArg<lng>; using V = lng; using F = A::F;
        static V value(Rng &rng) { return rnd(rng, -4, 4); }
        static F action(Rng &rng, int, int) { return {rnd(rng, -1, 1), rnd(rng, -3, 3)}; }
        static void act(V &v, const F &f, int) { v = f.a * v + f.b; }
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
    struct Bitwise {
        using A = RangeBitwiseRangeAndOrXor<ulng>; using V = ulng; using F = A::F;
        static V value(Rng &rng) { return rng() % 3 ? rng() % 16 : rng(); }
        static F action(Rng &rng, int, int) {
            ulng x = rng() % 3 ? rng() % 16 : rng();
            switch (rng() % 4) {
                case 0: return A::andWith(x);
                case 1: return A::orWith(x);
                case 2: return A::xorWith(x);
                default: return A::assign(x);}}
        static void act(V &v, const F &f, int) { v = (v & f.a) ^ f.b; }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.band, s.bor, s.bxor, s.len) : tuple(~ulng(0), ulng(0), ulng(0), lng(0)); }
        static auto expect(const vector<V> &a, int l, int r) {
            ulng x = ~ulng(0), y = 0, z = 0;
            for (int i = l; i < r; ++i) { x &= a[i]; y |= a[i]; z ^= a[i]; }
            return tuple(x, y, z, lng(r - l));}
        static bool ok(const auto &k, lng t) { return (std::get<0>(k) & ulng(t & 15)) == ulng(t & 15); }
    };
    template<bool LCM> struct GcdLcm {
        using A = RangeGcdLcm<lng, LCM>; using V = lng; using F = A::F;
        static V value(Rng &rng) { return rnd(rng, 0, 12); }
        static F action(Rng &rng, int, int) {
            lng x = rnd(rng, 0, 12);
            switch (rng() % 3) {
                case 0: return A::gcdWith(x);
                case 1: return A::lcmWith(x);
                default: return A::assign(x);}}
        static void act(V &v, const F &f, int) { v = lcm(gcd(v, f.a), f.b); }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.g, s.l, s.len) : tuple(lng(0), lng(0), lng(0)); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0), lng(0)); }
            lng g = 0, m = 1;
            for (int i = l; i < r; ++i) { g = gcd(g, a[i]); m = lcm(m, a[i]); }
            return tuple(g, LCM ? m : 0, lng(r - l));}
        static bool ok(const auto &k, lng t) { return std::get<0>(k) % max<lng>(1, t % 7) == 0; }
    };
    struct MaxSubarray {
        using A = RangeAssignMaxSubarray<lng>; using V = lng; using F = A::F;
        static V value(Rng &rng) { return rnd(rng, -6, 6); }
        static F action(Rng &rng, int, int) { return {rng() % 4 != 0, rnd(rng, -6, 6)}; }
        static void act(V &v, const F &f, int) { if (f.set) { v = f.x; }}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.sum, s.pre, s.suf, s.best, s.len) : tuple(lng(0), lng(0), lng(0), lng(0), lng(0)); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0), lng(0), lng(0), lng(0)); }
            lng sum = 0, pre = a[l], suf = a[r - 1], best = a[l];
            for (int i = l; i < r; ++i) {
                sum += a[i]; pre = max(pre, sum);
                lng s = 0;
                for (int j = i; j < r; ++j) { s += a[j]; best = max(best, s); }}
            for (lng i = r - 1, s = 0; i >= l; --i) { s += a[i]; suf = max(suf, s); }
            return tuple(sum, pre, suf, best, lng(r - l));}
        static bool ok(const auto &k, lng t) { return std::get<4>(k) == 0 || std::get<3>(k) <= t; }
    };

    template<typename C>
    void actRange(vector<typename C::V> &a, Rng &rng, int l, int r, typename C::A::F &f) {
        if constexpr (std::is_same_v<C, Arithmetic>) {
            auto [first, d] = C::action(rng, l, r);
            f = C::A::progression(l, first, d);
            for (int i = l; i < r; ++i) { a[i] += first + d * (i - l); }}
        else {
            f = C::action(rng, l, r);
            for (int i = l; i < r; ++i) { C::act(a[i], f, i); }}}

    template<typename C>
    void history(Rng &rng, const string &name, int n, int steps) {
        using A = typename C::A;
        using S = typename A::S;
        vector<typename C::V> a(n);
        vector<S> leaves;
        for (int i = 0; i < n; ++i) { a[i] = C::value(rng); leaves.push_back(C::leaf(i, a[i])); }
        LazySegmentTree<A> t(leaves);
        context = name + " n=" + show(n) + " seed=" + show(seed);
        bool all = n <= 17;
        auto verify = [&](const string &when) {
            if (all) {
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
                    if (n) {
                        typename A::F f;
                        actRange<C>(a, rng, p, p + 1, f);
                        t.apply(p, f);}
                    break;
                case 3: case 4: {
                    typename A::F f;
                    actRange<C>(a, rng, l, r, f);
                    t.apply(l, r, f);
                    break;}
                case 5:
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
        int steps = mode == "quick" ? 60 : mode == "full" ? 300 : 800;
        for (int n : sizes()) { history<C>(rng, name, n, n > 17 ? 4 * steps : steps); }
        std::cout << "PASS lazy " << name << '\n';}

    void dual(Rng &rng) {
        using A = RangeAffineRangeSum<mint>;
        for (int n : sizes()) {
            context = "DualSegmentTree affine n=" + show(n);
            vector<pair<lng, lng>> a(n);
            vector<A::F> init(n);
            for (int i = 0; i < n; ++i) { a[i] = {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; init[i] = {mint(a[i].first), mint(a[i].second)}; }
            DualSegmentTree<A> t(init);
            DualSegmentTree<A> z(n);
            for (int i = 0; i < n; ++i) { checkEqual(tuple(z.get(i).a.val(), z.get(i).b.val()), tuple(1u, 0u), "size ctor identity"); }
            int steps = mode == "quick" ? 100 : mode == "full" ? 1000 : 5000;
            for (int step = 0; step < steps; ++step) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                if (l > r) { swap(l, r); }
                lng fa = rnd(rng, 0, P - 1), fb = rnd(rng, 0, P - 1);
                switch (rng() % 4) {
                    case 0: case 1:
                        t.apply(l, r, {mint(fa), mint(fb)});
                        for (int i = l; i < r; ++i) { a[i] = {fa * a[i].first % P, (fa * a[i].second + fb) % P}; }
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
        LazySegmentTree<A> t(vector<A::S>(5, A::leaf(1)));
        auto u = t;
        u.apply(0, 5, 10);
        checkEqual(t.allProd().sum, lng(5), "copy independence");
        checkEqual(u.allProd().sum, lng(55), "copy apply");
        auto m = std::move(u);
        checkEqual(m.prod(1, 3).sum, lng(22), "moved prod");
        LazySegmentTree<A> sized(4);
        checkEqual(sized.allProd().len, lng(0), "size ctor holds e");
        LazySegmentTree<RangeSetRangeComposite<mint>> big(vector<RangeSetRangeComposite<mint>::S>(1000, RangeSetRangeComposite<mint>::leaf(1, 0)));
        big.apply(0, 1000, {true, mint(3), mint(1)});
        lng a = 1, b = 0;
        for (int i = 0; i < 1000; ++i) { a = 3 * a % P; b = (3 * b + 1) % P; }
        checkEqual(tuple(lng(big.allProd().a.val()), lng(big.allProd().b.val())), tuple(a, b), "composite power 1000");
        std::cout << "PASS edges, copies and empty trees\n";}

    void invalid(const string &name) {
        using A = RangeAddRangeSum<lng>;
        using T = LazySegmentTree<A>;
        T t(vector<A::S>(3, A::leaf(1)));
        DualSegmentTree<A> d(3);
        CommutativeDualSegmentTree<A> c(3);
        auto yes = [](const A::S &) { return true; };
        auto no = [](const A::S &) { return false; };
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
        edges();
        preset<AddSum>(rng, "RangeAddRangeSum");
        preset<AddMinMax<false>>(rng, "RangeAddRangeMin");
        preset<AddMinMax<true>>(rng, "RangeAddRangeMax");
        preset<Argmin>(rng, "RangeAddRangeArgmin");
        preset<MinCount>(rng, "RangeAddRangeMinCount");
        preset<AffineSum>(rng, "RangeAffineRangeSum");
        preset<AssignSum>(rng, "RangeAssignRangeSum");
        preset<SetComposite>(rng, "RangeSetRangeComposite");
        preset<Arithmetic>(rng, "RangeArithmeticAddRangeSum");
        preset<MinMaxArg>(rng, "RangeAffineRangeMinMaxArg");
        preset<Bitwise>(rng, "RangeBitwiseRangeAndOrXor");
        preset<GcdLcm<false>>(rng, "RangeGcdLcm<gcd>");
        preset<GcdLcm<true>>(rng, "RangeGcdLcm<gcd,lcm>");
        preset<MaxSubarray>(rng, "RangeAssignMaxSubarray");
        dual(rng);
        std::cout << "PASS lazysegmenttree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL lazysegmenttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
