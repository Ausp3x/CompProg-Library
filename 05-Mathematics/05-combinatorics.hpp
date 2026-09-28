#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"

// Prime M, 0 <= n < min(M::mod(), INT_MAX); factorial queries require index <= n.
// Negative selections give 0; numbers of choices/parts/objects must be nonnegative.
// All fields are read-only to callers. Every dynamic setMod, even with the same
// modulus, invalidates the table: reset before any query or direct field access.
// Changed moduli are assertion-checked; Core has no same-modulus generation tag.
// S: O(n + log(M::mod())), Q: O(1), M: O(peak n); three tables retain capacity.
template<typename M = mint>
struct PrimeCombinatorics {
    int n;
    typename M::Word modulus;
    vector<M> fac, inv_fac, der;

    explicit PrimeCombinatorics(int n = 0) { reset(n); }
    void reset(int N) {
        assert(N >= 0 && N < INT_MAX && ulng(N) < M::mod() && M::is_prime);
        n = N; modulus = M::mod(); fac.assign(n + 1, 1); inv_fac.resize(n + 1); der.assign(n + 1, 0);
        for (int i = 1; i <= n; ++i) { fac[i] = fac[i - 1] * i; }
        inv_fac[n] = inv(fac[n]);
        for (int i = n; i > 0; --i) { inv_fac[i - 1] = inv_fac[i] * i; }
        der[0] = 1;
        for (int i = 2; i <= n; ++i) { der[i] = M(i - 1) * (der[i - 1] + der[i - 2]); }}
    void check() const { assert(M::mod() == modulus && M::is_prime); }
    M factorial(int k) const { check(); assert(0 <= k && k <= n); return fac[k]; }
    M inverseFactorial(int k) const { check(); assert(0 <= k && k <= n); return inv_fac[k]; }
    M derangement(int k) const { check(); assert(0 <= k && k <= n); return der[k]; }

    M combiNR(int a, int b) const {
        check(); assert(a >= 0);
        if (b < 0 || b > a) { return 0; }
        assert(a <= n); return fac[a] * inv_fac[b] * inv_fac[a - b]; }
    M combiWR(int a, int b) const {
        check(); assert(a >= 0);
        if (b < 0) { return 0; }
        if (b == 0) { return 1; }
        if (a == 0) { return 0; }
        assert(lng(a) + b - 1 <= n); return combiNR(a + b - 1, b); }
    M permuNR(int a, int b) const {
        check(); assert(a >= 0);
        if (b < 0 || b > a) { return 0; }
        assert(a <= n); return fac[a] * inv_fac[a - b]; }
    // T: O(log(2 + b)), M: O(1); independent of the factorial table size.
    M permuWR(int a, int b) const {
        check(); assert(a >= 0); return b < 0 ? M(0) : pow(M(a), b); }
    // Ordered parts summing to total; positive=false allows zero parts' values.
    // Zero parts give one empty solution exactly when total=0.
    M starsBars(int total, int parts, bool positive = false) const {
        check(); assert(total >= 0 && parts >= 0);
        if (!parts) { return M(total == 0); }
        if (!positive) { return combiWR(parts, total); }
        return total < parts ? M(0) : combiNR(total - 1, parts - 1); }
    M catalan(int k) const {
        check(); assert(k >= 0 && 2 * lng(k) <= n);
        return combiNR(2 * k, k) - combiNR(2 * k, k + 1); }
    // A/B sequences with every prefix A>=B (strict: A>B for nonempty prefixes).
    // Empty sequence counts once in both conventions; strict removes its first A.
    M ballot(int a, int b, bool strict = false) const {
        check(); assert(a >= 0 && b >= 0);
        if (a == 0 && b == 0) { return 1; }
        if (a < b || (strict && a == b)) { return 0; }
        a -= strict; assert(lng(a) + b <= n);
        return combiNR(a + b, b) - combiNR(a + b, b - 1); }
    // T: O(k), M: O(1); k=parts.size(), sum(parts)<=n; empty list gives 1.
    M multinomial(const vector<int> &parts) const {
        check(); int total = 0; M r = 1;
        for (int k : parts) { assert(k >= 0 && k <= n - total); total += k; r *= inv_fac[k]; }
        return fac[total] * r; }
};
using ModFac = PrimeCombinatorics<mint>;

// Fibonacci pair (F_n,F_{n+1}) in the commutative ring T; no division needed.
// Unsigned integers of at least 32 bits compute modulo 2^width; narrow builtins
// are rejected. Signed integer types require all intermediates to fit. Use Exact
// APIs below for exact u64. Full unsigned n supported.
// T: O(log(2 + n)) ring operations, M: O(1).
template<typename T>
pair<T, T> fibonacciPair(ulng n) {
    static_assert(!std::is_integral_v<T> || sizeof(T) >= sizeof(int));
    T a = 0, b = 1;
    for (int i = int(std::bit_width(n)); i-- > 0;) {
        T c = a * (T(2) * b - a), d = a * a + b * b;
        if ((n >> i) & 1) { a = d; b = c + d; }
        else { a = c; b = d; }}
    return {a, b}; }
// T: O(log(2 + n)) ring operations, M: O(1).
template<typename T>
T fibonacci(ulng n) { return fibonacciPair<T>(n).first; }
// L_0=2,L_1=1; same ring/domain as fibonacciPair.
// T: O(log(2 + n)) ring operations, M: O(1).
template<typename T>
T lucas(ulng n) { auto [a, b] = fibonacciPair<T>(n); return T(2) * b - a; }

