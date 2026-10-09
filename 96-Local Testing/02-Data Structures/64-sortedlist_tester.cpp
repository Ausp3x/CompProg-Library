#include "../../02-Data Structures/64-sortedlist.hpp"

namespace {
    ulng seed = 20261009;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &a) {
        string s = "[";
        for (int i = 0; i < int(a.size()); ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Equivalence by first only, so stability (insertion order among equals) is observable.
    struct ByFirst { bool operator()(const pair<int, int> &a, const pair<int, int> &b) const { return a.first < b.first; } };

    // Bucket invariants: nonempty, sorted across buckets, sizes in [B/2, 2 * load] unless single, n within the rebuild window.
    template<typename L>
    void structure(const L &s, const string &at) {
        int sum = 0;
        for (int b = 0; b < int(s.a.size()); ++b) {
            int z = int(s.a[b].size());
            sum += z;
            checkEqual(z > 0 && z <= 2 * s.load, true, at + " bucket " + show(b) + " size " + show(z) + " in (0, 2 * load]");
            if (s.a.size() > 1) { checkEqual(z >= s.load / 2, true, at + " bucket " + show(b) + " size " + show(z) + " >= load/2=" + show(s.load / 2)); }
            if (b) { checkEqual(!s.cmp(s.a[b][0], s.a[b - 1].back()), true, at + " bucket order"); }}
        checkEqual(sum, s.size(), at + " size = bucket sum");
        checkEqual(s.size() <= 2 * s.built + s.BMIN && 2 * s.size() >= s.built, true, at + " rebuild window n=" + show(s.size()) + " built=" + show(s.built));
        checkEqual(s.load, max(s.BMIN, int(std::sqrt(double(s.RATIO) * s.built))), at + " B follows built");}

    // Oracle: a vector kept sorted by stable insertion at upper_bound.
    template<typename T, typename C>
    void verify(const SortedList<T, C> &s, const vector<T> &o, const vector<T> &probes, const C &cmp, const string &at, bool deep) {
        structure(s, at);
        checkEqual(s.size(), int(o.size()), at + " size");
        checkEqual(s.empty(), o.empty(), at + " empty");
        if (!deep) { return; }
        checkEqual(show(vector<T>(s.begin(), s.end())), show(o), at + " forward iteration");
        checkEqual(show(vector<T>(s.rbegin(), s.rend())), show(vector<T>(o.rbegin(), o.rend())), at + " reverse iteration");
        if (!o.empty()) {
            checkEqual(s.front(), o.front(), at + " front");
            checkEqual(s.back(), o.back(), at + " back");
            checkEqual(*--s.end(), o.back(), at + " --end()");}
        // Every k on small lists; 65 evenly spaced k (both ends included) on large ones.
        int n = int(o.size()), stride = max(1, n / 64);
        for (int k = 0; k < n; k = k + stride < n || k == n - 1 ? k + stride : n - 1) {
            checkEqual(s.kth(k), o[k], at + " kth(" + show(k) + ")");
            checkEqual(s[k], o[k], at + " operator[](" + show(k) + ")");}
        vector<int> before(s.a.size() + 1, 0);
        for (int b = 0; b < int(s.a.size()); ++b) { before[b + 1] = before[b] + int(s.a[b].size()); }
        // Iterator position from bucket prefix sums; the dereferenced value is checked against the oracle separately.
        auto pos = [&](typename SortedList<T, C>::iterator it) {
            int p = it == s.end() ? n : before[it.b] + it.i;
            checkEqual(s.position(it), p, at + " position");
            if (p < n) { checkEqual(*it, o[p], at + " iterator value at " + show(p)); }
            return p;};
        for (const T &x : probes) {
            int lo = int(lower_bound(o.begin(), o.end(), x, cmp) - o.begin()), hi = int(upper_bound(o.begin(), o.end(), x, cmp) - o.begin());
            string q = "(" + show(x) + ")";
            checkEqual(s.rank(x), lo, at + " rank" + q);
            checkEqual(s.upperRank(x), hi, at + " upperRank" + q);
            checkEqual(s.count(x), hi - lo, at + " count" + q);
            checkEqual(s.contains(x), hi > lo, at + " contains" + q);
            checkEqual(pos(s.lowerBound(x)), lo, at + " lowerBound" + q);
            checkEqual(pos(s.upperBound(x)), hi, at + " upperBound" + q);
            checkEqual(pos(s.next(x)), lo, at + " next" + q);
            checkEqual(pos(s.prev(x)), hi ? hi - 1 : int(o.size()), at + " prev" + q);
            if (lo < int(o.size())) { checkEqual(*s.lowerBound(x), o[lo], at + " *lowerBound" + q); }
            if (hi) { checkEqual(*s.prev(x), o[hi - 1], at + " *prev" + q); }}}

    template<typename T, typename C, typename Gen>
    void randomOps(Rng &rng, const string &name, C cmp, Gen gen, vector<T> probes, int ops, int maxSize, int deepEvery) {
        context = name + " seed=" + show(seed);
        vector<T> init;
        for (int i = int(rnd(rng, 0, 3)) * int(rnd(rng, 0, maxSize / 2)); i > 0; --i) { init.push_back(gen()); }
        SortedList<T, C> s(init, cmp);
        vector<T> o = init;
        std::stable_sort(o.begin(), o.end(), cmp);
        auto oracleErase = [&](int k) { T x = o[k]; o.erase(o.begin() + k); return x; };
        for (int step = 0; step < ops; ++step) {
            string at = "step " + show(step);
            // Alternate growth and shrink phases so growth rebuilds, shrink rebuilds, splits and merges all occur.
            bool grow = (step / max(100, maxSize / 2)) % 2 == 0 && int(o.size()) < maxSize;
            int op = rnd(rng, 0, 99) < (grow ? 80 : 20) ? 0 : int(rnd(rng, 30, 99));
            T x = gen();
            if (op == 0) {
                s.insert(x);
                o.insert(upper_bound(o.begin(), o.end(), x, cmp), x);}
            else if (op < 60) {
                auto it = lower_bound(o.begin(), o.end(), x, cmp);
                bool in = it != o.end() && !cmp(x, *it);
                if (in) { o.erase(it); }
                checkEqual(s.eraseOne(x), in, at + " eraseOne(" + show(x) + ")");}
            else if (op < 66) {
                auto l = lower_bound(o.begin(), o.end(), x, cmp), r = upper_bound(o.begin(), o.end(), x, cmp);
                int c = int(r - l);
                o.erase(l, r);
                checkEqual(s.erase(x), c, at + " erase(" + show(x) + ")");}
            else if (op < 74 && !o.empty()) {
                int k = int(rnd(rng, 0, int(o.size()) - 1));
                checkEqual(s.eraseKth(k), oracleErase(k), at + " eraseKth(" + show(k) + ")");}
            else if (op < 78 && !o.empty()) { checkEqual(s.popFront(), oracleErase(0), at + " popFront"); }
            else if (op < 82 && !o.empty()) { checkEqual(s.popBack(), oracleErase(int(o.size()) - 1), at + " popBack"); }
            else if (op < 83) { s.rebuild(); }
            else if (op == 83 && rnd(rng, 0, 9) == 0) { s.clear(); o.clear(); }
            else if (op == 84 && rnd(rng, 0, 9) == 0) {
                vector<T> v;
                for (int i = int(rnd(rng, 0, maxSize)); i > 0; --i) { v.push_back(gen()); }
                o = v;
                std::stable_sort(o.begin(), o.end(), cmp);
                if (rnd(rng, 0, 1)) { v = o; }
                s.rebuild(v);}
            verify(s, o, probes, cmp, at, step % (mode == "stress" ? 5 * deepEvery : deepEvery) == 0 || ops - step < 3);}
        std::cout << "PASS " << name << " ops=" << ops << '\n';}

    void edgeCases() {
        context = "edge seed=" + show(seed);
        SortedList<int> e;
        vector<int> probes{-1, 0, 1};
        verify(e, {}, probes, std::less<int>(), "empty", true);
        checkEqual(e.begin() == e.end() && e.rbegin() == e.rend(), true, "empty iterators");
        checkEqual(e.eraseOne(0) || e.erase(0) != 0, false, "erase from empty");
        e.insert(5);
        verify(e, {5}, {4, 5, 6}, std::less<int>(), "singleton", true);
        checkEqual(e.popBack(), 5, "singleton popBack");
        checkEqual(e.empty() && e.a.empty(), true, "drops last bucket");
        // Monotone, all-equal and sawtooth workloads across many rebuilds.
        int n = mode == "quick" ? 3000 : 40000;
        SortedList<int> s;
        vector<int> o;
        for (int i = 0; i < n; ++i) { s.insert(i); o.push_back(i); }
        verify(s, o, {-1, 0, n / 2, n - 1, n}, std::less<int>(), "increasing", true);
        for (int i = 0; i < n; ++i) { s.insert(-i); o.push_back(-i); }
        sort(o.begin(), o.end());
        verify(s, o, {-n, -1, 0, 1, n}, std::less<int>(), "decreasing", true);
        for (int i = 0; i < 2 * n; ++i) { s.insert(7); o.push_back(7); }
        sort(o.begin(), o.end());
        verify(s, o, {6, 7, 8}, std::less<int>(), "many equal", true);
        checkEqual(s.erase(7), 2 * n + 1, "erase spanning many buckets");
        o.erase(lower_bound(o.begin(), o.end(), 7), upper_bound(o.begin(), o.end(), 7));
        verify(s, o, {6, 7, 8}, std::less<int>(), "after erase all", true);
        int head = 0;
        while (s.size() > 10) {
            checkEqual(s.popFront(), o[head++], "drain popFront");
            if (s.size() % 997 == 0) { structure(s, "drain"); }}
        o.erase(o.begin(), o.begin() + head);
        verify(s, o, {0, n}, std::less<int>(), "drained", true);
        // Equal values in many buckets surrounded by others: erase removes only the run.
        SortedList<int> r;
        vector<int> ro;
        for (int i = 0; i < n; ++i) { int v = i % 3 == 0 ? 50 : i % 3 == 1 ? 10 : 90; r.insert(v); ro.push_back(v); }
        sort(ro.begin(), ro.end());
        checkEqual(r.erase(50), int(std::count(ro.begin(), ro.end(), 50)), "middle run erase count");
        ro.erase(lower_bound(ro.begin(), ro.end(), 50), upper_bound(ro.begin(), ro.end(), 50));
        verify(r, ro, {10, 50, 90}, std::less<int>(), "middle run erase", true);
        // Construction from sorted, unsorted and descending input; copies are independent.
        vector<int> u(n);
        for (int i = 0; i < n; ++i) { u[i] = (i * 7919) % 1009; }
        SortedList<int> cu(u);
        vector<int> su = u;
        sort(su.begin(), su.end());
        verify(cu, su, {0, 500, 1008}, std::less<int>(), "unsorted construction", true);
        SortedList<int, std::greater<int>> cg(u, std::greater<int>());
        vector<int> sg = su;
        reverse(sg.begin(), sg.end());
        verify(cg, sg, {0, 500, 1008, 2000}, std::greater<int>(), "greater construction", true);
        auto copy = cu;
        copy.erase(0);
        checkEqual(cu.count(0) > 0 && copy.count(0) == 0, true, "copy independence");
        // Stability among equivalent elements.
        vector<pair<int, int>> ps;
        for (int i = 0; i < 500; ++i) { ps.push_back({i % 4, i}); }
        SortedList<pair<int, int>, ByFirst> st(ps);
        vector<pair<int, int>> sp = ps;
        std::stable_sort(sp.begin(), sp.end(), ByFirst());
        verify(st, sp, {{-1, 0}, {0, 0}, {2, 0}, {3, 0}, {4, 0}}, ByFirst(), "stable construction", true);
        st.insert({2, -1});
        sp.insert(upper_bound(sp.begin(), sp.end(), pair<int, int>{2, -1}, ByFirst()), {2, -1});
        checkEqual(st.eraseOne({1, 0}), true, "stable eraseOne");
        sp.erase(lower_bound(sp.begin(), sp.end(), pair<int, int>{1, 0}, ByFirst()));
        verify(st, sp, {{0, 0}, {1, 0}, {2, 0}}, ByFirst(), "stable after insert and eraseOne", true);
        // Strings exercise moves out of buckets.
        SortedList<string> ss({"b", "a", "c", "a"});
        checkEqual(ss.eraseKth(1), string("a"), "string eraseKth");
        checkEqual(ss.popBack(), string("c"), "string popBack");
        checkEqual(show(vector<string>(ss.begin(), ss.end())), string("[a,b]"), "string contents");
        // Arguments aliasing stored elements: erase(front()) whose run ends exactly at bucket boundaries.
        vector<string> al(200, string(40, 'x'));
        al.insert(al.end(), 200, string(40, 'y'));
        SortedList<string> as(al);
        checkEqual(as.erase(as.front()), 200, "erase(front()) aliasing");
        as.insert(as.back());
        checkEqual(as.eraseOne(as.kth(0)) && as.size() == 200 && as.count(string(40, 'y')) == 200, true, "insert(back()) and eraseOne(kth(0)) aliasing");
        // Extreme values.
        SortedList<lng> x({LLONG_MAX, LLONG_MIN, 0, LLONG_MAX});
        verify(x, {LLONG_MIN, 0, LLONG_MAX, LLONG_MAX}, {LLONG_MIN, -1, LLONG_MAX}, std::less<lng>(), "extremes", true);
        std::cout << "PASS empty, singleton, monotone, equal runs, construction, comparator, stability, strings, extremes n=" << n << '\n';}

    void invalid(const string &name) {
        SortedList<int> s({1, 2, 3}), e;
        if (name == "kth-negative") { (void)s.kth(-1); }
        else if (name == "kth-end") { (void)s.kth(3); }
        else if (name == "erasekth-end") { (void)s.eraseKth(3); }
        else if (name == "front-empty") { (void)e.front(); }
        else if (name == "back-empty") { (void)e.back(); }
        else if (name == "popfront-empty") { (void)e.popFront(); }
        else if (name == "popback-empty") { (void)e.popBack(); }
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
        std::cout << "sortedlist seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        int f = mode == "quick" ? 1 : mode == "full" ? 8 : 40;
        vector<int> small;
        for (int v = -1; v <= 9; ++v) { small.push_back(v); }
        randomOps<int>(rng, "int values [0,8] size<=300", std::less<int>(), [&] { return int(rnd(rng, 0, 8)); }, small, 3000 * f, 300, 1);
        vector<int> wide;
        for (int v = -5; v <= 1005; v += 15) { wide.push_back(v); }
        randomOps<int>(rng, "int values [0,1000] size<=3000", std::less<int>(), [&] { return int(rnd(rng, 0, 1000)); }, wide, 4000 * f, 3000, 25);
        randomOps<int>(rng, "greater values [0,50] size<=2000", std::greater<int>(), [&] { return int(rnd(rng, 0, 50)); }, wide, 3000 * f, 2000, 25);
        vector<pair<int, int>> pp;
        for (int v = -1; v <= 6; ++v) { pp.push_back({v, 0}); }
        int id = 0;
        randomOps<pair<int, int>>(rng, "stable pairs by first size<=400", ByFirst(), [&] { return pair<int, int>{int(rnd(rng, 0, 5)), id++}; }, pp, 3000 * f, 400, 5);
        vector<lng> big{LLONG_MIN, -1, 0, LLONG_MAX};
        randomOps<lng>(rng, "lng full range size<=20000", std::less<lng>(), [&] { return lng(rng()); }, big, 3000 * f, 20000, 400);
        edgeCases();
        std::cout << "PASS sortedlist checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL sortedlist seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
