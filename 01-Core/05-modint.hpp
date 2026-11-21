#pragma once
#include "01-template.hpp"
#include "03-barrett.hpp"
#include "04-montgomery.hpp"

namespace modint_detail {
    // T: O(1), M: O(1); whether an exponent of 64 or 128 bits has bits from 2^64 on.
    template<class E> [[gnu::always_inline]] constexpr bool hi64(E b) { if constexpr (sizeof(E) > 8) { return (b >> 64) != 0; } else { return false; } }
    // T: O(1), M: O(1); b >= t by one-word comparisons.
    template<class E> [[gnu::always_inline]] constexpr bool atLeast(E b, ulng t) { return hi64(b) || ulng(b) >= t; }
    // T: O(1), M: O(1); the get and put areas of a stream buffer, reached through member pointers.
    struct Area : std::streambuf {
        using std::streambuf::gptr; using std::streambuf::egptr; using std::streambuf::gbump;
        using std::streambuf::pptr; using std::streambuf::epptr; using std::streambuf::pbump;
    };
#ifdef __GLIBCXX__
    // T: O(1), M: O(1); libstdc++ keeps the stream's character classification in the stream.
    struct IosCache : std::ios { using std::ios::_M_ctype; };
    // T: O(1), M: O(1); the stream's character classification, null when its locale has none.
    inline const std::ctype<char> *ctypeOf(std::ios &s) { return s.*&IosCache::_M_ctype; }
#else
    // T: O(1), M: O(1); the stream's character classification, null when its locale has none.
    inline const std::ctype<char> *ctypeOf(std::ios &s) { return std::has_facet<std::ctype<char>>(s.getloc()) ? &std::use_facet<std::ctype<char>>(s.getloc()) : nullptr; }
#endif
    inline constexpr ulng P10[19] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, 10000000000, 100000000000, 1000000000000, 10000000000000,
                                     100000000000000, 1000000000000000, 10000000000000000, 100000000000000000, 1000000000000000000};
    // BCD4[x]: the four decimal digits of x as bytes of value 0 to 9, the leading digit lowest.
    inline constexpr std::array<uint, 10000> BCD4 = [] {
        std::array<uint, 10000> t{};
        for (uint i = 0; i < 10000; ++i) { t[i] = i / 1000 | i / 100 % 10 << 8 | i / 10 % 10 << 16 | i % 10 << 24; }
        return t;}();
    // T: O(1), M: O(1); the digits of x < 10^4 as four bytes.
    [[gnu::always_inline]] constexpr uint bcd4(uint x) { return BCD4[x]; }
    // T: O(1), M: O(1); the digits of x < 10^8 as eight bytes.
    [[gnu::always_inline]] constexpr ulng bcd8(uint x) { return BCD4[x / 10000] | ulng(BCD4[x % 10000]) << 32; }
    // T: O(1), M: O(1); the decimal digits of x at w, where 20 bytes are writable: returns their number, and writes no byte behind them.
    template<typename W> [[gnu::always_inline]] inline uint digitsTo(char *w, W x) {
        constexpr ulng E8 = 100000000, ZEROS = 0x3030303030303030;
        if (x < 10) { *w = char('0' + x); return 1; }
        if (x < E8) { // k digits by stores that overlap
            ulng y = bcd8(uint(x)); uint z = uint(std::countr_zero(y)) >> 3, k = 8 - z; // z leading zeros
            y = (y | ZEROS) >> 8 * z;
            if (k >= 4) { std::memcpy(w, &y, 4); y >>= 8 * (k - 4); std::memcpy(w + (k - 4), &y, 4); }
            else { std::memcpy(w, &y, 2); w[k - 1] = char(y >> 8 * (k - 1)); }
            return k;}
        auto head4 = [&](uint h) { uint y = bcd4(h), z = uint(std::countr_zero(y)) >> 3; y = (y | uint(ZEROS)) >> 8 * z; std::memcpy(w, &y, 4); return 4 - z; };
        uint n; // the head's digits without leading zeros; the block after covers the bytes behind them
        if constexpr (sizeof(W) == 4) { n = head4(uint(x / E8)); }
        else if (x < E8 * E8) { ulng y = bcd8(uint(x / E8)); uint z = uint(std::countr_zero(y)) >> 3; y = (y | ZEROS) >> 8 * z; std::memcpy(w, &y, 8); n = 8 - z; }
        else { n = head4(uint(x / (E8 * E8))); x %= E8 * E8; ulng y = bcd8(uint(x / E8)) | ZEROS; std::memcpy(w + n, &y, 8); n += 8; }
        ulng y = bcd8(uint(x % E8)) | ZEROS; std::memcpy(w + n, &y, 8); return n + 8;}
    // T: O(d), M: O(1); the decimal digits of v on any stream; a set width or a non-decimal base goes through the stream's own formatting.
    template<typename W> [[gnu::noinline]] ostream &writeSlow(ostream &os, W v) {
        if (os.width() != 0 || (os.flags() & (std::ios::oct | std::ios::hex))) { return os << v; }
        char buf[20]; std::streamsize k = digitsTo(buf, v);
        if (!(os.flags() & std::ios::unitbuf) && os.good() && !os.tie()) { // write() without its sentry, which would do nothing
            try { if (os.rdbuf()->sputn(buf, k) != k) { os.setstate(std::ios::badbit); } }
            catch (...) {
                try { os.setstate(std::ios::badbit); } catch (...) {}
                if (os.exceptions() & std::ios::badbit) { throw; }}
            return os;}
        return os.write(buf, k);}
    template<typename T>
    concept Integer = std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>;
    template<typename C>
    concept Fixed = requires { std::integral_constant<typename C::Word, C::MOD>{}; };

    // T: O(log(a + 1) + log(n)), M: O(1); the Jacobi symbol (a / n) for odd n without a division; 0 on a shared factor.
    constexpr int jacobi(ulng a, ulng n) {
        ulng r = 0;
        while (a) {
            int t = std::countr_zero(a); a >>= t; r ^= ulng(t) & ((n ^ n >> 1) >> 1);
            ulng lt = -ulng(a < n), d = a - n;
            r ^= (a & n) >> 1 & lt; n = lt ? a : n; a = (d ^ lt) - lt;}
        return n == 1 ? (r & 1 ? -1 : 1) : 0;}
    // T: O(1), M: O(1); Montgomery product x y / 2^64 mod m for odd m and x y < 2^64 m: no branch, ni = -m^-1 mod 2^64 negated.
    [[gnu::always_inline]] constexpr ulng mmul(ulng x, ulng y, ulng ni, ulng m) {
        ulll z = ulll(x) * y; ulng t = ulng(z >> 64), u = ulng(ulll(ulng(z) * ni) * m >> 64);
        return t - u + (t < u ? m : 0);}
    // T: O(1), M: O(1); m^-1 mod 2^bits for odd m by Newton's iteration from the five bits of 3 m ^ 2, the error squared beside each step.
    [[gnu::always_inline]] constexpr ulng inv64(ulng m, int bits = 64) {
        ulng x = 3 * m ^ 2, y = 1 - m * x;
        if (bits <= 5) { return x; }
        for (int have = 10; have < bits; have *= 2) { x *= 1 + y; y *= y; }
        return x * (1 + y);}
    // T: O(1), M: O(1); the same without the correction for m < 2^62: factors and result below 2 m.
    [[gnu::always_inline]] constexpr ulng mmulLazy(ulng x, ulng y, ulng ni, ulng m) {
        ulll z = ulll(x) * y; return ulng(z >> 64) + m - ulng(ulll(ulng(z) * ni) * m >> 64);}
    // T: O(log(b + 1)), M: O(1); x^b, one for b = 0, by the binary loop on the product mul. A b below 2^64 starts at its lowest
    // set bit, which saves the product by one; a larger b runs the plain loop from one, its bits from 2^64 down first.
    template<class X> [[gnu::always_inline]] constexpr X powBits(X one, X x, auto b, auto mul) {
        if (hi64(b)) {
            X r = one;
            for (; hi64(b); b >>= 1, x = mul(x, x)) { if (b & 1) { r = mul(r, x); } }
            for (ulng f = ulng(b); f > 1; f >>= 1, x = mul(x, x)) { if (f & 1) { r = mul(r, x); } }
            return mul(r, x);}
        ulng f = ulng(b);
        if (!f) { return one; }
        for (; !(f & 1); f >>= 1) { x = mul(x, x); }
        X r = x; f >>= 1;
        if (!f) { return r; }
        for (x = mul(x, x); f > 1; f >>= 1, x = mul(x, x)) { if (f & 1) { r = mul(r, x); } }
        return mul(r, x);}
    // T: O(log(f + 1)), M: O(1); x^f by the binary loop from the low bit, its multiplier x or one under a mask: no branch reads
    // f's bits, at two products a bit on two chains.
    template<class X> [[gnu::always_inline]] constexpr X powSelect(X one, X x, ulng f, auto mul) {
        auto pick = [&](ulng bit) __attribute__((always_inline)) {
            if constexpr (std::is_integral_v<X>) { return X(one ^ ((x ^ one) & (0 - bit))); }
            else { X v = one; v.n ^= (x.n ^ one.n) & (0 - decltype(v.n)(bit)); return v; }};
        X r = one;
        for (; f > 1; f >>= 1, x = mul(x, x)) { r = mul(r, pick(f & 1)); }
        return f ? mul(r, x) : r;}
    // T: O(log(f + 1)), M: O(1); x^f by base-4 digits from the low end, t[d] collecting x^(4^i) over the digits equal to d: no
    // branch on f's bits either, at three products a digit and four behind them; ahead of powSelect where products overlap.
    template<class X> [[gnu::always_inline]] constexpr X powDigits(X one, X x, ulng f, auto mul) {
        X t[4] = {one, one, one, one};
        for (; f > 3; f >>= 2, x = mul(x, x), x = mul(x, x)) { t[f & 3] = mul(t[f & 3], x); }
        t[f] = mul(t[f], x); X v = mul(t[2], t[3]);
        return mul(mul(mul(t[1], t[3]), v), v);}

    // Forisek and Jancina's bases: an n < 2^32 that the trial primes through 71 do not settle is prime iff it passes the strong
    // test to the base FJ32[hash(n)]; checked against a sieve for every such n (the tester holds the table's digest).
    inline constexpr uint16_t FJ32[256] = {
        15591, 2018, 166, 7429, 8064, 16045, 10503, 4399, 1949, 1295, 2776, 3620, 560, 3128, 5212, 2657,
        2300, 2021, 4652, 1471, 9336, 4018, 2398, 20462, 10277, 8028, 2213, 6219, 620, 3763, 4852, 5012,
        3185, 1333, 6227, 5298, 1074, 2391, 5113, 7061, 803, 1269, 3875, 422, 751, 580, 4729, 10239,
        746, 2951, 556, 2206, 3778, 481, 1522, 3476, 481, 2487, 3266, 5633, 488, 3373, 6441, 3344,
        17, 15105, 1490, 4154, 2036, 1882, 1813, 467, 3307, 14042, 6371, 658, 1005, 903, 737, 1887,
        7447, 1888, 2848, 1784, 7559, 3400, 951, 13969, 4304, 177, 41, 19875, 3110, 13221, 8726, 571,
        7043, 6943, 1199, 352, 6435, 165, 1169, 3315, 978, 233, 3003, 2562, 2994, 10587, 10030, 2377,
        1902, 5354, 4447, 1555, 263, 27027, 2283, 305, 669, 1912, 601, 6186, 429, 1930, 14873, 1784,
        1661, 524, 3577, 236, 2360, 6146, 2850, 55637, 1753, 4178, 8466, 222, 2579, 2743, 2031, 2226,
        2276, 374, 2132, 813, 23788, 1610, 4422, 5159, 1725, 3597, 3366, 14336, 579, 165, 1375, 10018,
        12616, 9816, 1371, 536, 1867, 10864, 857, 2206, 5788, 434, 8085, 17618, 727, 3639, 1595, 4944,
        2129, 2029, 8195, 8344, 6232, 9183, 8126, 1870, 3296, 7455, 8947, 25017, 541, 19115, 368, 566,
        5674, 411, 522, 1027, 8215, 2050, 6544, 10049, 614, 774, 2333, 3007, 35201, 4706, 1152, 1785,
        1028, 1540, 3743, 493, 4474, 2521, 26845, 8354, 864, 18915, 5465, 2447, 42, 4511, 1660, 166,
        1249, 6259, 2553, 304, 272, 7286, 73, 6554, 899, 2816, 5197, 13330, 7054, 2818, 3199, 811,
        922, 350, 7514, 4452, 3449, 2663, 4708, 418, 1621, 1171, 3471, 88, 11345, 412, 1559, 194};

    // T: O(1), M: O(1); for an odd m coprime to the prime Q: the k < 32 that hold Q to an odd power when (Q / m) = -1, else 0.
    template<uint Q> constexpr uint symbolMask(ulng m) {
        constexpr uint SQUARES = [] { uint b = 0; for (uint i = 1; i < Q; ++i) { b |= uint(1) << i * i % Q; } return b; }();
        constexpr uint ODD = [] { uint b = 0; for (uint k = Q; k < 32; k += Q) { for (uint t = k; t % Q == 0; t /= Q) { b ^= uint(1) << k; } } return b; }();
        if constexpr (Q == 2) { return (m ^ m >> 1) & 2 ? ODD : 0; }
        else { return bool(SQUARES >> m % Q & 1) == ((Q & m & 3) == 3) ? ODD : 0; }} // reciprocity: the sign turns when both are 3 mod 4
    // T: O(log(m)), M: O(1); Baillie-PSW for an odd m from 2^32 that no trial prime divides: the strong test to base 2, then the extra strong Lucas
    // test. one = 2^64 mod m, ni = m^-1 mod 2^64, m - 1 = d 2^s. Out of line: its loops keep their registers whatever the caller holds.
    [[gnu::noinline]] inline bool strongLucas(ulng m, ulng one, ulng ni, ulng d, int s) {
        ulng minus = m - one;
        auto mul = [&](ulng x, ulng y) { return mmul(x, y, ni, m); };
        auto add = [&](ulng x, ulng y) { ulng t = x + y; return t - ((t < x || t >= m) ? m : 0); };
        auto sub = [&](ulng x, ulng y) { return x - y + (x < y ? m : 0); };
        ulng two = add(one, one), x = two;
        if (m >> 63) { // 2^d from the low bit: twice a factor would not fit a word
            ulng b = two, e = d;
            for (x = one; e > 1; e >>= 1, b = mul(b, b)) { if (e & 1) { x = mul(x, b); } }
            x = mul(x, b);}
        else { // from the top bit: a set bit doubles one factor of the square, 2 x^2 < m 2^64
            auto top = [&](auto f) __attribute__((always_inline)) { for (int j = std::bit_width(d) - 2; j >= 0; --j) { x = f(x, x + (x & (0 - (d >> j & 1)))); } };
            if (m >> 61) { top(mul); } // below 2^61 lazy products: 8 m^2 < m 2^64
            else { top([&](ulng p, ulng q) __attribute__((always_inline)) { return mmulLazy(p, q, ni, m); }); x -= x >= m ? m : 0; }}
        if (x != one && x != minus) {
            int i = 1;
            for (; i < s; ++i) { x = mul(x, x); if (x == minus) { break; } }
            if (i == s) { return false; }}
        // Extra strong Lucas test, Q = 1, on the first P >= 3 with (P^2 - 4 / m) = -1; no P below 29: a square m has none, any other is searched on.
        uint neg = symbolMask<2>(m) ^ symbolMask<3>(m) ^ symbolMask<5>(m) ^ symbolMask<7>(m) ^ symbolMask<11>(m)
            ^ symbolMask<13>(m) ^ symbolMask<17>(m) ^ symbolMask<19>(m) ^ symbolMask<23>(m) ^ symbolMask<29>(m); // the k < 31 with (k / m) = -1, by the primes in k
        uint hit = (neg ^ neg >> 4) & 0x7FFFFFE; ulng p = hit ? ulng(std::countr_zero(hit)) + 2 : 29; // hit: the P - 2 in [1, 26] whose (P - 2 / m) (P + 2 / m) is -1
        if (!hit) {
            ulng rt = ulng(std::sqrt(static_cast<long double>(m)));
            while (ulll(rt) * rt > m) { --rt; }
            while (ulll(rt + 1) * (rt + 1) <= m) { ++rt; }
            if (rt * rt == m) { return false; }
            for (;; ++p) { int j = jacobi(p * p - 4, m); if (j == -1) { break; } if (j == 0) { return false; } }}
        int s2 = std::countr_zero(m + 1); ulng d2 = (m + 1) >> s2, mp = two; // P 2^64 mod m
        for (ulng i = 2; i < p; ++i) { mp = add(mp, one); }
        ulng v = mp, w = sub(mul(mp, mp), two); // V_1 and V_2, then (V_k, V_(k + 1)) down the bits of d2
        for (int j = std::bit_width(d2) - 2; j >= 0; --j) {
            bool bit = d2 >> j & 1; ulng t = sub(mul(v, w), mp), u = bit ? w : v, sq = sub(mul(u, u), two);
            v = bit ? t : sq; w = bit ? sq : t;}
        if ((v == two || v == m - two) && add(w, w) == mul(mp, v)) { return true; } // U_d = 0 and V_d = +-2
        for (int i = 0; i < s2 - 1; ++i) { if (v == 0) { return true; } v = sub(mul(v, v), two); }
        return false;}
    // WHEEL: bit r is set iff r shares a factor with 30030 = 2 3 5 7 11 13.
    inline constexpr std::array<ulng, 470> WHEEL = [] {
        std::array<ulng, 470> t{};
        for (uint p : {2, 3, 5, 7, 11, 13}) { for (uint i = 0; i < 30030; i += p) { t[i >> 6] |= ulng(1) << (i & 63); } }
        return t;}();
    // T: O(1), M: O(1); the first of the primes P that divides m, else 0: each test a product by a constant, never a division.
    template<uint... P> [[gnu::always_inline]] constexpr uint trial(ulng m) { uint r = 0; (void)((m % P == 0 && (r = P)) || ...); return r; }
    // T: O(log(m)), M: O(1); isPrime for an m >= 5329 that no prime through 71 divides. Out of line: a caller's loop keeps its registers.
    [[gnu::noinline]] constexpr bool strongTest(ulng m, ulng r) {
        int s = std::countr_zero(m - 1); ulng d = (m - 1) >> s;
        if (std::is_constant_evaluated()) {
            for (ulng a : {2, 325, 9375, 28178, 450775, 9780504, 1795265022}) {
                a %= m; if (!a) { continue; }
                ulng x = 1;
                for (ulng e = d; e; e >>= 1, a = ulng(ulll(a) * a % m)) {
                    if (e & 1) { x = ulng(ulll(x) * a % m); }}
                if (x == 1 || x == m - 1) { continue; }
                int i = 1;
                for (; i < s; ++i) { x = ulng(ulll(x) * x % m); if (x == m - 1) { break; } }
                if (i == s) { return false; }}
            return true;}
        ulng ni = 3 * m ^ 2, one = r; // m^-1 mod 2^64 by Newton's iteration from five bits; 2^64 mod m: the caller's, else by one division
        if (!one && m - 2048 < 0xFFFFF800) { // in [2048, 2^32) a double division: its truncated quotient is floor(2^64 / m) or one more
            one = 0 - ulng(lng(0x1p64 / double(lng(m)))) * m; one += lng(one) < 0 ? m : 0;}
        else if (!one) { one = (0 - m) % m; }
        for (int i = 0; i < 4; ++i) { ni *= 2 - m * ni; }
        ulng minus = m - one;
        if (!(m >> 32)) {
            ulng h = m; h = ((h >> 16) ^ h) * 0x45d9f3b; h = ((h >> 16) ^ h) * 0x45d9f3b; h = ((h >> 16) ^ h) & 255;
            ulng a = FJ32[h]; if (a >= m) { a %= m; if (!a) { return false; } } // a base below m needs no division
            auto mul = [&](ulng x, ulng y) { return m - ulng(ulll(x * y * ni) * m >> 64); }; // x, y in [1, m]: the product's high half is 0
            ulng x = one, b = mmul(a * one, one, ni, m), e = d; // b = a 2^64 mod m
            for (; e > 1; e >>= 1, b = mul(b, b)) { x ^= (x ^ mul(x, b)) & -(e & 1); } // a select: no predictor knows the bits of e
            x = mul(x, b);
            if (x == one || x == minus) { return true; }
            for (int i = 1; i < s; ++i) { x = mul(x, x); if (x == minus) { return true; } }
            return false;}
        return strongLucas(m, one, ni, d, s);}
    // T: O(log(m)), M: O(1); exact over all of ulng: the primes through 71 by WHEEL and trial, then one hashed-base strong test below 2^32 (Forisek-
    // Jancina), Baillie-PSW from 2^32, seven-base Miller-Rabin in constant evaluation. r: 2^64 mod m for an odd m, when the caller holds it.
    [[gnu::always_inline]] constexpr bool isPrime(ulng m, ulng r = 0) {
        if (m < 2) { return false; }
        if (uint w = uint(m % 30030); WHEEL[w >> 6] >> (w & 63) & 1) { return m < 14 && (0x28AC >> m & 1); } // prime only as one of the six
        if (uint p = trial<17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71>(m)) { return m == p; }
        if (m < 5329) { return true; } // 73^2
        return strongTest(m, r);}

    // T: O(log(e + 1)), M: O(1); a^e mod m for odd m and a < m on Montgomery products, lazy below 2^62. SELECT: by powSelect over
    // e < 2^64, for an exponent that changes between calls. FORM: a and the result times 2^64. Out of line on purpose.
    template<bool SELECT, bool FORM = false> [[gnu::noinline, gnu::noipa]] ulng powMont(const Montgomery64 &c, ulng a, std::conditional_t<SELECT, ulng, ulll> e) {
        ulng ni = 0 - c.inv, m = c.mod;
        auto run = [&](auto mul) __attribute__((always_inline)) {
            ulng x = FORM ? a : mul(a, c.rsq), one = mul(1, c.rsq), r;
            if constexpr (SELECT) { r = powSelect(one, x, e, mul); } else { r = powBits(one, x, e, mul); }
            return FORM ? r - (r >= m ? m : 0) : mmul(r, 1, ni, m);};
        if (m >> 62) { return run([&](ulng x, ulng y) __attribute__((always_inline)) { return mmul(x, y, ni, m); }); }
        return run([&](ulng x, ulng y) __attribute__((always_inline)) { return mmulLazy(x, y, ni, m); });}

    // T: O(1), M: O(1); the multiple of m in [2^63, 2^63 + m), m itself from 2^63: added to a signed source it leaves an unsigned dividend.
    constexpr ulng biasOf(ulng m) { return m >> 63 ? m : (((ulng(1) << 63) - 1) / m + 1) * m; }
    // T: O(log(m)), M: O(1); m < 2^63: true iff the round-up reciprocal of m for 64-bit dividends fits the word (Granlund-Montgomery).
    constexpr bool shortReciprocal(ulng m) {
        int l = std::bit_width(m - 1);
        ulll lo = (ulll(1) << (64 + l)) / m, hi = ((ulll(1) << (64 + l)) + (ulll(1) << l)) / m;
        for (; l > 0 && lo / 2 < hi / 2; --l) { lo /= 2; hi /= 2; }
        return hi >> 64 == 0;}
    // T: O(1), M: O(1); true iff 2^w = 1 (mod m) for some w in [32, 64]: GCC then takes a 128-bit remainder by the constant m inline.
    constexpr bool unitShift(ulng m) {
        ulng p = (ulng(1) << 32) % m;
        for (int w = 32; w < 64 && p != 1; ++w) { p = ulng(2 * ulll(p) % m); }
        return p == 1;}
    template<typename C> concept PowerOfTwo = Fixed<C> && std::has_single_bit(C::MOD); // every result is a masked word
    template<typename C> concept Form = requires { typename C::Base; }; // Montgomery-form residues (MontContext)
    template<typename C> concept ExactStep = Fixed<C> && (C::EXACT || C::MOD == Barrett64::MERSENNE61);
    // Contexts whose product is one short word step (32-bit words, Mersenne61, a static power of two, static 64-bit moduli below 2^32 with a
    // one-word or exact reciprocal): pow keeps the plain loop there instead of Montgomery and takes powBlind on a changed exponent.
    template<typename C>
    concept Direct = !Form<C> && (std::is_same_v<typename C::Word, uint> || PowerOfTwo<C>
        || (Fixed<C> && (C::MOD == Barrett64::MERSENNE61 || (ulng(C::MOD) >> 32 == 0 && shortReciprocal(C::MOD)) || C::EXACT)));
    // pow leaves the binary loop from these exponents: a dynamic 64-bit word below 2^32, Montgomery, Montgomery under a static modulus from 2^63 (its own
    // product is shorter), a static 64-bit word below 2^32 on a repeated and a changed exponent; a change is noticed from TRACK, a dynamic Mersenne61's from TRACK61.
    constexpr ulng POW_SMALL = 4, POW_MONT = 5, POW_MONT63 = 9, POW_FIXED = 96, POW_FIXED_CHANGED = 16, POW_TRACK = 4, POW_TRACK61 = 8;
    // batchInv's scalar chains by size, {one below, two below, four below} and eight from there, by product: a static word product, the exact
    // and Mersenne61 products, a dynamic modulus (never two chains), Montgomery chains. AVX2 lanes of a 32-bit word: 8 from LANES, then 16, 32.
    constexpr int BATCH_STATIC[3] = {9, 22, 80}, BATCH_EXACT[3] = {11, 14, 64}, BATCH_DYNAMIC[3] = {13, 13, 96}, BATCH_MONT[3] = {6, 16, 256};
