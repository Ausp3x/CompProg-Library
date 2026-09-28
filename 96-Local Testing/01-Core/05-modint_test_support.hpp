#pragma once
#include <sstream>
#include <type_traits>

// Shared fixtures test all four public types in 05-modint.hpp, never a copied implementation.
// REQUIRE remains live in -DNDEBUG configurations.
#if TEST_DYNAMIC
inline TEST_TYPE<810> global_default_product = TEST_TYPE<810>(1000000) * TEST_TYPE<810>(1000000);
inline int global_setup = [] { TEST_TYPE<811>::setMod(17); return 0; }();
inline TEST_TYPE<811> global_custom_product = TEST_TYPE<811>(1000000) * TEST_TYPE<811>(1000000);
#endif
inline string context;
inline ulng test_seed = 20260927;
inline string test_mode = "quick";
[[noreturn]] inline void fail(const string &expr, int line, const string &detail = "") {
    cerr << "FAIL line=" << line << " seed=" << test_seed << " mode=" << test_mode
         << " input={" << context << "} operation=" << expr << ' ' << detail << '\n';
    std::exit(1);}
#define REQUIRE(x) do { if (!(x)) { fail(#x, __LINE__, "expected=true actual=false"); } } while (0)
#define EQUAL(a, b) do { auto actual_ = (a); auto expected_ = (b); if (actual_ != expected_) { \
    fail(#a, __LINE__, "expected=" + std::to_string(expected_) + " actual=" + std::to_string(actual_)); } } while (0)

inline bool primeOracle(ulng m) {
    if (m < 2) { return false; }
    for (ulng d = 2; d <= m / d; d++) { if (m % d == 0) { return false; } }
    return true;}
inline ulll parseWide(const string &s) {
    ulll x = 0;
    for (char c : s) { REQUIRE(c >= '0' && c <= '9'); x = 10 * x + uint(c - '0'); }
    return x;}
inline lll parseSigned(const string &s) {
    if (s[0] == '-') { ulll x = parseWide(s.substr(1)); return -lll(x - 1) - 1; }
    return lll(parseWide(s[0] == '+' ? s.substr(1) : s));}

template<class M> void traits() {
    using W = decltype(M::mod());
    static_assert(sizeof(M) == sizeof(W));
    static_assert(std::is_standard_layout_v<M> && std::is_trivially_copyable_v<M>);
    static_assert(std::is_default_constructible_v<M> && std::is_copy_constructible_v<M>);
    static_assert(std::is_move_constructible_v<M> && std::is_copy_assignable_v<M> && std::is_move_assignable_v<M>);
    static_assert(std::is_constructible_v<M, lll> && std::is_constructible_v<M, ulll>);
    static_assert(!std::is_constructible_v<M, double> && !std::is_convertible_v<M, int>);
    static_assert(std::is_same_v<decltype(std::declval<M &>() += M()), M &>);
    static_assert(std::is_same_v<decltype(std::declval<M &>() -= M()), M &>);
    static_assert(std::is_same_v<decltype(std::declval<M &>() *= M()), M &>);
    static_assert(std::is_same_v<decltype(std::declval<M &>() /= M()), M &>);
    static_assert(std::is_same_v<decltype(std::declval<M &>()++), M>);
    static_assert(std::is_same_v<decltype(++std::declval<M &>()), M &>);
    M x; EQUAL(x.val(), 0); EQUAL(M::init(0).val(), 0); EQUAL(M::raw(W(M::mod() - 1)).val(), M::mod() - 1);
    M y = M::init(W(M::mod() - 1)); EQUAL(y.n, y.val());
    M z(y); EQUAL(z.val(), y.val()); M w(std::move(z)); EQUAL(w.val(), y.val());
    z = w; z = std::move(w); z = z; EQUAL(z.val(), y.val());
    EQUAL(ulng(y), y.val()); EQUAL(ulll(y) == y.val(), true);
    EQUAL(lll(y) == y.val(), true);
    M one = 1; EQUAL(static_cast<signed char>(one), 1 % M::mod());
    EQUAL(static_cast<unsigned char>(one), 1 % M::mod()); EQUAL(short(one), 1 % M::mod());
    EQUAL(static_cast<unsigned short>(one), 1 % M::mod()); EQUAL(lng(one), 1 % M::mod());
    EQUAL(static_cast<long long>(one), 1 % M::mod()); EQUAL(static_cast<unsigned long long>(one), 1 % M::mod());
    if (y.val() <= INT_MAX) { EQUAL(int(y), int(y.val())); }
    if (y.val() <= UINT_MAX) { EQUAL(uint(y), uint(y.val())); }}

template<class M, class T> void construction(T x) {
    ulng want;
    if constexpr (std::is_signed_v<T>) {
        lll r = lll(x) % lll(M::mod()); if (r < 0) { r += M::mod(); } want = ulng(r);}
    else { want = ulng(ulll(x) % M::mod()); }
    EQUAL(M(x).val(), want); M a; REQUIRE(&(a = x) == &a); EQUAL(a.val(), want);
    if constexpr (requires { M::norm(x); }) { EQUAL(M::norm(x), want); }}

template<class M> void constructors() {
    construction<M>(false); construction<M>(true);
    construction<M>(std::numeric_limits<char>::min()); construction<M>(std::numeric_limits<char>::max());
    construction<M>(std::numeric_limits<signed char>::min()); construction<M>(std::numeric_limits<unsigned char>::max());
    construction<M>(std::numeric_limits<short>::min()); construction<M>(std::numeric_limits<unsigned short>::max());
    construction<M>(INT_MIN); construction<M>(INT_MAX); construction<M>(UINT_MAX);
    construction<M>(LONG_MIN); construction<M>(LONG_MAX); construction<M>(ULONG_MAX);
    construction<M>(LLONG_MIN); construction<M>(LLONG_MAX); construction<M>(ULLONG_MAX);
    construction<M>(std::numeric_limits<lll>::min()); construction<M>(std::numeric_limits<lll>::max());
    construction<M>(~ulll(0)); construction<M>(wchar_t(-1)); construction<M>(char16_t(65535)); construction<M>(char32_t(-1));
    for (lng a = -10; a <= 10; a++) { construction<M>(a); }
    construction<M>(M::mod()); construction<M>(ulll(M::mod()) + 1);
    construction<M>(-lll(M::mod())); construction<M>(-lll(M::mod()) - 1);}

template<class M> void pairCase(ulng a, ulng b) {
    ulng m = M::mod(); M x = a, y = b;
    context = "mod=" + std::to_string(m) + " a=" + std::to_string(a) + " b=" + std::to_string(b);
    ulng add = ulng((ulll(a) + b) % m), sub = ulng((ulll(a) + m - b) % m), mul = ulng(ulll(a) * b % m);
    EQUAL((x + y).val(), add); EQUAL((x - y).val(), sub); EQUAL((x * y).val(), mul);
    M z = x; REQUIRE(&(z += y) == &z); EQUAL(z.val(), add);
    z = x; REQUIRE(&(z -= y) == &z); EQUAL(z.val(), sub);
    z = x; REQUIRE(&(z *= y) == &z); EQUAL(z.val(), mul);
    EQUAL((x + b).val(), add); EQUAL((a + y).val(), add);
    EQUAL((x - b).val(), sub); EQUAL((a - y).val(), sub);
    EQUAL((x * b).val(), mul); EQUAL((a * y).val(), mul);
    EQUAL(x == y, a == b); EQUAL(x != y, a != b); EQUAL(x < y, a < b);
    EQUAL(x <= y, a <= b); EQUAL(x > y, a > b); EQUAL(x >= y, a >= b);
    EQUAL(x == b, a == b); EQUAL(a < y, a < b); EQUAL((x <=> y) < 0, a < b);
    EQUAL((+x).val(), a); EQUAL((-x).val(), a ? m - a : 0); EQUAL(bool(x), a != 0); EQUAL(!x, a == 0);
    z = x; EQUAL((z++).val(), a); EQUAL(z.val(), ulng((ulll(a) + 1) % m));
    z = x; REQUIRE(&++z == &z); EQUAL(z.val(), ulng((ulll(a) + 1) % m));
    z = x; EQUAL((z--).val(), a); EQUAL(z.val(), a ? a - 1 : m - 1);
    z = x; REQUIRE(&--z == &z); EQUAL(z.val(), a ? a - 1 : m - 1);
    M r = M(m - 1); bool unit = std::gcd(b, m) == 1;
    EQUAL(tryInv(y, r), unit);
    if (unit) {
        EQUAL(ulng(ulll(r.val()) * b % m), 1 % m); EQUAL(inv(y).val(), r.val());
        ulng quot = ulng(ulll(a) * r.val() % m);
        EQUAL((x / y).val(), quot); EQUAL((x / b).val(), quot); EQUAL((a / y).val(), quot);
        z = x; REQUIRE(&(z /= y) == &z); EQUAL(z.val(), quot);
        z = y; REQUIRE(tryInv(z, z)); EQUAL(z.val(), r.val());}
    else {
        EQUAL(r.val(), m - 1); z = y; REQUIRE(!tryInv(z, z)); EQUAL(z.val(), b);}
    if (a == b) {
        z = x; z += z; EQUAL(z.val(), add); z = x; z -= z; EQUAL(z.val(), 0);
        z = x; z *= z; EQUAL(z.val(), mul);
        if (unit) { z = x; z /= z; EQUAL(z.val(), 1 % m); }}}

template<class M> void batches() {
    ulng m = M::mod(); vector<M> out{M(3), M(5)}, empty;
    REQUIRE(batchInv(empty, out)); REQUIRE(out.empty()); REQUIRE(batchInv(out, out)); REQUIRE(out.empty());
    for (int n : {1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 257}) {
        vector<M> a(n); std::mt19937_64 rng(test_seed + ulng(n));
        for (int i = 0; i < n; i++) {
            ulng x; do { x = rng() % m; } while (std::gcd(x, m) != 1);
            a[i] = x; }
        REQUIRE(batchInv(a, out)); EQUAL(out.size(), a.size());
        for (int i = 0; i < n; i++) { EQUAL(ulng(ulll(a[i].val()) * out[i].val() % m), 1 % m); }
        auto alias = a; REQUIRE(batchInv(alias, alias)); REQUIRE(alias == out);
        if (m > 1) {
            for (int bad : {0, n / 2, n - 1}) {
                auto b = a; b[bad] = 0; auto saved = out; REQUIRE(!batchInv(b, out)); REQUIRE(saved == out);
                saved = b; REQUIRE(!batchInv(b, b)); REQUIRE(saved == b); }}}
    for (ulng d : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        if (m > d && m % d == 0) { vector<M> a{M(1), M(d), M(1)}, saved = out; REQUIRE(!batchInv(a, out)); REQUIRE(out == saved); }}}

template<class M> void streams() {
    ulng m = M::mod();
    for (const string &token : vector<string>{"0", "-0", "+0", "0001", "+0007", "-0007", "18446744073709551615", "-170141183460469231731687303715884105728", string(4096, '9')}) {
        ulng r = 0; bool neg = token[0] == '-';
        for (char c : token) { if (c >= '0' && c <= '9') { r = ulng((ulll(r) * 10 + uint(c - '0')) % m); } }
        if (neg && r) { r = m - r; }
        std::istringstream in(" \t\n" + token + " 123"); M a = 4; REQUIRE(&(in >> a) == &in); REQUIRE(!in.fail()); EQUAL(a.val(), r);
        int next; in >> next; EQUAL(next, 123); std::ostringstream os; REQUIRE(&(os << a) == &os); EQUAL(os.str() == std::to_string(r), true);}
    for (const string &token : vector<string>{"", " ", "+", "-", "x", "1x", "++1", "--1", "+-1", "0xFF", "1.0"}) {
        M a = 7; auto saved = a; std::istringstream in(token); in >> a; REQUIRE(in.fail()); REQUIRE(a == saved); }}

template<class M> void roots() {
    if (!M::is_prime) { return; }
    ulng m = M::mod();
    if (m <= 199) {
        for (ulng a = 0; a < m; a++) {
            context = "sqrt mod=" + std::to_string(m) + " a=" + std::to_string(a);
            ulng want = m;
            for (ulng r = 0; r < m; r++) { if (r * r % m == a) { want = r; break; } }
            M r = m - 1; EQUAL(trySqrt(M(a), r), want != m);
            if (want != m) { EQUAL(r.val(), want); EQUAL(sqrt(M(a)).val(), want); M alias = a; REQUIRE(trySqrt(alias, alias)); EQUAL(alias.val(), want); }
            else { EQUAL(r.val(), m - 1); EQUAL(sqrt(M(a)).val(), m - 1); M alias = a; REQUIRE(!trySqrt(alias, alias)); EQUAL(alias.val(), a); }}}
    else {
        std::mt19937_64 rng(test_seed ^ m); int count = test_mode == "quick" ? 12 : test_mode == "full" ? 100 : 1000;
        for (int i = 0; i < count; i++) {
            ulng x = rng() % m, a = ulng(ulll(x) * x % m); M r;
            context = "sqrt mod=" + std::to_string(m) + " square-of=" + std::to_string(x);
            REQUIRE(trySqrt(M(a), r)); EQUAL(r.val(), std::min(x, x ? m - x : 0)); EQUAL(sqrt(M(a)).val(), r.val()); }}}

template<class M> void verifyMod() {
    context = "construction mod=" + std::to_string(M::mod()); traits<M>(); constructors<M>(); streams<M>(); batches<M>();
    ulng m = M::mod();
    using Wide = typename M::Wide;
    for (Wide a : {Wide(0), Wide(1), Wide(m - 1), Wide(m), Wide(m) + 1, ~Wide(0)}) {
        EQUAL(M::red(a), ulng(a % m)); }
    EQUAL(M::isPrime(), M::is_prime);
    if (m <= 97) {
        for (ulng a = 0; a < m; a++) { for (ulng b = 0; b < m; b++) { pairCase<M>(a, b); } }}
    else {
        vector<ulng> edges{0, 1, m - 1, m / 2, m / 2 + 1, m - 2};
        for (ulng a : edges) { for (ulng b : edges) { pairCase<M>(a, b); } }
        std::mt19937_64 rng(test_seed ^ m); int count = test_mode == "quick" ? 100 : test_mode == "full" ? 3000 : 30000;
        for (int i = 0; i < count; i++) { pairCase<M>(rng() % m, rng() % m); }}
    roots<M>();
    EQUAL(pow(M(0), 0).val(), 1 % m); EQUAL(pow(M(0), 1).val(), 0);
    if (m == 1) { EQUAL(pow(M(0), std::numeric_limits<lll>::min()).val(), 0); }
    cout << "PASS arithmetic/constructors/batch/stream/root mod=" << m << '\n';}

// Dispatch is deliberately bounded to the declared Python corpus moduli.
template<class F> void dispatch(ulng m, F &&f) {
#if TEST_DYNAMIC
    using M = TEST_TYPE<731>; M::setMod(decltype(M::mod())(m)); f.template operator()<M>();
#else
#define MOD_CASE(P) case P: f.template operator()<TEST_TYPE<P>>(); break
    switch (m) {
    MOD_CASE(1); MOD_CASE(2); MOD_CASE(4); MOD_CASE(7); MOD_CASE(17); MOD_CASE(97);
    MOD_CASE(998244353); MOD_CASE(2147483647); MOD_CASE(2147483648); MOD_CASE(4294967291); MOD_CASE(4294967295);
#if TEST_WIDE
    MOD_CASE(4294967296ULL); MOD_CASE(2305843009213693951ULL); MOD_CASE(9223372036854775808ULL);
    MOD_CASE(18446744069414584321ULL); MOD_CASE(18446744073709551557ULL); MOD_CASE(18446744073709551615ULL);
#endif
    default: fail("supported modulus dispatch", __LINE__); }
#undef MOD_CASE
#endif
}

inline void compileTime() {
    static_assert(std::is_same_v<mint, ModInt<998244353>>);
    static_assert(std::is_same_v<decltype(ModInt<7>::mod()), uint>);
    static_assert(std::is_same_v<decltype(ModInt64<7>::mod()), ulng>);
    static_assert(std::is_same_v<decltype(DynModInt<>::mod()), uint>);
    static_assert(std::is_same_v<decltype(DynModInt64<>::mod()), ulng>);
    static_assert(std::is_same_v<DynModInt<>, DynModInt<0>> && std::is_same_v<DynModInt64<>, DynModInt64<0>>);
    static_assert(!std::is_same_v<ModInt<7>, ModInt64<7>> && !std::is_same_v<DynModInt<>, DynModInt64<>>);
    static_assert(!std::is_same_v<DynModInt<0>, DynModInt<1>> && !std::is_same_v<DynModInt64<0>, DynModInt64<1>>);
#if !TEST_DYNAMIC
    using M = TEST_TYPE<7>;
    static_assert(!TEST_TYPE<1>::is_prime && TEST_TYPE<2>::is_prime && !TEST_TYPE<4>::is_prime);
    static_assert(TEST_TYPE<97>::is_prime && !TEST_TYPE<561>::is_prime && !TEST_TYPE<1373653>::is_prime);
    static_assert(!TEST_TYPE<25326001>::is_prime && TEST_TYPE<4294967291>::is_prime);
    static_assert(TEST_TYPE<4294967295>::mod() == 4294967295U);
    static_assert(M(-1).val() == 6 && (M(3) + M(5)).val() == 1 && (M(3) * M(5)).val() == 1);
    static_assert(pow(M(3), -2).val() == 4 && inv(M(3)).val() == 5 && sqrt(M(2)).val() == 3);
    static_assert(M::init(6).val() == 6 && M::raw(6).val() == 6);
    static_assert([] { M r(4); return tryInv(M(3), r) && r.val() == 5; }());
    static_assert([] { M r(4); return trySqrt(M(2), r) && r.val() == 3; }());
#if TEST_WIDE
    using G = TEST_TYPE<18446744069414584321ULL>;
    static_assert(pow(G(1), 511).val() == 1 && pow(G(1), 512).val() == 1);
    static_assert(pow(G(1), 65536).val() == 1 && pow(G(1), std::numeric_limits<lll>::min()).val() == 1);
    static_assert(pow(G(1), ~ulll(0)).val() == 1);
    static_assert(TEST_TYPE<18446744069414584321ULL>::is_prime && TEST_TYPE<18446744073709551557ULL>::is_prime);
    static_assert(!TEST_TYPE<18446744073709551615ULL>::is_prime);
    static_assert(!TEST_TYPE<341550071728321ULL>::is_prime && !TEST_TYPE<3825123056546413051ULL>::is_prime);
#endif
#endif
}

inline void contexts() {
#if TEST_DYNAMIC
    EQUAL(global_default_product.val(), 757402647); EQUAL(global_custom_product.val(), 13);
    EQUAL(TEST_TYPE<811>::mod(), 17);
    using A = TEST_TYPE<1>; using B = TEST_TYPE<2>;
    EQUAL(A::mod(), 998244353); EQUAL(B::mod(), 998244353); REQUIRE(A::is_prime && B::is_prime);
    A::setMod(17); REQUIRE(A::is_prime); EQUAL(B::mod(), 998244353); A old = 11;
    B::setMod(12); REQUIRE(!B::is_prime); EQUAL(old.val(), 11);
    A::setMod(17, 0); REQUIRE(!A::is_prime && A::isPrime()); old = 13; EQUAL(old.val(), 13);
    A::setMod(97, 1); REQUIRE(A::is_prime); old = 96; EQUAL((old * old).val(), 1);
    int count = test_mode == "quick" ? 100 : test_mode == "full" ? 2000 : 10000;
    for (int m = 1; m <= count; m++) { A::setMod(m); context = "primality mod=" + std::to_string(m); EQUAL(A::is_prime, primeOracle(m)); }
    for (auto [m, prime] : vector<pair<ulng, bool>>{{561, false}, {2047, false}, {1373653, false}, {25326001, false}, {998244353, true}, {4294967291ULL, true}, {4294967295ULL, false}
#if TEST_WIDE
        , {341550071728321ULL, false}, {3825123056546413051ULL, false}, {18446744073709551557ULL, true}, {18446744073709551615ULL, false}
#endif
    }) { A::setMod(decltype(A::mod())(m)); EQUAL(A::is_prime, prime); }
    for (ulng m : vector<ulng>{1, 2, 4, 17, 97, 4294967295ULL, 17, 1}) {
        A::setMod(decltype(A::mod())(m)); old = -1; EQUAL(old.val(), m - 1); EQUAL((old * old).val(), 1 % m); EQUAL(B::mod(), 12); }
    cout << "PASS context defaults/IDs/reset/primality\n";
#endif
}

inline void protocol() {
    char op; ulng m;
    while (cin >> op >> m) {
        dispatch(m, [&]<class M>() {
            if (op == 'N') {
                char sign; string s; cin >> sign >> s; M a, b;
                if (sign == 's') { lll x = parseSigned(s); a = M(x); b = x; }
                else { ulll x = parseWide(s); a = M(x); b = x; }
                cout << a.val() << ' ' << b.val() << '\n';}
            else if (op == 'A') {
                ulng a, b; cin >> a >> b; M x = a, y = b, out = M(m - 1); bool ok = tryInv(y, out);
                cout << (x + y).val() << ' ' << (x - y).val() << ' ' << (x * y).val() << ' ' << ok << ' ' << out.val();
                if (ok) { cout << ' ' << (x / y).val(); } cout << '\n';}
            else if (op == 'P') {
                ulng a; char sign; string s; cin >> a >> sign >> s;
                if (sign == 's') { cout << pow(M(a), parseSigned(s)).val() << '\n'; }
                else { cout << pow(M(a), parseWide(s)).val() << '\n'; }}
            else if (op == 'R') {
                ulng a; cin >> a; M out = M(m - 1); bool ok = trySqrt(M(a), out);
                cout << ok << ' ' << out.val() << ' ' << sqrt(M(a)).val() << '\n';}
            else if (op == 'S') { string s; cin >> s; std::istringstream in(s); M a = 7; in >> a; cout << in.fail() << ' ' << a.val() << '\n'; }
            else { fail("protocol operation", __LINE__); }
        });}}

inline int death(const string &what) {
    if (what == "raw") { dispatch(17, []<class M>() { (void)M::raw(17); }); }
    else if (what == "init") { dispatch(17, []<class M>() { (void)M::init(17); }); }
    else if (what == "inverse") { dispatch(4, []<class M>() { (void)inv(M(2)); }); }
    else if (what == "division") { dispatch(4, []<class M>() { (void)(M(1) / M(2)); }); }
    else if (what == "negative-power") { dispatch(4, []<class M>() { (void)pow(M(2), -1); }); }
    else if (what == "root") { dispatch(4, []<class M>() { M out; (void)trySqrt(M(1), out); }); }
    else if (what == "legacy-root") { dispatch(4, []<class M>() { (void)sqrt(M(1)); }); }
    else if (what == "narrow") { dispatch(4294967295ULL, []<class M>() { (void)int(M(2147483648ULL)); }); }
#if TEST_DYNAMIC
    else if (what == "zero-modulus") { TEST_TYPE<9>::setMod(0); }
    else if (what == "false-prime") { TEST_TYPE<9>::setMod(4, 1); }
    else if (what == "prime-flag") { TEST_TYPE<9>::setMod(17, 2); }
    else if (what == "unknown-root") { using M = TEST_TYPE<9>; M::setMod(17, 0); M out; (void)trySqrt(M(1), out); }
#endif
    else { fail("unknown death case", __LINE__); }
    cerr << "precondition did not abort: " << what << '\n'; return 2;}

int main(int argc, char **argv) {
    if (argc > 1 && string(argv[1]) == "--oracle") { protocol(); return 0; }
    if (argc > 2 && string(argv[1]) == "--death") { return death(argv[2]); }
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        if (a == "--seed" && i + 1 < argc) { test_seed = std::stoull(argv[++i]); }
        else if (a == "--mode" && i + 1 < argc) { test_mode = argv[++i]; }
        else { fail("valid option", __LINE__, a); }}
    compileTime(); contexts();
    vector<ulng> mods{1, 2, 4, 7, 17, 97, 998244353, 2147483647, 2147483648, 4294967291ULL, 4294967295ULL};
#if TEST_WIDE
    mods.insert(mods.end(), {4294967296ULL, 2305843009213693951ULL, 9223372036854775808ULL, 18446744069414584321ULL, 18446744073709551557ULL, 18446744073709551615ULL});
#endif
    for (ulng m : mods) { dispatch(m, []<class M>() { verifyMod<M>(); }); }
#if TEST_DYNAMIC
    for (int m = 3; m <= (test_mode == "quick" ? 15 : test_mode == "full" ? 60 : 120); m++) {
        using M = TEST_TYPE<731>; M::setMod(m);
        for (int a = 0; a < m; a++) { for (int b = 0; b < m; b++) { pairCase<M>(a, b); } }
        roots<M>();}
#endif
    cout << "PASS all C++ features seed=" << test_seed << " mode=" << test_mode << '\n';}
