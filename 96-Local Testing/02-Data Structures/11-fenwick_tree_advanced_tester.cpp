#include "../../02-Data Structures/11-fenwick_tree_advanced.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    ulng seed = 20261008;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    string show(const mint &x) { return show(x.val()); }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    int steps() { return mode == "quick" ? 150 : mode == "full" ? 1500 : 4000; }
    vector<int> sizes() {
        if (mode == "quick") { return {0, 1, 2, 5, 8, 13}; }
        vector<int> s{0, 1, 2, 3, 4, 5, 7, 8, 9, 16, 17, 31, 64, 100};
        if (mode == "stress") { s.push_back(513); }
        return s;}

    template<typename T>
    void rangeAdd(Rng &rng, const string &name) {
        for (int n : sizes()) {
            context = name + " n=" + show(n) + " seed=" + show(seed);
            vector<T> a(n);
            for (T &x : a) { x = T(rnd(rng, -9, 9)); }
            FenwickRangeAdd<T> f(a), z(n);
            DualFenwick<T> d(a), dz(n);
            FenwickRangeArithmeticAdd<T> g(a), gz(n);
            vector<T> zero(n, T(0));
            for (int i = 0; i < n; ++i) { checkEqual(z.get(i), T(0), "size ctor"); checkEqual(dz.get(i), T(0), "dual size ctor"); checkEqual(gz.get(i), T(0), "arith size ctor"); }
            vector<T> b = a, c = a;
            for (int step = 0; step < steps(); ++step) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                if (l > r) { swap(l, r); }
                T x = T(rnd(rng, -50, 50)), s = T(rnd(rng, -7, 7));
                string at = "step=" + show(step) + " l=" + show(l) + " r=" + show(r);
                switch (rng() % 4) {
                    case 0:
                        f.add(l, r, x); d.add(l, r, x);
                        for (int i = l; i < r; ++i) { a[i] += x; b[i] += x; }
                        break;
                    case 1:
                        g.addProgression(l, r, x, s);
                        for (int i = l; i < r; ++i) { c[i] += x + s * T(i - l); }
                        break;
                    case 2:
                        g.add(l, r, x);
                        for (int i = l; i < r; ++i) { c[i] += x; }
                        break;
                    default: {
                        T sa = T(0), sc = T(0);
                        for (int i = l; i < r; ++i) { sa += a[i]; sc += c[i]; }
                        checkEqual(f.sum(l, r), sa, at + " sum");
                        checkEqual(g.sum(l, r), sc, at + " arith sum");}}
                if (n <= 17 || step % 50 == 0) {
                    T pa = T(0), pc = T(0);
                    for (int i = 0; i <= n; ++i) {
                        checkEqual(f.prefixSum(i), pa, at + " prefixSum(" + show(i) + ")");
                        checkEqual(g.prefixSum(i), pc, at + " arith prefixSum(" + show(i) + ")");
                        if (i < n) {
                            checkEqual(f.get(i), a[i], at + " get(" + show(i) + ")");
                            checkEqual(d.get(i), b[i], at + " dual get(" + show(i) + ")");
                            checkEqual(g.get(i), c[i], at + " arith get(" + show(i) + ")");
                            pa += a[i]; pc += c[i];}}}}}
        std::cout << "PASS " << name << " range add, dual, arithmetic progression\n";}

    void grids(Rng &rng) {
        vector<pair<int, int>> dims{{0, 0}, {0, 3}, {3, 0}, {1, 1}, {1, 7}, {5, 1}, {4, 4}, {5, 6}, {8, 9}};
        if (mode != "quick") { dims.push_back({17, 13}); dims.push_back({32, 33}); }
        for (auto [n, m] : dims) {
            context = "Fenwick2D n=" + show(n) + " m=" + show(m);
            vector<vector<lng>> a(n, vector<lng>(m)), b(n, vector<lng>(m, 0));
            for (auto &row : a) { for (lng &x : row) { x = rnd(rng, -9, 9); }}
            Fenwick2D<lng> f = n ? Fenwick2D<lng>(a) : Fenwick2D<lng>(n, m), z(n, m);
            Fenwick2DRangeAdd<lng> g(n, m);
            auto rect = [&](const vector<vector<lng>> &x, int i1, int j1, int i2, int j2) {
                lng s = 0;
                for (int i = i1; i < i2; ++i) { for (int j = j1; j < j2; ++j) { s += x[i][j]; }}
                return s;};
            for (int step = 0; step < steps() / 2; ++step) {
                int i1 = int(rnd(rng, 0, n)), i2 = int(rnd(rng, 0, n)), j1 = int(rnd(rng, 0, m)), j2 = int(rnd(rng, 0, m));
                if (i1 > i2) { swap(i1, i2); }
                if (j1 > j2) { swap(j1, j2); }
                lng x = rnd(rng, -20, 20);
                string at = "step=" + show(step) + " rect=" + show(i1) + "," + show(j1) + "," + show(i2) + "," + show(j2);
                if (rng() % 2 && n && m) {
                    int i = int(rnd(rng, 0, n - 1)), j = int(rnd(rng, 0, m - 1));
                    f.add(i, j, x); a[i][j] += x;}
                else {
                    g.add(i1, j1, i2, j2, x);
                    for (int i = i1; i < i2; ++i) { for (int j = j1; j < j2; ++j) { b[i][j] += x; }}}
                checkEqual(f.sum(i1, j1, i2, j2), rect(a, i1, j1, i2, j2), at + " sum");
                checkEqual(g.sum(i1, j1, i2, j2), rect(b, i1, j1, i2, j2), at + " range-add sum");
                if (n * m <= 81 && step % 5 == 0) {
                    for (int i = 0; i <= n; ++i) {
                        for (int j = 0; j <= m; ++j) {
                            checkEqual(f.prefixSum(i, j), rect(a, 0, 0, i, j), at + " prefixSum");
                            checkEqual(g.prefixSum(i, j), rect(b, 0, 0, i, j), at + " range-add prefixSum");
                            if (i < n && j < m) { checkEqual(f.get(i, j), a[i][j], at + " get"); checkEqual(z.get(i, j), lng(0), "size ctor"); }}}}}}
        vector<array<int, 3>> boxes{{0, 0, 0}, {0, 2, 2}, {2, 0, 2}, {2, 2, 0}, {1, 1, 1}, {3, 4, 5}, {5, 3, 4}};
        if (mode != "quick") { boxes.push_back({9, 8, 7}); boxes.push_back({16, 3, 17}); }
        for (auto [n, m, h] : boxes) {
            context = "Fenwick3D " + show(n) + "x" + show(m) + "x" + show(h);
            vector a(n, vector(m, vector<lng>(h)));
            for (auto &p : a) { for (auto &row : p) { for (lng &x : row) { x = rnd(rng, -9, 9); }}}
            Fenwick3D<lng> f = n && m ? Fenwick3D<lng>(a) : Fenwick3D<lng>(n, m, h), z(n, m, h);
            auto box = [&](int i1, int j1, int k1, int i2, int j2, int k2) {
                lng s = 0;
                for (int i = i1; i < i2; ++i) { for (int j = j1; j < j2; ++j) { for (int k = k1; k < k2; ++k) { s += a[i][j][k]; }}}
                return s;};
            for (int step = 0; step < steps() / 2; ++step) {
                array<int, 6> q{int(rnd(rng, 0, n)), int(rnd(rng, 0, m)), int(rnd(rng, 0, h)), int(rnd(rng, 0, n)), int(rnd(rng, 0, m)), int(rnd(rng, 0, h))};
                for (int t = 0; t < 3; ++t) {
                    if (q[t] > q[t + 3]) { swap(q[t], q[t + 3]); }}
                if (n && m && h) {
                    int i = int(rnd(rng, 0, n - 1)), j = int(rnd(rng, 0, m - 1)), k = int(rnd(rng, 0, h - 1));
                    lng x = rnd(rng, -9, 9);
                    f.add(i, j, k, x); a[i][j][k] += x;
                    checkEqual(f.get(i, j, k), a[i][j][k], "get");
                    checkEqual(z.get(i, j, k), lng(0), "size ctor");}
                checkEqual(f.sum(q[0], q[1], q[2], q[3], q[4], q[5]), box(q[0], q[1], q[2], q[3], q[4], q[5]), "sum step=" + show(step));
                checkEqual(f.prefixSum(q[3], q[4], q[5]), box(0, 0, 0, q[3], q[4], q[5]), "prefixSum step=" + show(step));}}
        std::cout << "PASS Fenwick2D, Fenwick2DRangeAdd, Fenwick3D\n";}

    void compressed(Rng &rng) {
        for (int round = 0; round < (mode == "quick" ? 5 : mode == "full" ? 40 : 200); ++round) {
            lng span = round % 3 == 0 ? 4 : round % 3 == 1 ? 1000 : lng(1e18);
            int k = int(rnd(rng, 0, 40));
            vector<pair<lng, lng>> pts(k);
            for (auto &[x, y] : pts) { x = rnd(rng, -span, span); y = rnd(rng, -span, span); }
            if (k) { pts.push_back(pts[0]); }
            context = "CompressedFenwick2D round=" + show(round) + " k=" + show(k);
            CompressedFenwick2D<lng> f(pts);
            map<pair<lng, lng>, lng> w;
            auto pick = [&]() { return pts.empty() || rng() % 4 == 0 ? rnd(rng, -span, span) : (rng() % 2 ? pts[rng() % pts.size()].first : pts[rng() % pts.size()].second); };
            for (int step = 0; step < steps() / 3; ++step) {
                if (!pts.empty() && rng() % 2) {
                    auto p = pts[rng() % pts.size()];
                    lng x = rnd(rng, -9, 9);
                    f.add(p.first, p.second, x); w[p] += x;}
                lng x1 = pick(), x2 = pick(), y1 = pick(), y2 = pick();
                if (x1 > x2) { swap(x1, x2); }
                if (y1 > y2) { swap(y1, y2); }
                lng want = 0, pre = 0;
                for (auto &[p, x] : w) {
                    if (x1 <= p.first && p.first < x2 && y1 <= p.second && p.second < y2) { want += x; }
                    if (p.first < x2 && p.second < y2) { pre += x; }}
                checkEqual(f.sum(x1, y1, x2, y2), want, "sum step=" + show(step));
                checkEqual(f.prefixSum(x2, y2), pre, "prefixSum step=" + show(step));}}
        std::cout << "PASS CompressedFenwick2D\n";}

    void prefixMonoid(Rng &rng) {
        auto mx = [](lng a, lng b) { return max(a, b); };
        auto g = [](lng a, lng b) { return gcd(a, b); };
        for (int n : sizes()) {
            context = "FenwickPrefixMonoid n=" + show(n);
            vector<lng> a(n), c(n);
            for (lng &x : a) { x = rnd(rng, -50, 50); }
            for (lng &x : c) { x = rnd(rng, 0, 60); }
            FenwickPrefixMonoid f(a, std::numeric_limits<lng>::min(), mx), z(n, std::numeric_limits<lng>::min(), mx);
            FenwickPrefixMonoid h(c, lng(0), g);
            for (int step = 0; step < steps(); ++step) {
                if (n) {
                    int i = int(rnd(rng, 0, n - 1));
                    lng x = rnd(rng, -60, 60), y = rnd(rng, 0, 60);
                    f.apply(i, x); a[i] = max(a[i], x);
                    h.apply(i, y); c[i] = gcd(c[i], y);}
                int r = int(rnd(rng, 0, n));
                lng m = std::numeric_limits<lng>::min(), gg = 0;
                for (int i = 0; i < r; ++i) { m = max(m, a[i]); gg = gcd(gg, c[i]); }
                checkEqual(f.prefix(r), m, "max prefix(" + show(r) + ")");
                checkEqual(h.prefix(r), gg, "gcd prefix(" + show(r) + ")");
                checkEqual(z.prefix(r), std::numeric_limits<lng>::min(), "identity prefix");}}
        std::cout << "PASS FenwickPrefixMonoid\n";}

    void bitset01(Rng &rng) {
        vector<int> ns{0, 1, 2, 63, 64, 65, 127, 128, 129, 200};
        if (mode != "quick") { ns.push_back(1000); ns.push_back(4096); }
        if (mode == "stress") { ns.push_back(20000); }
        for (int n : ns) {
            for (int density : {0, 1, 2, 3}) {
                context = "Fenwick01 n=" + show(n) + " density=" + show(density);
                vector<bool> a(n);
                for (int i = 0; i < n; ++i) { a[i] = density == 3 || (density && rng() % (4 - density) == 0); }
                FenwickSet f(a);
                Fenwick01 e(n);
                checkEqual(e.size(), 0, "empty size");
                auto verify = [&](const string &at) {
                    vector<int> elems;
                    for (int i = 0; i < n; ++i) {
                        if (a[i]) { elems.push_back(i); }}
                    checkEqual(f.size(), int(elems.size()), at + " size");
                    int sweep = n <= 300 ? n : 300;
                    for (int t = 0; t <= sweep; ++t) {
                        int i = n <= 300 ? t : int(rnd(rng, 0, n));
                        int rank = int(std::lower_bound(elems.begin(), elems.end(), i) - elems.begin());
                        checkEqual(f.rank(i), rank, at + " rank(" + show(i) + ")");
                        checkEqual(f.next(i), rank < int(elems.size()) ? elems[rank] : n, at + " next(" + show(i) + ")");
                        int up = int(std::upper_bound(elems.begin(), elems.end(), i - 1) - elems.begin());
                        checkEqual(f.prev(i - 1), up ? elems[up - 1] : -1, at + " prev(" + show(i - 1) + ")");
                        if (i < n) { checkEqual(f.get(i), bool(a[i]), at + " get(" + show(i) + ")"); checkEqual(f.contains(i), bool(a[i]), at + " contains"); }
                        int k = int(rnd(rng, 0, int(elems.size()) + 1));
                        checkEqual(f.kth(k), k < int(elems.size()) ? elems[k] : n, at + " kth(" + show(k) + ")");}
                    int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                    if (l > r) { swap(l, r); }
                    checkEqual(f.sum(l, r), int(std::count(a.begin() + l, a.begin() + r, true)), at + " sum");};
                verify("build");
                for (int step = 0; step < (n ? steps() / 10 : 1); ++step) {
                    if (n) {
                        int i = int(rnd(rng, 0, n - 1));
                        switch (rng() % 3) {
                            case 0: checkEqual(f.insert(i), !a[i], "insert(" + show(i) + ")"); a[i] = true; break;
                            case 1: checkEqual(f.erase(i), bool(a[i]), "erase(" + show(i) + ")"); a[i] = false; break;
                            default: f.add(i, a[i] ? -1 : 1); a[i] = !a[i];}}
                    if (step % 8 == 0) { verify("step=" + show(step)); }}}}
        std::cout << "PASS Fenwick01 / FenwickSet\n";}

    void invalid(const string &name) {
        FenwickRangeAdd<lng> f(3);
        DualFenwick<lng> d(3);
        FenwickRangeArithmeticAdd<lng> g(3);
        Fenwick2D<lng> t(2, 2);
        Fenwick2DRangeAdd<lng> u(2, 2);
        Fenwick3D<lng> b(2, 2, 2);
        CompressedFenwick2D<lng> c({{1, 1}});
        FenwickPrefixMonoid p(3, lng(0), [](lng x, lng y) { return max(x, y); });
        Fenwick01 s(70);
        if (name == "range-add-negative-size") { FenwickRangeAdd<lng> x(-1); }
        else if (name == "range-add-reversed") { f.add(2, 1, 1); }
        else if (name == "range-add-sum-end") { (void)f.sum(0, 4); }
        else if (name == "range-add-get-end") { (void)f.get(3); }
        else if (name == "dual-add-end") { d.add(0, 4, 1); }
        else if (name == "dual-get-negative") { (void)d.get(-1); }
        else if (name == "arith-reversed") { g.addProgression(2, 1, 1, 1); }
        else if (name == "arith-prefix-end") { (void)g.prefixSum(4); }
        else if (name == "grid-oversize") { Fenwick2D<lng> x(1 << 16, 1 << 16); }
        else if (name == "box-oversize") { Fenwick3D<lng> x(1 << 21, 1 << 21, 1 << 22); }
        else if (name == "grid-add-end") { t.add(2, 0, 1); }
        else if (name == "grid-sum-reversed") { (void)t.sum(1, 0, 0, 1); }
        else if (name == "grid-prefix-end") { (void)t.prefixSum(0, 3); }
        else if (name == "grid-ragged") { Fenwick2D<lng> x(vector<vector<lng>>{{1, 2}, {3}}); }
        else if (name == "grid-range-add-end") { u.add(0, 0, 3, 1, 1); }
        else if (name == "box-add-end") { b.add(0, 0, 2, 1); }
        else if (name == "box-prefix-negative") { (void)b.prefixSum(-1, 0, 0); }
        else if (name == "compressed-unregistered-x") { c.add(2, 1, 1); }
        else if (name == "compressed-unregistered-y") { c.add(1, 2, 1); }
        else if (name == "compressed-reversed") { (void)c.sum(2, 0, 1, 1); }
        else if (name == "monoid-apply-end") { p.apply(3, 1); }
        else if (name == "monoid-prefix-end") { (void)p.prefix(4); }
        else if (name == "bits-negative-size") { Fenwick01 x(-1); }
        else if (name == "bits-add-twice") { s.add(5, 1); s.add(5, 1); }
        else if (name == "bits-remove-absent") { s.add(5, -1); }
        else if (name == "bits-add-two") { s.add(5, 2); }
        else if (name == "bits-get-end") { (void)s.get(70); }
        else if (name == "bits-rank-end") { (void)s.rank(71); }
        else if (name == "bits-prev-low") { (void)s.prev(-2); }
        else if (name == "bits-next-high") { (void)s.next(71); }
        else if (name == "bits-kth-negative") { (void)s.kth(-1); }
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
        std::cout << "fenwick_tree_advanced seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        rangeAdd<lng>(rng, "lng");
        rangeAdd<mint>(rng, "mint");
        grids(rng);
        compressed(rng);
        prefixMonoid(rng);
        bitset01(rng);
        std::cout << "PASS fenwick_tree_advanced checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL fenwick_tree_advanced seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
