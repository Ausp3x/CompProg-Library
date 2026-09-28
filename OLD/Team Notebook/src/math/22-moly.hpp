#pragma once
#include "../1-Core/01-template.hpp"
#include "../1-Core/03-modint.hpp"

// T: O(n) or O(n * log(n)), M: O(n)
struct Moly {
    static constexpr int NTT_P = 998'244'353; 
    static constexpr int NTT_G = 3;
    using mint = ModInt<NTT_P>;

    vector<mint> v; // ascending powers
    Moly(int n = 1) : v(max(n, 1), mint::init(0)) {}
    Moly(const vector<mint> &a) : v(a) { trim(); }
    Moly(vector<mint> &&a) noexcept : v(std::move(a)) { trim(); }

    mint &operator[](int i) { return v[i]; }
    mint operator[](int i) const { return v[i]; }
    int deg() const { return v.size() - 1; }
    mint lead() const { return v.empty() ? mint::init(0) : v.back(); }
    int size() const { return v.size(); }
    void trim() {
        while (v.size() > 1 && v.back() == mint::init(0)) { v.pop_back(); }
        if (v.empty()) { v.push_back(mint::init(0)); }}
    
    Moly &operator+=(const Moly &o) {
        if (o.size() > size()) { v.resize(o.size(), mint::init(0)); }
        for (int i = 0; i < o.size(); i++) { v[i] += o[i]; }
        trim();
        return *this;}
    Moly &operator-=(const Moly &o) {
        if (o.size() > size()) { v.resize(o.size(), mint::init(0)); }
        for (int i = 0; i < o.size(); i++) { v[i] -= o[i]; }
        trim();
        return *this;}
    Moly &operator*=(mint o) { 
        if (o.n == 0) { v = {mint::init(0)}; return *this; }
        for (mint &x : v) { x *= o; }
        trim();
        return *this;}
    static void ntt(vector<mint> &a, bool is_inv) {
        #ifdef __AVX2__
        FastMoly::ntt(a, is_inv);
        #else 
        int n = a.size();
        for (int i = 1, j = 0; i < n; i++) {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1) { j ^= bit; }
            j ^= bit;
            if (i < j) { std::swap(a[i], a[j]); }}
        static vector<mint> w = {mint::init(0), mint::init(1)};
        if (w.size() < n) {
            int m = w.size(); w.resize(n);
            for (int len = m; len < n; len <<= 1) {
                mint w0 = pow(mint(NTT_G), (NTT_P - 1) / (2 * len));
                for (int i = len / 2; i < len; i++) {
                    w[2 * i] = w[i]; w[2 * i + 1] = w[i] * w0;}}}
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    uint u = a[i + j].n;
                    uint v = 1ULL * a[i + j + len].n * w[j + len].n % NTT_P;
                    a[i + j].n       = u + v - (u + v >= NTT_P) * NTT_P;
                    a[i + j + len].n = u - v + (u < v) * NTT_P;}}}
        if (is_inv) {
            reverse(a.begin() + 1, a.end());
            mint n_inv = inv(mint(n));
            for (mint &x : a) { x *= n_inv; }}
        #endif
    }
    Moly &operator*=(const Moly &o) {
        if (isNil() || o.isNil()) { v = {mint::init(0)}; return *this; }
        if (size() == 1) {
            mint v0 = v[0]; v = o.v;
            for (auto &x : v) { x *= v0; }
            trim();
            return *this;}
        if (o.size() == 1) {
            for (mint &x : v) { x *= o[0]; }
            trim();
            return *this;}
        int n = 1;
        while (n < size() + o.size() - 1) { n <<= 1; }
        static vector<mint> fa, fb;
        fa.assign(v.begin(), v.end());     fa.resize(n, mint::init(0));
        fb.assign(o.v.begin(), o.v.end()); fb.resize(n, mint::init(0));
        ntt(fa, false); ntt(fb, false);
        for (int i = 0; i < n; i++) { fa[i] *= fb[i]; }
        ntt(fa, true);
        v.assign(fa.begin(), fa.begin() + size() + o.size() - 1);
        trim();
        return *this;}
    friend pair<Moly, Moly> divMod(Moly a, const Moly &b) {
        if (a.size() < b.size()) { return {Moly({mint::init(0)}), std::move(a)}; }        
        int len = a.size() - b.size() + 1; Moly a_rev(len), b_rev(len);
        for (int i = 0; i < min(len, a.size()); i++) { a_rev[i] = a[a.size() - i - 1]; }
        for (int i = 0; i < min(len, b.size()); i++) { b_rev[i] = b[b.size() - i - 1]; }
        Moly q = std::move(a_rev) * invS(b_rev, len - 1);
        q.v.resize(len); reverse(q.v.begin(), q.v.end()); q.trim();
        Moly r = std::move(a); r -= q * b; r.trim();
        return {std::move(q), std::move(r)};}
    Moly &operator/=(const Moly &o) {
        if (size() < o.size()) { v = {mint::init(0)}; return *this; }
        return *this = divMod(std::move(*this), o).first;}
    Moly &operator%=(const Moly &o) {
        if (size() < o.size()) { return *this; }
        return *this = divMod(std::move(*this), o).second;}
    
    friend Moly operator+(Moly a) { return a; }
    friend Moly operator-(Moly a) { for (mint &x : a.v) { x = -x; } return a; }
    friend Moly operator+(Moly a, const Moly &b) { a += b; return a; }
    friend Moly operator-(Moly a, const Moly &b) { a -= b; return a; }
    friend Moly operator*(Moly a, mint b) { a *= b; return a; }
    friend Moly operator*(mint b, Moly a) { a *= b; return a; }
    friend Moly operator*(Moly a, const Moly &b) { a *= b; return a; }
    friend Moly operator/(Moly a, const Moly &b) { a /= b; return a; }
    friend Moly operator%(Moly a, const Moly &b) { a %= b; return a; }
    
    // Polynomial Ring Operations
    friend mint eval(const Moly &a, mint c) { // A(c)
        uint res = 0;
        for (int i = a.size() - 1; i >= 0; i--) { res = (1ULL * res * c.n + a[i].n) % NTT_P; }
        return mint::init(res);}
    friend vector<mint> chirpZ(const Moly &a, mint c, mint r, int m) { // A(c * r^i), i ∈ [0, m)
        if (a.isNil() || m <= 0) { return vector<mint>(max(m, 0), mint::init(0)); }
        if (r == mint::init(0)) { vector<mint> res(m, a[0]); res[0] = eval(a, c); return res; }
        int n = a.size();
        mint c_pow = mint::init(1), r_cur  = mint::init(1), r_pow  = mint::init(1);
        mint r_inv = inv(r),        r_icur = mint::init(1), r_ipow = mint::init(1);
        Moly A(n), B(n + m - 1);
        for (int i = 0; i < n; i++) {
            A.v[n - i - 1] = a[i] * c_pow * r_ipow; c_pow *= c; 
            r_ipow *= r_icur; r_icur *= r_inv;}
        r_cur  = mint::init(1), r_pow  = mint::init(1);
        for (int i = 0; i < n + m - 1; i++) { B.v[i] = r_pow; r_pow *= r_cur; r_cur *= r; }
        A *= B;
        if (A.size() < n + m - 1) { A.v.resize(n + m - 1, mint::init(0)); }
        A.v.erase(A.v.begin(), A.v.begin() + n - 1); A.v.resize(m);
        r_icur = mint::init(1), r_ipow = mint::init(1);
        for (int i = 0; i < m; i++) { 
            A.v[i] *= r_ipow;
            r_ipow *= r_icur; r_icur *= r_inv;}
        return std::move(A.v);}
    // T: O(n * log(n)^2)
    friend vector<mint> eval(const Moly &a, const vector<mint> &c) { // A(c_i), i ∈ [0, m)
        if (c.empty()) { return {}; }
        int m = c.size(); vector<Moly> segt(4 * m);
        auto build = [&](auto &&build, int i, int l, int r) -> void {
            if (l == r) { segt[i] = Moly({-c[l], mint::init(1)}); return; }
            int md = std::midpoint(l, r);
            build(build, 2 * i,     l,      md);
            build(build, 2 * i + 1, md + 1, r);
            segt[i] = segt[2 * i] * segt[2 * i + 1];};
        build(build, 1, 0, m - 1);
        vector<mint> res(m);
        auto solve = [&](auto &&solve, int i, int l, int r, Moly p) -> void {
            p %= segt[i];
            if (p.isNil()) { for (int j = l; j <= r; j++) { res[j] = mint::init(0); } return; }
            if (l == r) { res[l] = p[0]; return; }
            int md = std::midpoint(l, r);
            solve(solve, 2 * i,     l,      md, p);
            solve(solve, 2 * i + 1, md + 1, r,  std::move(p));};
        solve(solve, 1, 0, m - 1, a);
        return res;}
    // T: O(n^2)
    friend Moly exGcd(Moly a, Moly b, Moly &x, Moly &y) {
        x = Moly({mint::init(1)}); y = Moly({mint::init(0)});
        Moly X({mint::init(0)}), Y({mint::init(1)});
        while (!b.isNil()) {
            int na = a.size() - 1, nb = b.size() - 1;
            mint b_inv = inv(b.lead()); Moly q(na - nb + 1);
            for (int i = na; i >= nb; i--) {
                if (a[i].n == 0) { continue; }
                mint a_i = a[i] * b_inv; q[i - nb] = a_i;
                for (int j = 0; j <= nb; j++) { a[i - nb + j] -= a_i * b[j]; }}
            a.trim(); q.trim(); std::swap(a, b);
            std::swap(x, X); X -= q * x;
            std::swap(y, Y); Y -= std::move(q) * y;}
        if (!a.isNil()) { 
            mint a_inv = inv(a.lead()); a *= a_inv; x *= a_inv; y *= a_inv;}
        return a;}
    // T: O(n^2)
    friend Moly gcd(Moly a, Moly b) {
        while (!b.isNil()) {
            mint b_inv = inv(b.lead());
            for (int i = a.size() - 1, nb = b.size() - 1; i >= nb; i--) {
                if (a[i].n == 0) { continue; }
                mint a_i = a[i] * b_inv;
                for (int j = 0; j <= nb; j++) { a[i - nb + j] -= a_i * b[j]; }}
            a.trim(); std::swap(a, b);}
        if (!a.isNil()) { a *= inv(a.lead()); }
        return a;}
    // T: O(n^2)
    friend Moly invP(Moly a, const Moly &mod) {
        a %= mod;
        if (a.isNil()) { return Moly({mint::init(-1)}); }
        Moly x, y, g = exGcd(std::move(a), mod, x, y);
        if (g.size() != 1 || g.isNil()) { return Moly({mint::init(-1)}); }
        x %= mod; 
        return x;}
    // T: O(n^2)
    friend Moly lcm(Moly a, Moly b) {
        if (a.isNil() || b.isNil()) { return Moly({mint::init(0)}); }
        Moly g = gcd(a, b), res = (std::move(a) / g) * b;
        if (!res.isNil()) { res *= inv(res.lead()); }
        return res;}
    friend Moly powP(Moly a, lng b) {
        Moly res({mint::init(1)});
        while (b > 0) {
            if (b & 1) { res *= a; } 
            b >>= 1; 
            if (b > 0) { a *= a; }}
        return res;}
    friend Moly powMod(Moly a, lng b, const Moly &mod) {
        a %= mod; Moly res({mint::init(1)}); 
        while (b > 0) {
            if (b & 1) { res *= a; res %= mod; }
            b >>= 1;
            if (b > 0) { a *= a; a %= mod; }}
        return res;}
    friend Moly shift(const Moly &a, mint b) { // Taylor Shift: A(x + b)
        if (a.size() <= 1 || b.n == 0) { return a; }
        int n = a.size();
        static vector<mint> inv = {mint::init(0), mint::init(1)};
        static vector<mint> fac = {mint::init(1), mint::init(1)};
        static vector<mint> ifc = {mint::init(1), mint::init(1)};
        for (int i = inv.size(); i < n; i++) {
            inv.push_back(mint::init(NTT_P - NTT_P / i) * inv[NTT_P % i]);
            fac.push_back(fac.back() * mint::init(i));
            ifc.push_back(ifc.back() * inv.back());}
        mint b_pow = mint::init(1); Moly A(n), B(n);
        for (int i = 0; i < n; i++) { 
            A[n - i - 1] = a[i] * fac[i]; 
            B[i] = b_pow * ifc[i]; b_pow *= b;}
        A *= B;
        A.v.resize(n, mint::init(0));
        std::reverse(A.v.begin(), A.v.end());
        for (int i = 0; i < n; i++) { A[i] *= ifc[i]; }
        A.trim();
        return A;}
    // T: O(n * log(n)^2)
    static Moly inter(const vector<mint> &x, const vector<mint> &y) { // Lagrange Interpolation
        assert(x.size() == y.size());
        if (x.empty()) { return Moly({mint::init(0)}); }
        int n = x.size(); vector<Moly> segt(4 * n);
        auto build = [&](auto &&build, int i, int l, int r) -> void {
            if (l == r) { segt[i] = Moly({-x[l], mint::init(1)}); return; }
            int md = std::midpoint(l, r);
            build(build, 2 * i,     l,      md);
            build(build, 2 * i + 1, md + 1, r);
            segt[i] = segt[2 * i] * segt[2 * i + 1];};
        build(build, 1, 0, n - 1);
        vector<mint> dM(n);
        auto calc = [&](auto &&calc, int i, int l, int r, Moly p) -> void {
            p %= segt[i]; 
            if (p.isNil()) { for (int j = l; j <= r; j++) { dM[j] = mint::init(0); } return; }
            if (l == r) { dM[l] = p[0]; return; }
            int md = std::midpoint(l, r);
            calc(calc, 2 * i,     l,      md, p);
            calc(calc, 2 * i + 1, md + 1, r,  std::move(p));};
        calc(calc, 1, 0, n - 1, deriv(segt[1]));
        auto solve = [&](auto &&solve, int i, int l, int r) -> Moly {
            if (l == r) { return Moly({y[l] * inv(dM[l])}); }
            int md = std::midpoint(l, r);
            Moly l_res = solve(solve, 2 * i,     l,      md);
            Moly r_res = solve(solve, 2 * i + 1, md + 1, r);
            l_res *= segt[2 * i + 1]; r_res *= segt[2 * i]; l_res += r_res;
            return l_res;};
        return solve(solve, 1, 0, n - 1);}
    
    // Formal Power Series Operations
    // T: O(n^2 + n^{3/2} * log(n))
    friend Moly comp(const Moly &a, const Moly &b, int deg) { // Brent-Kung: A(B(x)) mod x^{deg + 1}
        if (a.isNil()) { return Moly({mint::init(0)}); }
        int n = min(int(a.size()), deg + 1), m = std::sqrt(n) + 1;
        vector<Moly> b_pow(m + 1); b_pow[0] = Moly({mint::init(1)});
        for (int i = 1; i <= m; i++) {
            b_pow[i] = b_pow[i - 1] * b;
            if (b_pow[i].size() > deg + 1) { b_pow[i].v.resize(deg + 1); }
            b_pow[i].trim();}
        Moly res, cur;
        for (int i = (n - 1) / m; i >= 0; i--) {
            cur.v.clear();
            for (int j = 0; j <= min(n - i * m - 1, m - 1); j++) {
                if (a[i * m + j].n == 0) { continue; }
                if (cur.size() < b_pow[j].size()) { 
                    cur.v.resize(b_pow[j].size(), mint::init(0));}
                for (int k = 0; k < b_pow[j].size(); k++) {
                    cur[k] += b_pow[j][k] * a[i * m + j];}}
            cur.trim();
            if (!res.isNil()) {
                res = std::move(res) * b_pow[m];
                if (res.size() > deg + 1) { res.v.resize(deg + 1); }}
            if (res.size() < cur.size()) { res.v.resize(cur.size(), mint::init(0)); }
            for (int j = 0; j < cur.size(); j++) { res[j] += cur[j]; }
            res.trim();}
        if (res.size() > deg + 1) { res.v.resize(deg + 1); }
        res.trim();
        return res;}
    friend Moly deriv(Moly a) {
        if (a.size() <= 1) { return Moly({mint::init(0)}); }
        for (int i = 1; i < a.size(); i++) { a[i - 1] = a[i] * mint::init(i); }
        a.v.pop_back(); a.trim();
        return a;}
    friend Moly exp(const Moly &a, int deg) { // e^A(x) mod x^{deg + 1}
        assert(a.isNil() || a[0].n == 0);
        Moly res({mint::init(1)});
        for (int len = 2; len < 2 * deg + 2; len <<= 1) {
            Moly cur = log(res, len - 1); cur.v.resize(len, mint::init(0));
            for (int i = 0; i < len; i++) {
                cur[i] = (i < a.size() ? a[i] : mint::init(0)) - cur[i];}
            cur[0] += mint::init(1);
            res *= cur; res.v.resize(len, mint::init(0)); res.trim();}
        res.v.resize(deg + 1, mint::init(0)); res.trim();
        return res;}
    friend Moly integ(Moly a) {
        int n = a.size();
        static vector<mint> inv = {mint::init(0), mint::init(1)};
        for (int i = inv.size(); i <= n; i++) {
            inv.push_back(mint::init(NTT_P - NTT_P / i) * inv[NTT_P % i]);}
        a.v.resize(n + 1);
        for (int i = n; i >= 1; i--) { a[i] = a[i - 1] * inv[i]; }
        a[0] = mint::init(0); a.trim();
        return a;}
    friend Moly invS(const Moly &a, int deg) { // A^{-1}(x) mod x^{deg + 1}
        assert(a[0] != mint::init(0));
        Moly res({inv(a[0])});
        for (int len = 2; len < 2 * deg + 2; len <<= 1) {
            static vector<mint> fa, fb;
            fa.assign(a.v.begin(), a.v.begin() + min(int(a.size()), len)); 
            fa.resize(2 * len, mint::init(0));
            fb.assign(res.v.begin(), res.v.end());
            fb.resize(2 * len, mint::init(0));
            ntt(fa, false); ntt(fb, false);
            for (int i = 0; i < 2 * len; i++) { fb[i] = fb[i] * (mint::init(2) - fa[i] * fb[i]); }
            ntt(fb, true);
            res.v.assign(fb.begin(), fb.begin() + len);}
        res.v.resize(deg + 1, mint::init(0)); res.trim();
        return res;}
    friend Moly log(const Moly &a, int deg) { // ln(A(x)) = \integ A'(x)/A(x) mod x^{deg + 1}
        assert(a.size() > 0 && a[0] == mint::init(1)); 
        Moly da = deriv(a);
        if (da.size() > deg) { da.v.resize(deg); }
        Moly res = integ(std::move(da) * invS(a, deg - 1));
        res.v.resize(deg + 1, mint::init(0)); res.trim();
        return res;}
    friend Moly powS(const Moly &a, lng b, int deg) { // A(x)^b mod x^{deg + 1}
        assert(b >= 0);
        if (b == 0) { return Moly({mint::init(1)}); }
        if (a.isNil()) { return Moly({mint::init(0)}); }
        int n = a.size(), i = 0;
        while (i < n && a[i].n == 0) { i++; }
        if (i == n || (i > 0 && b > deg / i)) { return Moly({mint::init(0)}); }
        mint a_inv = inv(a[i]); Moly res(n - i);
        for (int j = i; j < n; j++) { res[j - i] = a[j] * a_inv; }
        int shf = b * i; mint a_pow = pow(a[i], b);
        res = log(res, deg - shf); res *= mint(b); 
        res = exp(res, deg - shf); res.v.resize(deg + 1);
        for (int j = deg; j >= shf; j--) { res[j] = res[j - shf] * a_pow; }
        for (int j = 0; j < shf; j++) { res[j] = mint::init(0); }
        res.trim();
        return res;}
    // T: O(n * log(n)^2)
    friend mint solveKthTerm(Moly a, Moly b, lng k) { // Bostan-Mori
        assert(!b.isNil() && b[0].n != 0);
        Moly b_neg;
        while (k > 0) {
            b_neg.v.assign(b.v.begin(), b.v.end());
            for (int i = 1; i < b_neg.size(); i += 2) { b_neg[i] = -b_neg[i]; }
            a *= b_neg; b *= b_neg;
            int a_len = 0;
            for (int i = k & 1; i < a.size(); i += 2) { a[a_len++] = a[i]; }
            a.v.resize(a_len); a.trim();
            int b_len = 0;
            for (int i = 0; i < b.size(); i += 2) { b[b_len++] = b[i]; }
            b.v.resize(b_len); b.trim();
            k >>= 1;}
        return a.isNil() ? mint::init(0) : a[0] * inv(b[0]);}
    friend Moly sqrt(const Moly &a, int deg) { // A(x)^{1/2} mod x^{deg + 1}
        if (a.isNil()) { return Moly({mint::init(0)}); }
        int n = a.size(), i = 0;
        while (i < n && a[i].n == 0) { i++; }
        if (i == n || i / 2 > deg) { return Moly({mint::init(0)}); }
        if (i & 1) { return Moly({mint::init(-1)}); }
        mint a_rt2 = sqrt(a[i]);
        if (a_rt2.n == -1) { return Moly({mint::init(-1)}); }
        int shf = i / 2; mint inv2 = inv(mint::init(2)); Moly res({a_rt2}); 
        for (int len = 2; len / 2 <= deg - shf; len <<= 1) {
            Moly cur(min(n - i, len));
            for (int j = 0; j < cur.size(); j++) { cur[j] = a[i + j]; }
            cur = std::move(cur) * invS(res, len - 1); 
            res.v.resize(len, mint::init(0));
            for (int j = 0; j < cur.size(); j++) { res[j] = (res[j] + cur[j]) * inv2; }
            for (int j = cur.size(); j < len; j++) { res[j] *= inv2; }
            res.trim();}
        res.v.resize(deg + 1, mint::init(0));
        for (int j = deg; j >= shf; j--) { res[j] = res[j - shf]; }
        for (int j = 0; j < shf; j++) { res[j] = mint::init(0); }
        res.trim();
        return res;}
    // T: O(n^2)
    static Moly calcMinLinRec(const vector<mint> &seq) { // Berlekamp-Massey
        if (seq.empty()) { return Moly({mint::init(1)}); }
        int len = 0, p = -1; mint d_prv = mint::init(1);
        Moly res({mint::init(1)}), cur({mint::init(1)}), tmp;
        for (int i = 0; i < seq.size(); i++) {
            mint d = mint::init(0);
            for (int j = 0; j < min(len + 1, int(res.size())); j++) { d += res[j] * seq[i - j]; }
            if (d.n == 0) { continue; }
            int shf = i - p; mint dd = d * inv(d_prv);
            if (2 * len <= i) { tmp = res; } 
            if (res.size() < cur.size() + shf) { res.v.resize(cur.size() + shf, mint::init(0)); }
            for (int j = 0; j < cur.size(); j++) { res[j + shf] -= dd * cur[j]; }
            if (2 * len <= i) { len = i - len + 1; p = i; d_prv = d; std::swap(cur, tmp); }}
        res.trim();
        return res;}
    // T: O(n^2)
    static mint guessKthTerm(const vector<mint> &seq, lng k) {
        if (k < seq.size()) { return seq[k]; }
        Moly b = calcMinLinRec(seq);
        if (b.size() <= 1) { return mint::init(0); }
        Moly a(vector<mint>(seq.begin(), seq.begin() + b.size() - 1));
        a = std::move(a) * b; a.v.resize(b.size() - 1); a.trim();
        return solveKthTerm(std::move(a), std::move(b), k);}
    
    // T: O(1)
    bool isNil() const { return v.empty() || (v.size() == 1 && v[0].n == 0); }    
    bool operator==(const Moly &o) const { return v == o.v; }
    bool operator!=(const Moly &o) const { return !(*this == o); }
    
    Moly &operator<<=(int shf) {
        if (size() == 0 || (size() == 1 && v[0] == mint::init(0))) { return *this; }
        v.insert(v.begin(), shf, mint::init(0));
        return *this;}
    Moly &operator>>=(int shf) {
        if (shf >= size()) { v = {mint::init(0)}; return *this; }
        v.erase(v.begin(), v.begin() + shf);
        return *this;}
    
    friend Moly operator<<(Moly a, int shf) { a <<= shf; return a; }
    friend Moly operator>>(Moly a, int shf) { a >>= shf; return a; }
    
    friend ostream &operator<<(ostream &os, const Moly &a) {
        if (a.size() == 0) { return os << "[]"; }
        os << "[";
        for (int i = 0; i < a.size(); i++) { os << a[i].n << (i < a.size() - 1 ? ", " : ""); }
        return os << "]";}
};