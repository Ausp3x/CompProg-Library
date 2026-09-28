#line 2 "1-Core\\01-template.hpp"

// 知彼知己，百战不殆
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi    first
#define se    second
#define pb    push_back
using uint = uint32_t;
using lng = int64_t;    using ulng = uint64_t;
using lll = __int128_t; using ulll = __uint128_t;
template<typename T> 
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

constexpr int INF32 = 0x3f3f3f3f;
constexpr lng INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T> 
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T> 
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }
#line 3 "1-Core\\06-infintmini.hpp"

// T: O(n) or O(n * log(n)), M: O(n)
struct InfIntMini {
    static constexpr const char *DIG = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr uint BASE = 100000, BLEN = 5;
    static constexpr ulng NTT_P = 18446744069414584321ULL, NTT_G = 7;
    template<typename T>
    static constexpr bool isFastScalar = std::is_integral_v<T> && sizeof(T) <= 8 &&
                                         !std::same_as<std::remove_cv_t<T>, bool>;
    static_assert(ulll(1U << 29) * (BASE - 1) * (BASE - 1) < NTT_P);

    int sgn; bool is_inf = false;
    vector<uint> n;
    // T: O(1)
    InfIntMini() : sgn(1) {}
    // T: O(1)
    template<typename T>
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    InfIntMini(T N) { *this = N; }
    // T: O(n) or O(n^2)
    InfIntMini(std::string_view N, uint base = 10) { read(N, base); }

    // T: O(1)
    explicit operator lng() const {
        if (is_inf) { return sgn == 1 ? LLONG_MAX : LLONG_MIN; }
        if (isNil()) { return 0; }
        ulng res = 0, mul = 1;
        for (int i = 0; i < min(int(n.size()), 13); i++) { res += ulng(n[i]) * mul; mul *= BASE; }
        return sgn == 1 ? lng(res) : lng(-res);}
    // T: O(1)
    explicit operator ulng() const {
        if (is_inf) { return sgn == 1 ? ULLONG_MAX : 0; }
        if (isNil()) { return 0; }
        ulng res = 0, mul = 1;
        for (int i = 0; i < min(int(n.size()), 13); i++) { res += ulng(n[i]) * mul; mul *= BASE; }
        return sgn == 1 ? res : -res;}
    // T: O(n) or O(n^2)
    void read(std::string_view s, uint base = 10) {
        assert(base >= 2 && base <= 36 && "InfIntMini base error: Base must be in [2, 36].");
        sgn = 1; is_inf = false; n.clear();
        if (s.empty()) { *this = 0; return; }
        int pos = 0;
        while (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) {
            if (s[pos++] == '-') { sgn = -sgn; }}
        if (pos == s.size()) { *this = 0; return; }
        if (s.substr(pos) == "INF" || s.substr(pos) == "inf") { is_inf = true; return; }
        int res_sgn = sgn; sgn = 1;
        if (base == 10) {
            n.reserve((s.size() - pos + BLEN - 1) / BLEN);
            for (int i = int(s.size()); i > pos; i -= int(BLEN)) {
                uint cur = 0;
                for (int j = max(pos, i - int(BLEN)); j < i; j++) {
                    cur = 10 * cur + uint(s[j] - '0');}
                n.push_back(cur);}}
        else {
            for (; pos < s.size(); pos++) {
                char c = s[pos]; uint d = 0;
                if ('0' <= c && c <= '9') { d = c - '0'; }
                else if ('A' <= c && c <= 'Z') { d = 10 + (c - 'A'); }
                else if ('a' <= c && c <= 'z') { d = 10 + (c - 'a'); }
                mulMagSmall(base); addMagSmall(d);}}
        trim(); sgn = isNil() ? 1 : res_sgn;}
    // T: O(n) or O(n^2)
    string toString(uint base = 10) const {
        assert(base >= 2 && base <= 36 && "InfIntMini base error: Base must be in [2, 36].");
        if (is_inf) { return sgn == -1 ? "-inf" : "inf"; }
        if (isNil()) { return "0"; }
        if (base == 10) {
            string res; res.reserve(n.size() * BLEN + 1);
            if (sgn == -1) { res += '-'; }
            res += std::to_string(n.back());
            for (int i = int(n.size()) - 2; i >= 0; i--) {
                string cur = std::to_string(n[i]);
                res.append(BLEN - cur.size(), '0'); res += cur;}
            return res;}
        InfIntMini a = *this; a.sgn = 1; string res;
        while (!a.isNil()) { res += DIG[a.divMagSmall(base)]; }
        if (sgn == -1) { res += '-'; }
        reverse(res.begin(), res.end());
        return res;}
    void trim() {
        if (is_inf) { n.clear(); return; }
        if (n.empty()) { n.push_back(0); sgn = 1; return; }
        int pos = int(n.size()) - 1;
        while (pos > 0 && n[pos] == 0) { pos--; }
        if (pos + 1 < n.size()) { n.resize(pos + 1); }
        if (n.size() == 1 && n[0] == 0) { sgn = 1; }}
    // T: O(1)
    template<typename T>
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    InfIntMini &operator=(T N) {
        sgn = N < 0 ? -1 : 1; is_inf = false;
        ulll uN = N < 0 ? -ulll(N) : ulll(N);
        n.clear();
        do { n.push_back(uint(uN % BASE)); uN /= BASE; } while (uN > 0);
        return *this;}
    // T: O(n) or O(n^2)
    InfIntMini &operator=(std::string_view N) { read(N); return *this; }

