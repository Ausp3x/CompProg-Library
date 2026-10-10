#include "../../02-Data Structures/16-mergesorttree.hpp"

namespace {
    ulng seed = 20261010;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    pair<int, int> range(Rng &rng, int n) {
        int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
        if (l > r) { swap(l, r); }
        return {l, r};}

    lng value(Rng &rng, lng span, lng *) { return rnd(rng, -span, span); }
    string value(Rng &rng, lng span, string *) { string s; for (lng k = rnd(rng, 0, 2); k--;) { s += char('a' + rnd(rng, 0, min<lng>(span, 25))); } return s; }
    pair<int, int> value(Rng &rng, lng span, pair<int, int> *) { return {int(rnd(rng, 0, 2)), int(rnd(rng, -span, span))}; }

    // Checks every query of a structure against the multiset bag of values held in [l, r).
    template<typename T, typename Tree>
    void compare(Tree &t, const vector<T> &bag, int l, int r, Rng &rng, lng span) {
        vector<T> b = bag;
        sort(b.begin(), b.end());
        T x = rng() % 3 == 0 && !b.empty() ? b[rng() % b.size()] : value(rng, span + 1, (T *)nullptr);
        T y = value(rng, span + 1, (T *)nullptr);
        auto less = [&](const T &v) { return int(lower_bound(b.begin(), b.end(), v) - b.begin()); };
        checkEqual(t.countLess(l, r, x), less(x), "countLess");
        checkEqual(t.countRange(l, r, x, y), x < y ? less(y) - less(x) : 0, "countRange");
        checkEqual(t.countEqual(l, r, x), int(upper_bound(b.begin(), b.end(), x) - lower_bound(b.begin(), b.end(), x)), "countEqual");
        if (!b.empty()) { int k = int(rng() % b.size()); checkEqual(t.kth(l, r, k), b[k], "kth " + show(k)); }
        T got{};
        auto it = upper_bound(b.begin(), b.end(), x);
        bool ok = t.maxLeq(l, r, x, got);
        checkEqual(ok, it != b.begin(), "maxLeq found");
        if (ok) { checkEqual(got, *prev(it), "maxLeq"); }
        it = lower_bound(b.begin(), b.end(), x);
        ok = t.minGeq(l, r, x, got);
        checkEqual(ok, it != b.end(), "minGeq found");
        if (ok) { checkEqual(got, *it, "minGeq"); }}

    template<typename T>
    void staticRun(Rng &rng, int n, lng span, int queries, const string &name) {
        vector<T> a(n);
        for (auto &x : a) { x = value(rng, span, (T *)nullptr); }
        MergeSortTree<T> t(a);
        for (int q = 0; q < queries; ++q) {
            auto [l, r] = range(rng, n);
            context = name + " static n=" + show(n) + " span=" + show(span) + " query=" + show(q) + " l=" + show(l) + " r=" + show(r);
            compare<T>(t, vector<T>(a.begin() + l, a.begin() + r), l, r, rng, span);}
        // Legacy rangeMedianQuery(l, r - 1) returned the element of 0-based rank (r - l) / 2.
        for (int l = 0; l < n && l < 12; ++l) {
            for (int r = l + 1; r <= n; ++r) {
                vector<T> b(a.begin() + l, a.begin() + r);
                std::nth_element(b.begin(), b.begin() + (r - l) / 2, b.end());
                checkEqual(t.kth(l, r, (r - l) / 2), b[(r - l) / 2], "median");}}}

    template<typename T>
    void dynamicRun(Rng &rng, int n, lng span, int steps, bool single, const string &name) {
        vector<vector<T>> slot(n);
        DynamicMergeSortTree<T> t(n);
        if (single) {
            vector<T> a(n);
            for (int i = 0; i < n; ++i) { a[i] = value(rng, span, (T *)nullptr); slot[i] = {a[i]}; }
            t = DynamicMergeSortTree<T>(a);}
        for (int s = 0; s < steps; ++s) {
            context = name + " dynamic n=" + show(n) + " single=" + show(single) + " step=" + show(s);
            if (n) {
                int i = int(rnd(rng, 0, n - 1));
                T x = value(rng, span, (T *)nullptr);
                int kind = int(rng() % 3);
                if (single) { t.set(i, x); slot[i] = {x}; }
                else if (kind == 0 || slot[i].empty()) { t.insert(i, x); slot[i].push_back(x); }
                else {
                    if (kind == 1) { x = slot[i][rng() % slot[i].size()]; }
                    auto it = std::find(slot[i].begin(), slot[i].end(), x);
                    checkEqual(t.erase(i, x), it != slot[i].end(), "erase");
                    if (it != slot[i].end()) { slot[i].erase(it); }}}
            auto [l, r] = range(rng, n);
            vector<T> bag;
            for (int i = l; i < r; ++i) { bag.insert(bag.end(), slot[i].begin(), slot[i].end()); }
            context += " l=" + show(l) + " r=" + show(r);
            checkEqual(t.size(l, r), int(bag.size()), "size");
            compare<T>(t, bag, l, r, rng, span);}}

