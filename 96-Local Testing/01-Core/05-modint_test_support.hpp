#pragma once
#include <sstream>
#include <type_traits>

// Shared fixtures test all six public types in 05-modint.hpp, never a copied implementation.
// REQUIRE remains live in -DNDEBUG configurations.
#ifndef TEST_FORM
#define TEST_FORM 0 // 1: a Montgomery-form type, whose moduli are odd and whose field n holds the residue times 2^64
#endif
#ifndef TEST_PORTABLE
#define TEST_PORTABLE 0 // 1: the runner's copy of the header that looks the stream's classification up as on another standard library
#endif
constexpr bool usable(ulng m) { return !TEST_FORM || m % 2 == 1; } // whether the fixture's type takes the modulus m
constexpr ulng formOracle(ulng v, ulng m) { return TEST_FORM ? ulng((ulll(v) << 64) % m) : v; } // the field n of the residue v
#if TEST_DYNAMIC
inline TEST_TYPE<810> global_default_product = TEST_TYPE<810>(1000000) * TEST_TYPE<810>(1000000);
inline int global_setup = [] { TEST_TYPE<811>::setMod(17); return 0; }();
inline TEST_TYPE<811> global_custom_product = TEST_TYPE<811>(1000000) * TEST_TYPE<811>(1000000);
// Biased construction, Euclid and both power loops during static initialization: every context field is constant-initialized.
template<class M> M startup() { // (-1) / 3 + 2 * (-2)^1000003: the second power repeats the first one's exponent
    M r = M(-1) / M(3);
    for (int i = 0; i < 2; i++) { r += pow(M(-2), 1000003); }
    return r;}
inline TEST_TYPE<810> global_default_startup = startup<TEST_TYPE<810>>();
inline TEST_TYPE<811> global_custom_startup = startup<TEST_TYPE<811>>();
#endif
inline string context;
inline ulng test_seed = 20260927;
inline string test_mode = "quick";
[[noreturn]] inline void fail(const string &expr, int line, const string &detail = "") {
    cerr << "FAIL line=" << line << " seed=" << test_seed << " mode=" << test_mode
         << " input={" << context << "} operation=" << expr << ' ' << detail << '\n';
    std::exit(1);}
#define REQUIRE(x) do { if (!(x)) { fail(#x, __LINE__, "expected=true actual=false"); } } while (0)
template<class A, class B> bool differ(A a, B b) {
    if constexpr (std::is_integral_v<A> && std::is_integral_v<B> && !std::is_same_v<A, bool> && !std::is_same_v<B, bool>) { return std::cmp_not_equal(a, b); }
    else { return a != b; }}
#define EQUAL(a, b) do { auto actual_ = (a); auto expected_ = (b); if (differ(actual_, expected_)) { \
    fail(#a, __LINE__, "expected=" + std::to_string(expected_) + " actual=" + std::to_string(actual_)); } } while (0)

constexpr bool primeOracle(ulng m) {
    if (m < 2) { return false; }
    for (ulng d = 2; d <= m / d; d++) { if (m % d == 0) { return false; } }
    return true;}

// The path sweeps record their case in plain fields; the reproducer text is built only when a check fails.
#define TEXT_(x) #x
#define TEXT(x) TEXT_(x)
struct Probe { const char *what; ulng mod; bool neg; ulll x, y; };
inline Probe probe{"", 0, false, 0, 0};
inline string decimal(ulll x) {
    string s;
    do { s += char('0' + int(x % 10)); x /= 10; } while (x);
    return string(s.rbegin(), s.rend());}
[[noreturn, gnu::noinline]] inline void failProbe(const char *expr, int line, ulng want, ulng got) {
    context = string(probe.what) + " type=" + TEXT(TEST_TYPE) + " mod=" + std::to_string(probe.mod) + " x=" + (probe.neg ? "-" : "") + decimal(probe.x) + " y=" + decimal(probe.y);
    fail(expr, line, "expected=" + std::to_string(want) + " actual=" + std::to_string(got));}
#define CHECK(a, b) do { ulng got_ = ulng(a), want_ = ulng(b); if (got_ != want_) { failProbe(#a, __LINE__, want_, got_); } } while (0)

// Oracles: plain 128-bit arithmetic on a modulus the optimizer cannot see, schoolbook loops and searches; out of line, one copy each.
template<class T> constexpr bool SIGNED = std::is_signed_v<T> || std::is_same_v<T, lll>;
[[gnu::noipa]] constexpr ulng mulOracle(ulng a, ulng b, ulng m) { return ulng(ulll(a) * b % m); }
[[gnu::noipa]] constexpr ulng normOracle(bool neg, ulll mag, ulng m) { ulng r = ulng(mag % m); return neg && r ? m - r : r; }
[[gnu::noinline]] constexpr ulng powOracle(ulng a, ulll e, ulng m) { // square-and-multiply from the top bit down
    int k = 0;
    for (ulll t = e; t; t >>= 1) { k++; }
    ulng r = 1 % m;
    while (k--) { r = mulOracle(r, r, m); if (e >> k & 1) { r = mulOracle(r, a % m, m); } }
    return r;}
[[gnu::noinline]] constexpr bool invOracle(ulng a, ulng m, ulng &out) { // signed 128-bit extended Euclid, confirmed by the product
    lll r0 = lll(m), r1 = lll(a), x0 = 0, x1 = 1;
    while (r1) { lll q = r0 / r1; r0 = std::exchange(r1, r0 - q * r1); x0 = std::exchange(x1, x0 - q * x1); }
    if (r0 != 1) { return false; }
    out = ulng(x0 < 0 ? x0 + lll(m) : x0); REQUIRE(out < m && mulOracle(a, out, m) == 1 % m); return true;}
[[gnu::noinline]] inline bool primeWide(ulng m) { // Miller-Rabin over the first twelve primes, deterministic below 3.3 * 10^24
    if (m < 2) { return false; }
    static const ulng BASES[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    for (ulng p : BASES) { if (m % p == 0) { return m == p; } }
    int s = std::countr_zero(m - 1);
    for (ulng a : BASES) {
        ulng x = powOracle(a, (m - 1) >> s, m); bool witness = x != 1 && x != m - 1;
        for (int i = 1; i < s && witness; i++) { x = mulOracle(x, x, m); witness = x != m - 1; }
        if (witness) { return false; }}
    return true;}
[[gnu::noinline]] inline ulng hornerOracle(const string &digits, ulng m) {
    ulng r = 0;
    for (char c : digits) { r = ulng((ulll(r) * 10 + ulng(c - '0')) % m); }
    return r;}
template<class C> C contextIn(const modint_detail::Value<C> &);
template<class M> using contextOf = decltype(contextIn(std::declval<M>())); // the context a type derives from: its mod() hides the context's word
[[gnu::noinline]] inline ulng biasOracle(ulng m) { // the least multiple of m from 2^63 on, m itself when it is one
    if (m >> 63) { return m; }
    ulng r = (ulng(1) << 63) % m;
    return (ulng(1) << 63) + (r ? m - r : 0);}
// m < 2^63: some shift k <= ceil(log2(m)) has a multiplier a < 2^64 with 2^(64 + k) < a * m <= 2^(64 + k) + 2^k,
// which then divides every 64-bit dividend (Granlund-Montgomery, theorem 4.2); with rng the division itself is sampled.
[[gnu::noinline]] inline bool shortOracle(ulng m, std::mt19937_64 *rng = nullptr) {
    for (int k = 0; k < 64; k++) {
        ulll top = ulll(1) << (64 + k), a = top / m + 1;
        if (!(a >> 64) && a * m <= top + (ulll(1) << k)) {
            if (rng) {
                ulng last = ~ulng(0) / m * m;
                for (ulng n : {~ulng(0), last, last - 1, last - m, last - m - 1, m - 1, m, (*rng)(), (*rng)() / m * m, (*rng)() / m * m - 1}) { REQUIRE(ulng(ulll(n) * a >> 64 >> k) == n / m); }}
            return true;}
        if (ulll(1) << k >= m) { break; }}
    return false;}
[[gnu::noinline]] inline bool unitOracle(ulng m) { // the definition: 2^w leaves the residue 1 for some w in [32, 64]
    ulng x = powOracle(2, 32, m);
    for (int w = 32; w <= 64; w++, x = mulOracle(x, 2, m)) { if (x == 1) { return true; } }
    return false;}
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
    M x; EQUAL(x.val(), 0); EQUAL(M::raw(0).val(), 0); EQUAL(M::raw(W(M::mod() - 1)).val(), M::mod() - 1);
    M y = M::raw(W(M::mod() - 1)); EQUAL(y.n, formOracle(y.val(), M::mod()));
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
    if constexpr (requires { M::norm(x); }) { EQUAL(M::norm(x), formOracle(want, M::mod())); }}

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
    EQUAL(M::mul(typename M::Word(a), typename M::Word(formOracle(b, m))), mul); // a form's product divides by 2^64
    M z = x; REQUIRE(&(z += y) == &z); EQUAL(z.val(), add);
    z = x; REQUIRE(&(z -= y) == &z); EQUAL(z.val(), sub);
    z = x; REQUIRE(&(z *= y) == &z); EQUAL(z.val(), mul);
    EQUAL((x + b).val(), add); EQUAL((a + y).val(), add);
    EQUAL((x - b).val(), sub); EQUAL((a - y).val(), sub);
    EQUAL((x * b).val(), mul); EQUAL((a * y).val(), mul);
    // Literal operands: a static product of a compile-time constant carries the hint that it is below the modulus.
    EQUAL((x * 3 + 1).val(), ulng((ulll(a) * 3 + 1) % m)); EQUAL((-1 * x - 1).val(), ulng((ulll(m) * 2 - a - 1) % m));
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
    // Both sides of every size at which batchInv changes its chains (BATCH_* in the header), and sizes with every remainder of 2, 4, 8 and 32.
    for (int n : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 21, 22, 31, 32, 33, 47, 48, 63, 64, 65, 79, 80, 95, 96, 97, 191, 192, 193, 255, 256, 257, 511,
                  512, 513, 2047, 2048, 2049, 2100}) {
        vector<M> a(n); std::mt19937_64 rng(test_seed + ulng(n));
        for (int i = 0; i < n; i++) {
            ulng x; do { x = rng() % m; } while (std::gcd(x, m) != 1);
            a[i] = x;}
        REQUIRE(batchInv(a, out)); EQUAL(out.size(), a.size());
        for (int i = 0; i < n; i++) { EQUAL(ulng(ulll(a[i].val()) * out[i].val() % m), 1 % m); }
        auto alias = a; REQUIRE(batchInv(alias, alias)); REQUIRE(alias == out);
        if (m > 1) {
            for (int bad : {0, n / 2, n - 1}) {
                auto b = a; b[bad] = 0; auto saved = out; REQUIRE(!batchInv(b, out)); REQUIRE(saved == out);
                saved = b; REQUIRE(!batchInv(b, b)); REQUIRE(saved == b);}}}
    for (ulng d : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        if (m > d && m % d == 0) { vector<M> a{M(1), M(d), M(1)}, saved = out; REQUIRE(!batchInv(a, out)); REQUIRE(out == saved); }}}

// A stream buffer over memory with get and put areas of B bytes; B = 0: none, every character through a virtual call. Each refill is its own
// allocation of exactly the bytes delivered, so a sanitizer sees a read behind them. A refill from offset `stop` on throws; a put throws
// (burst) or, once `limit` bytes are out, fails.
struct Feed : std::streambuf {
    string text, sink; size_t at = 0, area, stop = string::npos, limit = string::npos; vector<char> in, out; int syncs = 0; bool burst = false;
    Feed(string t, size_t b) : text(std::move(t)), area(b), out(b) { if (area) { setp(out.data(), out.data() + area); } }
    void flush() { if (area) { sink.append(pbase(), pptr()); setp(out.data(), out.data() + area); } }
    void refill() const { if (at >= stop) { throw std::runtime_error("refill"); } }
    int_type underflow() override {
        refill();
        if (at == text.size()) { return traits_type::eof(); }
        if (!area) { return traits_type::to_int_type(text[at]); }
        size_t k = std::min(area, text.size() - at); in = vector<char>(text.data() + at, text.data() + at + k); at += k; setg(in.data(), in.data(), in.data() + k);
        return traits_type::to_int_type(in[0]);}
    int_type uflow() override {
        if (area) { return std::streambuf::uflow(); }
        refill(); return at == text.size() ? traits_type::eof() : traits_type::to_int_type(text[at++]);}
    int_type overflow(int_type c) override {
        if (burst) { throw std::runtime_error("put"); }
        if (sink.size() >= limit) { return traits_type::eof(); }
        flush();
        if (!traits_type::eq_int_type(c, traits_type::eof())) { sink += traits_type::to_char_type(c); }
        return traits_type::not_eof(c);}
    int sync() override { ++syncs; flush(); return 0; }
    string unread() const { return (gptr() ? string(gptr(), egptr()) : string()) + text.substr(at); }
    string written() const { return area ? sink + string(pbase(), pptr()) : sink; }
};
// The classic character classes with the characters of `more` added to white space and those of `less` removed; the table is a base so
// that it is filled first.
struct SpaceTable {
    std::ctype_base::mask tab[std::ctype<char>::table_size];
    SpaceTable(const string &more, const string &less) {
        std::copy(std::ctype<char>::classic_table(), std::ctype<char>::classic_table() + std::ctype<char>::table_size, tab);
        for (char c : more) { tab[uint8_t(c)] = std::ctype_base::mask(tab[uint8_t(c)] | std::ctype_base::space); }
        for (char c : less) { tab[uint8_t(c)] = std::ctype_base::mask(tab[uint8_t(c)] & ~std::ctype_base::space); }}
};
struct Spaces : SpaceTable, std::ctype<char> {
    Spaces(const string &more, const string &less) : SpaceTable(more, less), std::ctype<char>(tab) {}
};
// What a stream operation threw: 0 nothing, 1 the stream's failure, 2 the buffer's error.
template<class F> int thrown(F f) {
    try { f(); }
    catch (const std::ios_base::failure &) { return 1; }
    catch (const std::runtime_error &) { return 2; }
    return 0;}
