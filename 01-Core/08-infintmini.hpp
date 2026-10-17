#pragma once
#include "../01-Core/01-template.hpp"

// T: O(n) per linear operation, O(n * log(n)) per product, O(n * m) per division, M: O(n); base 1e5 limbs n plus a signed infinity.
struct InfIntMini {
    static constexpr int BASE = 100000;
    static constexpr ulng P = 0xFFFFFFFF00000001ULL, MUL_NTT = 13;
    int sgn = 1; bool is_inf = false;
    vector<uint> n;

    InfIntMini() {}
    template<typename T> requires std::is_integral_v<T> || std::same_as<T, lll> || std::same_as<T, ulll>
    InfIntMini(T x) { *this = x; }
    InfIntMini(string_view s, uint base = 10) { bool ok = read(s, base); assert(ok); (void)ok; }
    template<typename T> requires std::is_integral_v<T> || std::same_as<T, lll> || std::same_as<T, ulll>
    InfIntMini &operator=(T x) {
        is_inf = false; sgn = 1; ulll u = ulll(x);
        if constexpr (std::is_signed_v<T> || std::same_as<T, lll>) { if (x < 0) { sgn = -1; u = ulll(0) - u; } }
        n.clear();
        do { n.push_back(uint(u % BASE)); } while (u /= BASE);
        return *this;}
    // T: O(L) in base 10, O(L^2) otherwise; optional sign, digits below the base or (bases 2-23) "inf"; false and no change when malformed.
    bool read(string_view s, uint base = 10) {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        int sign = 1;
        if (!s.empty() && (s[0] == '+' || s[0] == '-')) { sign = s[0] == '-' ? -1 : 1; s.remove_prefix(1); }
        if (s.empty()) { return false; }
        if (base <= 23 && s.size() == 3 && (s[0] | 32) == 'i' && (s[1] | 32) == 'n' && (s[2] | 32) == 'f') { sgn = sign; is_inf = true; n.clear(); return true; }
        auto val = [](char c) { return uint(c >= '0' && c <= '9' ? c - '0' : (c | 32) >= 'a' && (c | 32) <= 'z' ? (c | 32) - 'a' + 10 : 99); };
        for (char c : s) { if (val(c) >= base) { return false; } }
        InfIntMini r;
        if (base == 10) {
            for (int i = int(s.size()); i > 0; i -= 5) {
                uint cur = 0;
                for (int j = max(0, i - 5); j < i; j++) { cur = 10 * cur + val(s[size_t(j)]); }
                r.n.push_back(cur);}}
        else { for (char c : s) { r.mulAdd(base, val(c)); } }
        r.trim(); r.sgn = r.isNil() ? 1 : sign;
        *this = std::move(r);
        return true;}
    // T: O(n) in base 10, O(n^2) otherwise; uppercase digits, "inf" / "-inf" for the sentinel.
    string toString(uint base = 10) const {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        if (is_inf) { return sgn == 1 ? "inf" : "-inf"; }
        string res;
        if (base == 10) {
            res = std::to_string(n.empty() ? 0 : n.back());
            for (int i = int(n.size()) - 2; i >= 0; i--) { string d = std::to_string(n[size_t(i)]); res += string(5 - d.size(), '0') + d; }}
        else {
            InfIntMini a = *this;
            do { res += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[a.divWord(base)]; } while (!a.isNil());
            reverse(res.begin(), res.end());}
        return (sgn == -1 && !isNil() ? "-" : "") + res;}

    bool isNil() const { return !is_inf && (n.empty() || (n.size() == 1 && n[0] == 0)); }
    explicit operator bool() const { return !isNil(); }
    void trim() {
        if (is_inf) { n.clear(); return; }
        while (n.size() > 1 && n.back() == 0) { n.pop_back(); }
        if (n.empty()) { n.push_back(0); }
        if (n.size() == 1 && n[0] == 0) { sgn = 1; }}
    // Finite values convert modulo 2^64 (BASE^13 is a multiple of 2^64), the sentinel saturates.
    explicit operator ulng() const {
        if (is_inf) { return sgn == 1 ? ~0ULL : 0; }
        ulng r = 0;
        for (int i = min(int(n.size()), 13); i-- > 0;) { r = r * BASE + n[size_t(i)]; }
        return sgn == 1 ? r : 0 - r;}
    explicit operator lng() const { return is_inf ? (sgn == 1 ? LLONG_MAX : LLONG_MIN) : lng(ulng(*this)); }

