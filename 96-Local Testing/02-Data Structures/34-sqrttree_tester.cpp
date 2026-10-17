#include "../../02-Data Structures/34-sqrttree.hpp"

namespace {
    ulng seed = 20261017;
    string mode = "full", context;
    lng checks = 0, ops = 0;
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    string show(uint8_t x) { return std::to_string(int(x)); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}
    void checkTrue(bool ok, const string &op) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + op + " failed"); }}

    // Affine maps x -> a x + b mod P; op(p, q) applies p first (noncommutative); ops counts calls.
    struct Affine {
        using S = pair<lng, lng>;
        static S op(const S &p, const S &q) { ++ops; return {q.first * p.first % P, (q.first * p.second + q.second) % P}; }
        static S e() { return {1, 0}; }
    };
    struct Concat {
        using S = string;
        static S op(const S &p, const S &q) { return p + q; }
        static S e() { return ""; }
    };
    struct Min {
        using S = int;
        static S op(S a, S b) { return min(a, b); }
        static S e() { return INT_MAX; }
    };
    // Affine maps mod 16 packed in one byte (a high nibble, b low): a cheap noncommutative monoid for huge n.
    struct Byte {
        using S = uint8_t;
        static S op(S p, S q) { ++ops; int pa = p >> 4, pb = p & 15, qa = q >> 4, qb = q & 15; return uint8_t((qa * pa % 16) << 4 | ((qa * pb + qb) % 16)); }
        static S e() { return 0x10; }
    };
    struct Or {
        using S = bool;
        static S op(S a, S b) { return a || b; }
        static S e() { return false; }
    };
    Affine::S value(Rng &rng, Affine *) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
    string value(Rng &rng, Concat *) { return string(1, char('a' + rnd(rng, 0, 25))) + (rng() % 4 ? "" : "z"); }
    int value(Rng &rng, Min *) { return int(rnd(rng, -5, 5)); }
    bool value(Rng &rng, Or *) { return rng() % 7 == 0; }
    uint8_t value(Rng &rng, Byte *) { return uint8_t(rng()); }

    template<typename M> typename M::S naive(const vector<typename M::S> &a, int l, int r) {
        typename M::S res = M::e();
        for (int i = l; i < r; ++i) { res = M::op(res, a[i]); }
        return res;}

    // Independent oracle for large n: bottom-up noncommutative segment tree.
    template<typename M>
    struct RefTree {
        int n;
        vector<typename M::S> t;
        explicit RefTree(const vector<typename M::S> &a) : n(int(a.size())), t(2 * a.size(), M::e()) {
            for (int i = 0; i < n; ++i) { t[n + i] = a[i]; }
            for (int i = n - 1; i > 0; --i) { t[i] = M::op(t[2 * i], t[2 * i + 1]); }}
        void set(int p, const typename M::S &x) {
            for (t[p += n] = x; p > 1; p >>= 1) { t[p >> 1] = (p & 1) ? M::op(t[p - 1], t[p]) : M::op(t[p], t[p + 1]); }}
        typename M::S query(int l, int r) const {
            typename M::S lo = M::e(), hi = M::e();
            for (l += n, r += n; l < r; l >>= 1, r >>= 1) {
                if (l & 1) { lo = M::op(lo, t[l++]); }
                if (r & 1) { hi = M::op(t[--r], hi); }}
            return M::op(lo, hi);}
    };

    // Every (l, r) for each n <= maxN, then point sets with all ranges rechecked against a left fold.
    template<typename M>
    void exhaustive(Rng &rng, int maxN, int updates, const string &name) {
        for (int n = 0; n <= maxN; ++n) {
            vector<typename M::S> a(n);
            for (auto &&x : a) { x = value(rng, (M *)nullptr); }
            SqrtTree<M> t(a);
            auto all = [&](const string &stage) {
                for (int l = 0; l <= n; ++l) {
                    for (int r = l; r <= n; ++r) {
                        context = name + " exhaustive n=" + show(n) + " " + stage + " l=" + show(l) + " r=" + show(r);
                        checkEqual(t.query(l, r), naive<M>(a, l, r), "query");}}};
            all("build");
            for (int s = 0; s < (n ? updates : 0); ++s) {
                int p = int(rnd(rng, 0, n - 1));
                a[p] = value(rng, (M *)nullptr);
                t.set(p, a[p]);
                all("set p=" + show(p));}}}

    // Interleaved sets and queries (short, long, block aligned) against a naive array.
    template<typename M>
    void randomMedium(Rng &rng, int rounds, int maxN, const string &name) {
        for (int s = 0; s < rounds; ++s) {
            int n = int(rnd(rng, 1, maxN));
            vector<typename M::S> a(n);
            for (auto &&x : a) { x = value(rng, (M *)nullptr); }
            SqrtTree<M> t(a);
            for (int k = 0; k < 60; ++k) {
                context = name + " medium round=" + show(s) + " n=" + show(n) + " step=" + show(k);
                if (rng() % 3 == 0) {
                    int p = int(rnd(rng, 0, n - 1));
                    a[p] = value(rng, (M *)nullptr); t.set(p, a[p]);}
                int l, r;
                int kind = int(rng() % 4);
                if (kind == 0) { l = int(rnd(rng, 0, n)); r = int(min<lng>(n, l + rnd(rng, 0, 9))); }
                else if (kind == 1) { int w = 1 << rnd(rng, 0, 12); l = int(rnd(rng, 0, n - 1)) / w * w; r = min(n, l + w); }
                else { l = int(rnd(rng, 0, n)); r = int(rnd(rng, l, n)); }
                checkEqual(t.query(l, r), naive<M>(a, l, r), "query l=" + show(l) + " r=" + show(r));}}}

    // Large affine trees around layer and block boundaries: reference segment tree oracle and op-count bounds.
    template<typename M>
    void large(Rng &rng, int n, int q) {
        context = "large n=" + show(n);
        vector<typename M::S> a(n);
        for (auto &x : a) { x = value(rng, (M *)nullptr); }
        ops = 0;
        SqrtTree<M> t(a);
        lng build = ops;
        RefTree<M> ref(a);
        lng cells = 0, update = 0;
        for (int i = 0; i < int(t.lay.size()); ++i) {
            int bs = (t.lay[i] + 1) / 2;
            cells += 3 * lng(t.n + t.m) + (1LL << t.lay[i]);
            update += 2 * (2LL << bs) + (i ? 2 * (1LL << t.lay[i]) : 0);}
        checkTrue(build <= cells, "build op count " + show(build) + " <= " + show(cells));
        checkTrue(update <= 16 * (1LL << ((t.lg + 1) / 2)) + 64, "update bound O(sqrt(n))");
        lng maxQuery = 0, maxSet = 0;
        for (int k = 0; k < q; ++k) {
            if (k % 5 == 0) {
                int p = int(rnd(rng, 0, n - 1));
                auto x = value(rng, (M *)nullptr);
                ops = 0; t.set(p, x); maxSet = max(maxSet, ops);
                ref.set(p, x);}
            int l, r;
            int kind = int(rng() % 5);
            if (kind == 0) { l = int(rnd(rng, 0, n)); r = int(min<lng>(n, l + rnd(rng, 0, 20))); }
            else if (kind == 1) { int w = 1 << rnd(rng, 0, t.lg); l = int(rnd(rng, 0, n - 1)) / w * w; r = min(n, l + w); }
            else if (kind == 2) { l = 0; r = n; }
            else { l = int(rnd(rng, 0, n)); r = int(rnd(rng, l, n)); }
            ops = 0;
            auto got = t.query(l, r);
            maxQuery = max(maxQuery, ops);
            checkEqual(got, ref.query(l, r), "query l=" + show(l) + " r=" + show(r));}
        checkTrue(maxQuery <= 9, "query op count " + show(maxQuery) + " <= 9");
        checkTrue(maxSet <= update, "set op count " + show(maxSet) + " <= " + show(update));}

    void refTreeSelfCheck(Rng &rng) {
        for (int n = 0; n <= 40; ++n) {
            vector<Affine::S> a(n);
            for (auto &x : a) { x = value(rng, (Affine *)nullptr); }
            RefTree<Affine> ref(a);
            for (int k = 0; k < 3 && n; ++k) { int p = int(rnd(rng, 0, n - 1)); a[p] = value(rng, (Affine *)nullptr); ref.set(p, a[p]); }
            for (int l = 0; l <= n; ++l) {
                for (int r = l; r <= n; ++r) {
                    context = "reference self-check n=" + show(n) + " l=" + show(l) + " r=" + show(r);
                    checkEqual(ref.query(l, r), naive<Affine>(a, l, r), "reference");}}}}

    void edges() {
        context = "edges";
        SqrtTree<Min> e(vector<int>{});
        checkEqual(e.query(0, 0), INT_MAX, "empty tree");
        SqrtTree<Concat> c(vector<string>{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"});
        checkEqual(c.query(1, 16), string("bcdefghijklmnop"), "concat order");
        for (int l = 0; l <= 17; ++l) { checkEqual(c.query(l, l), string(), "identity on empty"); }
        SqrtTree<Concat> copy = c;
        copy.set(3, "X");
        checkEqual(copy.query(0, 5), string("abcXe"), "copy set"); checkEqual(c.query(0, 5), string("abcde"), "original unchanged");
        c.set(4, c.v[0]);
        checkEqual(c.query(3, 6), string("daf"), "aliased set");
        c.set(4, "e"); c.set(4, "e");
        checkEqual(c.query(0, 17), string("abcdefghijklmnopq"), "repeated set");}

    void invalid(const string &name) {
        SqrtTree<Min> t(vector<int>{3, 1, 4});
        if (name == "query-reversed") { (void)t.query(2, 1); }
        else if (name == "query-range") { (void)t.query(0, 4); }
        else if (name == "query-negative") { (void)t.query(-1, 2); }
        else if (name == "set-range") { t.set(3, 0); }
        else if (name == "set-negative") { t.set(-1, 0); }
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
        std::cout << "sqrttree seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        refTreeSelfCheck(rng);
        int small = mode == "quick" ? 24 : mode == "full" ? 70 : 140;
        exhaustive<Affine>(rng, small, 6, "affine"); exhaustive<Concat>(rng, small / 2, 4, "concat");
        exhaustive<Min>(rng, small, 6, "min"); exhaustive<Or>(rng, small, 6, "or");
        int rounds = mode == "quick" ? 30 : mode == "full" ? 400 : 3000;
        randomMedium<Affine>(rng, rounds, 5000, "affine"); randomMedium<Concat>(rng, rounds / 4, 700, "concat");
        randomMedium<Or>(rng, rounds / 4, 3000, "or");
        for (int n : {1, 2, 3, 8, 9, 16, 17, 31, 32, 33, 64, 65, 255, 256, 257, 4095, 4096, 4097}) { large<Affine>(rng, n, 400); large<Byte>(rng, n, 400); }
        if (mode != "quick") { for (int n : {65535, 65536, 65537, 100003}) { large<Affine>(rng, n, 20000); } }
        if (mode == "stress") { large<Affine>(rng, 1 << 20, 200000); large<Affine>(rng, (1 << 20) + 1, 100000); large<Affine>(rng, (1 << 22) - 3, 50000); large<Byte>(rng, (1 << 24) + 1, 100000); }
        std::cout << "PASS sqrttree checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL sqrttree seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
