#include "../../07-Strings/03-stringhash.hpp"
#include "01-prefix_z_test_support.hpp"

template<bool W> auto reference(const vector<uint> &s, int l, int r, typename StringHash<W>::Word base) {
    using H = StringHash<W>; typename H::Word out{};
    for (int i = l; i < r; ++i) {
        if constexpr (W) { out = ulng(ulll(out) * base + ulng(s[i]) + 1); }
        else { for (int j = 0; j < 2; ++j) { out[j] = uint((ulll(out[j]) * base[j] + ulng(s[i]) + 1) % H::MOD[j]); } } }
    return out;
}
template<bool W> void values(const vector<uint> &s, typename StringHash<W>::Word base) {
    using H = StringHash<W>; H h(s, base); int n = int(s.size());
    context = "wrap=" + std::to_string(W) + " symbols=" + show(s);
    check(h.size(), n, "size"); auto copy = h; H moved(std::move(copy)); copy = h;
    check(moved.get(0, n) == copy.get(0, n), true, "copy-move-assignment");
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
        auto d = h.get(l, r); check(d.value == reference<W>(s, l, r, base), true, "substring-arithmetic");
        check(d.length, r - l, "length-tag"); check(d.base == base, true, "base-tag");
        auto rev = vector<uint>(s.begin() + l, s.begin() + r); reverse(rev.begin(), rev.end());
        check(h.reverseGet(l, r).value == reference<W>(rev, 0, int(rev.size()), base), true, "reverse-arithmetic");
        int m = (l + r) / 2;
        check(h.concat(h.get(l, m), h.get(m, r)) == d, true, "concatenation");
        check(h.lcp(h, l, r, l, r), r - l, "self-lcp"); check(h.lcs(h, l, r, l, r), r - l, "self-lcs"); } }
    auto twice = h.concat(h.get(0, n), h.get(0, n));
    auto three = h.concat(h.get(0, n), twice); vector<uint> triple(s);
    triple.insert(triple.end(), s.begin(), s.end()); triple.insert(triple.end(), s.begin(), s.end());
    check(three.value == reference<W>(triple, 0, int(triple.size()), base), true, "long-right-power");
    auto different_length = h.get(0, 0); ++different_length.length;
    check(h.get(0, 0) == different_length, false, "different-length-rejection");
    auto other_base = base;
    if constexpr (W) { other_base = base == 257 ? 259 : 257; }
    else { other_base[0] = base[0] == 257 ? 258 : 257; }
    H other(s, other_base); check(h.get(0, 0) == other.get(0, 0), false, "different-base-rejection");
    check(h.get(0, n).value == reference<W>(s, 0, n, base), true, "independent-context-lifetime");
}
template<bool W> void queries(const vector<uint> &s, const vector<uint> &t) {
    using H = StringHash<W>; auto base = H::randomBase(rng()); H h(s, base), g(t, base);
    context = "wrap=" + std::to_string(W) + " s=" + show(s) + " t=" + show(t);
    for (int rep = 0; rep < 100; ++rep) {
        int l = int(rng() % (s.size() + 1)), r = int(rng() % (s.size() + 1)); if (l > r) { swap(l, r); }
        int a = int(rng() % (t.size() + 1)), b = int(rng() % (t.size() + 1)); if (a > b) { swap(a, b); }
        int p = 0, q = 0;
        while (p < min(r - l, b - a) && s[l + p] == t[a + p]) { ++p; }
        while (q < min(r - l, b - a) && s[r - 1 - q] == t[b - 1 - q]) { ++q; }
        check(h.lcp(g, l, r, a, b), p, "direct-lcp"); check(h.lcs(g, l, r, a, b), q, "direct-lcs"); }
}
template<bool W> void oracle() {
    using H = StringHash<W>; ulng s; int n; cin >> s >> n; vector<uint> a(n);
    for (auto &x : a) { cin >> x; } H h(a, H::randomBase(s));
    auto print = [&](auto x) { if constexpr (W) { cout << x; } else { cout << x[0] << ' ' << x[1]; } };
    print(h.base); cout << '\n';
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
        print(h.get(l, r).value); cout << ' '; print(h.reverseGet(l, r).value); cout << '\n'; } }
}
int main(int argc, char **argv) {
    if (argc == 3 && string(argv[1]) == "--oracle") {
        if (string(argv[2]) == "64") { oracle<true>(); } else { oracle<false>(); } return 0; }
    if (argc == 3 && string(argv[1]) == "--invalid") {
        string op = argv[2]; StringHash<> h("abc"), other("x", {257, 259});
        if (op == "base-low") { StringHash<> bad("", {256, 257}); }
        if (op == "base-high") { StringHash<> bad("", {257, 1000000009}); }
        if (op == "base64-low") { StringHash64 bad("", 1); }
        if (op == "base64-even") { StringHash64 bad("", 258); }
        if (op == "alphabet") { StringHash<> bad(vector<uint>{1000000006}); }
        if (op == "left") { h.get(-1, 1); }
        if (op == "order") { h.get(2, 1); }
        if (op == "right") { h.reverseGet(0, 4); }
        if (op == "concat-base") { h.concat(h.get(0, 0), other.get(0, 0)); }
        if (op == "lcp-base") { h.lcp(other, 0, 1, 0, 1); }
        if (op == "lcs-range") { h.lcs(h, 0, 3, 0, 4); }
        if (op == "power") { h.power(-1); }
        if (op == "length-overflow") {
            auto x = h.get(0, 1), y = h.get(0, 0);
            for (int i = 0; i < 31; ++i) { y = h.concat(y, x); if (i < 30) { x = h.concat(x, x); } }
            h.concat(y, h.get(0, 1)); }
        return 3; }
    configure(argc, argv);
    words(limit(4, 7, 9), 2, [&](const auto &s) {
        vector<uint> a(s.begin(), s.end()); values<false>(a, StringHash<>::defaultBase()); values<true>(a, StringHash64::defaultBase()); });
    for (int i = 0; i < limit(20, 150, 1000); ++i) {
        vector<uint> s(rng() % 40), t(rng() % 45);
        for (auto &x : s) { x = uint(rng() % 5); } for (auto &x : t) { x = uint(rng() % 5); }
        values<false>(s, StringHash<>::randomBase(rng())); values<true>(s, StringHash64::randomBase(rng()));
        queries<false>(s, t); queries<true>(s, t); }
    for (auto base : vector<array<uint, 2>>{{257, 257}, {1000000006, 1000000008}}) { values<false>({0, 1000000005, 256, 1}, base); }
    for (ulng base : {ulng(257), std::numeric_limits<ulng>::max()}) { values<true>({0, UINT_MAX, 256, 1}, base); }
    string bytes; vector<uint> encoded;
    for (int i = 0; i < 256; ++i) { bytes += char(i); encoded.push_back(uint(i)); }
    check(StringHash<>(bytes).get(0, 256) == StringHash<>(encoded).get(0, 256), true, "unsigned-byte-encoding");
    check(StringHash64(bytes).get(0, 256) == StringHash64(encoded).get(0, 256), true, "unsigned-byte-encoding64");
    check(StringHash<>::randomBase(seed) == StringHash<>::randomBase(seed), true, "seed-repeatability");
    check(StringHash64::randomBase(seed) == StringHash64::randomBase(seed), true, "seed-repeatability64");
    string tm(1024, '\0'), complement(1024, '\0');
    for (int i = 0; i < 1024; ++i) { tm[i] = char(std::popcount(uint(i)) % 2); complement[i] = char(1 - tm[i]); }
    check(tm != complement, true, "collision-witness-distinct");
    for (int i = 0; i < 10; ++i) {
        auto base = StringHash64::randomBase(rng());
        check(StringHash64(tm, base).get(0, 1024) == StringHash64(complement, base).get(0, 1024), true, "known-word-ring-collision"); }
    int n = limit(10000, 200000, 1000000); string large(n, 'a'); StringHash<> h(large); StringHash64 wide(large);
    check(h.lcp(h, 0, n, 1, n), n - 1, "large-unary-lcp"); check(wide.lcs(wide, 0, n - 1, 1, n), n - 1, "large-unary-lcs64");
    large[n / 2] = 'b'; StringHash<> changed(large);
    check(h.lcp(changed, 0, n, 0, n), n / 2, "large-middle-mismatch");
    check(h.lcs(changed, 0, n, 0, n), n - 1 - n / 2, "large-middle-mismatch-suffix");
    cout << "PASS stringhash mode=" << mode << " seed=" << seed << " checks=" << cases << '\n';
}
