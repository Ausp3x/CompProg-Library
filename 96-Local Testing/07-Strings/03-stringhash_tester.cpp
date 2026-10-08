#include "../../07-Strings/03-stringhash.hpp"
#include "01-prefix_z_test_support.hpp"

constexpr ulng P61 = (1ULL << 61) - 1;
template<int K> using Word = typename StringHash<K>::Word;
template<int K> vector<ulll> moduli() {
    if constexpr (K == 0) { return {1000000007, 1000000009}; }
    else if constexpr (K == 1) { return {ulll(1) << 64}; }
    else { return {P61}; }}
template<int K> vector<ulng> parts(Word<K> w) {
    if constexpr (K == 0) { return {w[0], w[1]}; }
    else { return {w}; }}
template<int K> Word<K> make(const vector<ulng> &v) {
    if constexpr (K == 0) { return {uint(v[0]), uint(v[1])}; }
    else { return v[0]; }}
string name(int k) { return k == 0 ? "double" : k == 1 ? "wrap64" : "mersenne61"; }
// Independent Horner over 128-bit integers with every code reduced modulo each field.
template<int K> vector<ulng> referenceCodes(const vector<ulng> &codes, int l, int r, Word<K> base) {
    auto mods = moduli<K>();
    auto b = parts<K>(base);
    vector<ulng> out(mods.size());
    for (int j = 0; j < int(mods.size()); ++j) {
        ulll h = 0;
        for (int i = l; i < r; ++i) { h = (h * b[j] + codes[i] % mods[j]) % mods[j]; }
        out[j] = ulng(h);}
    return out;}
template<int K> vector<ulng> reference(const vector<uint> &s, int l, int r, Word<K> base) {
    vector<ulng> codes;
    for (uint x : s) { codes.push_back(ulng(x) + 1); }
    return referenceCodes<K>(codes, l, r, base);}
template<int K> void values(const vector<uint> &s, Word<K> base) {
    using H = StringHash<K>;
    H h(s, base);
    int n = int(s.size());
    context = name(K) + " symbols=" + show(s);
    check(h.size(), n, "size");
    auto copy = h;
    H moved(std::move(copy));
    copy = h;
    check(moved.get(0, n) == copy.get(0, n), true, "copy-move-assignment");
    for (int l = 0; l <= n; ++l) {
        for (int r = l; r <= n; ++r) {
            auto d = h.get(l, r);
            check(parts<K>(d.value), reference<K>(s, l, r, base), "substring-arithmetic");
            check(d.length, r - l, "length-tag"); check(d.base == base, true, "base-tag");
            vector<uint> sub(s.begin() + l, s.begin() + r), rev(sub.rbegin(), sub.rend());
            check(parts<K>(h.reverseGet(l, r).value), reference<K>(rev, 0, int(rev.size()), base), "reverse-arithmetic");
            check(h.hashOf(sub) == d, true, "hashOf-vector");
            int m = (l + r) / 2;
            check(h.concat(h.get(l, m), h.get(m, r)) == d, true, "concatenation");
            check(h.lcp(h, l, r, l, r), r - l, "self-lcp"); check(h.lcs(h, l, r, l, r), r - l, "self-lcs");}}
    auto twice = h.concat(h.get(0, n), h.get(0, n));
    auto three = h.concat(h.get(0, n), twice);
    vector<uint> triple(s);
    triple.insert(triple.end(), s.begin(), s.end()); triple.insert(triple.end(), s.begin(), s.end());
    check(parts<K>(three.value), reference<K>(triple, 0, int(triple.size()), base), "long-right-power");
    check(h.power(3 * n + 1) == h.multiply(h.power(3 * n), base), true, "power-beyond-table");
    auto different_length = h.get(0, 0);
    ++different_length.length;
    check(h.get(0, 0) == different_length, false, "different-length-rejection");
    auto other_base = base;
    if constexpr (K == 0) { other_base[0] = base[0] == 257 ? 258 : 257; }
    else if constexpr (K == 1) { other_base = base == 257 ? 259 : 257; }
    else { other_base = base == 257 ? 258 : 257; }
    H other(s, other_base);
    check(h.get(0, 0) == other.get(0, 0), false, "different-base-rejection");
    check(parts<K>(h.get(0, n).value), reference<K>(s, 0, n, base), "independent-context-lifetime");}
