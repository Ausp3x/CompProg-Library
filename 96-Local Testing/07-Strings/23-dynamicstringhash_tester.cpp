#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "../../06-Miscellaneous/02-customhash.hpp"
#include "../../02-Data Structures/19-treap.hpp"
#pragma GCC diagnostic pop
#include "../../02-Data Structures/12-lazysegmenttree.hpp"
#include "../../07-Strings/23-dynamicstringhash.hpp"

using U = vector<uint>;
ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
template<class A> void print(const A &x) {
    if constexpr (std::is_arithmetic_v<A>) { cerr << x; }
    else if constexpr (requires { x.value; }) { cerr << "{"; print(x.value); cerr << ", " << x.length << "}"; }
    else {
        cerr << '[';
        for (const auto &y : x) { print(y); cerr << ' '; }
        cerr << ']';}}
template<class A, class B> void expect(const A &actual, const B &expected, const char *op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=";
    print(expected);
    cerr << " actual=";
    print(actual);
    cerr << '\n';
    std::exit(1);}
// Independent polynomial hash of the codes x + 1, highest power first, read forwards or backwards.
template<int KIND> typename StringHash<KIND>::Digest brute(const U &s, int l, int r, typename StringHash<KIND>::Word base, bool backwards) {
    using H = StringHash<KIND>;
    typename H::Word h{};
    for (int t = 0; t < r - l; ++t) {
        ulng c = ulng(s[backwards ? r - 1 - t : l + t]) + 1;
        if constexpr (KIND == 0) {
            for (int i = 0; i < 2; ++i) { h[i] = uint((ulng(h[i]) * base[i] + c) % H::MOD[i]); }}
        else if constexpr (KIND == 1) { h = h * base + c; }
        else { h = ulng((ulll(h) * base + c) % H::P61); }}
    return {h, r - l, base};}
int brutePrefix(const U &s, int l, int r, int a, int b, bool suffix) {
    int k = 0;
    while (k < r - l && k < b - a && (suffix ? s[r - 1 - k] == s[b - 1 - k] : s[l + k] == s[a + k])) { ++k; }
    return k;}
int bruteCompare(const U &s, int l, int r, int a, int b) {
    U x(s.begin() + l, s.begin() + r), y(s.begin() + a, s.begin() + b);
    return x < y ? -1 : y < x ? 1 : 0;}
template<int KIND> uint symbol(std::mt19937_64 &rng, int sigma) {
    if (sigma > 0) { return uint(rng() % sigma); }
    return KIND == 0 ? uint(rng() % 1000000006) : uint(rng());}
template<int KIND> typename StringHash<KIND>::Word baseFor(std::mt19937_64 &rng, int it) {
    return it % 3 == 0 ? StringHash<KIND>::defaultBase() : StringHash<KIND>::randomBase(rng());}
// Range queries of one mirror against a structure exposing get, reverseGet, lcp, lcs, compare.
template<int KIND, class Q> void queries(const U &s, typename StringHash<KIND>::Word base, std::mt19937_64 &rng, int count, Q q) {
    int n = int(s.size());
    for (int t = 0; t < count; ++t) {
        int l = int(rng() % (n + 1)), r = l + int(rng() % (n - l + 1)), a = int(rng() % (n + 1)), b = a + int(rng() % (n - a + 1));
        if (t % 3 == 0 && r - l <= n - a) { b = a + (r - l); }
        q(l, r, a, b, brute<KIND>(s, l, r, base, false), brute<KIND>(s, l, r, base, true),
          brutePrefix(s, l, r, a, b, false), brutePrefix(s, l, r, a, b, true), bruteCompare(s, l, r, a, b));}}