inline void arm(std::ios &s, std::ios::iostate mask) { try { s.exceptions(mask); } catch (const std::ios_base::failure &) {} } // the mask may meet a state already set
[[gnu::cold]] inline string tokens(std::mt19937_64 &rng) { // up to six tokens of 0 to 64 digits, some signed, some with a foreign character, between white space of several kinds
    string text; auto pick = [&](std::initializer_list<int> v) { return v.begin()[rng() % v.size()]; };
    for (int k = int(rng() % 7); k-- > 0;) {
        for (int w = int(rng() % 3); w-- > 0;) { text += " \n\t\r\v\f, "[rng() % 8]; }
        if (rng() % 4 == 0) { text += "+-"[rng() % 2]; }
        for (int d = pick({0, 1, 1, 2, 6, 7, 8, 9, 9, 15, 16, 17, 18, 19, 20, 23, 24, 25, 26, 27, 33, 36, 37, 40, 64}); d-- > 0;) { text += char('0' + int(rng() % 10)); }
        if (rng() % 8 == 0) { text += "x.-+/:\xB0"[rng() % 7]; }
        if (k || rng() % 2) { text += " \n\t\r\v\f, "[rng() % 8]; }}
    return text;}
// operator>> against the stream library's own token read and operator<< against its integer output, on string streams and on Feed buffers of
// many sizes (a token across a refill, at the end of input, without any area), with a width, noskipws, a tied stream, a state already set,
// an exception mask, a buffer that throws or stops taking bytes, and locales whose white space is not the classic set (no digit or sign in
// it: streamLocales): states, exceptions, values, the unread input, the put area and the tied stream's flushes agree. Mode 6 changes the
// locale through ios_base alone: the classification libstdc++ keeps in the stream stays, and the reader goes by it as the library's
// sentry does; a lookup in the locale itself (TEST_PORTABLE) follows the change.
template<class M> void streamModel() {
    using W = typename M::Word;
    ulng m = M::mod(); std::mt19937_64 rng(test_seed ^ m ^ 0x606); int count = test_mode == "quick" ? 60 : test_mode == "full" ? 600 : 6000;
    static const std::locale commas(std::locale::classic(), new Spaces(",", "")), lines(std::locale::classic(), new Spaces("", " "));
    auto pick = [&](std::initializer_list<int> v) { return v.begin()[rng() % v.size()]; };
    auto mask = [&] {
        return std::ios::iostate(rng() % 3 ? 0 : pick({int(std::ios::failbit), int(std::ios::eofbit), int(std::ios::badbit), int(std::ios::failbit | std::ios::eofbit),
                                                       int(std::ios::failbit | std::ios::badbit | std::ios::eofbit)}));};
    auto residue = [&]() { // any residue, a small one, or one beside a power of ten: every digit count and both sides of each
        ulng r = rng(), p = 1; int kind = int(rng() % 4);
        if (kind == 0) { r %= 1000; }
        if (kind == 1) { for (int k = int(rng() % 20); k-- > 0;) { p *= 10; } r = p - 1 + rng() % 3; }
        return W(r % m);};
    for (int round = 0; round < count; round++) {
        string text = tokens(rng);
        size_t area = size_t(pick({0, 1, 2, 3, 7, 8, 9, 10, 15, 16, 17, 18, 19, 20, 24, 25, 26, 27, 28, 40, 8191})); int mode = int(rng() % 10), width = rng() % 5 ? 0 : 1 + int(rng() % 12);
        int kind = int(rng() % 6), preset = int(rng() % 3); // kind 0: a string buffer for input, 1: one for both directions, else a Feed
        auto which = kind == 1 ? std::ios::in | std::ios::out : std::ios::in; std::stringbuf sa(text, which), sb(text, which);
        Feed fa(text, area), fb(text, area), ta("", 4), tb("", 4); std::istream a(&fa), b(&fb); std::ostream oa(&ta), ob(&tb);
        if (kind < 2) { a.rdbuf(&sa); b.rdbuf(&sb); }
        if (rng() % 10 == 0) { fa.stop = fb.stop = rng() % (text.size() + 1); }
        auto ex = mask();
        for (std::istream *s : {&a, &b}) {
            if (mode == 1) { *s >> std::noskipws; }
            if (mode == 2) { s->imbue(commas); }
            if (mode == 3) { s->imbue(lines); }
            if (mode == 5) { s->setstate(preset == 0 ? std::ios::failbit : preset == 1 ? std::ios::eofbit : std::ios::badbit); }
            if (mode == 6) { if (s == &a) { s->std::ios_base::imbue(commas); } else if (TEST_PORTABLE) { s->imbue(commas); } }
            arm(*s, ex);}
        if (mode == 4) { a.tie(&oa); b.tie(&ob); }
        context = "stream model mod=" + std::to_string(m) + " text=[" + text + "] area=" + std::to_string(kind < 2 ? -1 - kind : int(area)) + " mode=" + std::to_string(mode) + " width=" + std::to_string(width)
            + " stop=" + std::to_string(lng(fa.stop)) + " mask=" + std::to_string(int(ex)) + " preset=" + std::to_string(preset);
        for (int reads = 0; reads < 9; reads++) {
            a.width(width); b.width(width);
            M x = M::raw(W(m - 1)); ulng want = m - 1; string token;
            int ea = thrown([&] { a >> x; }), eb = thrown([&] {
                if (b >> token) {
                    size_t i = token[0] == '+' || token[0] == '-';
                    if (i < token.size() && token.find_first_not_of("0123456789", i) == string::npos) {
                        want = hornerOracle(token.substr(i), m); if (token[0] == '-' && want) { want = m - want; }}
                    else { b.setstate(std::ios::failbit); }}});
            EQUAL(ea, eb); EQUAL(int(a.rdstate()), int(b.rdstate())); EQUAL(x.val(), want); EQUAL(a.width(), b.width()); EQUAL(ta.syncs, tb.syncs);
            if (kind >= 2) { EQUAL(fa.unread() == fb.unread(), true); }
            if (a.eof()) { break; }
            for (std::istream *s : {&a, &b}) { s->exceptions(std::ios::goodbit); s->clear(); arm(*s, ex); }}
        fa.stop = fb.stop = string::npos; a.exceptions(std::ios::goodbit); b.exceptions(std::ios::goodbit);
        EQUAL(string(std::istreambuf_iterator<char>(a.rdbuf()), {}) == string(std::istreambuf_iterator<char>(b.rdbuf()), {}), true);}
    for (int round = 0; round < count; round++) {
        size_t area = size_t(pick({0, 1, 2, 3, 19, 20, 21, 22, 25, 39, 40, 41, 64, 8192})); int kind = int(rng() % 6);
        auto which = kind == 1 ? std::ios::in | std::ios::out : std::ios::out; std::stringbuf sa(which), sb(which);
        Feed fa("", area), fb("", area), ta("", 4), tb("", 4); std::ostream a(&fa), b(&fb), oa(&ta), ob(&tb);
        if (kind < 2) { a.rdbuf(&sa); b.rdbuf(&sb); }
        if (rng() % 8 == 0) { fa.burst = fb.burst = true; }
        if (rng() % 8 == 0) { fa.limit = fb.limit = rng() % 30; }
        auto ex = mask();
        context = "stream model mod=" + std::to_string(m) + " output area=" + std::to_string(kind < 2 ? -1 - kind : int(area)) + " burst=" + std::to_string(fa.burst) + " limit=" + std::to_string(lng(fa.limit))
            + " mask=" + std::to_string(int(ex));
        for (int writes = 0; writes < 12; writes++) {
            int width = rng() % 4 ? 0 : int(rng() % 26), state = int(rng() % 40); char fill = " 0*"[rng() % 3];
            auto base = pick({0, 0, 0, 0, int(std::ios::dec), int(std::ios::dec), int(std::ios::oct), int(std::ios::hex), int(std::ios::oct | std::ios::hex), int(std::ios::dec | std::ios::hex)});
            auto adjust = pick({0, int(std::ios::left), int(std::ios::right), int(std::ios::internal)});
            auto flags = std::ios::fmtflags(base) | std::ios::fmtflags(adjust) | (rng() % 4 ? std::ios::fmtflags(0) : std::ios::showbase) | (rng() % 4 ? std::ios::fmtflags(0) : std::ios::showpos)
                | (rng() % 4 ? std::ios::fmtflags(0) : std::ios::uppercase) | (rng() % 6 ? std::ios::fmtflags(0) : std::ios::unitbuf);
            bool tied = rng() % 6 == 0;
            for (std::ostream *s : {&a, &b}) {
                s->exceptions(std::ios::goodbit); s->clear();
                s->flags(flags); s->width(width); s->fill(fill); s->tie(tied ? s == &a ? &oa : &ob : nullptr);
                if (state < 3) { s->setstate(state == 0 ? std::ios::failbit : state == 1 ? std::ios::badbit : std::ios::eofbit); }
                arm(*s, ex);}
            M x = M::raw(residue());
            int ea = thrown([&] { REQUIRE(&(a << x) == &a); }), eb = thrown([&] { b << x.val(); });
            EQUAL(ea, eb); EQUAL(int(a.rdstate()), int(b.rdstate())); EQUAL(a.width(), b.width()); EQUAL(ta.syncs, tb.syncs); EQUAL(fa.syncs, fb.syncs);
            EQUAL((kind < 2 ? sa.str() : fa.written()) == (kind < 2 ? sb.str() : fb.written()), true);
            EQUAL(fa.out == fb.out, true);}}} // the whole put area: no byte behind the digits was written

// The reader's own rule where a locale classes a digit or a sign as white space (user decision 2026-11-21), against a model written out here:
// leading white space is the locale's; then a sign and every digit belong to the token whatever their class, and any other character of the
// locale's white space ends it. Both paths: get areas of many sizes, string buffers, noskipws, widths.
template<class M> void streamLocales() {
    using W = typename M::Word;
    ulng m = M::mod(); std::mt19937_64 rng(test_seed ^ m ^ 0xa0a); int count = test_mode == "quick" ? 60 : test_mode == "full" ? 600 : 6000;
    static const std::locale locales[] = {std::locale(std::locale::classic(), new Spaces("5", "")), std::locale(std::locale::classic(), new Spaces("-", "")),
                                          std::locale(std::locale::classic(), new Spaces("+,", " ")), std::locale(std::locale::classic(), new Spaces("0", "\n")),
                                          std::locale(std::locale::classic(), new Spaces("0123456789+-", ""))};
    auto pick = [&](std::initializer_list<int> v) { return v.begin()[rng() % v.size()]; };
    auto digit = [](char c) { return c >= '0' && c <= '9'; };
    for (int round = 0; round < count; round++) {
        string text = tokens(rng); int which = int(rng() % 5); const std::ctype<char> &ct = std::use_facet<std::ctype<char>>(locales[which]);
        auto space = [&](char c) { return ct.is(std::ctype_base::space, c); };
        size_t area = size_t(pick({0, 1, 2, 3, 7, 8, 9, 10, 15, 16, 17, 18, 19, 20, 24, 25, 26, 27, 28, 40, 8191})), pos = 0, n = text.size();
        bool skip = rng() % 4 != 0, strings = rng() % 6 == 0; int width = rng() % 5 ? 0 : 1 + int(rng() % 12);
        std::stringbuf sa(text, std::ios::in); Feed fa(text, area); std::istream a(&fa);
        if (strings) { a.rdbuf(&sa); }
        a.imbue(locales[which]);
        if (!skip) { a >> std::noskipws; }
        context = "stream locales mod=" + std::to_string(m) + " text=[" + text + "] area=" + std::to_string(strings ? -1 : int(area)) + " locale=" + std::to_string(which) + " skipws=" + std::to_string(skip)
            + " width=" + std::to_string(width);
        for (int reads = 0; reads < 9; reads++) {
            a.width(width); M x = M::raw(W(m - 1)); a >> x;
            ulng want = m - 1; int state = 0, after = 0; size_t p = pos;
            if (skip) { while (p < n && space(text[p])) { p++; } }
            if (skip && p == n) { state = std::ios::eofbit | std::ios::failbit; after = width; } // the sentry fails: the width stays
            else {
                size_t from = p, left = width ? size_t(width) : n + 1; bool neg = false, bad = false;
                if (p < n && (text[p] == '+' || text[p] == '-')) { neg = text[p] == '-'; p++; left--; }
                size_t first = p;
                for (; left && p < n && (digit(text[p]) || !space(text[p])); p++, left--) { bad |= !digit(text[p]); }
                if (left && p == n) { state |= std::ios::eofbit; }
                if (p == from || p == first || bad) { state |= std::ios::failbit; }
                else { want = hornerOracle(text.substr(first, p - first), m); if (neg && want) { want = m - want; } }}
            pos = p;
            EQUAL(int(a.rdstate()), state); EQUAL(x.val(), want); EQUAL(a.width(), after);
            if (!strings) { EQUAL(fa.unread() == text.substr(pos), true); }
            if (a.eof()) { break; }
            a.clear();}}}

// Every place a token can end against the end of its get area: areas of 1 to 32 bytes, 0 to 8 bytes of white space, then a token of 1 to
// 26 digits, some behind a sign, and a line break; the last token ends with the input. Then a sign without a digit, nine bytes before
// the end of the input: failbit, the destination kept and the line break unread.
template<class M> void streamEdges() {
    using W = typename M::Word;
    ulng m = M::mod(); std::mt19937_64 rng(test_seed ^ m ^ 0xb0b);
    for (size_t area = 1; area <= 32; area++) {
        for (size_t lead = 0; lead <= 8; lead++) {
            for (const char *sign : {"-", "+"}) {
                string text = string(lead, ' ') + sign + "\n12345678"; Feed f(text, area); std::istream in(&f); M x = M::raw(W(m - 1));
                context = "stream edge mod=" + std::to_string(m) + " text=[" + text + "] area=" + std::to_string(area);
                in >> x; EQUAL(int(in.rdstate()), int(std::ios::failbit)); EQUAL(x.val(), m - 1); EQUAL(f.unread() == "\n12345678", true);}
            for (int len = 1; len <= 26; len++) {
                string body; int kind = int(rng() % 8); bool neg = kind < 2;
                for (int i = 0; i < len; i++) { body += char('0' + int(rng() % 10)); }
                string text = string(lead, ' ') + (neg ? "-" : kind == 2 ? "+" : "") + body + "\n8"; ulng want = hornerOracle(body, m);
                Feed f(text, area); std::istream in(&f); M x = M::raw(W(m - 1)), y = x;
                context = "stream edge mod=" + std::to_string(m) + " text=[" + text + "] area=" + std::to_string(area);
                in >> x; EQUAL(int(in.rdstate()), 0); EQUAL(x.val(), neg && want ? m - want : want); EQUAL(f.unread() == "\n8", true);
                in >> y; EQUAL(int(in.rdstate()), int(std::ios::eofbit)); EQUAL(y.val(), 8 % m);}}}}

