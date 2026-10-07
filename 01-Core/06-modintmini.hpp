#pragma once
#include "01-template.hpp"

// Four independently copyable structs; each needs only standard headers and the width aliases.

// S: O(1), Q: O(1), M: O(1); inv O(log(mod())), pow O(1 + log(|e| + 1)) plus inv for e < 0.
// Static modulus in [1, 2^32), constexpr; inv and division need a unit, tryInv returns false otherwise.
template<uint MOD> requires (MOD > 0)
struct ModIntMini {
    using Word = uint;
    Word n;

    static constexpr Word mod() { return MOD; }

    template<typename T = int> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    constexpr ModIntMini(T a = 0) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            lll r = lll(a) % lll(mod()); n = Word(r < 0 ? r + mod() : r);}
        else { n = Word(ulll(a) % mod()); }}
    static constexpr ModIntMini raw(Word a) { assert(a < mod()); ModIntMini r; r.n = a; return r; }
    constexpr Word val() const { return n; }

    constexpr ModIntMini &operator+=(ModIntMini a) { n = n >= mod() - a.n ? n - (mod() - a.n) : n + a.n; return *this; }
    constexpr ModIntMini &operator-=(ModIntMini a) { n = n < a.n ? mod() - (a.n - n) : n - a.n; return *this; }
    constexpr ModIntMini &operator*=(ModIntMini a) { n = Word(ulng(n) * a.n % MOD); return *this; }
    constexpr ModIntMini &operator/=(ModIntMini a) { return *this *= inv(a); }

    constexpr ModIntMini operator+() const { return *this; }
    constexpr ModIntMini operator-() const { return raw(n ? mod() - n : 0); }
    friend constexpr ModIntMini operator+(ModIntMini a, ModIntMini b) { return a += b; }
    friend constexpr ModIntMini operator-(ModIntMini a, ModIntMini b) { return a -= b; }
    friend constexpr ModIntMini operator*(ModIntMini a, ModIntMini b) { return a *= b; }
    friend constexpr ModIntMini operator/(ModIntMini a, ModIntMini b) { return a /= b; }
    friend constexpr bool operator==(ModIntMini a, ModIntMini b) { return a.n == b.n; }

    friend constexpr bool tryInv(ModIntMini a, ModIntMini &out) {
        Word r = a.n, s = mod(); lng x = 1, y = 0;
        while (s) {
            Word q = r / s; r = std::exchange(s, r % s);
            x = std::exchange(y, x - lng(q) * y);}
        if (r != 1) { return false; }
        out = raw(Word(x < 0 ? x + mod() : x)); return true;}
    friend constexpr ModIntMini inv(ModIntMini a) {
        ModIntMini r; bool ok = tryInv(a, r); assert(ok); (void)ok; return r;}

    template<typename T> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    friend constexpr ModIntMini pow(ModIntMini a, T e) {
        if (mod() == 1) { return raw(0); }
        ulll b = ulll(e);
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            if (e < 0) { a = inv(a); b = -b; }}
        ModIntMini r = raw(1);
        for (; b > 1; b >>= 1, a *= a) { if (b & 1) { r *= a; } }
        return b ? r * a : r;}
};

// S: O(1), Q: O(1), M: O(1); inv O(log(mod())), pow O(1 + log(|e| + 1)) plus inv for e < 0.
// Static modulus in [1, 2^64), constexpr; inv and division need a unit, tryInv returns false otherwise.
template<ulng MOD> requires (MOD > 0)
struct ModInt64Mini {
    using Word = ulng;
    Word n;

    static constexpr Word mod() { return MOD; }

