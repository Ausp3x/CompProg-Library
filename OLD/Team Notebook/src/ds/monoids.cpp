struct MonAlg {
    struct S { lng sum, mx, mn, mx_i, mn_i, len; };
    struct F {
        lng a, b;
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() { return {0, -INF64, INF64, -1, -1, 0}; }
    static constexpr inline F idF() { return {1, 0}; }
    static constexpr inline S defR(lng l, lng r) { return {0, 0, 0, l, l, r - l + 1}; }
    static constexpr inline S init(lng i, lng x) { return {x, x, x, i, i, 1}; }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) { return b; }
        if (b.len == 0) { return a; }
        S res;
        res.sum = a.sum + b.sum;
        if (a.mx >= b.mx) { res.mx = a.mx; res.mx_i = a.mx_i; } 
        else { res.mx = b.mx; res.mx_i = b.mx_i; }
        if (a.mn <= b.mn) { res.mn = a.mn; res.mn_i = a.mn_i; } 
        else { res.mn = b.mn; res.mn_i = b.mn_i; }
        res.len = a.len + b.len;
        return res;
    }
    static constexpr inline S map(F f, const S &a) {
        if (a.len == 0) { return a; }
        S res = a;
        res.sum = f.a * a.sum + f.b * a.len;
        if (f.a < 0) {
            res.mx = f.a * a.mn + f.b;
            res.mn = f.a * a.mx + f.b;
            res.mx_i = a.mn_i;
            res.mn_i = a.mx_i;
        } else {
            res.mx = f.a * a.mx + f.b;
            res.mn = f.a * a.mn + f.b;
        }
        return res;
    }
    static constexpr inline F cmp(F f, F g) { return {f.a * g.a, f.a * g.b + f.b}; }

    /*
    void addUpdate(lng l, lng r, lng x) { update(l, r, {1, x}); }
    void mulUpdate(lng l, lng r, lng x) { update(l, r, {x, 0}); }
    void setUpdate(lng l, lng r, lng x) { update(l, r, {0, x}); }
    */
};

struct MonBin {
    struct S { lng And, Or, Xor, len; };
    struct F { 
        lng a, b; 
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() { return {~0LL, 0, 0, 0}; }
    static constexpr inline F idF() { return {~0LL, 0}; }
    static constexpr inline S defR(lng l, lng r) { return {0, 0, 0, r - l + 1}; }
    static constexpr inline S init(lng i, lng x) { return {x, x, x, 1}; }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) { return b; }
        if (b.len == 0) { return a; }
        return {a.And & b.And, a.Or | b.Or, a.Xor ^ b.Xor, a.len + b.len};
    }
    static constexpr inline S map(F f, const S &a) {
        if (a.len == 0) { return a; }
        S res;
        res.And = ((a.And & f.a) & ~f.b) | (~(a.Or & f.a) & f.b);
        res.Or  = ((a.Or & f.a) & ~f.b) | (~(a.And & f.a) & f.b);
        res.Xor = (a.Xor & f.a) ^ (a.len & 1 ? f.b : 0);
        res.len = a.len;
        return res;
    }
    static constexpr inline F cmp(F f, F g) { return {g.a & f.a, (g.b & f.a) ^ f.b}; }

    /*
    void andUpdate(lng l, lng r, lng x) { update(l, r, {x, 0}); }
    void orUpdate(lng l, lng r, lng x)  { update(l, r, {~x, x}); }
    void xorUpdate(lng l, lng r, lng x) { update(l, r, {~0LL, x}); }
    void setUpdate(lng l, lng r, lng x) { update(l, r, {0, x}); }
    */
};

