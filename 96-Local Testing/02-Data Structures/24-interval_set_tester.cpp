#include "../../02-Data Structures/24-interval_set.hpp"

namespace {
    ulng seed = 20261008;
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
    int steps() { return mode == "quick" ? 200 : mode == "full" ? 2000 : 12000; }

    template<typename T>
    vector<pair<T, T>> runs(const vector<bool> &cov, T lo) {
        vector<pair<T, T>> res;
        for (int i = 0; i < int(cov.size()); ++i) {
            if (cov[i] && (i == 0 || !cov[i - 1])) { res.push_back({lo + i, lo + i}); }
            if (cov[i]) { res.back().second = lo + i + 1; }}
        return res;}

    template<typename T>
    void setSmall(Rng &rng, const string &name) {
        T lo = -20, hi = 60;
        int n = int(hi - lo);
        context = name + " IntervalSet seed=" + show(seed);
        IntervalSet<T> s;
        vector<bool> cov(n, false);
        auto verify = [&](const string &at) {
            auto want = runs(cov, lo);
            checkEqual(show(s.intervals()), show(want), at + " intervals");
            checkEqual(s.count(), int(want.size()), at + " count");
            T len = 0;
            for (auto &[a, b] : want) { len += b - a; }
            checkEqual(s.unionLength(), len, at + " unionLength");
            for (int i = -1; i <= n; ++i) {
                T x = lo + i;
                bool in = 0 <= i && i < n && cov[i];
                checkEqual(s.contains(x), in, at + " contains(" + show(x) + ")");
                pair<T, T> piece{x, x};
                for (auto &p : want) {
                    if (p.first <= x && x < p.second) { piece = p; }}
                checkEqual(s.find(x), piece, at + " find(" + show(x) + ")");
                T m = x;
                while (lo <= m && m < hi && cov[m - lo]) { ++m; }
                checkEqual(s.mex(x), m, at + " mex(" + show(x) + ")");
                T nx = x;
                while (nx < hi && !(lo <= nx && cov[nx - lo])) { ++nx; }
                checkEqual(s.next(x), nx < hi ? nx : std::numeric_limits<T>::max(), at + " next(" + show(x) + ")");
                for (int j = i; j <= n + 1; ++j) {
                    T y = lo + j;
                    bool all = true;
                    for (T z = x; z < y; ++z) { all = all && lo <= z && z < hi && cov[z - lo]; }
                    checkEqual(s.covers(x, y), all, at + " covers(" + show(x) + "," + show(y) + ")");}}};
        verify("empty");
        for (int step = 0; step < steps() / 4; ++step) {
            T l = T(rnd(rng, lo - 2, hi + 2)), r = T(rnd(rng, lo - 2, hi + 2));
            if (l > r) { swap(l, r); }
            if (rng() % 5 == 0) { r = T(l + rnd(rng, 0, 2)); }
            string at = "step=" + show(step) + " [" + show(l) + "," + show(r) + ")";
            T delta = 0;
            bool ins = rng() % 2;
            for (T x = l; x < r; ++x) {
                bool in = lo <= x && x < hi && cov[x - lo];
                if (lo <= x && x < hi) { cov[x - lo] = ins; }
                delta += in != ins && lo <= x && x < hi;}
            T outside = 0;
            for (T x = l; x < r; ++x) { outside += x < lo || x >= hi; }
            if (ins) { checkEqual(s.insert(l, r), delta + outside, at + " insert"); }
            else { checkEqual(s.erase(l, r), delta, at + " erase"); }
            if (ins) {
                for (T x = l; x < r; ++x) {
                    if (x < lo || x >= hi) { s.erase(x, x + 1); }}}
            verify(at);}
        IntervalSet<T> c = s;
        auto before = s.intervals();
        c.insert(lo - 5, hi + 5);
        checkEqual(c.count(), 1, "copy merged to one"); checkEqual(c.unionLength(), hi - lo + 10, "copy length");
        checkEqual(show(s.intervals()), show(before), "copy independence");
        std::cout << "PASS IntervalSet<" << name << "> against a dense bitmap\n";}