    template<typename T = int> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    constexpr ModInt64Mini(T a = 0) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            lll r = lll(a) % lll(mod()); n = Word(r < 0 ? r + mod() : r);}
        else { n = Word(ulll(a) % mod()); }}
    static constexpr ModInt64Mini raw(Word a) { assert(a < mod()); ModInt64Mini r; r.n = a; return r; }
    constexpr Word val() const { return n; }

    constexpr ModInt64Mini &operator+=(ModInt64Mini a) { n = n >= mod() - a.n ? n - (mod() - a.n) : n + a.n; return *this; }
    constexpr ModInt64Mini &operator-=(ModInt64Mini a) { n = n < a.n ? mod() - (a.n - n) : n - a.n; return *this; }
    constexpr ModInt64Mini &operator*=(ModInt64Mini a) { n = Word(ulll(n) * a.n % MOD); return *this; }
    constexpr ModInt64Mini &operator/=(ModInt64Mini a) { return *this *= inv(a); }

    constexpr ModInt64Mini operator+() const { return *this; }
    constexpr ModInt64Mini operator-() const { return raw(n ? mod() - n : 0); }
    friend constexpr ModInt64Mini operator+(ModInt64Mini a, ModInt64Mini b) { return a += b; }
    friend constexpr ModInt64Mini operator-(ModInt64Mini a, ModInt64Mini b) { return a -= b; }
    friend constexpr ModInt64Mini operator*(ModInt64Mini a, ModInt64Mini b) { return a *= b; }
    friend constexpr ModInt64Mini operator/(ModInt64Mini a, ModInt64Mini b) { return a /= b; }
    friend constexpr bool operator==(ModInt64Mini a, ModInt64Mini b) { return a.n == b.n; }

    friend constexpr bool tryInv(ModInt64Mini a, ModInt64Mini &out) {
        Word r = a.n, s = mod(); lll x = 1, y = 0;
        while (s) {
            Word q = r / s; r = std::exchange(s, r % s);
            x = std::exchange(y, x - lll(q) * y);}
        if (r != 1) { return false; }
        out = raw(Word(x < 0 ? x + mod() : x)); return true;}
    friend constexpr ModInt64Mini inv(ModInt64Mini a) {
        ModInt64Mini r; bool ok = tryInv(a, r); assert(ok); (void)ok; return r;}

    // T: O(1 + log(e + 1)), M: O(1); odd m, canonical a; noipa keeps the measured generic REDC kernel.
    [[gnu::noinline, gnu::noipa]] static Word powerMontgomery(Word a, Word m, ulll e) {
        Word v = m;
        for (int i = 3; i < 64; i *= 2) { v *= 2 - m * v; }
        v = -v;
        auto red = [m, v](ulll x) -> Word {
            Word h = Word(x >> 64), p = Word(ulll(Word(x) * v) * m >> 64);
            Word r = h + p + (Word(x) != 0);
            return r - ((r < h || r >= m) ? m : 0);};
        Word rsq = Word(-ulll(m) % m), r = red(rsq);
        a = red(ulll(a) * rsq);
        while (e) {
            if (e & 1) { r = red(ulll(r) * a); }
            e >>= 1; if (e) { a = red(ulll(a) * a); }}
        return red(r);}

    template<typename T> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    friend constexpr ModInt64Mini pow(ModInt64Mini a, T e) {
        if (mod() == 1) { return raw(0); }
        ulll b = ulll(e);
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            if (e < 0) { a = inv(a); b = -b; }}
        if (!std::is_constant_evaluated() && (mod() & 1) && b >= 512 &&
            (mod() != 2305843009213693951ULL || b >= 65536)) {
            return raw(powerMontgomery(a.n, mod(), b));}
        ModInt64Mini r = raw(1);
        for (; b > 1; b >>= 1, a *= a) { if (b & 1) { r *= a; } }
        return b ? r * a : r;}
};

// S: O(1), Q: O(1), M: O(1); inv O(log(mod())), pow O(1 + log(|e| + 1)) plus inv for e < 0.
// Runtime modulus in [1, 2^32) per ID, default 998244353; setMod stales all values; tryInv returns false on nonunits.
template<int ID = 0>
struct DynModIntMini {
    using Word = uint;
    static inline constinit Word modulus = 998244353;
    static inline constinit ulng reciprocal = ~ulng(0) / 998244353 + 1;
    Word n;