// A locale that groups digits: plain output writes none of its separators (user decision 2026-11-17); under a width or a base flag the
// stream formats the value itself, and groups.
struct Grouping : std::numpunct<char> {
    char do_thousands_sep() const override { return '\''; }
    string do_grouping() const override { return "\3"; }
};
template<class M> void streamGrouping() {
    static const std::locale grouped(std::locale::classic(), new Grouping); ulng m = M::mod();
    for (ulng v : {ulng(0), ulng(999), ulng(1000), ulng(1234567), m - 1, m / 2}) {
        M x = v % m; auto val = x.val(); context = "stream grouping mod=" + std::to_string(m) + " value=" + std::to_string(val);
        std::ostringstream a, b; a.imbue(grouped); b.imbue(grouped);
        a << x; EQUAL(a.str() == std::to_string(val), true);
        for (auto base : {std::ios::dec, std::ios::hex, std::ios::oct}) {
            a.str(""); b.str(""); a.setf(base, std::ios::basefield); b.setf(base, std::ios::basefield); a.width(base == std::ios::dec ? 30 : 0); b.width(a.width());
            a << x; b << val; EQUAL(a.str() == b.str(), true); EQUAL(a.str().find('\'') != string::npos, val >= (base == std::ios::dec ? 1000 : base == std::ios::hex ? 4096 : 512));}}}

template<class M> void streams() {
    ulng m = M::mod();
    for (const string &token : vector<string>{"0", "-0", "+0", "0001", "+0007", "-0007", "18446744073709551615", "-170141183460469231731687303715884105728", string(4096, '9')}) {
        ulng r = 0; bool neg = token[0] == '-';
        for (char c : token) { if (c >= '0' && c <= '9') { r = ulng((ulll(r) * 10 + uint(c - '0')) % m); } }
        if (neg && r) { r = m - r; }
        std::istringstream in(" \t\n" + token + " 123"); M a = 4; REQUIRE(&(in >> a) == &in); REQUIRE(!in.fail()); EQUAL(a.val(), r);
        int next; in >> next; EQUAL(next, 123); std::ostringstream os; REQUIRE(&(os << a) == &os); EQUAL(os.str() == std::to_string(r), true);}
    for (const string &token : vector<string>{"", " ", "+", "-", "x", "1x", "++1", "--1", "+-1", "0xFF", "1.0"}) {
        M a = 7; auto saved = a; std::istringstream in(token); in >> a; REQUIRE(in.fail()); REQUIRE(a == saved);}
    streamModel<M>(); streamLocales<M>(); streamEdges<M>(); streamGrouping<M>();}

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
            REQUIRE(trySqrt(M(a), r)); EQUAL(r.val(), std::min(x, x ? m - x : 0)); EQUAL(sqrt(M(a)).val(), r.val());}}}

template<class M> void verifyMod() {
    context = "construction mod=" + std::to_string(M::mod()); traits<M>(); constructors<M>(); streams<M>(); batches<M>();
    ulng m = M::mod();
    using Wide = typename M::Wide;
    for (Wide a : {Wide(0), Wide(1), Wide(m - 1), Wide(m), Wide(m) + 1, ~Wide(0)}) {
        EQUAL(M::red(a), ulng(a % m));}
    EQUAL(M::isPrime(), M::is_prime);
    if (m <= 97) {
        for (ulng a = 0; a < m; a++) { for (ulng b = 0; b < m; b++) { pairCase<M>(a, b); } }}
    else {
        vector<ulng> edges{0, 1, m - 1, m / 2, m / 2 + 1, m - 2};
        for (ulng a : edges) { for (ulng b : edges) { pairCase<M>(a, b); } }
        std::mt19937_64 rng(test_seed ^ m); int count = test_mode == "quick" ? 100 : test_mode == "full" ? 3000 : 30000;
        for (int i = 0; i < count; i++) { pairCase<M>(rng() % m, rng() % m); }}
    roots<M>();
    EQUAL(pow(M(0), 0).val(), 1 % m); EQUAL(pow(M(0), 1).val(), 0U);
    if (m == 1) { EQUAL(pow(M(0), std::numeric_limits<lll>::min()).val(), 0U); }
    cout << "PASS arithmetic/constructors/batch/stream/root mod=" << m << '\n';}

// Path sweeps: one check per internal path of the header against the oracles above. The sweeps are compiled once (cold: cheap to compile) and
// reach each type through its table of thunks, which keep the build's optimization level; a static type instantiates only the parts its class needs.
#define SOURCES bool, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t, short, unsigned short, int, uint, long, unsigned long, \
    long long, unsigned long long, lll, ulll
struct Source { bool sign; int bits; };
template<class... T> vector<Source> sourceList() { return {Source{SIGNED<T>, std::is_same_v<T, bool> ? 1 : int(8 * sizeof(T))}...}; }
template<class M, class... T> ulng constructAs(int t, ulll p) { // M built from the pattern p held in source type number t
    ulng r = 0; int i = 0;
    ((i++ == t ? void(r = M(T(p)).val()) : void()), ...);
    return r;}
// The powers every POWER type takes twice against the oracle: constant-evaluated (canonical binary loop) and at run time (base-4, Montgomery).
inline constexpr ulll POWER_EXPONENTS[] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 255, 256, 511, 512, 513, 4096, (ulll(1) << 32) + 1, ulll(1) << 63, (ulll(1) << 64) - 1, ulll(1) << 64, (ulll(1) << 70) + 5};
inline constexpr lll POWER_NEGATIVES[] = {-1, -2, -16, -17, -4096, LLONG_MIN, std::numeric_limits<lll>::min()};
// tryInv in constant evaluation, where the header's loops run in C++ alone: small operands, quotients of the modulus and 320 operands of
// every bit length, units or not.
template<class M> constexpr bool inverseTable() {
    ulng m = M::mod(), s = m;
    auto ok = [&](ulng a) { ulng want = 0; M r; bool unit = invOracle(a, m, want); return tryInv(M(a), r) == unit && (!unit || r.val() == want); };
    for (ulng d = 1; d <= 120; d++) {
        for (ulng a : {d % m, (m - d % m) % m, (m - 1) / d, ((m - 1) / d + 1) % m}) { if (!ok(a)) { return false; } }}
    for (int i = 0; i < 320; i++) { // a linear congruential sequence, cut to 64 down to 1 bits
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        if (!ok((s >> i % 64) % m)) { return false; }}
    return true;}
template<class M> constexpr bool powerTable() {
    ulng m = M::mod();
    for (ulng a : {m - 1, 3 % m, m / 2}) { for (ulll e : POWER_EXPONENTS) { if (pow(M(a), e).val() != powOracle(a, e, m)) { return false; } } }
    for (lll e : POWER_NEGATIVES) { if (pow(M(m - 1), e).val() != powOracle(m - 1, ulll(0) - ulll(e), m)) { return false; } } // m - 1 is its own inverse
    return true;}
// val() of a result, after its stored word: every operation leaves n in [0, mod), in a form too.
template<class M> ulng stored(M r) { if (r.n >= M::mod()) { failProbe("stored word below mod", __LINE__, ulng(M::mod()) - 1, ulng(r.n)); } return ulng(r.val()); }
enum Part { POWER = 1, ROOT = 2, READ = 4, ALL = 7 };
struct Ops {
    ulng (*mod)(); ulng (*bias)(); ulng (*last)(); bool (*prime)(); bool direct, fixed; int bytes, parts;
    ulng (*construct)(int, ulll); // source type number, pattern
    ulng (*binary)(int, ulng, ulng); // 0 x + y, 1 x - y, 2 x * y, 3 x -= y, 4 -x, 5 Context::mul
    bool (*inverse)(int, ulng, ulng &); // 0 tryInv into raw(out), 1 tryInv onto its own argument, 2 inv, 3 1 / x
    ulng (*power)(int, ulng, ulll); // POWER: pow with the exponent held as 0 int, 1 ulng, 2 ulll, 3 long long, 4 lll; 5 powBlind (a form: pow)
    bool (*root)(int, ulng, ulng &); // ROOT: 0 trySqrt into raw(out), 1 sqrt
    bool (*read)(const string &, ulng &, int &); // READ: operator>> into raw(a), then the next int
};
template<class M, int PARTS> const Ops &ops() {
    using W = typename M::Word;
    if constexpr (StaticModularInt<M>) { static_assert((!(PARTS & POWER) || powerTable<M>()) && (!(PARTS & ROOT) || M::is_prime) && inverseTable<M>()); }
    static const Ops o{
        [] { return ulng(M::mod()); }, [] { return ulng(M::BIAS); }, [] { return ulng(M::last); }, [] { return bool(M::is_prime); },
        modint_detail::Direct<M>, StaticModularInt<M>, int(sizeof(W)), PARTS,
        [](int t, ulll x) { return constructAs<M, SOURCES>(t, x); },
        [](int op, ulng a, ulng b) {
            M x = a, y = b;
            return op == 0 ? stored(x + y) : op == 1 ? stored(x - y) : op == 2 ? stored(x * y) : op == 3 ? stored(x -= y) : op == 4 ? stored(-x) : ulng(M::mul(W(a), W(formOracle(b, M::mod()))));},
        [](int form, ulng a, ulng &out) {
            M x = a, r = M::raw(W(out)); bool ok = true;
            if (form == 0) { ok = tryInv(x, r); }
            else if (form == 1) { r = x; ok = tryInv(r, r); }
            else if (form == 2) { r = inv(x); }
            else { r = M(1) / x; }
            out = stored(r); return ok;},
        [](int kind, ulng a, ulll e) {
            if constexpr (PARTS & POWER) {
                M x = a;
#if TEST_FORM
                if (kind == 5) { ulng keep = M::last; M r = pow(x, ulng(e)); M::last = keep; return stored(r); }
#else
                if (kind == 5) { return stored(M::powBlind(x, ulng(e))); }
#endif
                return stored(kind == 0 ? pow(x, int(e)) : kind == 1 ? pow(x, ulng(e)) : kind == 2 ? pow(x, e) : kind == 3 ? pow(x, static_cast<long long>(e)) : pow(x, lll(e)));}
            else { return ulng(kind) + a + ulng(e); }},
        [](int form, ulng a, ulng &out) {
            M r = M::raw(W(out)); bool ok = !form;
            if constexpr (PARTS & ROOT) {
                if (form == 0) { ok = trySqrt(M(a), r); }
                else { r = sqrt(M(a)); }}
            (void)a; out = stored(r); return ok;},
        [](const string &text, ulng &a, int &next) {
            std::istringstream in(text); M x = M::raw(W(a)); bool good = false;
            if constexpr (PARTS & READ) { in >> x; good = !in.fail(); }
            if (good) { in >> next; }
            a = stored(x); return good;}};
    return o;}

[[gnu::cold]] inline vector<ulll> normPatterns(ulng m, std::mt19937_64 &rng, int count) { // 128-bit two's-complement patterns; each source type takes those it can hold
    vector<ulll> pats;
    auto around = [&](ulll v, int d) { for (int i = -d; i <= d; i++) { pats.push_back(v + ulll(lll(i))); pats.push_back(ulll(lll(i)) - v); } };
    for (int k = 0; k < 128; k++) { around(ulll(1) << k, 2); }
    for (int j = 1; j <= 3; j++) { around(ulll(j) * m, 2); }
    for (int k : {7, 8, 15, 16, 31, 32, 63, 64, 127}) { around((ulll(1) << k) / m * m, 1); }
    around(~ulll(0) / m * m, 1); around(ulng(~biasOracle(m)), 2);
    for (int i = 0; i < count; i++) {
        ulll v = (ulll(rng()) << 64 | rng()) >> (rng() % 128);
        around(v, 0); around(v / m * m, 1);}
    return pats;}
[[gnu::cold]] inline void normSweep(const Ops &o, int count) {
    static const vector<Source> types = sourceList<SOURCES>();
    ulng m = o.mod(), bias = biasOracle(m); std::mt19937_64 rng(test_seed ^ m ^ 0x101);
    for (ulll p : normPatterns(m, rng, count)) {
        for (int t = 0; t < int(types.size()); t++) {
            int bits = types[size_t(t)].bits - types[size_t(t)].sign; bool neg = types[size_t(t)].sign && lll(p) < 0; ulll mag = neg ? ulll(0) - p : p;
            if (bits < 127 && mag >> bits && !(neg && mag == ulll(1) << bits)) { continue; } // the type cannot hold it
            probe = {"construction from x; y = source type number in SOURCES", m, neg, mag, ulll(t)}; CHECK(o.construct(t, p), normOracle(neg, mag, m));}}
    probe = {"BIAS", m, false, 0, 0}; CHECK(o.bias(), bias);}

[[gnu::cold]] inline vector<ulng> residues(ulng m, std::mt19937_64 &rng, int count) { // boundaries, high bit patterns, the golden-section residue with the longest Euclid, then random
    vector<ulng> r{0, 1, 2, 3, m - 1, m - 2, m - 3, m / 2, m / 2 + 1, m / 3, ulng(ulll(m) * 11400714819323198485ULL >> 64), 0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL,
                   0xFFFFFFFF00000000ULL, 0x00000000FFFFFFFFULL, 0x8000000000000000ULL, 0x7FFFFFFFFFFFFFFFULL, ~ulng(0)};
    for (int i = 0; i < count; i++) { r.push_back(rng()); }
    for (ulng &a : r) { a %= m; }
    return r;}
[[gnu::cold]] inline void binaryCase(const Ops &o, ulng a, ulng b) {
    ulng m = o.mod(), diff = ulng((ulll(a) + m - b) % m), prod = mulOracle(a, b, m); probe = {"x op y", m, false, a, b};
    CHECK(o.binary(0, a, b), (ulll(a) + b) % m); CHECK(o.binary(1, a, b), diff); CHECK(o.binary(2, a, b), prod);
    CHECK(o.binary(3, a, b), diff); CHECK(o.binary(4, a, b), a ? m - a : 0); CHECK(o.binary(5, a, b), prod);}