    // |this| = |this| * m + d for word m and digit d.
    void mulAdd(uint m, uint d) {
        ulng c = d;
        for (uint &x : n) { c += ulng(x) * m; x = uint(c % BASE); c /= BASE; }
        for (; c; c /= BASE) { n.push_back(uint(c % BASE)); }}
    // |this| += s * |o| with s = 1 or -1; the result must stay nonnegative.
    void addMag(const vector<uint> &o, int s) {
        if (n.size() < o.size()) { n.resize(o.size()); }
        int c = 0;
        for (size_t i = 0; i < n.size() && (i < o.size() || c); i++) {
            int x = int(n[i]) + c + s * int(i < o.size() ? o[i] : 0);
            c = x < 0 ? -1 : x >= BASE ? 1 : 0;
            n[i] = uint(x - c * BASE);}
        if (c > 0) { n.push_back(1); }
        trim();}
    // inf + x = inf, inf + (-inf) = 0.
    InfIntMini &operator+=(const InfIntMini &o) {
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn != o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (sgn == o.sgn || magCmp(*this, o) >= 0) { addMag(o.n, sgn == o.sgn ? 1 : -1); return *this; }
        InfIntMini t = o; t.addMag(n, -1);
        return *this = std::move(t);}
    InfIntMini &operator-=(const InfIntMini &o) { return *this += -o; }

    // a * b mod P for P = 2^64 - 2^32 + 1, using 2^64 = 2^32 - 1 and 2^96 = -1.
    static ulng mulP(ulng a, ulng b) {
        ulll x = ulll(a) * b; ulng lo = ulng(x), hl = ulng(x >> 64) & 0xFFFFFFFF, hh = ulng(x >> 96);
        ulng t = lo - hh + (lo < hh) * P, u = hl * 0xFFFFFFFF, r = t + u;
        r += (r < t) * 0xFFFFFFFFULL;
        return r >= P ? r - P : r;}
    static ulng powP(ulng a, ulng e) {
        ulng r = 1;
        for (; e; e >>= 1, a = mulP(a, a)) { if (e & 1) { r = mulP(r, a); } }
        return r;}
    // T: O(L * log(L)); length L a power of two at most 2^30, 7 generates the multiplicative group.
    static void ntt(vector<ulng> &a, bool inverse) {
        int L = int(a.size());
        for (int i = 1, j = 0; i < L; i++) {
            int bit = L >> 1;
            for (; j & bit; bit >>= 1) { j ^= bit; }
            j ^= bit;
            if (i < j) { std::swap(a[size_t(i)], a[size_t(j)]); }}
        vector<ulng> w(size_t(max(L / 2, 1)));
        for (int k = 1; k < L; k *= 2) {
            ulng z = powP(7, (P - 1) / ulng(2 * k));
            z = inverse ? powP(z, P - 2) : z; w[0] = 1;
            for (int j = 1; j < k; j++) { w[size_t(j)] = mulP(w[size_t(j - 1)], z); }
            for (int i = 0; i < L; i += 2 * k) {
                for (int j = 0; j < k; j++) {
                    ulng &x = a[size_t(i + j)], &y = a[size_t(i + j + k)], u = x, v = mulP(y, w[size_t(j)]), s = u + v;
                    x = s - ((s < u) | (s >= P)) * P; y = u - v + (u < v) * P;}}}
        if (inverse) { ulng s = powP(ulng(L), P - 2); for (ulng &x : a) { x = mulP(x, s); } }}
    // Schoolbook while n * m <= MUL_NTT * L * bit_width(L) for the transform length L, else the NTT; inf * 0 = 0.
    InfIntMini &operator*=(const InfIntMini &o) {
        if (isNil() || o.isNil()) { return *this = 0; }
        sgn *= o.sgn;
        if (is_inf || o.is_inf) { is_inf = true; n.clear(); return *this; }
        int na = int(n.size()), nb = int(o.n.size()), len = na + nb, L = int(std::bit_ceil(uint(len)));
        assert(len <= (1 << 30));
        vector<ulng> r(size_t(len), 0);
        if (ulng(na) * ulng(nb) <= MUL_NTT * ulng(L) * ulng(std::bit_width(uint(L)))) {
            const vector<uint> &x = na <= nb ? n : o.n, &y = na <= nb ? o.n : n;
            for (size_t i = 0; i < x.size(); i++) { for (size_t j = 0; j < y.size(); j++) { r[i + j] += ulng(x[i]) * y[j]; } }}
        else {
            vector<ulng> fa(n.begin(), n.end()), fb;
            fa.resize(size_t(L)); ntt(fa, false);
            if (this == &o) { fb = fa; }
            else { fb.assign(o.n.begin(), o.n.end()); fb.resize(size_t(L)); ntt(fb, false); }
            for (int i = 0; i < L; i++) { fa[size_t(i)] = mulP(fa[size_t(i)], fb[size_t(i)]); }
            ntt(fa, true);
            std::copy(fa.begin(), fa.begin() + len, r.begin());}
        n.resize(size_t(len)); ulng c = 0;
        for (int i = 0; i < len; i++) { c += r[size_t(i)]; n[size_t(i)] = uint(c % BASE); c /= BASE; }
        trim();
        return *this;}

