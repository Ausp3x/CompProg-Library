#include "../../01-Core/05-modint.hpp"
#include "../../01-Core/06-modintmini.hpp"

// Checks intentionally survive -DNDEBUG; Python supplies the independent oracle.
[[noreturn]] void fail(string_view message) {
    cerr << "FAIL " << message << '\n'; std::exit(1); }
void check(bool okay, string_view message) { if (!okay) { fail(message); } }

ulll readUnsigned(const string &s) {
    ulll x = 0;
    for (char c : s) { x = 10 * x + (c - '0'); }
    return x; }
lll readSigned(const string &s) {
    if (s[0] != '-') { return lll(readUnsigned(s)); }
    ulll x = readUnsigned(s.substr(1));
    return x ? -lll(x - 1) - 1 : 0; }

template<typename T, typename F>
void oracle(char operation) {
    if (operation == 'N') {
        char sign; string s; cin >> sign >> s;
        T a, b; F f;
        if (sign == 's') { lll x = readSigned(s); a = T(x); b = x; f = x; }
        else { ulll x = readUnsigned(s); a = T(x); b = x; f = x; }
        check(a.val() == f.val() && b == a, "normalization/full agreement and assignment");
        cout << a.val() << ' ' << b.val() << '\n'; return; }
    if (operation == 'P') {
        ulng a; char sign; string s; cin >> a >> sign >> s;
        T r; F f;
        if (sign == 's') { lll e = readSigned(s); r = pow(T(a), e); f = pow(F(a), e); }
        else { ulll e = readUnsigned(s); r = pow(T(a), e); f = pow(F(a), e); }
        check(r.val() == f.val(), "power/full agreement");
        cout << r.val() << '\n'; return; }
    if (operation != 'A') { fail("unknown oracle operation"); }
    ulng av, bv; cin >> av >> bv;
    T a = T::raw(typename T::Word(av)), b = T::raw(typename T::Word(bv));
    F af(av), bf(bv);
    T sum = a + b, diff = a - b, product = a * b, negative = -a;
    check(sum.val() == (af + bf).val() && diff.val() == (af - bf).val() &&
          product.val() == (af * bf).val() && negative.val() == (-af).val(), "arithmetic/full agreement");
    T x = a; check(&(x += b) == &x && x == sum, "compound addition/reference");
    x = a; check(&(x -= b) == &x && x == diff, "compound subtraction/reference");
    x = a; check(&(x *= b) == &x && x == product, "compound multiplication/reference");
    x = a; x += x; check(x == a + a, "self addition");
    x = a; x -= x; check(x == T(0), "self subtraction");
    x = a; x *= x; check(x == a * a, "self multiplication");
    check(+a == a && (a != b) == !(a == b), "unary plus/equality/inequality");
    T copy = a, moved = std::move(copy); check(moved == a, "copy/move value");
    x = a; x = x; check(x == a, "self assignment");

    T inverse = T::raw(T::mod() - 1), alias = b, quotient = inverse;
    F inverse_full = F::raw(F::mod() - 1);
    bool unit = tryInv(b, inverse), unit_full = tryInv(bf, inverse_full);
    check(unit == unit_full && inverse.val() == inverse_full.val(), "inverse/full agreement");
    bool aliased = tryInv(alias, alias);
    check(aliased == unit && alias == (unit ? inverse : b), "inverse alias/failure preservation");
    if (unit) {
        check(inv(b) == inverse && b * inverse == T(1), "unit inverse");
        quotient = a / b; x = a;
        check(&(x /= b) == &x && x == quotient && quotient.val() == (af / bf).val(), "division/reference");
        x = b; x /= x; check(x == T(1), "self division"); }
    cout << sum.val() << ' ' << diff.val() << ' ' << product.val() << ' ' << (a == b) << ' '
         << negative.val() << ' ' << unit << ' ' << inverse.val() << ' ' << quotient.val() << '\n'; }

