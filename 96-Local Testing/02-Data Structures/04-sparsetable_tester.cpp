#include "../../02-Data Structures/04-sparsetable.hpp"

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
    struct Minimum { lng operator()(lng a, lng b) const { return std::min(a, b); } };
    struct Maximum { lng operator()(lng a, lng b) const { return std::max(a, b); } };
    struct Add { lng operator()(lng a, lng b) const { return a + b; } };
    struct Join { string operator()(const string &a, const string &b) const { return a + b; } };
    using Table = SparseTable<lng, Minimum>;

    void numeric(const vector<lng> &a, bool sums = true) {
        int n = int(a.size()); context = "array=" + show(a);
        Table t(a, Minimum{}); SparseTable mx(a, Maximum{});
        SparseTable sum(a, Add{});
        checkEqual(t.n, n, "size"); checkEqual(t.h, int(std::bit_width(uint(n))), "height");
        for (int l = 0; l < n; ++l) {
            lng low = a[l], high = a[l], total = 0;
            for (int r = l + 1; r <= n; ++r) {
                low = std::min(low, a[r - 1]); high = std::max(high, a[r - 1]);
                string range = "(" + show(l) + "," + show(r) + ")";
                checkEqual(t.query(l, r), low, "query" + range);
                checkEqual(t.fold(l, r), low, "fold" + range);
                checkEqual(t.queryFast(l, r - 1), low, "inclusive queryFast" + range);
                checkEqual(t.querySlow(l, r - 1), low, "inclusive querySlow" + range);
                checkEqual(mx.query(l, r), high, "maximum query" + range);
                if (sums) {
                    total += a[r - 1];
                    checkEqual(sum.fold(l, r), total, "associative sum fold" + range);
                    checkEqual(sum.querySlow(l, r - 1), total, "inclusive sum querySlow" + range); }}}
        Table copied = t, moved = std::move(copied);
        if (n) { checkEqual(moved.query(0, n), *std::min_element(a.begin(), a.end()), "copy/move"); }
        else {
            std::ostringstream s; s << t;
            checkEqual(s.str(), string("[]"), "empty output"); }
        std::ostringstream got, want;
        got << t;
        if (!n) { want << "[]"; }
        else {
            want << "\n[\n";
            for (int h = t.h - 1; h >= 0; --h) {
                want << "  [";
                for (int l = 0; l <= n - (1 << h); ++l) {
                    lng low = a[l];
                    for (int r = l + 1; r < l + (1 << h); ++r) { low = std::min(low, a[r]); }
                    want << "[" << l << ", " << l + (1 << h) - 1 << "]: " << low
                         << (l < n - (1 << h) ? ", " : ""); }
                want << "]" << (h ? ",\n" : "\n"); }
            want << "]\n"; }
        checkEqual(got.str(), want.str(), "legacy diagnostic output");}

    void exhaustive() {
        int limit = mode == "quick" ? 4 : mode == "full" ? 7 : 9;
        for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 3 - 1; x /= 3; }
                numeric(a); }}
        numeric({std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()});
        numeric({17});
        std::cout << "PASS bounded exhaustive idempotent/associative/legacy/output tests\n";}

    void randomized(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 80 : 300;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = int(rng() % 150); vector<lng> a(n);
            for (lng &x : a) { x = lng(rng() % 2000000001) - 1000000000; }
            numeric(a);
            if (n) {
                context = "trial=" + show(trial) + " original=" + show(a);
                Table t(a, Minimum{}); lng low = *std::min_element(a.begin(), a.end());
                a.assign(n, 7000000001LL);
                checkEqual(t.query(0, n), low, "input ownership"); }}
        for (int n : {1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129}) {
            vector<lng> a(n); std::iota(a.begin(), a.end(), lng(-n)); numeric(a);
            std::reverse(a.begin(), a.end()); numeric(a); }
        std::cout << "PASS seeded arrays, input ownership and power-of-two boundaries\n";}

    struct Affine {
        int a, b;
        int eval(int x) const { return (a * x + b) % 97; }
    };
    struct Compose {
        Affine operator()(Affine x, Affine y) const { return {y.a * x.a % 97, (y.a * x.b + y.b) % 97}; }
    };
    struct Ends {
        int first, last;
        Ends() = delete;
        Ends(int first, int last) : first(first), last(last) {}
    };
    // Rectangular band: associative/idempotent, explicitly noncommutative.
    struct Band { Ends operator()(Ends a, Ends b) const { return Ends(a.first, b.last); } };
    struct CopyOnlyMin {
        CopyOnlyMin() = default;
        CopyOnlyMin(const CopyOnlyMin &) = default;
        CopyOnlyMin(CopyOnlyMin &&) = delete;
        int operator()(int a, int b) const { return std::min(a, b); }
    };

    void noncommutative(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 70 : 250;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 45);
            vector<string> a(n); vector<Affine> affine(n); vector<Ends> ends;
            for (int i = 0; i < n; ++i) {
                a[i] = string(1, char('a' + rng() % 5)); affine[i] = {int(rng() % 97), int(rng() % 97)};
                ends.emplace_back(i, n - i); }
            SparseTable text(a, Join{}); SparseTable composed(affine, Compose{}); SparseTable band(ends, Band{});
            context = "trial=" + show(trial) + " strings=" + show(a);
            for (int l = 0; l < n; ++l) {
                string joined;
                for (int r = l + 1; r <= n; ++r) {
                    joined += a[r - 1]; string range = "(" + show(l) + "," + show(r) + ")";
                    checkEqual(text.fold(l, r), joined, "ordered concatenation" + range);
                    checkEqual(text.querySlow(l, r - 1), joined, "inclusive concatenation" + range);
                    for (int x = 0; x < 3; ++x) {
                        int result = x;
                        for (int i = l; i < r; ++i) { result = affine[i].eval(result); }
                        checkEqual(composed.fold(l, r).eval(x), result, "affine fold" + range + " x=" + show(x)); }
                    checkEqual(band.query(l, r).first, l, "idempotent noncommutative first" + range);
                    checkEqual(band.query(l, r).last, n - r + 1, "idempotent noncommutative last" + range);
                    checkEqual(band.fold(l, r).first, l, "nondefault payload fold first" + range);
                    checkEqual(band.fold(l, r).last, n - r + 1, "nondefault payload fold last" + range); }}}
        context = "copy-only legacy callable";
        CopyOnlyMin f;
        SparseTable copied(vector<int>{3, 1, 2}, f);
        checkEqual(copied.query(0, 3), 1, "copy-only callable construction/query");
        std::cout << "PASS strings/affine and idempotent noncommutative generic payloads\n";}

    void invalid(const string &name) {
        Table t(vector<lng>{3, 1, 2}, Minimum{});
        if (name == "query-empty") { (void)t.query(1, 1); }
        else if (name == "query-negative") { (void)t.query(-1, 2); }
        else if (name == "query-end") { (void)t.query(0, 4); }
        else if (name == "query-reversed") { (void)t.query(2, 1); }
        else if (name == "fold-empty") { (void)t.fold(0, 0); }
        else if (name == "fold-negative") { (void)t.fold(-1, 2); }
        else if (name == "fold-end") { (void)t.fold(0, 4); }
        else if (name == "fast-reversed") { (void)t.queryFast(2, 1); }
        else if (name == "fast-end") { (void)t.queryFast(0, 3); }
        else if (name == "slow-reversed") { (void)t.querySlow(2, 1); }
        else if (name == "slow-end") { (void)t.querySlow(0, 3); }
        else if (name == "empty-table") { Table e(vector<lng>{}, Minimum{}); (void)e.query(0, 1); }
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
        std::cout << "sparsetable seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        std::mt19937_64 rng(seed);
        exhaustive(); randomized(rng); noncommutative(rng);
        std::cout << "PASS sparsetable checks=" << checks << '\n'; return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL sparsetable seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1; }
}