#ifdef __AVX2__
    constexpr bool AVX2 = true; // GCC's loop vectorizer then takes other source forms of ++ and unary minus
#else
    constexpr bool AVX2 = false;
#endif
    constexpr int INV_TABLE = 2048, INV_TINY = 16, INV_SKEW = 8; // tryInv: a table below INV_TABLE; a division first when one operand is INV_SKEW bits longer
    // A dynamic modulus m = q a + e and a < INV_TINY: a^-1 = t q + c for t e = -1 (mod a), t < a, c = (t e + 1) / a. INV_STEP[a][e] = t | c << 4,
    // 255 where gcd(a, e) > 1; INV_RECIP[a] = floor((2^64 - 1) / a).
    inline constexpr std::array<ulng, INV_TINY> INV_RECIP = [] { std::array<ulng, INV_TINY> r{}; for (int a = 1; a < INV_TINY; ++a) { r[a] = ~ulng(0) / ulng(a); } return r; }();
    inline constexpr std::array<std::array<uint8_t, INV_TINY>, INV_TINY> INV_STEP = [] {
        std::array<std::array<uint8_t, INV_TINY>, INV_TINY> s{};
        for (int a = 1; a < INV_TINY; ++a) {
            for (int e = 0; e < a; ++e) {
                int t = 0;
                while (t < a && (t * e + 1) % a) { ++t; }
                s[a][e] = uint8_t(t < a ? t | (t * e + 1) / a << 4 : 255);}}
        return s;}();
    constexpr int BATCH_LANES = 48, BATCH_LANES16 = 80, BATCH_LANES32 = 512, ROOT_ONES = 16; // ROOT_ONES: the least run of ones rootFixed chains

    // S: O(1), Q: O(1), M: O(1); Moller-Granlund division by the normalized d = m << sh with v = floor((2^128 - 1) / d) - 2^64.
    // Modulus in [1, 2^64); mul needs a < m, any b, and returns a * b % m.
    struct Reciprocal {
        int sh; ulng d, v;
        constexpr Reciprocal(ulng m) : sh(std::countl_zero(m)), d(m << sh), v(ulng(~ulll(0) / d)) {}
        constexpr Reciprocal(ulng m, ulll mu) : sh(std::countl_zero(m)), d(m << sh), v(ulng(mu >> sh)) {} // mu = floor((2^128 - 1) / m)
        // (xh * 2^64 + xl) % d for xh < d: the remainder of an already shifted dividend, still shifted.
        [[gnu::always_inline]] constexpr ulng rem2(ulng xh, ulng xl, ulng tl, ulng th) const { // th * 2^64 + tl = v * xh
            ulng ql = tl + xl, qh = th + xh + (ql < xl), r = xl - (qh + 1) * d;
            r += d & -ulng(r > ql);
            return min(r, r - d);}
        [[gnu::always_inline]] constexpr ulng rem2(ulng xh, ulng xl, bool regs = false) const {
            if (__builtin_constant_p(xh | xl) && !(xh | xl)) { return 0; } // a literal zero: GCC 14 and 15 do not fold the correction for it
#ifdef __x86_64__
            if (regs && !std::is_constant_evaluated()) { ulng tl, th; __asm__("mulq %3" : "=a"(tl), "=d"(th) : "%a"(v), "rm"(xh) : "cc"); return rem2(xh, xl, tl, th); }
#endif
            ulll t = ulll(v) * xh; return rem2(xh, xl, ulng(t), ulng(t >> 64));}
        // a * b % m for a < m. regs: both products by mulq, whose halves stay in registers; for a divisor read from memory GCC spills
        // the C++ products to the stack, while for a compile-time divisor it folds them and may move them out of a loop.
        [[gnu::always_inline]] constexpr ulng mul(ulng a, ulng b, bool regs = false) const {
#ifdef __x86_64__
            if (regs && !std::is_constant_evaluated()) {
                ulng xl, xh, tl, th;
                __asm__("mulq %3" : "=a"(xl), "=d"(xh) : "%a"(a << sh), "rm"(b) : "cc");
                __asm__("mulq %3" : "=a"(tl), "=d"(th) : "%a"(v), "rm"(xh) : "cc");
                return rem2(xh, xl, tl, th) >> sh;}
#endif
            ulll x = ulll(a << sh) * b; return rem2(ulng(x >> 64), ulng(x)) >> sh;}
        [[gnu::always_inline]] constexpr ulng rem(ulng r, ulng lo, bool regs = false) const { return rem2(r << sh | (lo >> 1) >> (63 - sh), lo << sh, regs) >> sh; } // (r * 2^64 + lo) % m, r < m
    };

    // S: O(log(M)), Q: O(1), M: O(1); red takes any double-width dividend, mul canonical factors.
    template<typename T, T M, int ID, bool STATIC = (M != 0)>
    struct Context {
        static_assert(M > 0);
        using Word = T;
        using Wide = std::conditional_t<std::is_same_v<T, uint>, ulng, ulll>;
        static constexpr T MOD = M;
        static constexpr bool is_prime = modint_detail::isPrime(M);
        static constexpr Montgomery64 montgomery{M | 1};
        static constexpr Reciprocal reciprocal{M};
        static constexpr ulng BIAS = biasOf(M);
        static constexpr ulng B32 = ulng(M) >> 31 ? 0 : ((ulng(1) << 31) - 1) / M * M + M; // least multiple of M from 2^31
        static constexpr ulng PM32 = ulng(M) < 2 ? 0 : ulng(((ulll(1) << 64) + M - 1) / M); // ceil(2^64 / M)
        // 2^FK = FC (mod M): a fold modulus has a 32-bit FC whose second fold cannot overflow.
        static constexpr int FK = std::bit_width(ulng(M));
        static constexpr ulng FC = (ulng(2) << (FK - 1)) - ulng(M);
        static constexpr bool FOLD = ulng(M) >> 32 != 0 && FC <= 0xFFFFFFFF && (FK == 64 || ulll(FC + 1) * (FC + 1) <= M);
        // Products of residues are below M^2: for M < 2^31 with a long reciprocal the round-up one at 2^(64 + PS) is exact on them.
        static constexpr bool EXACT = ulng(M) >> 31 == 0 && !std::has_single_bit(M) && !shortReciprocal(M);
        static constexpr int PS = max(std::bit_width(ulng(M)), 2) - 2;
        static constexpr ulng PM = ulng(((ulll(1) << (64 + PS)) + M - 1) / M);
        static constexpr bool LONG = ulng(M) >> 32 != 0 && !std::has_single_bit(M) && M != Barrett64::MERSENNE61 && M != 0xFFFFFFFF00000001 && !FOLD; // prod is reciprocal.mul

        static constexpr bool isPrime() { return is_prime; }
        static constexpr T canon(Wide y) { // y < 2 * M; branch-free: the difference wraps unless y >= M
            if constexpr (M <= std::numeric_limits<T>::max() / 2) { return min(T(y), T(T(y) - M)); }
            else { return T(min(y, Wide(y - M))); }}
        [[gnu::always_inline, gnu::flatten]] static constexpr T red(Wide x) {
            if constexpr (M == Barrett64::MERSENNE61) { return canon(Barrett64::fold61(Barrett64::fold61(x))); } // <= M + 64
            else if constexpr (std::is_same_v<T, uint> || std::has_single_bit(M) || unitShift(M)) { return T(x % M); } // GCC's own sequence: no division
            else if constexpr (M >> 32 == 0) { constexpr ulng C64 = ulng((ulll(1) << 64) % M); return T(((ulng(x >> 64) % M) * C64 + ulng(x) % M) % M); }
            else { return reciprocal.rem(ulng(x >> 64) % M, ulng(x), true); }}
        // A product with a compile-time operand tells GCC it is below M, as its own remainder does: the sums after it fold.
        template<typename R> [[gnu::always_inline]] static constexpr R below(R r, R a, R b) {
            if ((__builtin_constant_p(a) || __builtin_constant_p(b)) && r >= M) { __builtin_unreachable(); }
            return r;}
        [[gnu::always_inline, gnu::flatten]] static constexpr T mul(T a, T b) { return below(prod(a, b), a, b); }
        [[gnu::always_inline, gnu::flatten]] static constexpr T prod(T a, T b) {
            if constexpr (M == Barrett64::MERSENNE61) { return canon(Barrett64::fold61(Wide(a) * b)); } // < 2 * M
            else if constexpr (std::has_single_bit(M)) { return T(a * b & (M - 1)); }
            else if constexpr (EXACT) { ulng x = ulng(a) * b; return T(x - (ulng(ulll(x) * PM >> 64) >> PS) * M); }
            else if constexpr (std::is_same_v<T, uint>) { return red(Wide(a) * b); }
            else if constexpr (M >> 32 == 0) { return a * b % M; }
            else if constexpr (M == 0xFFFFFFFF00000001) { // Goldilocks: 2^64 = 2^32 - 1 and 2^96 = -1
                ulll x = Wide(a) * b; ulng lo = ulng(x), hi = ulng(x >> 64), hh = hi >> 32, hl = uint(hi);
                ulng t = lo - hh; t -= lo < hh ? 0xFFFFFFFF : 0;
                ulng u = (hl << 32) - hl, r = t + u; r += r < u ? 0xFFFFFFFF : 0;
                return min(r, r - M);}
            else if constexpr (FOLD) {
                constexpr ulng MASK = ~ulng(0) >> (64 - FK);
                ulll x = Wide(a) * b, y = ulll(ulng(x >> FK)) * FC + (ulng(x) & MASK);
                ulng z = ulng(y >> FK) * FC, r = (ulng(y) & MASK) + z;
                if constexpr (FK == 64) { r += r < z ? FC : 0; }
                return min(r, r - M);}
            else { return reciprocal.mul(a, b); }}
        [[gnu::always_inline]] static constexpr ulng mulWide(ulng a, ulng b) requires std::is_same_v<T, uint> { // a b mod M in a 64-bit register
            ulng x = a * b;
            if constexpr (EXACT) { return below(x - (ulng(ulll(x) * PM >> 64) >> PS) * M, a, b); } else { return x % M; }}
        // (hi * 2^64 + lo) % M for 32-bit words; kept out of line: inlined, GCC spills the caller's accumulator.
        [[gnu::noinline]] static constexpr T fold(ulng hi, ulng lo) requires std::is_same_v<T, uint> {
            constexpr ulng C64 = ulng((ulll(1) << 64) % M);
            return T(hi ? ((hi % M) * C64 + lo % M) % M : lo % M);}
    };

    // S: O(log(m)), O(1) with prm = 0; M: O(1) per ID and width.
    // setMod stales every old value even for the same m; prm -1 detects, 0 clears, 1 promises a prime unchecked.
    template<typename T, T M, int ID>
    struct Context<T, M, ID, false> {
        using Word = T;
        using Wide = std::conditional_t<std::is_same_v<T, uint>, ulng, ulll>;
        using Reducer = std::conditional_t<std::is_same_v<T, uint>, Barrett32, Barrett64>;
        // Each word the kernels read sits alone in a struct: a residue is stored as a plain word, which may change a plain static of its
        // type, so GCC would load that again after every store. MOD and BIAS mirror mod and bias for inspection.
        template<class U> struct Held { U v; };
        static inline constinit Held<ulng> minv{0}, minvOf{0}; // minv = minvOf^-1 mod 2^64: a 32-bit word's inverse builds it for an odd modulus, setMod leaves it
        static inline constinit Held<T> mod{998244353}, t64{T((ulll(1) << 64) % 998244353)}, t31{T((ulng(1) << 31) % 998244353)}; // 2^64 mod MOD; 2^31 mod MOD: 32-bit words
        static inline constinit Held<ulng> bias{biasOf(998244353)}, mthr{POW_SMALL}; // mthr: pow leaves the binary loop from this exponent
        // A 32-bit word: floor((2^64 - 1) / MOD), and 2^64 / MOD for a power of two from 2, where lazy is then exact.
        // A 64-bit word: 2 for a power of two, 1 for Mersenne61, 0 for any other modulus from 2^32, else floor((2^64 - 1) / MOD).
        static inline constinit Held<ulng> small{~ulng(0) / 998244353}, quot{~ulng(0) / 998244353}; // quot: floor((2^64 - 1) / MOD), 64-bit words
        static inline constinit T MOD = 998244353;
        static inline constinit bool is_prime = true;
        static inline constinit Reducer reducer{998244353};
        static inline constinit Montgomery64 montgomery{998244353};
        static inline constinit Reciprocal reciprocal{998244353};
        static inline constinit ulng BIAS = biasOf(998244353);
        static inline constinit bool sel = false; // (2^64 mod MOD) MOD >= 2^62: a sixteenth of random products and more need the correction
        static inline constinit bool mont = false; // pow by Montgomery products: odd, from 2^32, not Mersenne61
        static inline constinit bool selc = true; // (2^64 mod MOD) >= MOD / 8: a 64-bit source needs the correction often enough for a select

        static Reducer &reduction() { return reducer; }
        static bool isPrime() { return modint_detail::isPrime(mod.v, t64.v); }
        static void setMod(T m, int prm = -1) {
            assert(m && -1 <= prm && prm <= 1);
            reducer = Reducer(m); MOD = mod.v = m;
            mont = (m & 1) && ulng(m) != Barrett64::MERSENNE61 && ulng(m) >> 32 != 0;
            mthr.v = ulng(m) >> 32 == 0 || reducer.mask ? POW_SMALL : ulng(m) == Barrett64::MERSENNE61 ? POW_TRACK61 : m & 1 ? POW_MONT : ~ulng(0);
            ulng q; // floor((2^64 - 1) / m) from the reducer's reciprocal: setMod divides once
            if constexpr (std::is_same_v<T, ulng>) {
                ulll mu = reducer.mask ? ~ulll(0) >> std::countr_zero(m) : m == Barrett64::MERSENNE61 ? ~ulll(0) / Barrett64::MERSENNE61 : reducer.mu;
                montgomery = Montgomery64(m | 1); reciprocal = Reciprocal(m, mu); quot.v = q = ulng(mu >> 64);
                small.v = reducer.mask ? 2 : m == Barrett64::MERSENNE61 ? 1 : m >> 32 ? 0 : q;}
            else { q = reducer.mask ? ~ulng(0) >> std::countr_zero(m) : reducer.mu; small.v = q + (reducer.mask && m > 1); }
            ulng w = 0 - q * m; // 2^64 mod m, and m itself for a power of two
            BIAS = bias.v = (q / 2 + 1) * m; sel = !reducer.mask && ulng(m) >> 32 == 0 && ulng(ulll(w) * m >> 62) != 0;
            t64.v = T(w == m ? 0 : w);
            if constexpr (std::is_same_v<T, uint>) { selc = ulng(t64.v) >= ulng(m) / 8; t31.v = rem32(uint(1) << 31); }
            is_prime = prm == -1 ? isPrime() : bool(prm);}
        static uint rem32(uint v) { return uint(ulll((small.v + 1) * v) * mod.v >> 64); } // v % MOD for a 32-bit v (Lemire-Kaser-Kurz)
        // MOD < 2^32: lazy(x) is x mod MOD or that plus MOD, by the one-word reciprocal. The correction is a jump the predictor learns, or a
        // select where it is not rare: any value (redSel), a product of residues under sel (red), a constructed value under selc (redCtor).
        [[gnu::always_inline]] static ulng lazy(ulng x) { return x - ulng(ulll(x) * small.v >> 64) * mod.v; }
        [[gnu::always_inline]] static ulng jump(ulng r) { if (r >= mod.v) [[unlikely]] { __asm__("" : "+r"(r)); r -= mod.v; } return r; }
        [[gnu::always_inline]] static ulng red64(ulng x) { ulng r = lazy(x); if (sel) [[unlikely]] { return min(r, r - mod.v); } return jump(r); }
        [[gnu::always_inline, gnu::flatten]] static T red(Wide x) {
            if constexpr (std::is_same_v<T, uint>) {
                return uint(red64(x));}
            else {
                if (small.v - 1 < 2) { return reducer.reduce(x); } // a power of two or Mersenne61: a mask or two folds
                ulng hi = ulng(x >> 64);
                if (hi >= mod.v) { hi = rem64(hi); }
                return reciprocal.rem(hi, ulng(x));}}
        [[gnu::always_inline]] static ulng rem64(ulng u) { ulng r = u - ulng(ulll(u) * quot.v >> 64) * mod.v; return min(r, r - mod.v); } // u % MOD: the quotient is at most one short
        [[gnu::always_inline]] static ulng mulWide(ulng a, ulng b) { return red64(a * b); } // a b mod MOD for a, b < MOD, in a 64-bit register
        [[gnu::always_inline, gnu::flatten]] static uint redSel(ulng x) { ulng r = lazy(x); return uint(min(r, r - mod.v)); }
        [[gnu::always_inline, gnu::flatten]] static uint redJmp(ulng x) { return uint(jump(lazy(x))); }
        [[gnu::always_inline, gnu::flatten]] static uint redCtor(ulng x) { ulng r = lazy(x); return uint(selc ? min(r, r - mod.v) : jump(r)); }
        [[gnu::always_inline, gnu::flatten]] static T mul(T a, T b) {
            if constexpr (std::is_same_v<T, uint>) { return red(Wide(a) * b); }
            else if (ulng mu = small.v; !mu) { return reciprocal.mul(a, b, true); }
            else if (mu <= 2) {
                if (mu == 1) { return Context<T, Barrett64::MERSENNE61, 0>::prod(a, b); }
                ulng r = a * b & (mod.v - 1); __asm__("" : "+r"(r)); return r;} // opaque: else GCC 14 to 16 compile the sums after it as jumps
            else { ulng x = a * b, r = x - ulng(ulll(x) * mu >> 64) * mod.v; return min(r, r - mod.v); }}
    };

    // S, Q, M as Context; residues in Montgomery form, x 2^64 mod m, under the product mmul.
    // An odd modulus; a dynamic one is the state of Context<ulng, 0, ID>: setMod of either type sets both.
    template<ulng M, int ID>
    struct MontContext : Context<ulng, M, ID> {
        using Base = Context<ulng, M, ID>;
        static void setMod(ulng m, int prm = -1) requires (M == 0) { assert(m & 1); Base::setMod(m, prm); }
        [[gnu::always_inline]] static constexpr ulng mul(ulng a, ulng b) { // a b < m 2^64: one factor may be any word
            if constexpr (M != 0) { return mmul(a, b, 0 - Base::montgomery.inv, M); } else { return mmul(a, b, 0 - Base::montgomery.inv, Base::mod.v); }}
        static constexpr ulng cube() { return mul(Base::montgomery.rsq, Base::montgomery.rsq); } // 2^192 mod m
        static constexpr ulng one() { if constexpr (M != 0) { return ulng((ulll(1) << 64) % M); } else { return Base::t64.v; } } // the form of one
    };

    // S: O(N * log(MOD)) at compile time, Q: O(1), M: O(N); the words of the inverses below N of a static modulus, 0 where there is none.
    template<typename V, int N> struct InvTable { static constexpr auto value = V::template inverses<N>(); };
    // S: O(sqrt-table size * log(MOD)) at compile time, Q: O(1), M: O(table); the square-root tables of a static prime modulus.
    template<typename V> struct SqrtData { static constexpr auto value = V::sqrtStatic(); };

    // S: O(1), Q: O(1), M: O(1); operation bounds below.
    // Canonical n in [0, mod); inv needs a unit (tryInv returns false); sqrt returns -1 without a root.
    template<typename C>
    struct Value : C {
        using Word = typename C::Word;
        using Wide = typename C::Wide;
        static inline constinit ulng last = 0, rootLast = 0; // the previous exponent below 2^64 of pow and of a dynamic root power
        static inline constinit std::array<std::array<Word, 2>, INV_TABLE> cache{}; // a dynamic modulus: per value below INV_TABLE, the modulus its inverse was asked under and that word
        Word n;

        static constexpr Word mod() { if constexpr (Fixed<C>) { return C::MOD; } else { return C::mod.v; } }
        static constexpr Word wrap(Word x) { return Word(x & Word(mod() - 1)); } // x mod a power of two
        // The representation of one, the canonical residue of a representation, and the value of a canonical residue.
        static constexpr Word unit() { if constexpr (Form<C>) { return C::one(); } else { return mod() != 1; } }
        static constexpr Word plain(Word w) { if constexpr (Form<C>) { return C::mul(w, 1); } else { return w; } }
        static constexpr Value from(Word a) { if constexpr (Form<C>) { return at(C::mul(a, C::montgomery.rsq)); } else { return at(a); } }
        template<modint_detail::Integer T> static constexpr Word norm(T a) {
            using U = std::conditional_t<sizeof(T) <= 4, uint, ulng>;
            if constexpr (PowerOfTwo<C>) { return wrap(Word(a)); }
            if constexpr (Form<C>) { // one product by 2^128 mod m: any 64-bit factor keeps it below m 2^64
                if constexpr (sizeof(T) == 16) { // hi 2^64 + lo by two products; a negative hi counted 2^64 too much
                    ulng hi = ulng(ulll(a) >> 64), c = C::cube(); Value r = at(C::mul(ulng(a), C::montgomery.rsq)) + at(C::mul(hi, c));
                    if constexpr (std::is_same_v<T, lll>) { r -= at(c & (0 - (hi >> 63))); }
                    return r.n;}
                else if constexpr (!std::is_signed_v<T>) { return C::mul(ulng(a), C::montgomery.rsq); }
                else if constexpr (Fixed<C>) { return C::mul(ulng(lng(a)) + (C::BIAS & ulng(lng(a) >> 63)), C::montgomery.rsq); }
                else { return C::mul(ulng(lng(a)) + (C::bias.v & ulng(lng(a) >> 63)), C::montgomery.rsq); }}
            if constexpr (!Fixed<C> && sizeof(T) <= 4 && std::is_same_v<Word, ulng>) { // a modulus from 2^32 exceeds |a|, a power of two too
                if (!__builtin_constant_p(a) && mod() >> 32) [[likely]] { return ulng(lng(a)) + (mod() & ulng(lng(a) >> 63)); }}
            if constexpr (!Fixed<C> && (sizeof(T) <= 4 || std::is_same_v<Word, ulng>)) { // a dynamic power of two, where the test costs the other moduli nothing
                if (!__builtin_constant_p(a) && C::reducer.mask) [[unlikely]] { return wrap(Word(a)); }}
            if constexpr (sizeof(T) == 16 && (!Fixed<C> || C::MOD == Barrett64::MERSENNE61 || !unitShift(C::MOD))) {
                ulng hi = ulng(ulll(a) >> 64), lo = ulng(a), g = 0; // by halves; a negative value is the complement of a non-negative one
                if constexpr (std::is_same_v<T, lll>) { g = ulng(lng(hi) >> 63); }
                hi ^= g; lo ^= g; Word r, k = Word(g);
                if constexpr (!Fixed<C> && std::is_same_v<Word, ulng>) {
                    if (hi >= mod()) { hi = C::rem64(hi); }
                    r = C::reciprocal.rem(hi, lo);}
                else if constexpr (!Fixed<C>) { // (hi mod m) (2^64 mod m) + lo mod m, the last up to 2 m - 1: at most m^2 < 2^64
                    if (C::selc) [[likely]] { r = C::redSel(ulng(C::redSel(hi)) * C::t64.v + C::lazy(lo)); }
                    else { r = C::redJmp(ulng(C::redJmp(hi)) * C::t64.v + C::lazy(lo)); }}
                else if constexpr (std::is_same_v<Word, uint>) { r = C::fold(hi, lo); }
                else { r = C::red(ulll(hi) << 64 | lo); }
                return Word((r ^ k) + (k & mod()));}
            else if constexpr (std::is_same_v<T, lll>) { lll r = a % lll(mod()); return Word(r < 0 ? r + mod() : r); }
            else if constexpr (std::is_same_v<T, ulll>) { return Word(a % mod()); }
            else if constexpr (sizeof(T) <= 4 && Fixed<C> && ulng(C::MOD) >> 31 == 0 && C::MOD > 1 && ulll(C::MOD) >> (8 * sizeof(T) - std::is_signed_v<T>) == 0) {
                ulng x = std::is_signed_v<T> ? ulng(lng(a)) + C::B32 : ulng(a); // the type's range exceeds the modulus
                return Word(x - ulng(ulll(x) * C::PM32 >> 64) * mod());}
            else if constexpr (sizeof(T) <= 4 && !Fixed<C> && std::is_same_v<Word, uint>) {
                if constexpr (!std::is_signed_v<T>) { return C::rem32(uint(a)); }
                else if (__builtin_constant_p(a >= 0) && a >= 0) { return C::rem32(uint(a)); } // known non-negative, a literal above all
                else { uint r = C::rem32(uint(a) ^ (uint(1) << 31)), c = C::t31.v; return r - c + (r < c ? mod() : 0); }} // a + 2^31 first
            else if constexpr (!std::is_signed_v<T>) {
                if constexpr (std::is_same_v<Word, uint> && !Fixed<C>) { return C::redCtor(U(a)); }
                else if constexpr (std::is_same_v<Word, uint>) { return C::red(U(a)); }
                else if constexpr (!Fixed<C> && sizeof(T) <= 4) { // a, mod() < 2^32: the remainder by two products (Lemire, Kaser, Kurz), exact for 1 and powers of two too
                    if (__builtin_constant_p(a) || mod() >> 32) { return C::rem64(U(a)); }
                    return ulng(ulll((C::quot.v + 1) * U(a)) * mod() >> 64);}
                else if constexpr (!Fixed<C>) { return C::rem64(U(a)); }
                else { return Word(U(a) % mod()); }}
            else if constexpr (Fixed<C> && ulll(C::MOD) >> (8 * sizeof(T) - 1)) { // |a| <= mod()
                return Word(Word(a) + (mod() & Word(a < 0 ? -1 : 0)));}
            else if constexpr (!Fixed<C>) { // biased only below zero: a small dividend keeps the reducer's correction rare
                ulng u = ulng(lng(a)) + (C::bias.v & ulng(lng(a) >> 63));
                if constexpr (std::is_same_v<Word, uint>) { return C::redCtor(u); }
                else { return C::rem64(u); }}
            else if constexpr (std::has_single_bit(ulng(C::MOD) + 1) && !shortReciprocal(C::MOD)) { // 2^k - 1: the signed remainder is the shorter sequence
                lng r = a % lng(mod()); return Word(r) + (mod() & Word(r >> 63));}
            else { return Word((ulng(a) + (C::BIAS & ulng(lng(a) >> 63))) % mod()); }} // biased only below zero: no wrap, no branch, so a loop hoists it

        constexpr Value() : n(0) {} // zero needs no reduction: a dynamic context cannot fold norm(0)
        template<modint_detail::Integer T> constexpr Value(T a) : n(norm(a)) {}
        static constexpr Value raw(Word a) { assert(a < mod()); return from(a); }
        static constexpr Value at(Word a) { Value r; r.n = a; return r; } // the representation as it is: a table entry, a chain's word

        constexpr Word val() const { return plain(n); }
        template<modint_detail::Integer T> explicit constexpr operator T() const {
            Word v = val(); assert(ulll(v) <= ulll(std::numeric_limits<T>::max())); return T(v);}
        explicit constexpr operator bool() const { return n != 0; }

        constexpr Value &operator++() {
            if constexpr (PowerOfTwo<C>) { n = wrap(n + 1); }
            else if constexpr (Form<C>) { *this += at(unit()); }
            else if constexpr (Fixed<C> && sizeof(Word) == 4 && !AVX2) { n = n >= mod() - 1 ? n - (mod() - 1) : n + 1; }
            else if constexpr (sizeof(Word) == 8) { n = n == mod() - 1 ? 0 : n + 1; }
            else { Word v = n + 1; n = v == mod() ? 0 : v; }
            return *this;}
        constexpr Value &operator--() { // a select where the modulus is in memory, GCC's jump-free form where it is a constant
            if constexpr (PowerOfTwo<C>) { n = wrap(n - 1); }
            else if constexpr (Form<C>) { Word u = unit(), d = n - u; n = n < u ? d + mod() : d; }
            else if constexpr (sizeof(Word) == 8) { n = (n ? n : mod()) - 1; }
            else if constexpr (Fixed<C>) { Word d = n - 1; n = d + (mod() & (0 - Word(n == 0))); }
            else { Word d = n - 1; n = d < mod() ? d : mod() - 1; }
            return *this;}
        constexpr Value operator++(int) { Value a = *this; ++*this; return a; }
        constexpr Value operator--(int) { Value a = *this; --*this; return a; }
        constexpr Value &operator+=(Value a) {
            if constexpr (PowerOfTwo<C>) { n = wrap(n + a.n); } else { n = n >= mod() - a.n ? n - (mod() - a.n) : n + a.n; }
            return *this;}
        constexpr Value &operator-=(Value a) { // the borrow as a mask: no jump
            if constexpr (PowerOfTwo<C>) { n = wrap(n - a.n); } else { Word d = n - a.n; n = d + (mod() & -Word(n < a.n)); }
            return *this;}
        [[gnu::always_inline]] constexpr Value &operator*=(Value a) { n = C::mul(n, a.n); return *this; }
        constexpr Value &operator/=(Value a) { return *this *= inv(a); }

        constexpr Value operator+() const { return *this; }
        constexpr Value operator-() const { // zero stays zero by a select, never a jump: zeros arrive unpredictably
            Value r; Word v = mod() - n;
            if constexpr (PowerOfTwo<C>) { r.n = wrap(v); }
            else if constexpr (AVX2 && sizeof(Word) == 8) { r.n = min(v, Word(0 - n)); }
            else if constexpr (Fixed<C> && sizeof(Word) == 4) { r.n = v - (mod() & (0 - Word(n == 0))); }
            else { r.n = n ? v : 0; }
            return r;}
        friend constexpr Value operator+(Value a, Value b) { return a += b; }
        friend constexpr Value operator-(Value a, Value b) { return a -= b; }
        [[gnu::always_inline]] friend constexpr Value operator*(Value a, Value b) { return a *= b; }
        friend constexpr Value operator/(Value a, Value b) { return a /= b; }
        friend constexpr bool operator==(Value a, Value b) { return a.n == b.n; }
        friend constexpr auto operator<=>(Value a, Value b) { return a.val() <=> b.val(); }

        template<int N> static constexpr std::array<Word, N> inverses() { // InvTable's words, by Euclid
            std::array<Word, N> t{};
            for (Word i = 1; i < Word(N) && i < mod(); ++i) {
                Word r = mod(), s = i, x = 0, y = 1; bool pos = false;
                while (s) { Word q = r / s; r = std::exchange(s, Word(r % s)); x = std::exchange(y, Word(x + q * y)); pos = !pos; }
                if (r == 1) { t[i] = raw(pos ? x : mod() - x).n; }}
            return t;}
        // T: O(log(mod())), M: O(1); a^-1 for a >= 2 by subtraction and shifts on b under the odd w: a under mod(), or for EVEN, an even
        // mod() and an odd a, mod() under a: from y = mod()^-1 mod a the inverse is mod() - (mod() y - 1) / a, the division exact.
        template<bool EVEN> [[gnu::noinline]] static constexpr bool invBinary(Word a, Value &out) { // p b = s u 2^k, q b = -s v 2^k (mod w) and p v + q u = w, so p, q <= w
            ulng w = EVEN ? a : mod(), b = EVEN ? mod() : a;
            if constexpr (EVEN) { if (std::countl_zero(a) - std::countl_zero(mod()) >= INV_SKEW) { b = mod() % a; if (!b) { return false; } } } // else the loop's first steps shorten it
            int k = std::countr_zero(b), n = 0; ulng u = b >> k, v = w, p = 1, q = 0; // n counts the swaps: s = (-1)^n
            if (std::countl_zero(u) - std::countl_zero(v) >= INV_SKEW) { // one division, v = q u + r, for about half as many steps as bits
                ulng r;
                if (v >> 32) { q = v / u; r = v % u; } else { q = uint(v) / uint(u); r = uint(v) % uint(u); }
                if (!r) { if (u != 1) { return false; } v = 1; }
                else { int z = std::countr_zero(r); v = r >> z; p <<= z; k += z; }}
#ifdef __x86_64__
            if (!std::is_constant_evaluated() && u != v) { // the loop below with no copy between two values of u and no shift behind an addition's flags
                ulng d, t;
                __asm__("1: mov %[v], %[d]; sub %[u], %[d]; rep bsf %[d], %%rcx; mov %[u], %[t]; sub %[v], %[u]; cmovb %[d], %[u]; cmovb %[t], %[v]; mov %[p], %[t]; cmovae %[q], %[t]; adc $0, %[n]; lea (%[p],%[q]), %[p]; shr %%cl, %[u]; mov %[t], %[q]; shl %%cl, %[q]; add %%ecx, %[k]; cmp %[v], %[u]; jne 1b"
                    : [u] "+r" (u), [v] "+r" (v), [p] "+r" (p), [q] "+r" (q), [n] "+r" (n), [k] "+r" (k), [d] "=&r" (d), [t] "=&r" (t) : : "rcx", "cc");}
#endif
            while (u != v) { // both odd: the larger becomes the odd part of the difference
                ulng d = u - v, t = u < v ? p : q; int z = std::countr_zero(d);
                n += u < v; d = u < v ? 0 - d : d;
                v = min(u, v); u = d >> z; p += q; q = t << z; k += z;}
            if (u != 1) { return false; }
            ulng x = n & 1 ? q : p, m = w, ni; // x b = 2^k
            if constexpr (EVEN) {
                ni = inv64(m);
                if (sizeof(Word) == 8 && k > 64) { x = mmul(x, 1, ni, m); k -= 64; }
                if (k) { x = mmul(x, ulng(1) << (-k & 63), ni, m); }
                out = at(Word(mod() - (mod() * x - 1) * ni)); return true;}
            else if constexpr (!Fixed<C> && sizeof(Word) == 4) { // m^-1 mod 2^64, kept with its modulus
                if (C::minvOf.v != m) [[unlikely]] { C::minv.v = inv64(m); C::minvOf.v = m; }
                ni = C::minv.v;}
            else { ni = 0 - C::montgomery.inv; }
            if constexpr (Form<C>) { if (k > 64) { k -= 64; } else { x = mmul(x, C::montgomery.rsq, ni, m); } } // one factor of 2^64 more: the form
            else if (sizeof(Word) == 8 && k > 64) { x = mmul(x, 1, ni, m); k -= 64; }
            out = at(Word(mmul(x, ulng(1) << (-k & 63), ni, m))); return true;}
        // T: O(log(mod())), M: O(1); O(1) by Newton's iteration under a power of two and from a table for a value below INV_TABLE (a static
        // prime below it: every value), which a dynamic modulus fills with each unit's first inverse under it. Inline up to the loops.
        [[gnu::always_inline]] friend constexpr bool tryInv(Value a, Value &out) {
            if constexpr (Fixed<C>) {
                if constexpr (C::is_prime && C::MOD < INV_TABLE && !Form<C>) { if (!a.n) { return false; } out.n = InvTable<Value, int(C::MOD)>::value[a.n]; return true; }}
            Word s = a.val(); bool lift = PowerOfTwo<C>; int bits = 8 * int(sizeof(Word));
            if constexpr (PowerOfTwo<C>) { bits = std::countr_zero(C::MOD); }
            if constexpr (!Fixed<C> || C::MOD % 2 == 1) {
                if (mod() & 1) { if (s >= Word(INV_TABLE)) { return invBinary<false>(s, out); } }
                else if constexpr (!Fixed<C> && !Form<C>) { lift = C::reducer.mask; }}
            if (lift && (s & 1)) { out = at(wrap(Word(inv64(s, bits)))); return true; } // an odd value under a power of two
            if constexpr (PowerOfTwo<C> && C::MOD > 1) { return false; } // an even value
            else {
                if (s - 1 < Word(INV_TABLE - 1)) { // 1 <= s < INV_TABLE: its table, which invRest fills under a dynamic modulus
                    if constexpr (Fixed<C>) { Word t = InvTable<Value, INV_TABLE>::value[s]; if (t) { out.n = t; } return t != 0; }
                    else if (cache[s][0] == mod()) { out.n = cache[s][1]; return true; }}
                return invRest(s, out);}}
        // T: O(log(mod())), M: O(1); the rest of tryInv, out of line: a dynamic modulus's value below INV_TABLE, in O(1) below INV_TINY
        // (INV_STEP) and kept in the table; an even modulus; zero.
        [[gnu::noinline]] static constexpr bool invRest(Word a, Value &out) {
            Word m = mod();
            if constexpr (!Fixed<C>) {
                if (a - 1 < Word(INV_TABLE - 1)) {
                    if (a < Word(INV_TINY)) {
                        ulng q = ulng(ulll(m) * INV_RECIP[a] >> 64), e = m - q * a; bool up = e >= a; // q is one short at most
                        uint step = INV_STEP[a][up ? e - a : e];
                        if (step == 255) { return false; }
                        out = raw(Word((step & 15) * (q + up) + (step >> 4)));}
                    else if constexpr (Form<C>) { if (!invBinary<false>(a, out)) { return false; } }
                    else if (!(m & 1 ? invBinary<false>(a, out) : (a & 1) && invBinary<true>(a, out))) { return false; }
                    cache[a] = {m, out.n}; return true;}}
            if constexpr (!Form<C> && (!Fixed<C> || C::MOD % 2 == 0)) {
                if (!(m & 1)) { return (a & 1) && invBinary<true>(a, out); }} // an even modulus: a unit is odd
            if (m != 1) { return false; } // zero is left
            out = Value(); return true;}
        [[gnu::always_inline]] friend constexpr Value inv(Value a) {
            Value r; bool ok = tryInv(a, r); assert(ok); (void)ok; return r;}

        // T: O(log(f + 1)), M: O(1); a^f with no branch on f's bits, out of line, for an exponent that changed: powSelect, or base-4 digits
        // under a power of two. A 32-bit word and a dynamic modulus below 2^32 run on 64-bit registers and their own product.
        [[gnu::noinline, gnu::flatten]] static Value powBlind(Value a, ulng f) {
            auto run = [&](bool digits, auto one, auto x, auto mul) __attribute__((always_inline)) { return digits ? powDigits(one, x, f, mul) : powSelect(one, x, f, mul); };
            auto wide = [&](auto digits) __attribute__((always_inline)) {
                return at(Word(run(digits, ulng(mod() != 1), ulng(a.n), [](auto x, auto y) __attribute__((always_inline)) { return C::mulWide(x, y); })));};
            if constexpr (std::is_same_v<Word, uint> && !requires { requires C::EXACT; }) { return wide(PowerOfTwo<C>); }
            else if constexpr (!Fixed<C> && !Form<C>) { if (C::small.v > 2) { return wide(false); } }
            bool digits = PowerOfTwo<C>;
            if constexpr (!Fixed<C> && std::is_same_v<Word, ulng>) { digits = C::small.v == 2; }
            return run(digits, at(unit()), a, [](Value x, Value y) __attribute__((always_inline)) { return x * y; });}
        // T: O(1 + log(b + 1)), M: O(1); the binary loop. A 32-bit word runs it on 64-bit registers, which keeps zero extensions off
        // the squaring chain; a dynamic Mersenne61 and a large dynamic modulus run it on their own product, without the dispatch.
        [[gnu::always_inline]] static constexpr Value binary(Value a, auto b) {
            if constexpr (std::is_same_v<Word, uint>) {
                return at(Word(powBits(ulng(mod() != 1), ulng(a.n), b, [](ulng x, ulng y) __attribute__((always_inline)) { return C::mulWide(x, y); })));}
            else if constexpr (Form<C>) {} // its own product below
            else if constexpr (!Fixed<C>) {
                if (!std::is_constant_evaluated() && mod() == Barrett64::MERSENNE61) {
                    using M = Context<ulng, Barrett64::MERSENNE61, 0>;
                    return at(powBits(ulng(1), a.n, b, [](ulng x, ulng y) __attribute__((always_inline)) { return M::prod(x, y); }));}
                if (!std::is_constant_evaluated() && C::small.v == 0) {
                    return at(powBits(ulng(1), a.n, b, [](ulng x, ulng y) __attribute__((always_inline)) { return C::reciprocal.mul(x, y, true); }));}
                if (ulng k = mod() - 1; !std::is_constant_evaluated() && C::small.v == 2) {
                    return at(powBits(ulng(mod() != 1), a.n, b, [k](ulng x, ulng y) __attribute__((always_inline)) { return x * y & k; }));}}
            else if constexpr (requires { requires C::LONG; }) { // a dependent chain: nothing for GCC to fold or move out, so mulq
                if (!std::is_constant_evaluated()) {
                    return at(powBits(ulng(1), a.n, b, [](ulng x, ulng y) __attribute__((always_inline)) { return C::reciprocal.mul(x, y, true); }));}}
            return powBits(at(unit()), a, b, [](Value x, Value y) __attribute__((always_inline)) { return x * y; });}
        // T: O(1 + log(b + 1)), M: O(1); the binary loop of a dynamic 64-bit word, out of line, under a modulus below 2^32 (one-word
        // Barrett products, the correction a jump or under C::sel a select) or a power of two (a mask).
        [[gnu::noinline]] static Value powSmall(Value a, auto b) {
            ulng one = mod() != 1;
            return at(powBits(one, ulng(a.n), b, [](ulng x, ulng y) __attribute__((always_inline)) { return C::mulWide(x, y); }));}
        // T: O(1 + log(b + 1)), M: O(1); a^b outside the Direct fast path: the binary loop below the context's threshold, from
        // it Montgomery products, powSmall or powBlind. seen: the last exponent; one in [POW_TRACK, 2^64) that differs from it counts as changed.
        [[gnu::always_inline]] static constexpr Value power(Value a, auto b, ulng *seen = nullptr) {
            if constexpr (!Direct<C>) {
                if (!std::is_constant_evaluated()) {
                    auto changed = [&]() __attribute__((always_inline)) { return seen && !hi64(b) && ulng(b) >= POW_TRACK && ulng(b) != std::exchange(*seen, ulng(b)); };
                    auto mont = [&](bool select) __attribute__((always_inline)) {
                        return at(select ? powMont<true, Form<C>>(C::montgomery, a.n, ulng(b)) : powMont<false, Form<C>>(C::montgomery, a.n, b));};
                    if constexpr (Form<C>) { if (atLeast(b, POW_MONT)) { return mont(changed()); } }
                    else if constexpr (!Fixed<C>) {
                        if (atLeast(b, C::mthr.v)) {
                            bool select = changed();
                            if (C::small.v > 2) { if (!select) { return powSmall(a, b); } }
                            else if (C::mont) { return mont(select); }
                            if (select) { return powBlind(a, ulng(b)); }}}
                    else if constexpr (ulng(C::MOD) >> 32 == 0) {
                        if (atLeast(b, POW_TRACK)) {
                            bool select = changed();
                            if constexpr (C::MOD & 1) { if (atLeast(b, select ? POW_FIXED_CHANGED : POW_FIXED)) { return mont(select); } }
                            if (select) { return powBlind(a, ulng(b)); }}}
                    else if constexpr ((C::MOD & 1) && C::MOD != Barrett64::MERSENNE61) {
                        if (atLeast(b, ulng(C::MOD) >> 63 ? POW_MONT63 : POW_MONT)) { return mont(changed()); }}}}
            return binary(a, b);}
        // T: O(1 + log(|e| + 1)), plus O(log(mod())) for e < 0; M: O(1).
        // Direct contexts: an exponent in [POW_TRACK, 2^64) that differs from the previous call's takes powBlind, its bits being unlearned.
        template<modint_detail::Integer T> [[gnu::always_inline]] friend constexpr Value pow(Value a, T e) {
            std::conditional_t<(sizeof(T) > 8 || Fixed<C>), ulll, ulng> b = e; // a dynamic context runs a 64-bit exponent in one word
            if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
                if (e < 0) { a = inv(a); b = -b; }}
            if constexpr (Direct<C>) {
                if (!std::is_constant_evaluated() && !hi64(b) && ulng(b) >= POW_TRACK && ulng(b) != std::exchange(last, ulng(b))) { return powBlind(a, ulng(b)); }}
            return power(a, b, &last);}

        // T: O(log(mod())^2) expected, M: O(1); tab[k] = z^(q 2^k), of order 2^(s - k), for a nonresidue z of the prime mod() = q 2^s + 1, s >= 2.
        static constexpr std::array<Word, 64> sqrtRoots(int s) {
            std::array<Word, 64> tab{};
            for (Word z = 3; s && plain(tab[s - 1]) < 2; z += 2) { // the symbol filters z below 512; the last entry decides
                if (z < 512 && jacobi(mod() % z, z) != -1) { continue; }
                ulng none = 0; Value c = rootPower(raw(z), (mod() - 1) >> s, &none);
                for (int k = 0; k < s; ++k, c *= c) { tab[k] = c.n; }}
            return tab;}
        // Tables of the 2-group <R> of order 2^s, R = sqrtRoots(s)[0], in K windows of b = min(s, 8) bits; window j has weight
        // 2^(o + 8 j), o = s - 8 (K - 1) - b <= 0. slot: open addressing over 2^SLACK slots a key, a key before any free slot.
        template<int K, int N> struct SqrtTable {
            static constexpr int SLACK = 5; // slot[(g mult) >> (digits - b - SLACK)] = i for g = h^-i: mult separates the 2^b keys, or ok is false
            std::array<std::array<Word, N>, K> half; std::array<std::array<Word, N>, K - 1> diag;
            Word mult; std::array<uint8_t, N << SLACK> slot; bool ok;
        };
        // T: O(K * 2^b + log(mod())^2), M: O(1); half[j][v] = R^(v 2^(o + 8 j - 1)), diag[d - 1][v] = R^(v 2^(s - 8 (d + 1))), and
        // key[i] = h^-i for h = R^(2^(s - b)) of order 2^b: g = key[i] iff g h^i = 1.
        template<int K, int N> static constexpr void sqrtBuild(SqrtTable<K, N> &r, int s) {
            auto tab = sqrtRoots(s); int b = min(s, 8), k = (s - b + 7) / 8 + 1, o = s - 8 * (k - 1) - b, n = 1 << b;
            auto fill = [&](std::array<Word, N> &row, int e) { // row[v] = R^(v 2^e), for e < 0 at the multiples of 2^-e
                Value g = at(tab[max(e, 0)]), c = at(unit());
                for (int v = 0; v < n; v += 1 << max(-e, 0), c *= g) { row[v] = c.n; }};
            for (int d = 1; d < k; ++d) { fill(r.diag[d - 1], s - 8 * (d + 1)); }
            for (int j = 0; j < k; ++j) { fill(r.half[j], o + 8 * j - 1); }
            Value h = at(tab[s - b]), c = at(unit()); std::array<Word, N> key{}; std::array<int, N << r.SLACK> seen{};
            for (int i = 0; i < n; ++i, c *= h) { key[(n - i) & (n - 1)] = c.n; }
            int sh = std::numeric_limits<Word>::digits - b - r.SLACK, i = 0, cap = std::is_constant_evaluated() ? 1 << 16 : 1 << 10; ulng m = 0;
            for (int trial = 1; i < n && trial <= cap; ++trial) { // about 55 candidates expected at 2^b = 256
                m = m * 6364136223846793005 + 1442695040888963407; r.mult = Word(m >> (64 - std::numeric_limits<Word>::digits)) | 1;
                for (i = 0; i < n; ++i) {
                    Word j = Word(key[i] * r.mult) >> sh;
                    if (std::exchange(seen[j], trial) == trial) { break; }
                    r.slot[j] = uint8_t(i);}}
            r.ok = i == n;}
        static constexpr auto sqrtStatic() {
            constexpr int S = std::countr_zero(Word(C::MOD - 1)), B = S < 8 ? S : 8;
            SqrtTable<(S - B + 7) / 8 + 1, 1 << B> r{}; sqrtBuild(r, S); return r;}
        // T: O(log(mod())), M: O(1); a root of a != 0 from the tables, 0 for a nonresidue: 8 (k - 1) squarings after the power.
        template<int K, int N> static constexpr Value sqrtWindows(const SqrtTable<K, N> &t, int s, Value a) {
            int b = min(s, 8), k = (s - b + 7) / 8 + 1, o = s - 8 * (k - 1) - b, i[8]{};
            Value w = rootOdd(a, s), x = a * w, p[8]; p[k - 1] = x * w;
            for (int j = k - 1; j-- > 0;) { p[j] = p[j + 1]; for (int e = 0; e < 8; ++e) { p[j] *= p[j]; } }
            for (int j = 0; j < k; ++j) {
                Value u = p[j];
                for (int d = 1; d <= j; ++d) { u *= at(t.diag[d - 1][i[j - d]]); }
                i[j] = t.slot[Word(u.n * t.mult) >> (std::numeric_limits<Word>::digits - b - t.SLACK)];
                if (!j && (i[0] >> -o & 1)) { return 0; }
                x *= at(t.half[j][i[j]]);}
            return x;}
        // The dynamic modulus's square-root state: roots for Tonelli-Shanks, and from the 257th call on one modulus the tables.
        struct SqrtCache {
            Word mod = 0; int calls = 0; bool rooted = false, wide = false;
            std::array<Word, 64> roots{}; SqrtTable<std::is_same_v<Word, uint> ? 4 : 8, 256> table{};
        };
        static inline constinit SqrtCache sqrtCache{};
        static inline constinit int sqrtRun = 0; // non-residues in a row, capped at 8: from there a large modulus asks the symbol first
        [[gnu::noinline]] static constexpr Value rootPowerOut(Value a, ulng b) { return power(a, b); }
        // The power of a square root; out of line on Direct contexts, whose caller's code it would displace. A dynamic context tracks
        // the exponent in rootLast, apart from pow's (seen: the caller's instead): a new modulus brings a new one, whose bits are unlearned.
        [[gnu::always_inline]] static constexpr Value rootPower(Value a, ulng b, ulng *seen = nullptr) {
            if constexpr (Fixed<C>) { if constexpr (Direct<C>) { return rootPowerOut(a, b); } else { return power(a, b); } }
            else if constexpr (Direct<C>) { return b != std::exchange(seen ? *seen : rootLast, b) ? powBlind(a, b) : rootPowerOut(a, b); }
            else { return power(a, b, seen ? seen : &rootLast); }}
        // x^(2^K - 1) on a product: f(2 a) = f(a)^(2^a) f(a), f(a + 1) = f(a)^2 x.
        template<int K> [[gnu::always_inline]] static constexpr auto powOnes(auto x, auto mul) {
            if constexpr (K == 1) { return x; }
            else if constexpr (K % 2) { auto f = powOnes<K - 1>(x, mul); return mul(mul(f, f), x); }
            else { auto f = powOnes<K / 2>(x, mul), g = f; for (int i = 0; i < K / 2; ++i) { g = mul(g, g); } return mul(g, f); }}
        // a^E for a constant E = (2^K - 1) 2^T + L, L < 2^T, with a run of K >= ROOT_ONES ones on top: the run by powOnes, T squarings,
        // a^L beside them; Montgomery products where long powers use them. Any other E: rootPower.
        template<ulng E> static constexpr Value rootFixed(Value a) {
            constexpr int W = std::bit_width(E), K = E ? std::countl_one(E << (64 - W)) : 0, T = W - K; constexpr ulng L = T ? E & ((ulng(1) << T) - 1) : 0;
            if constexpr (K >= ROOT_ONES) {
                auto go = [](auto x, auto mul) __attribute__((always_inline)) {
                    auto r = powOnes<K>(x, mul), y = x, z = x; bool any = false;
                    for (int i = 0; i < T; ++i) {
                        r = mul(r, r);
                        if (L >> i & 1) { z = any ? mul(z, y) : y; any = true; }
                        if (L >> i >> 1) { y = mul(y, y); }}
                    return any ? mul(r, z) : r;};
                if constexpr (Form<C> || (!Direct<C> && (C::MOD & 1) && C::MOD != Barrett64::MERSENNE61)) {
                    if (!std::is_constant_evaluated()) {
                        const Montgomery64 &c = C::montgomery; ulng ni = 0 - c.inv, m = c.mod;
                        auto run = [&](auto mul) __attribute__((always_inline)) {
                            if constexpr (Form<C>) { ulng r = go(a.n, mul); return at(r - (r >= m ? m : 0)); }
                            else { return raw(mmul(go(mul(a.n, c.rsq), mul), 1, ni, m)); }};
                        if constexpr (ulng(C::MOD) >> 62) { return run([&](ulng x, ulng y) __attribute__((always_inline)) { return mmul(x, y, ni, m); }); }
                        else { return run([&](ulng x, ulng y) __attribute__((always_inline)) { return mmulLazy(x, y, ni, m); }); }}}
                return go(a, [](Value x, Value y) __attribute__((always_inline)) { return x * y; });}
            return rootPower(a, E);}
        // The power of the root for p = 3 mod 4, and the first power of the windows.
        [[gnu::always_inline]] static constexpr Value root34(Value a) {
            if constexpr (Fixed<C>) { return rootFixed<ulng(C::MOD) / 4 + 1>(a); } else { return rootPower(a, mod() / 4 + 1); }}
        [[gnu::always_inline]] static constexpr Value rootOdd(Value a, int s) {
            if constexpr (Fixed<C>) { return rootFixed<((ulng(C::MOD) - 1) >> std::countr_zero(ulng(C::MOD) - 1)) / 2>(a); }
            else { return rootPower(a, ((mod() - 1) >> s) / 2); }}
        // T: O(log(p)) for p = mod() = q 2^s + 1 static, 3 mod 4, 5 mod 8, or from the 257th call on a dynamic p (O(2^min(s, 8) + log(p)^2)
        // once); before it O(log(p)^2) expected by Cipolla for s^2 above 15 (64-bit words: 9) log2(p), else O(log(p) + s^2). M: O(1).
        friend constexpr bool trySqrt(Value a, Value &out) {
            assert(C::is_prime);
            if (plain(a.n) < 2 || mod() == 2) { out = a; return true; }
            Value x; bool counted = false; auto no = [&] { if (counted) { sqrtRun += sqrtRun < 8; } return false; };
            if (mod() % 4 == 3) {
                x = root34(a); if (x * x != a) { return false; }}
            else {
                if constexpr (std::is_same_v<Word, ulng>) { counted = !std::is_constant_evaluated() && mod() >> 32 != 0; }
                if (counted && sqrtRun >= 8 && jacobi(a.n, mod()) != 1) { return false; }
                int s = std::countr_zero(Word(mod() - 1)); x = 0;
                if constexpr (Fixed<C>) {
                    if constexpr (C::is_prime && C::MOD % 4 == 1) { static_assert(SqrtData<Value>::value.ok); x = sqrtWindows(SqrtData<Value>::value, s, a); }}
                else if (s == 2) { // Atkin: b = (2 a)^((p - 5) / 8), i = 2 a b^2 has i^2 = -1 iff a is a residue
                    Value b, c, i;
                    if constexpr (std::is_same_v<Word, uint>) {
                        auto twice = [](Value v) { Word h = mod() - v.n; return raw(v.n >= h ? v.n - h : v.n + v.n); };
                        b = rootPower(twice(a), mod() / 8); c = a * b;
                        i = raw(C::redSel(Wide(twice(c).n) * b.n));} // i = +-1 for a non-residue: the correction jump would be a coin flip
                    else { b = rootPower(a + a, mod() / 8); c = a * b; i = (c + c) * b; }
                    if (Word(plain(i.n) - 2) < Word(mod() - 3)) { x = c * (i - 1); }} // i^4 = 1: i^2 = -1 unless i = +-1
                else {
                    SqrtCache &sc = sqrtCache;
                    if (sc.mod != mod()) {
                        sc.mod = mod(); sc.calls = 0; sc.rooted = false;
                        sc.wide = s * s > (std::is_same_v<Word, uint> ? 15 : 9) * std::bit_width(mod());}
                    if (sc.calls == 256) { sqrtBuild(sc.table, s); }
                    sc.calls += sc.calls <= 256; // the 257th call builds; a table without a separating multiplier is never used
                    if (sc.calls > 256 && sc.table.ok) { x = sqrtWindows(sc.table, s, a); }
                    else {
                        bool wide = sc.wide; Value b = 1, d = b - a;
                        if (wide) { // Cipolla: (b + w)^((p + 1) / 2) with w^2 = d = b^2 - a a nonresidue
                            if (jacobi(a.n, mod()) != 1) { return no(); }
                            for (; plain(b.n) < 64 && d.n && jacobi(d.n, mod()) != -1; ++b) { d += b + b + 1; }
                            wide = plain(b.n) < 64;}
                        if (wide) {
                            x = b;
                            if (d.n) {
                                Value y = 1; Word e = mod() / 2 + 1;
                                for (int j = std::bit_width(e) - 2; j >= 0; --j) {
                                    Value xy = x * y; x = x * x + y * y * d; y = xy + xy;
                                    if (e >> j & 1) { Value v = x * b + y * d; y = x + y * b; x = v; }}}}
                        else { // Tonelli-Shanks: x * x = a * t with t of order 2^i
                            Value w = rootPower(a, ((mod() - 1) >> s) / 2), t, u; x = a * w; t = u = x * w; int i = 0;
                            for (; u != 1 && i < s; ++i) { u *= u; }
                            if (i == s) { return no(); }
                            if (i && !sc.rooted) { sc.roots = sqrtRoots(s); sc.rooted = true; }
                            while (i) {
                                x *= at(sc.roots[s - i - 1]); t *= at(sc.roots[s - i]); int m = i;
                                for (i = 0, u = t; u != 1 && i < m - 1; ++i) { u *= u; }}}}}
                if (!x.n) { return no(); }}
            if (counted) { sqrtRun = 0; }
            Word v = plain(x.n); out = raw(min(v, Word(mod() - v))); return true;}
        friend constexpr Value sqrt(Value a) {
            if (mod() % 4 == 3) { // a select: a jump on "is a residue" would be a coin flip
                assert(C::is_prime); Value x = root34(a); Word k = Word(0) - Word(x * x == a), v = plain(x.n);
                return from(Word((min(v, Word(mod() - v)) & k) | (Word(mod() - 1) & ~k)));}
            Value r; return trySqrt(a, r) ? r : raw(mod() - 1);}

        // Prefix products in L chains over the residues of the index and one over the tail, joined before the one inverse: LS scalar chains (MONT: on Montgomery
        // products), or 8 NV AVX2 lanes of a 32-bit word and an odd modulus. A Montgomery chain keeps its product times R^-(blocks so far); its inverse cancels that power.
        template<int LS, int NV, bool MONT> [[gnu::noinline]] static bool batchChains(const vector<Value> &a, vector<Value> &out) {
            constexpr int L = NV ? 8 * NV : LS;
            int k = int(a.size()), i = 0, top = k - k % L;
            vector<Value> r(k); Value t; Word p[L + 1], pre[L + 2], one = unit(); // chains as words: no reduction to construct them
            for (int c = 0; c <= L; ++c) { p[c] = one; }
            [[maybe_unused]] ulng ni = 0 - C::montgomery.inv, m = mod();
            [[maybe_unused]] auto mul = [&](Word x, Word y) __attribute__((always_inline)) {
                if constexpr (MONT) { return Word(mmul(x, y, ni, m)); } else { return C::mul(x, y); }};
#ifdef __AVX2__
            [[maybe_unused]] std::conditional_t<(NV > 0), Montgomery32, uint> mg(NV ? uint(mod()) : 1u); [[maybe_unused]] __m256i v[4];
            [[maybe_unused]] auto mul8 = [](const auto &g, __m256i x, __m256i y) __attribute__((always_inline, flatten)) { return g.mul8(x, y, false); }; // else a call
            if constexpr (NV > 0) {
                for (int c = 0; c < NV; ++c) { v[c] = _mm256_set1_epi32(1); }
                for (; i < top; i += L) {
                    for (int c = 0; c < NV; ++c) {
                        __m256i x = mg.load(&a[i + 8 * c].n); mg.store(&r[i + 8 * c].n, v[c]); v[c] = mul8(mg, v[c], x);}}
                for (int c = 0; c < NV; ++c) { mg.store(p + 8 * c, v[c]); }}
#endif
            if constexpr (!NV) {
                for (; i < top; i += L) {
                    for (int c = 0; c < L; ++c) { r[i + c].n = p[c]; p[c] = mul(p[c], a[i + c].n); }}}
            for (i = top; i < k; ++i) { r[i].n = p[L]; p[L] = C::mul(p[L], a[i].n); }
            pre[0] = one;
            for (int c = 0; c <= L; ++c) { pre[c + 1] = C::mul(pre[c], p[c]); }
            t.n = pre[L + 1];
            if (!tryInv(t, t)) { return false; }
            for (int c = L + 1; c-- > 0;) { Word w = p[c]; p[c] = C::mul(t.n, pre[c]); t.n = C::mul(t.n, w); } // p[c] = 1 / (its chain's product)
            for (i = k; i-- > top;) { Word w = a[i].n; r[i].n = C::mul(r[i].n, p[L]); p[L] = C::mul(p[L], w); }
#ifdef __AVX2__
            if constexpr (NV > 0) {
                for (int c = 0; c < NV; ++c) { v[c] = mg.load(p + 8 * c); }
                for (i = top; i > 0; i -= L) {
                    for (int c = 0; c < NV; ++c) {
                        Word *o = &r[i - L + 8 * c].n; __m256i x = mg.load(&a[i - L + 8 * c].n);
                        mg.store(o, mul8(mg, mg.load(o), v[c])); v[c] = mul8(mg, v[c], x);}}}
#endif
            if constexpr (!NV) {
                for (i = top; i > 0; i -= L) {
                    for (int c = 0; c < L; ++c) { Word w = a[i - L + c].n; r[i - L + c].n = mul(r[i - L + c].n, p[c]); p[c] = mul(p[c], w); }}}
            out = std::move(r); return true;}
        // T: O(k + log(mod())), M: O(k); k = a.size() <= INT_MAX, output unchanged on failure.
        friend bool batchInv(const vector<Value> &a, vector<Value> &out) {
            assert(a.size() <= INT_MAX); int k = int(a.size());
            bool mont = false; // Montgomery chains: an odd modulus from 2^32 whose product is not one short step
            if constexpr (Form<C>) { mont = true; } // its own product already
            else if constexpr (std::is_same_v<Word, ulng> && !Direct<C>) { mont = (mod() & 1) && ulng(mod()) >> 32 && mod() != Barrett64::MERSENNE61; }
            const int *s = mont ? BATCH_MONT : !Fixed<C> ? BATCH_DYNAMIC : ExactStep<C> ? BATCH_EXACT : BATCH_STATIC;
            if (k < s[0]) {
                vector<Value> r(k); Value t = at(unit());
                for (int i = 0; i < k; ++i) { r[i] = t; t *= a[i]; }
                if (!tryInv(t, t)) { return false; }
                for (int i = k; i-- > 0;) { Value v = a[i]; r[i] *= t; t *= v; }
                out = std::move(r); return true;}
#ifdef __AVX2__
            if constexpr (std::is_same_v<Word, uint>) {
                if (k >= BATCH_LANES && (mod() & 1) && mod() > 1) {
                    return k < BATCH_LANES16 ? batchChains<8, 1, false>(a, out) : k < BATCH_LANES32 ? batchChains<8, 2, false>(a, out) : batchChains<8, 4, false>(a, out);}}
#endif
            if constexpr (std::is_same_v<Word, ulng> && !Direct<C>) {
                if (mont) { return k < s[1] ? batchChains<2, 0, !Form<C>>(a, out) : k < s[2] ? batchChains<4, 0, !Form<C>>(a, out) : batchChains<8, 0, !Form<C>>(a, out); }}
            if constexpr (Fixed<C>) { if (k < s[1]) { return batchChains<2, 0, false>(a, out); } }
            return k < s[2] ? batchChains<4, 0, false>(a, out) : batchChains<8, 0, false>(a, out);}
        // r 10^n + chunk for n <= 18 and chunk < 10^n. A zero r, which most tokens end with, leaves the chunk alone, except under a
        // dynamic 64-bit word, whose one reciprocal step (the sum is below mod() 10^n) serves both.
        [[gnu::always_inline]] static Value append(Value r, uint n, ulng chunk) {
            if constexpr (Fixed<C> || Form<C> || std::is_same_v<Word, uint>) {
                if (!r.n) { return Fixed<C> && !Form<C> && C::MOD >= P10[18] ? at(Word(chunk)) : Value(chunk); }}
            if constexpr (Form<C>) { // r 10^n as a canonical residue, then the sum into the form: any 64-bit word converts by one product
                ulng x = C::mul(r.n, P10[n]) + chunk;
                if constexpr (!Fixed<C> || C::MOD > ~ulng(0) - P10[18]) { x += x < chunk ? 0 - mod() : 0; } // the sum left the word
                return at(C::mul(x, C::montgomery.rsq));}
            else if constexpr (std::is_same_v<Word, ulng> && (!Fixed<C> || requires { requires C::LONG; })) {
                ulll x = ulll(r.n) * P10[n] + chunk; return at(C::reciprocal.rem(ulng(x >> 64), ulng(x)));}
            else { return r * Value(P10[n]) + Value(chunk); }}
        [[gnu::noinline]] static Value appendOut(Value r, uint n, ulng chunk) { return append(r, n, chunk); } // inside a token: its temporaries stay out of the scan loop
        // The last group of a token, fewer than eight bytes before e and at least eight behind the area's start: finished in place when a
        // white-space byte follows its digits there.
        [[gnu::noinline]] static std::pair<const char *, Value> tail(const char *s, const char *e, const std::ctype<char> *ct, bool neg, Value r, ulng chunk, uint n) {
            uint left = uint(e - s);
            if (!left) { return {}; }
            ulng c; std::memcpy(&c, e - 8, 8); c = (c >> 8 * (8 - left)) ^ 0x3030303030303030; // the bytes from e on read as non-digits
            uint k = uint(std::countr_zero((c | (c + 0x7676767676767676)) & 0x8080808080808080)) >> 3;
            if (k == left) { return {}; } // the digits reach the end of the area
            if (k) {
                c <<= 64 - 8 * k;
                c = (c * 10 + (c >> 8)) & 0x00ff00ff00ff00ff; c = (c * 100 + (c >> 16)) & 0x0000ffff0000ffff; c = (c * 10000 + (c >> 32)) & 0xffffffff;
                if (n + k > 18) { r = appendOut(r, n, chunk); chunk = 0; n = 0; }
                chunk = P10[k] * chunk + c; n += k; s += k;}
            if (!n || !ct->is(std::ctype_base::space, *s)) { return {}; }
            r = appendOut(r, n, chunk);
            return {s, neg ? -r : r};}
        // The digits of a token from s on, in groups of eight: its end and value, the end null on failure. MORE: any number of digits, r the
        // value before s and the chunk the n digits of its last group; else up to 18 in a loop that holds no residue, a longer token going on in scanLong.
        template<bool MORE, bool TAIL> [[gnu::always_inline]] static std::pair<const char *, Value> digits(const char *s, const char *lim, const std::ctype<char> *ct, bool neg, Value r, ulng chunk, uint n) {
            for (uint k = MORE ? n : 8; k == 8;) {
                if (s > lim) { if constexpr (TAIL) { return tail(s, lim + 8, ct, neg, r, chunk, n); } else { return {}; } }
                ulng c; std::memcpy(&c, s, 8); c ^= 0x3030303030303030;
                ulng bad = (c | (c + 0x7676767676767676)) & 0x8080808080808080; // the lowest set byte is the first non-digit
                k = bad ? uint(std::countr_zero(bad)) >> 3 : 8;
                if (k == 0) { break; }
                c <<= 64 - 8 * k; // k digits behind leading zeros
                c = (c * 10 + (c >> 8)) & 0x00ff00ff00ff00ff; c = (c * 100 + (c >> 16)) & 0x0000ffff0000ffff; c = (c * 10000 + (c >> 32)) & 0xffffffff;
                if (n + k > 18) { // the chunk is full
                    if constexpr (!MORE) { auto [t, v] = scanLong(s + k, lim, ct, append(r, n, chunk), c, k); return {t, neg ? -v : v}; }
                    else { r = appendOut(r, n, chunk); chunk = 0; n = 0; }}
                chunk = P10[k] * chunk + c; n += k; s += k;}
            if (!n || !ct->is(std::ctype_base::space, *s)) { return {}; }
            r = append(r, n, chunk);
            return {s, neg ? -r : r};}
        [[gnu::noinline]] static std::pair<const char *, Value> scanLong(const char *s, const char *lim, const std::ctype<char> *ct, Value r, ulng chunk, uint n) { return digits<true, false>(s, lim, ct, false, r, chunk, n); }
        // The token at q when it and the byte behind it end eight bytes clear of e: its end and value, the end null otherwise. TAIL: tail finishes a later end.
        template<bool TAIL> [[gnu::always_inline]] static std::pair<const char *, Value> scan(const char *q, const char *e, const std::ctype<char> *ct) {
            if (e - q < 8) { return {}; }
            bool neg = false;
            if (*q == '-' || *q == '+') [[unlikely]] { neg = *q == '-'; __asm__("" : "+r"(q)); ++q; } // a jump: the digits' address waits for no byte
            return digits<false, TAIL>(q, e - 8, ct, neg, Value(), 0, 0);}
        // T: O(d), M: O(1); one signed decimal token between the white space the stream's sentry skips (ctypeOf), but a leading sign and every digit
        // belong to it whatever the locale calls them; any stream state and width; failbit preserves the destination. A get area holding the token is scanned in place first.
        [[gnu::noinline]] static istream &readSlow(istream &is, Value &a, const std::ctype<char> *ct) {
            std::ios::iostate err = std::ios::goodbit; int c = EOF; bool peeked = false;
            if (!is.good()) { is.setstate(std::ios::failbit); return is; } // the sentry by hand: its peek serves the extraction
            try {
                if (is.tie()) { is.tie()->flush(); }
                if (is.flags() & std::ios::skipws) {
                    if (!ct) { throw std::bad_cast(); }
                    for (c = is.rdbuf()->sgetc(); c != EOF && ct->is(std::ctype_base::space, char(c));) { c = is.rdbuf()->snextc(); }
                    if (c == EOF) { err |= std::ios::eofbit; }
                    peeked = true;}}
            catch (...) {
                try { is.setstate(std::ios::badbit); } catch (...) {}
                if (is.exceptions() & std::ios::badbit) { throw; }}
            if (!is.good() || err) { is.setstate(err | std::ios::failbit); return is; }
            bool any = false, bad = false, neg = false; Value r; ulng chunk = 0; uint n = 0;
            try {
                if (!ct) { ct = &std::use_facet<std::ctype<char>>(is.getloc()); }
                std::streambuf *sb = is.rdbuf();
                std::streamsize left = is.width() > 0 ? is.width() : std::numeric_limits<std::streamsize>::max();
                if (is.width() == 0) {
                    const char *g = (sb->*&Area::gptr)();
                    if (auto [t, v] = scan<true>(g, (sb->*&Area::egptr)(), ct); t && ulng(t - g) <= INT_MAX) { a = v; (sb->*&Area::gbump)(int(t - g)); return is; }}
                if (!peeked) { c = sb->sgetc(); }
                auto more = [&]() { return left > 0 && c != EOF && (uint(c - '0') < 10 || !ct->is(std::ctype_base::space, char(c))); }; // a digit is never white space
                if (c == '+' || c == '-') { neg = c == '-'; any = true; --left; c = sb->snextc(); }
                for (; more(); c = sb->snextc(), --left) {
                    any = true;
                    if (c < '0' || c > '9') { bad = true; continue; }
                    if (n == 18) { r = appendOut(r, n, chunk); chunk = 0; n = 0; }
                    chunk = 10 * chunk + ulng(c - '0'); ++n;}
                if (left > 0 && c == EOF) { err |= std::ios::eofbit; }
                is.width(0);}
            catch (...) { // as the library's extractors: badbit, and the exception again when the mask asks for it
                try { is.setstate(std::ios::badbit); } catch (...) {}
                if (is.exceptions() & std::ios::badbit) { throw; }}
            if (!any) { err |= std::ios::failbit; }
            if (err) { is.setstate(err); }
            if (is.fail()) { return is; }
            if (bad || !n) { is.setstate(std::ios::failbit); return is; }
            r = append(r, n, chunk);
            a = neg ? -r : r; return is;}
        friend istream &operator>>(istream &is, Value &a) {
            const std::ctype<char> *ct = ctypeOf(is);
            if (ct && is.good() && !is.tie() && (is.flags() & std::ios::skipws) && is.width() == 0) { // a sentry would only skip the white space
                std::streambuf *sb = is.rdbuf();
                const char *g = (sb->*&Area::gptr)(), *e = (sb->*&Area::egptr)(), *q = g;
                while (q < e && ct->is(std::ctype_base::space, *q)) { ++q; }
                if (auto [t, v] = scan<false>(q, e, ct); t && ulng(t - g) <= INT_MAX) { a = v; (sb->*&Area::gbump)(int(t - g)); return is; }}
            return readSlow(is, a, ct);}
        // T: O(d), M: O(1); the decimal digits of val(), as os << val() writes them in every stream state.
        [[gnu::always_inline]] friend ostream &operator<<(ostream &os, Value a) {
            if (os.width() == 0 && !(os.flags() & (std::ios::oct | std::ios::hex | std::ios::unitbuf)) && os.good() && !os.tie()) { // a sentry would do nothing
                std::streambuf *sb = os.rdbuf(); char *w = (sb->*&Area::pptr)();
                if ((sb->*&Area::epptr)() - w >= 20) { (sb->*&Area::pbump)(int(digitsTo(w, a.val()))); return os; }}
            return writeSlow(os, a.val());}
    };
} // namespace modint_detail