template<bool WIDE>
void staticOracle(ulng m, char operation) {
#define STATIC_CASE(M) case M: if constexpr (WIDE) { oracle<ModInt64Mini<M>, ModInt64<M>>(operation); } \
    else { oracle<ModIntMini<M>, ModInt<M>>(operation); } break
#define WIDE_CASE(M) case M: oracle<ModInt64Mini<M>, ModInt64<M>>(operation); break
    switch (m) {
        STATIC_CASE(1); STATIC_CASE(2); STATIC_CASE(3); STATIC_CASE(4);
        STATIC_CASE(5); STATIC_CASE(6); STATIC_CASE(7); STATIC_CASE(8);
        STATIC_CASE(9); STATIC_CASE(10); STATIC_CASE(11); STATIC_CASE(12);
        STATIC_CASE(13); STATIC_CASE(14); STATIC_CASE(15); STATIC_CASE(16);
        STATIC_CASE(17); STATIC_CASE(97); STATIC_CASE(998244353);
        STATIC_CASE(2147483647); STATIC_CASE(2147483648);
        STATIC_CASE(4294967291); STATIC_CASE(4294967295);
        default:
            if constexpr (WIDE) {
                switch (m) {
                    WIDE_CASE(4294967296ULL); WIDE_CASE(2305843009213693951ULL);
                    WIDE_CASE(9223372036854775807ULL); WIDE_CASE(9223372036854775808ULL);
                    WIDE_CASE(18446744069414584321ULL); WIDE_CASE(18446744073709551557ULL);
                    WIDE_CASE(18446744073709551615ULL);
                    default: fail("unsupported static wide oracle modulus"); } }
            else { fail("unsupported static oracle modulus"); } }
#undef STATIC_CASE
#undef WIDE_CASE
}

template<typename T>
constexpr bool constantFixture() {
    T a(-2), out(5);
    if (a.val() != 15 || T::mod() != 17 || T::raw(3).val() != 3) { return false; }
    if ((a + 4).val() != 2 || (a - 4).val() != 11 || (a * 4).val() != 9) { return false; }
    if (pow(T(3), -3).val() != 12 || inv(T(3)).val() != 6) { return false; }
    if (!tryInv(T(3), out) || out.val() != 6 || (a / 3).val() != 5) { return false; }
    a = -1; a += 3; a *= 4; a -= 1; a /= 7;
    return a == T(1) && -a == T(16) && +a == a && a != T(0); }
static_assert(constantFixture<ModIntMini<17>>());
static_assert(constantFixture<ModInt64Mini<17>>());
static_assert(std::is_same_v<mintmini, ModIntMini<998244353>>);
static_assert(ModIntMini<4294967295U>(-1).val() == 4294967294U);
static_assert((ModInt64Mini<18446744073709551615ULL>(-1) *
               ModInt64Mini<18446744073709551615ULL>(-1)).val() == 1);
static_assert(inv(ModIntMini<1>(0)).val() == 0);
static_assert(pow(ModInt64Mini<1>(0), 0).val() == 0);

template<typename T, typename F, typename W>
constexpr bool typeContract() {
    return std::is_same_v<typename T::Word, W> && sizeof(T) == sizeof(W) &&
           !std::is_constructible_v<T, double> && !std::is_constructible_v<T, F> &&
           !std::is_constructible_v<F, T> && !std::is_convertible_v<T, W> &&
           !requires(T a) { pow(a, 1.5); } && !requires(T a) { int(a); } &&
           !requires(T a) { bool(a); } && !requires(T a) { !a; } &&
           !requires(T a) { a < a; } && !requires(T a) { ++a; } &&
           !requires(T a) { a++; } && !requires(T a) { --a; } &&
           !requires(T a) { a--; } && !requires { T::init(W(0)); } &&
           !requires { T::isPrime(); } && !requires { T::is_prime; } &&
           !requires(T a) { sqrt(a); } && !requires(T a) { trySqrt(a, a); } &&
           !requires(vector<T> a) { batchInv(a, a); } &&
           !requires(T a, istream &in) { in >> a; } &&
           !requires(T a, ostream &out) { out << a; }; }
static_assert(typeContract<ModIntMini<17>, ModInt<17>, uint>());
static_assert(typeContract<ModInt64Mini<17>, ModInt64<17>, ulng>());
static_assert(typeContract<DynModIntMini<71>, DynModInt<71>, uint>());
static_assert(typeContract<DynModInt64Mini<71>, DynModInt64<71>, ulng>());
static_assert(!std::is_constructible_v<ModIntMini<17>, ModIntMini<19>>);
static_assert(!std::is_constructible_v<ModInt64Mini<17>, ModIntMini<17>>);
static_assert(!std::is_constructible_v<DynModIntMini<71>, DynModIntMini<72>>);
static_assert(!std::is_constructible_v<DynModInt64Mini<71>, DynModIntMini<71>>);