    // |this| /= d for a word d >= 1; returns the remainder.
    ulng divWord(ulng d) {
        assert(d >= 1);
        d = max<ulng>(d, 1); ulll r = 0;
        for (int i = int(n.size()) - 1; i >= 0; i--) { r = r * BASE + n[size_t(i)]; n[size_t(i)] = uint(r / d); r %= d; }
        trim();
        return ulng(r);}
    // T: O((n - m + 1) * m); |a| / |b| for b of m >= 2 limbs, a becomes |a| % |b|; the estimate from three and two top limbs exceeds the digit by at most 1.
    static InfIntMini divLong(InfIntMini &a, const InfIntMini &b) {
        int m = int(b.n.size()); InfIntMini q; q.n.assign(size_t(max(int(a.n.size()) - m + 1, 1)), 0);
        assert(m >= 2);
        if (m < 2) { return q; }
        a.n.push_back(0);
        lng top = lng(b.n[size_t(m - 1)]) * BASE + b.n[size_t(m - 2)];
        for (int i = int(a.n.size()) - m - 1; i >= 0; i--) {
            auto addMul = [&](lng k) {
                lng c = 0;
                for (int j = 0; j <= m; j++) {
                    c += a.n[size_t(i + j)] + k * (j < m ? b.n[size_t(j)] : 0);
                    lng d = (c % BASE + BASE) % BASE;
                    a.n[size_t(i + j)] = uint(d); c = (c - d) / BASE;}
                return c;};
            lng qh = min(((lng(a.n[size_t(i + m)]) * BASE + a.n[size_t(i + m - 1)]) * BASE + a.n[size_t(i + m - 2)]) / top, lng(BASE - 1));
            if (addMul(-qh) < 0) { qh--; addMul(1); }
            q.n[size_t(i)] = uint(qh);}
        a.trim(); q.trim();
        return q;}
    // Truncating; the remainder keeps the dividend's sign; inf / p = inf, x / inf = 0, inf / inf = 1, inf % p = 0, x % inf = x.
    friend pair<InfIntMini, InfIntMini> divMod(InfIntMini a, InfIntMini b) {
        assert(!b.isNil());
        if (b.isNil()) { return {0, a}; }
        if (a.is_inf || b.is_inf) {
            if (a.is_inf && b.is_inf) { return {a.sgn * b.sgn, 0}; }
            if (b.is_inf) { return {0, a}; }
            a.sgn *= b.sgn;
            return {a, 0};}
        int qs = a.sgn * b.sgn, rs = a.sgn; a.sgn = 1;
        InfIntMini q;
        if (b.n.size() <= 3) {
            ulng d = 0;
            for (int i = int(b.n.size()); i-- > 0;) { d = d * BASE + b.n[size_t(i)]; }
            InfIntMini r = a.divWord(d); q = std::move(a); a = std::move(r);}
        else { q = divLong(a, b); }
        q.sgn = q.isNil() ? 1 : qs; a.sgn = a.isNil() ? 1 : rs;
        return {std::move(q), std::move(a)};}
    InfIntMini &operator/=(InfIntMini o) { return *this = divMod(std::move(*this), std::move(o)).first; }
    InfIntMini &operator%=(InfIntMini o) { return *this = divMod(std::move(*this), std::move(o)).second; }

