#pragma once
#include "01-template.hpp"
#include "03-barrett.hpp"
#include "04-montgomery.hpp"

namespace modint_detail {
template<typename T>
concept Integer = std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>;

// Deterministic Miller-Rabin on the entire unsigned 64-bit domain.
// T: O(log(m)), M: O(1), with seven fixed witnesses.
constexpr bool isPrime(ulng m) {
    if (m < 2) { return false; }
    for (uint p : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        if (m % p == 0) { return m == p; }}
    int s = std::countr_zero(m - 1); ulng d = (m - 1) >> s;
    for (ulng a : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
        a %= m; if (!a) { continue; }
        ulng x = 1;
        for (ulng e = d; e; e >>= 1, a = ulng(ulll(a) * a % m)) {
            if (e & 1) { x = ulng(ulll(x) * a % m); }}
        if (x == 1 || x == m - 1) { continue; }
        int i = 1;
        for (; i < s; ++i) { x = ulng(ulll(x) * x % m); if (x == m - 1) { break; } }
        if (i == s) { return false; }}
    return true;}

// T: O(1 + log(e + 1)), M: O(1); odd modulus, ordinary input/output.
// GCC's constant-modulus specialization regresses this full-width REDC loop;
// retain the measured generic kernel, including setup. See 24-modint.md.
[[gnu::noinline, gnu::noipa]] inline ulng powerMontgomery(ulng a, ulng m, ulll e) {
    Montgomery64 c(m); ulng x = c.init(a), r = c.init(1);
    while (e) {
        if (e & 1) { r = c.mul(r, x); }
        e >>= 1; if (e) { x = c.mul(x, x); }}
    return c.get(r); }

// Contexts are shared per exact type; callers must not modify their fields.
// S: O(log(M)), Q: O(1), M: O(1); static primality is compile-time.
template<typename T, T M, int ID, bool STATIC = (M != 0)>
struct Context {
    static_assert(M > 0);
    using Word = T;
    using Wide = std::conditional_t<std::is_same_v<T, uint>, ulng, ulll>;
    static constexpr T MOD = M;
    static constexpr bool is_prime = modint_detail::isPrime(M);
    static constexpr bool isPrime() { return is_prime; }
    static constexpr T red(Wide x) { return T(x % M); }
};

// All old values/caches become stale on every setMod, even with the same m.
// No generation tag: stale values may only be discarded or overwritten.
// prm=-1 detects primality; 0 disables the prime flag; 1 promises a prime.
// S: O(log(m)) automatic/checked-prime, O(1) with prm=0; M: O(1) per ID/width.
template<typename T, T M, int ID>
struct Context<T, M, ID, false> {
    using Word = T;
    using Wide = std::conditional_t<std::is_same_v<T, uint>, ulng, ulll>;
    using Reducer = std::conditional_t<std::is_same_v<T, uint>, Barrett32, Barrett64>;
    static inline constinit T MOD = 998244353;
    static inline constinit bool is_prime = true;
    static Reducer &reduction() { static Reducer r(MOD); return r; }
    static bool isPrime() { return modint_detail::isPrime(MOD); }
    static void setMod(T m, int prm = -1) {
        assert(m && -1 <= prm && prm <= 1);
        assert(prm != 1 || modint_detail::isPrime(m));
        reduction() = Reducer(m); MOD = m;
        is_prime = prm == -1 ? isPrime() : bool(prm);}
    static T red(Wide x) {
        if constexpr (std::is_same_v<T, ulng>) {
            if (!std::has_single_bit(MOD) && MOD != 2305843009213693951ULL) { return T(x % MOD); }}
        return reduction().reduce(x); }
};

// Canonical n in [0,mod). Do not mutate n except to another canonical value.
// Arithmetic and raw/init require live values. Division/inv and negative pow
// require units; tryInv/batchInv report nonunits without changing output.
// Modulus one is the zero ring: its sole element is a unit, inverse=0, 0^0=0.
// Comparisons order canonical representatives. Integer casts require a fit.
// sqrt/trySqrt require a prime flag; sqrt returns -1 for no root, otherwise the
// smaller root (so its sentinel cannot collide). Prefer trySqrt's bool status.
// S: O(1), Q: O(1), M: O(1); pow/inv/root/batch/stream bounds below.
template<typename C>
struct Value : C {
    using Word = typename C::Word;
    using Wide = typename C::Wide;
    Word n;