template<int K> void queries(const vector<uint> &s, const vector<uint> &t) {
    using H = StringHash<K>;
    auto base = H::randomBase(rng());
    H h(s, base), g(t, base);
    context = name(K) + " s=" + show(s) + " t=" + show(t);
    for (int rep = 0; rep < 100; ++rep) {
        int l = int(rng() % (s.size() + 1)), r = int(rng() % (s.size() + 1));
        if (l > r) { swap(l, r); }
        int a = int(rng() % (t.size() + 1)), b = int(rng() % (t.size() + 1));
        if (a > b) { swap(a, b); }
        int p = 0, q = 0;
        while (p < min(r - l, b - a) && s[l + p] == t[a + p]) { ++p; }
        while (q < min(r - l, b - a) && s[r - 1 - q] == t[b - 1 - q]) { ++q; }
        check(h.lcp(g, l, r, a, b), p, "direct-lcp"); check(h.lcs(g, l, r, a, b), q, "direct-lcs");}}
// encode/add/subtract/multiply against 128-bit modular references on boundary residues.
template<int K> void helpers() {
    using H = StringHash<K>;
    auto mods = moduli<K>();
    context = name(K) + " arithmetic helpers";
    vector<vector<ulng>> residues(mods.size());
    for (int j = 0; j < int(mods.size()); ++j) {
        ulng m1 = ulng(mods[j] - 1);
        residues[j] = {0, 1, 2, 256, m1 / 2, m1 - 1, m1};
        for (int i = 0; i < 6; ++i) { residues[j].push_back(ulng(ulll(rng()) % mods[j])); }}
    int r = int(residues[0].size());
    for (int x = 0; x < r * r; ++x) {
        for (int y = 0; y < r * r; ++y) {
            vector<ulng> a, b, sum, diff, prod;
            for (int j = 0; j < int(mods.size()); ++j) {
                ulng u = residues[j][(x + j) % r], v = residues[j][(y + 3 * j) % r];
                a.push_back(u); b.push_back(v);
                sum.push_back(ulng((ulll(u) + v) % mods[j]));
                diff.push_back(ulng((ulll(u) + mods[j] - v) % mods[j]));
                prod.push_back(ulng(ulll(u) * v % mods[j]));}
            check(parts<K>(H::add(make<K>(a), make<K>(b))), sum, "add");
            check(parts<K>(H::subtract(make<K>(a), make<K>(b))), diff, "subtract");
            check(parts<K>(H::multiply(make<K>(a), make<K>(b))), prod, "multiply");}}
    vector<ulng> codes{0, 1, 1000000006, 1000000007, 1000000008, 1000000009, UINT_MAX, 1ULL << 32, (1ULL << 32) + 1,
                       P61 - 1, P61, P61 + 1, ~0ULL - 1, ~0ULL};
    for (int i = 0; i < 20; ++i) { codes.push_back(rng()); }
    for (ulng c : codes) {
        vector<ulng> expected;
        for (auto m : mods) { expected.push_back(ulng(c % m)); }
        check(parts<K>(H::encode(c)), expected, "encode-reduces " + std::to_string(c));}
    for (int rep = 0; rep < limit(30, 300, 2000); ++rep) {
        vector<ulng> s(rng() % 15);
        for (auto &c : s) { c = codes[rng() % codes.size()]; }
        H h;
        h.base = H::randomBase(rng());
        h.build(int(s.size()), [&](int i) { return s[i]; });
        int n = int(s.size());
        context = name(K) + " build codes=" + show(s);
        check(h.size(), n, "build-size");
        for (int l = 0; l <= n; ++l) {
            for (int rr = l; rr <= n; ++rr) {
                vector<ulng> rev(s.rbegin() + (n - rr), s.rbegin() + (n - l));
                check(parts<K>(h.get(l, rr).value), referenceCodes<K>(s, l, rr, h.base), "build-get");
                check(parts<K>(h.reverseGet(l, rr).value), referenceCodes<K>(rev, 0, rr - l, h.base), "build-reverseGet");}}}
    H a, b;
    a.build(1, [](int) { return (1ULL << 32) + 1; }); b.build(1, [](int) { return 1ULL; });
    context = name(K) + " finding-3 regression";
    check(a.get(0, 1) == b.get(0, 1), false, "2^32+1-versus-1");}