// S: O(1), Q: O(1), M: O(1); static modulus in [1, 2^32) or [1, 2^64).
template<uint MOD> requires (MOD > 0)
using ModInt = modint_detail::Value<modint_detail::Context<uint, MOD, 0>>;
template<ulng MOD> requires (MOD > 0)
using ModInt64 = modint_detail::Value<modint_detail::Context<ulng, MOD, 0>>;

// Runtime modulus per ID and width, default 998244353; single-threaded.
template<int ID = 0> using DynModInt = modint_detail::Value<modint_detail::Context<uint, 0, ID>>;
template<int ID = 0> using DynModInt64 = modint_detail::Value<modint_detail::Context<ulng, 0, ID>>;

// Montgomery-form storage, n = x 2^64 mod MOD, for an odd modulus: products about twice as fast from 2^32, val() one reduction.
// DynMontModInt64<ID> has the modulus of DynModInt64<ID>.
template<ulng MOD> requires (MOD % 2 == 1)
using MontModInt64 = modint_detail::Value<modint_detail::MontContext<MOD, 0>>;
template<int ID = 0> using DynMontModInt64 = modint_detail::Value<modint_detail::MontContext<0, ID>>;

using ModInt61 = ModInt64<Barrett64::MERSENNE61>;

using mint = ModInt<998244353>;

template<typename T> concept ModularInt = requires(T a) {
    typename T::Word; { T::mod() } -> std::same_as<typename T::Word>; { a.val() } -> std::same_as<typename T::Word>; a.n;};
template<typename T> concept StaticModularInt = ModularInt<T> && requires { std::integral_constant<typename T::Word, T::mod()>{}; };