[[gnu::cold]] inline void inverseCase(const Ops &o, ulng b) {
    ulng m = o.mod(), want = m - 1, out = m - 1; bool unit = invOracle(b, m, want);
    probe = {"inverse of x", m, false, b, 0}; CHECK(unit, std::gcd(b, m) == 1);
    CHECK(o.inverse(0, b, out), unit); CHECK(out, want);
    CHECK(o.inverse(1, b, out), unit); CHECK(out, unit ? want : b);
    if (unit) { o.inverse(2, b, out); CHECK(out, want); o.inverse(3, b, out); CHECK(out, want); }}
[[gnu::cold]] inline void ringSweep(const Ops &o, int count) {
    ulng m = o.mod(); std::mt19937_64 rng(test_seed ^ m ^ 0x202); vector<ulng> r = residues(m, rng, count);
    for (ulng a : r) { inverseCase(o, a); }
    for (ulng a = 0; a < m && a < 257; a++) { inverseCase(o, a); inverseCase(o, m - 1 - a); }
    for (ulng d = 1; d < 400; d++) { inverseCase(o, (m - 1) / d); inverseCase(o, ((m - 1) / d + 1) % m); inverseCase(o, m / d % m); } // quotients of the modulus: long runs of one step
    r.resize(std::min(r.size(), size_t(24)));
    for (ulng a : r) { for (ulng b : r) { binaryCase(o, a, b); } }
    for (int i = 0; i < 20 * count; i++) { binaryCase(o, rng() % m, rng() % m); }}

// pow against the oracle (a negative exponent through the oracle's inverse), and the exponent the type remembers afterwards: one in
// [from, 2^64), from being where pow asks whether it changed: 4, but 8 for a dynamic Mersenne61, 5 for a form and for an odd modulus
// from 2^32, 9 for a static odd one from 2^63; an even modulus from 2^32 never asks (a dynamic one asks at 2^64 - 1). powBlind: from 4.
[[gnu::cold]] inline void powerSweep(const Ops &o, int top, int count) {
    ulng m = o.mod(); std::mt19937_64 rng(test_seed ^ m ^ 0x303); bool quick = test_mode == "quick";
    bool two = std::has_single_bit(m), direct = !TEST_FORM && (o.bytes == 4 || (o.fixed && (two || m == Barrett64::MERSENNE61 || (m >> 32 == 0 && shortOracle(m)) || (m >> 31 == 0 && !std::has_single_bit(m)))));
    ulng from = TEST_FORM ? 5 : direct || two || m >> 32 == 0 ? 4 : m == Barrett64::MERSENNE61 ? 8 : m % 2 == 0 ? ~ulng(0) : o.fixed && m >> 63 ? 9 : 5;
    auto asks = [&](ulll b) { return !(b >> 64) && b >= from && !(o.fixed && from == ~ulng(0)); };
    probe = {"Direct classification", m, false, 0, 0}; CHECK(o.direct, direct);
    auto unit = [&] { ulng a = rng() % m; while (std::gcd(a, m) != 1) { a = rng() % m; } return a; };
    auto check = [&](int kind, ulng a, ulll e) { // kind as in Ops::power, e the two's-complement pattern of the exponent
        ulng base = a, before = o.last(); bool neg = (kind == 0 || kind == 3 || kind == 4) && lll(e) < 0; ulll b = neg ? ulll(0) - e : e;
        if (neg) { REQUIRE(invOracle(a, m, base)); }
        ulng want = powOracle(base, b, m); probe = {"pow(y, x); exponent held as int, ulng, ulll, long long, lll by run order", m, neg, b, a};
        CHECK(o.power(kind, a, e), want); CHECK(o.last(), asks(b) ? ulng(b) : before);
        return want;};
    check(1, 1 % m, ulng(1) << 40); // no exponent below repeats this one: the first of two equal calls is a changed exponent
    vector<ulng> bases{rng() % m, rng() % m, 0, 1 % m, 2 % m, m - 1};
    for (size_t j = 0; j < bases.size(); j++) {
        for (int e = 0; e <= (j < 2 || !quick ? top : 64); e++) {
            ulng a = bases[j], want = check(0, a, ulll(e)); check(1, a, ulll(e));
            if (e >= 4) { CHECK(o.power(5, a, ulll(e)), want); }}}
    for (auto [e, f] : vector<pair<ulng, ulng>>{{16, 17}, {100, 3}, {15, 16}, {4095, 4096}, {65536, 65536}, {ulng(1) << 32, (ulng(1) << 32) + 1}, {ulng(1) << 63, ~ulng(0)}}) {
        for (int i = 0; i < 6; i++) { check(1, rng() % m, i & 1 ? f : e); }}
    for (int i = 0; i < count; i++) {
        ulng e = rng() >> (rng() % 64); check(1, rng() % m, e); check(1, rng() % m, e); check(2, rng() % m, e);
        if (e >= 4) { ulng a = rng() % m; probe = {"powBlind(y, x)", m, false, e, a}; CHECK(o.power(5, a, e), powOracle(a, e, m)); }
        check(2, rng() % m, ulll(rng() | 1) << 64 | e); check(3, unit(), ~ulll(e >> 1)); check(4, unit(), ~(ulll(rng() >> (rng() % 64)) << 64 | e));}
    for (lll e : {lll(-1), lll(-2), lll(-15), lll(-16), lll(-17), lll(-4096), lll(INT_MIN) + 1, lll(INT_MIN)}) { ulng a = unit(); check(0, a, ulll(e)); check(0, a, ulll(e)); check(3, a, ulll(e)); check(4, a, ulll(e)); }
    for (lll e : {lll(LLONG_MIN), lll(LLONG_MIN) + 1, lll(INT_MIN) - 1}) { ulng a = unit(); check(3, a, ulll(e)); check(3, a, ulll(e)); check(4, a, ulll(e)); }
    for (lll e : {std::numeric_limits<lll>::min(), std::numeric_limits<lll>::min() + 1, -(lll(1) << 64), -(lll(1) << 64) - 1, 1 - (lll(1) << 64), lll(LLONG_MIN) - 1}) { check(4, unit(), ulll(e)); }
    for (ulll e : {ulll(1) << 64, (ulll(1) << 64) + 1, (ulll(1) << 64) + 16, ulll(1) << 127, ~ulll(0)}) { check(2, rng() % m, e); check(2, m - 1, e); }
    for (ulng a : {m - 1, 3 % m, m / 2}) { for (ulll e : POWER_EXPONENTS) { check(2, a, e); } } // the table powerTable takes in constant evaluation
    for (lll e : POWER_NEGATIVES) { check(4, m - 1, ulll(e)); }}

// trySqrt and sqrt; root = ~0 when only its existence is known: the result must then square back and be the smaller root.
[[gnu::cold]] inline void rootCase(const Ops &o, ulng a, bool has, ulng root = ~ulng(0)) {
    ulng m = o.mod(), out = m - 1; probe = {"sqrt of x", m, false, a, root}; CHECK(o.root(0, a, out), has);
    if (has && root == ~ulng(0)) { root = out; CHECK(mulOracle(root, root, m), a); CHECK(root <= m - root, true); }
    CHECK(out, has ? root : m - 1); o.root(1, a, out); CHECK(out, has ? root : m - 1);}
[[gnu::cold]] inline void rootSweep(const Ops &o, int count, ulng limit) {
    if (!o.prime()) { return; }
    ulng m = o.mod(); std::mt19937_64 rng(test_seed ^ m ^ 0x404);
    if (m <= limit) { // every residue against the table of least roots
        vector<ulng> least(m, m);
        for (ulng x = 0; x <= m / 2; x++) { if (ulng a = x * x % m; least[a] == m) { least[a] = x; } }
        for (ulng a = 0; a < m; a++) { rootCase(o, a, least[a] != m, least[a]); }
        return;}
    auto residue = [&](ulng a) { return !a || powOracle(a, (m - 1) / 2, m) == 1; }; // Euler's criterion
    int s = std::countr_zero(m - 1); ulng z = 2;
    while (residue(z)) { z++; }
    ulng c = powOracle(z, (m - 1) >> s, m); // order 2^s
    rootCase(o, 0, true, 0); rootCase(o, 1, true, 1);
    for (int i = 0; i < s; i++) { // squares whose odd power has order exactly 2^i, each beside a non-residue
        for (int j = 0; j < count; j++) {
            ulng y = powOracle(rng() % (m - 1) + 1, ulll(1) << s, m), x = mulOracle(powOracle(c, ulll(1) << (s - 1 - i), m), y, m);
            rootCase(o, mulOracle(x, x, m), true, std::min(x, m - x)); rootCase(o, mulOracle(c, mulOracle(y, y, m), m), false);}}
    for (int j = 0; j < 20 * count; j++) {
        ulng a = rng() % m, x = rng() % m;
        rootCase(o, mulOracle(x, x, m), true, std::min(x, x ? m - x : 0)); rootCase(o, a, residue(a));}
    for (ulng a = 2; a < 64; a++) { rootCase(o, a, residue(a)); rootCase(o, m - a, residue(m - a)); }}

// operator>>: one token of optional sign and digits between whitespace, read in groups of eight digits, two groups per modular step; a failed
// read keeps the destination. The 18-digit prefixes below make the value read so far 0 mod m at group boundaries of several alignments.
struct StreamCase { string text; bool ok; ulng want; };
[[gnu::cold]] inline vector<StreamCase> streamCases(ulng m, std::mt19937_64 &rng) {
    constexpr ulng P18 = 1000000000000000000; vector<StreamCase> r;
    auto digits = [&](int n) { string s(size_t(n), '0'); for (char &c : s) { c = char('0' + int(rng() % 10)); } return s; };
    auto pad = [](ulng x) { string s = std::to_string(x); return string(18 - s.size(), '0') + s; };
    auto add = [&](const string &body) {
        for (const char *sign : {"", "+", "-"}) { ulng v = hornerOracle(body, m); r.push_back({sign + body, true, *sign == '-' && v ? m - v : v}); }};
    vector<int> lengths{53, 54, 55, 72, 73, 300, 1000};
    for (int n = 1; n <= 41; n++) { lengths.push_back(n); }
    for (int n : lengths) {
        add(digits(n)); add(string(size_t(n), '9')); add("1" + string(size_t(n - 1), '0')); add(string(size_t(n), '0'));}
    for (int z : {1, 17, 18, 19, 36, 40}) { for (int n : {1, 18, 19}) { add(string(size_t(z), '0') + digits(n)); } }
    for (int n : {1, 17, 18, 19, 36, 37}) { // the first 18-digit chunk is 0 mod m, then the first two together are
        add(pad(m < P18 ? (rng() % ((P18 - 1) / m) + 1) * m : 0) + digits(n));
        ulng first = rng() % P18, second = normOracle(true, ulll(first) * P18, m);
        if (second < P18) { add(pad(first) + pad(second) + digits(n)); add(pad(first) + pad(second)); }}
    add(std::to_string(m)); add(std::to_string(m) + string(18, '0')); add(decimal(ulll(m) * m));
    for (int n : {1, 8, 16, 20, 40}) { // every position; '/' and ':' border the digits, the last three have the top bit set (0xBA carries in the group test)
        for (int at = 0; at <= n; at++) {
            for (char bad : {'x', '/', ':', '.', '-', '+', char(0xB0), char(0xBA), char(0xFF)}) {
                if (!at && (bad == '-' || bad == '+')) { continue; }
                string s = digits(n); s.insert(size_t(at), 1, bad); r.push_back({s, false, 0}); r.push_back({"-" + s, false, 0});}}}
    return r;}
[[gnu::cold]] inline void streamSweep(const Ops &o) {
    ulng m = o.mod(); std::mt19937_64 rng(test_seed ^ m ^ 0x505);
    for (const StreamCase &c : streamCases(m, rng)) {
        ulng a = m - 1; int next = 0; bool good = o.read(" \n" + c.text + "\t77", a, next);
        if (good != c.ok || a != (c.ok ? c.want : m - 1) || (good && next != 77)) {
            context = "stream type=" + string(TEXT(TEST_TYPE)) + " mod=" + std::to_string(m) + " token=" + c.text;
            fail("in >> a", __LINE__, "expected=" + (c.ok ? std::to_string(c.want) : "failbit, destination " + std::to_string(m - 1)) + " actual=" + std::to_string(a) + (good ? "" : " failbit"));}}}

// Every context field setMod owns, against the modulus now set (also the default context before any setMod).
template<class M> void stateCheck() {
    using Wide = typename M::Wide; ulng m = M::mod(); probe = {"context after setMod, dividend x", m, false, 0, 0};
    CHECK(M::BIAS, biasOracle(m)); CHECK(M::reduction().mod, m); CHECK(&M::reduction() == &M::reducer, true);
    CHECK(M::is_prime, primeWide(m)); CHECK(M::isPrime(), primeWide(m));
    bool wide = sizeof(typename M::Word) == 8, big = m >> 32 != 0, two = std::has_single_bit(m); ulll w = (ulll(1) << 64) % m; // 2^64 mod m, taken in (0, m] for the select flag
    CHECK(M::small.v, wide && two ? 2 : wide && m == Barrett64::MERSENNE61 ? 1 : big ? 0 : ~ulng(0) / m + (two && m > 1));
    CHECK(M::sel, !two && !big && (w ? w : m) * m >> 62 != 0); CHECK(M::mont, big && m % 2 == 1 && m != Barrett64::MERSENNE61);
    CHECK(M::mthr.v, !big || two ? 4 : m == Barrett64::MERSENNE61 ? 8 : m % 2 == 0 ? ~ulng(0) : 5);
    CHECK(contextOf<M>::mod.v, m); CHECK(M::MOD, m); CHECK(M::bias.v, biasOracle(m)); CHECK(M::t64.v, w);
    if constexpr (sizeof(typename M::Word) == 4) { CHECK(M::t31.v, (ulng(1) << 31) % m); CHECK(M::selc, w >= m / 8); }
    else { CHECK(M::quot.v, two ? ~ulng(0) >> std::countr_zero(m) : ~ulng(0) / m); }
#if TEST_FORM
    CHECK(M::one(), w); CHECK(M::cube(), mulOracle(mulOracle(ulng(w), ulng(w), m), ulng(w), m)); CHECK(DynModInt64<731>::mod(), m); // the modulus of the canonical type of this ID
#endif
    for (Wide x : {Wide(0), Wide(m - 1), Wide(m), Wide(m - 1) * Wide(m - 1), ~Wide(0), ~Wide(0) / 3}) {
        probe.x = x; CHECK(M::reducer.reduce(x), x % m); CHECK(M::red(x), x % m);}
    if constexpr (sizeof(typename M::Word) == 8) {
        ulng odd = m | 1, d = m << std::countl_zero(m);
        CHECK(M::reciprocal.sh, std::countl_zero(m)); CHECK(M::reciprocal.d, d); CHECK(M::reciprocal.v, ~ulll(0) / d - (ulll(1) << 64));
        CHECK(M::montgomery.mod, odd); CHECK(M::montgomery.mod * M::montgomery.inv, ~ulng(0)); CHECK(M::montgomery.rsq, (~ulll(0) % odd + 1) % odd);}}

