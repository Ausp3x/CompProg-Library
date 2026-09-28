// Unchanged legacy extraction: OLD/algorithms.cpp:1260-1316.
// Status: legacy-reference; correctness, performance and API audit pending.
struct MonSet {
    struct S { 
        // states
        lng len;
    };

    struct F { 
        bool to_set; 
        lng set_upd;

        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() {
        return {0};
    }

    static constexpr inline F idF() {
        return {false, 0};
    }

    static constexpr inline S defR(lng l, lng r) {
        return {r - l + 1};
    }

    static constexpr inline S init(lng i, lng x) {
        return {1};
    }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) {
            return b;}
        
        if (b.len == 0) {
            return a;}
        
        S res;
        // ope states
        res.len = a.len + b.len;

        return res;
    }

    static constexpr inline S map(F f, const S &a) {
        if (!f.to_set || a.len == 0) {
            return a;}
        
        S res = a;
        // map states

        return res;
    }

    static constexpr inline F cmp(F f, F g) {
        return f.to_set ? f : g;
    }
};  

