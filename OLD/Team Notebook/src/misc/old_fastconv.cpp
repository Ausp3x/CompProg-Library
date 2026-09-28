// S: NA, U: O(n log n), Q: NA, M: O(1)
struct FastConv {
    template<typename T> static void fctOr(vector<T> &a, bool inv) {
        int n = int(a.size());
        for (int len = 1; len < n; len <<= 1) {
        for (int i = 0; i < n; i += 2 * len) {
        for (int j = 0; j < len; j++) {
            if (!inv) { a[i + j + len] += a[i + j]; } 
            else      { a[i + j + len] -= a[i + j]; }
        }}}
    }
    
    template<typename T> static void fctAnd(vector<T> &a, bool inv) {
        int n = int(a.size());
        for (int len = 1; len < n; len <<= 1) {
        for (int i = 0; i < n; i += 2 * len) {
        for (int j = 0; j < len; j++) {
            if (!inv) { a[i + j] += a[i + j + len]; } 
            else { a[i + j] -= a[i + j + len]; }
        }}}
    }
    
    template<typename T> static void fctXor(vector<T> &a, bool inv) {
        int n = int(a.size());
        for (int len = 1; len < n; len <<= 1) {
        for (int i = 0; i < n; i += 2 * len) {
        for (int j = 0; j < len; j++) {
            T u = a[i + j], v = a[i + j + len]; 
            a[i + j] = u + v; a[i + j + len] = u - v;
        }}}
        if (inv) { 
            T ci = T(1) / T(n); 
            for (auto &x : a) { x *= ci; }
        }
    }
    
    template<typename T> static void fctGcdSlow(vector<T> &a, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) { a[i] += a[j]; }}
        } else {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) { a[i] -= a[j]; }}
        }
    }
    
    template<typename T> static void fctGcd(vector<T> &a, const vector<int> &prms, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = n / p; i >= 1; i--) { a[i] += a[i * p]; }
            } 
        } else { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = 1; i <= n / p; i++) { a[i] -= a[i * p]; }
            }
        }
    }
    
    template<typename T> static void fctLcmSlow(vector<T> &a, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) { a[j] += a[i]; }}
        } else {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) { a[j] -= a[i]; }}
        }
    }
    
    template<typename T> static void fctLcm(vector<T> &a, const vector<int> &prms, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = 1; i <= n / p; i++) { a[i * p] += a[i]; }
            } 
        } else { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = n / p; i >= 1; i--) { a[i * p] -= a[i]; }
            } 
        }
    }
};