// The tables of the inverses below INV_TABLE, and the values just above them. A static modulus holds every word, 0 where there is none (a prime
// below INV_TABLE one word a residue). A dynamic type keeps the pair of each unit it has answered under the modulus now set, except under a
// power of two, whose lift answers; a planted word comes back: the table answers, not the loops.
template<class M> void tableInverses() {
    using W = typename M::Word; ulng m = M::mod(), size = ulng(modint_detail::INV_TABLE); probe = {"inverse of x from its table", m, false, 0, 0};
    for (ulng a = 1; a < size + 8 && a < m; ++a) {
        ulng want = 0; bool unit = invOracle(a, m, want); M x = M::raw(W(a)), y = x; probe.x = a;
        CHECK(tryInv(x, y), unit); CHECK(y.val(), unit ? want : a);
        if (a >= size) { continue; }
        if constexpr (StaticModularInt<M>) {
            if constexpr (M::is_prime && M::MOD < modint_detail::INV_TABLE && !TEST_FORM) { CHECK((modint_detail::InvTable<M, int(M::MOD)>::value[a]), want); }
            else if constexpr (!std::has_single_bit(ulng(M::MOD))) { CHECK((modint_detail::InvTable<M, modint_detail::INV_TABLE>::value[a]), unit ? formOracle(want, m) : 0); }}
        else {
            bool kept = unit && !std::has_single_bit(m); // a power of two answers by its lift
            CHECK(M::cache[a][0] == m, kept);
            if (kept) {
#if TEST_FORM
                using B = DynModInt64<731>; // the canonical type of this ID has the same modulus and its own words
                CHECK(inv(B::raw(a)).val(), want); CHECK(B::cache[a][1], want); CHECK(inv(x).val(), want);
#endif
                CHECK(M::cache[a][1], formOracle(want, m));
                M::cache[a][1] = W(formOracle((want + 1) % m, m)); CHECK(inv(x).val(), (want + 1) % m);
                M::cache[a][1] = W(formOracle(want, m)); CHECK(inv(x).val(), want);}}}}
// The lift of the static power of two 2^K, whose Newton steps the exponent sets: every value near both ends, random ones, and constant evaluation.
template<int K> void lift() {
    using W = typename TEST_TYPE<1>::Word; using M = TEST_TYPE<W(1) << K>; ulng m = M::mod(); std::mt19937_64 rng(test_seed ^ m ^ 0x707);
    probe = {"inverse of x under a power of two", m, false, 0, 0};
    auto one = [&](ulng a) { ulng want = 0; bool unit = invOracle(a, m, want); M x = M::raw(W(a)), y = x; probe.x = a; CHECK(tryInv(x, y), unit); CHECK(y.val(), unit ? want : a); };
    for (ulng a = 0; a < 4096 && a < m; ++a) { one(a); one(m - 1 - a); }
    for (int i = 0; i < 4096; i++) { one(rng() % m); }
    static_assert(inverseTable<M>());}
template<int... K> void lifts() { (lift<K>(), ...); cout << "PASS " << sizeof...(K) << " static powers of two, inverses by the lift\n"; }

template<class M, int PARTS> void paths() {
    bool quick = test_mode == "quick"; int count = quick ? 8 : test_mode == "full" ? 200 : 2000; const Ops &o = ops<M, PARTS>();
    if constexpr (!StaticModularInt<M>) { stateCheck<M>(); }
    normSweep(o, 8 * count); ringSweep(o, count); tableInverses<M>();
    if (PARTS & POWER) { powerSweep(o, o.direct || !quick ? 4096 : 64, 5 * count); }
    if (PARTS & ROOT) { rootSweep(o, quick ? 2 : test_mode == "full" ? 20 : 100, 4096); }
    if (PARTS & READ) { streamSweep(o); }
    cout << "PASS path sweeps construction/ring/inverse" << (PARTS & POWER ? "/power" : "") << (PARTS & ROOT ? "/root" : "") << (PARTS & READ ? "/stream" : "") << " mod=" << o.mod() << '\n';}

// Dispatch is deliberately bounded to the declared Python corpus moduli.
#if TEST_DYNAMIC
template<class F> void dispatch(ulng m, F &&f) { using M = TEST_TYPE<731>; M::setMod(decltype(M::mod())(m)); f.template operator()<M>(); }
#else
#define MOD_CASE(P) case P: f.template operator()<TEST_TYPE<P>>(); break
template<class F> void dispatch(ulng m, F &&f) {
    switch (m) {
    MOD_CASE(1); MOD_CASE(7); MOD_CASE(17); MOD_CASE(97); MOD_CASE(998244353); MOD_CASE(2147483647); MOD_CASE(4294967291); MOD_CASE(4294967295);
#if TEST_FORM
    MOD_CASE(15);
#else
    MOD_CASE(2); MOD_CASE(4); MOD_CASE(2147483648);
#endif
#if TEST_WIDE
    MOD_CASE(2305843009213693951ULL); MOD_CASE(18446744069414584321ULL); MOD_CASE(18446744073709551557ULL); MOD_CASE(18446744073709551615ULL);
#if !TEST_FORM
    MOD_CASE(4294967296ULL); MOD_CASE(9223372036854775808ULL);
#endif
#endif
    default: fail("supported modulus dispatch", __LINE__);}}
#undef MOD_CASE
#endif

inline void compileTime() {
#if TEST_FORM && !TEST_DYNAMIC
    using F = TEST_TYPE<7>;
    static_assert(F(-1).val() == 6 && (F(3) + F(5)).val() == 1 && (F(3) * F(5)).val() == 1 && F::raw(6).val() == 6 && F::raw(6).n == 5 && F(1).n == 2); // 2^64 = 2 mod 7
    static_assert(pow(F(3), -2).val() == 4 && inv(F(3)).val() == 5 && sqrt(F(2)).val() == 3 && F::mul(6, F(6).n) == 1);
    static_assert([] { F r(4); return tryInv(F(3), r) && r.val() == 5; }());
    static_assert([] { F r(4); return trySqrt(F(2), r) && r.val() == 3; }());
    static_assert([] { F a(6); ++a; --a; a--; a++; return a.val() == 6 && (-a).val() == 1 && F(lll(-1)).val() == 6 && F(~ulll(0)).val() == 3; }()); // 2^128 - 1 = 3 mod 7
    static_assert(TEST_TYPE<97>::is_prime && !TEST_TYPE<561>::is_prime && !TEST_TYPE<1>::is_prime && TEST_TYPE<18446744073709551557ULL>::is_prime && !TEST_TYPE<3825123056546413051ULL>::is_prime);
    static_assert(pow(TEST_TYPE<18446744069414584321ULL>(7), ~ulll(0)).val() == powOracle(7, ~ulll(0), 18446744069414584321ULL));
#endif
#if !TEST_DYNAMIC && !TEST_FORM
    using M = TEST_TYPE<7>;
    static_assert(!TEST_TYPE<1>::is_prime && TEST_TYPE<2>::is_prime && !TEST_TYPE<4>::is_prime);
    static_assert(TEST_TYPE<97>::is_prime && !TEST_TYPE<561>::is_prime && !TEST_TYPE<1373653>::is_prime);
    static_assert(!TEST_TYPE<25326001>::is_prime && TEST_TYPE<4294967291>::is_prime);
    static_assert(TEST_TYPE<4294967295>::mod() == 4294967295U);
    static_assert(M(-1).val() == 6 && (M(3) + M(5)).val() == 1 && (M(3) * M(5)).val() == 1);
    static_assert(pow(M(3), -2).val() == 4 && inv(M(3)).val() == 5 && sqrt(M(2)).val() == 3);
    static_assert(M::raw(6).val() == 6);
    static_assert(M::montgomery.mod == 7 && ulng(M::montgomery.mod * M::montgomery.inv) == ~ulng(0));
    static_assert(TEST_TYPE<4294967291>::canon(ulng(1) << 32) == (ulng(1) << 32) - 4294967291 && TEST_TYPE<4294967291>::canon(2 * ulng(4294967291) - 1) == 4294967290);
    static_assert(TEST_TYPE<2147483648>::canon(2 * ulng(2147483648) - 1) == 2147483647 && TEST_TYPE<2147483648>::canon(2147483648) == 0);
    static_assert([] { M r(4); return tryInv(M(3), r) && r.val() == 5; }());
    static_assert([] { M r(4); return trySqrt(M(2), r) && r.val() == 3; }());
#if TEST_WIDE
    using G = TEST_TYPE<18446744069414584321ULL>;
    static_assert(pow(G(1), 511).val() == 1 && pow(G(1), 512).val() == 1);
    static_assert(pow(G(1), 65536).val() == 1 && pow(G(1), std::numeric_limits<lll>::min()).val() == 1);
    static_assert(pow(G(1), ~ulll(0)).val() == 1);
    static_assert(G::montgomery.mod == 18446744069414584321ULL && TEST_TYPE<9223372036854775808ULL>::montgomery.mod == 9223372036854775809ULL);
    using P = TEST_TYPE<18446744073709551557ULL>;
    static_assert(P::canon(ulll(1) << 64) == 59 && P::canon(2 * ulll(18446744073709551557ULL) - 1) == 18446744073709551556ULL && P::canon(18446744073709551557ULL) == 0);
    static_assert(TEST_TYPE<18446744069414584321ULL>::is_prime && TEST_TYPE<18446744073709551557ULL>::is_prime);
    static_assert(!TEST_TYPE<18446744073709551615ULL>::is_prime);
    static_assert(!TEST_TYPE<341550071728321ULL>::is_prime && !TEST_TYPE<3825123056546413051ULL>::is_prime);
#endif
#endif
    static_assert(std::is_same_v<mint, ModInt<998244353>>);
    static_assert(std::is_same_v<decltype(ModInt<7>::mod()), uint>);
    static_assert(std::is_same_v<decltype(ModInt64<7>::mod()), ulng>);
    static_assert(std::is_same_v<decltype(DynModInt<>::mod()), uint>);
    static_assert(std::is_same_v<decltype(DynModInt64<>::mod()), ulng>);
    static_assert(std::is_same_v<DynModInt<>, DynModInt<0>> && std::is_same_v<DynModInt64<>, DynModInt64<0>>);
    static_assert(!std::is_same_v<ModInt<7>, ModInt64<7>> && !std::is_same_v<DynModInt<>, DynModInt64<>>);
    static_assert(!std::is_same_v<DynModInt<0>, DynModInt<1>> && !std::is_same_v<DynModInt64<0>, DynModInt64<1>>);
    static_assert(std::is_same_v<ModInt61, ModInt64<2305843009213693951ULL>>);
    static_assert(ModInt61::is_prime && ModInt61::mod() == 2305843009213693951ULL);
    constexpr ulll M61 = ModInt61::mod();
    static_assert(ModInt61::red(0) == 0 && ModInt61::red(M61 - 1) == M61 - 1 && ModInt61::red(M61) == 0 && ModInt61::red(2 * M61) == 0);
    static_assert(ModInt61::red((M61 << 61) - 1) == ((M61 << 61) - 1) % M61 && ModInt61::red(~ulll(0)) == ~ulll(0) % M61);
    static_assert(ModInt61::mul(ulng(M61 - 1), ulng(M61 - 1)) == 1 && ModInt61::mul(2, ulng(M61 - 1)) == M61 - 2);
    static_assert(ModInt61::canon(0) == 0 && ModInt61::canon(M61 - 1) == M61 - 1 && ModInt61::canon(M61) == 0 && ModInt61::canon(2 * M61 - 1) == M61 - 1);
    static_assert(ModularInt<TEST_TYPE<731>> && ModularInt<mint> && ModularInt<ModInt61> && !ModularInt<int> && !ModularInt<ulng>);
    static_assert(StaticModularInt<mint> && StaticModularInt<ModInt61> && !StaticModularInt<DynModInt<>> && !StaticModularInt<DynModInt64<>>);
    using modint_detail::Direct, modint_detail::unitShift, modint_detail::Reciprocal;
    static_assert(Direct<mint> && Direct<DynModInt<0>> && Direct<ModInt61> && Direct<ModInt64<998244353>> && Direct<ModInt64<4294967291>> && Direct<ModInt64<17>>);
    static_assert(Direct<ModInt64<7>> && Direct<ModInt64<469762049>> && Direct<ModInt64<167772161>> && Direct<ModInt64<2147483647>>); // a long reciprocal below 2^31: the exact round-up product
    static_assert(Direct<ModInt64<128>> && Direct<ModInt64<ulng(1) << 63>> && Direct<ModInt<1>>); // a power of two: the product is a mask
    static_assert(!Direct<ModInt64<3000000019ULL>> && !Direct<ModInt64<4294967311ULL>> && !Direct<ModInt64<4611686018427387847ULL>>);
    static_assert(!Direct<ModInt64<18446744073709551557ULL>> && !Direct<DynModInt64<0>>);
    static_assert(!Direct<MontModInt64<7>> && !Direct<MontModInt64<998244353>> && !Direct<MontModInt64<Barrett64::MERSENNE61>> && !Direct<DynMontModInt64<0>>); // a form keeps its own product
    static_assert(ModularInt<MontModInt64<7>> && StaticModularInt<MontModInt64<7>> && ModularInt<DynMontModInt64<>> && !StaticModularInt<DynMontModInt64<>>);
    static_assert(std::is_same_v<decltype(MontModInt64<7>::mod()), ulng> && std::is_same_v<DynMontModInt64<>, DynMontModInt64<0>> && !std::is_same_v<MontModInt64<7>, ModInt64<7>>);
    static_assert(unitShift(3) && unitShift(7) && unitShift(2147483647) && unitShift(4294967295U) && unitShift(Barrett64::MERSENNE61) && unitShift(~ulng(0)));
    static_assert(!unitShift(1) && !unitShift(2) && !unitShift(998244353) && !unitShift(1000000007) && !unitShift(4294967291U) && !unitShift(4294967311ULL));
    static_assert(!unitShift(9223372036854775783ULL) && !unitShift(9223372036854775837ULL) && !unitShift(18446744069414584321ULL) && !unitShift(18446744073709551557ULL));
    static_assert(Reciprocal(1).mul(0, ~ulng(0)) == 0 && Reciprocal(3).mul(2, ~ulng(0)) == 0 && Reciprocal(4294967311ULL).mul(4294967310ULL, ~ulng(0)) == mulOracle(4294967310ULL, ~ulng(0), 4294967311ULL));
    static_assert(Reciprocal(~ulng(0) - 58).mul(~ulng(0) - 59, ~ulng(0)) == mulOracle(~ulng(0) - 59, ~ulng(0), ~ulng(0) - 58) && Reciprocal(ulng(1) << 63).mul(1, ~ulng(0)) == ~ulng(0) >> 1);
#if !TEST_DYNAMIC && !TEST_FORM
    // BIAS of each swept static modulus: its least multiple from 2^63 on, the modulus itself from 2^63 on.
    constexpr auto bias = [](ulng m) { return ulng(((ulll(1) << 63) + m - 1) / m * m); };
#define BIAS_IS(...) static_assert(TEST_TYPE<__VA_ARGS__>::BIAS == bias(__VA_ARGS__))
    BIAS_IS(1); BIAS_IS(2); BIAS_IS(3); BIAS_IS(7); BIAS_IS(17); BIAS_IS(128); BIAS_IS(998244353); BIAS_IS(1000000007); BIAS_IS(167772161); BIAS_IS(469762049);
    BIAS_IS(2147483647); BIAS_IS(2147483648); BIAS_IS(2147483693); BIAS_IS(4294967291); BIAS_IS(4294967295);
#if TEST_WIDE
    BIAS_IS(4294967311ULL); BIAS_IS(4179340454199820289ULL); BIAS_IS(1000000000000000000ULL); BIAS_IS(2305843009213693951ULL); BIAS_IS(4611686018427387847ULL);
    BIAS_IS(9223372036854775783ULL); BIAS_IS(9223372036854775837ULL); BIAS_IS(18446744073709551557ULL);
#endif
#undef BIAS_IS
#endif
    static_assert(StaticModularInt<TEST_TYPE<731>> == !TEST_DYNAMIC);}