template<int K> void contexts() {
    using H = StringHash<K>;
    auto mods = moduli<K>();
    context = name(K) + " random bases";
    for (int i = 0; i < 2000; ++i) {
        auto b = parts<K>(H::randomBase(rng()));
        for (int j = 0; j < int(b.size()); ++j) {
            check(b[j] >= 257 && (K == 1 ? b[j] % 2 == 1 : ulll(b[j]) <= mods[j] - (K == 2 ? 2 : 1)), true, "randomBase-range");}}
    check(H::randomBase(seed) == H::randomBase(seed), true, "seed-repeatability");
    string bytes;
    vector<uint> encoded;
    for (int i = 0; i < 256; ++i) { bytes += char(i); encoded.push_back(uint(i)); }
    H h(bytes);
    check(h.get(0, 256) == H(encoded).get(0, 256), true, "unsigned-byte-encoding");
    check(h.hashOf(bytes) == h.get(0, 256), true, "hashOf-bytes");
    check(h.hashOf(string_view(bytes).substr(100, 50)) == h.get(100, 150), true, "hashOf-substring");
    check(h.hashOf("") == h.get(7, 7), true, "hashOf-empty");
    context = name(K) + " digest ordering";
    for (int rep = 0; rep < limit(20, 100, 500); ++rep) {
        string s(rng() % 12, 'a');
        for (char &c : s) { c = char('a' + rng() % 3); }
        H g(s, H::randomBase(rng()));
        set<typename H::Digest> digests;
        set<string> exact;
        vector<typename H::Digest> all;
        for (int l = 0; l <= int(s.size()); ++l) {
            for (int r = l; r <= int(s.size()); ++r) {
                digests.insert(g.get(l, r)); exact.insert(s.substr(l, r - l)); all.push_back(g.get(l, r));}}
        check(digests.size(), exact.size(), "set-of-digests-distinct-substrings");
        for (int i = 0; i + 1 < int(all.size()); ++i) {
            auto &x = all[i], &y = all[i + 1];
            auto key = [&](const typename H::Digest &d) { return std::tuple(parts<K>(d.value), d.length, parts<K>(d.base)); };
            check(x < y, key(x) < key(y), "ordering-matches-members");
            check(x == y, key(x) == key(y), "equality-matches-members");}}}
template<int K> void oracle() {
    using H = StringHash<K>;
    ulng s; int n; cin >> s >> n;
    vector<uint> a(n);
    for (auto &x : a) { cin >> x; }
    H h(a, H::randomBase(s));
    auto print = [&](auto x) {
        auto p = parts<K>(x);
        for (int j = 0; j < int(p.size()); ++j) { cout << (j ? " " : "") << p[j]; }};
    print(h.base); cout << '\n';
    for (int l = 0; l <= n; ++l) {
        for (int r = l; r <= n; ++r) {
            print(h.get(l, r).value); cout << ' '; print(h.reverseGet(l, r).value); cout << '\n';}}}
template<int K> void large() {
    using H = StringHash<K>;
    int n = limit(10000, 200000, 1000000);
    string s(n, 'a');
    H h(s);
    context = name(K) + " large n=" + std::to_string(n);
    check(h.lcp(h, 0, n, 1, n), n - 1, "large-unary-lcp"); check(h.lcs(h, 0, n - 1, 1, n), n - 1, "large-unary-lcs");
    s[n / 2] = 'b';
    H changed(s);
    check(h.lcp(changed, 0, n, 0, n), n / 2, "large-middle-mismatch");
    check(h.lcs(changed, 0, n, 0, n), n - 1 - n / 2, "large-middle-mismatch-suffix");}