    static void setMod(Word m) {
        assert(m); reciprocal = ~ulng(0) / m + 1; modulus = m;}

    // Internal product reduction: x < mod()^2, not an arbitrary wide dividend.
    static Word red(ulng x) {
        if ((modulus & (modulus - 1)) == 0) { return Word(x) & (modulus - 1); }
        ulng q = ulng(ulll(x) * reciprocal >> 64), y = q * modulus;
        return Word(x - y + (x < y ? modulus : 0));}
    static Word mod() { return modulus; }

    template<typename T = int> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    DynModIntMini(T a = 0) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            lll r = lll(a) % lll(mod()); n = Word(r < 0 ? r + mod() : r);}
        else { n = Word(ulll(a) % mod()); }}
    static DynModIntMini raw(Word a) { assert(a < mod()); DynModIntMini r; r.n = a; return r; }
    Word val() const { return n; }

    DynModIntMini &operator+=(DynModIntMini a) { n = n >= mod() - a.n ? n - (mod() - a.n) : n + a.n; return *this; }
    DynModIntMini &operator-=(DynModIntMini a) { n = n < a.n ? mod() - (a.n - n) : n - a.n; return *this; }
    DynModIntMini &operator*=(DynModIntMini a) { n = red(ulng(n) * a.n); return *this; }
    DynModIntMini &operator/=(DynModIntMini a) { return *this *= inv(a); }

    DynModIntMini operator+() const { return *this; }
    DynModIntMini operator-() const { return raw(n ? mod() - n : 0); }
    friend DynModIntMini operator+(DynModIntMini a, DynModIntMini b) { return a += b; }
    friend DynModIntMini operator-(DynModIntMini a, DynModIntMini b) { return a -= b; }
    friend DynModIntMini operator*(DynModIntMini a, DynModIntMini b) { return a *= b; }
    friend DynModIntMini operator/(DynModIntMini a, DynModIntMini b) { return a /= b; }
    friend bool operator==(DynModIntMini a, DynModIntMini b) { return a.n == b.n; }

    friend bool tryInv(DynModIntMini a, DynModIntMini &out) {
        Word r = a.n, s = mod(); lng x = 1, y = 0;
        while (s) {
            Word q = r / s; r = std::exchange(s, r % s);
            x = std::exchange(y, x - lng(q) * y);}
        if (r != 1) { return false; }
        out = raw(Word(x < 0 ? x + mod() : x)); return true;}
    friend DynModIntMini inv(DynModIntMini a) {
        DynModIntMini r; bool ok = tryInv(a, r); assert(ok); (void)ok; return r;}

    template<typename T> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    friend DynModIntMini pow(DynModIntMini a, T e) {
        if (mod() == 1) { return raw(0); }
        ulll b = ulll(e);
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            if (e < 0) { a = inv(a); b = -b; }}
        DynModIntMini r = raw(1);
        for (; b > 1; b >>= 1, a *= a) { if (b & 1) { r *= a; } }
        return b ? r * a : r;}
};

// S: O(1), Q: O(1), M: O(1); inv O(log(mod())), pow O(1 + log(|e| + 1)) plus inv for e < 0.
// Runtime modulus in [1, 2^64) per ID, default 998244353; setMod stales all values; tryInv returns false on nonunits.
template<int ID = 0>
struct DynModInt64Mini {
    using Word = ulng;
    static inline constinit Word modulus = 998244353;
    Word n;

    static void setMod(Word m) { assert(m); modulus = m; }

    // Internal product reduction: x < mod()^2, not an arbitrary wide dividend.
    static Word red(ulll x) {
        if (std::has_single_bit(modulus)) { return Word(x) & (modulus - 1); }
        if (modulus == 2305843009213693951ULL) {
            Word r = Word(x >> 61) + (Word(x) & modulus); return r >= modulus ? r - modulus : r;}
        return Word(x % modulus);}
    static Word mod() { return modulus; }