#if TEST_DYNAMIC
inline void contexts() {
    EQUAL(global_default_product.val(), 757402647); EQUAL(global_custom_product.val(), 13);
    for (auto [got, p] : {pair<ulng, ulng>{global_default_startup.val(), 998244353}, {global_custom_startup.val(), 17}}) {
        ulng third = 0; REQUIRE(invOracle(3, p, third)); EQUAL(got, (mulOracle(p - 1, third, p) + 2 * powOracle(p - 2, 1000003, p)) % p);}
    EQUAL(TEST_TYPE<811>::mod(), 17);
    using A = TEST_TYPE<1>; using B = TEST_TYPE<2>;
    EQUAL(A::mod(), 998244353); EQUAL(B::mod(), 998244353); REQUIRE(A::is_prime && B::is_prime); EQUAL(A::montgomery.mod, 998244353);
    constexpr ulng COMPOSITE = TEST_FORM ? 15 : 12;
    A::setMod(17); REQUIRE(A::is_prime); EQUAL(B::mod(), 998244353); A old = 11;
    B::setMod(COMPOSITE); REQUIRE(!B::is_prime); EQUAL(old.val(), 11); EQUAL(B::montgomery.mod, sizeof(typename B::Word) == 8 ? COMPOSITE | 1 : 998244353);
    A::setMod(17, 0); REQUIRE(!A::is_prime && A::isPrime()); old = 13; EQUAL(old.val(), 13);
    A::setMod(15, 1); REQUIRE(A::is_prime && !A::isPrime()); // prm = 1 is a promise, taken unchecked
    A::setMod(97, 1); REQUIRE(A::is_prime); old = 96; EQUAL((old * old).val(), 1);
    EQUAL(A::reduction().mod, 97); EQUAL(A::reduction().reduce(typename A::Wide(96) * 96), 1); EQUAL(A::mul(96, decltype(A::mod())(formOracle(96, 97))), 1);
    EQUAL(A::montgomery.mod, sizeof(typename A::Word) == 8 ? 97 : 998244353); EQUAL(A::montgomery.mod * A::montgomery.inv, ~ulng(0));
    A::setMod(decltype(A::mod())(sizeof(decltype(A::mod())) == 8 ? 2305843009213693951ULL : 17));
    EQUAL(A::reduction().mod, A::mod()); EQUAL(A::mul(A::mod() - 1, decltype(A::mod())(formOracle(A::mod() - 1, A::mod()))), 1); EQUAL(A::red(typename A::Wide(A::mod() - 1) * (A::mod() - 1)), 1);
    int count = test_mode == "quick" ? 100 : test_mode == "full" ? 2000 : 10000;
    for (int m = 1; m <= count; m++) { if (usable(ulng(m))) { A::setMod(m); context = "primality mod=" + std::to_string(m); EQUAL(A::is_prime, primeOracle(m)); } }
    for (auto [m, prime] : vector<pair<ulng, bool>>{{561, false}, {2047, false}, {1373653, false}, {25326001, false}, {998244353, true}, {4294967291ULL, true}, {4294967295ULL, false}
#if TEST_WIDE
        , {341550071728321ULL, false}, {3825123056546413051ULL, false}, {18446744073709551557ULL, true}, {18446744073709551615ULL, false}
#endif
    }) { A::setMod(decltype(A::mod())(m)); EQUAL(A::is_prime, prime); }
    // Literal operands: a compile-time constant reaches the reducers' fold of a zero dividend, from every width and with every kind of modulus.
    for (ulng m : vector<ulng>{1, 2, 7, 8, 998244353, ulng(~decltype(A::mod())(0))}) {
        if (!usable(m)) { continue; }
        A::setMod(decltype(A::mod())(m)); context = "literals mod=" + std::to_string(m);
        EQUAL(A(0).val(), 0); EQUAL(A(0U).val(), 0); EQUAL(A(0LL).val(), 0); EQUAL(A(0ULL).val(), 0); EQUAL(A(lll(0)).val(), 0); EQUAL(A(ulll(0)).val(), 0);
        EQUAL(A(1).val(), 1 % m); EQUAL(A(7).val(), 7 % m); EQUAL(A(8ULL).val(), 8 % m); EQUAL(A(-1).val(), m - 1); EQUAL(A(-8LL).val(), (m - 8 % m) % m);
        EQUAL(A(ulll(7)).val(), 7 % m); EQUAL(A(lll(-7)).val(), (m - 7 % m) % m); EQUAL(A(ulll(1) << 64).val(), ulng((ulll(1) << 64) % m));
        A x = 7; REQUIRE((x == 0) == (m == 1 || m == 7)); REQUIRE(x - 7 == 0); REQUIRE(x * 0 == 0); EQUAL((x + 0).val(), 7 % m);}
    for (ulng m : vector<ulng>{1, 2, 4, 17, 97, 4294967295ULL, 17, 1}) {
        if (!usable(m)) { continue; }
        A::setMod(decltype(A::mod())(m)); old = -1; EQUAL(old.val(), m - 1); EQUAL((old * old).val(), 1 % m); EQUAL(B::mod(), COMPOSITE);}
    cout << "PASS context defaults/IDs/reset/primality\n";}
#else
inline void contexts() {}
#endif

#if TEST_WIDE && !TEST_DYNAMIC
// Type-independent internals, run in this fixture alone: isPrime in both evaluation modes, Reciprocal::mul, biasOf, shortReciprocal, unitShift.
struct PrimeCase { ulng m; bool prime; };
inline constexpr PrimeCase PRIME_CASES[] = {
    {0, false}, {1, false}, {2, true}, {3, true}, {4, false}, {5, true}, {9, false}, {25, false}, {37, true}, {41, true}, {49, false}, {61, true}, {73, true}, {121, false}, {169, false}, {193, true},
    {1369, false}, {1681, false}, {1763, false}, {14089, false}, // 37^2, 41^2, 41 * 43, 73 * 193 (a witness of the seven is a multiple)
    {7, true}, {11, true}, {13, true}, {14, false}, {17, true}, {19, true}, {23, true}, {29, true}, {31, true}, {43, true}, {47, true}, {53, true}, {59, true}, {67, true}, {71, true}, {79, true},
    {289, false}, {5041, false}, {5183, false}, {5323, true}, {5329, false}, {5333, true}, {5767, false}, // 17^2, 71^2, 71 * 73, the primes around 73^2 = 5329, 73 * 79
    {30029, true}, {30030, false}, {30031, false}, {30043, false}, {30047, true}, {60077, true}, {510510, false}, {1021020 + 73 * 73, false}, // residues of 30030: -1, 0, 1 = 59 * 509, 13 = 13 * 2311, 17
    {561, false}, {1105, false}, {1729, false}, {41041, false}, {825265, false}, {321197185, false}, {5394826801ULL, false}, {232250619601ULL, false}, {9746347772161ULL, false},
    {1436697831295441ULL, false}, {60977817398996785ULL, false}, // Carmichael numbers
    {2047, false}, {3277, false}, {4033, false}, {4681, false}, {8321, false}, {1373653, false}, {25326001, false}, {3215031751ULL, false}, {4759123141ULL, false},
    {2152302898747ULL, false}, {3474749660383ULL, false}, {341550071728321ULL, false}, {3825123056546413051ULL, false}, // least strong pseudoprimes to growing base sets; 4759123141 passes 2, 7, 61
    {2269093, false}, {2284453, false}, {3539101, false}, {916327, false}, {2205967, false}, {2387797, false}, {79381, false}, {178709, false}, {721801, false},
    // the least composites past the trial primes that pass two of the witnesses 2, 7, 61: three each for {2, 7}, {2, 61}, {7, 61}
    {65521, true}, {65537, true}, {4293001441ULL, false}, {4295098369ULL, false}, {2147483647, true}, {2147483648, false}, {2147483659, true}, {4294967279ULL, true}, {4294967291ULL, true},
    {4294967295ULL, false}, {4294967296ULL, false}, {4294967297ULL, false}, {4294967311ULL, true}, // both sides of the witness-set switch at 2^32
    {18446744030759878681ULL, false}, {18446743979220271189ULL, false}, {4611686014132420609ULL, false}, // (2^32 - 5)^2, (2^32 - 5)(2^32 - 17), (2^31 - 1)^2
    {999999999999999989ULL, true}, {1000000000000000003ULL, true}, {1000000000000000009ULL, true}, {2305843009213693951ULL, true}, {2305843009213693953ULL, false}, {4611686018427387847ULL, true},
    {4611686018427387904ULL, false}, {4611686018427388039ULL, true}, {9223372036854775783ULL, true}, {9223372036854775808ULL, false}, {9223372036854775837ULL, true}, {4179340454199820289ULL, true},
    {18446744069414584321ULL, true}, {18446744073709551556ULL, false}, {18446744073709551557ULL, true}, {18446744073709551615ULL, false}};
constexpr int primeTable() { // index of the first table entry isPrime gets wrong, -1 when none
    int i = 0;
    for (PrimeCase c : PRIME_CASES) { if (modint_detail::isPrime(c.m) != c.prime) { return i; } i++; }
    return -1;}
constexpr int primeRange() {
    for (int m = 0; m < 6144; m++) { if (modint_detail::isPrime(ulng(m)) != primeOracle(ulng(m))) { return m; } } // past 73^2: the wheel, the trial primes, the bound
    return -1;}
static_assert(primeTable() == -1 && primeRange() == -1); // the constant-evaluated path: seven witnesses on plain 128-bit remainders
// The bases of the hashed test below 2^32 are data no sampled test reaches: this digest pins the table that the sieve run over
// every n below 2^32 (stress mode, below) has checked, so a changed entry fails to compile.
constexpr ulng baseDigest() {
    ulng h = 0xcbf29ce484222325ULL;
    for (uint16_t b : modint_detail::FJ32) { h = (h ^ b) * 0x100000001b3ULL; }
    return h;}
static_assert(baseDigest() == 0x61c727cfd77f7a03ULL);

[[gnu::cold]] inline void primality() {
    bool quick = test_mode == "quick", full = test_mode == "full"; std::mt19937_64 rng(test_seed ^ 0x606);
    auto check = [&](ulng m, bool want) { probe = {"isPrime(x) at run time", m, false, m, 0}; CHECK(modint_detail::isPrime(m), want); };
    for (PrimeCase c : PRIME_CASES) {
        probe = {"oracle primality of x against the table", c.m, false, c.m, 0}; CHECK(primeWide(c.m), c.prime);
        if (c.m >> 36 == 0) { CHECK(primeOracle(c.m), c.prime); }
        check(c.m, c.prime);}
    int n = 1 << (quick ? 17 : full ? 21 : 24); vector<bool> composite(size_t(n), false);
    for (lng i = 2; i * i < n; i++) { if (!composite[size_t(i)]) { for (lng j = i * i; j < n; j += i) { composite[size_t(j)] = true; } } }
    for (int i = 0; i < n; i++) {
        bool want = i >= 2 && !composite[size_t(i)]; check(ulng(i), want);
        if (i < 1 << 16) { probe.what = "oracle primality of x against the sieve"; CHECK(primeWide(ulng(i)), want); }}
    ulng w = quick ? 300 : full ? 20000 : 100000;
    for (ulng m = (ulng(1) << 32) - w; m <= (ulng(1) << 32) + w; m++) { // the witness-set switch, by trial division
        bool want = primeOracle(m); probe = {"oracle primality of x against trial division", m, false, m, 0}; CHECK(primeWide(m), want); check(m, want);}
    for (int k : {31, 61, 62, 63}) { for (ulng m = (ulng(1) << k) - w; m <= (ulng(1) << k) + w; m++) { check(m, primeWide(m)); } }
    for (ulng i = 0; i <= 2 * w; i++) { check(~ulng(0) - i, primeWide(~ulng(0) - i)); }
    int count = quick ? 2000 : full ? 50000 : 500000;
    for (int i = 0; i < count; i++) { ulng m = rng() >> (rng() % 64) | 1; check(m, primeWide(m)); }
    auto nextPrime = [](ulng x) { while (!primeWide(x)) { x++; } return x; };
    vector<ulng> near;
    for (ulng m = (ulng(1) << 32) - 400; m <= (ulng(1) << 32) + 400; m++) { if (primeOracle(m)) { near.push_back(m); } }
    for (ulng p : near) { for (ulng q : near) { if (ulll x = ulll(p) * q; !(x >> 64)) { check(ulng(x), false); } } }
    for (int i = 0; i < count / 100; i++) { // semiprimes of mixed factor sizes, then (k + 1)(2k + 1) and (6k + 1)(12k + 1)(18k + 1) with prime factors
        int bits = int(rng() % 25) + 8; ulng p = nextPrime(rng() >> (64 - bits) | 1), q = nextPrime(rng() >> bits | 1);
        if (ulll x = ulll(p) * q; !(x >> 64)) { check(ulng(x), false); }
        ulng k = rng() >> (33 + rng() % 28);
        while (!primeWide(k + 1) || !primeWide(2 * k + 1)) { k++; }
        check((k + 1) * (2 * k + 1), false);
        k = rng() % 200000 + 1;
        while (!primeWide(6 * k + 1) || !primeWide(12 * k + 1) || !primeWide(18 * k + 1)) { k++; }
        if (ulll x = ulll(6 * k + 1) * (12 * k + 1) * (18 * k + 1); !(x >> 64)) { check(ulng(x), false); }}
#if defined(NDEBUG) && !defined(__AVX2__)
    if (!quick && !full) { // stress, one optimized build: every n below 2^32 against a sieve of the odd numbers (256 MB)
        const ulng lim = ulng(1) << 32; vector<ulng> odd(lim / 128 + 1); // bit i: 2 i + 1 is composite
        for (ulng i = 1; (2 * i + 1) * (2 * i + 1) < lim; i++) {
            if (odd[i >> 6] >> (i & 63) & 1) { continue; }
            for (ulng p = 2 * i + 1, j = (p * p - 1) / 2; j < lim / 2; j += p) { odd[j >> 6] |= ulng(1) << (j & 63); }}
        for (ulng m = 0; m < lim; m++) { check(m, m == 2 || (m > 2 && (m & 1) && !(odd[(m / 2) >> 6] >> ((m / 2) & 63) & 1))); }
        cout << "PASS isPrime against a sieve for every n below 2^32\n";}
#endif
    cout << "PASS isPrime run-time path: table, sieve below " << n << ", windows at 2^31 2^32 2^61 2^62 2^63 2^64, random and adversarial composites\n";}