    template<typename T>
    void frequencyRun(Rng &rng, int n, lng span, int steps, const string &name) {
        vector<T> a(n);
        for (auto &x : a) { x = value(rng, span, (T *)nullptr); }
        PointSetRangeFrequency<T> t(a);
        for (int s = 0; s < steps; ++s) {
            context = name + " frequency n=" + show(n) + " step=" + show(s);
            if (n && rng() % 2) { int i = int(rnd(rng, 0, n - 1)); a[i] = value(rng, span, (T *)nullptr); t.set(i, a[i]); }
            auto [l, r] = range(rng, n);
            T x = rng() % 2 && n ? a[rng() % n] : value(rng, span + 1, (T *)nullptr);
            checkEqual(t.count(l, r, x), int(std::count(a.begin() + l, a.begin() + r, x)), "count");
            if (n) { int i = int(rnd(rng, 0, n - 1)); checkEqual(t.get(i), a[i], "get"); }}}

    void edges() {
        context = "edges";
        MergeSortTree<lng> z;
        lng out = 7;
        checkEqual(z.countLess(0, 0, 5), 0, "static empty"); checkEqual(z.maxLeq(0, 0, 5, out), false, "static empty maxLeq"); checkEqual(out, 7, "out untouched");
        lng big = std::numeric_limits<lng>::max(), low = std::numeric_limits<lng>::min();
        MergeSortTree<lng> e(vector<lng>{big, low, 0, big, low});
        checkEqual(e.countLess(0, 5, big), 3, "extreme countLess"); checkEqual(e.countEqual(0, 5, low), 2, "extreme countEqual"); checkEqual(e.kth(0, 5, 4), big, "extreme kth");
        checkEqual(e.countRange(0, 5, 5, 1), 0, "reversed value range");
        DynamicMergeSortTree<lng> d0(0), d(3);
        checkEqual(d0.size(0, 0), 0, "dynamic empty");
        d.insert(1, 4); d.insert(1, 4); d.insert(2, -1);
        checkEqual(d.countEqual(0, 3, 4), 2, "dynamic duplicates"); checkEqual(d.erase(0, 4), false, "dynamic erase absent slot");
        checkEqual(d.erase(1, 4), true, "dynamic erase one copy"); checkEqual(d.countEqual(1, 2, 4), 1, "dynamic one copy left");
        auto copy = d;
        copy.insert(0, 9);
        checkEqual(d.size(0, 3), 2, "dynamic copy independent");
        PointSetRangeFrequency<lng> f(vector<lng>{1, 1, 2});
        f.set(0, 2); f.set(1, 2);
        checkEqual(f.count(0, 3, 1), 0, "frequency value vanishes"); checkEqual(int(f.pos.size()), 1, "frequency erases empty set");}

    void invalid(const string &name) {
        MergeSortTree<lng> t(vector<lng>{3, 1, 2});
        DynamicMergeSortTree<lng> d(vector<lng>{3, 1, 2});
        PointSetRangeFrequency<lng> f(vector<lng>{3, 1, 2});
        lng out = 0;
        if (name == "static-count-reversed") { (void)t.countLess(2, 1, 0); }
        else if (name == "static-count-end") { (void)t.countEqual(0, 4, 0); }
        else if (name == "static-kth-k") { (void)t.kth(0, 3, 3); }
        else if (name == "static-kth-empty") { (void)t.kth(1, 1, 0); }
        else if (name == "static-maxleq-end") { (void)t.maxLeq(0, 4, 1, out); }
        else if (name == "static-mingeq-negative") { (void)t.minGeq(-1, 2, 1, out); }
        else if (name == "dynamic-negative-size") { DynamicMergeSortTree<lng> x(-1); }
        else if (name == "dynamic-insert-end") { d.insert(3, 1); }
        else if (name == "dynamic-erase-negative") { (void)d.erase(-1, 1); }
        else if (name == "dynamic-set-multi") { d.insert(0, 5); d.set(0, 1); }
        else if (name == "dynamic-set-empty") { DynamicMergeSortTree<lng> x(2); x.set(1, 4); }
        else if (name == "dynamic-kth-k") { (void)d.kth(0, 3, 3); }
        else if (name == "dynamic-count-reversed") { (void)d.countLess(2, 1, 0); }
        else if (name == "frequency-get-end") { (void)f.get(3); }
        else if (name == "frequency-set-negative") { f.set(-1, 0); }
        else if (name == "frequency-count-reversed") { (void)f.count(2, 1, 0); }
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
        std::cout << "mergesorttree seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        vector<int> sizes = mode == "quick" ? vector<int>{0, 1, 2, 3, 5, 8, 13} : vector<int>{0, 1, 2, 3, 4, 5, 7, 8, 9, 16, 17, 31, 33, 64, 100, 129};
        if (mode == "stress") { sizes.push_back(513); sizes.push_back(1000); }
        int steps = mode == "quick" ? 100 : mode == "full" ? 700 : 3000;
        for (int n : sizes) {
            for (lng span : {lng(0), lng(3), lng(1000000000000)}) {
                staticRun<lng>(rng, n, span, steps, "lng");
                dynamicRun<lng>(rng, n, span, steps, true, "lng"); dynamicRun<lng>(rng, n, span, steps, false, "lng");
                frequencyRun<lng>(rng, n, span, steps, "lng");}
            staticRun<string>(rng, n, 3, steps / 4, "string"); staticRun<pair<int, int>>(rng, n, 4, steps / 4, "pair");
            dynamicRun<string>(rng, n, 3, steps / 4, false, "string"); frequencyRun<string>(rng, n, 2, steps / 4, "string");}
        std::cout << "PASS mergesorttree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL mergesorttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
