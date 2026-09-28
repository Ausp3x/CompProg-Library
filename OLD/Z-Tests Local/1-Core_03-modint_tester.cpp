#include "../1-Core/03-modint.hpp"

constexpr const char *GREEN = "\033[1;32m";
constexpr const char *RED = "\033[1;31m";
constexpr const char *RESET = "\033[0m";
string context;

[[noreturn]] void fail(const string &expr, int line) {
    cerr << RED << "FAIL: 1-Core_03-modint_tester.cpp:" << line << ": " << expr;
    if (!context.empty()) { cerr << " [" << context << ']'; }
    cerr << RESET << '\n'; std::abort();
}
#define REQUIRE(x) do { if (!(x)) fail(#x, __LINE__); } while (0)

template<int P> int norm(lll x) { x %= P; return int(x + (x < 0) * P); }
template<int P> int norm(ulll x) { return int(x % P); }

template<class M> M binaryPow(M a, lng e) {
    ulng u = e < 0 ? -ulng(e) : ulng(e);
    if (e < 0) { a = inv(a); }
    M r = 1;
    while (u) { if (u & 1) { r *= a; } a *= a; u >>= 1; }
    return r;
}

template<int P> void exhaustiveRing() {
    using M = ModInt<P>;
    context = "mod=" + std::to_string(P);
    for (int a = 0; a < P; a++) for (int b = 0; b < P; b++) {
        M x = a, y = b;
        REQUIRE(int(x + y) == (a + b) % P);
        REQUIRE(int(x - y) == (a - b + P) % P);
        REQUIRE(int(x * y) == int(lng(a) * b % P));
        M z = x; REQUIRE(&(z += y) == &z); REQUIRE(int(z) == (a + b) % P);
        z = x; REQUIRE(&(z -= y) == &z); REQUIRE(int(z) == (a - b + P) % P);
        z = x; REQUIRE(&(z *= y) == &z); REQUIRE(int(z) == int(lng(a) * b % P));
        z = x; M old = z++; REQUIRE(int(old) == a && int(z) == (a + 1) % P);
        z = x; old = z--; REQUIRE(int(old) == a && int(z) == (a + P - 1) % P);
        z = x; REQUIRE(int(++z) == (a + 1) % P);
        z = x; REQUIRE(int(--z) == (a + P - 1) % P);
        REQUIRE(int(+x) == a && int(-x) == (a ? P - a : 0));
        REQUIRE(bool(x) == (a != 0) && (!x) == (a == 0));
        REQUIRE((x == y) == (a == b)); REQUIRE((x < y) == (a < b));
        REQUIRE((x <= y) == (a <= b)); REQUIRE((x > y) == (a > b));
        REQUIRE((x >= y) == (a >= b));
        if (b && std::gcd(b, P) == 1) {
            M iy = inv(y); REQUIRE(int(y * iy) == 1 % P);
            z = x; REQUIRE(&(z /= y) == &z); REQUIRE(z == x * iy); REQUIRE(x / y == z);
        }
    }
    context.clear();
}

template<int P> void exhaustiveSqrt() {
    using M = ModInt<P>;
    static_assert(M::is_prime);
    context = "sqrt mod=" + std::to_string(P);
    for (int a = 0; a < P; a++) {
        int want = -1;
        for (int x = 0; x < P; x++) if (lng(x) * x % P == a) { want = want < 0 ? x : min(want, x); }
        M r = sqrt(M(a));
        if (want < 0) { REQUIRE(int(r) == P - 1); }
        else { REQUIRE(int(r * r) == a); REQUIRE(int(r) <= P - int(r)); }
    }
    context.clear();
}