    template<typename T = int> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    DynModInt64Mini(T a = 0) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            lll r = lll(a) % lll(mod()); n = Word(r < 0 ? r + mod() : r);}
        else { n = Word(ulll(a) % mod()); }}
    static DynModInt64Mini raw(Word a) { assert(a < mod()); DynModInt64Mini r; r.n = a; return r; }
    Word val() const { return n; }

    DynModInt64Mini &operator+=(DynModInt64Mini a) { n = n >= mod() - a.n ? n - (mod() - a.n) : n + a.n; return *this; }
    DynModInt64Mini &operator-=(DynModInt64Mini a) { n = n < a.n ? mod() - (a.n - n) : n - a.n; return *this; }
    DynModInt64Mini &operator*=(DynModInt64Mini a) { n = red(ulll(n) * a.n); return *this; }
    DynModInt64Mini &operator/=(DynModInt64Mini a) { return *this *= inv(a); }

    DynModInt64Mini operator+() const { return *this; }
    DynModInt64Mini operator-() const { return raw(n ? mod() - n : 0); }
    friend DynModInt64Mini operator+(DynModInt64Mini a, DynModInt64Mini b) { return a += b; }
    friend DynModInt64Mini operator-(DynModInt64Mini a, DynModInt64Mini b) { return a -= b; }
    friend DynModInt64Mini operator*(DynModInt64Mini a, DynModInt64Mini b) { return a *= b; }
    friend DynModInt64Mini operator/(DynModInt64Mini a, DynModInt64Mini b) { return a /= b; }
    friend bool operator==(DynModInt64Mini a, DynModInt64Mini b) { return a.n == b.n; }

    friend bool tryInv(DynModInt64Mini a, DynModInt64Mini &out) {
        Word r = a.n, s = mod(); lll x = 1, y = 0;
        while (s) {
            Word q = r / s; r = std::exchange(s, r % s);
            x = std::exchange(y, x - lll(q) * y);}
        if (r != 1) { return false; }
        out = raw(Word(x < 0 ? x + mod() : x)); return true;}
    friend DynModInt64Mini inv(DynModInt64Mini a) {
        DynModInt64Mini r; bool ok = tryInv(a, r); assert(ok); (void)ok; return r;}

    // T: O(1 + log(e + 1)), M: O(1); odd m, canonical a; noipa keeps the measured generic REDC kernel.
    [[gnu::noinline, gnu::noipa]] static Word powerMontgomery(Word a, Word m, ulll e) {
        Word v = m;
        for (int i = 3; i < 64; i *= 2) { v *= 2 - m * v; }
        v = -v;
        auto red = [m, v](ulll x) -> Word {
            Word h = Word(x >> 64), p = Word(ulll(Word(x) * v) * m >> 64);
            Word r = h + p + (Word(x) != 0);
            return r - ((r < h || r >= m) ? m : 0);};
        Word rsq = Word(-ulll(m) % m), r = red(rsq);
        a = red(ulll(a) * rsq);
        while (e) {
            if (e & 1) { r = red(ulll(r) * a); }
            e >>= 1; if (e) { a = red(ulll(a) * a); }}
        return red(r);}

    template<typename T> requires (std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>)
    friend DynModInt64Mini pow(DynModInt64Mini a, T e) {
        if (mod() == 1) { return raw(0); }
        ulll b = ulll(e);
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            if (e < 0) { a = inv(a); b = -b; }}
        if ((mod() & 1) && mod() != 2305843009213693951ULL && b >= 512) {
            return raw(powerMontgomery(a.n, mod(), b));}
        DynModInt64Mini r = raw(1);
        for (; b > 1; b >>= 1, a *= a) { if (b & 1) { r *= a; } }
        return b ? r * a : r;}
};

using mintmini = ModIntMini<998244353>;
