#include "../../02-Data Structures/13-dynamicsegmenttree.hpp"
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
    int steps() { return mode == "quick" ? 120 : mode == "full" ? 900 : 1400; }
    vector<pair<lng, lng>> universes() {
        vector<pair<lng, lng>> u{{0, 0}, {0, 1}, {0, 2}, {-7, 9}, {0, 64}, {-3, 100}, {lng(-1) << 61, lng(1) << 61}, {(lng(-1) << 61) + 5, lng(1) << 61}, {std::numeric_limits<lng>::min(), std::numeric_limits<lng>::min() + (lng(1) << 62)}, {std::numeric_limits<lng>::max() - (lng(1) << 62), std::numeric_limits<lng>::max()}};
        if (mode == "quick") { u = {{0, 1}, {-7, 9}, {lng(-1) << 61, lng(1) << 61}}; }
        return u;}

    struct Sum {
        using M = RangeAddRangeSum<lng>;
        using K = tuple<lng, lng>;
        static M::S leaf(Rng &rng, K &k) { lng v = rnd(rng, -9, 9); k = {v, 1}; return M::leaf(v); }
        static K key(const M::S &s) { return {s.sum, s.len}; }
        static K e() { return {0, 0}; }
        static K op(const K &a, const K &b) { return {std::get<0>(a) + std::get<0>(b), std::get<1>(a) + std::get<1>(b)}; }
        static bool ok(const K &k, lng t) { return std::get<1>(k) <= t; }
        static bool small(const K &k) { return std::abs(std::get<0>(k)) <= 1000000 && std::get<1>(k) <= 1000000; }
    };
    struct Composite {
        using M = RangeSetRangeComposite<mint>;
        using K = tuple<lng, lng, lng>;
        static M::S leaf(Rng &rng, K &k) { lng a = rnd(rng, 0, P - 1), b = rnd(rng, 0, P - 1); k = {a, b, 1}; return M::leaf(mint(a), mint(b)); }
        static K key(const M::S &s) { return {lng(s.a.val()), lng(s.b.val()), s.len}; }
        static K e() { return {1, 0, 0}; }
        static K op(const K &p, const K &q) { return {std::get<0>(q) * std::get<0>(p) % P, (std::get<0>(q) * std::get<1>(p) + std::get<1>(q)) % P, std::get<2>(p) + std::get<2>(q)}; }
        static bool ok(const K &k, lng t) { return std::get<2>(k) <= t; }
        static bool small(const K &k) { return std::get<2>(k) <= 1000000; }
    };
    template<typename C>
    typename C::K expect(const map<lng, typename C::K> &m, lng l, lng r) {
        typename C::K res = C::e();
        for (auto it = m.lower_bound(l); it != m.end() && it->first < r; ++it) { res = C::op(res, it->second); }
        return res;}

    template<typename C>
    void plain(Rng &rng, const string &name) {
        using M = typename C::M;
        using S = typename M::S;
        using K = typename C::K;
        for (auto [lo, hi] : universes()) {
            context = name + " universe=[" + show(lo) + "," + show(hi) + ") seed=" + show(seed);
            DynamicSegmentTree<M> t(lo, hi);
            t.reserve(64);
            array<int, 3> roots{0, 0, 0};
            array<map<lng, K>, 3> ref;
            bool small = hi - lo <= 100;
            auto randomKey = [&]() { return hi - lo <= 100 || rng() % 3 ? rnd(rng, lo, hi - 1) : (rng() % 2 ? lo + rnd(rng, 0, 40) : hi - 1 - rnd(rng, 0, 40)); };
            auto verify = [&](int w, const string &at) {
                checkEqual(C::key(t.allProd(roots[w])), expect<C>(ref[w], lo, hi), at + " allProd");
                checkEqual(roots[w] == 0, ref[w].empty(), at + " empty tree is handle 0");
                vector<pair<lng, S>> seen;
                t.enumerate(roots[w], [&](lng i, const S &x) { seen.push_back({i, x}); });
                checkEqual(int(seen.size()), int(ref[w].size()), at + " enumerate size");
                int idx = 0;
                for (auto &[i, v] : ref[w]) {
                    checkEqual(seen[idx].first, i, at + " enumerate key");
                    checkEqual(C::key(seen[idx].second), expect<C>(ref[w], i, i + 1), at + " enumerate value");
                    ++idx;}
                if (small) {
                    for (lng l = lo; l <= hi; ++l) {
                        for (lng r = l; r <= hi; ++r) { checkEqual(C::key(t.prod(roots[w], l, r)), expect<C>(ref[w], l, r), at + " prod(" + show(l) + "," + show(r) + ")"); }}}
                else {
                    for (int q = 0; q < 10; ++q) {
                        lng l = randomKey(), r = randomKey();
                        if (l > r) { swap(l, r); }
                        checkEqual(C::key(t.prod(roots[w], l, r + 1)), expect<C>(ref[w], l, r + 1), at + " prod");}}};
            for (int w = 0; w < 3; ++w) { verify(w, "empty"); }
            int allocated = 0;
            for (int step = 0; step < (hi > lo ? steps() : 3); ++step) {
                int w = int(rng() % 3);
                string at = "step=" + show(step) + " root=" + show(w);
                if (hi == lo) { checkEqual(t.maxRight(roots[w], lo, [](const S &) { return true; }), lo, "empty maxRight"); checkEqual(t.minLeft(roots[w], lo, [](const S &) { return true; }), lo, "empty minLeft"); continue; }
                lng i = randomKey();
                switch (rng() % 11) {
                    case 0: case 1: {
                        K k;
                        S x = C::leaf(rng, k);
                        roots[w] = t.set(roots[w], i, x); ref[w][i] = k;
                        ++allocated;
                        break;}
                    case 2: {
                        K k;
                        S x = C::leaf(rng, k);
                        roots[w] = t.apply(roots[w], i, x);
                        ref[w][i] = ref[w].count(i) ? C::op(ref[w][i], k) : k;
                        ++allocated;
                        break;}
                    case 9: {
                        int u = (w + 1) % 3;
                        if (!C::small(expect<C>(ref[u], lo, hi))) { break; }
                        roots[w] = t.set(roots[w], i, t.allProd(roots[u]));
                        ref[w][i] = expect<C>(ref[u], lo, hi);
                        ++allocated;
                        break;}
                    case 3:
                        checkEqual(C::key(t.get(roots[w], i)), expect<C>(ref[w], i, i + 1), at + " get(" + show(i) + ")");
                        break;
                    case 4: {
                        lng th = rnd(rng, 0, 6), want = hi;
                        for (auto it = ref[w].lower_bound(i); it != ref[w].end(); ++it) {
                            if (!C::ok(expect<C>(ref[w], i, it->first + 1), th)) { want = it->first; break; }}
                        checkEqual(t.maxRight(roots[w], i, [&](const S &s) { return C::ok(C::key(s), th); }), want, at + " maxRight(" + show(i) + ",t=" + show(th) + ")");
                        break;}
                    case 5: {
                        lng th = rnd(rng, 0, 6), r = i + 1, want = lo;
                        for (auto it = ref[w].lower_bound(r); it != ref[w].begin();) {
                            --it;
                            if (!C::ok(expect<C>(ref[w], it->first, r), th)) { want = it->first + 1; break; }}
                        checkEqual(t.minLeft(roots[w], r, [&](const S &s) { return C::ok(C::key(s), th); }), want, at + " minLeft(" + show(r) + ",t=" + show(th) + ")");
                        break;}
                    case 6: {
                        int u = (w + 1) % 3;
                        roots[w] = t.meld(roots[w], roots[u]); roots[u] = 0;
                        for (auto &[k, v] : ref[u]) { ref[w][k] = ref[w].count(k) ? C::op(ref[w][k], v) : v; }
                        ref[u].clear();
                        break;}
                    case 7: {
                        int u = (w + 2) % 3;
                        lng cutAt = rng() % 8 == 0 ? (rng() % 2 ? lo : hi) : i;
                        auto [a, b] = t.split(roots[w], cutAt);
                        roots[w] = a;
                        roots[u] = t.meld(roots[u], b);
                        allocated += 64;
                        for (auto it = ref[w].lower_bound(cutAt); it != ref[w].end(); it = ref[w].erase(it)) { ref[u][it->first] = ref[u].count(it->first) ? C::op(ref[u][it->first], it->second) : it->second; }
                        break;}
                    default:
                        checkEqual(C::key(t.prod(roots[w], lo, i + 1)), expect<C>(ref[w], lo, i + 1), at + " prefix prod");}
                checkEqual(t.nodeCount() <= 64 * allocated + 3, true, at + " live nodes bounded");
                checkEqual(int(t.t.size()) - 1 - int(t.spare.size()), t.nodeCount(), at + " nodeCount");
                if (small) { for (int v = 0; v < 3; ++v) { verify(v, at + " root=" + show(v)); }}
                else if (step % 25 == 0) { verify(w, at); }}}
        std::cout << "PASS DynamicSegmentTree " << name << " (set, get, apply, prod, allProd, searches, enumerate, meld, split)\n";}

    void persistent(Rng &rng) {
        using C = Composite;
        using M = C::M;
        for (auto [lo, hi] : universes()) {
            if (hi == lo) { continue; }
            context = "persistent universe=[" + show(lo) + "," + show(hi) + ")";
            DynamicSegmentTree<M, true> t(lo, hi);
            vector<pair<int, map<lng, C::K>>> versions{{0, {}}};
            for (int step = 0; step < steps() / 3; ++step) {
                auto [root, ref] = versions[rng() % versions.size()];
                lng i = hi - lo <= 100 || rng() % 2 ? rnd(rng, lo, hi - 1) : lo + rnd(rng, 0, 30);
                C::K k;
                M::S x = C::leaf(rng, k);
                if (rng() % 2) { root = t.set(root, i, x); ref[i] = k; }
                else { root = t.apply(root, i, x); ref[i] = ref.count(i) ? C::op(ref[i], k) : k; }
                versions.push_back({root, ref});
                for (int q = 0; q < 3; ++q) {
                    auto &[vr, vm] = versions[rng() % versions.size()];
                    lng l = rnd(rng, lo, hi), r = rnd(rng, lo, hi);
                    if (l > r) { swap(l, r); }
                    checkEqual(C::key(t.prod(vr, l, r)), expect<C>(vm, l, r), "version prod step=" + show(step));
                    checkEqual(C::key(t.allProd(vr)), expect<C>(vm, lo, hi), "version allProd step=" + show(step));
                    lng k = rnd(rng, lo, hi - 1);
                    checkEqual(C::key(t.get(vr, k)), expect<C>(vm, k, k + 1), "version get step=" + show(step));
                    int count = 0;
                    t.enumerate(vr, [&](lng, const M::S &) { ++count; });
                    checkEqual(count, int(vm.size()), "version enumerate step=" + show(step));}}}
        std::cout << "PASS persistent path copying keeps every version\n";}

    void lazySmall(Rng &rng) {
        using A = RangeAffineRangeMinMaxArg<lng>;
        using S = A::S;
        for (auto [lo, hi] : universes()) {
            if (hi - lo > 100) { continue; }
            for (lng c : {0, 3, -2}) {
                context = "DynamicLazySegmentTree minmaxarg universe=[" + show(lo) + "," + show(hi) + ") c=" + show(c);
                auto fill = [c](lng l, lng r) { return S{c * (r - l), c, c, r - l, l, l, l}; };
                DynamicLazySegmentTree<A, decltype(fill)> t(lo, hi, fill);
                t.reserve(256);
                int n = int(hi - lo);
                vector<lng> a(n, c);
                auto expect = [&](lng l, lng r) {
                    if (l == r) { return tuple(lng(0), lng(0), lng(0), lng(0), lng(-1), lng(-1)); }
                    lng s = 0, mx = a[l - lo], mn = a[l - lo], mxi = l, mni = l;
                    for (lng i = l; i < r; ++i) {
                        s += a[i - lo];
                        if (a[i - lo] > mx) { mx = a[i - lo]; mxi = i; }
                        if (a[i - lo] < mn) { mn = a[i - lo]; mni = i; }}
                    return tuple(s, mx, mn, r - l, mxi, mni);};
                auto key = [](const S &s) { return s.len ? tuple(s.sum, s.mx, s.mn, s.len, s.mxi, s.mni) : tuple(lng(0), lng(0), lng(0), lng(0), lng(-1), lng(-1)); };
                auto ok = [](const auto &k, lng th) { return std::get<3>(k) == 0 || (std::get<2>(k) >= th && std::get<1>(k) <= th + 6); };
                checkEqual(key(t.allProd()), expect(lo, hi), "fill allProd");
                for (int step = 0; step < (n ? steps() / 2 : 2); ++step) {
                    lng l = rnd(rng, lo, hi), r = rnd(rng, lo, hi);
                    if (l > r) { swap(l, r); }
                    string at = "step=" + show(step) + " l=" + show(l) + " r=" + show(r);
                    switch (rng() % 7) {
                        case 0: case 1: {
                            A::F f{rnd(rng, -1, 1), rnd(rng, -3, 3)};
                            t.apply(l, r, f);
                            for (lng i = l; i < r; ++i) { a[i - lo] = f.a * a[i - lo] + f.b; }
                            break;}
                        case 2:
                            if (n) { lng i = rnd(rng, lo, hi - 1), v = rnd(rng, -5, 5); t.set(i, A::leaf(i, v)); a[i - lo] = v; }
                            break;
                        case 3:
                            if (n) { lng i = rnd(rng, lo, hi - 1); checkEqual(key(t.get(i)), expect(i, i + 1), at + " get(" + show(i) + ")"); }
                            break;
                        case 4: {
                            lng th = rnd(rng, -6, 6), want = l;
                            if (!ok(expect(l, l), th)) { break; }
                            while (want < hi && ok(expect(l, want + 1), th)) { ++want; }
                            checkEqual(t.maxRight(l, [&](const S &s) { return ok(key(s), th); }), want, at + " maxRight t=" + show(th));
                            break;}
                        case 5: {
                            lng th = rnd(rng, -6, 6), want = r;
                            while (want > lo && ok(expect(want - 1, r), th)) { --want; }
                            checkEqual(t.minLeft(r, [&](const S &s) { return ok(key(s), th); }), want, at + " minLeft t=" + show(th));
                            break;}
                        default:
                            checkEqual(key(t.prod(l, r)), expect(l, r), at + " prod");}
                    checkEqual(key(t.allProd()), expect(lo, hi), at + " allProd");
                    if (n <= 20) {
                        for (lng x = lo; x <= hi; ++x) {
                            for (lng y = x; y <= hi; ++y) { checkEqual(key(t.prod(x, y)), expect(x, y), at + " prod(" + show(x) + "," + show(y) + ")"); }}}}}}
        std::cout << "PASS DynamicLazySegmentTree on small universes\n";}

    void lazyLarge(Rng &rng) {
        using A = RangeAddRangeSum<lng>;
        using S = A::S;
        for (int round = 0; round < (mode == "quick" ? 2 : 8); ++round) {
            lng lo = lng(-1) << 40, hi = (lng(1) << 40) + round;
            context = "DynamicLazySegmentTree sum large round=" + show(round);
            int q = steps() / 4;
            vector<array<lng, 3>> ops(q);
            vector<lng> cuts{lo, hi};
            for (auto &op : ops) {
                lng l = rnd(rng, lo, hi), r = rnd(rng, lo, hi);
                if (l > r) { swap(l, r); }
                op = {l, r, rnd(rng, 0, 9)};
                cuts.push_back(l); cuts.push_back(r);}
            sort(cuts.begin(), cuts.end());
            cuts.erase(unique(cuts.begin(), cuts.end()), cuts.end());
            int segs = int(cuts.size()) - 1;
            vector<lng> val(segs, 1);
            auto fill = [](lng l, lng r) { return S{r - l, r - l}; };
            DynamicLazySegmentTree<A, decltype(fill)> t(lo, hi, fill);
            auto segOf = [&](lng x) { return int(lower_bound(cuts.begin(), cuts.end(), x) - cuts.begin()); };
            auto segAt = [&](lng x) { return int(std::upper_bound(cuts.begin(), cuts.end(), x) - cuts.begin()) - 1; };
            auto expect = [&](lng l, lng r) {
                lng s = 0;
                for (int k = segAt(l); k < segs && cuts[k] < r; ++k) { s += val[k] * (min(r, cuts[k + 1]) - max(l, cuts[k])); }
                return tuple(s, r - l);};
            for (int step = 0; step < q; ++step) {
                auto [l, r, x] = ops[step];
                string at = "step=" + show(step);
                if (rng() % 2) {
                    t.apply(l, r, x);
                    for (int k = segOf(l); k < segOf(r); ++k) { val[k] += x; }}
                else { checkEqual(tuple(t.prod(l, r).sum, t.prod(l, r).len), expect(l, r), at + " prod"); }
                if (step % 5 == 0) {
                    lng cap = rnd(rng, 0, 400), want = hi, acc = 0;
                    for (int k = segAt(l); k < segs; ++k) {
                        lng len = cuts[k + 1] - max(l, cuts[k]);
                        if (acc + val[k] * len > cap) { want = max(l, cuts[k]) + (cap - acc) / val[k]; break; }
                        acc += val[k] * len;}
                    checkEqual(t.maxRight(l, [&](const S &s) { return s.sum <= cap; }), want, at + " maxRight cap=" + show(cap));
                    want = lo; acc = 0;
                    for (int k = r > lo ? segAt(r - 1) : -1; k >= 0; --k) {
                        lng len = min(r, cuts[k + 1]) - cuts[k];
                        if (acc + val[k] * len > cap) { want = min(r, cuts[k + 1]) - (cap - acc) / val[k]; break; }
                        acc += val[k] * len;}
                    checkEqual(t.minLeft(r, [&](const S &s) { return s.sum <= cap; }), want, at + " minLeft cap=" + show(cap));}
                checkEqual(t.allProd().sum, std::get<0>(expect(lo, hi)), at + " allProd");}
            checkEqual(t.nodeCount() <= 12 * 64 * q + 2, true, "node count bounded");}
        std::cout << "PASS DynamicLazySegmentTree on a 2^41 universe\n";}

    void lazyCompositeWide(Rng &rng) {
        using A = RangeSetRangeComposite<mint>;
        using K = Composite::K;
        auto power = [](K p, lng k) {
            K res = Composite::e();
            for (; k; k >>= 1) {
                if (k & 1) { res = Composite::op(res, p); }
                if (k > 1) { p = Composite::op(p, p); }}
            return res;};
        for (auto [lo, hi] : vector<pair<lng, lng>>{{0, lng(1) << 62}, {std::numeric_limits<lng>::min(), std::numeric_limits<lng>::min() + (lng(1) << 62)}}) {
            context = "DynamicLazySegmentTree composite wide universe=[" + show(lo) + "," + show(hi) + ") seed=" + show(seed);
            auto fill = [](lng l, lng r) { return A::S{mint(1), mint(0), r - l}; };
            DynamicLazySegmentTree<A, decltype(fill)> t(lo, hi, fill);
            map<lng, pair<lng, lng>> piece{{lo, {1, 0}}};
            auto cut = [&](lng x) {
                if (x == hi) { return; }
                auto it = std::prev(piece.upper_bound(x));
                if (it->first != x) { piece[x] = it->second; }};
            auto expect = [&](lng l, lng r) {
                K res = Composite::e();
                for (auto it = std::prev(piece.upper_bound(l)); it != piece.end() && it->first < r; ++it) {
                    lng b = std::next(it) == piece.end() ? hi : std::next(it)->first;
                    res = Composite::op(res, power({it->second.first, it->second.second, 1}, min(r, b) - max(l, it->first)));}
                return res;};
            auto randomPoint = [&]() {
                ulng span = ulng(hi) - ulng(lo);
                return rng() % 4 ? lng(ulng(lo) + rng() % (span + 1)) : (rng() % 2 ? lo + rnd(rng, 0, 3) : hi - rnd(rng, 0, 3));};
            int q = mode == "quick" ? 40 : steps() / 3;
            for (int step = 0; step < q; ++step) {
                lng l = step == 0 ? lo : randomPoint(), r = step == 0 ? hi : randomPoint();
                if (l > r) { swap(l, r); }
                string at = "step=" + show(step) + " l=" + show(l) + " r=" + show(r);
                if (step == 0 || rng() % 2) {
                    lng a = rnd(rng, 0, P - 1), b = rnd(rng, 0, P - 1);
                    t.apply(l, r, {true, mint(a), mint(b)});
                    if (l < r) {
                        cut(l); cut(r);
                        piece.erase(piece.lower_bound(l), piece.lower_bound(r));
                        piece[l] = {a, b};}}
                else { checkEqual(Composite::key(t.prod(l, r)), expect(l, r), at + " prod"); }
                checkEqual(Composite::key(t.allProd()), expect(lo, hi), at + " allProd");}}
        std::cout << "PASS DynamicLazySegmentTree composite preset on 2^62 universes\n";}

    void dual(Rng &rng) {
        using A = RangeAffineRangeSum<mint>;
        for (auto [lo, hi] : universes()) {
            context = "DynamicDualSegmentTree universe=[" + show(lo) + "," + show(hi) + ")";
            DynamicDualSegmentTree<A> t(lo, hi);
            t.reserve(256);
            bool small = hi - lo <= 100;
            int q = small ? steps() / 2 : steps() / 4;
            vector<pair<lng, lng>> ranges(q);
            vector<lng> cuts{lo, hi};
            for (auto &[l, r] : ranges) {
                l = rnd(rng, lo, hi); r = rnd(rng, lo, hi);
                if (l > r) { swap(l, r); }
                cuts.push_back(l); cuts.push_back(r);}
            sort(cuts.begin(), cuts.end());
            cuts.erase(unique(cuts.begin(), cuts.end()), cuts.end());
            vector<pair<lng, lng>> val(cuts.size(), {1, 0});
            auto segOf = [&](lng x) { return int(lower_bound(cuts.begin(), cuts.end(), x) - cuts.begin()); };
            for (int step = 0; step < q; ++step) {
                auto [l, r] = ranges[step];
                lng fa = rnd(rng, 0, P - 1), fb = rnd(rng, 0, P - 1);
                t.apply(l, r, {mint(fa), mint(fb)});
                for (int k = segOf(l); k < segOf(r); ++k) { val[k] = {fa * val[k].first % P, (fa * val[k].second + fb) % P}; }
                for (int k = 0; k < 4; ++k) {
                    lng i = hi == lo ? lo : rnd(rng, lo, hi - 1);
                    if (hi == lo) { break; }
                    A::F g = t.get(i);
                    checkEqual(pair<lng, lng>(g.a.val(), g.b.val()), val[segOf(i + 1) - 1], "step=" + show(step) + " get(" + show(i) + ")");}
                if (small) {
                    for (lng i = lo; i < hi; ++i) {
                        A::F g = t.get(i);
                        checkEqual(pair<lng, lng>(g.a.val(), g.b.val()), val[segOf(i + 1) - 1], "step=" + show(step) + " sweep get(" + show(i) + ")");}}}
            checkEqual(t.nodeCount() <= 4 * 64 * q + 1, true, "node count bounded");}
        std::cout << "PASS DynamicDualSegmentTree\n";}

    void invalid(const string &name) {
        using M = RangeAddRangeSum<lng>;
        DynamicSegmentTree<M> t(-2, 5);
        auto fill = [](lng l, lng r) { return M::S{0, r - l}; };
        DynamicLazySegmentTree<M, decltype(fill)> z(-2, 5, fill);
        DynamicDualSegmentTree<M> d(-2, 5);
        auto yes = [](const M::S &) { return true; };
        auto no = [](const M::S &) { return false; };
        int r = t.set(0, 1, M::leaf(1));
        if (name == "reversed-universe") { DynamicSegmentTree<M> x(3, 2); }
        else if (name == "wide-universe") { DynamicSegmentTree<M> x(lng(-1) << 62, (lng(1) << 62) + 1); }
        else if (name == "set-low") { (void)t.set(r, -3, M::leaf(1)); }
        else if (name == "apply-high") { (void)t.apply(r, 5, M::leaf(1)); }
        else if (name == "get-high") { (void)t.get(r, 5); }
        else if (name == "prod-reversed") { (void)t.prod(r, 3, 2); }
        else if (name == "prod-high") { (void)t.prod(r, 0, 6); }
        else if (name == "max-right-low") { (void)t.maxRight(r, -3, yes); }
        else if (name == "max-right-identity") { (void)t.maxRight(r, 0, no); }
        else if (name == "min-left-high") { (void)t.minLeft(r, 6, yes); }
        else if (name == "min-left-identity") { (void)t.minLeft(r, 0, no); }
        else if (name == "split-high") { (void)t.split(r, 6); }
        else if (name == "meld-self") { (void)t.meld(r, r); }
        else if (name == "lazy-reversed-universe") { DynamicLazySegmentTree<M, decltype(fill)> x(1, 0, fill); }
        else if (name == "lazy-set-high") { z.set(5, M::leaf(1)); }
        else if (name == "lazy-get-low") { (void)z.get(-3); }
        else if (name == "lazy-apply-reversed") { z.apply(2, 1, 1); }
        else if (name == "lazy-prod-high") { (void)z.prod(0, 6); }
        else if (name == "lazy-max-right-identity") { (void)z.maxRight(0, no); }
        else if (name == "lazy-min-left-low") { (void)z.minLeft(-3, yes); }
        else if (name == "dual-reversed-universe") { DynamicDualSegmentTree<M> x(1, 0); }
        else if (name == "dual-get-high") { (void)d.get(5); }
        else if (name == "dual-apply-high") { d.apply(0, 6, 1); }
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
        std::cout << "dynamicsegmenttree seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        plain<Sum>(rng, "sum");
        plain<Composite>(rng, "composite");
        persistent(rng);
        lazySmall(rng);
        lazyLarge(rng);
        lazyCompositeWide(rng);
        dual(rng);
        std::cout << "PASS dynamicsegmenttree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL dynamicsegmenttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