// Exact APIs accept all unsigned 64-bit inputs. True writes the exact count;
// false means the count exceeds UINT64_MAX and leaves out unchanged. Impossible
// selections count as zero; empty selections/arrangements count once, including 0^0.
// T: O(min(b, a-b)) until overflow (at most 34 iterations), M: O(1).
inline bool combiExact(ulng a, ulng b, ulng &out) {
    if (b > a) { out = 0; return true; }
    b = min(b, a - b); ulng r = 1;
    for (ulng i = 0; i < b; ++i) {
        ulll x = ulll(r) * (a - b + i + 1) / (i + 1);
        if (x > UINT64_MAX) { return false; }
        r = ulng(x); }
    out = r; return true; }
// T: O(min(b,21)), M: O(1); overflow stops before a long iteration.
inline bool permuExact(ulng a, ulng b, ulng &out) {
    if (b > a) { out = 0; return true; }
    ulng r = 1;
    for (ulng i = 0; i < b; ++i) { if (__builtin_mul_overflow(r, a - i, &r)) { return false; } }
    out = r; return true; }
// T: O(min(n,21)), M: O(1).
inline bool factorialExact(ulng n, ulng &out) { return permuExact(n, n, out); }
// T: O(min(b,a-1)) until overflow (at most 34 iterations), M: O(1).
inline bool combiRepExact(ulng a, ulng b, ulng &out) {
    if (b == 0) { out = 1; return true; }
    if (a == 0) { out = 0; return true; }
    ulll n = ulll(a) + b - 1;
    if (n > UINT64_MAX) { return false; }
    return combiExact(ulng(n), b, out); }
// T: O(log(2 + b)), M: O(1).
inline bool permuRepExact(ulng a, ulng b, ulng &out) {
    ulng r = 1;
    while (b) {
        if ((b & 1) && __builtin_mul_overflow(r, a, &r)) { return false; }
        b >>= 1;
        if (b && __builtin_mul_overflow(a, a, &a)) { return false; }}
    out = r; return true; }
// T: O(min(total,parts)) until overflow, M: O(1).
inline bool starsBarsExact(ulng total, ulng parts, ulng &out, bool positive = false) {
    if (!parts) { out = total == 0; return true; }
    if (!positive) { return combiRepExact(parts, total, out); }
    if (total < parts) { out = 0; return true; }
    return combiExact(total - 1, parts - 1, out); }

namespace combinatorics_detail {
    // Positive rational product with an integral result; cancel before multiplying.
    inline bool ratioProduct(ulng r, ulll a, ulll b, ulll c, ulll d, ulng &out) {
        array<ulll, 3> num{r, a, b};
        for (ulll denominator : {c, d}) {
            for (ulll &factor : num) {
                ulll g = gcd(factor, denominator); factor /= g; denominator /= g; }
            assert(denominator == 1); }
        ulll value = 1;
        for (ulll factor : num) {
            if (factor > UINT64_MAX / value) { return false; }
            value *= factor; }
        out = ulng(value); return true; }
} // namespace combinatorics_detail

// Same prefix/empty conventions as PrimeCombinatorics::ballot. Direct ballot
// recurrence avoids overflowing a binomial whose final ballot count still fits.
// T: O(min(b,37) * log(2 + a + b)), M: O(1); logarithm bounds gcd cancellation.
inline bool ballotExact(ulng a, ulng b, ulng &out, bool strict = false) {
    if (a == 0 && b == 0) { out = 1; return true; }
    if (a < b || (strict && a == b)) { out = 0; return true; }
    a -= strict; ulll delta = a - b; ulng r = 1;
    for (ulng i = 0; i < b; ++i) {
        ulll k = ulll(i) + 1;
        if (!combinatorics_detail::ratioProduct(r, delta + 2 * k, delta + 2 * k - 1,
                                               k, delta + k + 1, r)) { return false; }}
    out = r; return true; }
// T: O(min(n,37) * log(2 + n)), M: O(1); largest fitting n=36.
inline bool catalanExact(ulng n, ulng &out) { return ballotExact(n, n, out); }
// T: O(k * 34), M: O(1); k=parts.size(); empty list gives one arrangement.
inline bool multinomialExact(const vector<ulng> &parts, ulng &out) {
    ulng total = 0, r = 1;
    for (ulng k : parts) {
        if (k > UINT64_MAX - total) { return false; }
        total += k; ulng c;
        if (!combiExact(total, k, c) || __builtin_mul_overflow(r, c, &r)) { return false; }}
    out = r; return true; }
// T: O(min(n,20)), M: O(1); D_0=1,D_1=0; largest fitting n=20.
inline bool derangementExact(ulng n, ulng &out) {
    if (n > 20) { return false; }
    ulng a = 1, b = 0;
    for (ulng i = 2; i <= n; ++i) { ulng c = (i - 1) * (a + b); a = b; b = c; }
    out = n == 0 ? 1 : b; return true; }
// T: O(log(2 + n)), M: O(1); both entries fit exactly through n=92.
inline bool fibonacciPairExact(ulng n, pair<ulng, ulng> &out) {
    if (n > 92) { return false; }
    auto [a, b] = fibonacciPair<ulll>(n); out = {ulng(a), ulng(b)}; return true; }
// T: O(log(2 + n)), M: O(1); largest fitting n=93.
inline bool fibonacciExact(ulng n, ulng &out) {
    if (n > 93) { return false; }
    out = ulng(fibonacci<ulll>(n)); return true; }
// T: O(log(2 + n)), M: O(1); largest fitting n=92.
inline bool lucasExact(ulng n, ulng &out) {
    if (n > 92) { return false; }
    out = ulng(lucas<ulll>(n)); return true; }
