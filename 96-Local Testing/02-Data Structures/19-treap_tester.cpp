#include "../../02-Data Structures/19-treap.hpp"
#include "../../02-Data Structures/00-monoids.hpp"

namespace {
    ulng seed = 20261013;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &v) { string s = "["; for (auto &x : v) { s += show(x) + ","; } return s + "]"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Treap height by iterative depth-first search.
    template<typename Tr> int height(const Tr &tr, int v) {
        int res = 0;
        vector<pair<int, int>> st;
        if (v) { st.push_back({v, 1}); }
        while (!st.empty()) {
            auto [u, d] = st.back(); st.pop_back();
            res = max(res, d);
            if (tr.t[u].l) { st.push_back({tr.t[u].l, d + 1}); }
            if (tr.t[u].r) { st.push_back({tr.t[u].r, d + 1}); }}
        return res;}

    // Several multisets share one pool; every operation is mirrored on std::multiset references.
    template<typename T>
    void keyRandom(Rng &rng, int steps, auto gen, const string &name) {
        constexpr int K = 4;
        Treap<T> tr(seed ^ 0x51);
        int root[K] = {};
        std::multiset<T> ref[K];
        auto compare = [&](int i) {
            checkEqual(tr.size(root[i]), int(ref[i].size()), "size tree=" + show(i));
            checkEqual(tr.values(root[i]), vector<T>(ref[i].begin(), ref[i].end()), "values tree=" + show(i));};
        for (int s = 0; s < steps; ++s) {
            int i = int(rng() % K), j = int((i + 1 + rng() % (K - 1)) % K), op = int(rng() % 20);
            T x = gen(rng);
            context = name + " step=" + show(s) + " op=" + show(op) + " tree=" + show(i) + " x=" + show(x);
            if (ref[i].size() > 400) { op = 6; }
            if (op < 6) { tr.insert(root[i], x); ref[i].insert(x); }
            else if (op < 9) {
                auto it = ref[i].find(x);
                checkEqual(tr.erase(root[i], x), it != ref[i].end(), "erase");
                if (it != ref[i].end()) { ref[i].erase(it); }}
            else if (op == 9) {
                auto [a, b] = tr.split(root[i], x);
                vector<T> lo(ref[i].begin(), ref[i].lower_bound(x)), hi(ref[i].lower_bound(x), ref[i].end());
                checkEqual(tr.values(a), lo, "split left"); checkEqual(tr.values(b), hi, "split right");
                root[i] = tr.merge(a, b);}
            else if (op == 10) {
                root[i] = tr.unite(root[i], root[j]); root[j] = 0;
                ref[i].insert(ref[j].begin(), ref[j].end()); ref[j].clear();
                compare(i);}
            else if (op == 11) {
                vector<T> a(rnd(rng, 0, 40));
                for (auto &y : a) { y = gen(rng); }
                sort(a.begin(), a.end());
                root[j] = tr.build(a); ref[j] = std::multiset<T>(a.begin(), a.end());
                compare(j);}
            else if (op == 12) {
                // Swapping the parts below x between two trees merges ordered halves from different trees.
                auto [ai, bi] = tr.split(root[i], x);
                auto [aj, bj] = tr.split(root[j], x);
                root[i] = tr.merge(aj, bi); root[j] = tr.merge(ai, bj);
                std::multiset<T> ni(ref[j].begin(), ref[j].lower_bound(x)), nj(ref[i].begin(), ref[i].lower_bound(x));
                ni.insert(ref[i].lower_bound(x), ref[i].end()); nj.insert(ref[j].lower_bound(x), ref[j].end());
                ref[i] = ni; ref[j] = nj;
                compare(i); compare(j);}
            else {
                const auto &r = ref[i];
                int f = tr.find(root[i], x);
                checkEqual(f != 0, r.count(x) > 0, "find");
                if (f) { checkEqual(tr.t[f].key, x, "find key"); }
                checkEqual(tr.rank(root[i], x), int(std::distance(r.begin(), r.lower_bound(x))), "rank");
                checkEqual(tr.upperRank(root[i], x), int(std::distance(r.begin(), r.upper_bound(x))), "upperRank");
                int lb = tr.lowerBound(root[i], x), ub = tr.upperBound(root[i], x);
                checkEqual(lb != 0, r.lower_bound(x) != r.end(), "lowerBound found");
                if (lb) { checkEqual(tr.t[lb].key, *r.lower_bound(x), "lowerBound key"); }
                checkEqual(ub != 0, r.upper_bound(x) != r.end(), "upperBound found");
                if (ub) { checkEqual(tr.t[ub].key, *r.upper_bound(x), "upperBound key"); }
                if (!r.empty()) {
                    int k = int(rnd(rng, 0, lng(r.size()) - 1));
                    checkEqual(tr.kth(root[i], k), *std::next(r.begin(), k), "kth k=" + show(k));}}
            if (s % 16 == 0) { compare(i); }}
        for (int i = 0; i < K; ++i) { compare(i); }}

    void keyEdges(Rng &rng) {
        context = "key edges";
        Treap<lng> tr(7);
        int v = 0;
        checkEqual(tr.size(v), 0, "empty size"); checkEqual(tr.find(v, 1), 0, "empty find"); checkEqual(tr.rank(v, 1), 0, "empty rank");
        checkEqual(tr.lowerBound(v, 1), 0, "empty lowerBound"); checkEqual(tr.erase(v, 1), false, "empty erase");
        checkEqual(tr.build({}), 0, "empty build"); checkEqual(tr.values(0), vector<lng>{}, "empty values");
        auto [a, b] = tr.split(0, 5);
        checkEqual(a + b, 0, "empty split"); checkEqual(tr.merge(0, 0), 0, "empty merge"); checkEqual(tr.unite(0, 0), 0, "empty unite");
        lng lo = std::numeric_limits<lng>::min(), hi = std::numeric_limits<lng>::max();
        v = tr.build({lo, lo, 0, hi, hi});
        checkEqual(tr.rank(v, lo), 0, "rank min"); checkEqual(tr.upperRank(v, hi), 5, "upperRank max");
        checkEqual(tr.upperBound(v, hi), 0, "upperBound max"); checkEqual(tr.kth(v, 4), hi, "kth max");
        checkEqual(tr.erase(v, lo) && tr.erase(v, lo) && !tr.erase(v, lo), true, "erase duplicates");
        int w = tr.build({1, 2, 3});
        int both = tr.merge(tr.build({-5, 0}), w);
        checkEqual(tr.values(both), vector<lng>{-5, 0, 1, 2, 3}, "merge built trees");
        Treap<lng> copy = tr;
        copy.insert(both, 9);
        checkEqual(tr.values(both), vector<lng>{-5, 0, 1, 2, 3}, "copy independence");
        // Same seed and operations give the same pool; a different seed changes priorities.
        Treap<lng> x(123), y(123), z(124);
        int rx = 0, ry = 0, rz = 0;
        for (int i = 0; i < 200; ++i) { lng k = rnd(rng, 0, 50); x.insert(rx, k); y.insert(ry, k); z.insert(rz, k); }
        bool same = rx == ry, differ = false;
        for (int i = 0; i < int(x.t.size()); ++i) {
            same = same && x.t[i].pri == y.t[i].pri && x.t[i].l == y.t[i].l && x.t[i].r == y.t[i].r;
            differ = differ || x.t[i].pri != z.t[i].pri;}
        checkEqual(same, true, "seed reproducible"); checkEqual(differ, true, "seed changes priorities");
        // Keys that alias the pool while it grows (review regression).
        Treap<lng> al(3);
        int ar = al.build({1, 5, 9});
        std::multiset<lng> aref = {1, 5, 9};
        for (int i = 0; i < 300; ++i) {
            int k = int(rnd(rng, 0, al.size(ar) - 1));
            aref.insert(al.kth(ar, k)); al.insert(ar, al.kth(ar, k));
            int f = al.lowerBound(ar, 5);
            if (i % 3 == 0 && f) { aref.erase(aref.find(al.t[f].key)); al.erase(ar, al.t[f].key); }}
        checkEqual(al.values(ar), vector<lng>(aref.begin(), aref.end()), "aliasing keys");
        // Sorted insertions and small-to-large unions keep the expected logarithmic height.
        int n = mode == "quick" ? 20000 : 200000;
        Treap<lng> big(99);
        int r = 0;
        for (int i = 0; i < n; ++i) { big.insert(r, i); }
        int bound = 4 * int(std::bit_width(uint(n))) + 20;
        checkEqual(height(big, r) <= bound, true, "height sorted inserts n=" + show(n));
        int u = 0;
        for (int i = 0; i < n / 64; ++i) { u = big.unite(u, big.build({lng(i), lng(i) + n, lng(i) + 2 * n})); }
        checkEqual(big.size(u), 3 * (n / 64), "unite size");
        checkEqual(height(big, u) <= bound, true, "height after unions");}

    struct Mod {
        lng v;
        Mod(lng x = 0) : v((x % P + P) % P) {}
        Mod operator+(const Mod &o) const { return Mod(v + o.v); }
        Mod operator*(const Mod &o) const { return Mod(v * o.v % P); }
        bool operator==(const Mod &o) const { return v == o.v; }
    };
    std::ostream &operator<<(std::ostream &os, const Mod &m) { return os << m.v; }

    // Composite of affine maps in both directions; reverse swaps them, so prod stays exact under reversal.
    struct RevComposite {
        using B = RangeAssignRangeComposite<Mod>;
        struct S { B::S f, g; };
        using F = B::F;
        static S op(const S &x, const S &y) { return {B::op(x.f, y.f), B::op(y.g, x.g)}; }
        static S e() { return {B::e(), B::e()}; }
        static S mapping(const F &h, const S &x) { return {B::mapping(h, x.f), B::mapping(h, x.g)}; }
        static F composition(const F &a, const F &b) { return B::composition(a, b); }
        static F id() { return B::id(); }
        static S reverse(const S &x) { return {x.g, x.f}; }
        static S leaf(Mod a, Mod b) { return {B::leaf(a, b), B::leaf(a, b)}; }
        static string key(const S &x) { return show(x.f.a) + "x+" + show(x.f.b) + "/" + show(x.g.a) + "x+" + show(x.g.b) + "/" + show(x.f.len); }
        static S value(Rng &rng) { return leaf(rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)); }
        static F action(Rng &rng) { return {rng() % 4 != 0, rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
    };
    // Strings in both directions; the action upper- or lower-cases every letter.
    struct RevString {
        struct S { string f, g; };
        using F = int;
        static S op(const S &x, const S &y) { return {x.f + y.f, y.g + x.g}; }
        static S e() { return {"", ""}; }
        static string cased(string s, F h) { for (char &c : s) { c = char(h == 1 ? std::toupper(c) : std::tolower(c)); } return s; }
        static S mapping(const F &h, const S &x) { return h ? S{cased(x.f, h), cased(x.g, h)} : x; }
        static F composition(const F &a, const F &b) { return a ? a : b; }
        static F id() { return 0; }
        static S reverse(const S &x) { return {x.g, x.f}; }
        static string key(const S &x) { return x.f + "/" + x.g; }
        static S value(Rng &rng) { string c(1, char('a' + rnd(rng, 0, 2))); return {c, c}; }
        static F action(Rng &rng) { return int(rnd(rng, 0, 2)); }
    };
    // Commutative products need no reverse hook.
    struct AffineSum : RangeAffineRangeSum<Mod> {
        static string key(const S &x) { return show(x.sum) + "/" + show(x.len); }
        static S value(Rng &rng) { return {Mod(rnd(rng, 0, P - 1)), Mod(1)}; }
        static F action(Rng &rng) { return {Mod(rnd(rng, 0, P - 1)), Mod(rnd(rng, 0, P - 1))}; }
    };
    // Range add with range max: monotone predicates for maxRight and minLeft.
    struct AddMax : RangeAddRangeMax<lng> {
        static string key(const S &x) { return show(x); }
        static S value(Rng &rng) { return rnd(rng, 0, 50); }
        static F action(Rng &rng) { return rnd(rng, -3, 5); }
    };

    // The library's Reversible adapter over the composite preset (non-static op, e and mapping).
    struct AdapterComposite : Reversible<RangeAssignRangeComposite<Mod>> {
        using B = RangeAssignRangeComposite<Mod>;
        static string key(const S &x) { return show(x.x.a) + "x+" + show(x.x.b) + "/" + show(x.y.a) + "x+" + show(x.y.b) + "/" + show(x.x.len); }
        static S value(Rng &rng) { return leaf(B::leaf(rnd(rng, 0, P - 1), rnd(rng, 0, P - 1))); }
        static F action(Rng &rng) { return rng() % 4 ? B::assign(rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)) : B::id(); }
    };
    // Runtime state: affine sums modulo a member modulus; generators read the round's modulus.
    lng treapMod = 7;
    struct ModAffine {
        lng mod;
        struct S { lng sum, len; };
        struct F { lng a, b; };
        S op(const S &x, const S &y) const { return {(x.sum + y.sum) % mod, x.len + y.len}; }
        S e() const { return {0, 0}; }
        S mapping(const F &f, const S &x) const { return {lng((lll(f.a) * x.sum + lll(f.b) * x.len) % mod), x.len}; }
        F composition(const F &f, const F &g) const { return {lng(lll(f.a) * g.a % mod), lng((lll(f.a) * g.b + f.b) % mod)}; }
        F id() const { return {1, 0}; }
        static string key(const S &x) { return show(x.sum) + "/" + show(x.len); }
        static S value(Rng &rng) { return {rnd(rng, 0, treapMod - 1), 1}; }
        static F action(Rng &rng) { return {rnd(rng, 0, treapMod - 1), rnd(rng, 0, treapMod - 1)}; }
    };

    template<typename A>
    typename A::S fold(const vector<typename A::S> &a, int l, int r, const A &m) {
        typename A::S res = m.e();
        for (int i = l; i < r; ++i) { res = m.op(res, a[i]); }
        return res;}
    template<typename A>
    vector<string> keys(const vector<typename A::S> &a) { vector<string> res; for (auto &x : a) { res.push_back(A::key(x)); } return res; }

    // Three sequences share one pool and mirror vectors; every operation, including splits that move pieces between sequences.
    template<typename A>
    void implicitRandom(Rng &rng, int steps, const string &name, auto pred, A m = {}) {
        using S = typename A::S;
        constexpr int K = 3;
        ImplicitTreap<A> tr(seed ^ 0x77, m);
        int root[K] = {};
        vector<S> ref[K];
        auto compare = [&](int i) {
            checkEqual(tr.size(root[i]), int(ref[i].size()), "size seq=" + show(i));
            checkEqual(keys<A>(tr.values(root[i])), keys<A>(ref[i]), "values seq=" + show(i));};
        for (int s = 0; s < steps; ++s) {
            int i = int(rng() % K), j = int((i + 1 + rng() % (K - 1)) % K), n = int(ref[i].size()), op = int(rng() % 16);
            context = name + " step=" + show(s) + " op=" + show(op) + " seq=" + show(i) + " n=" + show(n);
            if (n > 300) { op = 2; }
            if (op < 2 || n == 0) {
                int p = int(rnd(rng, 0, n));
                S x = A::value(rng);
                tr.insert(root[i], p, x); ref[i].insert(ref[i].begin() + p, x);}
            else if (op == 2) { int p = int(rnd(rng, 0, n - 1)); tr.erase(root[i], p); ref[i].erase(ref[i].begin() + p); }
            else if (op == 3) { int p = int(rnd(rng, 0, n - 1)); checkEqual(A::key(tr.at(root[i], p)), A::key(ref[i][p]), "at p=" + show(p)); }
            else if (op == 4) { int p = int(rnd(rng, 0, n - 1)); S x = A::value(rng); tr.set(root[i], p, x); ref[i][p] = x; }
            else if (op < 8) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, l, n));
                if (op == 5) { tr.reverse(root[i], l, r); std::reverse(ref[i].begin() + l, ref[i].begin() + r); }
                else if (op == 6) {
                    auto f = A::action(rng);
                    tr.apply(root[i], l, r, f);
                    for (int k = l; k < r; ++k) { ref[i][k] = m.mapping(f, ref[i][k]); }}
                else { checkEqual(A::key(tr.prod(root[i], l, r)), A::key(fold<A>(ref[i], l, r, m)), "prod [" + show(l) + "," + show(r) + ")"); }}
            else if (op == 8) {
                // Rotation: split at k and merge the parts in swapped order.
                int k = int(rnd(rng, 0, n));
                auto [a, b] = tr.split(root[i], k);
                root[i] = tr.merge(b, a);
                std::rotate(ref[i].begin(), ref[i].begin() + k, ref[i].end());}
            else if (op == 9) {
                // Moves a prefix of sequence i to the end of sequence j.
                int k = int(rnd(rng, 0, n));
                auto [a, b] = tr.split(root[i], k);
                root[j] = tr.merge(root[j], a); root[i] = b;
                ref[j].insert(ref[j].end(), ref[i].begin(), ref[i].begin() + k); ref[i].erase(ref[i].begin(), ref[i].begin() + k);
                compare(j);}
            else if (op == 10) {
                vector<S> a(rnd(rng, 0, 30));
                for (auto &x : a) { x = A::value(rng); }
                root[j] = tr.build(a); ref[j] = a;
                compare(j);}
            else if (op < 13) {
                auto [g, t] = pred(rng);
                auto ok = [&](const S &x) { return g(x, t); };
                int l = int(rnd(rng, 0, n)), want = l;
                S acc = m.e();
                while (want < n && ok(m.op(acc, ref[i][want]))) { acc = m.op(acc, ref[i][want++]); }
                checkEqual(tr.maxRight(root[i], l, ok), want, "maxRight l=" + show(l));
                int r = int(rnd(rng, 0, n));
                want = r; acc = m.e();
                while (want > 0 && ok(m.op(ref[i][want - 1], acc))) { acc = m.op(ref[i][--want], acc); }
                checkEqual(tr.minLeft(root[i], r, ok), want, "minLeft r=" + show(r));}
            if (s % 8 == 0) { compare(i); }}
        for (int i = 0; i < K; ++i) { compare(i); }}

    void implicitEdges() {
        context = "implicit edges";
        // Same seed and operations give the same pool; a different seed changes priorities.
        ImplicitTreap<AddMax> p1(77), p2(77), p3(78);
        int r1 = 0, r2 = 0, r3 = 0;
        for (int i = 0; i < 200; ++i) { p1.insert(r1, i / 2, i); p2.insert(r2, i / 2, i); p3.insert(r3, i / 2, i); }
        bool same = r1 == r2, differ = false;
        for (int i = 0; i < int(p1.t.size()); ++i) {
            same = same && p1.t[i].pri == p2.t[i].pri && p1.t[i].l == p2.t[i].l && p1.t[i].r == p2.t[i].r;
            differ = differ || p1.t[i].pri != p3.t[i].pri;}
        checkEqual(same, true, "implicit seed reproducible"); checkEqual(differ, true, "implicit seed changes priorities");
        checkEqual(p1.values(r1), p3.values(r3), "seed does not change values");
        // Values that alias the pool while it grows (review regression).
        for (int i = 0; i < 300; ++i) { p1.insert(r1, 0, p1.t[r1].val); p3.insert(r3, 0, p3.at(r3, 0)); }
        checkEqual(p1.size(r1), 500, "aliasing insert size"); checkEqual(p3.at(r3, 0), p3.at(r3, 299), "aliasing insert value");
        ImplicitTreap<RevString> tr(5);
        int v = 0;
        checkEqual(tr.size(v), 0, "empty size"); checkEqual(tr.prod(v, 0, 0).f, string(), "empty prod");
        checkEqual(tr.maxRight(v, 0, [](const RevString::S &) { return true; }), 0, "empty maxRight");
        checkEqual(tr.build({}), 0, "empty build");
        v = tr.build({{"a", "a"}, {"b", "b"}, {"c", "c"}, {"d", "d"}});
        tr.reverse(v, 0, 4); tr.reverse(v, 1, 3); tr.apply(v, 0, 2, 1);
        checkEqual(tr.prod(v, 0, 4).f, string("DBca"), "reverse twice and apply");
        checkEqual(tr.prod(v, 0, 4).g, string("acBD"), "reversed product");
        tr.reverse(v, 2, 2); tr.apply(v, 3, 3, 2);
        checkEqual(tr.prod(v, 0, 4).f, string("DBca"), "empty-range updates");
        ImplicitTreap<RevString> copy = tr;
        copy.erase(v, 0);
        checkEqual(tr.prod(v, 0, 4).f, string("DBca"), "copy independence");
        checkEqual(tr.maxRight(v, 4, [](const RevString::S &) { return true; }), 4, "maxRight at end");
        checkEqual(tr.minLeft(v, 0, [](const RevString::S &) { return true; }), 0, "minLeft at start");}

    void invalid(const string &name) {
        Treap<lng> tr;
        int v = tr.build({1, 2, 3});
        ImplicitTreap<AddMax> it;
        int u = it.build({4, 5, 6});
        auto yes = [](lng) { return true; };
        if (name == "treap-kth-range") { (void)tr.kth(v, 3); }
        else if (name == "treap-kth-negative") { (void)tr.kth(v, -1); }
        else if (name == "treap-kth-empty") { (void)tr.kth(0, 0); }
        else if (name == "treap-merge-order") { (void)tr.merge(v, tr.build({0, 5})); }
        else if (name == "treap-build-unsorted") { (void)tr.build({2, 1}); }
        else if (name == "implicit-insert-range") { it.insert(u, 4, 1); }
        else if (name == "implicit-erase-range") { it.erase(u, 3); }
        else if (name == "implicit-at-range") { (void)it.at(u, 3); }
        else if (name == "implicit-at-negative") { (void)it.at(u, -1); }
        else if (name == "implicit-set-range") { it.set(u, 3, 1); }
        else if (name == "implicit-split-range") { (void)it.split(u, 4); }
        else if (name == "implicit-split-negative") { (void)it.split(u, -1); }
        else if (name == "implicit-reverse-reversed") { it.reverse(u, 2, 1); }
        else if (name == "implicit-apply-range") { it.apply(u, 0, 4, 1); }
        else if (name == "implicit-prod-range") { (void)it.prod(u, -1, 2); }
        else if (name == "implicit-maxright-identity") { (void)it.maxRight(u, 0, [](lng) { return false; }); }
        else if (name == "implicit-maxright-range") { (void)it.maxRight(u, 4, yes); }
        else if (name == "implicit-minleft-range") { (void)it.minLeft(u, 4, yes); }
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
        std::cout << "treap seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        int steps = mode == "quick" ? 3000 : mode == "full" ? 100000 : 600000;
        keyEdges(rng);
        implicitEdges();
        keyRandom<lng>(rng, steps, [](Rng &g) { return rnd(g, -30, 30); }, "key small");
        keyRandom<lng>(rng, steps / 2, [](Rng &g) { return lng(g()); }, "key wide");
        keyRandom<string>(rng, steps / 4, [](Rng &g) { return string(1, char('a' + rnd(g, 0, 5))) + (g() % 2 ? "" : "q"); }, "key string");
        auto lenPred = [](Rng &g) { return pair{[](const RevComposite::S &x, lng t) { return x.f.len <= t; }, rnd(g, 0, 40)}; };
        auto aPred = [](Rng &g) { return pair{[](const RevString::S &x, lng t) { return std::count(x.f.begin(), x.f.end(), 'a') + std::count(x.f.begin(), x.f.end(), 'A') <= t; }, rnd(g, 0, 12)}; };
        auto sumPred = [](Rng &g) { return pair{[](const AffineSum::S &x, lng t) { return x.len.v <= t; }, rnd(g, 0, 40)}; };
        auto maxPred = [](Rng &g) { return pair{[](lng x, lng t) { return x <= t; }, rnd(g, 0, 60)}; };
        implicitRandom<RevComposite>(rng, steps, "revcomposite", lenPred);
        implicitRandom<RevString>(rng, steps / 2, "revstring", aPred);
        implicitRandom<AffineSum>(rng, steps, "affinesum", sumPred);
        implicitRandom<AddMax>(rng, steps, "addmax", maxPred);
        auto adapterPred = [](Rng &g) { return pair{[](const AdapterComposite::S &x, lng t) { return x.x.len <= t; }, rnd(g, 0, 40)}; };
        implicitRandom<AdapterComposite>(rng, steps, "Reversible adapter composite", adapterPred);
        auto modPred = [](Rng &g) { return pair{[](const ModAffine::S &x, lng t) { return x.len <= t; }, rnd(g, 0, 40)}; };
        for (lng mod : {lng(7), lng(998244353)}) {
            treapMod = mod;
            implicitRandom<ModAffine>(rng, steps / 2, "stateful affine mod " + show(mod), modPred, ModAffine{mod});}
        std::cout << "PASS treap checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL treap seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