template<bool opt = false>
struct MonGcd {
    struct S { lng Gcd, Lcm, len; };
    struct F { 
        lng a, b; 
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() { 
        if constexpr (opt) { return {0, 1, 0}; }
        return {0, 0, 0}; 
    }    
    static constexpr inline F idF() { return {0, 1}; }
    static constexpr inline S defR(lng l, lng r) {
        if constexpr (opt) { return {0, 1, r - l + 1}; }
        return {0, 0, r - l + 1};
    }
    static constexpr inline S init(lng i, lng x) {
        if constexpr (opt) { return {x, x, 1}; }
        return {x, 0, 1};
    }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) { return b; }
        if (b.len == 0) { return a; }
        S res;
        res.Gcd = gcd(a.Gcd, b.Gcd);
        if constexpr (opt) { res.Lcm = lcm(a.Lcm, b.Lcm); }
        res.len = a.len + b.len;
        return res;
    }
    static constexpr inline S map(F f, const S &a) {
        if (a.len == 0) { return a; }
        S res;
        res.Gcd = lcm(gcd(a.Gcd, f.a), f.b);
        if constexpr (opt) { res.Lcm = lcm(gcd(a.Lcm, f.a), f.b); }
        res.len = a.len;
        return res;
    }
    static constexpr inline F cmp(F f, F g) { return {gcd(g.a, f.a), lcm(gcd(g.b, f.a), f.b)}; }

    /*
    void gcdUpdate(lng l, lng r, lng x) { update(l, r, {x, 1}); }
    void lcmUpdate(lng l, lng r, lng x) { update(l, r, {0, x}); }
    void setUpdate(lng l, lng r, lng x) { update(l, r, {1, x}); }
    */
};

struct MonSar {
    struct S { 
        lng sum, max_prf, max_suf, max_sum, min_prf, min_suf, min_sum, len; 
        void rev() { std::swap(max_prf, max_suf); std::swap(min_prf, min_suf); }
    };
    struct F {
        bool to_set; lng set_upd;
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() { return {0, -INF64, -INF64, -INF64, INF64, INF64, INF64, 0};}
    static constexpr inline F idF() { return {false, 0}; }
    static constexpr inline S defR(lng l, lng r) { return {0, 0, 0, 0, 0, 0, 0, r - l + 1}; }
    static constexpr inline S init(lng i, lng x) { return {x, x, x, x, x, x, x, 1}; }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) { return b; }
        if (b.len == 0) { return a; }
        S res;
        res.sum = a.sum + b.sum;
        res.max_prf = max(a.max_prf, a.sum + b.max_prf);
        res.max_suf = max(b.max_suf, b.sum + a.max_suf);
        res.max_sum = max({a.max_sum, b.max_sum, a.max_suf + b.max_prf});
        res.min_prf = min(a.min_prf, a.sum + b.min_prf);
        res.min_suf = min(b.min_suf, b.sum + a.min_suf);
        res.min_sum = min({a.min_sum, b.min_sum, a.min_suf + b.min_prf});
        res.len = a.len + b.len;
        return res;
    }
    static constexpr inline S map(F f, const S &a) {
        if (!f.to_set || a.len == 0) { return a; }
        S res = a;
        res.sum = f.set_upd * a.len;
        res.max_prf = res.max_suf = res.max_sum = f.set_upd > 0 ? res.sum : f.set_upd;
        res.min_prf = res.min_suf = res.min_sum = f.set_upd < 0 ? res.sum : f.set_upd;
        return res;
    }
    static constexpr inline F cmp(F f, F g) { return f.to_set ? f : g; }
};

struct MonSet {
    struct S { /* states */ lng len; };
    struct F { 
        bool to_set; lng set_upd;
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() { return {0}; }
    static constexpr inline F idF() { return {false, 0}; }
    static constexpr inline S defR(lng l, lng r) { return {r - l + 1}; }
    static constexpr inline S init(lng i, lng x) { return {1}; }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) { return b; }
        if (b.len == 0) { return a; }
        S res;
        // ope states
        res.len = a.len + b.len;
        return res;
    }
    static constexpr inline S map(F f, const S &a) {
        if (!f.to_set || a.len == 0) { return a; }
        S res = a;
        // map states
        return res;
    }
    static constexpr inline F cmp(F f, F g) { return f.to_set ? f : g; }
};