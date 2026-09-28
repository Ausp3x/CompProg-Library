#include "../1-Core/04-dynmodint.hpp"

constexpr const char *GREEN = "\033[1;32m";
constexpr const char *RED = "\033[1;31m";
constexpr const char *RESET = "\033[0m";
string context;

[[noreturn]] void fail(const string &expr, int line) {
    cerr << RED << "FAIL: 1-Core_04-dynmodint_tester.cpp:" << line << ": " << expr;
    if (!context.empty()) { cerr << " [" << context << ']'; }
    cerr << RESET << '\n'; std::abort();
}
#define REQUIRE(x) do { if (!(x)) fail(#x, __LINE__); } while (0)

bool primeOracle(int n) {
    if (n < 2) { return false; }
    for (int d = 2; lng(d) * d <= n; d++) if (n % d == 0) { return false; }
    return true;
}
template<class T> int normOracle(T x, int mod) {
    if constexpr (std::is_same_v<T, ulll>) { return int(x % uint(mod)); }
    else { lll r = lll(x) % mod; return int(r + (r < 0) * mod); }
}
template<> int normOracle<ulng>(ulng x, int mod) { return int(ulll(x) % uint(mod)); }

template<class M> M binaryPow(M a, lng e) {
    ulng u = e < 0 ? -ulng(e) : ulng(e);
    if (e < 0) { a = inv(a); }
    M r = 1;
    while (u) { if (u & 1) { r *= a; } a *= a; u >>= 1; }
    return r;
}

using D = DynModInt<731>;

void exhaustiveRing(int mod) {
    D::setMod(mod); context = "mod=" + std::to_string(mod);
    for (int a = 0; a < mod; a++) for (int b = 0; b < mod; b++) {
        D x = a, y = b;
        REQUIRE(int(x + y) == int((lng(a) + b) % mod));
        REQUIRE(int(x - y) == int((lng(a) - b + mod) % mod));
        REQUIRE(int(x * y) == int(lng(a) * b % mod));
        D z = x; REQUIRE(&(z += y) == &z); REQUIRE(int(z) == (a + b) % mod);
        z = x; REQUIRE(&(z -= y) == &z); REQUIRE(int(z) == (a - b + mod) % mod);
        z = x; REQUIRE(&(z *= y) == &z); REQUIRE(int(z) == int(lng(a) * b % mod));
        z = x; D old = z++; REQUIRE(int(old) == a && int(z) == (a + 1) % mod);
        z = x; old = z--; REQUIRE(int(old) == a && int(z) == (a + mod - 1) % mod);
        z = x; REQUIRE(int(++z) == (a + 1) % mod);
        z = x; REQUIRE(int(--z) == (a + mod - 1) % mod);
        REQUIRE(int(+x) == a && int(-x) == (a ? mod - a : 0));
        REQUIRE(bool(x) == (a != 0) && (!x) == (a == 0));
        REQUIRE((x == y) == (a == b)); REQUIRE((x < y) == (a < b));
        REQUIRE((x <= y) == (a <= b)); REQUIRE((x > y) == (a > b)); REQUIRE((x >= y) == (a >= b));
        if (b && std::gcd(b, mod) == 1) {
            D iy = inv(y); REQUIRE(int(y * iy) == 1 % mod);
            z = x; REQUIRE(&(z /= y) == &z); REQUIRE(z == x * iy); REQUIRE(x / y == z);
        }
    }
    context.clear();
}

