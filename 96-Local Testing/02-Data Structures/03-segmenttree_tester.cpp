#include "../../02-Data Structures/03-segmenttree.hpp"
#include "../../02-Data Structures/00-monoids.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    int checks = 0;
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &a) {
        string s = "[";
        for (int i = 0; i < int(a.size()); ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename T, size_t K> string show(const array<T, K> &a) { return show(vector<T>(a.begin(), a.end())); }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (got != want) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    struct Add { using S = lng; static S op(S a, S b) { return a + b; } static S e() { return 0; } };
    struct Join { string operator()(const string &a, const string &b) const { return a + b; } };
    using Tree = SegmentTree<Add>;
    // Runtime state through a member and through a capturing lambda.
    struct ModSum { lng mod; using S = lng; S op(S a, S b) const { return (a + b) % mod; } S e() const { return 0; } };

    void numeric(const vector<lng> &a, bool searches) {
        int n = int(a.size()); context = "array=" + show(a);
        Tree t(a), empty(n);
        checkEqual(empty.allProd(), 0, "identity-filled/allProd");
        lng total = 0;
        for (int l = 0; l <= n; ++l) {
            lng sum = 0;
            for (int r = l; r <= n; ++r) {
                checkEqual(t.prod(l, r), sum, "prod(" + show(l) + "," + show(r) + ")");
                if (r < n) { sum += a[r]; }}
            if (l < n) { checkEqual(t.get(l), a[l], "get(" + show(l) + ")"); total += a[l]; }}
        checkEqual(t.allProd(), total, "allProd");
        checkEqual(show(t.values()), show(a), "values");
        if (n) {
            lng delta = a[n - 1] >= 0 ? -1 : 1; Tree u = t; u.apply(n - 1, delta);
            checkEqual(u.get(n - 1), a[n - 1] + delta, "apply"); checkEqual(u.allProd(), total + delta, "apply aggregate");}
        if (searches) {
            for (lng cap = 0; cap <= total + 1; ++cap) {
                auto pred = [cap](lng x) { return x <= cap; };
                for (int l = 0; l <= n; ++l) {
                    int r = l; lng sum = 0;
                    while (r < n && sum + a[r] <= cap) { sum += a[r++]; }
                    checkEqual(t.maxRight(l, pred), r, "maxRight(" + show(l) + ",cap=" + show(cap) + ")");}
                for (int r = 0; r <= n; ++r) {
                    int l = r; lng sum = 0;
                    while (l && a[l - 1] + sum <= cap) { sum += a[--l]; }
                    checkEqual(t.minLeft(r, pred), l, "minLeft(" + show(r) + ",cap=" + show(cap) + ")");}}}
        Tree copy = t, moved = std::move(copy);
        checkEqual(moved.allProd(), total, "move");
        if (n) {
            moved.set(0, -17); checkEqual(t.get(0), a[0], "copy independence");
            t.set(0, t.get(0)); checkEqual(t.get(0), a[0], "set/get alias");
            lng before = t.allProd(); t.set(0, t.allProd()); checkEqual(t.get(0), before, "set/aggregate alias");}}

    void exhaustive() {
        int limit = mode == "quick" ? 4 : mode == "full" ? 6 : 8;
        for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 3; x /= 3; }
                numeric(a, true);}}
        numeric({-10, 0, 7, -4, 8}, false);
        numeric({std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()}, false);
        Tree single({7}), triple({1, 2, 3});
        checkEqual(single.n, 1, "braced singleton size"); checkEqual(single.get(0), lng(7), "braced singleton value");
        checkEqual(triple.prod(0, 3), lng(6), "braced list");
        std::cout << "PASS bounded exhaustive range/point/search/copy tests\n";}

    void booleans() {
        auto orf = [](bool x, bool y) { return x || y; };
        auto andf = [](bool x, bool y) { return x && y; };
        int limit = mode == "quick" ? 5 : mode == "full" ? 8 : 10;
        for (int n = 0; n <= limit; ++n) {
            for (int mask = 0; mask < (1 << n); ++mask) {
                vector<bool> a(n);
                for (int i = 0; i < n; ++i) { a[i] = mask >> i & 1; }
                context = "bool n=" + show(n) + " mask=" + show(mask);
                SegmentTree o(a, Monoid{false, orf});
                SegmentTree c(a, Monoid{true, andf});
                bool any = false, all = true;
                for (int i = 0; i < n; ++i) {
                    const bool &got = o.get(i);
                    checkEqual(got, bool(a[i]), "bool get(" + show(i) + ")"); any = any || a[i]; all = all && a[i];}
                const bool &oa = o.allProd(), &ca = c.allProd();
                checkEqual(oa, any, "or allProd"); checkEqual(ca, all, "and allProd");
                checkEqual(o.values() == a, true, "bool values");
                for (int l = 0; l <= n; ++l) {
                    bool x = false, y = true;
                    for (int r = l; r <= n; ++r) {
                        checkEqual(o.prod(l, r), x, "or prod(" + show(l) + "," + show(r) + ")");
                        checkEqual(c.prod(l, r), y, "and prod(" + show(l) + "," + show(r) + ")");
                        checkEqual(c.fold(l, r, true, [](bool u, bool s) { return u && s; }), y, "and fold(" + show(l) + "," + show(r) + ")");
                        checkEqual(o.foldRight(l, r, false, [](bool s, bool u) { return s || u; }), x, "or foldRight(" + show(l) + "," + show(r) + ")");
                        if (r < n) { x = x || a[r]; y = y && a[r]; }}
                    int r = l;
                    while (r < n && !a[r]) { ++r; }
                    checkEqual(o.maxRight(l, [](bool v) { return !v; }), r, "or maxRight(" + show(l) + ")");
                    r = l;
                    while (r < n && a[r]) { ++r; }
                    checkEqual(c.maxRight(l, [](bool v) { return v; }), r, "and maxRight(" + show(l) + ")");}
                for (int r = 0; r <= n; ++r) {
                    int l = r;
                    while (l > 0 && !a[l - 1]) { --l; }
                    checkEqual(o.minLeft(r, [](bool v) { return !v; }), l, "or minLeft(" + show(r) + ")");
                    l = r;
                    while (l > 0 && a[l - 1]) { --l; }
                    checkEqual(c.minLeft(r, [](bool v) { return v; }), l, "and minLeft(" + show(r) + ")");}
                if (n) {
                    o.apply(n - 1, true); checkEqual(bool(o.get(n - 1)), true, "or apply");
                    c.apply(0, false); checkEqual(bool(c.allProd()), false, "and apply");}}}
        std::cout << "PASS bool payload get/allProd/values/prod/search/apply n<=" << limit << "\n";}

    void randomHistories(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 100 : 500;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 220);
            vector<lng> a(n); for (lng &x : a) { x = lng(rng() % 100); }
            Tree t(a);
            for (int step = 0; step < 250; ++step) {
                context = "trial=" + show(trial) + " step=" + show(step) + " array=" + show(a);
                int p = int(rng() % n); lng value = lng(rng() % 100);
                t.set(p, value); a[p] = value;
                context += " update=(" + show(p) + "," + show(value) + ")";
                checkEqual(t.get(p), value, "set/get(" + show(p) + ")");
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (l > r) { std::swap(l, r); }
                lng sum = 0; for (int i = l; i < r; ++i) { sum += a[i]; }
                checkEqual(t.prod(l, r), sum, "random prod(" + show(l) + "," + show(r) + ")");
                lng cap = lng(rng() % (100 * n + 1));
                auto pred = [cap](lng x) { return x <= cap; };
                int right = l, left = r; sum = 0;
                while (right < n && sum + a[right] <= cap) { sum += a[right++]; }
                checkEqual(t.maxRight(l, pred), right, "random maxRight(" + show(l) + ",cap=" + show(cap) + ")");
                sum = 0;
                while (left && a[left - 1] + sum <= cap) { sum += a[--left]; }
                checkEqual(t.minLeft(r, pred), left, "random minLeft(" + show(r) + ",cap=" + show(cap) + ")");
                checkEqual(t.allProd(), std::accumulate(a.begin(), a.end(), lng(0)), "random allProd");}}
        std::cout << "PASS seeded update/prod/search histories\n";}

    void stateful(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 60 : 300;
        for (int trial = 0; trial < rounds; ++trial) {
            lng mod = array<lng, 4>{2, 7, 97, 1000000007}[trial % 4];
            int n = int(rng() % 60);
            vector<lng> a(n); for (lng &x : a) { x = lng(rng() % ulng(mod)); }
            SegmentTree s(a, ModSum{mod});
            SegmentTree c(a, Monoid{lng(0), [mod](lng x, lng y) { return (x + y) % mod; }});
            for (int step = 0; step < 100; ++step) {
                context = "stateful mod=" + show(mod) + " trial=" + show(trial) + " step=" + show(step);
                int l = int(rng() % ulng(n + 1)), r = int(rng() % ulng(n + 1));
                if (l > r) { std::swap(l, r); }
                lng want = 0; for (int i = l; i < r; ++i) { want = (want + a[i]) % mod; }
                checkEqual(s.prod(l, r), want, "member state prod"); checkEqual(c.prod(l, r), want, "capturing lambda prod");
                if (n) { int p = int(rng() % ulng(n)); lng x = lng(rng() % ulng(mod)); s.apply(p, x); c.apply(p, x); a[p] = (a[p] + x) % mod; }}}
        std::cout << "PASS stateful monoids (member modulus, capturing lambda)\n";}

    struct Affine {
        int a, b;
        int eval(int x) const { return (a * x + b) % 97; }
    };
    struct Compose {
        Affine operator()(Affine x, Affine y) const { return {y.a * x.a % 97, (y.a * x.b + y.b) % 97}; }
    };
    struct Box {
        lng x;
        Box() = delete;
        explicit Box(lng x) : x(x) {}
    };
    struct BoxAdd { Box operator()(const Box &a, const Box &b) const { return Box(a.x + b.x); } };

    void noncommutative(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 80 : 250;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = int(rng() % 40);
            vector<string> a(n); vector<Affine> affine(n);
            for (int i = 0; i < n; ++i) {
                a[i] = string(1, char('a' + rng() % 3)); affine[i] = {int(rng() % 97), int(rng() % 97)};}
            SegmentTree s(a, Monoid{string(), Join{}});
            SegmentTree t(affine, Monoid{Affine{1, 0}, Compose{}});
            for (int step = 0; step < 3; ++step) {
                context = "trial=" + show(trial) + " step=" + show(step) + " strings=" + show(a);
                for (int l = 0; l <= n; ++l) {
                    string joined;
                    for (int r = l; r <= n; ++r) {
                        checkEqual(s.prod(l, r), joined, "concatenation(" + show(l) + "," + show(r) + ")");
                        for (int x = 0; x < 3; ++x) {
                            int result = x;
                            for (int i = l; i < r; ++i) { result = affine[i].eval(result); }
                            checkEqual(t.prod(l, r).eval(x), result, "affine evaluation(" + show(l) + "," + show(r) + ",x=" + show(x) + ")");}
                        if (r < n) { joined += a[r]; }}
                    auto pred = [](const string &x) { return x.find("ab") == string::npos; };
                    int right = l, left = l; joined.clear();
                    while (right < n && pred(joined + a[right])) { joined += a[right++]; }
                    checkEqual(s.maxRight(l, pred), right, "ordered maxRight(" + show(l) + ")");
                    joined.clear();
                    while (left && pred(a[left - 1] + joined)) { joined = a[--left] + joined; }
                    checkEqual(s.minLeft(l, pred), left, "ordered minLeft(" + show(l) + ")");}
                if (n) {
                    int p = int(rng() % n); a[p] = "abc"; affine[p] = {int(rng() % 97), int(rng() % 97)};
                    s.set(p, a[p]); t.set(p, affine[p]);
                    int q = int(rng() % n); string tail(1, char('a' + rng() % 3));
                    Affine g{int(rng() % 97), int(rng() % 97)}, old = affine[q];
                    s.apply(q, tail); a[q] += tail; t.apply(q, g);
                    for (int x = 0; x < 97; ++x) { checkEqual(t.get(q).eval(x), g.eval(old.eval(x)), "affine apply order x=" + show(x)); }
                    affine[q] = t.get(q);}}
            for (int q = 0; q < n; ++q) {
                auto u = s; u.apply(q, string("z"));
                string all;
                for (int i = 0; i < n; ++i) { all += i == q ? a[i] + "z" : a[i]; }
                checkEqual(u.get(q), a[q] + "z", "string apply order p=" + show(q)); checkEqual(u.allProd(), all, "string apply aggregate p=" + show(q));}}
        context = "non-default-constructible payload";
        SegmentTree b(vector<Box>{Box(4), Box(-3)}, Monoid{Box(0), BoxAdd{}});
        checkEqual(b.prod(0, 2).x, 1, "Box fold"); b.set(1, Box(8)); checkEqual(b.allProd().x, 12, "Box set");
        std::cout << "PASS noncommutative strings/affine/search and generic payloads\n";}

    // Max-plus matrices act on row vectors (fold) and column vectors (foldRight); the oracles apply them one leaf at a time.
    using MaxPlus = MaxPlusMatrix<lng, 3>;
    using Mat = MaxPlus::S;
    using Vec = array<lng, 3>;
    constexpr lng NONE = MaxPlus::NONE;
    Vec rowTimes(const Vec &v, const Mat &a) {
        Vec res; res.fill(NONE);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (v[i] != NONE && a[i][j] != NONE) { res[j] = max(res[j], v[i] + a[i][j]); }}}
        return res;}
    Vec timesColumn(const Mat &a, const Vec &v) {
        Vec res; res.fill(NONE);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (a[i][j] != NONE && v[j] != NONE) { res[i] = max(res[i], a[i][j] + v[j]); }}}
        return res;}

    void folds(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 80 : 250;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = int(rng() % 40);
            vector<Mat> mats(n); vector<Affine> affine(n); vector<string> words(n);
            auto draw = [&](int i) {
                for (auto &row : mats[i]) { for (lng &x : row) { x = rng() % 4 ? lng(rng() % 19) - 9 : NONE; }}
                affine[i] = {int(rng() % 97), int(rng() % 97)}; words[i] = string(1, char('a' + rng() % 3));};
            for (int i = 0; i < n; ++i) { draw(i); }
            SegmentTree<MaxPlus> mt(mats);
            SegmentTree t(affine, Monoid{Affine{1, 0}, Compose{}});
            int ops = 0;
            SegmentTree s(words, Monoid{string(), [&ops](const string &u, const string &w) { ++ops; return u + w; }});
            int bound = 2 * std::bit_width(unsigned(mt.base)), calls = 0;
            auto row = [&](Vec v, const Mat &a) { ++calls; return rowTimes(v, a); };
            auto column = [&](const Mat &a, Vec v) { ++calls; return timesColumn(a, v); };
            for (int step = 0; step < 3; ++step) {
                context = "fold trial=" + show(trial) + " step=" + show(step) + " n=" + show(n);
                for (int l = 0; l <= n; ++l) {
                    for (int r = l; r <= n; ++r) {
                        string at = "(" + show(l) + "," + show(r) + ")";
                        Vec v{lng(rng() % 7), NONE, lng(rng() % 7) - 3}, want = v, wantColumn = v;
                        for (int i = l; i < r; ++i) { want = rowTimes(want, mats[i]); }
                        for (int i = r; i-- > l;) { wantColumn = timesColumn(mats[i], wantColumn); }
                        calls = 0; checkEqual(mt.fold(l, r, v, row), want, "max-plus fold" + at);
                        checkEqual(calls <= bound, true, "fold act calls" + at);
                        calls = 0; checkEqual(mt.foldRight(l, r, v, column), wantColumn, "max-plus foldRight" + at);
                        checkEqual(calls <= bound, true, "foldRight act calls" + at);
                        int x = int(rng() % 97), y = x;
                        Affine g{int(rng() % 97), int(rng() % 97)}, c{1, 0};
                        for (int i = l; i < r; ++i) { y = affine[i].eval(y); c = Compose{}(c, affine[i]); }
                        c = Compose{}(c, g);
                        checkEqual(t.fold(l, r, x, [](int u, const Affine &f) { return f.eval(u); }), y, "point set range composite fold" + at);
                        Affine h = t.foldRight(l, r, g, [](const Affine &f, const Affine &u) { return Compose{}(f, u); });
                        checkEqual(97 * h.a + h.b, 97 * c.a + c.b, "composite foldRight" + at);
                        string joined;
                        for (int i = l; i < r; ++i) { joined += words[i]; }
                        ops = 0;
                        checkEqual(s.fold(l, r, string("<"), [](string u, const string &w) { return u + w; }), "<" + joined, "string fold" + at);
                        checkEqual(s.foldRight(l, r, string(">"), [](const string &w, string u) { return w + u; }), joined + ">", "string foldRight" + at);
                        checkEqual(ops, 0, "folds form no product" + at);}}
                if (n) { int p = int(rng() % n); draw(p); mt.set(p, mats[p]); t.set(p, affine[p]); s.set(p, words[p]); }}}
        std::cout << "PASS fold/foldRight: max-plus row and column vectors, affine values and maps, strings\n";}

    void invalid(const string &name) {
        Tree t(vector<lng>{1, 2, 3});
        if (name == "negative-size") { Tree x(-1); }
        else if (name == "oversize") { Tree x(Tree::MAX_SIZE + 1); }
        else if (name == "vector-size") { (void)Tree::checkedSize(size_t(Tree::MAX_SIZE) + 1); }
        else if (name == "get-negative") { (void)t.get(-1); }
        else if (name == "get-end") { (void)t.get(3); }
        else if (name == "set-end") { t.set(3, 1); }
        else if (name == "apply-end") { t.apply(3, 1); }
        else if (name == "prod-negative") { (void)t.prod(-1, 2); }
        else if (name == "prod-reversed") { (void)t.prod(2, 1); }
        else if (name == "prod-end") { (void)t.prod(0, 4); }
        else if (name == "max-right-index") { (void)t.maxRight(4, [](lng) { return true; }); }
        else if (name == "min-left-index") { (void)t.minLeft(-1, [](lng) { return true; }); }
        else if (name == "max-right-identity") { (void)t.maxRight(0, [](lng) { return false; }); }
        else if (name == "min-left-identity") { (void)t.minLeft(3, [](lng) { return false; }); }
        else if (name == "fold-negative") { (void)t.fold(-1, 2, lng(0), std::plus<lng>()); }
        else if (name == "fold-reversed") { (void)t.fold(2, 1, lng(0), std::plus<lng>()); }
        else if (name == "fold-end") { (void)t.fold(0, 4, lng(0), std::plus<lng>()); }
        else if (name == "fold-right-negative") { (void)t.foldRight(-1, 2, lng(0), std::plus<lng>()); }
        else if (name == "fold-right-reversed") { (void)t.foldRight(2, 1, lng(0), std::plus<lng>()); }
        else if (name == "fold-right-end") { (void)t.foldRight(0, 4, lng(0), std::plus<lng>()); }
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
        std::cout << "segmenttree seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        std::mt19937_64 rng(seed);
        exhaustive(); booleans(); randomHistories(rng); stateful(rng); noncommutative(rng); folds(rng);
        std::cout << "PASS segmenttree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL segmenttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