// The fixed moduli of the Reciprocal sweep, and all products of fixed operands under one of them chained into a word: by the oracle, and by
// Reciprocal::mul, whose C++ path runs only in constant evaluation (the table) and whose assembly only at run time (reciprocals).
constexpr std::array<ulng, 31 + 3 * 63> reciprocalMods() {
    constexpr ulng M61 = Barrett64::MERSENNE61, P = ulng(1) << 32, Q = ulng(1) << 62, H = ulng(1) << 63;
    std::array<ulng, 31 + 3 * 63> mods{1, 2, 3, 5, 7, 10, 998244353, 2147483647, P - 5, P - 1, P, P + 1, P + 15, 1000000000000000000ULL, M61 - 1, M61, M61 + 1, M61 + 2, Q - 57, Q - 1, Q, Q + 1,
                                       H - 25, H - 1, H, H + 1, H + 29, 18446744069414584321ULL, ~ulng(0) - 58, ~ulng(0) - 1, ~ulng(0)};
    int n = 31;
    for (int k = 1; k < 64; k++) { for (ulng d : {0, 1, 2}) { mods[size_t(n++)] = (ulng(1) << k) - 1 + d; } }
    return mods;}
template<bool ORACLE> constexpr ulng reciprocalDigest(ulng m) {
    constexpr ulng P = ulng(1) << 32, H = ulng(1) << 63;
    modint_detail::Reciprocal r(m); ulng h = 0;
    const ulng as[] = {0, 1 % m, 2 % m, m - 1, m / 2, m / 3, 0xAAAAAAAAAAAAAAAAULL % m, ~ulng(0) % m};
    const ulng bs[] = {0, 1, 2, m - 1, m, m + 1, P - 1, P, H - 1, H, ~ulng(0) - 1, ~ulng(0), 0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL, 0xFFFFFFFF00000000ULL};
    for (ulng a : as) { for (ulng b : bs) { h = h * 0x9E3779B97F4A7C15ULL + (ORACLE ? mulOracle(a, b, m) : r.mul(a, b)); } }
    return h;}
template<bool ORACLE> constexpr auto reciprocalDigests() {
    auto mods = reciprocalMods(); std::array<ulng, mods.size()> d{};
    for (size_t i = 0; i < mods.size(); i++) { d[i] = reciprocalDigest<ORACLE>(mods[i]); }
    return d;}
inline constexpr auto RECIPROCAL_DIGESTS = reciprocalDigests<true>();
static_assert(reciprocalDigests<false>() == RECIPROCAL_DIGESTS); // the C++ path
[[gnu::noipa]] inline ulng reciprocalRun(ulng m) { return reciprocalDigest<false>(m); }

[[gnu::cold]] inline void reciprocals() {
    using modint_detail::Reciprocal;
    constexpr ulng P = ulng(1) << 32, H = ulng(1) << 63;
    std::mt19937_64 rng(test_seed ^ 0x707); int count = test_mode == "quick" ? 300 : test_mode == "full" ? 20000 : 200000;
    auto fixed = reciprocalMods(); vector<ulng> mods(fixed.begin(), fixed.end());
    for (size_t i = 0; i < fixed.size(); i++) { probe = {"Reciprocal(mod).mul digest at run time", fixed[i], false, 0, 0}; CHECK(reciprocalRun(fixed[i]), RECIPROCAL_DIGESTS[i]); }
    for (int i = 0; i < count; i++) { ulng m = rng() >> (rng() % 64); mods.push_back(m + !m); }
    for (ulng m : mods) {
        Reciprocal r(m); ulng d = m << std::countl_zero(m); probe = {"Reciprocal(mod).mul(x, y)", m, false, 0, 0};
        CHECK(r.sh, std::countl_zero(m)); CHECK(r.d, d); CHECK(r.v, ~ulll(0) / d - (ulll(1) << 64));
        vector<ulng> as = residues(m, rng, 4), bs{0, 1, 2, m - 1, m, m + 1, P - 1, P, H - 1, H, ~ulng(0) - 1, ~ulng(0), 0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL, 0xFFFFFFFF00000000ULL,
                                                  rng(), rng(), rng(), rng()};
        for (ulng a : as) { for (ulng b : bs) { probe.x = a; probe.y = b; CHECK(r.mul(a, b), mulOracle(a, b, m)); } }}
    cout << "PASS Reciprocal::mul over " << mods.size() << " moduli\n";}

[[gnu::cold]] inline vector<ulng> selectorMods(std::mt19937_64 &rng) {
    bool quick = test_mode == "quick"; vector<ulng> ms;
    for (ulng m = 1; m <= ulng(1) << (quick ? 12 : 17); m++) { ms.push_back(m); }
    for (int k = 2; k < 64; k++) { for (ulng d : {0, 1, 2, 3, 4}) { ms.push_back((ulng(1) << k) - 2 + d); } }
    ms.insert(ms.end(), {167772161, 469762049, 998244353, 1000000007, 2147483659, 2147483693, 4294967291ULL, 4294967311ULL, 1000000000000000000ULL, 1000000000000000009ULL, 4179340454199820289ULL,
                         4611686018427387847ULL, 9223372036854775783ULL, 9223372036854775837ULL, 18446744069414584321ULL, 18446744073709551557ULL, 18446744073709551614ULL, 18446744073709551615ULL});
    for (int i = 0; i < (quick ? 2000 : 200000); i++) { ulng m = rng() >> (rng() % 64); ms.push_back(m + !m); }
    // Order of 2 exactly 65, one past unitShift's window: 31 * 8191 and the other divisors of 2^65 - 1 above 8191.
    ms.insert(ms.end(), {253921, 145295143558111ULL, 4504149450301441ULL, 1190112520884487201ULL});
    return ms;}
[[gnu::cold]] inline void selectors() {
    std::mt19937_64 rng(test_seed ^ 0x808);
    for (ulng m : selectorMods(rng)) {
        probe = {"biasOf(x), shortReciprocal(x), unitShift(x)", m, false, m, 0}; CHECK(modint_detail::biasOf(m), biasOracle(m)); CHECK(modint_detail::unitShift(m), unitOracle(m));
        if (!(m >> 63)) { CHECK(modint_detail::shortReciprocal(m), shortOracle(m, &rng)); }}
    cout << "PASS biasOf, shortReciprocal and unitShift against their definitions\n";}
#endif

#if TEST_DYNAMIC
// Every residue of every small modulus: inverses against a table searched by repeated addition, roots against the table of least roots.
[[gnu::cold]] inline void rings() {
    using M = TEST_TYPE<731>; using W = typename M::Word; bool quick = test_mode == "quick", full = test_mode == "full"; const Ops &o = ops<M, ALL>();
    for (ulng m = 1; m <= (quick ? 150 : full ? 1000 : 2000); m++) {
        if (!usable(m)) { continue; }
        M::setMod(W(m)); vector<ulng> table(m, m);
        for (ulng a = 0; a < m; a++) { for (ulng b = 0, p = 0; b < m && table[a] == m; b++, p = p + a >= m ? p + a - m : p + a) { if (p == 1 % m) { table[a] = b; } } }
        for (ulng a = 0; a < m; a++) {
            ulng out = m - 1; probe = {"inverse of x, every residue", m, false, a, 0};
            CHECK(o.inverse(0, a, out), table[a] != m); CHECK(out, table[a] != m ? table[a] : m - 1);}}
    vector<ulng> primes{12289};
    if (!quick) { primes.insert(primes.end(), {40961, 65537}); }
    for (ulng p = 2; p <= (quick ? 600 : full ? 2000 : 8000); p++) { if (primeOracle(p)) { primes.push_back(p); } }
    for (ulng p : primes) { if (usable(p)) { REQUIRE(primeOracle(p)); M::setMod(W(p)); rootSweep(o, 0, p); } }
    cout << "PASS every inverse of every modulus up to " << (quick ? 150 : full ? 1000 : 2000) << ", every root of " << primes.size() << " small primes\n";}

// The dynamic square-root state: Tonelli-Shanks or Cipolla (2-part s with s^2 above 15 or, in 64-bit words, 9 bit lengths of p) for 256 calls
// on one modulus, tables from the 257th, a fresh count on another modulus. From 2^32 on, eight non-residues in a row put the symbol first.
[[gnu::cold]] inline void rootState() {
    using M = TEST_TYPE<731>; using W = typename M::Word; const Ops &o = ops<M, ALL>(); std::mt19937_64 rng(test_seed ^ 0x909);
    vector<ulng> primes{12289, 65537, 998244353, 3221225473ULL};
#if TEST_WIDE
    primes.insert(primes.end(), {1099511628329ULL, 2305843009255636993ULL, 2305843011210182657ULL, 9223373136366406321ULL, 18446744069414584321ULL});
#endif
    auto residue = [](ulng a, ulng p) { return powOracle(a, (p - 1) / 2, p) == 1; };
    auto draw = [&](ulng p) { ulng a = rng() % (p - 2) + 2; while (residue(a, p)) { a = rng() % (p - 2) + 2; } return a; }; // a non-residue
    auto base = [&](ulng p) { return rng() % (p - 3) + 2; }; // in [2, p - 2]: its square is not 1, which sqrt answers before the state
    M::sqrtRun = 0;
    for (ulng p : primes) {
        M::setMod(17); rootCase(o, 2, true, 6); M::setMod(W(p)); // 17 = 2^4 + 1 takes the state from the prime before
        int s = std::countr_zero(p - 1); bool counted = sizeof(W) == 8 && p >> 32; probe = {"square-root state of the modulus, x = calls so far", p, false, 0, 0};
        for (int i = 1; i <= 140; i++) { // two calls a case: Tonelli-Shanks or Cipolla to the 128th case, the tables after it
            ulng x = base(p), a = i % 3 ? mulOracle(x, x, p) : draw(p);
            if (i % 3) { rootCase(o, a, true, std::min(x, p - x)); } else { rootCase(o, a, false); }
            probe.x = ulll(i); CHECK(M::sqrtCache.mod, p); CHECK(M::sqrtCache.calls, std::min(2 * i, 257)); CHECK(M::sqrtCache.wide, s * s > (sizeof(W) == 4 ? 15 : 9) * std::bit_width(p));
            CHECK(M::sqrtRun, counted && i % 3 == 0 ? 2 : 0);}
        for (int stage = 0; stage < 2; stage++) { // a run of non-residues on the tables, then on the first calls of a fresh state
            for (int i = 1; i <= 12; i++) { rootCase(o, draw(p), false); CHECK(M::sqrtRun, counted ? std::min(2 * i, 8) : 0); }
            if (!stage) { CHECK(M::sqrtCache.calls, 257); M::setMod(17); rootCase(o, 2, true, 6); M::setMod(W(p)); }
            else { CHECK(M::sqrtCache.calls, counted ? 10 : 26); } // two calls of the root below, then the run: from its eighth call the symbol answers before the state is touched
            ulng x = base(p); rootCase(o, mulOracle(x, x, p), true, std::min(x, p - x)); CHECK(M::sqrtRun, 0);}}
    cout << "PASS square-root state: " << primes.size() << " moduli across the 257th call, a modulus change and a run of non-residues\n";}
#endif

#if TEST_FORM && !TEST_DYNAMIC
// rootFixed alone: its callers' products would reduce a word it left at or above the modulus.
template<ulng P, ulng E> void fixedRoot() {
    using M = TEST_TYPE<P>; std::mt19937_64 rng(test_seed ^ P ^ E);
    for (int i = 0; i < 300; i++) { ulng a = rng() % P; probe = {"rootFixed<y>(x)", P, false, a, E}; CHECK(stored(M::template rootFixed<E>(M(a))), powOracle(a, E, P)); }}
#endif

