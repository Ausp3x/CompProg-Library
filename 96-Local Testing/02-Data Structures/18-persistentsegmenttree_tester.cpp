#include "../../02-Data Structures/18-persistentsegmenttree.hpp"
#include "../../02-Data Structures/00-monoids.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    ulng seed = 20261027;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng P = 998244353, INF = std::numeric_limits<lng>::max(), NEG = std::numeric_limits<lng>::min();
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) {
        ulng span = ulng(hi) - ulng(lo) + 1;
        return lng(ulng(lo) + (span ? rng() % span : rng()));}
    int pick(Rng &rng, int lo, int hi) { return int(rnd(rng, lo, hi)); }
    int scale(int quick, int full, int stress) { return mode == "quick" ? quick : mode == "full" ? full : stress; }
    template<typename T> string show(const T &x) {
        if constexpr (std::is_integral_v<T>) { return std::to_string(x); }
        std::ostringstream s;
        s << x;
        return s.str();}
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename... T> string show(const tuple<T...> &t) {
        string s = "(";
        std::apply([&](const auto &...x) { int k = 0; ((s += (k++ ? "," : "") + show(x)), ...); }, t);
        return s + ")";}
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (int i = 0; i < int(v.size()); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    // op is the operation text or, in hot loops, a lambda building it only on failure.
    template<typename T, typename U, typename M> void checkEqual(const T &got, const U &want, const M &op) {
        ++checks;
        if (!(got == want)) {
            string s;
            if constexpr (std::is_invocable_v<M>) { s = op(); }
            else { s = op; }
            throw std::runtime_error(context + " operation=" + s + " expected=" + show(want) + " actual=" + show(got));}}

    // Order on key only, so equivalent elements differ by tag (their index).
    struct Loose {
        int key, tag;
        bool operator<(const Loose &o) const { return key < o.key; }
        bool operator==(const Loose &o) const { return key == o.key && tag == o.tag; }
        friend std::ostream &operator<<(std::ostream &s, const Loose &x) { return s << "{" << x.key << "," << x.tag << "}"; }
    };

    // Stateful acted monoid: polynomial hash of a sequence under elementwise affine maps; noncommutative op and composition, F with operator==.
    struct HashAffine {
        lng mod, base;
        struct S { lng h, g, pw, len; };
        struct F { lng a, b; bool operator==(const F &) const = default; };
        S op(const S &x, const S &y) const { return {(x.h * y.pw + y.h) % mod, (x.g * y.pw + y.g) % mod, x.pw * y.pw % mod, x.len + y.len}; }
        S e() const { return {0, 0, 1, 0}; }
        S mapping(const F &f, const S &x) const { return {(f.a * x.h + f.b * x.g) % mod, x.g, x.pw, x.len}; }
        F composition(const F &f, const F &g) const { return {f.a * g.a % mod, (f.a * g.b + f.b) % mod}; }
        F id() const { return {1, 0}; }
    };

    // Specs: the oracle value K, its digest D compared with key(S), and the oracle's own arithmetic.
    struct SumSpec {
        using A = Sum<lng>;
        using K = lng;
        A mon() const { return {}; }
        K e() const { return 0; }
        K op(K a, K b) const { return a + b; }
        K leaf(Rng &rng) const { return rnd(rng, 0, 9); }
        lng val(K k) const { return k; }
        K key(lng s) const { return s; }
        K digest(K k) const { return k; }
        bool ok(K k, lng th) const { return k <= th; }
        lng threshold(Rng &rng) const { return rng() % 3 ? rnd(rng, 0, 60) : rnd(rng, 0, lng(1) << 36); }
        bool bounded(K k) const { return k <= 1000000; }
    };
    struct AffSpec {
        using A = PairMonoid<AffineComposite<mint>, Sum<lng>>;
        using K = tuple<lng, lng, lng>;
        A mon() const { return {}; }
        K e() const { return {1, 0, 0}; }
        K op(const K &p, const K &q) const {
            auto [a, b, c] = p;
            auto [x, y, z] = q;
            return {x * a % P, (x * b + y) % P, c + z};}
        K leaf(Rng &rng) const { return {rng() % 4 ? rnd(rng, 0, P - 1) : rnd(rng, 0, 2), rnd(rng, 0, P - 1), 1}; }
        A::S val(const K &k) const { return {{mint(std::get<0>(k)), mint(std::get<1>(k))}, std::get<2>(k)}; }
        K key(const A::S &s) const { return {lng(s.first.a.val()), lng(s.first.b.val()), s.second}; }
        K digest(const K &k) const { return k; }
        bool ok(const K &k, lng th) const { return std::get<2>(k) <= th; }
        lng threshold(Rng &rng) const { return rng() % 3 ? rnd(rng, 0, 12) : rng() % 2 ? rnd(rng, 0, lng(1) << 32) : rnd(rng, 0, INF); }
        bool bounded(const K &k) const { return std::get<2>(k) <= 1000; }
    };
    struct AffineSumSpec {
        using A = RangeAffineRangeSum<mint>;
        using K = pair<lng, lng>;
        A mon() const { return {}; }
        K e() const { return {0, 0}; }
        K op(const K &x, const K &y) const { return {(x.first + y.first) % P, x.second + y.second}; }
        K leaf(Rng &rng) const { return {rnd(rng, 0, P - 1), 1}; }
        A::S val(const K &k) const { return {mint(k.first), mint(k.second % P)}; }
        K key(const A::S &s) const { return {lng(s.sum.val()), lng(s.len.val())}; }
        K digest(const K &k) const { return {k.first, k.second % P}; }
        A::F action(Rng &rng) const {
            int c = int(rng() % 4);
            return c == 0 ? A::F{mint(1), mint(0)} : c == 1 ? A::F{mint(0), mint(rnd(rng, 0, P - 1))} : A::F{mint(rnd(rng, 0, P - 1)), mint(rnd(rng, 0, P - 1))};}
        K map(const A::F &f, const K &k) const { return {lng((lll(f.a.val()) * k.first + lll(f.b.val()) * (k.second % P)) % P), k.second}; }
        bool ok(const K &k, lng th) const { return k.second <= th; }
        lng threshold(Rng &rng) const { return rnd(rng, 0, 12); }
        bool bounded(const K &k) const { return k.second <= 1000; }
    };
    struct HashSpec {
        using A = HashAffine;
        using K = vector<lng>;
        using D = tuple<lng, lng, lng, lng>;
        lng mod, base;
        A mon() const { return {mod, base}; }
        K e() const { return {}; }
        K op(K a, const K &b) const { a.insert(a.end(), b.begin(), b.end()); return a; }
        K leaf(Rng &rng) const { return {rnd(rng, 0, mod - 1)}; }
        D digest(const K &k) const {
            lng h = 0, g = 0, pw = 1;
            for (lng x : k) { h = (h * base + x) % mod; g = (g * base + 1) % mod; pw = pw * base % mod; }
            return {h, g, pw, lng(k.size())};}
        A::S val(const K &k) const { auto [h, g, pw, len] = digest(k); return {h, g, pw, len}; }
        D key(const A::S &s) const { return {s.h, s.g, s.pw, s.len}; }
        A::F action(Rng &rng) const { return rng() % 4 ? A::F{rnd(rng, 0, mod - 1), rnd(rng, 0, mod - 1)} : A::F{1, 0}; }
        K map(const A::F &f, K k) const {
            for (lng &x : k) { x = (f.a * x + f.b) % mod; }
            return k;}
        bool ok(const D &d, lng th) const { return std::get<3>(d) <= th; }
        lng threshold(Rng &rng) const { return rnd(rng, 0, 12); }
        bool bounded(const K &k) const { return k.size() <= 48; }
    };
    struct AddSumSpec {
        using A = RangeAddRangeSum<lng>;
        using K = pair<lng, lng>;
        A mon() const { return {}; }
        K e() const { return {0, 0}; }
        K op(const K &x, const K &y) const { return {x.first + y.first, x.second + y.second}; }
        K leaf(Rng &rng) const { return {rnd(rng, 0, 9), 1}; }
        A::S val(const K &k) const { return {k.first, k.second}; }
        K key(const A::S &s) const { return {s.sum, s.len}; }
        K digest(const K &k) const { return k; }
        lng action(Rng &rng) const { return rnd(rng, 0, 4); }
        K map(lng f, const K &k) const { return {k.first + f * k.second, k.second}; }
        bool ok(const K &k, lng th) const { return k.first <= th; }
        lng threshold(Rng &rng) const { return rng() % 3 ? rnd(rng, 0, 80) : rnd(rng, 0, lng(1) << 40); }
        bool bounded(const K &k) const { return k.first <= 1000000 && k.second <= 1000; }
    };
    struct AddMinSpec {
        using A = RangeAddRangeMin<lng>;
        using K = lng;
        A mon() const { return {}; }
        K e() const { return INF; }
        K op(K a, K b) const { return min(a, b); }
        K leaf(Rng &rng) const { return rnd(rng, -50, 50); }
        lng val(K k) const { return k; }
        K key(lng s) const { return s; }
        K digest(K k) const { return k; }
        lng action(Rng &rng) const { return rnd(rng, -5, 5); }
        K map(lng f, K k) const { return k == INF ? INF : k + f; }
        bool ok(K k, lng th) const { return k >= th; }
        lng threshold(Rng &rng) const { return rnd(rng, -120, 60); }
        bool bounded(K) const { return true; }
    };

    // Brute force: a full copy of the array per version.
    template<typename Spec>
    struct VecOracle {
        using K = typename Spec::K;
        static constexpr bool DENSE = true;
        const Spec &sp;
        lng n;
        vector<vector<K>> ver;
        VecOracle(const Spec &s, lng N, const K &fill) : sp(s), n(N), ver{vector<K>(size_t(N), fill)} {}
        int count() const { return int(ver.size()); }
        K get(int v, lng i) const { return ver[v][i]; }
        K prod(int v, lng l, lng r) const {
            K res = sp.e();
            for (lng i = l; i < r; ++i) { res = sp.op(res, ver[v][i]); }
            return res;}
        bool bounded(int v) const { return std::all_of(ver[v].begin(), ver[v].end(), [&](const K &k) { return sp.bounded(k); }); }
        void set(int v, lng i, const K &x) {
            auto a = ver[v];
            a[i] = x;
            ver.push_back(std::move(a));}
        template<typename F> void apply(int v, lng l, lng r, const F &f) {
            auto a = ver[v];
            for (lng i = l; i < r; ++i) { a[i] = sp.map(f, a[i]); }
            ver.push_back(std::move(a));}
        void copy(int v, int w, lng l, lng r) {
            auto a = ver[v];
            for (lng i = l; i < r; ++i) { a[i] = ver[w][i]; }
            ver.push_back(std::move(a));}
        void split(int v, lng i) {
            vector<K> x(size_t(n), sp.e()), y(size_t(n), sp.e());
            for (lng j = 0; j < n; ++j) { (j < i ? x : y)[j] = ver[v][j]; }
            ver.push_back(std::move(x)); ver.push_back(std::move(y));}
        void merge(int v, int w) {
            vector<K> a(size_t(n), sp.e());
            for (lng i = 0; i < n; ++i) { a[i] = sp.op(ver[v][i], ver[w][i]); }
            ver.push_back(std::move(a));}
        template<typename G> lng maxRight(int v, lng l, G ok) const {
            K acc = sp.e();
            for (lng r = l; r < n; ++r) {
                K next = sp.op(acc, ver[v][r]);
                if (!ok(next)) { return r; }
                acc = std::move(next);}
            return n;}
        template<typename G> lng minLeft(int v, lng r, G ok) const {
            K acc = sp.e();
            for (lng l = r; l > 0; --l) {
                K next = sp.op(ver[v][l - 1], acc);
                if (!ok(next)) { return l; }
                acc = std::move(next);}
            return 0;}
    };

    // Map-based oracle for huge n: per version, a map from piece start to the value of every position in the piece.
    template<typename Spec>
    struct PieceOracle {
        using K = typename Spec::K;
        using Map = map<lng, K>;
        static constexpr bool DENSE = false;
        const Spec &sp;
        lng n;
        vector<Map> ver;
        PieceOracle(const Spec &s, lng N, const K &fill) : sp(s), n(N), ver(1) { if (n) { ver[0][0] = fill; }}
        int count() const { return int(ver.size()); }
        K power(K x, lng k) const {
            K res = sp.e();
            for (; k; k >>= 1) {
                if (k & 1) { res = sp.op(res, x); }
                if (k > 1) { x = sp.op(x, x); }}
            return res;}
        lng end(const Map &m, typename Map::const_iterator it) const { return std::next(it) == m.end() ? n : std::next(it)->first; }
        void cut(Map &m, lng x) const {
            if (x <= 0 || x >= n) { return; }
            auto it = std::prev(m.upper_bound(x));
            if (it->first != x) { m.emplace(x, it->second); }}
        K get(int v, lng i) const { return std::prev(ver[v].upper_bound(i))->second; }
        K prod(int v, lng l, lng r) const {
            K res = sp.e();
            if (l >= r) { return res; }
            const Map &m = ver[v];
            for (auto it = std::prev(m.upper_bound(l)); it != m.end() && it->first < r; ++it) { res = sp.op(res, power(it->second, min(r, end(m, it)) - max(l, it->first))); }
            return res;}
        bool bounded(int v) const { return std::all_of(ver[v].begin(), ver[v].end(), [&](const auto &p) { return sp.bounded(p.second); }); }
        void set(int v, lng i, const K &x) {
            Map m = ver[v];
            cut(m, i); cut(m, i + 1);
            m[i] = x;
            ver.push_back(std::move(m));}
        template<typename F> void apply(int v, lng l, lng r, const F &f) {
            Map m = ver[v];
            cut(m, l); cut(m, r);
            for (auto it = m.lower_bound(l); it != m.end() && it->first < r; ++it) { it->second = sp.map(f, it->second); }
            ver.push_back(std::move(m));}
        // a outside [l, r), b inside.
        Map splice(const Map &a, const Map &b, lng l, lng r) const {
            Map x = a, y = b;
            cut(x, l); cut(x, r); cut(y, l); cut(y, r);
            x.erase(x.lower_bound(l), x.lower_bound(r));
            for (auto it = y.lower_bound(l); it != y.end() && it->first < r; ++it) { x.insert(*it); }
            return x;}
        void copy(int v, int w, lng l, lng r) { ver.push_back(splice(ver[v], ver[w], l, r)); }
        void split(int v, lng i) {
            Map z;
            if (n) { z[0] = sp.e(); }
            Map x = splice(z, ver[v], 0, i), y = splice(z, ver[v], i, n);
            ver.push_back(std::move(x)); ver.push_back(std::move(y));}
        void merge(int v, int w) {
            Map m;
            for (const Map *s : {&ver[v], &ver[w]}) {
                for (auto &[i, k] : *s) { m[i] = sp.e(); }}
            for (auto &[i, k] : m) { k = sp.op(get(v, i), get(w, i)); }
            ver.push_back(std::move(m));}
        template<typename G> lng maxRight(int v, lng l, G ok) const {
            if (l == n) { return n; }
            const Map &m = ver[v];
            K acc = sp.e();
            for (auto it = std::prev(m.upper_bound(l)); it != m.end(); ++it) {
                lng a = max(l, it->first), b = end(m, it);
                K all = sp.op(acc, power(it->second, b - a));
                if (ok(all)) { acc = std::move(all); continue; }
                lng lo = 0, hi = b - a;
                while (hi - lo > 1) {
                    lng mid = std::midpoint(lo, hi);
                    if (ok(sp.op(acc, power(it->second, mid)))) { lo = mid; }
                    else { hi = mid; }}
                return a + lo;}
            return n;}
        template<typename G> lng minLeft(int v, lng r, G ok) const {
            if (r == 0) { return 0; }
            const Map &m = ver[v];
            K acc = sp.e();
            for (auto it = std::prev(m.upper_bound(r - 1));; --it) {
                lng a = it->first, b = min(r, end(m, it));
                K all = sp.op(power(it->second, b - a), acc);
                if (!ok(all)) {
                    lng lo = 0, hi = b - a;
                    while (hi - lo > 1) {
                        lng mid = std::midpoint(lo, hi);
                        if (ok(sp.op(power(it->second, mid), acc))) { lo = mid; }
                        else { hi = mid; }}
                    return b - lo;}
                acc = std::move(all);
                if (it == m.begin()) { return 0; }}}
    };

    // Random positions in [0, hi], biased to both ends and to recently used points.
    struct Points {
        Rng &rng;
        lng n;
        vector<lng> recent{0};
        lng operator()(lng hi) {
            lng c = rnd(rng, 0, 7);
            lll x = c < 2 ? rnd(rng, 0, 3) : c < 4 ? hi - rnd(rng, 0, 3) : c < 6 ? lll(recent[rng() % recent.size()]) + rnd(rng, -1, 1) : rnd(rng, 0, hi);
            lng y = lng(std::clamp(x, lll(0), lll(hi)));
            if (recent.size() < 32) { recent.push_back(y); }
            else { recent[rng() % 32] = y; }
            return y;}
        pair<lng, lng> range() {
            lng l = (*this)(n), r = (*this)(n);
            if (l > r) { swap(l, r); }
            return {l, r};}
    };

    void selfCheck() {
        context = "oracle self-check";
        HashSpec hs{97, 10};
        checkEqual(hs.digest({1, 2}), HashSpec::D(12, 11, 3, 2), "hash digest of [1,2], base 10, mod 97");
        checkEqual(hs.map({2, 1}, {1, 2}), vector<lng>{3, 5}, "hash elementwise affine");
        checkEqual(AffSpec().op({2, 3, 1}, {5, 7, 1}), AffSpec::K(10, 22, 2), "affine op applies the left map first");
        checkEqual(AffineSumSpec().map({mint(2), mint(3)}, {5, 2}), AffineSumSpec::K(16, 2), "affine sum mapping");
        checkEqual(AddMinSpec().map(3, INF), INF, "min identity stays under add");
        SumSpec s;
        VecOracle<SumSpec> o(s, 4, 1);
        o.set(0, 2, 5);
        checkEqual(o.prod(1, 0, 4), lng(8), "dense prod");
        checkEqual(o.maxRight(1, 0, [](lng k) { return k <= 6; }), 2, "dense maxRight");
        checkEqual(o.minLeft(1, 4, [](lng k) { return k <= 6; }), 2, "dense minLeft");}

    // The map oracle against the dense one on small n, including merges, splits and actions.
    template<typename Spec>
    void oracleCrossCheck(Rng &rng, const Spec &sp, const string &name) {
        for (int n : {1, 2, 3, 7, 20}) {
            context = "piece oracle vs dense oracle " + name + " n=" + show(n) + " seed=" + show(seed);
            auto fill = sp.leaf(rng);
            VecOracle<Spec> a(sp, n, fill);
            PieceOracle<Spec> b(sp, n, fill);
            Points pts{rng, n};
            for (int step = 0; step < scale(20, 80, 200); ++step) {
                int vc = a.count(), v = pick(rng, 0, vc - 1), w = pick(rng, 0, vc - 1), c = int(rng() % 5);
                lng i = pts(n - 1), j = pts(n);
                auto [l, r] = pts.range();
                if (c == 0) { auto x = sp.leaf(rng); a.set(v, i, x); b.set(v, i, x); }
                else if (c == 1) { auto f = sp.action(rng); a.apply(v, l, r, f); b.apply(v, l, r, f); }
                else if (c == 2) { a.copy(v, w, l, r); b.copy(v, w, l, r); }
                else if (c == 3) { a.split(v, j); b.split(v, j); }
                else if (a.bounded(v) && a.bounded(w)) { a.merge(v, w); b.merge(v, w); }
                checkEqual(b.count(), a.count(), "versions");
                for (int k = vc; k < a.count(); ++k) {
                    string at = "step=" + show(step) + " version=" + show(k);
                    for (lng x = 0; x <= n; ++x) {
                        for (lng y = x; y <= n; ++y) { checkEqual(sp.digest(b.prod(k, x, y)), sp.digest(a.prod(k, x, y)), [&] { return at + " prod(" + show(x) + "," + show(y) + ")"; }); }
                        lng th = sp.threshold(rng);
                        auto ok = [&](const auto &q) { return sp.ok(sp.digest(q), th); };
                        checkEqual(b.maxRight(k, x, ok), a.maxRight(k, x, ok), [&] { return at + " maxRight(" + show(x) + ",th=" + show(th) + ")"; });
                        checkEqual(b.minLeft(k, x, ok), a.minLeft(k, x, ok), [&] { return at + " minLeft(" + show(x) + ",th=" + show(th) + ")"; });}}}}
        std::cout << "PASS map oracle agrees with the dense oracle (" << name << ")\n";}

    // One random persistent history checked against the oracle; every new version fully, a random older one each step, all at the end.
    template<bool COMM, template<typename> typename Oracle, typename Spec>
    void history(Rng &rng, const Spec &sp, lng n, int init, int steps, bool merges, const string &name) {
        using A = typename Spec::A;
        using S = typename A::S;
        using K = typename Spec::K;
        using Tree = PersistentSegmentTree<A, COMM>;
        constexpr bool LAZY = requires { typename A::F; };
        constexpr bool DENSE = Oracle<Spec>::DENSE;
        string base = name + " COMMUTATIVE=" + show(COMM) + " n=" + show(n) + " init=" + array<string, 3>{"empty", "fill", "vector"}[init] + " seed=" + show(seed);
        context = base;
        K fill = init == 1 ? sp.leaf(rng) : sp.e();
        Oracle<Spec> o(sp, n, fill);
        vector<S> start;
        if constexpr (DENSE) {
            if (init == 2) {
                for (lng i = 0; i < n; ++i) { o.ver[0][i] = sp.leaf(rng); start.push_back(sp.val(o.ver[0][i])); }}}
        Tree t = init == 0 ? Tree(n, sp.mon()) : init == 1 ? Tree(n, sp.val(fill), sp.mon()) : Tree(start, sp.mon());
        int h = int(std::bit_width(ulng(n))) + 1;
        checkEqual(t.versionCount(), 1, "initial versionCount");
        if (init == 0 || n == 0) { checkEqual(t.nodeCount(), 0, "initial nodeCount"); }
        else if (init == 1) { checkEqual(t.nodeCount() <= 2 * h, true, [&] { return "fill nodeCount " + show(t.nodeCount()) + " <= 2 * bit_width(n) + 2"; }); }
        else { checkEqual(t.nodeCount(), 2 * n - 1, "vector nodeCount"); }
        if (rng() % 2) {
            int c = t.nodeCount();
            t.reserve(pick(rng, 0, 4 * steps * h));
            checkEqual(t.nodeCount(), c, "reserve keeps nodeCount");}
        Points pts{rng, n};
        auto treeOk = [&sp](lng th) { return [&sp, th](const S &s) { return sp.ok(sp.key(s), th); }; };
        auto refOk = [&sp](lng th) { return [&sp, th](const K &k) { return sp.ok(sp.digest(k), th); }; };
        auto searches = [&](int v, lng l, lng r, const string &at) {
            lng th = sp.threshold(rng);
            checkEqual(t.maxRight(v, l, treeOk(th)), o.maxRight(v, l, refOk(th)), [&] { return at + " maxRight(" + show(v) + "," + show(l) + ",th=" + show(th) + ")"; });
            checkEqual(t.minLeft(v, r, treeOk(th)), o.minLeft(v, r, refOk(th)), [&] { return at + " minLeft(" + show(v) + "," + show(r) + ",th=" + show(th) + ")"; });};
        auto verify = [&](int v, bool heavy, const string &at) {
            checkEqual(sp.key(t.allProd(v)), sp.digest(o.prod(v, 0, n)), [&] { return at + " allProd(" + show(v) + ")"; });
            if constexpr (DENSE) {
                if (heavy) {
                    vector<S> got = t.toVector(v);
                    checkEqual(lng(got.size()), n, [&] { return at + " toVector size"; });
                    for (lng i = 0; i < n; ++i) { checkEqual(sp.key(got[i]), sp.digest(o.get(v, i)), [&] { return at + " toVector(" + show(v) + ")[" + show(i) + "]"; }); }}}
            if (heavy && n <= 12) {
                for (lng l = 0; l <= n; ++l) {
                    for (lng r = l; r <= n; ++r) { checkEqual(sp.key(t.prod(v, l, r)), sp.digest(o.prod(v, l, r)), [&] { return at + " prod(" + show(v) + "," + show(l) + "," + show(r) + ")"; }); }
                    if (l < n) { checkEqual(sp.key(t.get(v, l)), sp.digest(o.get(v, l)), [&] { return at + " get(" + show(v) + "," + show(l) + ")"; }); }
                    searches(v, l, l, at);}
                return;}
            for (int q = 0; q < (heavy ? 6 : 2); ++q) {
                auto [l, r] = pts.range();
                checkEqual(sp.key(t.prod(v, l, r)), sp.digest(o.prod(v, l, r)), [&] { return at + " prod(" + show(v) + "," + show(l) + "," + show(r) + ")"; });
                if (n) {
                    lng i = pts(n - 1);
                    checkEqual(sp.key(t.get(v, i)), sp.digest(o.get(v, i)), [&] { return at + " get(" + show(v) + "," + show(i) + ")"; });}
                searches(v, l, r, at);}};
        verify(0, true, "initial");
        for (int step = 0; step < steps; ++step) {
            int vc = t.versionCount(), v = rng() % 2 ? vc - 1 : pick(rng, 0, vc - 1), w = rng() % 6 ? pick(rng, 0, vc - 1) : v, before = t.nodeCount(), made = 1, c = int(rng() % 10);
            auto [l, r] = pts.range();
            lng i = n ? pts(n - 1) : 0, bound = 4 * h;
            string at = "step=" + show(step), what;
            if (c >= 7) { verify(v, false, at + " query"); continue; }
            if (c == 6 && !(merges && o.bounded(v) && o.bounded(w))) { c = 4; }
            if (n == 0 && c <= 3) { c = rng() % 2 && LAZY ? 2 : 4; }
            if (c <= 1 || (c <= 3 && !LAZY)) {
                K x = sp.leaf(rng);
                if (rng() % 4 == 0 && n <= INT_MAX && sp.bounded(o.prod(w, l, r))) { x = o.prod(w, l, r); }
                what = "set(" + show(v) + "," + show(i) + ")";
                checkEqual(t.set(v, i, sp.val(x)), vc, [&] { return at + " " + what + " id"; });
                o.set(v, i, x);
                bound = 2 * h;}
            else if (c <= 3) {
                if constexpr (LAZY) {
                    auto f = sp.action(rng);
                    what = "apply(" + show(v) + "," + show(l) + "," + show(r) + ")";
                    checkEqual(t.apply(v, l, r, f), vc, [&] { return at + " " + what + " id"; });
                    o.apply(v, l, r, f);}}
            else if (c == 4) {
                what = "copyRange(" + show(v) + "," + show(w) + "," + show(l) + "," + show(r) + ")";
                checkEqual(t.copyRange(v, w, l, r), vc, [&] { return at + " " + what + " id"; });
                o.copy(v, w, l, r);}
            else if (c == 5) {
                lng j = pts(n);
                what = "split(" + show(v) + "," + show(j) + ")";
                checkEqual(t.split(v, j), pair(vc, vc + 1), [&] { return at + " " + what + " ids"; });
                o.split(v, j);
                made = 2;
                bound = 8 * h;}
            else {
                what = "merge(" + show(v) + "," + show(w) + ")";
                checkEqual(t.merge(v, w), vc, [&] { return at + " " + what + " id"; });
                o.merge(v, w);
                bound = INF;}
            context = base + " last=" + what;
            checkEqual(t.versionCount(), vc + made, [&] { return at + " " + what + " versionCount"; });
            checkEqual(o.count(), vc + made, [&] { return at + " oracle versions"; });
            checkEqual(t.nodeCount() - before <= bound, true, [&] { return at + " " + what + " new nodes " + show(t.nodeCount() - before) + " <= " + show(bound); });
            for (int k = vc; k < vc + made; ++k) { verify(k, true, at + " " + what); }
            verify(pick(rng, 0, vc - 1), false, at + " older");}
        context = base;
        for (int k = 0; k < t.versionCount(); ++k) { verify(k, true, "final"); }}

    // apply over never-set positions of an empty tree keeps them e() and allocates nothing.
    template<bool COMM, typename Spec>
    void nullNodes(Rng &rng, const Spec &sp, lng n, const string &name) {
        context = name + " null nodes COMMUTATIVE=" + show(COMM) + " n=" + show(n) + " seed=" + show(seed);
        PersistentSegmentTree<typename Spec::A, COMM> t(n, sp.mon());
        Points pts{rng, n};
        auto e = sp.digest(sp.e());
        for (int step = 1; step <= 20; ++step) {
            auto [l, r] = pts.range();
            lng i = pts(n - 1);
            checkEqual(t.apply(pick(rng, 0, step - 1), l, r, sp.action(rng)), step, "apply id");
            checkEqual(t.nodeCount(), 0, [&] { return "apply(" + show(l) + "," + show(r) + ") on never-set positions allocates nothing"; });
            checkEqual(sp.key(t.get(step, i)), e, [&] { return "get never-set " + show(i); });
            checkEqual(sp.key(t.allProd(step)), e, "allProd");
            checkEqual(sp.key(t.prod(step, l, r)), e, "prod");}
        lng i = pts(n - 1), j = (i + 1) % n;
        auto x = sp.leaf(rng);
        auto f = sp.action(rng);
        int v = t.apply(t.set(20, i, sp.val(x)), 0, n, f);
        checkEqual(sp.key(t.get(v, i)), sp.digest(sp.map(f, x)), "set position after apply");
        checkEqual(sp.key(t.allProd(v)), sp.digest(sp.map(f, x)), "allProd after apply");
        if (j != i) { checkEqual(sp.key(t.get(v, j)), e, "never-set neighbour stays e()"); }}

    void segmentTests(Rng &rng) {
        vector<int> ns{0, 1, 2, 3, 5, 8, 13};
        if (mode != "quick") { ns = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 31, 32, 33, 64, 100}; }
        if (mode == "stress") {
            for (int k = 0; k < 6; ++k) { ns.push_back(pick(rng, 1, 200)); }}
        int steps = scale(30, 100, 260), round = 0;
        auto hash = [&]() {
            lng mod = array<lng, 4>{2, 97, 998244353, 1000000007}[round++ % 4];
            return HashSpec{mod, rnd(rng, 1, mod - 1)};};
        auto all = [&](int n, int init, int q, bool merges, const string &tag) {
            history<false, VecOracle>(rng, SumSpec{}, n, init, q, merges, tag + "Sum<lng>");
            history<true, VecOracle>(rng, SumSpec{}, n, init, q, merges, tag + "Sum<lng>");
            history<false, VecOracle>(rng, AffSpec{}, n, init, q, merges, tag + "PairMonoid<AffineComposite<mint>,Sum<lng>>");
            history<false, VecOracle>(rng, AffineSumSpec{}, n, init, q, merges, tag + "RangeAffineRangeSum<mint>");
            history<false, VecOracle>(rng, hash(), n, init, q, merges, tag + "stateful HashAffine");
            history<false, VecOracle>(rng, AddSumSpec{}, n, init, q, merges, tag + "RangeAddRangeSum<lng>");
            history<true, VecOracle>(rng, AddSumSpec{}, n, init, q, merges, tag + "RangeAddRangeSum<lng>");
            history<false, VecOracle>(rng, AddMinSpec{}, n, init, q, merges, tag + "RangeAddRangeMin<lng>");
            history<true, VecOracle>(rng, AddMinSpec{}, n, init, q, merges, tag + "RangeAddRangeMin<lng>");};
        for (int n : ns) {
            for (int init = 0; init < 3; ++init) { all(n, init, steps, true, ""); }}
        std::cout << "PASS PersistentSegmentTree small n (" << ns.size() << " sizes x 3 builds x 9 configurations, " << steps << " steps each)\n";
        int medium = scale(200, 1000, 1500);
        for (int init = 0; init < 3; ++init) { all(medium, init, scale(15, 60, 100), true, "medium "); }
        std::cout << "PASS PersistentSegmentTree n=" << medium << "\n";
        for (lng n : {lng(1), lng(5), lng(64), lng(INT_MAX), INF}) {
            nullNodes<false>(rng, AffineSumSpec{}, n, "RangeAffineRangeSum<mint>");
            nullNodes<false>(rng, hash(), n, "stateful HashAffine");
            nullNodes<false>(rng, AddSumSpec{}, n, "RangeAddRangeSum<lng>");
            nullNodes<true>(rng, AddSumSpec{}, n, "RangeAddRangeSum<lng>");
            nullNodes<true>(rng, AddMinSpec{}, n, "RangeAddRangeMin<lng>");}
        std::cout << "PASS PersistentSegmentTree null nodes stay e() under apply\n";
        oracleCrossCheck(rng, AddSumSpec{}, "RangeAddRangeSum<lng>");
        oracleCrossCheck(rng, AffineSumSpec{}, "RangeAffineRangeSum<mint>");
        oracleCrossCheck(rng, AddMinSpec{}, "RangeAddRangeMin<lng>");
        int large = scale(25, 120, 600);
        vector<lng> huge{INT_MAX, INF};
        if (mode != "quick") { huge.push_back((1 << 30) + 1); huge.push_back((lng(1) << 62) + 3); }
        for (lng n : huge) {
            // Merging two filled trees costs O(n), so merges run only on sparse (empty-built) histories; filled sums over n > 2^31 would overflow lng.
            bool sums = n <= INT_MAX;
            history<false, PieceOracle>(rng, SumSpec{}, n, 0, large, true, "large Sum<lng>");
            if (sums) { history<false, PieceOracle>(rng, SumSpec{}, n, 1, large, false, "large Sum<lng>"); }
            history<false, PieceOracle>(rng, AffSpec{}, n, 0, large, true, "large PairMonoid<AffineComposite<mint>,Sum<lng>>");
            history<false, PieceOracle>(rng, AffSpec{}, n, 1, large, false, "large PairMonoid<AffineComposite<mint>,Sum<lng>>");
            history<false, PieceOracle>(rng, AffineSumSpec{}, n, 0, large, true, "large RangeAffineRangeSum<mint>");
            history<false, PieceOracle>(rng, AddSumSpec{}, n, 0, large, true, "large RangeAddRangeSum<lng>");
            history<true, PieceOracle>(rng, AddSumSpec{}, n, 0, large, true, "large RangeAddRangeSum<lng>");
            if (sums) { history<false, PieceOracle>(rng, AddSumSpec{}, n, 1, large, false, "large RangeAddRangeSum<lng>"); }
            if (sums) { history<true, PieceOracle>(rng, AddSumSpec{}, n, 1, large, false, "large RangeAddRangeSum<lng>"); }
            history<true, PieceOracle>(rng, AddMinSpec{}, n, 0, large, true, "large RangeAddRangeMin<lng>");
            history<true, PieceOracle>(rng, AddMinSpec{}, n, 1, large, false, "large RangeAddRangeMin<lng>");
            history<false, PieceOracle>(rng, AddMinSpec{}, n, 1, large, false, "large RangeAddRangeMin<lng>");}
        std::cout << "PASS PersistentSegmentTree n up to 2^63 - 1 against the map oracle (" << large << " steps)\n";}

    // Dual specs: K is the composed action at a position, map(f, g) composes f after g.
    struct DualAffineSpec {
        using A = RangeAffineRangeSum<mint>;
        using K = pair<lng, lng>;
        A mon() const { return {}; }
        K e() const { return {1, 0}; }
        A::F action(Rng &rng) const { return rng() % 5 ? A::F{mint(rnd(rng, 0, P - 1)), mint(rnd(rng, 0, P - 1))} : A::F{mint(1), mint(0)}; }
        K key(const A::F &f) const { return {lng(f.a.val()), lng(f.b.val())}; }
        K map(const A::F &f, const K &g) const { auto [a, b] = key(f); return {a * g.first % P, (a * g.second + b) % P}; }
    };
    struct DualHashSpec {
        using A = HashAffine;
        using K = pair<lng, lng>;
        lng mod;
        A mon() const { return {mod, 1}; }
        K e() const { return {1, 0}; }
        A::F action(Rng &rng) const { return rng() % 4 ? A::F{rnd(rng, 0, mod - 1), rnd(rng, 0, mod - 1)} : A::F{1, 0}; }
        K key(const A::F &f) const { return {f.a, f.b}; }
        K map(const A::F &f, const K &g) const { return {f.a * g.first % mod, (f.a * g.second + f.b) % mod}; }
    };
    struct DualAddSpec {
        using A = RangeAddRangeSum<lng>;
        using K = lng;
        A mon() const { return {}; }
        K e() const { return 0; }
        lng action(Rng &rng) const { return rng() % 5 ? rnd(rng, -1000000000, 1000000000) : 0; }
        K key(lng f) const { return f; }
        K map(lng f, K g) const { return f + g; }
    };
    template<typename Act>
    struct DualModSpec {
        using A = Act;
        using K = lng;
        A act;
        lng mod;
        A mon() const { return act; }
        K e() const { return 0; }
        lng action(Rng &rng) const { return rnd(rng, 0, mod - 1); }
        K key(lng f) const { return f; }
        K map(lng f, K g) const { return (f + g) % mod; }
    };

    template<bool COMM, template<typename> typename Oracle, typename Spec>
    void dualHistory(Rng &rng, const Spec &sp, lng n, int steps, const string &name) {
        using Tree = PersistentDualSegmentTree<typename Spec::A, COMM>;
        string base = name + " dual COMMUTATIVE=" + show(COMM) + " n=" + show(n) + " seed=" + show(seed);
        context = base;
        Tree t(n, sp.mon());
        Oracle<Spec> o(sp, n, sp.e());
        int h = int(std::bit_width(ulng(n))) + 1;
        checkEqual(t.nodeCount(), 0, "initial nodeCount");
        checkEqual(t.versionCount(), 1, "initial versionCount");
        if (rng() % 2) {
            t.reserve(pick(rng, 0, 4 * steps * h));
            checkEqual(t.nodeCount(), 0, "reserve keeps nodeCount");}
        Points pts{rng, n};
        auto verify = [&](int v, bool heavy, const string &at) {
            if (n && heavy && n <= 64) {
                for (lng i = 0; i < n; ++i) { checkEqual(sp.key(t.get(v, i)), o.get(v, i), [&] { return at + " get(" + show(v) + "," + show(i) + ")"; }); }
                return;}
            for (int q = 0; n && q < (heavy ? 8 : 3); ++q) {
                lng i = pts(n - 1);
                checkEqual(sp.key(t.get(v, i)), o.get(v, i), [&] { return at + " get(" + show(v) + "," + show(i) + ")"; });}};
        for (int step = 0; step < steps; ++step) {
            int vc = t.versionCount(), v = rng() % 2 ? vc - 1 : pick(rng, 0, vc - 1), before = t.nodeCount();
            string at = "step=" + show(step);
            if (rng() % 4 == 0) { verify(v, false, at + " query"); continue; }
            auto [l, r] = pts.range();
            auto f = sp.action(rng);
            string what = "apply(" + show(v) + "," + show(l) + "," + show(r) + ")";
            context = base + " last=" + what;
            checkEqual(t.apply(v, l, r, f), vc, [&] { return at + " " + what + " id"; });
            o.apply(v, l, r, f);
            checkEqual(t.versionCount(), vc + 1, [&] { return at + " versionCount"; });
            checkEqual(t.nodeCount() - before <= 4 * h, true, [&] { return at + " " + what + " new nodes " + show(t.nodeCount() - before); });
            verify(vc, true, at + " " + what);
            verify(pick(rng, 0, vc - 1), false, at + " older");}
        context = base;
        for (int k = 0; k < t.versionCount(); ++k) { verify(k, true, "final"); }}

    void dualTests(Rng &rng) {
        vector<int> ns{0, 1, 2, 3, 5, 8, 13};
        if (mode != "quick") { ns = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 31, 33, 64, 100, 1000}; }
        if (mode == "stress") {
            for (int k = 0; k < 8; ++k) { ns.push_back(pick(rng, 1, 2000)); }}
        int steps = scale(40, 200, 500);
        for (int n : ns) {
            lng mod = array<lng, 3>{2, 97, 1000000007}[rng() % 3];
            auto add = [mod](lng x, lng y) { return (x + y) % mod; };
            MonoidAction act{Monoid{lng(0), add}};
            dualHistory<false, VecOracle>(rng, DualAffineSpec{}, n, steps, "RangeAffineRangeSum<mint>");
            dualHistory<false, VecOracle>(rng, DualHashSpec{mod}, n, steps, "stateful HashAffine");
            dualHistory<true, VecOracle>(rng, DualAddSpec{}, n, steps, "RangeAddRangeSum<lng>");
            dualHistory<false, VecOracle>(rng, DualAddSpec{}, n, steps, "RangeAddRangeSum<lng>");
            dualHistory<true, VecOracle>(rng, DualModSpec<decltype(act)>{act, mod}, n, steps, "MonoidAction<Monoid<lambda>>");}
        for (lng n : {lng(INT_MAX), (lng(1) << 30) + 1, INF, (lng(1) << 62) + 3}) {
            dualHistory<false, PieceOracle>(rng, DualAffineSpec{}, n, steps, "large RangeAffineRangeSum<mint>");
            dualHistory<true, PieceOracle>(rng, DualAddSpec{}, n, steps, "large RangeAddRangeSum<lng>");
            dualHistory<false, PieceOracle>(rng, DualHashSpec{1000000007}, n, steps, "large stateful HashAffine");}
        std::cout << "PASS PersistentDualSegmentTree (" << ns.size() << " sizes + n up to 2^63 - 1, " << steps << " steps)\n";}

    template<typename T, typename W>
    void valueTree(Rng &rng, const vector<T> &a, const vector<W> &w, const vector<T> &extra, int queries, const string &name) {
        int n = int(a.size());
        context = name + " n=" + show(n) + " weighted=" + show(!w.empty()) + " seed=" + show(seed);
        PersistentValueTree<T, W> t(a, w);
        vector<T> probes = extra;
        for (const T &x : a) { probes.push_back(x); }
        auto query = [&](int l, int r, bool all) {
            string at = "l=" + show(l) + " r=" + show(r);
            int len = r - l;
            vector<int> ord(len);
            iota(ord.begin(), ord.end(), l);
            sort(ord.begin(), ord.end(), [&](int i, int j) { return a[i] < a[j] || (!(a[j] < a[i]) && i < j); });
            for (int k = 0; k < len; ++k) {
                if (all || rng() % ulng(len) < 2) { checkEqual(t.rangeKth(l, r, k), a[ord[k]], [&] { return at + " rangeKth(" + show(k) + ")"; }); }}
            if (!w.empty()) {
                W sum = W();
                for (int k = 0; k <= len; ++k) {
                    if (all || k == len || rng() % ulng(len) < 2) { checkEqual(t.kthSum(l, r, k), sum, [&] { return at + " kthSum(" + show(k) + ")"; }); }
                    if (k < len) { sum += w[ord[k]]; }}}
            for (int q = 0; q < (all ? 8 : 3); ++q) {
                const T &x = probes[rng() % probes.size()], &y = probes[rng() % probes.size()], &sentinel = probes[rng() % probes.size()];
                int eq = 0, in = 0, le = -1, ge = -1;
                for (int i = l; i < r; ++i) {
                    eq += !(a[i] < x) && !(x < a[i]);
                    in += !(a[i] < x) && a[i] < y;
                    if (!(x < a[i]) && (le < 0 || !(a[i] < a[le]))) { le = i; }
                    if (!(a[i] < x) && (ge < 0 || a[i] < a[ge])) { ge = i; }}
                string q2 = at + " x=" + show(x);
                checkEqual(t.countEqual(l, r, x), eq, [&] { return q2 + " countEqual"; });
                checkEqual(t.countRange(l, r, x, y), x < y ? in : 0, [&] { return q2 + " countRange to " + show(y); });
                T res = sentinel;
                checkEqual(t.maxLeq(l, r, x, res), le >= 0, [&] { return q2 + " maxLeq found"; });
                checkEqual(res, le >= 0 ? a[le] : sentinel, [&] { return q2 + " maxLeq value (untouched when absent)"; });
                res = sentinel;
                checkEqual(t.minGeq(l, r, x, res), ge >= 0, [&] { return q2 + " minGeq found"; });
                checkEqual(res, ge >= 0 ? a[ge] : sentinel, [&] { return q2 + " minGeq value (untouched when absent)"; });}};
        if (n <= 24) {
            for (int l = 0; l <= n; ++l) {
                for (int r = l; r <= n; ++r) { query(l, r, true); }}
            return;}
        query(0, n, false);
        for (int q = 0; q < queries; ++q) {
            int l = pick(rng, 0, n), r = pick(rng, 0, n);
            if (l > r) { swap(l, r); }
            query(l, r, false);}}

    void valueTests(Rng &rng) {
        vector<lng> extremes{NEG, NEG + 1, -1, 0, 1, INF - 1, INF};
        vector<int> ns{0, 1, 2, 5, 13};
        if (mode != "quick") { ns = {0, 1, 2, 3, 4, 5, 7, 8, 13, 16, 24}; }
        int medium = scale(300, 4000, 20000), queries = scale(30, 200, 300);
        auto lngs = [&](int n, int kind) {
            vector<lng> a(n);
            for (lng &x : a) { x = kind == 0 ? rnd(rng, -3, 3) : kind == 1 ? lng(rng()) : extremes[rng() % extremes.size()]; }
            return a;};
        auto weights = [&](int n) {
            vector<lng> w(n);
            for (lng &x : w) { x = rnd(rng, -1000000000000, 1000000000000); }
            return w;};
        for (int n : ns) {
            for (int kind = 0; kind < 3; ++kind) {
                auto a = lngs(n, kind);
                valueTree(rng, a, weights(n), extremes, 0, "PersistentValueTree<lng> kind=" + show(kind));
                valueTree(rng, a, vector<lng>{}, extremes, 0, "PersistentValueTree<lng> unweighted kind=" + show(kind));}
            vector<string> s(n);
            for (string &x : s) { x = string(size_t(rnd(rng, 0, 3)), 'a'); for (char &ch : x) { ch = char('a' + rng() % 2); }}
            valueTree(rng, s, weights(n), vector<string>{"", "a", "ab", "b", "zz"}, 0, "PersistentValueTree<string>");
            vector<pair<int, int>> p(n);
            vector<int> pw(n);
            for (int i = 0; i < n; ++i) { p[i] = {pick(rng, -2, 2), pick(rng, -2, 2)}; pw[i] = pick(rng, -1000, 1000); }
            valueTree(rng, p, pw, vector<pair<int, int>>{{INT_MIN, INT_MIN}, {0, 0}, {INT_MAX, INT_MAX}}, 0, "PersistentValueTree<pair<int,int>,int>");
            valueTree(rng, p, vector<int>{}, vector<pair<int, int>>{{0, 0}}, 0, "PersistentValueTree<pair<int,int>,int> unweighted");
            vector<Loose> o(n);
            for (int i = 0; i < n; ++i) { o[i] = {pick(rng, -2, 2), i}; }
            valueTree(rng, o, weights(n), vector<Loose>{{-3, -1}, {0, -1}, {3, -1}}, 0, "PersistentValueTree<key-only order>");}
        context = "PersistentValueTree n=0";
        PersistentValueTree<lng> z;
        lng res = 7;
        checkEqual(z.kthSum(0, 0, 0), lng(0), "unweighted n=0 kthSum");
        checkEqual(z.maxLeq(0, 0, 0, res) || z.minGeq(0, 0, 0, res) || res != 7, false, "n=0 searches report absence");
        for (int kind = 0; kind < 3; ++kind) { valueTree(rng, lngs(medium, kind), weights(medium), extremes, queries, "PersistentValueTree<lng> medium kind=" + show(kind)); }
        std::cout << "PASS PersistentValueTree (" << ns.size() << " exhaustive sizes, n=" << medium << " with " << queries << " ranges)\n";}

    template<typename T>
    void distinctCount(Rng &rng, const vector<T> &a, int queries, const string &name) {
        int n = int(a.size());
        context = name + " n=" + show(n) + " seed=" + show(seed);
        PersistentDistinctCount<T> t(a);
        if (n <= 48) {
            for (int l = 0; l <= n; ++l) {
                set<T> s;
                for (int r = l; r <= n; ++r) {
                    checkEqual(t.countDistinct(l, r), int(s.size()), [&] { return "countDistinct(" + show(l) + "," + show(r) + ")"; });
                    if (r < n) { s.insert(a[r]); }}}
            return;}
        for (int q = 0; q < queries; ++q) {
            int l = pick(rng, 0, n), r = pick(rng, 0, n);
            if (l > r) { swap(l, r); }
            checkEqual(t.countDistinct(l, r), int(set<T>(a.begin() + l, a.begin() + r).size()), [&] { return "countDistinct(" + show(l) + "," + show(r) + ")"; });}}

    void distinctTests(Rng &rng) {
        vector<int> ns{0, 1, 2, 3, 5, 8, 16, 48};
        int medium = scale(500, 5000, 10000), queries = scale(30, 200, 300);
        ns.push_back(medium);
        for (int n : ns) {
            for (int kind = 0; kind < 5; ++kind) {
                vector<lng> a(n);
                lng c = rnd(rng, -9, 9);
                for (int i = 0; i < n; ++i) { a[i] = kind == 0 ? rnd(rng, 0, 3) : kind == 1 ? c : kind == 2 ? lng(i) : kind == 3 ? array<lng, 3>{NEG, 0, INF}[rng() % 3] : rnd(rng, 0, n); }
                if (kind == 2) { std::shuffle(a.begin(), a.end(), rng); }
                distinctCount(rng, a, queries, "PersistentDistinctCount<lng> kind=" + show(kind));}
            vector<string> s(n);
            for (string &x : s) { x = string(size_t(rnd(rng, 0, 2)), 'a'); for (char &ch : x) { ch = char('a' + rng() % 2); }}
            distinctCount(rng, s, queries, "PersistentDistinctCount<string>");}
        std::cout << "PASS PersistentDistinctCount (all-equal, all-distinct, extremes, strings; n up to " << medium << ")\n";}

    void invalid(const string &name) {
        using A = RangeAddRangeSum<lng>;
        PersistentSegmentTree<A> t(5);
        PersistentDualSegmentTree<A> d(5);
        PersistentValueTree<lng> u({3, 1, 2});
        PersistentValueTree<lng> wv({3, 1, 2}, {1, 1, 1});
        PersistentDistinctCount<lng> c({1, 1, 2});
        auto yes = [](const A::S &) { return true; };
        auto no = [](const A::S &) { return false; };
        A::S x{1, 1};
        lng res = 0;
        if (name == "seg-negative-size") { PersistentSegmentTree<A> z(-1); }
        else if (name == "seg-fill-negative-size") { PersistentSegmentTree<A> z(-1, x); }
        else if (name == "seg-version-high") { (void)t.get(1, 0); }
        else if (name == "seg-version-negative") { (void)t.allProd(-1); }
        else if (name == "seg-get-high") { (void)t.get(0, 5); }
        else if (name == "seg-get-empty") { PersistentSegmentTree<A> z(0); (void)z.get(0, 0); }
        else if (name == "seg-tovector-version") { (void)t.toVector(1); }
        else if (name == "seg-set-negative") { (void)t.set(0, -1, x); }
        else if (name == "seg-set-version") { (void)t.set(2, 0, x); }
        else if (name == "seg-apply-reversed") { (void)t.apply(0, 3, 2, 1); }
        else if (name == "seg-apply-high") { (void)t.apply(0, 0, 6, 1); }
        else if (name == "seg-apply-version") { (void)t.apply(1, 0, 1, 1); }
        else if (name == "seg-copy-version") { (void)t.copyRange(0, 1, 0, 1); }
        else if (name == "seg-copy-reversed") { (void)t.copyRange(0, 0, 4, 3); }
        else if (name == "seg-copy-negative") { (void)t.copyRange(0, 0, -1, 3); }
        else if (name == "seg-split-high") { (void)t.split(0, 6); }
        else if (name == "seg-split-version") { (void)t.split(-1, 0); }
        else if (name == "seg-merge-version") { (void)t.merge(0, 1); }
        else if (name == "seg-prod-reversed") { (void)t.prod(0, 3, 2); }
        else if (name == "seg-prod-high") { (void)t.prod(0, 0, 6); }
        else if (name == "seg-max-right-identity") { (void)t.maxRight(0, 0, no); }
        else if (name == "seg-max-right-high") { (void)t.maxRight(0, 6, yes); }
        else if (name == "seg-max-right-version") { (void)t.maxRight(1, 0, yes); }
        else if (name == "seg-min-left-identity") { (void)t.minLeft(0, 5, no); }
        else if (name == "seg-min-left-negative") { (void)t.minLeft(0, -1, yes); }
        else if (name == "seg-min-left-version") { (void)t.minLeft(1, 5, yes); }
        else if (name == "seg-min-left-high") { (void)t.minLeft(0, 6, yes); }
        else if (name == "seg-huge-get-high") { PersistentSegmentTree<A> z(INF); (void)z.get(0, INF); }
        else if (name == "seg-huge-prod-high") { PersistentSegmentTree<A> z(INF - 1, x); (void)z.prod(0, 0, INF); }
        else if (name == "dual-negative-size") { PersistentDualSegmentTree<A> z(-1); }
        else if (name == "dual-get-high") { (void)d.get(0, 5); }
        else if (name == "dual-get-version") { (void)d.get(1, 0); }
        else if (name == "dual-apply-reversed") { (void)d.apply(0, 3, 2, 1); }
        else if (name == "dual-apply-high") { (void)d.apply(0, 0, 6, 1); }
        else if (name == "dual-apply-version") { (void)d.apply(1, 0, 1, 1); }
        else if (name == "dual-huge-get-high") { PersistentDualSegmentTree<A> z(INF); (void)z.get(0, INF); }
        else if (name == "value-weight-size") { PersistentValueTree<lng> z({1, 2, 3}, {1, 2}); }
        else if (name == "value-count-reversed") { (void)u.countEqual(2, 1, 1); }
        else if (name == "value-range-high") { (void)u.countRange(0, 4, 0, 5); }
        else if (name == "value-kth-high") { (void)u.rangeKth(0, 2, 2); }
        else if (name == "value-kth-negative") { (void)u.rangeKth(0, 2, -1); }
        else if (name == "value-kth-empty") { (void)u.rangeKth(1, 1, 0); }
        else if (name == "value-kthsum-unweighted") { (void)u.kthSum(0, 2, 1); }
        else if (name == "value-kthsum-high") { (void)wv.kthSum(0, 2, 3); }
        else if (name == "value-kthsum-negative") { (void)wv.kthSum(0, 2, -1); }
        else if (name == "value-maxleq-reversed") { (void)u.maxLeq(2, 1, 1, res); }
        else if (name == "value-mingeq-high") { (void)u.minGeq(0, 4, 1, res); }
        else if (name == "distinct-reversed") { (void)c.countDistinct(2, 1); }
        else if (name == "distinct-high") { (void)c.countDistinct(0, 4); }
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
        std::cout << "persistentsegmenttree seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        selfCheck();
        segmentTests(rng);
        dualTests(rng);
        valueTests(rng);
        distinctTests(rng);
        std::cout << "PASS persistentsegmenttree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL persistentsegmenttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
