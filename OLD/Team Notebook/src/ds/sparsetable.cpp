// S: O(n * log(n)), U: NA, Q: O(1) or O(log(n)), M: O(n * log(n))
template<typename T, typename F>
struct SparseTable {
    int n; 
    vector<vector<T>> v; 
    F f;
    
    SparseTable(const vector<T> &a, F f) : n(int(a.size())), f(f) {
        int k = std::__lg(n) + 1; 
        v.assign(k, vector<T>(n)); v[0] = a;
        for (int i = 1; i < k; i++) {
            for (int j = 0; j + (1 << i) <= n; j++) {
                v[i][j] = f(v[i - 1][j], v[i - 1][j + (1 << (i - 1))]);}}
    }

    T queryFast(int l, int r) { 
        int i = std::__lg(r - l + 1); 
        return f(v[i][l], v[i][r - (1 << i) + 1]); 
    }

    T querySlow(int l, int r) {
        T res = v[0][l];
        for (int i = std::__lg(r - l + 1); i >= 0; i--) {
            if ((1 << i) <= r - l + 1) { 
                res = f(res, v[i][l]); l += (1 << i);}}
        return res;
    }
};