void testCompileTimeAndLayout() {
    static_assert(!ModInt<1>::is_prime && ModInt<2>::is_prime && ModInt<3>::is_prime && ModInt<7>::is_prime && ModInt<61>::is_prime);
    static_assert(!ModInt<4>::is_prime && ModInt<97>::is_prime && !ModInt<561>::is_prime);
    static_assert(!ModInt<2'047>::is_prime && !ModInt<1'373'653>::is_prime && !ModInt<25'326'001>::is_prime);
    static_assert(ModInt<998'244'353>::is_prime && ModInt<INT_MAX>::is_prime);
    static_assert(ModInt<97>::mod() == 97);
    static_assert(sizeof(ModInt<7>) == sizeof(int));
    static_assert(std::is_trivially_copyable_v<ModInt<7>> && std::is_standard_layout_v<ModInt<7>>);
    static_assert(int(ModInt<7>(-1)) == 6);
    static_assert(int(ModInt<7>(3) + ModInt<7>(5)) == 1);
    static_assert(int(ModInt<7>(3) * ModInt<7>(5)) == 1);
    static_assert(int(pow(ModInt<7>(3), -2)) == 4);
    static_assert(int(sqrt(ModInt<7>(2))) == 3);
    static_assert(int(ModInt<7>::init(6)) == 6);
    static_assert(std::is_same_v<mint, ModInt<998'244'353>>);
}

void testConstructionAssignmentAndIO() {
    using M = ModInt<97>;
    for (lng x : {LLONG_MIN, -10'000'000'000LL, -98LL, -97LL, -1LL, 0LL, 1LL, 97LL, 98LL, 10'000'000'000LL, LLONG_MAX}) {
        context = "signed=" + std::to_string(x); REQUIRE(int(M(x)) == norm<97>(lll(x)));
        M y; REQUIRE(&(y = x) == &y); REQUIRE(int(y) == norm<97>(lll(x)));
    }
    for (ulng x : {0ULL, 1ULL, 96ULL, 97ULL, 98ULL, 10'000'000'000ULL, ULLONG_MAX}) {
        context = "unsigned=" + std::to_string(x); REQUIRE(int(M(x)) == norm<97>(ulll(x)));
        M y; REQUIRE(&(y = x) == &y); REQUIRE(int(y) == norm<97>(ulll(x)));
    }
    lll lo = lll(ulll(1) << 127), hi = (lll(1) << 126) - 1; hi += (lll(1) << 126);
    ulll umax = ~ulll(0);
    REQUIRE(int(M(lo)) == norm<97>(lo)); REQUIRE(int(M(hi)) == norm<97>(hi)); REQUIRE(int(M(umax)) == norm<97>(umax));
    M assigned; assigned = lo; REQUIRE(int(assigned) == norm<97>(lo)); assigned = umax; REQUIRE(int(assigned) == norm<97>(umax));
    REQUIRE(M(true) == 1 && M(false) == 0 && M(char(-1)) == norm<97>(lll(char(-1))));
    REQUIRE(M::init(0) == 0 && M::init(96) == 96);
    stringstream ss; ss << M(-1) << ' ' << 10'000'000'000LL << ' ' << -10'000'000'000LL;
    M a, b, c; ss >> a >> b >> c;
    REQUIRE(ss && a == 96 && int(b) == norm<97>(lll(10'000'000'000LL)) && int(c) == norm<97>(lll(-10'000'000'000LL)));
    context.clear();
}

void testFailedInput() {
    using M = ModInt<97>;
    for (string input : {string(), string("x"), string("999999999999999999999999999999999999")}) {
        M x = 42; stringstream ss(input); ss >> x;
        context = "failed input=" + (input.empty() ? string("<EOF>") : input);
        REQUIRE(ss.fail()); REQUIRE(x == 42);
    }
    context.clear();
}

void testMixedOperationsAndPowers() {
    using M = ModInt<17>;
    M x = 5;
    REQUIRE(x + 20 == 8 && 20 + x == 8); REQUIRE(x - 20 == 2 && 20 - x == 15);
    REQUIRE(x * 20 == 15 && 20 * x == 15); REQUIRE(x / 2 == x * inv(M(2)) && 2 / x == M(2) * inv(x));
    REQUIRE(x == 22 && 22 == x && x != 6);
    for (int a = 1; a < 17; a++) for (lng e : {LLONG_MIN, -1000LL, -65LL, -2LL, -1LL, 0LL, 1LL, 2LL, 65LL, 1000LL, LLONG_MAX}) {
        context = "pow a=" + std::to_string(a) + " e=" + std::to_string(e);
        REQUIRE(pow(M(a), e) == binaryPow(M(a), e));
    }
    REQUIRE(pow(M(0), 0) == 1 && pow(M(0), 9) == 0);
    context.clear();
}

template<int P> void randomLarge(uint64_t seed) {
    using M = ModInt<P>; mt19937_64 rng(seed);
    for (int t = 0; t < 30'000; t++) {
        ulng a = rng(), b = rng(); M x = a, y = b;
        uint A = uint(ulll(a) % P), B = uint(ulll(b) % P);
        context = "large mod=" + std::to_string(P) + " t=" + std::to_string(t);
        REQUIRE(int(x + y) == int((ulng(A) + B) % P));
        REQUIRE(int(x - y) == int((ulng(A) + P - B) % P));
        REQUIRE(int(x * y) == int(ulll(A) * B % P));
        if (B && std::gcd(B, uint(P)) == 1) { REQUIRE(y * inv(y) == 1); REQUIRE((x / y) * y == x); }
    }
    context.clear();
}

template<int P> void randomLargeSqrt(uint64_t seed) {
    using M = ModInt<P>; static_assert(M::is_prime); mt19937_64 rng(seed);
    for (int t = 0; t < 3000; t++) {
        M x = rng(), a = x * x, r = sqrt(a);
        context = "large sqrt mod=" + std::to_string(P) + " t=" + std::to_string(t);
        REQUIRE(r * r == a); REQUIRE(int(r) <= P - int(r));
    }
    context.clear();
}

int main() {
    testCompileTimeAndLayout();
    exhaustiveRing<1>(); exhaustiveRing<2>(); exhaustiveRing<3>(); exhaustiveRing<4>(); exhaustiveRing<5>();
    exhaustiveRing<6>(); exhaustiveRing<7>(); exhaustiveRing<8>(); exhaustiveRing<11>(); exhaustiveRing<12>(); exhaustiveRing<17>();
    testConstructionAssignmentAndIO(); testMixedOperationsAndPowers();
    exhaustiveSqrt<2>(); exhaustiveSqrt<3>(); exhaustiveSqrt<5>(); exhaustiveSqrt<7>(); exhaustiveSqrt<11>();
    exhaustiveSqrt<13>(); exhaustiveSqrt<17>(); exhaustiveSqrt<97>();
    randomLarge<998'244'353>(0x123456789abcdef0ULL);
    randomLarge<2'000'000'000>(0xfedcba9876543210ULL);
    randomLarge<INT_MAX>(0x3141592653589793ULL);
    randomLargeSqrt<998'244'353>(0xa5a5a5a5d3c4b2e1ULL);
    randomLargeSqrt<1'000'000'007>(0x5a5a5a5a1b2c3d4eULL);
    randomLargeSqrt<INT_MAX>(0x9e3779b97f4a7c15ULL);
    testFailedInput();
    cout << GREEN << "PASS: 1-Core_03-modint_tester" << RESET << '\n';
}
