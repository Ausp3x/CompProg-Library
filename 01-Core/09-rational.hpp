#pragma once
#include "../01-Core/01-template.hpp"

namespace rational_detail {
    // Exact arithmetic one width above a magnitude: ulll for ulng, (high, low) pairs for ulll; other types (InfInt) use their own.
    using W2 = pair<ulll, ulll>;
    template<typename X> X mulW(const X &a, const X &b) { return a * b; }
    inline ulll mulW(ulng a, ulng b) { return ulll(a) * b; }
    inline W2 mulW(ulll a, ulll b) {
        ulll a0 = ulng(a), a1 = a >> 64, b0 = ulng(b), b1 = b >> 64, lo = a0 * b0, m1 = a0 * b1 + (lo >> 64), m2 = a1 * b0 + ulng(m1);
        return {a1 * b1 + (m1 >> 64) + (m2 >> 64), (m2 << 64) | ulng(lo)};}
    template<typename X> X addW(const X &a, const X &b) { return a + b; }
    inline W2 addW(W2 a, W2 b) { ulll lo = a.second + b.second; return {a.first + b.first + (lo < a.second), lo}; }
    template<typename X> X subW(const X &a, const X &b) { return a - b; }
    inline W2 subW(W2 a, W2 b) { return {a.first - b.first - (a.second < b.second), a.second - b.second}; }
    template<typename X> X shlW(const X &a, int s) { return a << s; }
    inline ulll shlW(ulng a, int s) { return ulll(a) << s; }
    inline W2 shlW(ulll a, int s) { return s == 0 ? W2{0, a} : s < 128 ? W2{a >> (128 - s), a << s} : W2{a << (s - 128), 0}; }
    // q = x / d for 0 < d < 2^127 (2^63 for ulng); returns x % d.
    template<typename X> X divW(const X &x, const X &d, X &q) { auto res = divMod(x, d); q = std::move(res.first); return std::move(res.second); }
    inline ulng divW(ulll x, ulng d, ulll &q) { q = x / d; return ulng(x % d); }
    inline ulll divW(W2 x, ulll d, W2 &q) {
        if (x.first == 0) { q = {0, x.second / d}; return x.second % d; }
        q = {x.first / d, 0};
        ulll r = x.first % d;
        for (int i = 127; i >= 0; i--) {
            r = (r << 1) | ((x.second >> i) & 1);
            bool bit = r >= d;
            r -= bit ? d : 0; q.second |= ulll(bit) << i;}
        return r;}
    template<typename X> bool narrowW(const X &w, X &out) { out = w; return true; }
    inline bool narrowW(ulll w, ulng &out) { out = ulng(w); return w >> 64 == 0; }
    inline bool narrowW(W2 w, ulll &out) { out = w.second; return w.first == 0; }
    template<typename X> ulll low128(const X &w) { ulll r = 0; w.tryCast(r); return r; }
    inline ulll low128(ulll w) { return w; }
    inline ulll low128(W2 w) { return w.second; }
    template<typename X> int bitLen(const X &a) { return bitLength(a); }
    inline int bitLen(ulng a) { return std::bit_width(a); }
    inline int bitLen(ulll a) { return a >> 64 ? 128 - std::countl_zero(ulng(a >> 64)) : std::bit_width(ulng(a)); }
    template<typename X> X gcdM(const X &a, const X &b) { return gcd(a, b); }
} // namespace rational_detail