template<int KIND> void pointUpdate(const string &mode) {
    std::mt19937_64 rng(test_seed + 10 + KIND);
    int rounds = mode == "quick" ? 30 : mode == "full" ? 200 : 800;
    for (int it = 0; it < rounds; ++it) {
        int n = it % 10 == 0 ? 0 : int(rng() % 120), sigma = it % 4 == 3 ? 0 : 1 + int(rng() % 3);
        U s(n);
        for (uint &x : s) { x = symbol<KIND>(rng, sigma); }
        auto base = baseFor<KIND>(rng, it);
        PointUpdateHash<KIND> h(s, base);
        context = "PointUpdateHash<" + std::to_string(KIND) + "> n=" + std::to_string(n) + " it=" + std::to_string(it);
        ++cases;
        for (int step = 0; step < 60; ++step) {
            if (n && step % 2) {
                int i = int(rng() % n);
                s[i] = symbol<KIND>(rng, sigma);
                h.set(i, s[i]);}
            expect(h.size(), n, "PointUpdateHash::size");
            queries<KIND>(s, base, rng, 4, [&](int l, int r, int a, int b, auto fw, auto bw, int lcp, int lcs, int cmp) {
                expect(h.get(l, r), fw, "PointUpdateHash::get");
                expect(h.reverseGet(l, r), bw, "PointUpdateHash::reverseGet");
                expect(h.lcp(l, r, a, b), lcp, "PointUpdateHash::lcp");
                expect(h.lcs(l, r, a, b), lcs, "PointUpdateHash::lcs");
                expect(h.compare(l, r, a, b), cmp, "PointUpdateHash::compare");});}
        StringHash<KIND> fixed(s, base);
        expect(h.get(0, n), fixed.get(0, n), "PointUpdateHash agrees with StringHash");}
    string bytes = "\x80\xff\x01z\x80\xff";
    PointUpdateHash<KIND> hb(bytes);
    StringHash<KIND> sb(bytes);
    context = "PointUpdateHash bytes";
    expect(hb.get(0, 6), sb.get(0, 6), "PointUpdateHash string_view");
    expect(hb.lcp(0, 2, 4, 6), 2, "PointUpdateHash high bytes lcp");
    expect(hb.compare(3, 4, 0, 1), -1, "PointUpdateHash high bytes order");
    cout << "PASS PointUpdateHash<" << KIND << "> rounds=" << rounds << '\n';}
template<int KIND> void sequence(const string &mode) {
    std::mt19937_64 rng(test_seed + 20 + KIND);
    int rounds = mode == "quick" ? 20 : mode == "full" ? 120 : 500;
    for (int it = 0; it < rounds; ++it) {
        int n = int(rng() % 60), sigma = it % 4 == 3 ? 0 : 1 + int(rng() % 3);
        U s(n);
        for (uint &x : s) { x = symbol<KIND>(rng, sigma); }
        auto base = baseFor<KIND>(rng, it);
        SequenceHash<KIND> h(base, rng());
        int root = h.build(h.leaves(s));
        context = "SequenceHash<" + std::to_string(KIND) + "> it=" + std::to_string(it);
        ++cases;
        for (int step = 0; step < 80; ++step) {
            n = int(s.size());
            int op = int(rng() % 7), i = int(rng() % (n + 1)), j = i + int(rng() % (n - i + 1));
            uint x = symbol<KIND>(rng, sigma);
            if (op == 0) { h.insert(root, i, h.leaf(x)); s.insert(s.begin() + i, x); }
            else if (op == 1 && i < n) { h.erase(root, i); s.erase(s.begin() + i); }
            else if (op == 2 && i < n) { h.set(root, i, h.leaf(x)); s[i] = x; }
            else if (op == 3) { h.reverse(root, i, j); std::reverse(s.begin() + i, s.begin() + j); }
            else if (op == 4) {
                auto [a, b] = h.split(root, i);
                expect(h.size(a), i, "SequenceHash::split size");
                expect(h.get(b, 0, n - i), brute<KIND>(s, i, n, base, false), "SequenceHash::get after split");
                root = h.merge(b, a);
                std::rotate(s.begin(), s.begin() + i, s.end());}
            else if (op == 5) {
                if constexpr (KIND != 1) {
                    h.apply(root, i, j, h.assignment(x));
                    fill(s.begin() + i, s.begin() + j, x);}}
            n = int(s.size());
            expect(h.size(root), n, "SequenceHash::size");
            if (n && step % 10 == 0) {
                int p = int(rng() % n);
                expect(h.at(root, p).h, dynamic_hash_detail::code<KIND>(s[p]), "SequenceHash::at");}
            queries<KIND>(s, base, rng, 2, [&](int l, int r, int a, int b, auto fw, auto bw, int lcp, int lcs, int cmp) {
                expect(h.get(root, l, r), fw, "SequenceHash::get");
                expect(h.reverseGet(root, l, r), bw, "SequenceHash::reverseGet");
                expect(h.lcp(root, l, r, a, b), lcp, "SequenceHash::lcp");
                expect(h.lcs(root, l, r, a, b), lcs, "SequenceHash::lcs");
                expect(h.compare(root, l, r, a, b), cmp, "SequenceHash::compare");});}
        auto values = h.values(root);
        expect(int(values.size()), int(s.size()), "SequenceHash::values size");
        for (size_t p = 0; p < values.size(); ++p) { expect(values[p].h, dynamic_hash_detail::code<KIND>(s[p]), "SequenceHash::values"); }}
    SequenceHash<KIND> hb;
    string bytes = "\xff\x80?";
    int rb = hb.build(hb.leaves(bytes));
    context = "SequenceHash bytes";
    expect(hb.get(rb, 0, 3), StringHash<KIND>(bytes).get(0, 3), "SequenceHash leaves(string_view)");
    expect(hb.compare(rb, 2, 3, 0, 1), -1, "SequenceHash high bytes order");
    cout << "PASS SequenceHash<" << KIND << "> rounds=" << rounds << '\n';}
