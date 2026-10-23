#include "../../02-Data Structures/36-fastset.hpp"

namespace {
    ulng seed = 20261023;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}

    // Brute force: std::set with lower_bound and upper_bound.
    struct Brute {
        int n;
        std::set<int> s;
        bool has(int x) const { return s.count(x) > 0; }
        int next(int x) const {
            if (max(x, 0) >= n) { return n; }
            auto it = s.lower_bound(max(x, 0));
            return it == s.end() ? n : *it;}
        int prev(int x) const {
            if (min(x, n - 1) < 0) { return -1; }
            auto it = s.upper_bound(min(x, n - 1));
            return it == s.begin() ? -1 : *std::prev(it);}
        vector<int> keys(int l, int r) const { return vector<int>(s.lower_bound(l), s.lower_bound(r)); }
    };
    // Sizes around the 64-ary level boundaries.
    vector<int> sizes(bool big) {
        vector<int> res{0, 1, 2, 63, 64, 65, 127, 128, 129, 200, 4095, 4096, 4097};
        if (big) { res.insert(res.end(), {262143, 262144, 262145}); }
        return res;}
    vector<int> probes(Rng &rng, int n) {
        vector<int> res{INT_MIN, -64, -2, -1, 0, 1, n - 2, n - 1, n, n + 1, n + 64, INT_MAX};
        for (int k = 0; k < 8; ++k) { res.push_back(int(rnd(rng, 0, max(n - 1, 0)))); }
        for (int b = 64; b <= n; b *= 64) { res.insert(res.end(), {b - 1, b, b + 1}); }
        return res;}

    template<typename D>
    void checkQueries(Rng &rng, D &d, const Brute &b, bool all) {
        int n = b.n;
        vector<int> xs = probes(rng, n);
        if (all) { for (int x = -1; x <= n; ++x) { xs.push_back(x); } }
        for (int x : xs) {
            checkEqual(d.next(x), b.next(x), "next(" + show(x) + ")");
            checkEqual(d.prev(x), b.prev(x), "prev(" + show(x) + ")");
            if (0 <= x && x < n) { checkEqual(d.contains(x), b.has(x), "contains(" + show(x) + ")"); }}
        int l = int(rnd(rng, 0, n)), r = int(rnd(rng, l, min(n, l + 4096)));
        vector<int> got;
        d.enumerate(l, r, [&](int x) { got.push_back(x); });
        checkEqual(got, b.keys(l, r), "enumerate(" + show(l) + "," + show(r) + ")");}

    void checkFast(Rng &rng, FastSet &d, const Brute &b, bool all) {
        int cnt = int(b.s.size());
        checkEqual(d.size(), cnt, "size"); checkEqual(d.empty(), cnt == 0, "empty");
        checkEqual(d.min(), b.next(0), "min"); checkEqual(d.max(), b.prev(b.n - 1), "max");
        checkQueries(rng, d, b, all);}

    void fastHistories(Rng &rng, bool big, int steps) {
        for (int n : sizes(big)) {
            for (int density : {0, 1, 50, 99, 100}) {
                context = "FastSet n=" + show(n) + " density=" + show(density) + " seed=" + show(seed);
                Brute b{n, {}};
                for (int x = 0; x < n; ++x) {
                    if (int(rnd(rng, 0, 99)) < density) { b.s.insert(b.s.end(), x); }}
                FastSet d(n, [&](int x) { return b.has(x); });
                bool all = n <= 300;
                checkFast(rng, d, b, all);
                // Small sizes are checked exhaustively after every step; only larger ones grow with the mode.
                for (int step = 0; step < (all ? min(steps, 3000) : steps) && n; ++step) {
                    context = "FastSet n=" + show(n) + " density=" + show(density) + " step=" + show(step) + " seed=" + show(seed);
                    int x = int(rnd(rng, 0, n - 1)), kind = int(rng() % 5);
                    if (kind < 2) { checkEqual(d.insert(x), b.s.insert(x).second, "insert(" + show(x) + ")"); }
                    else if (kind < 4) { checkEqual(d.erase(x), b.s.erase(x) == 1, "erase(" + show(x) + ")"); }
                    else {
                        // Erase while enumerating a range.
                        int l = int(rnd(rng, 0, n)), r = int(rnd(rng, l, min(n, l + 300)));
                        vector<int> got, want = b.keys(l, r);
                        d.enumerate(l, r, [&](int y) { got.push_back(y); d.erase(y); });
                        for (int y : want) { b.s.erase(y); }
                        checkEqual(got, want, "enumerate-erase(" + show(l) + "," + show(r) + ")");}
                    if (all || step % 32 == 0) { checkFast(rng, d, b, all); }}
                checkFast(rng, d, b, all);}}
        std::cout << "PASS FastSet\n";}

    void decrementalHistories(Rng &rng, bool big, int steps) {
        for (int n : sizes(big)) {
            for (int order = 0; order < 4; ++order) {
                context = "DecrementalSet n=" + show(n) + " order=" + show(order) + " seed=" + show(seed);
                Brute b{n, {}};
                for (int x = 0; x < n; ++x) { b.s.insert(b.s.end(), x); }
                DecrementalSet d(n);
                bool all = n <= 300;
                checkQueries(rng, d, b, all);
                // Orders: random keys, ascending, descending, whole words in random word order.
                vector<int> seq(n);
                iota(seq.begin(), seq.end(), 0);
                if (order == 0) { std::shuffle(seq.begin(), seq.end(), rng); }
                if (order == 2) { reverse(seq.begin(), seq.end()); }
                if (order == 3) {
                    vector<int> words(n / 64 + 1);
                    iota(words.begin(), words.end(), 0); std::shuffle(words.begin(), words.end(), rng);
                    seq.clear();
                    for (int w : words) { for (int x = 64 * w; x < min(n, 64 * w + 64); ++x) { seq.push_back(x); } }}
                int limit = min(n, all ? n : steps);
                for (int step = 0; step < limit; ++step) {
                    context = "DecrementalSet n=" + show(n) + " order=" + show(order) + " step=" + show(step) + " seed=" + show(seed);
                    int x = seq[step];
                    if (rng() % 8 == 0) {
                        int l = int(rnd(rng, 0, n)), r = int(rnd(rng, l, min(n, l + 200)));
                        vector<int> got, want = b.keys(l, r);
                        d.enumerate(l, r, [&](int y) { got.push_back(y); d.erase(y); });
                        for (int y : want) { b.s.erase(y); }
                        checkEqual(got, want, "enumerate-erase(" + show(l) + "," + show(r) + ")");}
                    checkEqual(d.erase(x), b.s.erase(x) == 1, "erase(" + show(x) + ")");
                    if (all || step % 64 == 0) { checkQueries(rng, d, b, all); }}
                checkQueries(rng, d, b, all);}}
        std::cout << "PASS DecrementalSet\n";}

    // Large: std::set as the oracle.
    void large(Rng &rng, int n, int q) {
        context = "large FastSet n=" + show(n) + " seed=" + show(seed);
        FastSet f(n);
        std::set<int> s;
        for (int step = 0; step < q; ++step) {
            int x = int(rnd(rng, 0, n - 1));
            int kind = int(rng() % 4);
            if (kind == 0) { checkEqual(f.insert(x), s.insert(x).second, "insert"); }
            else if (kind == 1) { checkEqual(f.erase(x), s.erase(x) == 1, "erase"); }
            else if (kind == 2) { auto it = s.lower_bound(x); checkEqual(f.next(x), it == s.end() ? n : *it, "next"); }
            else { auto it = s.upper_bound(x); checkEqual(f.prev(x), it == s.begin() ? -1 : *std::prev(it), "prev"); }}
        checkEqual(f.size(), int(s.size()), "size");
        context = "large DecrementalSet n=" + show(n) + " seed=" + show(seed);
        DecrementalSet d(n);
        std::set<int> t;
        for (int x = 0; x < n; ++x) { t.insert(t.end(), x); }
        for (int step = 0; step < q; ++step) {
            int x = int(rnd(rng, 0, n - 1));
            int kind = int(rng() % 3);
            if (kind == 0) { checkEqual(d.erase(x), t.erase(x) == 1, "erase"); }
            else if (kind == 1) { auto it = t.lower_bound(x); checkEqual(d.next(x), it == t.end() ? n : *it, "next"); }
            else { auto it = t.upper_bound(x); checkEqual(d.prev(x), it == t.begin() ? -1 : *std::prev(it), "prev"); }}}

    void edges() {
        context = "edges";
        FastSet z;
        checkEqual(z.min(), 0, "empty min is n"); checkEqual(z.max(), -1, "empty max"); checkEqual(z.empty(), true, "empty");
        DecrementalSet dz;
        checkEqual(dz.next(INT_MIN), 0, "empty next"); checkEqual(dz.prev(INT_MAX), -1, "empty prev");
        FastSet f(130);
        f.insert(129); f.insert(0);
        FastSet copy = f;
        copy.erase(129);
        checkEqual(f.max(), 129, "copy independent"); checkEqual(copy.max(), 0, "copy erase");
        checkEqual(f.insert(0), false, "duplicate insert"); checkEqual(f.size(), 2, "size after duplicate");
        int calls = 0;
        f.enumerate(0, 0, [&](int) { ++calls; }); f.enumerate(130, 130, [&](int) { ++calls; });
        checkEqual(calls, 0, "empty ranges");
        // The predicate build calls f once per key in increasing order.
        vector<int> asked;
        FastSet g(130, [&](int x) { asked.push_back(x); return x % 3 == 0; });
        vector<int> all(130);
        iota(all.begin(), all.end(), 0);
        checkEqual(asked, all, "build calls"); checkEqual(g.size(), 44, "build size");
        // enumerate is a next loop: f may erase smaller keys and insert keys ahead.
        FastSet h(200);
        h.insert(5); h.insert(10);
        vector<int> seen;
        h.enumerate(6, 200, [&](int x) { seen.push_back(x); if (x == 10) { h.erase(5); h.insert(150); } });
        checkEqual(seen, vector<int>{10, 150}, "insert ahead is visited"); checkEqual(h.min(), 10, "smaller key erased");
        std::cout << "PASS edges\n";}

    void invalid(const string &name) {
        FastSet f(70);
        DecrementalSet d(70);
        auto none = [](int) {};
        if (name == "insert-range") { f.insert(70); }
        else if (name == "erase-negative") { f.erase(-1); }
        else if (name == "contains-range") { (void)f.contains(70); }
        else if (name == "enumerate-reversed") { f.enumerate(5, 4, none); }
        else if (name == "enumerate-past") { f.enumerate(0, 71, none); }
        else if (name == "size-negative") { FastSet bad(-1); }
        else if (name == "decremental-erase-range") { d.erase(70); }
        else if (name == "decremental-contains-negative") { (void)d.contains(-1); }
        else if (name == "decremental-enumerate-past") { d.enumerate(0, 71, none); }
        else if (name == "decremental-enumerate-reversed") { d.enumerate(3, 2, none); }
        else if (name == "decremental-size-negative") { DecrementalSet bad(-1); }
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
        std::cout << "fastset seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        int steps = mode == "quick" ? 300 : mode == "full" ? 3000 : 20000;
        fastHistories(rng, mode != "quick", steps);
        decrementalHistories(rng, mode != "quick", steps);
        if (mode != "quick") { large(rng, 1000000, 2000000); large(rng, 1 << 18, 1000000); }
        if (mode == "stress") { large(rng, 1 << 22, 8000000); }
        std::cout << "PASS fastset checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL fastset seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
