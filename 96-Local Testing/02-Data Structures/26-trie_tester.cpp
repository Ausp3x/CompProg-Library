#include "../../02-Data Structures/26-trie.hpp"

namespace {
    ulng seed = 20261014;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) {
        if constexpr (std::is_integral_v<T>) { return std::to_string(x); }
        else { std::ostringstream s; s << x; return s.str(); }}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Domain of B-bit keys of T, written without the header's encoding.
    template<typename T, int B> struct Dom {
        using U = std::make_unsigned_t<T>;
        static constexpr int W = std::numeric_limits<U>::digits;
        static T lo() { if constexpr (std::is_signed_v<T>) { return B == W ? std::numeric_limits<T>::min() : T(-(lng(1) << (B - 1))); } else { return T(0); }}
        static T hi() {
            if constexpr (std::is_signed_v<T>) { return B == W ? std::numeric_limits<T>::max() : T((lng(1) << (B - 1)) - 1); }
            else { return B == W ? std::numeric_limits<T>::max() : T((ulng(1) << B) - 1); }}
        static T any(Rng &rng) {
            U r = U(rng());
            if (B < W) { r = U(r & ((U(1) << (B < W ? B : 0)) - 1)); }
            if constexpr (std::is_signed_v<T>) { return B == W ? T(r) : T(lng(r) - (lng(1) << (B - 1))); }
            else { return T(r); }}
        static vector<T> pool(Rng &rng) {
            vector<T> p{lo(), hi(), T(lo() / 2), T(hi() / 2), T(0)};
            if (hi() >= T(1)) { p.push_back(T(1)); }
            if constexpr (std::is_signed_v<T>) { p.push_back(T(-1)); }
            for (int i = 0; i < 4; ++i) { p.push_back(any(rng)); }
            return p;}
        static T pick(Rng &rng, const vector<T> &p) { return rng() % 3 ? p[rng() % p.size()] : any(rng); }
    };
    template<typename T> T bxor(T a, T b) { return T(a ^ b); }

    // xorMin and xorMax are checked for the first three arguments.
    template<typename T, int B>
    void checkAgainst(const BinaryTrie<T, B> &t, const std::multiset<T> &ref, const vector<T> &args) {
        checkEqual(t.size(), int(ref.size()), "size");
        vector<T> sorted(ref.begin(), ref.end());
        for (size_t i = 0; i < args.size(); ++i) {
            T a = args[i];
            checkEqual(t.count(a), int(ref.count(a)), "count " + show(a));
            checkEqual(t.countLess(a), int(std::lower_bound(sorted.begin(), sorted.end(), a) - sorted.begin()), "countLess " + show(a));
            T got = T(0);
            bool ok = t.lowerBound(a, got);
            auto it = ref.lower_bound(a);
            checkEqual(ok, it != ref.end(), "lowerBound status " + show(a));
            if (ok) { checkEqual(got, *it, "lowerBound " + show(a)); }
            if (!sorted.empty() && i < 3) {
                T mn = bxor(sorted[0], a), mx = mn;
                for (size_t j = 1; j < sorted.size(); ++j) { mn = min(mn, bxor(sorted[j], a)); mx = max(mx, bxor(sorted[j], a)); }
                checkEqual(t.xorMin(a), mn, "xorMin " + show(a)); checkEqual(t.xorMax(a), mx, "xorMax " + show(a));}}
        for (int k = 0; k < int(sorted.size()); k += 1 + int(sorted.size()) / 7) { checkEqual(t.kth(k), sorted[k], "kth " + show(k)); }
        if (!sorted.empty()) { checkEqual(t.kth(int(sorted.size()) - 1), sorted.back(), "kth last"); }}

    template<typename T, int B>
    void trieRandom(Rng &rng, int steps, const string &name) {
        using D = Dom<T, B>;
        vector<T> pool = D::pool(rng);
        BinaryTrie<T, B> t;
        std::multiset<T> ref;
        for (int s = 0; s < steps; ++s) {
            context = name + " step=" + show(s) + " size=" + show(ref.size());
            int op = int(rnd(rng, 0, 19));
            T a = D::pick(rng, pool);
            if (op < 9 && ref.size() < 200) {
                int c = int(rnd(rng, 0, 3));
                t.insert(a, c);
                for (int i = 0; i < c; ++i) { ref.insert(a); }}
            else if (op < 18) {
                if (!ref.empty() && rng() % 2) { a = *std::next(ref.begin(), lng(rng() % ref.size())); }
                int c = int(rnd(rng, 0, 3)), want = min(c, int(ref.count(a)));
                checkEqual(t.erase(a, c), want, "erase " + show(a) + " c=" + show(c));
                for (int i = 0; i < want; ++i) { ref.erase(ref.find(a)); }}
            else {
                T x = D::pick(rng, pool);
                t.applyXor(x);
                std::multiset<T> next;
                for (T v : ref) { next.insert(bxor(v, x)); }
                ref.swap(next);}
            vector<T> args{a, D::pick(rng, pool), D::any(rng), D::lo(), D::hi()};
            if (!ref.empty()) { args.push_back(*ref.begin()); args.push_back(*ref.rbegin()); }
            checkAgainst(t, ref, args);}}

    // Every multiset with multiplicities 0..2 over a 3-bit domain, under every global xor, against every query argument.
    template<typename T>
    void trieExhaustive(const string &name) {
        using D = Dom<T, 3>;
        vector<T> keys;
        for (lng v = lng(D::lo()); v <= lng(D::hi()); ++v) { keys.push_back(T(v)); }
        for (int mask = 0; mask < 6561; ++mask) {
            BinaryTrie<T, 3> t;
            std::multiset<T> ref;
            for (int i = 0, m = mask; i < 8; ++i, m /= 3) {
                if (m % 3) { t.insert(keys[i], m % 3); }
                for (int c = 0; c < m % 3; ++c) { ref.insert(keys[i]); }}
            for (T x : keys) {
                context = name + " exhaustive mask=" + show(mask) + " xor=" + show(x);
                BinaryTrie<T, 3> u = t;
                u.applyXor(x);
                std::multiset<T> r2;
                for (T v : ref) { r2.insert(bxor(v, x)); }
                checkAgainst(u, r2, keys);}}}

    // Prefix versions answer range queries over a[l, r) through (root[l], root[r]).
    template<typename T, int B>
    void persistentRange(Rng &rng, int n, int queries, const string &name) {
        using D = Dom<T, B>;
        vector<T> pool = D::pool(rng), a(n);
        PersistentBinaryTrie<T, B> t;
        vector<int> root{0};
        for (int i = 0; i < n; ++i) { a[i] = D::pick(rng, pool); root.push_back(t.insert(root.back(), a[i])); }
        for (int s = 0; s < queries; ++s) {
            int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
            if (l > r) { swap(l, r); }
            T x = D::pick(rng, pool), y = D::pick(rng, pool);
            context = name + " range query=" + show(s) + " l=" + show(l) + " r=" + show(r) + " x=" + show(x);
            vector<T> xs, raw(a.begin() + l, a.begin() + r);
            for (int i = l; i < r; ++i) { xs.push_back(bxor(a[i], x)); }
            sort(xs.begin(), xs.end());
            int u = root[l], v = root[r];
            checkEqual(t.size(u, v), r - l, "size");
            checkEqual(t.count(u, v, y), int(std::count(raw.begin(), raw.end(), y)), "count");
            checkEqual(t.countLess(u, v, y, x), int(std::lower_bound(xs.begin(), xs.end(), y) - xs.begin()), "countLess");
            checkEqual(t.countLess(u, v, y), int(std::count_if(raw.begin(), raw.end(), [&](T z) { return z < y; })), "countLess default xor");
            if (l < r) {
                checkEqual(t.xorMin(u, v, x), xs.front(), "xorMin"); checkEqual(t.xorMax(u, v, x), xs.back(), "xorMax");
                int k = int(rnd(rng, 0, r - l - 1));
                checkEqual(t.kth(u, v, k, x), xs[k], "kth " + show(k));
                sort(raw.begin(), raw.end());
                checkEqual(t.kth(u, v, k), raw[k], "kth default xor");}}}

    // Branching versions with inserts and erases; queries on (0, v) and on included pairs (u, v).
    template<typename T, int B>
    void persistentBranching(Rng &rng, int steps, const string &name) {
        using D = Dom<T, B>;
        vector<T> pool = D::pool(rng);
        PersistentBinaryTrie<T, B> t;
        vector<int> ver{0};
        vector<std::multiset<T>> ref(1);
        for (int s = 0; s < steps; ++s) {
            context = name + " branching step=" + show(s);
            int from = int(rng() % ver.size()), c = int(rnd(rng, 0, 3));
            T a = D::pick(rng, pool);
            std::multiset<T> m = ref[from];
            if (rng() % 3 && m.size() < 200) {
                ver.push_back(t.insert(ver[from], a, c));
                for (int i = 0; i < c; ++i) { m.insert(a); }}
            else {
                if (!m.empty() && rng() % 2) { a = *std::next(m.begin(), lng(rng() % m.size())); }
                int want = min(c, int(m.count(a)));
                int nv = t.erase(ver[from], a, c);
                if (!want) { checkEqual(nv, ver[from], "erase nothing keeps version"); }
                ver.push_back(nv);
                for (int i = 0; i < want; ++i) { m.erase(m.find(a)); }}
            ref.push_back(m);
            int v = int(rng() % ver.size()), u = int(rng() % ver.size());
            bool inc = std::includes(ref[v].begin(), ref[v].end(), ref[u].begin(), ref[u].end());
            if (!inc) { u = 0; }
            vector<T> diff;
            std::set_difference(ref[v].begin(), ref[v].end(), ref[u].begin(), ref[u].end(), std::back_inserter(diff));
            T x = D::pick(rng, pool), y = D::pick(rng, pool);
            vector<T> xs;
            for (T z : diff) { xs.push_back(bxor(z, x)); }
            sort(xs.begin(), xs.end());
            checkEqual(t.size(ver[u], ver[v]), int(diff.size()), "size");
            checkEqual(t.count(ver[u], ver[v], y), int(std::count(diff.begin(), diff.end(), y)), "count");
            checkEqual(t.countLess(ver[u], ver[v], y, x), int(std::lower_bound(xs.begin(), xs.end(), y) - xs.begin()), "countLess");
            if (!diff.empty()) {
                checkEqual(t.xorMin(ver[u], ver[v], x), xs.front(), "xorMin"); checkEqual(t.xorMax(ver[u], ver[v], x), xs.back(), "xorMax");
                int k = int(rng() % diff.size());
                checkEqual(t.kth(ver[u], ver[v], k, x), xs[k], "kth");}}}

    void edges() {
        context = "edges";
        BinaryTrie<int> t;
        int out = 7;
        checkEqual(t.size(), 0, "empty size"); checkEqual(t.count(5), 0, "empty count"); checkEqual(t.countLess(INT_MAX), 0, "empty countLess");
        checkEqual(t.lowerBound(INT_MIN, out), false, "empty lowerBound"); checkEqual(out, 7, "lowerBound leaves res");
        checkEqual(t.erase(3, 5), 0, "erase absent");
        t.insert(INT_MIN); t.insert(INT_MAX, 2); t.insert(-1); t.insert(0, 0);
        checkEqual(t.size(), 4, "size"); checkEqual(t.count(0), 0, "insert zero copies");
        checkEqual(t.kth(0), INT_MIN, "kth min"); checkEqual(t.kth(3), INT_MAX, "kth max");
        checkEqual(t.xorMax(0), INT_MAX, "xorMax"); checkEqual(t.xorMin(-1), INT_MIN, "xorMin -1 (INT_MAX ^ -1)");
        checkEqual(t.erase(INT_MAX, 9), 2, "erase all copies");
        BinaryTrie<int> copy = t;
        copy.applyXor(-1); copy.insert(5);
        checkEqual(t.size(), 2, "copy independent size"); checkEqual(t.kth(1), -1, "copy independent kth");
        checkEqual(copy.kth(0), 0, "-1 xor -1"); checkEqual(copy.kth(1), 5, "insert after xor"); checkEqual(copy.kth(2), INT_MAX, "INT_MIN xor -1");
        BinaryTrie<ulng> u;
        u.insert(~0ULL); u.insert(0);
        checkEqual(u.xorMin(~0ULL), 0ULL, "ulng xorMin"); checkEqual(u.xorMax(1), ~0ULL ^ 1, "ulng xorMax");
        checkEqual(u.countLess(~0ULL), 1, "ulng countLess max");
        PersistentBinaryTrie<lng> p;
        int v1 = p.insert(0, LLONG_MIN), v2 = p.insert(v1, LLONG_MAX), v3 = p.erase(v2, LLONG_MIN);
        checkEqual(p.size(0, v3), 1, "persistent erase"); checkEqual(p.size(0, v2), 2, "old version kept");
        checkEqual(p.kth(0, v3, 0), LLONG_MAX, "persistent kth"); checkEqual(p.xorMax(v1, v2, 0), LLONG_MAX, "difference");
        checkEqual(p.erase(v3, 5, 3), v3, "persistent erase absent");
        checkEqual(p.size(0, 0), 0, "empty version");}

    void invalid(const string &name) {
        BinaryTrie<uint, 4> t;
        BinaryTrie<int, 4> s;
        PersistentBinaryTrie<int, 4> p;
        if (name == "trie-insert-domain") { t.insert(16); }
        else if (name == "trie-signed-domain") { s.insert(8); }
        else if (name == "trie-signed-domain-low") { s.insert(-9); }
        else if (name == "trie-count-domain") { (void)t.count(16); }
        else if (name == "trie-countless-domain") { (void)t.countLess(99); }
        else if (name == "trie-applyxor-domain") { t.applyXor(16); }
        else if (name == "trie-xormin-empty") { (void)t.xorMin(1); }
        else if (name == "trie-xormax-empty") { (void)t.xorMax(1); }
        else if (name == "trie-kth-range") { t.insert(3); (void)t.kth(1); }
        else if (name == "trie-insert-negative") { t.insert(3, -1); }
        else if (name == "trie-erase-negative") { t.insert(3); (void)t.erase(3, -1); }
        else if (name == "trie-size-overflow") { t.insert(3, INT_MAX); t.insert(4, 1); }
        else if (name == "persistent-version-range") { (void)p.insert(5, 1); }
        else if (name == "persistent-domain") { (void)p.insert(0, 8); }
        else if (name == "persistent-kth-range") { int v = p.insert(0, 1); (void)p.kth(0, v, 1); }
        else if (name == "persistent-xormax-empty") { (void)p.xorMax(0, 0, 1); }
        else if (name == "persistent-not-included") { int v = p.insert(0, 1); (void)p.countLess(v, 0, 2); }
        else if (name == "trie-kth-negative") { t.insert(3); (void)t.kth(-1); }
        else if (name == "trie-xormin-domain") { t.insert(3); (void)t.xorMin(16); }
        else if (name == "trie-xormax-domain") { t.insert(3); (void)t.xorMax(16); }
        else if (name == "persistent-insert-negative") { (void)p.insert(0, 1, -1); }
        else if (name == "persistent-count-overflow") { int v = p.insert(0, 1, INT_MAX); (void)p.insert(v, 2, 1); }
        else if (name == "persistent-erase-negative") { int v = p.insert(0, 1); (void)p.erase(v, 1, -1); }
        else if (name == "persistent-xormin-empty") { int v = p.insert(0, 1); (void)p.xorMin(v, v, 1); }
        else if (name == "persistent-count-domain") { int v = p.insert(0, 1); (void)p.count(0, v, 8); }
        else if (name == "persistent-kth-mask-domain") { int v = p.insert(0, 1); (void)p.kth(0, v, 0, -9); }
        else if (name == "persistent-countless-domain") { int v = p.insert(0, 1); (void)p.countLess(0, v, 1, 8); }
        else if (name == "persistent-xormax-domain") { int v = p.insert(0, 1); (void)p.xorMax(0, v, 8); }
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
        std::cout << "trie seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        int steps = mode == "quick" ? 1500 : mode == "full" ? 20000 : 30000;
        edges();
        trieExhaustive<uint>("uint3"); trieExhaustive<int>("int3");
        trieRandom<uint, 4>(rng, steps, "uint4"); trieRandom<int, 5>(rng, steps, "int5");
        trieRandom<int, 32>(rng, steps, "int"); trieRandom<uint, 32>(rng, steps, "uint");
        trieRandom<lng, 64>(rng, steps, "lng"); trieRandom<ulng, 64>(rng, steps, "ulng");
        trieRandom<lng, 40>(rng, steps, "lng40"); trieRandom<int16_t, 9>(rng, steps, "int16_9");
        int n = mode == "quick" ? 60 : mode == "full" ? 400 : 800, rq = steps / 4, br = min(steps / 2, 20000);
        persistentRange<int, 6>(rng, n, rq, "int6"); persistentRange<uint, 4>(rng, n, rq, "uint4");
        persistentRange<lng, 64>(rng, n, rq, "lng"); persistentRange<ulng, 64>(rng, n, rq, "ulng");
        persistentBranching<int, 5>(rng, br, "int5"); persistentBranching<ulng, 64>(rng, br, "ulng");
        persistentBranching<lng, 64>(rng, br, "lng");
        std::cout << "PASS trie checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL trie seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