template<int KIND> void doubleEnded(const string &mode) {
    std::mt19937_64 rng(test_seed + 30 + KIND);
    int rounds = mode == "quick" ? 20 : mode == "full" ? 150 : 600;
    for (int it = 0; it < rounds; ++it) {
        auto base = baseFor<KIND>(rng, it);
        DequeHash<KIND> h(base);
        deque<uint> mirror;
        int sigma = it % 4 == 3 ? 0 : 1 + int(rng() % 3);
        context = "DequeHash<" + std::to_string(KIND) + "> it=" + std::to_string(it);
        ++cases;
        for (int step = 0; step < 300; ++step) {
            int op = int(rng() % 5);
            uint x = symbol<KIND>(rng, sigma);
            if (op == 0) { h.pushBack(x); mirror.push_back(x); }
            else if (op == 1) { h.pushFront(x); mirror.push_front(x); }
            else if (op == 2 && !mirror.empty()) { h.popBack(); mirror.pop_back(); }
            else if (op == 3 && !mirror.empty()) { h.popFront(); mirror.pop_front(); }
            U s(mirror.begin(), mirror.end());
            int n = int(s.size());
            expect(h.size(), n, "DequeHash::size");
            int l = int(rng() % (n + 1)), r = l + int(rng() % (n - l + 1));
            expect(h.get(l, r), brute<KIND>(s, l, r, base, false), "DequeHash::get");}}
    cout << "PASS DequeHash<" << KIND << "> rounds=" << rounds << '\n';}
// Monoid laws and the range assignment, directly and through LazySegmentTree.
template<int KIND> void monoid(const string &mode) {
    using A = HashMonoid<KIND>;
    std::mt19937_64 rng(test_seed + 40 + KIND);
    int rounds = mode == "quick" ? 100 : mode == "full" ? 1000 : 4000;
    auto base = baseFor<KIND>(rng, 1);
    SequenceHash<KIND> maker(base);
    auto of = [&](const U &s) {
        typename A::S x = A::e();
        for (uint c : s) { x = A::op(x, maker.leaf(c)); }
        return x;};
    auto same = [](const typename A::S &x, const typename A::S &y) { return x.h == y.h && x.r == y.r && x.p == y.p; };
    for (int it = 0; it < rounds; ++it) {
        U a(rng() % 8), b(rng() % 8), c(rng() % 8);
        for (U *s : {&a, &b, &c}) {
            for (uint &x : *s) { x = symbol<KIND>(rng, 3); }}
        context = "HashMonoid<" + std::to_string(KIND) + "> it=" + std::to_string(it);
        ++cases;
        U ab = a, ba = b, abc;
        ab.insert(ab.end(), b.begin(), b.end());
        abc = ab;
        abc.insert(abc.end(), c.begin(), c.end());
        expect(same(A::op(A::e(), of(a)), of(a)) && same(A::op(of(a), A::e()), of(a)), true, "HashMonoid identity");
        expect(same(A::op(A::op(of(a), of(b)), of(c)), A::op(of(a), A::op(of(b), of(c)))), true, "HashMonoid associativity");
        expect(of(ab).h, brute<KIND>(ab, 0, int(ab.size()), base, false).value, "HashMonoid op forward");
        expect(of(ab).r, brute<KIND>(ab, 0, int(ab.size()), base, true).value, "HashMonoid op reversed");
        expect(of(ab).p, dynamic_hash_detail::power<KIND>(base, ab.size()), "HashMonoid power");
        U rev(ab.rbegin(), ab.rend());
        expect(same(A::reverse(of(ab)), of(rev)), true, "HashMonoid reverse");
        if constexpr (KIND != 1) {
            uint x = symbol<KIND>(rng, 0);
            U all(abc.size(), x);
            expect(same(A::mapping(maker.assignment(x), of(abc)), of(all)), true, "HashMonoid mapping");
            uint y = symbol<KIND>(rng, 3);
            expect(same(A::mapping(A::composition(maker.assignment(y), maker.assignment(x)), of(abc)), of(U(abc.size(), y))), true, "HashMonoid composition");
            expect(same(A::mapping(A::id(), of(abc)), of(abc)), true, "HashMonoid mapping(id)");
            expect(same(A::mapping(A::composition(A::id(), maker.assignment(x)), of(abc)), of(all)), true, "HashMonoid composition(id, g)");
            expect(same(A::mapping(A::composition(maker.assignment(x), A::id()), of(abc)), of(all)), true, "HashMonoid composition(f, id)");
            expect(same(A::mapping(maker.assignment(x), A::e()), A::e()), true, "HashMonoid mapping(f, e)");}}
    if constexpr (KIND != 1) {
        int n = mode == "quick" ? 200 : 2000;
        U s(n);
        for (uint &x : s) { x = symbol<KIND>(rng, 2); }
        LazySegmentTree<A> seg(maker.leaves(s));
        context = "HashMonoid in LazySegmentTree";
        for (int t = 0; t < 400; ++t) {
            int l = int(rng() % (n + 1)), r = l + int(rng() % (n - l + 1));
            if (t % 2) {
                uint x = symbol<KIND>(rng, 2);
                seg.apply(l, r, maker.assignment(x));
                fill(s.begin() + l, s.begin() + r, x);}
            else { expect(seg.prod(l, r).h, brute<KIND>(s, l, r, base, false).value, "LazySegmentTree<HashMonoid>::prod"); }}}
    cout << "PASS HashMonoid<" << KIND << "> rounds=" << rounds << '\n';}