    InfIntMini operator+() const { return *this; }
    InfIntMini operator-() const { InfIntMini r = *this; if (!r.isNil()) { r.sgn = -r.sgn; } return r; }
    friend InfIntMini operator+(InfIntMini a, const InfIntMini &b) { a += b; return a; }
    friend InfIntMini operator-(InfIntMini a, const InfIntMini &b) { a -= b; return a; }
    friend InfIntMini operator*(InfIntMini a, const InfIntMini &b) { a *= b; return a; }
    friend InfIntMini operator/(InfIntMini a, const InfIntMini &b) { a /= b; return a; }
    friend InfIntMini operator%(InfIntMini a, const InfIntMini &b) { a %= b; return a; }
    friend InfIntMini abs(InfIntMini a) { a.sgn = 1; return a; }

    friend std::strong_ordering magCmp(const InfIntMini &a, const InfIntMini &b) {
        if (a.is_inf || b.is_inf) { return int(a.is_inf) <=> int(b.is_inf); }
        if (a.isNil() || b.isNil()) { return int(!a.isNil()) <=> int(!b.isNil()); }
        if (a.n.size() != b.n.size()) { return a.n.size() <=> b.n.size(); }
        for (size_t i = a.n.size(); i-- > 0;) { if (a.n[i] != b.n[i]) { return a.n[i] <=> b.n[i]; } }
        return std::strong_ordering::equal;}
    friend std::strong_ordering operator<=>(const InfIntMini &a, const InfIntMini &b) {
        int as = a.isNil() ? 0 : a.sgn, bs = b.isNil() ? 0 : b.sgn;
        if (as != bs || as == 0) { return as <=> bs; }
        return as == 1 ? magCmp(a, b) : magCmp(b, a);}
    friend bool operator==(const InfIntMini &a, const InfIntMini &b) { return (a <=> b) == 0; }

    // T: O(M(n * e)); e >= 0, inf^0 = 1.
    friend InfIntMini pow(InfIntMini a, lng e) {
        assert(e >= 0);
        InfIntMini r = 1;
        if (e < 0) { return 0; }
        if (a.is_inf && e > 0) { a.sgn = e & 1 ? a.sgn : 1; return a; }
        for (; e; e >>= 1) {
            if (e & 1) { r *= a; }
            if (e > 1) { a *= a; }}
        return r;}
    // T: O(n^2); nonnegative, gcd(inf, x) = |x| for x != 0 and inf otherwise.
    friend InfIntMini gcd(InfIntMini a, InfIntMini b) {
        a.sgn = b.sgn = 1;
        while (!b.isNil()) { a %= b; std::swap(a, b); }
        return a;}

    struct SetBase { uint base; };
    static int getBaseId() { static const int id = std::ios_base::xalloc(); return id; }
    static uint streamBase(std::ios_base &s) {
        if (s.flags() & std::ios_base::hex) { return 16; }
        if (s.flags() & std::ios_base::oct) { return 8; }
        uint base = uint(s.iword(getBaseId()));
        return base ? base : 10;}
    friend istream &operator>>(istream &is, SetBase sb) { is.iword(getBaseId()) = int(sb.base); return is; }
    friend ostream &operator<<(ostream &os, SetBase sb) { os.iword(getBaseId()) = int(sb.base); return os; }
    // One whitespace-delimited token; a malformed token sets failbit and leaves a unchanged.
    friend istream &operator>>(istream &is, InfIntMini &a) {
        string s;
        if (is >> s && !a.read(s, streamBase(is))) { is.setstate(std::ios_base::failbit); }
        return is;}
    friend ostream &operator<<(ostream &os, const InfIntMini &a) { return os << a.toString(streamBase(os)); }
};
using iintmini = InfIntMini;