template<typename T>
void nativeFixture() {
    check(T().val() == 0 && T::mod() == 17, "default construction/modulus");
    check(T(false).val() == 0 && T(true).val() == 1, "bool construction");
    check(T(int8_t(-128)).val() == 8 && T(int16_t(-32768)).val() == 8, "small signed minima");
    check(T(std::numeric_limits<int>::min()).val() == 8, "int signed minimum");
    check(T(std::numeric_limits<lng>::min()).val() == 8, "lng signed minimum");
    check(T(-lll(ulll(1) << 126) - lll(ulll(1) << 126)).val() == 8, "lll signed minimum");
    check(T(std::numeric_limits<uint>::max()).val() == 0 &&
          T(std::numeric_limits<ulng>::max()).val() == 0 && T(~ulll(0)).val() == 0, "unsigned maxima");
    check(pow(T(3), false).val() == 1 && pow(T(3), true).val() == 3, "bool exponents");
    check(pow(T(3), int8_t(-3)).val() == 12 && pow(T(3), lng(-3)).val() == 12, "native signed exponents");
    check(pow(T(3), uint(3)).val() == 10 && pow(T(3), ulng(3)).val() == 10, "native unsigned exponents");
    T a = -1; check(a.val() == 16, "implicit construction");
    a = std::numeric_limits<lng>::min(); check(a.val() == 8, "implicit assignment");
    check((2 + a).val() == 10 && (2 - a).val() == 11 && (2 * a).val() == 16 &&
          (2 / a).val() == 13, "left integer operands");
    check((a + 2).val() == 10 && (a - 2).val() == 6 && (a * 2).val() == 16 &&
          (a / 2).val() == 4, "right integer operands"); }

void fixtures() {
    check(DynModIntMini<70>::mod() == 998244353 && DynModInt64Mini<70>::mod() == 998244353,
          "dynamic default modulus");
    DynModIntMini<71>::setMod(17); DynModInt64Mini<71>::setMod(17);
    nativeFixture<ModIntMini<17>>(); nativeFixture<ModInt64Mini<17>>();
    nativeFixture<DynModIntMini<71>>(); nativeFixture<DynModInt64Mini<71>>();
    DynModIntMini<72>::setMod(12); DynModInt64Mini<72>::setMod(19);
    DynModInt<72>::setMod(23); DynModInt64<72>::setMod(29);
    check(DynModIntMini<72>::mod() == 12 && DynModInt64Mini<72>::mod() == 19 &&
          DynModIntMini<71>::mod() == 17 && DynModInt64Mini<71>::mod() == 17 &&
          DynModInt<72>::mod() == 23 && DynModInt64<72>::mod() == 29, "ID/width/full-mini separation");
    using A = DynModIntMini<73>; using B = DynModInt64Mini<73>;
    A old_a(13); B old_b(13);
    for (ulng m : {1ULL, 17ULL, 17ULL, 4294967295ULL, 2ULL, 998244353ULL}) {
        A::setMod(uint(m)); B::setMod(m);
        // Old values are invalid after every reset: overwrite before inspecting them.
        old_a = -1; old_b = -1;
        check(old_a.val() == m - 1 && old_b.val() == m - 1, "reset/overwrite");
        check((old_a * old_a).val() == 1 % m && (old_b * old_b).val() == 1 % m,
              "reset/reduction refresh"); }
    cout << "PASS native constructors/exponents, constexpr, IDs, widths, reset and mixed inclusion\n"; }

template<typename T>
void death(const string &name) {
    if (name == "raw") { (void)T::raw(T::mod()); }
    else if (name == "inverse") { (void)inv(T(6)); }
    else if (name == "division") { T x(1); x /= T(6); }
    else if (name == "negative-power") { (void)pow(T(6), -1); }
    else { fail("unknown precondition fixture"); } }

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false); cin.tie(nullptr);
    if (argc == 2 && string(argv[1]) == "--oracle") {
        int kind; char operation; ulng m;
        while (cin >> kind >> operation >> m) {
            if (kind == 0) { staticOracle<false>(m, operation); }
            else if (kind == 1) { staticOracle<true>(m, operation); }
            else if (kind == 2) {
                DynModIntMini<74>::setMod(uint(m)); DynModInt<74>::setMod(uint(m), 0);
                oracle<DynModIntMini<74>, DynModInt<74>>(operation); }
            else if (kind == 3) {
                DynModInt64Mini<74>::setMod(m); DynModInt64<74>::setMod(m, 0);
                oracle<DynModInt64Mini<74>, DynModInt64<74>>(operation); }
            else { fail("unknown oracle type"); } }
        return cin.eof() ? 0 : 2; }
    if (argc == 4 && string(argv[1]) == "--death") {
        int kind = std::stoi(argv[2]); string name(argv[3]);
        if (name == "zero-modulus") {
            if (kind == 2) { DynModIntMini<75>::setMod(0); }
            else if (kind == 3) { DynModInt64Mini<75>::setMod(0); }
            else { fail("zero-modulus runtime fixture is dynamic only"); } }
        else if (kind == 0) { death<ModIntMini<12>>(name); }
        else if (kind == 1) { death<ModInt64Mini<12>>(name); }
        else if (kind == 2) { DynModIntMini<75>::setMod(12); death<DynModIntMini<75>>(name); }
        else if (kind == 3) { DynModInt64Mini<75>::setMod(12); death<DynModInt64Mini<75>>(name); }
        else { fail("unknown precondition type"); }
        fail("precondition unexpectedly returned"); }
    fixtures(); }