// Long sequences: periodic texts with long common prefixes, mixed edits against the mirror.
template<int KIND> void large(const string &mode) {
    std::mt19937_64 rng(test_seed + 50 + KIND);
    int n = mode == "quick" ? 20000 : mode == "full" ? 100000 : 300000, q = mode == "quick" ? 2000 : 20000;
    U s(n);
    for (int i = 0; i < n; ++i) { s[i] = uint(i % 7 == 6); }
    PointUpdateHash<KIND> h(s);
    SequenceHash<KIND> t;
    int root = t.build(t.leaves(s));
    DequeHash<KIND> d;
    for (uint x : s) { d.pushFront(x); }
    U rev(s.rbegin(), s.rend());
    context = "large n=" + std::to_string(n);
    auto base = StringHash<KIND>::defaultBase();
    for (int it = 0; it < q; ++it) {
        int i = int(rng() % n), a = int(rng() % n), len = int(rng() % (n - max(i, a) + 1));
        if (it % 50 == 0) {
            uint x = uint(rng() % 2);
            s[i] = x; h.set(i, x); t.set(root, i, t.leaf(x));}
        int want = brutePrefix(s, i, i + len, a, a + len, false);
        expect(h.lcp(i, i + len, a, a + len), want, "PointUpdateHash::lcp large");
        if (it % 10 == 0) {
            expect(t.lcp(root, i, i + len, a, a + len), want, "SequenceHash::lcp large");
            expect(h.get(i, i + len), t.get(root, i, i + len), "PointUpdateHash and SequenceHash digests");}}
    for (int it = 0; it < 50; ++it) {
        int l = int(rng() % n), r = l + int(rng() % (n - l + 1));
        expect(d.get(l, r), brute<KIND>(rev, l, r, base, false), "DequeHash::get large");}
    cout << "PASS large<" << KIND << "> n=" << n << " queries=" << q << '\n';}
template<int KIND> void all(const string &mode) {
    pointUpdate<KIND>(mode);
    sequence<KIND>(mode);
    doubleEnded<KIND>(mode);
    monoid<KIND>(mode);
    large<KIND>(mode);}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i + 1 < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { invalid = argv[i + 1]; }}
#ifdef _GLIBCXX_DEBUG
    if (mode == "stress") { mode = "full"; }
#endif
    if (!invalid.empty()) {
        PointUpdateHash<0> h(string("abc"));
        if (invalid == "point-set") { h.set(3, 'a'); }
        if (invalid == "point-range") { h.get(2, 1); }
        if (invalid == "point-symbol") { h.set(0, 1000000006); }
        if (invalid == "point-lcp") { h.lcp(0, 4, 0, 1); }
        if (invalid == "base-small") { PointUpdateHash<2>(string("a"), 3); }
        if (invalid == "base-even") { DequeHash<1>(1000); }
        if (invalid == "deque-pop") { DequeHash<0>().popFront(); }
        if (invalid == "deque-range") { DequeHash<2> d; d.pushBack(1); d.get(0, 2); }
        if (invalid == "sequence-range") { SequenceHash<0> t; int v = t.build(t.leaves(string("ab"))); t.lcp(v, 0, 3, 0, 1); }
        if (invalid == "sequence-symbol") { SequenceHash<0>().leaf(1000000006); }
        return 0;}
    all<0>(mode);
    all<1>(mode);
    all<2>(mode);
    cout << "PASS dynamicstringhash seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