    void addMagSmall(uint o) {
        if (is_inf || o == 0) { return; }
        ulng carry = ulng(o);
        for (int i = 0; i < n.size() && carry; i++) {
            ulng cur = n[i] + carry;
            n[i] = uint(cur % BASE); carry = cur / BASE;}
        while (carry) { n.push_back(uint(carry % BASE)); carry /= BASE; }}
    void addMag(const vector<uint> &o) {
        if (is_inf || o.empty()) { return; }
        int n_len = int(n.size()), o_len = int(o.size());
        if (n_len < o_len) { n.resize(o_len, 0); n_len = o_len; }
        uint carry = 0;
        for (int i = 0; i < o_len; i++) {
            uint cur = n[i] + o[i] + carry;
            carry = cur >= BASE; n[i] = cur - carry * BASE;}
        for (int i = o_len; i < n_len && carry; i++) {
            if (++n[i] < BASE) { carry = 0; }
            else { n[i] = 0; }}
        if (carry) { n.push_back(1); }}
    void subMag(const vector<uint> &o) {
        if (is_inf || o.empty()) { return; }
        int n_len = int(n.size()), o_len = int(o.size()), borrow = 0;
        for (int i = 0; i < o_len; i++) {
            int cur = int(n[i]) - int(o[i]) - borrow;
            borrow = cur < 0; n[i] = uint(cur + borrow * BASE);}
        for (int i = o_len; i < n_len && borrow; i++) {
            if (n[i] > 0) { n[i]--; borrow = 0; }
            else { n[i] = BASE - 1; }}
        trim();}
    void rsubMag(const vector<uint> &o) {
        if (is_inf || o.empty()) { return; }
        int o_len = int(o.size()), borrow = 0; n.resize(o_len, 0);
        for (int i = 0; i < o_len; i++) {
            int cur = int(o[i]) - int(n[i]) - borrow;
            borrow = cur < 0; n[i] = uint(cur + borrow * BASE);}
        trim();}
    InfIntMini &operator+=(const InfIntMini &o) {
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn != o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { return *this = o; }
        if (sgn == o.sgn) { addMag(o.n); return *this; }
        auto cmp = magCmp(*this, o);
        if (cmp == 0) { return *this = 0; }
        if (cmp > 0) { subMag(o.n); }
        else { rsubMag(o.n); sgn = o.sgn; }
        return *this;}
    InfIntMini &operator-=(const InfIntMini &o) {
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn == o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = -o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { *this = o; sgn = -sgn; return *this; }
        if (sgn != o.sgn) { addMag(o.n); return *this; }
        auto cmp = magCmp(*this, o);
        if (cmp == 0) { return *this = 0; }
        if (cmp > 0) { subMag(o.n); }
        else { rsubMag(o.n); sgn = -sgn; }
        return *this;}