void testDefaultsSetModAndIds() {
    using A = DynModInt<1>; using B = DynModInt<2>;
    static_assert(sizeof(A) == sizeof(int));
    static_assert(std::is_trivially_copyable_v<A> && std::is_standard_layout_v<A>);
    REQUIRE(A::MOD == 998'244'353 && A::is_prime);
    REQUIRE(A::mod() == 998'244'353 && B::mod() == 998'244'353);
    REQUIRE(A::IMOD == -1ULL / 998'244'353 + 1);
    REQUIRE(B::MOD == 998'244'353);
    A::setMod(17); REQUIRE(A::MOD == 17 && A::is_prime && A::IMOD == -1ULL / 17 + 1);
    REQUIRE(A::mod() == 17 && B::mod() == 998'244'353);
    B::setMod(12); REQUIRE(B::MOD == 12 && !B::is_prime);
    D::setMod(97, true); REQUIRE(D::MOD == 97 && D::is_prime && D::IMOD == -1ULL / 97 + 1);
    D::setMod(12, false); REQUIRE(D::MOD == 12 && !D::is_prime && D::IMOD == -1ULL / 12 + 1);
    D::setMod(1, false); REQUIRE(D::MOD == 1 && D::mod() == 1 && !D::is_prime && D::IMOD == 0);
    for (int m = 1; m <= 10'000; m++) { D::setMod(m); context = "prime mod=" + std::to_string(m); REQUIRE(D::is_prime == primeOracle(m)); }
    for (auto [m, p] : vector<pair<int, bool>>{{561, false}, {2'047, false}, {1'373'653, false}, {25'326'001, false}, {1'000'000'007, true}, {998'244'353, true}, {INT_MAX - 1, false}, {INT_MAX, true}}) {
        D::setMod(m); context = "prime mod=" + std::to_string(m); REQUIRE(D::is_prime == p);
    }
    context.clear();
}

void testNormalizationAssignmentInitAndIO() {
    D::setMod(97);
    for (lng x : {LLONG_MIN, -10'000'000'000LL, -98LL, -97LL, -1LL, 0LL, 1LL, 97LL, 98LL, 10'000'000'000LL, LLONG_MAX}) {
        context = "signed=" + std::to_string(x); REQUIRE(D::norm(x) == normOracle(x, 97)); REQUIRE(int(D(x)) == normOracle(x, 97));
        D y; REQUIRE(&(y = x) == &y); REQUIRE(int(y) == normOracle(x, 97));
    }
    for (ulng x : {0ULL, 1ULL, 96ULL, 97ULL, 98ULL, 10'000'000'000ULL, ULLONG_MAX}) {
        context = "unsigned=" + std::to_string(x); REQUIRE(D::norm(x) == normOracle(x, 97)); REQUIRE(int(D(x)) == normOracle(x, 97));
        D y; REQUIRE(&(y = x) == &y); REQUIRE(int(y) == normOracle(x, 97));
    }
    lll lo = lll(ulll(1) << 127), hi = (lll(1) << 126) - 1; hi += (lll(1) << 126); ulll umax = ~ulll(0);
    REQUIRE(D::norm(lo) == normOracle(lo, 97)); REQUIRE(D::norm(hi) == normOracle(hi, 97)); REQUIRE(D::norm(umax) == normOracle(umax, 97));
    D y; y = lo; REQUIRE(int(y) == normOracle(lo, 97)); y = umax; REQUIRE(int(y) == normOracle(umax, 97));
    REQUIRE(D(true) == 1 && D(false) == 0 && D(char(-1)) == normOracle(char(-1), 97));
    REQUIRE(D::init(0) == 0 && D::init(96) == 96);
    stringstream ss; ss << D(-1) << ' ' << 10'000'000'000LL << ' ' << -10'000'000'000LL;
    D a, b, c; ss >> a >> b >> c;
    REQUIRE(ss && a == 96 && int(b) == normOracle(10'000'000'000LL, 97) && int(c) == normOracle(-10'000'000'000LL, 97));
    context.clear();
}

void testFailedInput() {
    D::setMod(97);
    for (string input : {string(), string("x"), string("999999999999999999999999999999999999")}) {
        D x = 42; stringstream ss(input); ss >> x;
        context = "failed input=" + (input.empty() ? string("<EOF>") : input);
        REQUIRE(ss.fail()); REQUIRE(x == 42);
    }
    context.clear();
}

void testPowersMixedAndSqrt() {
    D::setMod(17); D x = 5;
    REQUIRE(x + 20 == 8 && 20 + x == 8); REQUIRE(x - 20 == 2 && 20 - x == 15);
    REQUIRE(x * 20 == 15 && 20 * x == 15); REQUIRE(x / 2 == x * inv(D(2)) && 2 / x == D(2) * inv(x));
    REQUIRE(x == 22 && 22 == x && x != 6);
    for (int a = 1; a < 17; a++) for (lng e : {LLONG_MIN, -1000LL, -65LL, -2LL, -1LL, 0LL, 1LL, 2LL, 65LL, 1000LL, LLONG_MAX}) {
        context = "pow a=" + std::to_string(a) + " e=" + std::to_string(e); REQUIRE(pow(D(a), e) == binaryPow(D(a), e));
    }
    REQUIRE(pow(D(0), 0) == 1 && pow(D(0), 9) == 0);
    for (int mod = 2; mod <= 199; mod++) if (primeOracle(mod)) {
        D::setMod(mod);
        for (int a = 0; a < mod; a++) {
            context = "sqrt mod=" + std::to_string(mod) + " a=" + std::to_string(a);
            bool residue = false; for (int r = 0; r < mod; r++) residue |= lng(r) * r % mod == a;
            D r = sqrt(D(a));
            if (residue) { REQUIRE(int(r * r) == a); REQUIRE(int(r) <= mod - int(r)); }
            else { REQUIRE(int(r) == mod - 1); }
        }
    }
    D::setMod(1); REQUIRE(pow(D(0), 0) == 0 && pow(D(0), 100) == 0);
    context.clear();
}

void stressReductionAndLargeArithmetic() {
    mt19937_64 rng(0xc001d00d1234abcdULL);
    for (int mod : {1, 2, 3, 4, 17, 998'244'353, 1'000'000'007, 2'000'000'000, INT_MAX - 1, INT_MAX}) {
        D::setMod(mod);
        for (ulng a : {0ULL, 1ULL, ulng(mod - 1), ulng(mod), ulng(mod) + 1, ULLONG_MAX}) {
            context = "boundary red mod=" + std::to_string(mod) + " a=" + std::to_string(a);
            REQUIRE(D::red(a) == a % uint(mod));
        }
        for (int t = 0; t < 50'000; t++) {
            ulng a = rng(), b = rng(); context = "stress mod=" + std::to_string(mod) + " t=" + std::to_string(t);
            REQUIRE(D::red(a) == a % uint(mod));
            D x = a, y = b; uint A = uint(a % uint(mod)), B = uint(b % uint(mod));
            REQUIRE(int(x + y) == int((ulng(A) + B) % uint(mod)));
            REQUIRE(int(x - y) == int((ulng(A) + uint(mod) - B) % uint(mod)));
            REQUIRE(int(x * y) == int(ulll(A) * B % uint(mod)));
            if (B && std::gcd(B, uint(mod)) == 1 && (t & 31) == 0) { REQUIRE(y * inv(y) == 1); REQUIRE((x / y) * y == x); }
        }
    }
    for (int t = 0; t < 5'000; t++) {
        int mod = int(rng() % INT_MAX) + 1; D::setMod(mod);
        ulng u = rng(); lng s = lng(rng()); ulll u128 = (ulll(rng()) << 64) | rng(); lll s128 = lll(u128);
        context = "random mod=" + std::to_string(mod) + " t=" + std::to_string(t);
        REQUIRE(D::red(u) == u % uint(mod)); REQUIRE(int(D(u)) == normOracle(u, mod)); REQUIRE(int(D(s)) == normOracle(s, mod));
        REQUIRE(int(D(u128)) == normOracle(u128, mod)); REQUIRE(int(D(s128)) == normOracle(s128, mod));
    }
    context.clear();
}

void randomLargeSqrt() {
    mt19937_64 rng(0x6a09e667f3bcc909ULL);
    for (int mod : {998'244'353, 1'000'000'007, INT_MAX}) {
        D::setMod(mod); REQUIRE(D::is_prime);
        for (int t = 0; t < 3000; t++) {
            D x = rng(), a = x * x, r = sqrt(a);
            context = "large sqrt mod=" + std::to_string(mod) + " t=" + std::to_string(t);
            REQUIRE(r * r == a); REQUIRE(int(r) <= mod - int(r));
        }
    }
    context.clear();
}

int main() {
    testDefaultsSetModAndIds();
    for (int mod = 1; mod <= 80; mod++) { exhaustiveRing(mod); }
    testNormalizationAssignmentInitAndIO(); testPowersMixedAndSqrt(); stressReductionAndLargeArithmetic();
    randomLargeSqrt(); testFailedInput();
    cout << GREEN << "PASS: 1-Core_04-dynmodint_tester" << RESET << '\n';
}