// Static: one modulus per class of code path, with the parts that class needs (each thunk table costs compile time).
// Dynamic: one setMod sequence of very different moduli, several revisited, so a stale context field shows; then the small rings.
#if TEST_DYNAMIC
// The inverses below INV_TINY of a dynamic modulus, which its quotient by the value gives: moduli of every bit length, multiples of a value and
// their neighbours, the top of the word; the second call reads the table.
[[gnu::cold]] inline void quotients() {
    using M = TEST_TYPE<731>; using W = typename M::Word; std::mt19937_64 rng(test_seed ^ 0x808); int bits = 8 * int(sizeof(W));
    int count = test_mode == "quick" ? 2000 : test_mode == "full" ? 20000 : 200000;
    for (int i = 0; i < count; i++) {
        ulng s = rng(), a = 1 + s % 15, m = (rng() | ulng(1) << 63) >> (64 - (2 + int((s >> 8) % ulng(bits - 1))));
        if (i % 4 == 1) { m = m / a * a + (s >> 16) % 3 - 1; }
        if (i % 16 == 3) { m = ulng(W(0 - 1 - (s >> 20) % 64)); }
        m = std::max<ulng>(ulng(W(m)), 2) | TEST_FORM;
        M::setMod(W(m)); probe = {"inverse of x by the quotient of the modulus", m, false, 0, 0};
        for (ulng v = 1; v < ulng(modint_detail::INV_TINY) && v < m; v++) {
            ulng want = 0; bool unit = invOracle(v, m, want); M x = M::raw(W(v)), y = x, z = x; probe.x = v;
            CHECK(tryInv(x, y), unit); CHECK(y.val(), unit ? want : v); CHECK(tryInv(x, z), unit); CHECK(z.val(), unit ? want : v);}}
    cout << "PASS " << count << " moduli, the inverses below " << modint_detail::INV_TINY << " by quotient\n";}
#endif
[[gnu::cold]] inline void sweeps() {
#if TEST_DYNAMIC
    using M = TEST_TYPE<731>;
    vector<ulng> sequence{1, 2, 2147483647, 4294967291ULL, 3, 998244353, 4294967295ULL, 2, 2147483648, 469762049, 1, 1000000007, 2147483659, 7, 32768, 2147483693, 128, 167772161, 12, 65537, 4, 998244353,
                          3221225473ULL, 12289, 2039, 4294967294ULL, 1000000, 4000000000ULL, 3221225472ULL, 2047, 2046}; // 3 * 2^30: the swapped loop divides first
#if TEST_WIDE
    sequence.insert(sequence.end(), {2305843009213693951ULL, 9223372036854775783ULL, 7, 9223372036854775837ULL, 2305843009213693951ULL, 18446744073709551557ULL, 18446744073709551615ULL,
                                     4611686018427387904ULL, 2, 1000000000000000000ULL, 2305843009213693951ULL, 2305843009213693953ULL, 1, 18446744073709551614ULL, 4294967296ULL,
                                     4179340454199820289ULL, 4611686018427387847ULL, 4611686018427387903ULL, 4611686018427387905ULL, 9223372036854775808ULL, 18446744069414584321ULL,
                                     4294967311ULL, 2305843009213693951ULL, 998244353, 1099511628329ULL, 2305843009255636993ULL, 2305843011210182657ULL, 9223373136366406321ULL, 8589934817ULL,
                                     13835058055282163712ULL, 12297829382473034410ULL, 6917529027641081856ULL, 18446744073709551612ULL}); // even: 3 * 2^62, 0xAA..AA, 3 * 2^61, 2^64 - 4
#endif
    for (ulng m : sequence) { if (usable(m)) { M::setMod(decltype(M::mod())(m)); paths<M, ALL>(); } }
    rings(); quotients(); rootState();
#elif TEST_FORM
    // The lazy product's range below 2^62 and its end, both sides of 2^63, the fold moduli of the canonical type, composites, and tiny moduli.
    paths<TEST_TYPE<998244353>, ALL>(); paths<TEST_TYPE<4294967311ULL>, ALL>(); paths<TEST_TYPE<2305843009213693951ULL>, ALL>(); paths<TEST_TYPE<4611686018427387847ULL>, POWER | ROOT>();
    paths<TEST_TYPE<9223372036854775783ULL>, POWER | ROOT>(); paths<TEST_TYPE<9223372036854775837ULL>, ALL>(); paths<TEST_TYPE<18446744069414584321ULL>, POWER | ROOT>();
    paths<TEST_TYPE<18446744073709551557ULL>, ALL>(); paths<TEST_TYPE<18446744073709551615ULL>, POWER | READ>(); paths<TEST_TYPE<4179340454199820289ULL>, ROOT>();
    paths<TEST_TYPE<1>, POWER>(); paths<TEST_TYPE<3>, POWER | ROOT>(); paths<TEST_TYPE<7>, ALL>(); paths<TEST_TYPE<999999999999999999ULL>, POWER>(); paths<TEST_TYPE<2047>, 0>();
    // Runs of ones on top of the exponent: alone, above zeros, above a mixed tail; lazy products below 2^62, exact ones above.
    fixedRoot<4611686018427387847ULL, 4611686018427387847ULL / 4 + 1>(); fixedRoot<998244353, (ulng(1) << 40) - 1>(); fixedRoot<2305843009213693951ULL, ((ulng(1) << 20) - 1) << 9>();
    fixedRoot<4294967311ULL, ((ulng(1) << 17) - 1) << 7 | 0x53>(); fixedRoot<18446744073709551557ULL, 18446744073709551557ULL / 4 + 1>(); fixedRoot<7, 0xFFFF>();
#elif TEST_WIDE
    primality(); reciprocals(); selectors();
    paths<TEST_TYPE<998244353>, POWER>(); paths<TEST_TYPE<469762049>, POWER | ROOT>(); paths<TEST_TYPE<4294967291>, POWER | ROOT>();
    paths<TEST_TYPE<4294967311ULL>, ALL>(); paths<TEST_TYPE<1000000000000000000ULL>, POWER | READ>(); paths<ModInt61, ALL>();
    paths<TEST_TYPE<4179340454199820289ULL>, ROOT>(); paths<TEST_TYPE<9223372036854775783ULL>, POWER | ROOT>(); paths<TEST_TYPE<18446744073709551557ULL>, ALL>();
    // A long reciprocal in [2^31, 2^32), a power of two, the fold moduli 2^64 - 2^32 + 1 and 2^62 - 57, the top bit set.
    paths<TEST_TYPE<2147751449>, POWER | ROOT>(); paths<TEST_TYPE<2147483648>, POWER>(); paths<TEST_TYPE<18446744069414584321ULL>, ALL>();
    paths<TEST_TYPE<4611686018427387847ULL>, POWER | READ>(); paths<TEST_TYPE<9223372036854775837ULL>, POWER>();
    // Even moduli: below 2^32, the top of the word, 3 * 2^62, and the power of two 2^63.
    paths<TEST_TYPE<4000000000ULL>, 0>(); paths<TEST_TYPE<18446744073709551614ULL>, 0>(); paths<TEST_TYPE<13835058055282163712ULL>, 0>(); paths<TEST_TYPE<9223372036854775808ULL>, 0>();
    // Both kinds of inverse table on a 64-bit word: primes below INV_TABLE (2, 13, 2039), a composite below it, a prime above it.
    paths<TEST_TYPE<2>, 0>(); paths<TEST_TYPE<13>, 0>(); paths<TEST_TYPE<2039>, 0>(); paths<TEST_TYPE<2047>, 0>(); paths<TEST_TYPE<2053>, 0>();
    lifts<1, 5, 6, 10, 11, 20, 21, 31, 32, 40, 41, 62, 63>();
#else
    paths<TEST_TYPE<1>, POWER>(); paths<TEST_TYPE<2>, POWER | ROOT>(); paths<TEST_TYPE<3>, ROOT>(); paths<TEST_TYPE<7>, 0>(); paths<TEST_TYPE<128>, POWER>(); paths<TEST_TYPE<2147483648>, POWER>();
    paths<mint, ALL>(); paths<TEST_TYPE<469762049>, ROOT>(); paths<TEST_TYPE<2147483693>, ROOT>(); paths<TEST_TYPE<4294967291>, ALL>();
    // Root tables of one window (2-part 2^8), of two (2^9, 2^16) and of four (2^30, top bit set); both sides of the inverse table's bound 2048.
    paths<TEST_TYPE<257>, ROOT>(); paths<TEST_TYPE<7681>, ROOT>(); paths<TEST_TYPE<65537>, ROOT>(); paths<TEST_TYPE<3221225473>, POWER | ROOT>();
    paths<TEST_TYPE<2039>, 0>(); paths<TEST_TYPE<2053>, 0>(); paths<TEST_TYPE<1000000>, 0>(); paths<TEST_TYPE<4294967294U>, 0>(); // the last two: even, not a power of two
    paths<TEST_TYPE<2046>, 0>(); paths<TEST_TYPE<2047>, 0>(); paths<TEST_TYPE<3221225472U>, 0>(); // composites below INV_TABLE; 3 * 2^30: the swapped loop divides first
    lifts<1, 2, 5, 6, 10, 11, 20, 21, 30, 31>();
#endif
    cout << "PASS path sweeps\n";}

#if TEST_WIDE && !TEST_DYNAMIC
// Feitsma and Galway's list of the base-2 Fermat pseudoprimes below 2^64 on stdin, one a line. Every composite that passes the strong
// test to base 2 is in it, so isPrime is exact from 2^32 iff it rejects each entry; an independent strong test on plain 128-bit
// remainders counts the entries that only the Lucas half stops. Prints: total, strong, accepted, entries out of order, last.
inline int pseudoprimes() {
    auto mulmod = [](ulng a, ulng b, ulng m) { return ulng(ulll(a) * b % m); };
    auto strong = [&](ulng m) {
        ulng d = m - 1; int s = std::countr_zero(d); d >>= s;
        ulng x = 1, b = 2 % m;
        for (ulng e = d; e; e >>= 1, b = mulmod(b, b, m)) { if (e & 1) { x = mulmod(x, b, m); } }
        if (x == 1 || x == m - 1) { return true; }
        for (int i = 1; i < s; i++) { x = mulmod(x, x, m); if (x == m - 1) { return true; } }
        return false;};
    ulng total = 0, passed = 0, accepted = 0, order = 0, prev = 0; char line[64];
    while (fgets(line, sizeof line, stdin)) {
        ulng m = strtoull(line, nullptr, 10); total++; order += m <= prev; prev = m;
        passed += strong(m); accepted += modint_detail::isPrime(m);}
    cout << total << ' ' << passed << ' ' << accepted << ' ' << order << ' ' << prev << '\n'; return 0;}
#endif

inline void protocol() {
    char op; ulng m;
    while (cin >> op >> m) {
        dispatch(m, [&]<class M>() {
            if (op == 'N') {
                char sign; string s; cin >> sign >> s; M a, b;
                if (sign == 's') {
                    lll x = parseSigned(s); a = M(x);
                    if (x >= INT_MIN && x <= INT_MAX) { b = int(x); }
                    else if (x >= LLONG_MIN && x <= LLONG_MAX) { b = lng(x); }
                    else { b = x; }}
                else {
                    ulll x = parseWide(s); a = M(x);
                    if (x <= UINT_MAX) { b = uint(x); }
                    else if (x <= ULLONG_MAX) { b = ulng(x); }
                    else { b = x; }}
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
            else { fail("protocol operation", __LINE__); }});}}

inline constexpr ulng RING = TEST_FORM ? 15 : 4, ZERO_DIVISOR = TEST_FORM ? 3 : 2; // a composite modulus the type takes and a non-unit of it
inline int death(const string &what) {
    if (what == "raw") { dispatch(17, []<class M>() { (void)M::raw(17); }); }
    else if (what == "inverse") { dispatch(RING, []<class M>() { (void)inv(M(ZERO_DIVISOR)); }); }
    else if (what == "division") { dispatch(RING, []<class M>() { (void)(M(1) / M(ZERO_DIVISOR)); }); }
    else if (what == "negative-power") { dispatch(RING, []<class M>() { (void)pow(M(ZERO_DIVISOR), -1); }); }
    else if (what == "root") { dispatch(RING, []<class M>() { M out; (void)trySqrt(M(1), out); }); }
    else if (what == "legacy-root") { dispatch(RING, []<class M>() { (void)sqrt(M(1)); }); }
    else if (what == "narrow") { dispatch(4294967295ULL, []<class M>() { (void)int(M(2147483648ULL)); }); }
#if TEST_DYNAMIC
    else if (what == "zero-modulus") { TEST_TYPE<9>::setMod(0); }
    else if (what == "prime-flag") { TEST_TYPE<9>::setMod(17, 2); }
    else if (what == "unknown-root") { using M = TEST_TYPE<9>; M::setMod(17, 0); M out; (void)trySqrt(M(1), out); }
#if TEST_FORM
    else if (what == "even-modulus") { TEST_TYPE<9>::setMod(12); }
#endif
#endif
    else { fail("unknown death case", __LINE__); }
    cerr << "precondition did not abort: " << what << '\n'; return 2;}

int main(int argc, char **argv) {
    if (argc > 1 && string(argv[1]) == "--oracle") { protocol(); return 0; }
    if (argc > 2 && string(argv[1]) == "--death") { return death(argv[2]); }
#if TEST_WIDE && !TEST_DYNAMIC
    if (argc > 1 && string(argv[1]) == "--pseudoprimes") { return pseudoprimes(); }
#endif
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        if (a == "--seed" && i + 1 < argc) { test_seed = std::stoull(argv[++i]); }
        else if (a == "--mode" && i + 1 < argc) { test_mode = argv[++i]; }
        else { fail("valid option", __LINE__, a); }}
    compileTime(); contexts();
#if TEST_DYNAMIC
    EQUAL(TEST_TYPE<731>::mod(), 998244353); paths<TEST_TYPE<731>, ALL>(); // the default context: nothing has set this ID's modulus yet
#endif
    vector<ulng> mods{1, 2, 4, 7, 17, 97, 998244353, 2147483647, 2147483648, 4294967291ULL, 4294967295ULL};
#if TEST_WIDE
    mods.insert(mods.end(), {4294967296ULL, 2305843009213693951ULL, 9223372036854775808ULL, 18446744069414584321ULL, 18446744073709551557ULL, 18446744073709551615ULL});
#endif
    for (ulng m : mods) { if (usable(m)) { dispatch(m, []<class M>() { verifyMod<M>(); }); } }
#if TEST_DYNAMIC
    for (int m = 3; m <= (test_mode == "quick" ? 15 : test_mode == "full" ? 60 : 120); m++) {
        if (!usable(ulng(m))) { continue; }
        using M = TEST_TYPE<731>; M::setMod(m);
        for (int a = 0; a < m; a++) { for (int b = 0; b < m; b++) { pairCase<M>(a, b); } }
        roots<M>();}
#endif
    sweeps();
    cout << "PASS all C++ features seed=" << test_seed << " mode=" << test_mode << '\n';}