    void mulMagSmall(uint o) {
        if (is_inf || o == 1) { return; }
        if (isNil() || o == 0) { *this = 0; return; }
        ulng carry = 0;
        for (int i = 0; i < n.size(); i++) {
            ulng cur = ulng(n[i]) * o + carry;
            n[i] = uint(cur % BASE); carry = cur / BASE;}
        while (carry) { n.push_back(uint(carry % BASE)); carry /= BASE; }}
    // T: O(n^2)
    static void mulMag(std::span<const uint> a, std::span<const uint> b, vector<uint> &res) {
        if (a.size() > b.size()) { std::swap(a, b); }
        assert(ulll(a.size()) * BASE * (BASE - 1) < (ulll(1) << 64) &&
               "InfIntMini multiplication error: Raw coefficient overflow.");
        static vector<ulng> cur; cur.assign(a.size() + b.size(), 0);
        for (int i = 0; i < a.size(); i++) {
            ulng a_i = ulng(a[i]);
            for (int j = 0; j < b.size(); j++) { cur[i + j] += a_i * b[j]; }}
        res.resize(cur.size()); ulng carry = 0;
        for (int i = 0; i < cur.size(); i++) {
            carry += cur[i]; res[i] = uint(carry % BASE); carry /= BASE;}
        assert(carry == 0);
        while (res.size() > 1 && res.back() == 0) { res.pop_back(); }}
    // T: O(1)
    template<typename T> requires isFastScalar<T>
    static pair<int, ulng> sgnMag(T o) {
        using U = std::make_unsigned_t<std::remove_cv_t<T>>;
        if constexpr (std::is_signed_v<std::remove_cv_t<T>>) {
            if (o < 0) { return {-1, ulng(U(0) - U(o))}; }}
        return {1, ulng(U(o))};}
    template<typename T> requires isFastScalar<T>
    InfIntMini &operator*=(T o) {
        auto [o_sgn, uo] = sgnMag(o);
        if (isNil() || uo == 0) { return *this = 0; }
        if (is_inf) { sgn *= o_sgn; n.clear(); return *this; }
        sgn *= o_sgn;
        if (uo == 1) { return *this; }
        if (uo <= 0xFFFFFFFFULL) { mulMagSmall(uint(uo)); return *this; }
        int len = 0; array<uint, 4> digs;
        do { digs[len++] = uint(uo % BASE); uo /= BASE; } while (uo > 0);
        mulMag(n, std::span<const uint>(digs.data(), len), n);
        return *this;}
    // T: O(1)
    static inline ulng modP(ulll a) {
        ulng l = ulng(a), hl = uint(a >> 64), hh = ulng(a >> 96);
        ulll cur = ulll(l) + (ulll(hl) << 32); ulng sub = hl + hh;
        cur += (cur < sub) * NTT_P; cur -= sub;
        ulng res = ulng(cur);
        res += ulng(0xFFFFFFFFULL * (cur >> 64)); res -= (res >= NTT_P) * NTT_P;
        return res;}
    // T: O(1)
    static ulng nttPow(ulng a, ulng b) {
        if (b == 0) { return 1; }
        a = modP(a); ulng res = a;
        for (int i = std::bit_width(b) - 2; i >= 0; i--) {
            res = modP(ulll(res) * res);
            if ((b >> i) & 1) { res = modP(ulll(res) * a); }}
        return res;}
    static void ntt(vector<ulng> &a, bool is_inv) {
        assert(!a.empty() && std::has_single_bit(a.size()) && a.size() <= (1U << 30) &&
               "InfIntMini NTT error: Length must be a power of two at most 2^30.");
        int n = int(a.size());
        static vector<ulng> w = {0, 1};
        if (w.size() < n) {
            int m = int(w.size()); w.resize(n);
            for (int len = m; len < n; len <<= 1) {
                ulng w0 = nttPow(NTT_G, (NTT_P - 1) / (2 * len));
                for (int i = len / 2; i < len; i++) {
                    w[2 * i] = w[i];
                    w[2 * i + 1] = modP(ulll(w[i]) * w0);}}}
        if (!is_inv) {
            for (int len = n / 2; len > 0; len >>= 1) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; j++) {
                        ulng u = a[i + j], v = a[i + j + len], uv = u + v;
                        a[i + j]       = uv - ((uv < u) | (uv >= NTT_P)) * NTT_P;
                        a[i + j + len] = modP(ulll(u - v + (u < v) * NTT_P) * w[len + j]);}}}}
        else {
            for (int len = 1; len < n; len <<= 1) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; j++) {
                        ulng iw = j == 0 ? 1 : NTT_P - w[2 * len - j];
                        ulng u = a[i + j], v = modP(ulll(a[i + j + len]) * iw), uv = u + v;
                        a[i + j]       = uv - ((uv < u) | (uv >= NTT_P)) * NTT_P;
                        a[i + j + len] = u - v + (u < v) * NTT_P;}}}
            ulng n_inv = NTT_P - ((NTT_P - 1) >> std::countr_zero(uint(n)));
            for (ulng &x : a) { x = modP(ulll(x) * n_inv); }}}
    InfIntMini &operator*=(const InfIntMini &o) {
        if (isNil() || o.isNil()) { return *this = 0; }
        if (is_inf || o.is_inf) { sgn *= o.sgn; is_inf = true; n.clear(); return *this; }
        if (o.n.size() == 1) {
            int o_sgn = o.sgn;
            mulMagSmall(o.n[0]); sgn *= o_sgn;
            return *this;}
        if (n.size() == 1) {
            int a_sgn = sgn; uint ua = n[0];
            *this = o; mulMagSmall(ua); sgn *= a_sgn;
            return *this;}
        int n_len = int(n.size()), o_len = int(o.n.size());
        bool is_square = this == &o; sgn *= o.sgn;
        bool use_naive = min(n_len, o_len) < 64;
        assert(ulng(n_len) + o_len - 1 <= (1U << 30) && "InfIntMini multiplication error: Number is too large.");
        int req = n_len + o_len - 1, len = int(std::bit_ceil(uint(req)));
        if (!use_naive && !is_square && n_len == o_len) { is_square = n == o.n; }
        use_naive |= ulng(n_len) * o_len <=
                     (30ULL - 10ULL * is_square) * len * std::countr_zero(uint(len));
        if (use_naive) { mulMag(n, o.n, n); return *this; }
        static vector<ulng> fa, fb;
        fa.resize(len); std::copy(n.begin(), n.end(), fa.begin());
        std::fill(fa.begin() + n_len, fa.end(), 0);
        ntt(fa, false);
        if (is_square) {
            for (int i = 0; i < len; i++) { fa[i] = modP(ulll(fa[i]) * fa[i]); }}
        else {
            fb.resize(len); std::copy(o.n.begin(), o.n.end(), fb.begin());
            std::fill(fb.begin() + o_len, fb.end(), 0);
            ntt(fb, false);
            for (int i = 0; i < len; i++) { fa[i] = modP(ulll(fa[i]) * fb[i]); }}
        ntt(fa, true);
        n.resize(req);
        ulng carry = 0;
        for (int i = 0; i < req; i++) {
            carry += fa[i]; n[i] = uint(carry % BASE); carry /= BASE;}
        while (carry) { n.push_back(uint(carry % BASE)); carry /= BASE; }
        trim();
        return *this;}

    uint divMagSmall(uint o) {
        assert(o > 0 && "InfIntMini divMagSmall error: Invalid divisor.");
        if (is_inf || o == 1) { return 0; }
        ulng rem = 0, mgc = ULLONG_MAX / o + 1;
        for (int i = int(n.size()) - 1; i >= 0; i--) {
            ulng cur = n[i] + rem * BASE;
            ulng qi = ulng(ulll(cur) * mgc >> 64), ri = cur - qi * o;
            if (ri >= o) { qi--; ri += o; }
            n[i] = uint(qi); rem = ri;}
        trim();
        return uint(rem);}
    template<typename T> requires isFastScalar<T>
    InfIntMini &operator/=(T o) {
        assert(o != 0 && "InfIntMini scalar division error: Division by zero.");
        auto [o_sgn, uo] = sgnMag(o);
        if (is_inf) { sgn *= o_sgn; n.clear(); return *this; }
        if (isNil()) { return *this; }
        sgn *= o_sgn;
        if (uo == 1) { return *this; }
        ulng rem = 0;
        if (uo <= ULLONG_MAX / BASE) {
            ulng mgc = ULLONG_MAX / uo + 1;
            for (int i = int(n.size()) - 1; i >= 0; i--) {
                ulng cur = n[i] + rem * BASE;
                ulng qi = ulng(ulll(cur) * mgc >> 64), ri = cur - qi * uo;
                if (ri >= uo) { qi--; ri += uo; }
                n[i] = uint(qi); rem = ri;}}
        else {
            static constexpr ulng BASE3 = ulng(BASE) * BASE * BASE;
            int i = int(n.size()) - 1, lead = int(n.size() % 3);
            while (lead--) { rem = rem * BASE + n[i]; n[i--] = 0; }
            for (; i >= 2; i -= 3) {
                ulng blk = (ulng(n[i]) * BASE + n[i - 1]) * BASE + n[i - 2];
                ulll cur = ulll(rem) * BASE3 + blk;
                ulng qi = ulng(cur / uo); rem = ulng(cur % uo);
                n[i - 2] = uint(qi % BASE); qi /= BASE;
                n[i - 1] = uint(qi % BASE); n[i] = uint(qi / BASE);}}
        trim();
        return *this;}
    template<typename T> requires isFastScalar<T>
    InfIntMini &operator%=(T o) {
        assert(o != 0 && "InfIntMini scalar modulo error: Modulo by zero.");
        ulng uo = sgnMag(o).second;
        if (is_inf) { return *this = 0; }
        if (isNil() || uo == 1) { return *this = 0; }
        int a_sgn = sgn; ulng rem = 0;
        if (uo <= ULLONG_MAX / BASE) {
            ulng mgc = ULLONG_MAX / uo + 1;
            for (int i = int(n.size()) - 1; i >= 0; i--) {
                ulng cur = rem * BASE + n[i];
                ulng qi = ulng(ulll(cur) * mgc >> 64);
                rem = cur - qi * uo;
                if (rem >= uo) { rem += uo; }}}
        else {
            static constexpr ulng BASE3 = ulng(BASE) * BASE * BASE;
            int i = int(n.size()) - 1, lead = int(n.size() % 3);
            while (lead--) { rem = rem * BASE + n[i--]; }
            for (; i >= 2; i -= 3) {
                ulng blk = (ulng(n[i]) * BASE + n[i - 1]) * BASE + n[i - 2];
                rem = ulng((ulll(rem) * BASE3 + blk) % uo);}}
        *this = rem;
        if (a_sgn == -1 && !isNil()) { sgn = -1; }
        return *this;}
    // T: O(n^2)
    friend pair<InfIntMini, InfIntMini> divModSlow(InfIntMini a, InfIntMini b) {
        assert(!b.isNil() && "InfIntMini divMod error: Division by zero.");
        if (a.is_inf || b.is_inf) {
            if (a.is_inf && b.is_inf) { return {a.sgn * b.sgn, 0}; }
            if (b.is_inf) { return {0, std::move(a)}; }
            a.sgn *= b.sgn;
            return {std::move(a), 0};}
        int a_sgn = a.sgn, b_sgn = b.sgn;
        a.sgn = 1; b.sgn = 1;
        auto cmp = magCmp(a, b);
        if (cmp < 0) { a.sgn = a_sgn; a.trim(); return {0, std::move(a)}; }
        if (cmp == 0) { return {a_sgn * b_sgn, 0}; }
        if (b.n.size() == 1) {
            InfIntMini rem = a.divMagSmall(b.n[0]);
            a.sgn = a.isNil() ? 1 : a_sgn * b_sgn;
            if (a_sgn == -1 && rem > 0) { rem.sgn = -1; }
            return {std::move(a), std::move(rem)};}
        uint norm = BASE / (b.n.back() + 1);
        a.mulMagSmall(norm); b.mulMagSmall(norm);
        int n = int(a.n.size()), m = int(b.n.size()); a.n.push_back(0);
        ulng mgc = ULLONG_MAX / b.n.back() + 1;
        InfIntMini q; q.n.assign(n - m + 1, 0);
        for (int i = n - m; i >= 0; i--) {
            ulng num = ulng(a.n[i + m]) * BASE + a.n[i + m - 1];
            ulng d = ulng(ulll(num) * mgc >> 64), rem = num - d * b.n.back();
            if (rem >= b.n.back()) { d--; rem += b.n.back(); }
            if (d >= BASE) { d = BASE - 1; rem = num - d * b.n.back(); }
            while (rem < BASE && d * b.n[m - 2] > rem * BASE + a.n[i + m - 2]) {
                d--; rem += b.n.back();}
            ulng carry = 0;
            for (int j = 0; j < m; j++) {
                ulng cur = d * b.n[j] + carry;
                carry = cur / BASE; uint sub = uint(cur % BASE);
                if (a.n[i + j] < sub) { a.n[i + j] += BASE - sub; carry++; }
                else { a.n[i + j] -= sub; }}
            bool neg = a.n[i + m] < carry;
            a.n[i + m] = uint(a.n[i + m] + neg * BASE - carry);
            if (neg) {
                d--; carry = 0;
                for (int j = 0; j < m; j++) {
                    ulng cur = ulng(a.n[i + j]) + b.n[j] + carry;
                    carry = cur / BASE; a.n[i + j] = uint(cur % BASE);}
                a.n[i + m] = uint((a.n[i + m] + carry) % BASE);}
            q.n[i] = uint(d);}
        q.sgn = a_sgn * b_sgn; q.trim();
        a.n.resize(m); a.trim(); a.divMagSmall(norm);
        if (a_sgn == -1 && !a.isNil()) { a.sgn = -1; }
        return {std::move(q), std::move(a)};}
    // T: O(n^2)
    friend pair<InfIntMini, InfIntMini> divMod(InfIntMini a, InfIntMini b) {
        return divModSlow(std::move(a), std::move(b));}
    // T: O(n^2)
    InfIntMini &operator/=(InfIntMini o) {
        assert(!o.isNil() && "InfIntMini division error: Division by zero.");
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf) { return *this = sgn * o.sgn; }
            if (o.is_inf) { return *this = 0; }
            sgn *= o.sgn; n.clear();
            return *this;}
        if (isNil()) { return *this; }
        if (o.n.size() <= 3) {
            int o_sgn = o.sgn; ulng uo = 0;
            for (int i = int(o.n.size()) - 1; i >= 0; i--) { uo = uo * BASE + o.n[i]; }
            *this /= uo;
            if (o_sgn == -1 && !isNil()) { sgn = -sgn; }
            return *this;}
        return *this = divMod(std::move(*this), std::move(o)).first;}
    // T: O(n^2)
    InfIntMini &operator%=(InfIntMini o) {
        assert(!o.isNil() && "InfIntMini modulo error: Modulo by zero.");
        if (is_inf) { return *this = 0; }
        if (o.is_inf) { return *this; }
        if (isNil()) { return *this; }
        if (o.n.size() <= 3) {
            ulng uo = 0;
            for (int i = int(o.n.size()) - 1; i >= 0; i--) { uo = uo * BASE + o.n[i]; }
            return *this %= uo;}
        return *this = divMod(std::move(*this), std::move(o)).second;}

    InfIntMini operator+() const { return *this; }
    InfIntMini operator-() const & {
        InfIntMini res = *this;
        if (!res.isNil()) { res.sgn = -res.sgn; }
        return res;}
    // T: O(1)
    InfIntMini operator-() && {
        if (!isNil()) { sgn = -sgn; }
        return std::move(*this);}
    friend InfIntMini operator+(InfIntMini a, const InfIntMini &b) { a += b; return a; }
    friend InfIntMini operator-(InfIntMini a, const InfIntMini &b) { a -= b; return a; }
    template<typename T> requires isFastScalar<T>
    friend InfIntMini operator*(InfIntMini a, T b) { a *= b; return a; }
    template<typename T> requires isFastScalar<T>
    friend InfIntMini operator*(T b, InfIntMini a) { a *= b; return a; }
    friend InfIntMini operator*(InfIntMini a, const InfIntMini &b) { a *= b; return a; }
    template<typename T> requires isFastScalar<T>
    friend InfIntMini operator/(InfIntMini a, T b) { a /= b; return a; }
    // T: O(n^2)
    friend InfIntMini operator/(InfIntMini a, InfIntMini b) { a /= std::move(b); return a; }
    template<typename T> requires isFastScalar<T>
    friend InfIntMini operator%(InfIntMini a, T b) { a %= b; return a; }
    // T: O(n^2)
    friend InfIntMini operator%(InfIntMini a, InfIntMini b) { a %= std::move(b); return a; }

    friend InfIntMini abs(InfIntMini a) { a.sgn = 1; return a; }

    // T: O(1)
    bool isNil() const { return !is_inf && (n.empty() || (n.size() == 1 && n[0] == 0)); }
    // T: O(1)
    explicit operator bool() const { return !isNil(); }
    // T: O(1)
    bool operator!() const { return isNil(); }
    friend std::strong_ordering magCmp(const InfIntMini &a, const InfIntMini &b) {
        if (a.is_inf && b.is_inf) { return std::strong_ordering::equal; }
        if (a.is_inf || b.is_inf) { return a.is_inf ? std::strong_ordering::greater : std::strong_ordering::less; }
        if (a.isNil() || b.isNil()) { return int(!a.isNil()) <=> int(!b.isNil()); }
        if (a.n.size() != b.n.size()) { return a.n.size() <=> b.n.size(); }
        for (int i = int(a.n.size()) - 1; i >= 0; i--) {
            if (a.n[i] != b.n[i]) { return a.n[i] <=> b.n[i]; }}
        return std::strong_ordering::equal;}
    friend std::strong_ordering operator<=>(const InfIntMini &a, const InfIntMini &b) {
        if (a.is_inf && b.is_inf) { return a.sgn <=> b.sgn; }
        if (a.is_inf || b.is_inf) { return a.sgn * a.is_inf <=> b.sgn * b.is_inf; }
        if (a.isNil() || b.isNil()) { return int(!a.isNil()) * a.sgn <=> int(!b.isNil()) * b.sgn; }
        if (a.sgn != b.sgn) { return a.sgn <=> b.sgn; }
        return a.sgn == 1 ? magCmp(a, b) : magCmp(b, a);}
    friend bool operator==(const InfIntMini &a, const InfIntMini &b) {
        if (a.is_inf || b.is_inf) { return a.sgn == b.sgn && a.is_inf == b.is_inf; }
        if (a.isNil() && b.isNil()) { return true; }
        return a.sgn == b.sgn && a.n == b.n;}

    // T: O(1)
    struct SetBase { uint base; SetBase(uint Base) : base(Base) {} };
    // T: O(1)
    static int getBaseId() { static const int id = std::ios_base::xalloc(); return id; }
    // T: O(1)
    friend istream &operator>>(istream &is, SetBase sb) { is.iword(getBaseId()) = long(sb.base); return is; }
    // T: O(n) or O(n^2)
    friend istream &operator>>(istream &is, InfIntMini &a) {
        static string s; s.clear();
        uint base = uint(is.iword(getBaseId()));
        base = base == 0 ? 10 : base;
        if (is.flags() & std::ios_base::oct) { base = 8; }
        else if (is.flags() & std::ios_base::hex) { base = 16; }
        if (is >> s) { a.read(s, base); }
        return is;}
    // T: O(1)
    friend ostream &operator<<(ostream &os, SetBase sb) { os.iword(getBaseId()) = long(sb.base); return os; }
    // T: O(n) or O(n^2)
    friend ostream &operator<<(ostream &os, const InfIntMini &a) {
        uint base = uint(os.iword(getBaseId()));
        base = base == 0 ? 10 : base;
        if (os.flags() & std::ios_base::oct) { base = 8; }
        else if (os.flags() & std::ios_base::hex) { base = 16; }
        return os << a.toString(base);}
};
using iint = InfIntMini;
#line 2 "X-Tests Abbreviated\\Yosupo-BigInteger-03-DivBigInt_infintmini.cpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iint a, b;
        cin >> a >> b;
        auto [q, r] = divMod(a, b);
        cout << q << ' ' << r << '\n';
    }

    return 0;
}
