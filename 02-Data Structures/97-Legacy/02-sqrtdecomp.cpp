// Unchanged legacy extraction: OLD/algorithms.cpp:1864-1966.
// Status: legacy-reference; correctness, performance and API audit pending.
// S: O(n), U: O(sqrt(n)), Q: O(sqrt(n)), M: O(n)
template<typename T, typename F = std::plus<T>>
struct SqrtDecomp {
    int n, m;
    vector<T> v, blks, lazy;
    T id;
    F f;

    SqrtDecomp(int N, int M = sqrt(max(N, 1)), T ID = T(0), F f_ = F()) : 
        n(N), m(M), v(N, ID), blks((N + m - 1) / m, ID), lazy((N + m - 1) / m, ID), id(ID), f(f_) {}

    SqrtDecomp(vector<T> &&a, int M = sqrt(max(int(a.size()), 1)), T ID = T(0), F f_ = F()) : 
        n(a.size()), m(M), v(std::move(a)), blks((n + m - 1) / m, ID), lazy((n + m - 1) / m, ID), id(ID), f(f_) {
        for (int i = 0; i < n; i++) {
            blks[i / m] = f(blks[i / m], v[i]);}
    }

    inline void pull(int bi) {
        int l = bi * m;
        int r = min(l + m, n);
        blks[bi] = id;
        for (int i = l; i < r; i++) {
            blks[bi] = f(blks[bi], v[i]);}
    }

    inline void push(int bi) {
        if (lazy[bi] == id) {
            return;}
        
        int l = bi * m;
        int r = min(l + m, n);
        for (int i = l; i < r; i++) {
            v[i] = f(v[i], lazy[bi]);} 
        lazy[bi] = id;
    }

    void opeUpdate(int i, T x) {
        if (i < 0 || i >= n) {
            return;}
        
        int bi = i / m;
        push(bi);
        v[i] = f(v[i], x);
        pull(bi);
    }

    void setUpdate(int i, T x) {
        if (i < 0 || i >= n) {
            return;}
        
        int bi = i / m;
        push(bi);
        v[i] = x;
        pull(bi);
    }

    T query(int l, int r) {
        l = max(l, 0); 
        r = min(r, n - 1);
        if (l > r) {
            return id;}

        int bl = l / m, br = r / m;
        push(bl);
        if (bl != br) {
            push(br);}
        
        T res = id;
        if (bl == br) {
            for (int i = l; i <= r; i++) { 
                res = f(res, v[i]);}
        } else {
            for (int i = l; i < (bl + 1) * m; i++) { 
                res = f(res, v[i]);}
            for (int i = bl + 1; i < br; i++) { 
                res = f(res, f(blks[i], lazy[i]));}
            for (int i = br * m; i <= r; i++) { 
                res = f(res, v[i]);}
        }

        return res;
    }

    friend ostream &operator<<(ostream &os, const SqrtDecomp &a) {
        if (a.n == 0) {
            return os << "[]\n";}

        os << "\n[\n";
        for (int i = 0; i < a.blks.size(); i++) {
            int l = i * a.m;
            int r = min(a.n, l + a.m) - 1;
            if (l > r) {
                break;}
            
            os << "  [" << l << ", " << r << "]: [" << a.blks[i] << " | ";
            for (int j = l; j <= r; j++) {
                os << a.v[j] << (j < r ? ", " : "");}
            os << "]" << (i < a.blks.size() - 1 ? ",\n" : "\n");
        }
        
        return os << "]\n";
    }
};