// T: O(w) word operations per arithmetic operation for lng and lll (binary gcd, w = 64 or 128), the InfInt gcd and product bounds for InfInt; M: O(1) words, O(n) for InfInt.
template<typename T> struct Rational {
    static constexpr bool BOUNDED = std::is_integral_v<T> || std::same_as<T, lll>;
    static_assert(std::same_as<T, lng> || std::same_as<T, lll> || (!BOUNDED && std::is_class_v<T>), "Rational<T> needs T = lng, lll or InfInt");
    using M = typename std::conditional_t<BOUNDED, std::make_unsigned<T>, std::type_identity<T>>::type;
    T num = 0, den = 1;

    Rational() {}
    Rational(T n, T d = 1) : num(std::move(n)), den(std::move(d)) { normalize(); }
    template<typename I> requires (!BOUNDED && (std::is_integral_v<I> || std::same_as<I, lll> || std::same_as<I, ulll>))
    Rational(I n) : num(n), den(1) {}
    template<typename A, typename B = int> requires std::floating_point<A> || std::floating_point<B>
    Rational(A, B = 1) = delete;
    template<typename U> requires (!std::same_as<U, T>)
    explicit Rational(const Rational<U> &o) {
        T n = 0, d = 1;
        bool ok = cast(o.num, n) && cast(o.den, d);
        assert(ok);
        if (ok) { num = std::move(n); den = std::move(d); }}
    // n / d with d > 0 and gcd(n, d) = 1, without a gcd.
    static Rational raw(T n, T d) {
        assert(d > 0);
        Rational r;
        if (d > 0) { r.num = std::move(n); r.den = std::move(d); }
        return r;}
    // The exact value of a finite x; it must fit T.
    static Rational fromDouble(long double x) {
        assert(std::isfinite(x));
        if (!std::isfinite(x) || x == 0) { return Rational(); }
        int e; ulng m = ulng(std::ldexp(std::fabs(std::frexp(x, &e)), 64));
        int tz = std::countr_zero(m);
        M a = M(m >> tz), b = 1;
        e += tz - 64;
        Rational r;
        bool ok = (e >= 0 ? shl(a, e) : shl(b, -e)) && set(r, x < 0, a, b);
        assert(ok); (void)ok;
        return r;}
    // T: O(L) for lng and lll; [+-] then p/q, or digits with at most one point and, in base 10, an exponent [eE][+-]digits below 2^31;
    // false and no change when malformed, the denominator is zero or the value does not fit; for lng and lll p, q and the significand must fit T's unsigned width.
    bool read(string_view s, uint base = 10) {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        bool neg = !s.empty() && s[0] == '-';
        if (!s.empty() && (s[0] == '+' || s[0] == '-')) { s.remove_prefix(1); }
        M a = 0, b = 1, p = 1;
        if (size_t slash = s.find('/'); slash != string_view::npos) {
            if (!parse(s.substr(0, slash), base, a) || !parse(s.substr(slash + 1), base, b) || b == M(0)) { return false; }}
        else {
            size_t ex = base == 10 ? s.find_first_of("eE") : string_view::npos;
            string_view m = s.substr(0, ex);
            size_t dot = m.find('.');
            string d = string(m.substr(0, dot)) + string(dot == string_view::npos ? "" : m.substr(dot + 1));
            lng e = dot == string_view::npos ? 0 : -lng(m.size() - dot - 1);
            if (ex != string_view::npos) {
                string_view es = s.substr(ex + 1);
                bool en = !es.empty() && es[0] == '-';
                if (!es.empty() && (es[0] == '+' || es[0] == '-')) { es.remove_prefix(1); }
                if (es.empty() || !std::ranges::all_of(es, [](char c) { return c >= '0' && c <= '9'; })) { return false; }
                lng v = 0;
                for (char c : es) { v = min(10 * v + (c - '0'), lng(1) << 40); }
                e += en ? -v : v;}
            for (; d.size() > 1 && d.back() == '0'; e++) { d.pop_back(); }
            if (e < INT_MIN || e > INT_MAX || !parse(d, base, a)) { return false; }
            if (a == M(0)) { e = 0; }
            if (BOUNDED && e < 0) {
                // a / base^k: cancel each prime of the base from a before forming the denominator, so only the reduced one must fit.
                for (uint q = 2, rest = base; q <= rest; q++) {
                    ulng need = 0;
                    for (; rest % q == 0; rest /= q) { need += 0 - ulng(e); }
                    for (; need && a % M(q) == M(0); need--) { a /= M(q); }
                    if (!powM(M(q), need, p) || !mulM(b, p, b)) { return false; }}}
            else if (!powM(M(base), e < 0 ? 0 - ulng(e) : ulng(e), p) || (e >= 0 && !mulM(a, p, a))) { return false; }
            else { b = e < 0 ? p : b; }}
        M g = rational_detail::gcdM(a, b);
        Rational r;
        if (!set(r, neg && a != M(0), a / g, b / g)) { return false; }
        *this = std::move(r);
        return true;}
    // Digits below base into a magnitude; false when empty, malformed or overflowing a bounded T.
    static bool parse(string_view s, uint base, M &out) {
        auto val = [](char c) { return uint(c >= '0' && c <= '9' ? c - '0' : (c | 32) >= 'a' && (c | 32) <= 'z' ? (c | 32) - 'a' + 10 : 99); };
        if (s.empty() || std::ranges::any_of(s, [&](char c) { return val(c) >= base; })) { return false; }
        if constexpr (BOUNDED) {
            M v = 0;
            for (char c : s) { if (!mulM(v, M(base), v) || __builtin_add_overflow(v, M(val(c)), &v)) { return false; } }
            out = v;
            return true;}
        else { return out.read(s, base); }}
    template<typename U> static bool cast(const U &x, T &out) {
        if constexpr (!BOUNDED) { out = T(x); return true; }
        else if constexpr (!Rational<U>::BOUNDED) { return x.tryCast(out); }
        else { out = T(x); return U(out) == x; }}

    static M mag(const T &x) {
        if constexpr (BOUNDED) { return x < 0 ? M(0) - M(x) : M(x); }
        else { return abs(x); }}
    static bool mulM(const M &a, const M &b, M &res) {
        if constexpr (BOUNDED) { return !__builtin_mul_overflow(a, b, &res); }
        else { res = a * b; return true; }}
    static bool shl(M &a, int s) {
        if constexpr (BOUNDED) { if (s >= int(8 * sizeof(M)) || (s > 0 && a >> (8 * sizeof(M) - size_t(s)))) { return false; } }
        a <<= s;
        return true;}
    static bool powM(M a, ulng k, M &res) {
        for (res = 1; k; k >>= 1) {
            if ((k & 1) && !mulM(res, a, res)) { return false; }
            if (k > 1 && !mulM(a, a, a)) { return false; }}
        return true;}
    // res = +-num / den when den != 0 and both fit T; res is unchanged otherwise.
    static bool set(Rational &res, bool neg, const M &num, const M &den) {
        if (den == M(0)) { return false; }
        if constexpr (BOUNDED) {
            constexpr M MAX = M(std::numeric_limits<T>::max());
            if (num > MAX + M(neg) || den > MAX) { return false; }
            res.num = neg ? T(M(0) - num) : T(num); res.den = T(den);}
        else { res.num = neg ? -num : num; res.den = den; }
        return true;}
    void normalize() {
        if constexpr (!BOUNDED) { assert(!num.is_inf && !den.is_inf); }
        assert(den != 0);
        M a = mag(num), b = mag(den), g = rational_detail::gcdM(a, b);
        bool ok = g != M(0) && set(*this, (num < 0) != (den < 0) && a != M(0), a / g, b / g);
        assert(ok);
        if (!ok) { num = 0; den = 1; }}

    int sign() const { return (num > 0) - (num < 0); }
    bool isInteger() const { return den == 1; }
    double toDouble() const { return toFloat<double>(); }
    long double toLongDouble() const { return toFloat<long double>(); }
    // Q = floor(|num| 2^s / den) keeps MANT + 1 or MANT + 2 bits (fewer for subnormals) and the rest is a sticky bit, so one rounding is exact.
    template<typename F> F toFloat() const {
        using namespace rational_detail;
        constexpr int MANT = std::numeric_limits<F>::digits, EMIN = std::numeric_limits<F>::min_exponent - MANT;
        if (num == 0) { return F(0); }
        M a = mag(num), b = M(den);
        F sg = num < 0 ? F(-1) : F(1);
        // Exact operands: one IEEE division already rounds once.
        if constexpr (BOUNDED) { if (bitLen(a) <= MANT && bitLen(b) <= MANT) { return sg * (F(a) / F(b)); } }
        int s = min(MANT + 1 - (bitLen(a) - bitLen(b)), 1 - EMIN);
        decltype(mulW(a, b)) q;
        bool sticky = divW(shlW(a, max(s, 0)), M(b << max(-s, 0)), q) != M(0);
        ulll Q = low128(q);
        if (Q == 0) { return sg * F(0); }
        int u = max(bitLen(Q) - s - MANT, EMIN), k = s + u;
        ulll m = Q >> k, rest = Q & ((ulll(1) << k) - 1), half = ulll(1) << (k - 1);
        m += rest > half || (rest == half && (sticky || (m & 1)));
        return sg * std::ldexp(F(m), u);}
    static string digits(const M &v, uint base) {
        if constexpr (BOUNDED) {
            string s; M x = v;
            do { s += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[size_t(x % base)]; x /= base; } while (x != 0);
            return string(s.rbegin(), s.rend());}
        else { return v.toString(base); }}
    // T: O(L) for lng and lll; "p" or "p/q" with uppercase digits.
    string toString(uint base = 10) const {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        string res = (num < 0 ? "-" : "") + digits(mag(num), base);
        return den == 1 ? res : res + "/" + digits(M(den), base);}
    // T: O(k) word operations (InfInt operations for InfInt); k >= 0 digits after the point, rounded half away from zero; zero has no sign.
    string toDecimal(int k) const {
        assert(k >= 0);
        M r = mag(num % den), d = M(den);
        string res = digits(mag(num / den), 10) + (k > 0 ? "." : "");
        for (int i = 0; i < k; i++) {
            int dig = 0; M t = 0;
            for (int j = 0; j < 10; j++) {
                if (t >= d - r) { t -= d - r; dig++; }
                else { t += r; }}
            res += char('0' + dig); r = t;}
        if (r >= d - r) {
            int i = int(res.size()) - 1;
            for (; i >= 0 && (res[size_t(i)] == '9' || res[size_t(i)] == '.'); i--) { res[size_t(i)] = res[size_t(i)] == '.' ? '.' : '0'; }
            if (i < 0) { res.insert(res.begin(), '1'); }
            else { res[size_t(i)]++; }}
        return num < 0 && res.find_first_not_of("0.") != string::npos ? "-" + res : res;}
    // num * den^-1 in a modular type Mint (ModInt, ModInt64, DynModInt, minis); den must be a unit.
    template<typename Mint> Mint toMod() const {
        if constexpr (BOUNDED) { return Mint(num) / Mint(den); }
        else { lll a = 0, b = 0; (num % T(Mint::mod())).tryCast(a); (den % T(Mint::mod())).tryCast(b); return Mint(a) / Mint(b); }}

    // Knuth: g = gcd(b, d), t = a (d / g) +- c (b / g), g2 = gcd(t mod g, g); the sum is (t / g2) / ((b / g) (d / g2)).
    static bool addTo(const Rational &x, const Rational &y, bool sub, Rational &res) {
        using namespace rational_detail;
        M b = M(x.den), d = M(y.den), g = gcdM(b, d), b1 = b / g, d1 = d / g, g2 = 1, num, den;
        auto p = mulW(mag(x.num), d1), q = mulW(mag(y.num), b1);
        bool nx = x.num < 0, ny = (y.num < 0) != sub, neg = nx;
        decltype(p) t = nx == ny ? addW(p, q) : p < q ? subW(q, p) : subW(p, q), tq = t;
        neg = nx != ny && p < q ? ny : neg;
        if (g != M(1)) { g2 = gcdM(M(divW(t, g, tq)), g); divW(t, g2, tq); }
        return narrowW(tq, num) && mulM(b1, d / g2, den) && set(res, neg, num, den);}
    // Cross-cancelled: g1 = gcd(a, d), g2 = gcd(c, b); (a / g1) (c / g2) / ((b / g2) (d / g1)) is reduced.
    static bool mulTo(const Rational &x, const Rational &y, bool div, Rational &res) {
        using namespace rational_detail;
        M a = mag(x.num), b = M(x.den), c = mag(y.num), d = M(y.den), num, den;
        bool neg = (x.num < 0) != (y.num < 0);
        if (div) { assert(c != M(0)); std::swap(c, d); }
        if (d == M(0)) { return false; }
        if (a == M(0) || c == M(0)) { res = Rational(); return true; }
        M g1 = gcdM(a, d), g2 = gcdM(c, b);
        return mulM(a / g1, c / g2, num) && mulM(b / g2, d / g1, den) && set(res, neg, num, den);}
    Rational &operator+=(const Rational &o) { bool ok = addTo(*this, o, false, *this); assert(ok); (void)ok; return *this; }
    Rational &operator-=(const Rational &o) { bool ok = addTo(*this, o, true, *this); assert(ok); (void)ok; return *this; }
    Rational &operator*=(const Rational &o) { bool ok = mulTo(*this, o, false, *this); assert(ok); (void)ok; return *this; }
    Rational &operator/=(const Rational &o) { bool ok = mulTo(*this, o, true, *this); assert(ok); (void)ok; return *this; }

    Rational operator+() const { return *this; }
    Rational operator-() const { Rational r; bool ok = set(r, num > 0, mag(num), M(den)); assert(ok); (void)ok; return r; }
    friend Rational operator+(Rational x, const Rational &y) { x += y; return x; }
    friend Rational operator-(Rational x, const Rational &y) { x -= y; return x; }
    friend Rational operator*(Rational x, const Rational &y) { x *= y; return x; }
    friend Rational operator/(Rational x, const Rational &y) { x /= y; return x; }
    // res is unchanged when the exact result does not fit T (or the divisor is zero).
    friend bool tryAdd(const Rational &x, const Rational &y, Rational &res) { return addTo(x, y, false, res); }
    friend bool trySub(const Rational &x, const Rational &y, Rational &res) { return addTo(x, y, true, res); }
    friend bool tryMul(const Rational &x, const Rational &y, Rational &res) { return mulTo(x, y, false, res); }
    friend bool tryDiv(const Rational &x, const Rational &y, Rational &res) { return mulTo(x, y, true, res); }
    friend std::strong_ordering operator<=>(const Rational &x, const Rational &y) {
        if constexpr (std::same_as<T, lng>) { return lll(x.num) * y.den <=> lll(y.num) * x.den; }
        int sx = x.sign(), sy = y.sign();
        if (sx != sy || sx == 0) { return sx <=> sy; }
        auto c = rational_detail::mulW(mag(x.num), M(y.den)) <=> rational_detail::mulW(mag(y.num), M(x.den));
        return sx > 0 ? c : 0 <=> c;}
    friend bool operator==(const Rational &x, const Rational &y) = default;

    friend Rational abs(const Rational &x) { Rational r; bool ok = set(r, false, mag(x.num), M(x.den)); assert(ok); (void)ok; return r; }
    friend Rational inv(const Rational &x) {
        assert(x.num != 0);
        Rational r; bool ok = set(r, x.num < 0, M(x.den), mag(x.num));
        assert(ok); (void)ok;
        return r;}
    // T: O(log(|e|)) products; e < 0 inverts a nonzero base.
    friend Rational pow(const Rational &x, lng e) {
        M a = mag(x.num), b = M(x.den), pa = 1, pb = 1;
        if (e < 0) { assert(x.num != 0); std::swap(a, b); }
        ulng k = e < 0 ? 0 - ulng(e) : ulng(e);
        Rational r; bool ok = powM(a, k, pa) && powM(b, k, pb) && set(r, x.num < 0 && (k & 1), pa, pb);
        assert(ok); (void)ok;
        return r;}
    friend T trunc(const Rational &x) { return x.num / x.den; }
    friend T floor(const Rational &x) { T q = x.num / x.den; return x.num < 0 && q * x.den != x.num ? q - 1 : q; }
    friend T ceil(const Rational &x) { T q = x.num / x.den; return x.num > 0 && q * x.den != x.num ? q + 1 : q; }
    // Half away from zero.
    friend T round(const Rational &x) { M r = mag(x.num % x.den), d = M(x.den); T q = x.num / x.den; return r >= d - r ? (x.num < 0 ? q - 1 : q + 1) : q; }
    friend Rational fracPart(const Rational &x) { T r = x.num % x.den; return raw(r < 0 ? r + x.den : r, x.den); }

    friend istream &operator>>(istream &is, Rational &x) {
        string s;
        if (is >> s && !x.read(s)) { is.setstate(std::ios_base::failbit); }
        return is;}
    friend ostream &operator<<(ostream &os, const Rational &x) { return os << x.toString(); }
};
using Frac = Rational<lng>;

// T: O(1) for lng and lll, O(n) for InfInt, M: O(1).
template<typename T> struct std::hash<Rational<T>> {
    size_t operator()(const Rational<T> &x) const { size_t h = std::hash<T>()(x.num); return h ^ (std::hash<T>()(x.den) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2)); }
};
