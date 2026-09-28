#include "../../02-Data Structures/03-segmenttree.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    int checks = 0;
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &a) {
        string s = "[";
        for (int i = 0; i < int(a.size()); ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (got != want) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}
    struct Add { lng operator()(lng a, lng b) const { return a + b; } };
    struct Join { string operator()(const string &a, const string &b) const { return a + b; } };
    using Tree = SegmentTree<lng, Add>;

    void numeric(const vector<lng> &a, bool searches) {
        int n = int(a.size()); context = "array=" + show(a);
        Tree t(a, 0, Add{}), empty(n, 0, Add{});
        checkEqual(empty.allQuery(), 0, "identity-filled/allQuery");
        lng total = 0;
        for (int l = 0; l <= n; ++l) {
            lng sum = 0;
            for (int r = l; r <= n; ++r) {
                checkEqual(t.query(l, r), sum, "query(" + show(l) + "," + show(r) + ")");
                if (r < n) { sum += a[r]; }}
            if (l < n) { checkEqual(t.get(l), a[l], "get(" + show(l) + ")"); total += a[l]; }}
        checkEqual(t.allQuery(), total, "allQuery");
        if (searches) {
            for (lng cap = 0; cap <= total + 1; ++cap) {
                auto pred = [cap](lng x) { return x <= cap; };
                for (int l = 0; l <= n; ++l) {
                    int r = l; lng sum = 0;
                    while (r < n && sum + a[r] <= cap) { sum += a[r++]; }
                    checkEqual(t.maxRight(l, pred), r, "maxRight(" + show(l) + ",cap=" + show(cap) + ")"); }
                for (int r = 0; r <= n; ++r) {
                    int l = r; lng sum = 0;
                    while (l && a[l - 1] + sum <= cap) { sum += a[--l]; }
                    checkEqual(t.minLeft(r, pred), l, "minLeft(" + show(r) + ",cap=" + show(cap) + ")"); }}}
        Tree copy = t, moved = std::move(copy);
        checkEqual(moved.allQuery(), total, "move");
        if (n) {
            moved.set(0, -17); checkEqual(t.get(0), a[0], "copy independence");
            t.set(0, t.get(0)); checkEqual(t.get(0), a[0], "set/get alias");
            lng before = t.allQuery(); t.set(0, t.allQuery()); checkEqual(t.get(0), before, "set/aggregate alias"); }}

    void exhaustive() {
        int limit = mode == "quick" ? 4 : mode == "full" ? 6 : 8;
        for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 3; x /= 3; }
                numeric(a, true); }}
        numeric({-10, 0, 7, -4, 8}, false);
        numeric({std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()}, false);
        std::cout << "PASS bounded exhaustive range/point/search/copy tests\n";}

    void randomHistories(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 100 : 500;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 220);
            vector<lng> a(n); for (lng &x : a) { x = lng(rng() % 100); }
            Tree t(a, 0, Add{});
            for (int step = 0; step < 250; ++step) {
                context = "trial=" + show(trial) + " step=" + show(step) + " array=" + show(a);
                int p = int(rng() % n); lng value = lng(rng() % 100);
                t.set(p, value); a[p] = value;
                context += " update=(" + show(p) + "," + show(value) + ")";
                checkEqual(t.get(p), value, "set/get(" + show(p) + ")");
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (l > r) { std::swap(l, r); }
                lng sum = 0; for (int i = l; i < r; ++i) { sum += a[i]; }
                checkEqual(t.query(l, r), sum, "random query(" + show(l) + "," + show(r) + ")");
                lng cap = lng(rng() % (100 * n + 1));
                auto pred = [cap](lng x) { return x <= cap; };
                int right = l, left = r; sum = 0;
                while (right < n && sum + a[right] <= cap) { sum += a[right++]; }
                checkEqual(t.maxRight(l, pred), right, "random maxRight(" + show(l) + ",cap=" + show(cap) + ")");
                sum = 0;
                while (left && a[left - 1] + sum <= cap) { sum += a[--left]; }
                checkEqual(t.minLeft(r, pred), left, "random minLeft(" + show(r) + ",cap=" + show(cap) + ")");
                checkEqual(t.allQuery(), std::accumulate(a.begin(), a.end(), lng(0)), "random allQuery"); }}
        std::cout << "PASS seeded update/query/search histories\n";}

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
                a[i] = string(1, char('a' + rng() % 3)); affine[i] = {int(rng() % 97), int(rng() % 97)}; }
            SegmentTree s(a, string(), Join{});
            SegmentTree t(affine, Affine{1, 0}, Compose{});
            for (int step = 0; step < 3; ++step) {
                context = "trial=" + show(trial) + " step=" + show(step) + " strings=" + show(a);
                for (int l = 0; l <= n; ++l) {
                    string joined;
                    for (int r = l; r <= n; ++r) {
                        checkEqual(s.query(l, r), joined, "concatenation(" + show(l) + "," + show(r) + ")");
                        for (int x = 0; x < 3; ++x) {
                            int result = x;
                            for (int i = l; i < r; ++i) { result = affine[i].eval(result); }
                            checkEqual(t.query(l, r).eval(x), result, "affine evaluation(" + show(l) + "," + show(r) + ",x=" + show(x) + ")"); }
                        if (r < n) { joined += a[r]; }}
                    auto pred = [](const string &x) { return x.find("ab") == string::npos; };
                    int right = l, left = l; joined.clear();
                    while (right < n && pred(joined + a[right])) { joined += a[right++]; }
                    checkEqual(s.maxRight(l, pred), right, "ordered maxRight(" + show(l) + ")");
                    joined.clear();
                    while (left && pred(a[left - 1] + joined)) { joined = a[--left] + joined; }
                    checkEqual(s.minLeft(l, pred), left, "ordered minLeft(" + show(l) + ")"); }
                if (n) {
                    int p = int(rng() % n); a[p] = "abc"; affine[p] = {int(rng() % 97), int(rng() % 97)};
                    s.set(p, a[p]); t.set(p, affine[p]); }}}
        context = "non-default-constructible payload";
        SegmentTree b(vector<Box>{Box(4), Box(-3)}, Box(0), BoxAdd{});
        checkEqual(b.query(0, 2).x, 1, "Box fold"); b.set(1, Box(8)); checkEqual(b.allQuery().x, 12, "Box set");
        std::cout << "PASS noncommutative strings/affine/search and generic payloads\n";}

    void invalid(const string &name) {
        Tree t(vector<lng>{1, 2, 3}, 0, Add{});
        if (name == "negative-size") { Tree x(-1, 0, Add{}); }
        else if (name == "oversize") { Tree x(Tree::MAX_SIZE + 1, 0, Add{}); }
        else if (name == "vector-size") { (void)Tree::checkedSize(size_t(Tree::MAX_SIZE) + 1); }
        else if (name == "get-negative") { (void)t.get(-1); }
        else if (name == "get-end") { (void)t.get(3); }
        else if (name == "set-end") { t.set(3, 1); }
        else if (name == "query-negative") { (void)t.query(-1, 2); }
        else if (name == "query-reversed") { (void)t.query(2, 1); }
        else if (name == "query-end") { (void)t.query(0, 4); }
        else if (name == "max-right-index") { (void)t.maxRight(4, [](lng) { return true; }); }
        else if (name == "min-left-index") { (void)t.minLeft(-1, [](lng) { return true; }); }
        else if (name == "max-right-identity") { (void)t.maxRight(0, [](lng) { return false; }); }
        else if (name == "min-left-identity") { (void)t.minLeft(3, [](lng) { return false; }); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
}

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
        exhaustive(); randomHistories(rng); noncommutative(rng);
        std::cout << "PASS segmenttree checks=" << checks << '\n'; return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL segmenttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1; }
}