    void setLarge(Rng &rng) {
        context = "IntervalSet large coordinates";
        IntervalSet<lng> s;
        vector<pair<lng, lng>> ref;
        lng big = lng(1e18);
        auto normalize = [&]() {
            sort(ref.begin(), ref.end());
            vector<pair<lng, lng>> out;
            for (auto &[a, b] : ref) {
                if (!out.empty() && out.back().second >= a) { out.back().second = max(out.back().second, b); }
                else { out.push_back({a, b}); }}
            ref = out;};
        for (int step = 0; step < steps() / 2; ++step) {
            lng l = rnd(rng, -big, big), r = rnd(rng, -big, big);
            if (rng() % 3 == 0 && !ref.empty()) { l = ref[rng() % ref.size()].first; r = l + rnd(rng, 0, big); }
            if (rng() % 3 == 0 && !ref.empty()) { r = ref[rng() % ref.size()].second; }
            if (l > r) { swap(l, r); }
            lng before = 0;
            for (auto &[a, b] : ref) { before += b - a; }
            if (rng() % 3) {
                ref.push_back({l, r}); normalize();
                lng after = 0;
                for (auto &[a, b] : ref) { after += b - a; }
                checkEqual(s.insert(l, r), after - before, "insert step=" + show(step));}
            else {
                vector<pair<lng, lng>> out;
                for (auto &[a, b] : ref) {
                    if (a < l) { out.push_back({a, min(b, l)}); }
                    if (r < b) { out.push_back({max(a, r), b}); }}
                ref = out;
                lng after = 0;
                for (auto &[a, b] : ref) { after += b - a; }
                checkEqual(s.erase(l, r), before - after, "erase step=" + show(step));}
            checkEqual(show(s.intervals()), show(ref), "intervals step=" + show(step));
            lng x = rnd(rng, -big, big);
            if (rng() % 2 && !ref.empty()) { x = ref[rng() % ref.size()].first + rnd(rng, -1, 1); }
            pair<lng, lng> piece{x, x};
            lng nx = std::numeric_limits<lng>::max();
            for (auto &p : ref) {
                if (p.first <= x && x < p.second) { piece = p; }
                if (p.second > x) { nx = min(nx, max(p.first, x)); }}
            checkEqual(s.find(x), piece, "find step=" + show(step));
            checkEqual(s.mex(x), piece.second, "mex step=" + show(step));
            checkEqual(s.next(x), nx, "next step=" + show(step));
            checkEqual(s.covers(x, x + 1), piece.first != piece.second, "covers step=" + show(step));}
        std::cout << "PASS IntervalSet with 1e18 coordinates against a sorted-merge oracle\n";}