template<int K> void suite() {
    words(limit(4, 7, 9), 2, [&](const auto &s) { values<K>(vector<uint>(s.begin(), s.end()), StringHash<K>::defaultBase()); });
    for (int i = 0; i < limit(20, 150, 1000); ++i) {
        vector<uint> s(rng() % 40), t(rng() % 45);
        for (auto &x : s) { x = uint(rng() % 5); }
        for (auto &x : t) { x = uint(rng() % 5); }
        values<K>(s, StringHash<K>::randomBase(rng()));
        queries<K>(s, t);}
    if constexpr (K == 0) {
        for (auto base : vector<array<uint, 2>>{{257, 257}, {1000000006, 1000000008}}) { values<0>({0, 1000000005, 256, 1}, base); }}
    else if constexpr (K == 1) {
        for (ulng base : {ulng(257), ulng(~0ULL)}) { values<1>({0, UINT_MAX, 256, 1}, base); }}
    else {
        for (ulng base : {ulng(257), P61 - 1}) { values<2>({0, UINT_MAX, 256, 1}, base); }}
    helpers<K>(); contexts<K>(); large<K>();}
int main(int argc, char **argv) {
    if (argc == 3 && string(argv[1]) == "--oracle") {
        string k = argv[2];
        if (k == "64") { oracle<1>(); }
        else if (k == "61") { oracle<2>(); }
        else { oracle<0>(); }
        return 0;}
    if (argc == 3 && string(argv[1]) == "--invalid") {
        string op = argv[2];
        StringHash<> h("abc"), other("x", {257, 259});
        if (op == "base-low") { StringHash<> bad("", {256, 257}); }
        if (op == "base-high") { StringHash<> bad("", {257, 1000000009}); }
        if (op == "base64-low") { StringHash64 bad("", 1); }
        if (op == "base64-even") { StringHash64 bad("", 258); }
        if (op == "base61-low") { StringHash61 bad("", 256); }
        if (op == "base61-high") { StringHash61 bad("", P61); }
        if (op == "alphabet") { StringHash<> bad(vector<uint>{1000000006}); }
        if (op == "hashof-alphabet") { h.hashOf(vector<uint>{1, 1000000006}); }
        if (op == "build-negative") { h.build(-1, [](int) { return 1ULL; }); }
        if (op == "left") { h.get(-1, 1); }
        if (op == "order") { h.get(2, 1); }
        if (op == "right") { h.reverseGet(0, 4); }
        if (op == "concat-base") { h.concat(h.get(0, 0), other.get(0, 0)); }
        if (op == "lcp-base") { h.lcp(other, 0, 1, 0, 1); }
        if (op == "lcs-range") { h.lcs(h, 0, 3, 0, 4); }
        if (op == "power") { h.power(-1); }
        if (op == "length-overflow") {
            auto x = h.get(0, 1), y = h.get(0, 0);
            for (int i = 0; i < 31; ++i) {
                y = h.concat(y, x);
                if (i < 30) { x = h.concat(x, x); }}
            h.concat(y, h.get(0, 1));}
        return 3;}
    configure(argc, argv);
    suite<0>(); suite<1>(); suite<2>();
    check(StringHash<true>::defaultBase(), StringHash64::defaultBase(), "bool-true-is-wrap64");
    string tm(1024, '\0'), complement(1024, '\0');
    for (int i = 0; i < 1024; ++i) { tm[i] = char(std::popcount(uint(i)) % 2); complement[i] = char(1 - tm[i]); }
    check(tm != complement, true, "collision-witness-distinct");
    for (int i = 0; i < 10; ++i) {
        auto base = StringHash64::randomBase(rng());
        check(StringHash64(tm, base).get(0, 1024) == StringHash64(complement, base).get(0, 1024), true, "known-word-ring-collision");}
    check(StringHash61(tm).get(0, 1024) == StringHash61(complement).get(0, 1024), false, "mersenne-separates-thue-morse");
    check(StringHash<>(tm).get(0, 1024) == StringHash<>(complement).get(0, 1024), false, "double-separates-thue-morse");
    cout << "PASS stringhash mode=" << mode << " seed=" << seed << " checks=" << cases << '\n';}