    static constexpr Word mod() { return C::MOD; }
    template<Integer T> static constexpr Word norm(T a) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            lll r = lll(a) % lll(mod()); return Word(r < 0 ? r + mod() : r); }
        else { return Word(ulll(a) % mod()); }}
    template<Integer T = int> constexpr Value(T a = 0) : n(norm(a)) {}
    static constexpr Value init(Word a) { assert(a < mod()); Value r; r.n = a; return r; }
    static constexpr Value raw(Word a) { return init(a); }
    constexpr Word val() const { return n; }
    template<Integer T> constexpr Value &operator=(T a) { n = norm(a); return *this; }
    template<Integer T> explicit constexpr operator T() const {
        assert(ulll(n) <= ulll(std::numeric_limits<T>::max())); return T(n); }
    explicit constexpr operator bool() const { return n != 0; }
    constexpr bool operator!() const { return n == 0; }

    constexpr Value &operator++() { n = n == mod() - 1 ? 0 : n + 1; return *this; }
    constexpr Value &operator--() { n = n ? n - 1 : mod() - 1; return *this; }
    constexpr Value operator++(int) { Value a = *this; ++*this; return a; }
    constexpr Value operator--(int) { Value a = *this; --*this; return a; }
    constexpr Value &operator+=(Value a) { n = n >= mod() - a.n ? n - (mod() - a.n) : n + a.n; return *this; }
    constexpr Value &operator-=(Value a) { n = n < a.n ? mod() - (a.n - n) : n - a.n; return *this; }
    constexpr Value &operator*=(Value a) { n = C::red(Wide(n) * a.n); return *this; }
    constexpr Value &operator/=(Value a) { return *this *= inv(a); }

    constexpr Value operator+() const { return *this; }
    constexpr Value operator-() const { return init(n ? mod() - n : 0); }
    friend constexpr Value operator+(Value a, Value b) { return a += b; }
    friend constexpr Value operator-(Value a, Value b) { return a -= b; }
    friend constexpr Value operator*(Value a, Value b) { return a *= b; }
    friend constexpr Value operator/(Value a, Value b) { return a /= b; }
    friend constexpr bool operator==(Value a, Value b) { return a.n == b.n; }
    friend constexpr auto operator<=>(Value a, Value b) { return a.n <=> b.n; }

    // T: O(log(mod())), M: O(1). Euclidean coefficients fit signed 128 bits.
    friend constexpr bool tryInv(Value a, Value &out) {
        Word r = a.n, s = mod(); lll x = 1, y = 0;
        while (s) {
            Word q = r / s; r = std::exchange(s, r % s);
            x = std::exchange(y, x - lll(q) * y);}
        if (r != 1) { return false; }
        out = init(Word(x < 0 ? x + mod() : x)); return true;}
    friend constexpr Value inv(Value a) {
        Value r; bool ok = tryInv(a, r); assert(ok); (void)ok; return r;}
    // T: O(1 + log(|e| + 1)), plus O(log(mod())) for e<0; M: O(1).
    // All native integer exponent widths, including signed minima.
    template<Integer T> friend constexpr Value pow(Value a, T e) {
        ulll b = ulll(e);
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            if (e < 0) { a = inv(a); b = -b; }}
        if constexpr (std::is_same_v<Word, ulng>) {
            if (!std::is_constant_evaluated() && (mod() & 1) && b >= 512 &&
                (mod() != 2305843009213693951ULL || b >= 65536)) {
                return init(modint_detail::powerMontgomery(a.n, mod(), b)); }}
        Value r = 1;
        for (; b > 1; b >>= 1, a *= a) { if (b & 1) { r *= a; } }
        return b ? r * a : r;}

    // Tonelli-Shanks: T: O((z + log(p)) * log(p)), M: O(1), p=mod();
    // z is the first quadratic nonresidue searched from 2 (only p%4==1).
    // No probabilistic runtime claim: deterministic search, exact result.
    friend constexpr bool trySqrt(Value a, Value &out) {
        assert(C::is_prime);
        if (a.n < 2 || mod() == 2) { out = a; return true; }
        Value x;
        if (mod() % 4 == 3) {
            x = pow(a, mod() / 4 + 1); if (x * x != a) { return false; } }
        else {
            if (pow(a, (mod() - 1) / 2) != 1) { return false; }
            int s = std::countr_zero(Word(mod() - 1)); Word q = (mod() - 1) >> s;
            Value z = 2;
            while (pow(z, (mod() - 1) / 2) == 1) { ++z; }
            Value c = pow(z, q), t = pow(a, q); x = pow(a, q / 2 + 1);
            while (t != 1) {
                int i = 1; Value u = t * t;
                while (u != 1) { u *= u; ++i; }
                Value b = c;
                for (int j = 0; j < s - i - 1; ++j) { b *= b; }
                x *= b; c = b * b; t *= c; s = i; }}
        out = init(min(x.n, Word(mod() - x.n))); return true;}
    friend constexpr Value sqrt(Value a) { Value r; return trySqrt(a, r) ? r : init(mod() - 1); }

    // T: O(k + log(mod())), M: O(k) temporary/output; k=a.size() <= INT_MAX.
    // One inverse; empty succeeds. Output unchanged on failure; a/out may alias.
    friend bool batchInv(const vector<Value> &a, vector<Value> &out) {
        assert(a.size() <= INT_MAX); int k = int(a.size());
        vector<Value> r(k); Value p = 1;
        for (int i = 0; i < k; ++i) { r[i] = p; p *= a[i]; }
        if (!tryInv(p, p)) { return false; }
        for (int i = k; i-- > 0;) { Value v = a[i]; r[i] *= p; p *= v; }
        out = std::move(r); return true;}

    // T: O(d), M: O(d) token buffer; arbitrary-length signed decimal only.
    // Malformed token sets failbit and preserves the destination.
    friend istream &operator>>(istream &is, Value &a) {
        string s; if (!(is >> s)) { return is; }
        size_t i = (s[0] == '+' || s[0] == '-'); // string supports full size_t.
        if (i == s.size()) { is.setstate(std::ios::failbit); return is; }
        Value r = 0, ten = 10;
        for (; i < s.size(); ++i) {
            if (s[i] < '0' || s[i] > '9') { is.setstate(std::ios::failbit); return is; }
            r = r * ten + (s[i] - '0');}
        a = s[0] == '-' ? -r : r; return is;}
    friend ostream &operator<<(ostream &os, Value a) { return os << a.n; }
};
} // namespace modint_detail

// Full nonzero uint modulus domain; arithmetic is constexpr, canonical storage.
// S: O(1), Q: O(1), M: O(1); exceptional operation bounds on Value above.
template<uint MOD> requires (MOD > 0)
using ModInt = modint_detail::Value<modint_detail::Context<uint, MOD, 0>>;
using mint = ModInt<998244353>;