    bool merged = false;
    void mapTests(Rng &rng) {
        for (auto [lo, hi] : vector<pair<lng, lng>>{{0, 0}, {0, 1}, {-5, 7}, {0, 40}, {-30, 50}}) {
            context = "IntervalMap universe=[" + show(lo) + "," + show(hi) + ")";
            int n = int(hi - lo);
            IntervalMap<lng, int> t(lo, hi, 7);
            ChthollyTree<lng, int> alias(lo, hi, 7);
            checkEqual(alias.count(), t.count(), "alias construction");
            vector<int> a(n, 7);
            auto pieces = [&](lng l, lng r) {
                vector<tuple<lng, lng, int>> out;
                t.enumerate(l, r, [&](lng x, lng y, const int &v) { out.push_back({x, y, v}); });
                return out;};
            auto verify = [&](const string &at) {
                checkEqual(t.count(), n ? int(pieces(lo, hi).size()) : 0, at + " count");
                lng pos = lo;
                for (auto [x, y, v] : pieces(lo, hi)) {
                    checkEqual(x, pos, at + " piece start");
                    checkEqual(x < y, true, at + " nonempty piece");
                    for (lng i = x; i < y; ++i) { checkEqual(a[i - lo], v, at + " uniform piece at " + show(i)); }
                    pos = y;}
                checkEqual(pos, n ? hi : lo, at + " pieces cover universe");
                for (lng x = lo; x < hi; ++x) {
                    auto [l, r, v] = t.get(x);
                    checkEqual(l <= x && x < r, true, at + " get range");
                    checkEqual(v, a[x - lo], at + " get value");
                    checkEqual(l == lo || a[l - 1 - lo] != v || !merged, true, at + " piece boundary is real");}};
            for (int step = 0; step < (n ? steps() / 5 : 2); ++step) {
                lng l = rnd(rng, lo, hi), r = rnd(rng, lo, hi);
                if (l > r) { swap(l, r); }
                string at = "step=" + show(step) + " [" + show(l) + "," + show(r) + ")";
                merged = false;
                switch (rng() % 6) {
                    case 0: case 1: {
                        int v = int(rnd(rng, 0, 3));
                        lng removedLen = 0;
                        bool exact = true;
                        t.assign(l, r, v, [&](lng x, lng y, const int &old) {
                            removedLen += y - x;
                            for (lng i = x; i < y; ++i) { exact = exact && a[i - lo] == old; }});
                        checkEqual(removedLen, r - l, at + " removed length");
                        checkEqual(exact, true, at + " removed values");
                        for (lng i = l; i < r; ++i) { a[i - lo] = v; }
                        if (l < r) {
                            auto [pl, pr, pv] = t.get(l);
                            checkEqual(pl == l, l == lo || a[l - 1 - lo] != v, at + " assign merges left neighbour");
                            checkEqual(pr == r, r == hi || a[r - lo] != v, at + " assign merges right neighbour");}
                        break;}
                    case 2: {
                        lng pos = l;
                        t.apply(l, r, [&](lng x, lng y, int &v) { checkEqual(x, pos, at + " apply piece chain"); pos = y; v = (2 * v + 1) % 1000; });
                        checkEqual(pos, r, at + " apply pieces cover range");
                        for (lng i = l; i < r; ++i) { a[i - lo] = (2 * a[i - lo] + 1) % 1000; }
                        break;}
                    case 3: {
                        lng want = 0;
                        for (lng i = l; i < r; ++i) { want += a[i - lo] * i; }
                        lng got = t.fold(l, r, lng(0), [](lng acc, lng x, lng y, const int &v) { return acc + v * (x + y - 1) * (y - x) / 2; });
                        checkEqual(got, want, at + " fold weighted sum");
                        break;}
                    case 4: {
                        t.merge(l, r);
                        if (l == lo && r == hi) { merged = true; }
                        lng prevEnd = lo;
                        int prevVal = 0;
                        for (auto [x, y, v] : pieces(lo, hi)) {
                            if (x > lo && l <= x && x <= r) { checkEqual(v != prevVal, true, at + " merge leaves no equal neighbours at " + show(x)); }
                            prevEnd = y; prevVal = v;}
                        (void)prevEnd;
                        break;}
                    default:
                        if (n) {
                            auto it = t.split(l < hi ? l : lo);
                            checkEqual(it->first, l < hi ? l : lo, at + " split key");
                            checkEqual(it->second, a[(l < hi ? l : lo) - lo], at + " split value");}
                        checkEqual(t.split(hi) == t.m.end(), true, at + " split at hi");}
                verify(at);
                if (merged) {
                    int want = 0;
                    for (int i = 0; i < n; ++i) { want += i == 0 || a[i] != a[i - 1]; }
                    checkEqual(t.count(), want, at + " minimal after merge");}}}
        std::cout << "PASS IntervalMap / ChthollyTree against a dense array\n";}

    void invalid(const string &name) {
        IntervalSet<lng> s;
        s.insert(0, 5);
        IntervalMap<lng, int> t(0, 5, 1);
        if (name == "set-insert-reversed") { s.insert(3, 2); }
        else if (name == "set-erase-reversed") { s.erase(3, 2); }
        else if (name == "set-covers-reversed") { (void)s.covers(3, 2); }
        else if (name == "map-reversed-universe") { IntervalMap<lng, int> x(1, 0, 1); }
        else if (name == "map-get-end") { (void)t.get(5); }
        else if (name == "map-assign-end") { t.assign(0, 6, 1); }
        else if (name == "map-assign-reversed") { t.assign(3, 2, 1); }
        else if (name == "map-apply-negative") { t.apply(-1, 2, [](lng, lng, int &) {}); }
        else if (name == "map-enumerate-end") { t.enumerate(0, 6, [](lng, lng, const int &) {}); }
        else if (name == "map-split-end") { (void)t.split(6); }
        else if (name == "map-merge-reversed") { t.merge(4, 3); }
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
        std::cout << "interval_set seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        setSmall<lng>(rng, "lng");
        setSmall<int>(rng, "int");
        setLarge(rng);
        mapTests(rng);
        std::cout << "PASS interval_set checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL interval_set seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
