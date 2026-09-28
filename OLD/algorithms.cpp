// 人外有人，天外有天
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi     first
#define se     second
#define pb     push_back
#define all(x) (x).begin(), (x).end()
#define ral(x) (x).rbegin(), (x).rend()
#define sze(x) int((x).size())
using lng = long long;
using lll = __int128;
template<typename T> 
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

int const INF32 = 0x3f3f3f3f;
lng const INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T> 
constexpr inline bool chmax(T &a, const T &b) { 
    return a < b ? a = b, 1 : 0; 
}

template<typename T> 
constexpr inline bool chmin(T &a, const T &b) { 
    return a > b ? a = b, 1 : 0; 
}

// T: O(n^2 / 64), M: O(n^2 / 64)
struct BitMatrix {
    using ulng = uint64_t;

    int n, m, h;
    vector<ulng> v;

    BitMatrix(int N = 0, int M = 0) : n(N * (M > 0)), m(M * (N > 0)), h((m + 63) >> 6), v(n * h, 0) {}
    
    BitMatrix(int N, int M, vector<ulng> a) : n(N * (M > 0)), m(M * (N > 0)), h((m + 63) >> 6), v(std::move(a)) {
        assert(v.size() == n * h);
    }

    BitMatrix(int N, int M, const string &a) : n(N * (M > 0)), m(M * (N > 0)) {
        assert(a.size() == n * m);

        h = (m + 63) >> 6;
        v.assign(n * h, 0);
        for (int i = 0; i < n; i++) {
            ulng *v_i = row(i);
            for (int j = 0; j < m; j++) {
                v_i[j >> 6] |= ulng(a[i * m + j] != '0') << (j & 63);}
        }
    }

    BitMatrix(const string &a, int axis = 0) {
        if (a.empty()) {
            n = 0;
            m = 0;
            h = 0;
            return;
        }

        if (axis == 0) {
            n = 1; 
            m = a.size();
            h = (m + 63) >> 6;
            v.assign(h, 0);
            ulng *v_0 = row(0);
            for (int j = 0; j < m; j++) {
                v_0[j >> 6] |= ulng(a[j] != '0') << (j & 63);}
        } else if (axis == 1) {
            n = a.size(); 
            m = 1;  
            h = 1;
            v.assign(n, 0);
            for (int i = 0; i < n; i++) {
                v[i] = a[i] != '0';}
        }
    }

    BitMatrix(const vector<string> &a) {
        if (a.empty()) {
            n = 0;
            m = 0;
            h = 0;
            return;
        }
        
        n = a.size();
        m = a[0].size();
        h = (m + 63) >> 6;
        v.assign(n * h, 0);
        for (int i = 0; i < n; i++) {
            ulng *v_i = row(i);
            for (int j = 0; j < m; j++) {
                v_i[j >> 6] |= ulng(a[i][j] != '0') << (j & 63);}
        }
    }

    ulng *row(int i) { 
        return v.data() + i * h; 
    }
    
    const ulng *row(int i) const { 
        return v.data() + i * h; 
    }
    
    int size() const { 
        return n; 
    }

    bool get(int i, int j) const {
        return (v[i * h + (j >> 6)] >> (j & 63)) & 1;
    }

    void set(int i, int j, bool opt = 1) {
        v[i * h + (j >> 6)] ^= (v[i * h + (j >> 6)] ^ (-ulng(opt))) & (1ULL << (j & 63));
    }

    void tog(int i, int j) {
        v[i * h + (j >> 6)] ^= 1ULL << (j & 63);
    }

    BitMatrix &operator+=(const BitMatrix &o) {
        assert(n == o.n && m == o.m);

        for (int i = 0; i < n * h; i++) {
            v[i] ^= o.v[i];}

        return *this;
    }

    BitMatrix &operator-=(const BitMatrix &o) {
        return *this += o;
    }

    BitMatrix &operator*=(int o) {
        if (o == 0) {
            std::fill(v.begin(), v.end(), 0);}
        
        return *this;
    }

    // T: O(n^3 / 64)
    BitMatrix &operator*=(const BitMatrix &o) {
        return *this = *this * o;
    }

    BitMatrix operator+() const {
        return *this;
    }

    BitMatrix operator-() const {
        return *this;
    }

    friend BitMatrix operator+(BitMatrix a, const BitMatrix &b) {
        a += b;

        return a;
    }

    friend BitMatrix operator+(const BitMatrix &a, BitMatrix &&b) { 
        b += a; 
        
        return std::move(b); 
    }

    friend BitMatrix operator-(BitMatrix a, const BitMatrix &b) {
        a -= b;

        return a;
    }

    friend BitMatrix operator-(const BitMatrix &a, BitMatrix &&b) { 
        b -= a; 
        
        return std::move(b); 
    }

    friend BitMatrix operator*(BitMatrix a, int b) {
        a *= b;

        return a;
    }

    friend BitMatrix operator*(int b, BitMatrix a) {
        a *= b;

        return a;
    }

    // T: O(n^3 / 64)
    friend BitMatrix operator*(const BitMatrix &a, const BitMatrix &b) {
        assert(a.m == b.n);

        BitMatrix res(a.n, b.m);
        for (int i = 0; i < a.n; i++) {
            const ulng *a_i = a.row(i);
            ulng *res_i = res.row(i);
            for (int kh = 0; kh < a.h; kh++) {
                ulng msk = a_i[kh];
                while (msk) {
                    int k = (kh << 6) | __builtin_ctzll(msk);
                    if (k >= a.m) {
                        break;}
                    
                    msk &= msk - 1;
                    const ulng *b_k = b.row(k);
                    for (int j = 0; j < res.h; j++) {
                        res_i[j] ^= b_k[j];}
                }
            }
        }

        return res;
    }

    // T: O(n^3 / 64)
    friend BitMatrix concat(const BitMatrix &a, const BitMatrix &b, int axis = 0) {
        BitMatrix res;
        if (axis == 0) {
            assert(a.n == b.n);

            int q = a.m >> 6, r = a.m & 63;
            res = BitMatrix(a.n, a.m + b.m);
            for (int i = 0; i < a.n; i++) {
                const ulng *a_i = a.row(i);
                const ulng *b_i = b.row(i);
                ulng *res_i = res.row(i);
                std::copy_n(a_i, a.h, res_i);
                if (r == 0) {
                    for (int j = 0; j < b.h; j++) {
                        res_i[q + j] |= b_i[j];}
                } else {
                    for (int j = 0; j < b.h; j++) {
                        res_i[q + j] |= (b_i[j] << r);
                        if (q + j + 1 < res.h) {
                            res_i[q + j + 1] |= (b_i[j] >> (64 - r));}
                    }
                }
            }
        } else if (axis == 1) {
            assert(a.m == b.m);

            res = BitMatrix(a.n + b.n, a.m);
            std::copy(a.v.begin(), a.v.end(), res.v.begin());
            std::copy(b.v.begin(), b.v.end(), res.v.begin() + a.v.size());
        }

        return res;
    }

    // T: O(n^3 / 64)
    friend int det(BitMatrix a) {
        assert(a.n == a.m);

        for (int i = 0; i < a.n; i++) {
            int piv = i;
            while (piv < a.n && !a.get(piv, i)) {
                piv++;}
            
            if (piv == a.n) {
                return 0;}
            
            if (i != piv) {
                std::swap_ranges(a.row(i), a.row(i) + a.h, a.row(piv));}
            
            ulng *a_i = a.row(i);
            for (int k = i + 1; k < a.n; k++) {
                if (a.get(k, i)) {
                    ulng *a_k = a.row(k);
                    for (int j = i >> 6; j < a.h; j++) {
                        a_k[j] ^= a_i[j];}
                }
            }
        }

        return 1;
    }

    // T: O(n^3 / 64)
    friend BitMatrix inv(BitMatrix a) {
        assert(a.n == a.m);
    
        return solveLin(std::move(a), eye(a.n, a.n)); 
    }

    // T: O(n^3 / 64 * log(n))
    friend BitMatrix pow(BitMatrix a, lng b) {
        assert(a.n == a.m && b >= 0);
        
        BitMatrix res = eye(a.n, a.n);
        BitMatrix tmp(a.n, a.n);

        auto mul = [&](BitMatrix &A, const BitMatrix &B) -> void {
            std::fill(tmp.v.begin(), tmp.v.end(), 0);
            for (int i = 0; i < A.n; i++) {
                const ulng *A_i = A.row(i);
                ulng *tmp_i = tmp.row(i);
                for (int kh = 0; kh < A.h; kh++) {
                    ulng msk = A_i[kh];
                    while (msk) {
                        int k = (kh << 6) | __builtin_ctzll(msk);
                        if (k >= A.m) {
                            break;}
                        
                        msk &= msk - 1;
                        const ulng *B_k = B.row(k);
                        for (int j = 0; j < res.h; j++) {
                            tmp_i[j] ^= B_k[j];}
                    }
                }
            }
            A.v.swap(tmp.v);
        };
        
        while (b > 0) {
            if (b & 1) {
                res *= a;}

            b >>= 1;
            if (b > 0) {
                a *= a;}
        }

        return res;
    }

    // T: O(n^3 / 64)
    friend int rnk(BitMatrix a) {
        a = ref(std::move(a));

        int r = 0;
        for (int i = 0; i < a.n; i++) {
            bool chk = false;
            ulng *a_i = a.row(i);
            for (int j = 0; j < a.h; j++) {
                if (a_i[j] > 0) {
                    chk = true;
                    break;
                }
            }
            if (chk) {
                r++;
            } else {
                break;
            }
        }

        return r;
    }

    // T: O(n^3 / 64)
    friend BitMatrix ref(BitMatrix a) {
        int r = 0;
        for (int c = 0; r < a.n && c < a.m; c++) {
            int piv = r;
            while (piv < a.n && !a.get(piv, c)) {
                piv++;}
            
            if (piv == a.n) {
                continue;}
            
            if (r != piv) {
                std::swap_ranges(a.row(r), a.row(r) + a.h, a.row(piv));}
            
            ulng *a_r = a.row(r);
            for (int i = r + 1; i < a.n; i++) {
                if (a.get(i, c)) {
                    ulng *a_i = a.row(i);
                    for (int j = c >> 6; j < a.h; j++) {
                        a_i[j] ^= a_r[j];}
                }
            }
            r++;
        }

        return a;
    }

    // T: O(n^3 / 64)
    friend BitMatrix rref(BitMatrix a) {
        int r = 0;
        for (int c = 0; r < a.n && c < a.m; c++) {
            int piv = r;
            while (piv < a.n && !a.get(piv, c)) {
                piv++;}
            
            if (piv == a.n) {
                continue;}
            
            if (r != piv) {
                std::swap_ranges(a.row(r), a.row(r) + a.h, a.row(piv));}
            
            ulng *a_r = a.row(r);
            for (int i = 0; i < a.n; i++) {
                if (i == r || !a.get(i, c)) {
                    continue;}
                
                ulng *a_i = a.row(i);
                for (int j = c >> 6; j < a.h; j++) {
                    a_i[j] ^= a_r[j];}
            }
            r++;
        }

        return a;
    }

    // T: O(n^3 / 64)
    friend BitMatrix solveLin(BitMatrix a, BitMatrix b) {
        assert(a.n == b.n);

        int r = 0;
        vector<int> pivs(a.m, -1);
        for (int c = 0; r < a.n && c < a.m; c++) {
            int piv = r;
            while (piv < a.n && !a.get(piv, c)) {
                piv++;}
            
            if (piv == a.n) {
                continue;}
            
            if (r != piv) {
                std::swap_ranges(a.row(r), a.row(r) + a.h, a.row(piv));
                std::swap_ranges(b.row(r), b.row(r) + b.h, b.row(piv)); 
            }
            
            ulng *a_r = a.row(r);
            ulng *b_r = b.row(r);
            for (int i = 0; i < a.n; i++) {
                if (i == r || !a.get(i, c)) {
                    continue;}

                ulng *a_i = a.row(i);
                ulng *b_i = b.row(i);
                for (int j = c >> 6; j < a.h; j++) {
                    a_i[j] ^= a_r[j];}
                for (int j = 0; j < b.h; j++) {
                    b_i[j] ^= b_r[j];}
            }
            pivs[c] = r++;
        }
        for (int i = r; i < a.n; i++) {
            ulng *b_i = b.row(i);
            for (int j = 0; j < b.h; j++) {
                if (b_i[j] != 0) {
                    return BitMatrix();}}
        }
        
        BitMatrix res(a.m, b.m);
        for (int c = 0; c < a.m; c++) {
            if (pivs[c] != -1) {
                const ulng *b_piv = b.row(pivs[c]);
                ulng *res_c = res.row(c);
                for (int j = 0; j < b.h; j++) {
                    res_c[j] = b_piv[j];}
            }
        }
        
        return res;
    }

    friend int trc(const BitMatrix &a) {
        assert(a.n == a.m);

        int res = 0;
        for (int i = 0; i < a.n; i++) {
            res ^= a.get(i, i);}

        return res;
    }

    friend BitMatrix trp(const BitMatrix &a) { 
        BitMatrix res(a.m, a.n);
        for (int i = 0; i < a.n; i++) {
            const ulng *a_i = a.row(i);
            for (int kh = 0; kh < a.h; kh++) {
                ulng msk = a_i[kh];
                while (msk) {
                    int k = (kh << 6) | __builtin_ctzll(msk);
                    if (k >= a.m) {
                        break;}
                    
                    msk &= msk - 1;
                    res.set(k, i);
                }
            }
        }
        
        return res;
    }

    static BitMatrix cross(const BitMatrix &a, const BitMatrix &b, int axis = 0) {
        assert((a.n == 1 || a.m == 1) && max(a.n, a.m) == 3);
        assert((b.n == 1 || b.m == 1) && max(b.n, b.m) == 3);

        auto get = [](const BitMatrix &A, int i) -> int {
            return A.n == 1 ? A.get(0, i) : A.get(i, 0);
        };

        int x = (get(a, 1) & get(b, 2)) ^ (get(a, 2) & get(b, 1));
        int y = (get(a, 2) & get(b, 0)) ^ (get(a, 0) & get(b, 2));
        int z = (get(a, 0) & get(b, 1)) ^ (get(a, 1) & get(b, 0));

        BitMatrix res;
        if (axis == 0) {
            res = BitMatrix(1, 3);
            res.set(0, 0, x & 1);
            res.set(0, 1, y & 1);
            res.set(0, 2, z & 1);
        } else {
            res = BitMatrix(3, 1);
            res.set(0, 0, x & 1);
            res.set(1, 0, y & 1);
            res.set(2, 0, z & 1);
        }
        
        return res;
    }

    static int dot(const BitMatrix &a, const BitMatrix &b) {
        assert((a.n == 1 || a.m == 1) && (b.n == 1 || b.m == 1));
        assert(max(a.n, a.m) == max(b.n, b.m));
        
        if (a.n == b.n && a.m == b.m) {
            int res = 0;
            for (int i = 0; i < a.n; i++) {
                const ulng *a_i = a.row(i);
                const ulng *b_i = b.row(i);
                for (int j = 0; j < a.h; j++) {
                    res ^= __builtin_popcountll(a_i[j] & b_i[j]) & 1;}
            }
            return res;
        }

        auto get = [](const BitMatrix &A, int i) -> int {
            return A.n == 1 ? A.get(0, i) : A.get(i, 0);
        };

        int res = 0;
        for (int i = 0; i < max(a.n, a.m); i++) {
            res ^= (get(a, i) & get(b, i));}

        return res;
    }

    static BitMatrix eye(int N, int M) {
        BitMatrix res(N, M);
        for (int i = 0; i < min(N, M); i++) {
            res.set(i, i);}      
        
        return res;
    }

    friend bool operator==(const BitMatrix &a, const BitMatrix &b) = default;

    friend ostream &operator<<(ostream &os, const BitMatrix &a) {
        if (a.n == 0 || a.m == 0) {
            return os << "[]";}
        
        os << "[";
        for (int i = 0; i < a.n; i++) {
            os << "[";
            for (int j = 0; j < a.m; j++) {
                os << a.get(i, j) << (j < a.m - 1 ? ", " : "");}
            os << "]" << (i < a.n - 1 ? ", " : "");
        }
        
        return os << "]";
    }
};

// T: O(n^2), M: O(n)
template<typename T>
struct SparseMatrix {
    int n, m;
    vector<int> rwp;
    vector<int> col;
    vector<T> val;

    SparseMatrix(int N = 0, int M = 0) : n(N), m(M) {
        assert((n == 0 && m == 0) || (n > 0 && m > 0));

        rwp.assign(n + 1, 0);
    }

    SparseMatrix(int N, int M, const vector<tuple<int, int, T>> &a) : n(N), m(M) {
        assert((n == 0 && m == 0) || (n > 0 && m > 0));

        rwp.assign(n + 1, 0);
        col.reserve(a.size());
        val.reserve(a.size());

        for (const auto &[r, c, v] : a) {
            assert(0 <= r && r < n && 0 <= c && c < m);
            rwp[r + 1]++;
        }
        
        for (int i = 0; i < n; i++) {
            rwp[i + 1] += rwp[i];}
        
        col.resize(a.size());
        val.resize(a.size());
        vector<int> cur_rwp = rwp;
        for (const auto &[r, c, v] : a) {
            int pos = cur_rwp[r]++;
            col[pos] = c;
            val[pos] = v;
        }
    }

    int size() const { 
        return n; 
    }

    // T: O(K) 
    vector<T> operator*(const vector<T> &b) const {
        assert(m == b.size());

        vector<T> res(n, T(0));
        for (int i = 0; i < n; i++) {
            T sum = 0;
            for (int j = rwp[i]; j < rwp[i + 1]; j++) {
                sum += val[j] * b[col[j]];}
            
            res[i] = sum;
        }

        return res;
    }

    static bool isNil(const T &x) {
        if constexpr (std::is_floating_point_v<T>) {
            return -1e-9 < x && x < 1e-9;
        } else {
            return x == T(0);
        }
    }

    // T: O(N^2)
    static vector<T> berlekampMassey(const vector<T> &s) {
        vector<T> c = {T(1)}, old_c = {T(1)};
        int f = -1;
        T df = 1;

        for (int i = 0; i < s.size(); i++) {
            T d = 0;
            for (int j = 0; j < c.size(); j++) {
                d += c[j] * s[i - j];}

            if (isNil(d)) {
                continue;}

            vector<T> next_c = c;
            T coef = d / df;
            
            if (next_c.size() < old_c.size() + i - f) {
                next_c.resize(old_c.size() + i - f, T(0));}
            
            for (int j = 0; j < old_c.size(); j++) {
                next_c[j + i - f] -= coef * old_c[j];}

            if (i - f + old_c.size() > c.size()) {
                old_c = c;
                f = i;
                df = d;
            }
            c = std::move(next_c);
        }
        
        return c;
    }

    // T: O(L^2)
    static vector<T> polyMulMod(const vector<T> &a, const vector<T> &b, const vector<T> &c) {
        int L = c.size() - 1;
        vector<T> res(a.size() + b.size() - 1, T(0));
        for (int i = 0; i < a.size(); i++) {
            for (int j = 0; j < b.size(); j++) {
                res[i + j] += a[i] * b[j];}}
        
        for (int i = res.size() - 1; i >= L; i--) {
            if (isNil(res[i])) {
                continue;}
            
            for (int j = 1; j <= L; j++) {
                res[i - j] -= res[i] * c[j];}
        }
        res.resize(min((int)res.size(), L));
        
        return res;
    }

    // T: O(N * K + N^2)
    friend T det(const SparseMatrix &a) {
        assert(a.n == a.m);
        static_assert(!std::is_integral_v<T>);

        std::mt19937 rng(1337);
        uint32_t max_val = 10000;
        if constexpr (requires { int(T()); } && !std::is_floating_point_v<T>) {
            max_val = uint32_t(int(T(-1)));
        }

        vector<T> D(a.n);
        T det_D = 1;
        for (int i = 0; i < a.n; i++) {
            D[i] = T((rng() % max_val) + 1);
            det_D *= D[i];
        }

        vector<T> u(a.n), v(a.n);
        for (int i = 0; i < a.n; i++) {
            u[i] = T(rng() % (max_val + 1));
            v[i] = T(rng() % (max_val + 1));
        }

        vector<T> seq(2 * a.n);
        for (int i = 0; i < 2 * a.n; i++) {
            T dot = 0;
            for (int j = 0; j < a.n; j++) {
                dot += u[j] * v[j];}
            
            seq[i] = dot;

            vector<T> dv(a.n);
            for (int j = 0; j < a.n; j++) {
                dv[j] = D[j] * v[j];}
            
            v = a * dv;
        }

        vector<T> min_poly = berlekampMassey(seq);

        if (min_poly.size() - 1 < a.n) {
            return T(0);}

        T det_AD = min_poly.back();
        if (a.n % 2 == 1) {
            det_AD = -det_AD;}

        return det_AD / det_D;
    }

    // T: O(N * K + N^2)
    friend vector<T> solveLin(const SparseMatrix &a, vector<T> b) {
        assert(a.n == a.m && a.n == b.size());
        static_assert(!std::is_integral_v<T>);

        std::mt19937 rng(1337);
        uint32_t max_val = 10000;
        if constexpr (requires { int(T()); } && !std::is_floating_point_v<T>) {
            max_val = uint32_t(int(T(-1)));
        }

        vector<T> u(a.n);
        for (int i = 0; i < a.n; i++) {
            u[i] = T(rng() % (max_val + 1));}
        
        vector<T> seq(2 * a.n);
        vector<T> cur = b;
        for (int i = 0; i < 2 * a.n; i++) {
            T dot = 0;
            for (int j = 0; j < a.n; j++) {
                dot += u[j] * cur[j];}
            
            seq[i] = dot;
            cur = a * cur;
        }

        vector<T> min_poly = berlekampMassey(seq);
        int L = min_poly.size() - 1;

        assert(!isNil(min_poly.back()) && "Linear system solution does not exist or matrix is singular with respect to b.");

        T inv_cL = T(1) / min_poly.back();
        vector<T> res(a.n, T(0));
        cur = b;
        for (int i = 0; i < L; i++) {
            T coef = -min_poly[L - 1 - i] * inv_cL;
            for (int j = 0; j < a.n; j++) {
                res[j] += cur[j] * coef;}
            
            if (i + 1 < L) {
                cur = a * cur;}
        }

        return res;
    }

    // T: O(N * K + N * L + L^2 * log(k)) where L <= N
    friend vector<T> pow(const SparseMatrix &a, lng k, vector<T> v) {
        assert(a.n == a.m && a.n == v.size() && k >= 0);

        std::mt19937 rng(1337);
        uint32_t max_val = 10000;
        if constexpr (requires { int(T()); } && !std::is_floating_point_v<T>) {
            max_val = uint32_t(int(T(-1)));
        }

        vector<T> u(a.n);
        for (int i = 0; i < a.n; i++) {
            u[i] = T(rng() % (max_val + 1));}
        
        vector<T> seq(2 * a.n);
        vector<T> cur = v;
        for (int i = 0; i < 2 * a.n; i++) {
            T dot = 0;
            for (int j = 0; j < a.n; j++) {
                dot += u[j] * cur[j];}
            
            seq[i] = dot;
            cur = a * cur;
        }

        vector<T> min_poly = berlekampMassey(seq);

        vector<T> res_poly = {T(1)};
        vector<T> base_poly = {T(0), T(1)};
        while (k > 0) {
            if (k & 1) {
                res_poly = polyMulMod(res_poly, base_poly, min_poly);}
            
            k >>= 1;
            if (k > 0) {
                base_poly = polyMulMod(base_poly, base_poly, min_poly);}
        }

        vector<T> res(a.n, T(0));
        cur = v;
        for (int i = 0; i < res_poly.size(); i++) {
            for (int j = 0; j < a.n; j++) {
                res[j] += cur[j] * res_poly[i];}
            
            if (i + 1 < res_poly.size()) {
                cur = a * cur;}
        }

        return res;
    }

    // T: O(k * K)
    friend vector<T> steadyState(const SparseMatrix &a, vector<T> v, int max_iter = 1000) {
        assert(a.n == a.m && a.n == v.size());

        for (int i = 0; i < max_iter; i++) {
            v = a * v;}
        
        return v;
    }
};

// S: O(1), U: NA, Q: O(1), M: O(1)
struct Random {
    std::mt19937_64 rng; 

    Random() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    int randInt(int l, int r) {
        return std::uniform_int_distribution<int>(l, r)(rng);
    }

    lng randLng(lng l, lng r) {
        return std::uniform_int_distribution<lng>(l, r)(rng);
    }
};
inline Random rng;

// S: NA, U: NA, Q: O(1), M: O(1)
struct CustomHash {
    using ulng = unsigned long long;
    static inline const ulng rnd = 
        std::chrono::steady_clock::now().time_since_epoch().count() + 
        reinterpret_cast<uintptr_t>(&rnd);

    static ulng splitMix64(ulng x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;

        return x ^ (x >> 31);
    }

    template<typename T>
    requires std::is_convertible_v<T, ulng>
    size_t operator()(const T &x) const {
        return splitMix64(ulng(x) + rnd);
    }

    size_t operator()(std::string_view x) const {
        ulng hash = 5381 + rnd;
        for (char y : x) {
            hash = ((hash << 5) + hash) ^ y;}
        
        return splitMix64(hash);
    }
    
    template<typename T1, typename T2>
    size_t operator()(const pair<T1, T2> &x) const {
        return (splitMix64(ulng(x.first) + rnd) << 3) ^ (splitMix64(ulng(x.second) + rnd) >> 1);
    }

    // Q: O(n)
    template<typename ...Ts>
    size_t operator()(const tuple<Ts...> &x) const {
        size_t seed = 0;
        std::apply(
            [&](const auto &...args) {
                ((seed = ((seed << 3) ^ (splitMix64(ulng(args) + rnd) >> 1)) + 0x9e3779b9), ...);
            }, x
        );
        
        return seed;
    }

    // Q: O(n)
    template<std::ranges::range T>
    requires (!std::is_convertible_v<T, std::string_view>)
    size_t operator()(const T &x) const {
        size_t seed = 0;
        for (const auto &y : x) {
            seed ^= (*this)(y) + (seed << 6) + (seed >> 2) + 0x9e3779b9;}
        
        return seed;
    }
};
template<typename K, typename V> 
using safe_unordered_map = unordered_map<K, V, CustomHash>;
template<typename T> 
using safe_unordered_set = unordered_set<T, CustomHash>;



struct MonAlg {
    struct S { 
        lng sum;
        lng mx, mn;
        lng mx_i, mn_i;
        lng len;
    };

    struct F {
        lng a, b;
        
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() {
        return {0, -INF64, INF64, -1, -1, 0};
    }

    static constexpr inline F idF() {
        return {1, 0};
    }
    
    static constexpr inline S defR(lng l, lng r) {
        return {0, 0, 0, l, l, r - l + 1};
    }

    static constexpr inline S init(lng i, lng x) {
        return {x, x, x, i, i, 1};
    }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) {
            return b;}

        if (b.len == 0) {
            return a;}
        
        S res;
        res.sum = a.sum + b.sum;
        if (a.mx >= b.mx) {
            res.mx = a.mx;
            res.mx_i = a.mx_i;
        } else {
            res.mx = b.mx;
            res.mx_i = b.mx_i;
        }
        if (a.mn <= b.mn) {
            res.mn = a.mn;
            res.mn_i = a.mn_i;
        } else {
            res.mn = b.mn;
            res.mn_i = b.mn_i;
        }
        res.len = a.len + b.len;

        return res;
    }

    static constexpr inline S map(F f, const S &a) {
        if (a.len == 0) {
            return a;}
        
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

    static constexpr inline F cmp(F f, F g) {
        return {f.a * g.a, f.a * g.b + f.b};
    }

    /*
    void addUpdate(lng l, lng r, lng x) { 
        update(l, r, {1, x});
    }

    void mulUpdate(lng l, lng r, lng x) { 
        update(l, r, {x, 0});
    }

    void setUpdate(lng l, lng r, lng x) { 
        update(l, r, {0, x});
    }
    */
};

struct MonBin {
    struct S { 
        lng And, Or, Xor;
        lng len;
    };

    struct F { 
        lng a, b; 
        
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() { 
        return {~0LL, 0, 0, 0}; 
    }
    
    static constexpr inline F idF() { 
        return {~0LL, 0}; 
    }

    static constexpr inline S defR(lng l, lng r) {
        return {0, 0, 0, r - l + 1};
    }

    static constexpr inline S init(lng i, lng x) {
        return {x, x, x, 1};
    }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) {
            return b;}

        if (b.len == 0) {
            return a;}

        return {a.And & b.And, a.Or | b.Or, a.Xor ^ b.Xor, a.len + b.len};
    }

    static constexpr inline S map(F f, const S &a) {
        if (a.len == 0) {
            return a;}

        S res;
        res.And = ((a.And & f.a) & ~f.b) | (~(a.Or & f.a) & f.b);
        res.Or  = ((a.Or & f.a) & ~f.b) | (~(a.And & f.a) & f.b);
        res.Xor = (a.Xor & f.a) ^ (a.len & 1 ? f.b : 0);
        res.len = a.len;
        
        return res;
    }

    static constexpr inline F cmp(F f, F g) {
        return {g.a & f.a, (g.b & f.a) ^ f.b};
    }

    /*
    void andUpdate(lng l, lng r, lng x) { 
        update(l, r, {x, 0});
    }

    void orUpdate(lng l, lng r, lng x)  { 
        update(l, r, {~x, x}); 
    }
    
    void xorUpdate(lng l, lng r, lng x) { 
        update(l, r, {~0LL, x}); 
    }
    
    void setUpdate(lng l, lng r, lng x) { 
        update(l, r, {0, x}); 
    }
    */
};

template<bool opt = false>
struct MonGcd {
    struct S { 
        lng Gcd, Lcm;
        lng len;
    };

    struct F { 
        lng a, b; 

        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() { 
        if constexpr (opt) { 
            return {0, 1, 0};}
        
        return {0, 0, 0}; 
    }
    
    static constexpr inline F idF() { 
        return {0, 1}; 
    }

    static constexpr inline S defR(lng l, lng r) {
        if constexpr (opt) {
            return {0, 1, r - l + 1};}

        return {0, 0, r - l + 1};
    }

    static constexpr inline S init(lng i, lng x) {
        if constexpr (opt) {
            return {x, x, 1};}

        return {x, 0, 1};
    }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) {
            return b;}

        if (b.len == 0) {
            return a;}

        S res;
        res.Gcd = gcd(a.Gcd, b.Gcd);
        if constexpr (opt) {
            res.Lcm = lcm(a.Lcm, b.Lcm);}
        res.len = a.len + b.len;
        
        return res;
    }

    static constexpr inline S map(F f, const S &a) {
        if (a.len == 0) {
            return a;}

        S res;
        res.Gcd = lcm(gcd(a.Gcd, f.a), f.b);
        if constexpr (opt) {
            res.Lcm = lcm(gcd(a.Lcm, f.a), f.b);}
        res.len = a.len;
        
        return res;
    }

    static constexpr inline F cmp(F f, F g) {
        return {gcd(g.a, f.a), lcm(gcd(g.b, f.a), f.b)};
    }

    /*
    void gcdUpdate(lng l, lng r, lng x) { 
        update(l, r, {x, 1});
    }
    
    void lcmUpdate(lng l, lng r, lng x) { 
        update(l, r, {0, x}); 
    }
    
    void setUpdate(lng l, lng r, lng x) { 
        update(l, r, {1, x}); 
    }
    */
};

struct MonSar {
    struct S { 
        lng sum;
        lng max_prf, max_suf, max_sum;
        lng min_prf, min_suf, min_sum;
        lng len;
    };

    struct F {
        bool to_set;
        lng set_upd;
        
        bool operator==(const F&) const = default;
    };

    static constexpr inline S idS() {
        return {0, -INF64, -INF64, -INF64, INF64, INF64, INF64, 0};
    }

    static constexpr inline F idF() {
        return {false, 0};
    }
    
    static constexpr inline S defR(lng l, lng r) {
        return {0, 0, 0, 0, 0, 0, 0, r - l + 1};
    }

    static constexpr inline S init(lng i, lng x) {
        return {x, x, x, x, x, x, x, 1};
    }

    static constexpr inline S ope(const S &a, const S &b) {
        if (a.len == 0) {
            return b;}

        if (b.len == 0) {
            return a;}
        
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
        if (!f.to_set || a.len == 0) return a;
        
        S res = a;
        res.sum = f.set_upd * a.len;
        res.max_prf = res.max_suf = res.max_sum = f.set_upd > 0 ? res.sum : f.set_upd;
        res.min_prf = res.min_suf = res.min_sum = f.set_upd < 0 ? res.sum : f.set_upd;
        
        return res;
    }

    static constexpr inline F cmp(F f, F g) {
        return f.to_set ? f : g;
    }
};

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



// 1.5.6 Continued Fractions

// 2 Data Structures

// 2.1 Fundamentals

// 2.1.2 Sparse Table

// S: O(n * log(n)), U: NA, Q: O(1), M: O(n * log(n))
template<typename T, typename F>
struct SparseTable {
    int n, h;
    vector<T> v;
    F f;

    SparseTable(vector<T> a, const F &fnc) : n(a.size()), v(std::move(a)), f(fnc) {
        if (n == 0) {
            h = 0;
            return;
        }
        
        h = __lg(n) + 1;
        v.resize(h * n);

        for (int i = 1; i < h; i++) {
            int i_cur = i * n;
            int i_prv = i_cur - n;
            for (int j = 0; j + (1 << i) <= n; j++) {
                v[i_cur + j] = f(v[i_prv + j], v[i_prv + j + (1 << (i - 1))]);}
        }
    }

    T queryFast(int l, int r) const {
        assert(0 <= l && l <= r && r < n);

        int i = __lg(r - l + 1);

        return f(v[i * n + l], v[i * n + r - (1 << i) + 1]);
    }

    // Q: O(log(n))
    T querySlow(int l, int r) const {
        assert(0 <= l && l <= r && r < n);
        
        int len = r - l + 1;
        T res;
        for (int i = __lg(len), non = true; i >= 0; i--) {
            if ((len >> i) & 1) {
                if (non) {
                    res = v[i * n + l];
                    non = false;
                } else {
                    res = f(res, v[i * n + l]);
                }
                
                l += 1 << i;
            }
        }

        return res;
    }

    friend ostream &operator<<(ostream &os, const SparseTable &a) {
        if (a.n == 0) {
            return os << "[]";}
        
        os << "\n[\n";
        for (int i = a.h - 1; i >= 0; i--) {
            os << "  [";
            for (int j = 0; j + (1 << i) <= a.n; j++) {
                os << "[" << j << ", " << j + (1 << i) - 1 << "]: " << a.v[i * a.n + j] << (j + (1 << i) < a.n ? ", " : "");}
            os << "]" << (i > 0 ? ",\n" : "\n");
        }
        
        return os << "]\n";
    }
};

// 2.2 Trees

// 2.2.1 Disjoint Set Union

// S: O(n), U: O(a(n)), Q: O(a(n)), M: O(n)
struct DSU {
    int n, ncon;
    vector<int> par, siz, esz;
    vector<lng> dis;
    vector<bool> is_bip;

    DSU(int N) : n(N), ncon(N) {
        par.resize(n);
        iota(par.begin(), par.end(), 0);
        siz.assign(n, 1);
        esz.assign(n, 0);
        dis.assign(n, 0);
        is_bip.assign(n, true);
    }

    bool uniteSets(int u, int v, lng w = 1) {
        int ru = findSet(u);
        int rv = findSet(v);

        if (ru == rv) {
            esz[ru]++;
            if ((dis[u] - dis[v] - w) % 2 != 0) {
                is_bip[ru] = false;}
            return false;
        }

        if (siz[ru] < siz[rv]) {
            std::swap(ru, rv);
            std::swap(u, v);
            w = -w;
        }

        ncon--;
        par[rv] = ru;
        siz[ru] += siz[rv];
        esz[ru] += esz[rv] + 1;
        dis[rv] = dis[u] - dis[v] - w;
        is_bip[ru] = is_bip[ru] && is_bip[rv];

        return true;
    }

    int findSet(int u) {
        if (u == par[u]) {
            return u;}

        int p = par[u];
        par[u] = findSet(p);
        dis[u] += dis[p];

        return par[u];
    }

    int getDis(int u) {
        findSet(u);

        return dis[u];
    }
    
    int getEsiz(int u) {
        return esz[findSet(u)];
    }

    int getSize(int u) {
        return siz[findSet(u)];
    }

    bool isBipartite(int u) {
        return is_bip[findSet(u)];
    }

    bool isSameSet(int u, int v) {
        return findSet(u) == findSet(v);
    }
};

// 2.2.2 Fenwick Tree

// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct FenTreeRangeAdd1D {
    int n;
    vector<T> v1, v2;

    FenTreeRangeAdd1D(int N) : n(N), v1(N, T(0)), v2(N, T(0)) {}

    FenTreeRangeAdd1D(vector<T> a) : n(a.size()), v1(std::move(a)), v2(n, T(0)) {
        for (int i = 0; i < n; i++) {
            int j = i | (i + 1);
            if (j < n) {
                v1[j] += v1[i];}
        }
    }

    void _add(int i_cur, T x1, T x2) {
        for (int i = i_cur; i < n; i |= i + 1) {
            v1[i] += x1;
            v2[i] += x2;
        }
    }

    void addUpdate(int l, int r, T x) {
        if (l > r) {
            return;}
        
        _add(l,     -x * T(l - 1), x);
        _add(r + 1, x * T(r),      -x);
    }

    T _sum(int i_cur) const {
        if (i_cur < 0) {
            return T(0);}

        T r1 = T(0), r2 = T(0);
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            r1 += v1[i];
            r2 += v2[i];
        }

        return r2 * T(i_cur) + r1;
    }

    T sumQuery(int l, int r) const {
        if (l > r) {
            return T(0);}
        
        return _sum(r) - _sum(l - 1);
    }

    friend ostream &operator<<(ostream &os, const FenTreeRangeAdd1D &a) {
        if (a.n == 0) {
            return os << "[]";}
            
        int mx_h = 0;
        for (int i = 0; i < a.n; i++) {
            mx_h = max(mx_h, __builtin_ctz(~i));}
        
        os << "\n[\n";
        for (int h = mx_h; h >= 0; h--) {
            os << "  [";
            bool is_fi = true;
            for (int i = 0; i < a.n; i++) {
                if (__builtin_ctz(~i) == h) {
                    os << (is_fi ? "[" : ", [") << (i & (i + 1)) << ", " << i << "]: (" << a.v1[i] << ", " << a.v2[i] << ")";
                    is_fi = false;
                }
            }
            os << "]" << (h > 0 ? ",\n" : "\n");
        }
        
        return os << "]\n";
    }
};

// S: O(n^d), U: O(log(n)^d), Q: O(log(n)^d), M: O(n^d)
template<typename T, typename F = std::plus<T>, typename F_inv = std::minus<T>>
struct FenTree {
    int d, n, m, l;
    vector<T> v;
    T id;
    F f;
    F_inv f_inv;

    FenTree(int N,               T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(1), n(N), m(0), l(0), v(N, ID),         id(ID), f(f_), f_inv(f_inv_) {}

    FenTree(int N, int M,        T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(2), n(N), m(M), l(0), v(N * M, ID),     id(ID), f(f_), f_inv(f_inv_) {}

    FenTree(int N, int M, int L, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(3), n(N), m(M), l(L), v(N * M * L, ID), id(ID), f(f_), f_inv(f_inv_) {}

    FenTree(vector<T> a, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(1), n(a.size()), m(0), l(0), v(std::move(a)), id(ID), f(f_), f_inv(f_inv_) {
        for (int i = 0; i < n; i++) {
            int p = i | (i + 1);
            if (p < n) {
                v[p] = f(v[p], v[i]);}
        }
    }

    FenTree(const vector<vector<T>> &a, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        FenTree(a.size(), a.empty() ? 0 : a[0].size(), ID, f_, f_inv_) {
        for (int i = 0; i < n; i++) {
            int i_flat = i * m;
            for (int j = 0; j < m; j++) {
                v[i_flat + j] = a[i][j];}
        }

        for (int i = 0; i < n; i++) {
            int p = i | (i + 1);
            if (p < n) {
                int i_flat = i * m;
                int p_flat = p * m;
                for (int j = 0; j < m; j++) {
                    v[p_flat + j] = f(v[p_flat + j], v[i_flat + j]);}
            }
        }
        
        for (int i = 0; i < n; i++) {
            int i_flat = i * m;
            for (int j = 0; j < m; j++) {
                int p = j | (j + 1);
                if (p < m) {
                    v[i_flat + p] = f(v[i_flat + p], v[i_flat + j]);}
            }
        }
    }

    FenTree(const vector<vector<vector<T>>> &a, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        FenTree(a.size(), a.empty() ? 0 : a[0].size(), (a.empty() || a[0].empty()) ? 0 : a[0][0].size(), ID, f_, f_inv_) {
        for (int i = 0; i < n; i++) {
            int i_flat = i * m * l;
            for (int j = 0; j < m; j++) {
                int ij_flat = i_flat + j * l;
                for (int k = 0; k < l; k++) {
                    v[ij_flat + k] = a[i][j][k];}
            }
        }
        
        for (int i = 0; i < n; i++) {
            int p = i | (i + 1);
            if (p < n) {
                int i_flat = i * m * l;
                int p_flat = p * m * l;
                for (int j = 0; j < m; j++) {
                    int ij_flat = i_flat + j * l;
                    int pj_flat = p_flat + j * l;
                    for (int k = 0; k < l; k++) {
                        v[pj_flat + k] = f(v[pj_flat + k], v[ij_flat + k]);}
                }
            }
        }
        
        for (int i = 0; i < n; i++) {
            int i_flat = i * m * l;
            for (int j = 0; j < m; j++) {
                int p = j | (j + 1);
                if (p < m) {
                    int ij_flat = i_flat + j * l;
                    int ip_flat = i_flat + p * l;
                    for (int k = 0; k < l; k++) {
                        v[ip_flat + k] = f(v[ip_flat + k], v[ij_flat + k]);}
                }
            }
        }
        
        for (int i = 0; i < n; i++) {
            int i_flat = i * m * l;
            for (int j = 0; j < m; j++) {
                int ij_flat = i_flat + j * l;
                for (int k = 0; k < l; k++) {
                    int p = k | (k + 1);
                    if (p < l) {
                        v[ij_flat + p] = f(v[ij_flat + p], v[ij_flat + k]);}
                }
            }
        }
    }

    // 1D
    void update(int i_cur, T x) {
        assert(d == 1);

        for (int i = i_cur; i < n; i |= i + 1) {
            v[i] = f(v[i], x);}
    }

    T _query(int i_cur) const {
        assert(d == 1);
        
        T res = id;
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            res = f(res, v[i]);}
        
        return res;
    }

    T query(int i1, int i2) const {
        assert(d == 1);

        if (i1 > i2) {
            return id;}
        
        return f_inv(_query(i2), (i1 > 0 ? _query(i1 - 1) : id));
    }

    int lowerBound(T x) const {
        assert(d == 1);

        if (x <= id || n == 0) {
            return 0;}

        int cur = 0;
        for (int i = __lg(max(1, n)); i >= 0; i--) {
            int nxt = cur + (1 << i);
            if (nxt <= n && v[nxt - 1] < x) {
                x = f_inv(x, v[nxt - 1]);
                cur = nxt;
            }
        }

        return cur;
    }

    // 2D
    inline int getIdx(int i, int j) const {
        return i * m + j;
    }

    void update(int i_cur, int j_cur, T x) {
        assert(d == 2);

        for (int i = i_cur; i < n; i |= i + 1) {
            int i_flat = i * m;
            for (int j = j_cur; j < m; j |= j + 1) {
                v[i_flat + j] = f(v[i_flat + j], x);}
        }
    }

    T _query(int i_cur, int j_cur) const {
        assert(d == 2);
        
        if (i_cur < 0 || j_cur < 0) {
            return id;}
        
        T res = id;
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            int i_flat = i * m;
            for (int j = j_cur; j >= 0; j = (j & (j + 1)) - 1) {
                res = f(res, v[i_flat + j]);}
        }
        
        return res;
    }

    T query(int i1, int j1, int i2, int j2) const {
        assert(d == 2);

        if (i1 > i2 || j1 > j2) {
            return id;}
        
        T res = _query(i2, j2);
        res   = f_inv(res, _query(i1 - 1, j2));
        res   = f_inv(res, _query(i2, j1 - 1));
        res   = f(res, _query(i1 - 1, j1 - 1));
        
        return res;
    }

    // 3D
    inline int getIdx(int i, int j, int k) const {
        return (i * m + j) * l + k;
    }

    void update(int i_cur, int j_cur, int k_cur, T x) {
        assert(d == 3);

        for (int i = i_cur; i < n; i |= i + 1) {
            int i_flat = i * m * l;
            for (int j = j_cur; j < m; j |= j + 1) {
                int ij_flat = i_flat + j * l;
                for (int k = k_cur; k < l; k |= k + 1) {
                    v[ij_flat + k] = f(v[ij_flat + k], x);}
            }
        }
    }

    T _query(int i_cur, int j_cur, int k_cur) const {
        assert(d == 3);
        
        if (i_cur < 0 || j_cur < 0 || k_cur < 0) {
            return id;}
        
        T res = id;
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            int i_flat = i * m * l;
            for (int j = j_cur; j >= 0; j = (j & (j + 1)) - 1) {
                int ij_flat = i_flat + j * l;
                for (int k = k_cur; k >= 0; k = (k & (k + 1)) - 1) {
                    res = f(res, v[ij_flat + k]);}
            }
        }
        
        return res;
    }

    T query(int i1, int j1, int k1, int i2, int j2, int k2) const {
        assert(d == 3);

        if (i1 > i2 || j1 > j2 || k1 > k2) {
            return id;}

        T res = _query(i2, j2, k2);
        res   = f_inv(res, _query(i1 - 1, j2, k2));
        res   = f_inv(res, _query(i2, j1 - 1, k2));
        res   = f_inv(res, _query(i2, j2, k1 - 1));
        res   = f(res, _query(i1 - 1, j1 - 1, k2));
        res   = f(res, _query(i1 - 1, j2, k1 - 1));
        res   = f(res, _query(i2, j1 - 1, k1 - 1));
        res   = f_inv(res, _query(i1 - 1, j1 - 1, k1 - 1));

        return res;
    }

    friend ostream &operator<<(ostream &os, const FenTree &a) {
        if (a.n == 0) {
            return os << "[]";}
        
        os << "\n[\n";
        if (a.d == 1) {
            int mx_h = 0;
            for (int i = 0; i < a.n; i++) {
                mx_h = max(mx_h, __builtin_ctz(~i));}
            
            for (int h = mx_h; h >= 0; h--) {
                os << "  [";
                bool is_fi = true;
                for (int i = 0; i < a.n; i++) {
                    if (__builtin_ctz(~i) == h) {
                        os << (is_fi ? "[" : ", [") << (i & (i + 1)) << ", " << i << "]: " << a.v[i];
                        is_fi = false;
                    }
                }
                os << "]" << (h > 0 ? ",\n" : "\n");
            }
        } else if (a.d == 2) {
            for (int i = 0; i < a.n; i++) {
                os << "  [";
                for (int j = 0; j < a.m; j++) {
                    os << "[" 
                       << (i & (i + 1)) << ", " << i << "; " 
                       << (j & (j + 1)) << ", " << j << "]: " 
                       << a.v[i * a.m + j] << (j < a.m - 1 ? ", " : "");
                }
                os << "]" << (i < a.n - 1 ? ",\n" : "\n");
            }
        } else if (a.d == 3) {
            for (int i = 0; i < a.n; i++) {
                os << "  [\n";
                for (int j = 0; j < a.m; j++) {
                    os << "    [";
                    for (int k = 0; k < a.l; k++) {
                        os << "[" 
                           << (i & (i + 1)) << ", " << i << "; " 
                           << (j & (j + 1)) << ", " << j << "; " 
                           << (k & (k + 1)) << ", " << k << "]: " 
                           << a.v[(i * a.m + j) * a.l + k] << (k < a.l - 1 ? ", " : "");
                    }
                    os << "]" << (j < a.m - 1 ? ",\n" : "\n");
                }
                os << "  ]" << (i < a.n - 1 ? ",\n" : "\n");
            }
        }
        
        return os << "]\n";
    }
};

// 2.2.3 Square Root Decomposition

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

// S: O(q * log(q)), U: NA, Q: O(n * sqrt(q)), M: O(q)
struct Mo {
    struct Query {
        int l, r, i;
        lng hil_ord;
    };

    int n, q;
    vector<Query> Q;

    Mo(int N) : n(N), q(0) {}

    static lng getHilOrd(int x, int y, int pow = 20) {
        lng res = 0;
        for (int i = 1 << (pow - 1); i > 0; i >>= 1) {
            int xi = (x & i) > 0, yi = (y & i) > 0;            
            if (yi == 0) {
                x += (xi == 1) * ((1 << pow) - 2 * x - 1);
                y += (xi == 1) * ((1 << pow) - 2 * y - 1);
                std::swap(x, y);
            }
            
            res += lng(i) * i * ((3 * xi) ^ yi);
        }

        return res;
    }

    void addQuery(int l, int r) {
        Q.push_back({l, r, q++, getHilOrd(l, r)});
    }

    template<typename Add, typename Del, typename Ret>
    void solve(Add &&add, Del &&del, Ret &&ret) {
        if (q == 0) {
            return;}

        sort(Q.begin(), Q.end(), [&](const Query &a, const Query &b) {
            return a.hil_ord < b.hil_ord;
        });

        int l = 0, r = -1;
        for (const Query &cur : Q) {
            while (l > cur.l) {
                add(--l);}
            while (r < cur.r) {
                add(++r);}
            
            while (l < cur.l) { 
                del(l++);}
            while (r > cur.r) { 
                del(r--);}
            
            ret(cur.i);
        }
    }

    friend ostream &operator<<(ostream &os, const Mo &a) {
        if (a.q == 0) {
            return os << "[]\n";}
        
        os << "\n[\n";
        for (int i = 0; i < a.Q.size(); i++) {
            os << "  [Q" << i << " | id: " << a.Q[i].i << "]: "
               << "[l: " << a.Q[i].l << ", r: " << a.Q[i].r << ", hil_ord: " << a.Q[i].hil_ord << "]" 
               << (i < a.Q.size() - 1 ? ",\n" : "\n");
        }
        
        return os << "]\n";
    }
};

// 2.2.4 Segment Tree

// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename Mon>
struct SegTree {    
    using S = typename Mon::S;
    using F = typename Mon::F;

    int n;
    vector<S> tree;
    vector<F> lazy;

    template<typename T>
    SegTree(std::span<const T> &a) : 
        n(a.size()), 
        tree(2 * std::bit_ceil(static_cast<unsigned>(max(n, 1))), Mon::idS()), 
        lazy(2 * std::bit_ceil(static_cast<unsigned>(max(n, 1))), Mon::idF()) {
        if (n > 0) {
            build(1, 0, n - 1, a);}
    }

    template<typename T>
    void build(int i, int l, int r, std::span<const T> &a) {
        if (l == r) {
            tree[i] = Mon::init(l, a[l]);
            return;
        }

        int md = std::midpoint(l, r);
        build(2 * i,     l,      md, a);
        build(2 * i + 1, md + 1, r,  a);
        tree[i] = Mon::ope(tree[2 * i], tree[2 * i + 1]);
    }

    inline void apply(int i, F f) {
        tree[i] = Mon::map(f, tree[i]);
        lazy[i] = Mon::cmp(f, lazy[i]);
    }

    inline void push(int i) {
        if (lazy[i] == Mon::idF()) {
            return;}
        
        apply(2 * i,     lazy[i]);
        apply(2 * i + 1, lazy[i]);
        lazy[i] = Mon::idF();
    }

    void _update(int i, int l, int r, int ql, int qr, F f) {
        if (ql <= l && r <= qr) {
            apply(i, f);
            return;
        }

        push(i);
        
        int md = std::midpoint(l, r);
        if (ql <= md) {
            _update(2 * i,     l,      md, ql, qr, f);}
        if (qr > md) {
            _update(2 * i + 1, md + 1, r,  ql, qr, f);}
        tree[i] = Mon::ope(tree[2 * i], tree[2 * i + 1]);
    }

    void update(int l, int r, F f) { 
        l = max(l, 0); 
        r = min(r, n - 1);
        if (l <= r) {
            _update(1, 0, n - 1, l, r, f);}
    }

    template<class G>
    int _maxR(int i, int l, int r, int ql, G &g, S &acc) {
        if (r < ql) {
            return -1;}
        
        if (ql <= l) {
            S nac = Mon::ope(acc, tree[i]);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l - 1;} 

        push(i);
        int md = std::midpoint(l, r);
        int res = _maxR(2 * i, l, md, ql, g, acc);
        if (res != -1) {
            return res;}
        
        return _maxR(2 * i + 1, md + 1, r, ql, g, acc);
    }

    template<class G>
    int maxR(int l, G g) { // Returns max r in [l, n - 1] where g(query(l, r)) is true. If g(a[l]) is false, returns l - 1.
        l = max(l, 0);
        if (l > n - 1) {
            return l - 1;}

        S acc = Mon::idS();
        int res = _maxR(1, 0, n - 1, l, g, acc);
        
        return res == -1 ? n - 1 : res;
    }

    template<class G>
    int _minL(int i, int l, int r, int qr, G &g, S &acc) {
        if (l > qr) {
            return -1;}
        
        if (r <= qr) {
            S nac = Mon::ope(tree[i], acc);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l + 1;}

        push(i);
        int md = std::midpoint(l, r);
        int res = _minL(2 * i + 1, md + 1, r, qr, g, acc);
        if (res != -1) {
            return res;}

        return _minL(2 * i, l, md, qr, g, acc);
    }
    
    template<class G>
    int minL(int r, G g) { // Returns min l in [0, r] where g(query(l, r)) is true. If g(a[r]) is false, returns r + 1.
        r = min(r, n - 1);
        if (r < 0) {
            return 0;}

        S acc = Mon::idS();
        int res = _minL(1, 0, n - 1, r, g, acc);
        
        return res == -1 ? 0 : res;
    }

    S _query(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) {
            return tree[i];}
        
        push(i);
        
        int md = std::midpoint(l, r);
        if (qr <= md) {
            return _query(2 * i,     l,      md, ql, qr);}
        if (ql > md) {
            return _query(2 * i + 1, md + 1, r,  ql, qr);}
        
        return Mon::ope(_query(2 * i, l, md, ql, qr), _query(2 * i + 1, md + 1, r, ql, qr));
    }

    S query(int l, int r) { 
        l = max(l, 0); 
        r = min(r, n - 1);
        if (l > r) {
            return Mon::idS();}
        
        return _query(1, 0, n - 1, l, r); 
    }

    // Q: O(1)
    S queryAll() {
        return tree[1];
    }
};

// S: O(q * log(n)), U: O(log(n)), Q: O(log(n)), M: O(q * log(n))
template<typename Mon>
struct DynSegTree {
    using S = typename Mon::S;
    using F = typename Mon::F;

    struct Node {
        S val;
        F lzy;
        int lc = -1, rc = -1;

        Node(lng l, lng r) : val(Mon::defR(l, r)), lzy(Mon::idF()) {}
    };

    lng n;
    int root = -1;
    vector<Node> tree;
    
    DynSegTree(lng N) : n(N) {
        tree.reserve(500'000);
        if (n > 0) {
            root = createNode(0, n - 1);}
    }

    template<typename T>
    DynSegTree(std::span<const T> &a) : n(a.size()) {
        tree.reserve(4 * n);
        if (n > 0) {
            root = build(0, n - 1, a);}
    }

    // S: O(n)
    template<typename T>
    int build(lng l, lng r, std::span<const T> &a) {
        int cur = createNode(l, r);
        
        if (l == r) {
            tree[cur].val = Mon::init(l, a[l]);
            return cur;
        }

        lng md = std::midpoint(l, r);
        int nlc = build(l,      md, a);
        tree[cur].lc = nlc;
        int nrc = build(md + 1, r,  a);
        tree[cur].rc = nrc;
        tree[cur].val = Mon::ope(tree[nlc].val, tree[nrc].val);

        return cur;
    }

    int createNode(lng l, lng r) {
        tree.emplace_back(l, r);
        
        return tree.size() - 1;
    }

    inline void apply(int i, F f) {
        tree[i].val = Mon::map(f, tree[i].val);
        tree[i].lzy = Mon::cmp(f, tree[i].lzy);
    }

    inline void push(int i, lng l, lng r) {
        if (tree[i].lzy == Mon::idF()) {
            return;}
        
        lng md = std::midpoint(l, r);
        if (tree[i].lc == -1) {
            int nlc = createNode(l, md);
            tree[i].lc = nlc;
        }
        if (tree[i].rc == -1) {
            int nrc = createNode(md + 1, r);
            tree[i].rc = nrc;
        }

        apply(tree[i].lc, tree[i].lzy);
        apply(tree[i].rc, tree[i].lzy);
        tree[i].lzy = Mon::idF();
    }

    void _update(int i, lng l, lng r, lng ql, lng qr, F f) {
        if (ql <= l && r <= qr) {
            apply(i, f);
            return;
        }

        push(i, l, r);
        
        lng md = std::midpoint(l, r);
        if (ql <= md) {
            if (tree[i].lc == -1) {
                int nlc = createNode(l, md);
                tree[i].lc = nlc;
            }

            _update(tree[i].lc, l,      md, ql, qr, f);
        }
        if (qr > md) {
            if (tree[i].rc == -1) {
                int nrc = createNode(md + 1, r);
                tree[i].rc = nrc;
            }

            _update(tree[i].rc, md + 1, r,  ql, qr, f);
        }

        S l_val = tree[i].lc != -1 ? tree[tree[i].lc].val : Mon::defR(l, md);
        S r_val = tree[i].rc != -1 ? tree[tree[i].rc].val : Mon::defR(md + 1, r);
        tree[i].val = Mon::ope(l_val, r_val);
    }

    void update(lng l, lng r, F f) { 
        l = max(l, 0LL);
        r = min(r, n - 1);
        if (l <= r) {
            _update(root, 0, n - 1, l, r, f);}
    }

    template<class G>
    lng _maxR(int i, lng l, lng r, lng ql, G &g, S &acc) {
        if (r < ql) {
            return -1;}
        
        if (ql <= l) {
            S nac = Mon::ope(acc, i == -1 ? Mon::defR(l, r) : tree[i].val);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l - 1;}

        if (i != -1) {
            push(i, l, r);}

        lng md = std::midpoint(l, r);
        lng res = _maxR(i == -1 ? -1 : tree[i].lc, l, md, ql, g, acc);
        if (res != -1) {
            return res;}
        
        return _maxR(i == -1 ? -1 : tree[i].rc, md + 1, r, ql, g, acc);
    }

    template<class G>
    lng maxR(lng l, G g) { // Returns max r in [l, n - 1] where g(query(l, r)) is true. If g(a[l]) is false, returns l - 1.
        if (l < 0 || l > n - 1) {
            return l - 1;}

        S acc = Mon::idS();
        lng res = _maxR(root, 0, n - 1, l, g, acc);

        return res == -1 ? n - 1 : res;
    }

    template<class G>
    lng _minL(int i, lng l, lng r, lng qr, G &g, S &acc) {
        if (l > qr) {
            return -1;}
        
        if (r <= qr) {
            S nac = Mon::ope(i == -1 ? Mon::defR(l, r) : tree[i].val, acc);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l + 1;}

        if (i != -1) {
            push(i, l, r);}

        lng md = std::midpoint(l, r);
        lng res = _minL(i == -1 ? -1 : tree[i].rc, md + 1, r, qr, g, acc);
        if (res != -1) {
            return res;}
        
        return _minL(i == -1 ? -1 : tree[i].lc, l, md, qr, g, acc);
    }

    template<class G>
    lng minL(lng r, G g) { // Returns min l in [0, r] where g(query(l, r)) is true. If g(a[r]) is false, returns r + 1.
        if (r < 0 || r > n - 1) {
            return r + 1;}
        
        S acc = Mon::idS();
        lng res = _minL(root, 0, n - 1, r, g, acc);
        
        return res == -1 ? 0 : res;
    }

    S _query(int i, lng l, lng r, lng ql, lng qr) {
        if (i == -1) {
            if (max(l, ql) > min(r, qr)) {
                return Mon::idS();}

            return Mon::defR(max(l, ql), min(r, qr));
        }

        if (ql <= l && r <= qr) {
            return tree[i].val;}
        
        push(i, l, r);
        
        lng md = std::midpoint(l, r);
        if (qr <= md) {
            return _query(tree[i].lc, l,      md, ql, qr);}
        if (ql > md) {
            return _query(tree[i].rc, md + 1, r,  ql, qr);}
        
        return Mon::ope(_query(tree[i].lc, l, md, ql, qr), _query(tree[i].rc, md + 1, r, ql, qr));
    }

    S query(lng l, lng r) { 
        if (l < 0 || r > n - 1 || l > r) {
            return Mon::idS();}

        return _query(root, 0, n - 1, l, r); 
    }

    // Q: O(1)
    S queryAll() {
        return root == -1 ? Mon::defR(0, n - 1) : tree[root].val;
    }
};

// S: O(q * log(n)), U: O(log(n)), Q: O(log(n)), M: O(q * log(n))
template<typename Mon>
struct PerSegTree {
    using S = typename Mon::S;
    using F = typename Mon::F;

    struct Node {
        S val;
        F lzy;
        int lc = -1, rc = -1;

        Node(lng l, lng r) : val(Mon::defR(l, r)), lzy(Mon::idF()) {}
    };

    lng n;
    vector<int> roots;
    vector<Node> tree;

    PerSegTree(lng N) : n(N) {
        tree.reserve(10'000'000); 
        roots.push_back(cloneNode(-1, 0, n - 1));
    }

    template<typename T>
    PerSegTree(std::span<const T> &a) : n(a.size()) {
        tree.reserve(10'000'000); 
        roots.push_back(build(0, n - 1, a));
    }

    // S: O(n)
    template<typename T>
    int build(lng l, lng r, std::span<const T> &a) {
        int cur = cloneNode(-1, l, r);
        
        if (l == r) {
            tree[cur].val = Mon::init(l, a[l]);
            return cur;
        }

        lng md = std::midpoint(l, r);
        int nlc = build(l,      md, a);
        tree[cur].lc = nlc;
        int nrc = build(md + 1, r,  a);
        tree[cur].rc = nrc;
        tree[cur].val = Mon::ope(tree[nlc].val, tree[nrc].val);

        return cur;
    }

    int cloneNode(int i, lng l, lng r) {
        if (i == -1) {
            tree.emplace_back(l, r);
        } else {
            Node tmp = tree[i];
            tree.push_back(tmp);
        }

        return tree.size() - 1;
    }

    inline void apply(int i, F f) {
        tree[i].val = Mon::map(f, tree[i].val);
        tree[i].lzy = Mon::cmp(f, tree[i].lzy);
    }

    inline void push(int i, lng l, lng r) {
        if (tree[i].lzy == Mon::idF()) {
            return;}
        
        lng md = std::midpoint(l, r);
        int nlc = cloneNode(tree[i].lc, l,      md);
        tree[i].lc = nlc;
        int nrc = cloneNode(tree[i].rc, md + 1, r);
        tree[i].rc = nrc;

        apply(tree[i].lc, tree[i].lzy);
        apply(tree[i].rc, tree[i].lzy);
        tree[i].lzy = Mon::idF();
    }

    int _modify(int i1, int i2, lng l, lng r, lng ql, lng qr, F acc) {
        if (ql <= l && r <= qr) {
            int j = cloneNode(i2, l, r);
            apply(j, acc);
            return j;
        }

        int j = cloneNode(i1, l, r);
       
        push(j, l, r);

        int lc2 = -1, rc2 = -1;
        F nac = acc;
        if (i2 != -1) {
            lc2 = tree[i2].lc;
            rc2 = tree[i2].rc;
            nac = Mon::cmp(acc, tree[i2].lzy);
        }

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _modify(tree[j].lc, lc2, l,      md, ql, qr, nac);
            tree[j].lc = nlc;
        }
        if (qr > md) {
            int nrc = _modify(tree[j].rc, rc2, md + 1, r,  ql, qr, nac);
            tree[j].rc = nrc;
        }

        S l_val = tree[j].lc != -1 ? tree[tree[j].lc].val : Mon::defR(l, md);
        S r_val = tree[j].rc != -1 ? tree[tree[j].rc].val : Mon::defR(md + 1, r);
        tree[j].val = Mon::ope(l_val, r_val);

        return j;
    }

    int modify(int ver1, int ver2, lng l, lng r) {
        if (l < 0 || r > n - 1 || l > r) {
            int cur = roots[ver1];
            roots.push_back(cur);
            return roots.size() - 1;
        }
        
        int cur = _modify(roots[ver1], roots[ver2], 0, n - 1, l, r, Mon::idF());
        roots.push_back(cur);
        
        return roots.size() - 1;
    }

    int _update(int i, lng l, lng r, lng ql, lng qr, F f) {
        int j = cloneNode(i, l, r);
        
        if (ql <= l && r <= qr) {
            apply(j, f);
            return j;
        }

        push(j, l, r);
        
        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _update(tree[j].lc, l,      md, ql, qr, f);
            tree[j].lc = nlc;
        }
        if (qr > md)  {
            int nrc = _update(tree[j].rc, md + 1, r,  ql, qr, f);
            tree[j].rc = nrc;
        }

        S l_val = tree[j].lc != -1 ? tree[tree[j].lc].val : Mon::defR(l, md);
        S r_val = tree[j].rc != -1 ? tree[tree[j].rc].val : Mon::defR(md + 1, r);
        tree[j].val = Mon::ope(l_val, r_val);
        
        return j;
    }

    int update(int ver, lng l, lng r, F f) { 
        if (l < 0 || r > n - 1 || l > r) {
            int cur = roots[ver];
            roots.push_back(cur);
            return roots.size() - 1;
        }
        
        int cur = _update(roots[ver], 0, n - 1, l, r, f);
        roots.push_back(cur);

        return roots.size() - 1;
    }

    template<class G>
    lng _maxR(int i, lng l, lng r, lng ql, G &g, S &acc, F tac) {
        if (r < ql) {
            return -1;}
        
        if (ql <= l) {
            S val = i == -1 ? Mon::defR(l, r) : tree[i].val;
            val = Mon::map(tac, val);
            S nac = Mon::ope(acc, val);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l - 1;}

        tac = i == -1 ? tac : Mon::cmp(tac, tree[i].lzy);
 
        lng md = std::midpoint(l, r);
        lng res = _maxR(i == -1 ? -1 : tree[i].lc, l, md, ql, g, acc, tac);
        if (res != -1) {
            return res;}
        
        return _maxR(i == -1 ? -1 : tree[i].rc, md + 1, r, ql, g, acc, tac);
    }

    template<class G>
    lng maxR(int ver, lng l, G g) { // Returns max r in [l, n - 1] where g(query(l, r)) is true. If g(a[l]) is false, returns l - 1.
        if (l < 0 || l > n - 1) {
            return l - 1;}

        S acc = Mon::idS();
        lng res = _maxR(roots[ver], 0, n - 1, l, g, acc, Mon::idF());
        
        return res == -1 ? n - 1 : res;
    }

    template<class G>
    lng _minL(int i, lng l, lng r, lng qr, G &g, S &acc, F tac) {
        if (l > qr) {
            return -1;}
        
        if (r <= qr) {
            S val = i == -1 ? Mon::defR(l, r) : tree[i].val;
            val = Mon::map(tac, val);
            S nac = Mon::ope(val, acc);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l + 1;}
        
        tac = i == -1 ? tac : Mon::cmp(tac, tree[i].lzy);

        lng md = std::midpoint(l, r);
        lng res = _minL(i == -1 ? -1 : tree[i].rc, md + 1, r, qr, g, acc, tac);
        if (res != -1) {
            return res;}
        
        return _minL(i == -1 ? -1 : tree[i].lc, l, md, qr, g, acc, tac);
    }

    template<class G>
    lng minL(int ver, lng r, G g) { // Returns min l in [0, r] where g(query(l, r)) is true. If g(a[r]) is false, returns r + 1.
        if (r < 0 || r > n - 1) {
            return r + 1;}
        
        S acc = Mon::idS();
        lng res = _minL(roots[ver], 0, n - 1, r, g, acc, Mon::idF());
        
        return res == -1 ? 0 : res;
    }

    S _query(int i, lng l, lng r, lng ql, lng qr, F acc) {
        if (i == -1) {
            if (max(l, ql) > min(r, qr)) {
                return Mon::idS();}

            return Mon::map(acc, Mon::defR(max(l, ql), min(r, qr))); 
        }

        if (ql <= l && r <= qr) {
            return Mon::map(acc, tree[i].val);}

        F nac = Mon::cmp(acc, tree[i].lzy); 

        lng md = std::midpoint(l, r);
        if (qr <= md) {
            return _query(tree[i].lc, l,      md, ql, qr, nac);}
        if (ql > md) {
            return _query(tree[i].rc, md + 1, r,  ql, qr, nac);}
        
        return Mon::ope(_query(tree[i].lc, l, md, ql, qr, nac), _query(tree[i].rc, md + 1, r, ql, qr, nac));
    }

    S query(int ver, lng l, lng r) { 
        if (l < 0 || r > n - 1 || l > r) {
            return Mon::idS();}

        return _query(roots[ver], 0, n - 1, l, r, Mon::idF()); 
    }

    // Q: O(1)
    S queryAll(int ver) {
        return roots[ver] == -1 ? Mon::defR(0, n - 1) : tree[roots[ver]].val;
    }
};

// S: O(n^2), U: O(log(n)^2), Q: O(log(n)^2), M: O(n^2)
template<typename Mon>
struct SegTree2D {
    using S = typename Mon::S;
    using F = typename Mon::F;

    int n, m;
    vector<vector<S>> tree;

    template<typename T>
    SegTree2D(const vector<vector<T>> &a) : n(a.size()), m(a.empty() ? 0 : a[0].size()) {
        if (n == 0 || m == 0) {
            return;}
        
        tree.assign(4 * n, vector<S>(4 * m, Mon::idS()));
        build(1, 0, n - 1, a);
    }

    template<typename T>
    void build(int i, int l1, int r1, const vector<vector<T>> &a) {
        if (l1 != r1) {
            int md = l1 + (r1 - l1) / 2;
            build(2 * i,     l1,     md, a);
            build(2 * i + 1, md + 1, r1, a);
        }
        
        _build(i, l1, r1, 1, 0, m - 1, a);
    }

    template<typename T>
    void _build(int i, int l1, int r1, int j, int l2, int r2, const vector<vector<T>> &a) {
        if (l2 == r2) {
            if (l1 == r1) {
                tree[i][j] = Mon::init(l2, a[l1][l2]);
            } else {
                tree[i][j] = Mon::ope(tree[2 * i][j], tree[2 * i + 1][j]);
            }
            return;
        }

        int md = l2 + (r2 - l2) / 2;
        _build(i, l1, r1, 2 * j,     l2,     md, a);
        _build(i, l1, r1, 2 * j + 1, md + 1, r2, a);
        tree[i][j] = Mon::ope(tree[i][2 * j], tree[i][2 * j + 1]);
    }

    void _update_j(int i, int l1, int r1, int j, int l2, int r2, int qj, F f) {
        if (l2 == r2) {
            if (l1 == r1) {
                tree[i][j] = Mon::map(f, tree[i][j]);
            } else {
                tree[i][j] = Mon::ope(tree[2 * i][j], tree[2 * i + 1][j]);
            }
            return;
        }

        int md = l2 + (r2 - l2) / 2;
        if (qj <= md) {
            _update_j(i, l1, r1, 2 * j,     l2,     md, qj, f);
        } else {
            _update_j(i, l1, r1, 2 * j + 1, md + 1, r2, qj, f);
        }    
        if (l1 == r1) {
            tree[i][j] = Mon::ope(tree[i][2 * j], tree[i][2 * j + 1]);
        } else {
            tree[i][j] = Mon::ope(tree[2 * i][j], tree[2 * i + 1][j]);
        }
    }

    void _update_i(int i, int l1, int r1, int qi, int qj, F f) {
        if (l1 != r1) {
            int md = l1 + (r1 - l1) / 2;
            if (qi <= md) {
                _update_i(2 * i,     l1,     md, qi, qj, f);
            } else {
                _update_i(2 * i + 1, md + 1, r1, qi, qj, f);
            }
        }
        
        _update_j(i, l1, r1, 1, 0, m - 1, qj, f);
    }

    void update(int i, int j, F f) {
        if (i >= 0 && i < n && j >= 0 && j < m) {
            _update_i(1, 0, n - 1, i, j, f);}
    }

    S _query_j(int i, int j, int l2, int r2, int qj1, int qj2) {
        if (qj1 <= l2 && r2 <= qj2) {
            return tree[i][j];}

        int md = l2 + (r2 - l2) / 2;
        if (qj2 <= md) {
            return _query_j(i, 2 * j,     l2,     md, qj1, qj2);}
        if (qj1 > md) {
            return _query_j(i, 2 * j + 1, md + 1, r2, qj1, qj2);}

        return Mon::ope(_query_j(i, 2 * j,     l2,     md, qj1, qj2), 
                        _query_j(i, 2 * j + 1, md + 1, r2, qj1, qj2));
    }

    S _query_i(int i, int l1, int r1, int qi1, int qj1, int qi2, int qj2) {
        if (qi1 <= l1 && r1 <= qi2) {
            return _query_j(i, 1, 0, m - 1, qj1, qj2);}

        int md = l1 + (r1 - l1) / 2;
        if (qi2 <= md) {
            return _query_i(2 * i,     l1,     md, qi1, qj1, qi2, qj2);}
        if (qi1 > md) {
            return _query_i(2 * i + 1, md + 1, r1, qi1, qj1, qi2, qj2);}

        return Mon::ope(_query_i(2 * i,     l1,     md, qi1, qj1, qi2, qj2), 
                        _query_i(2 * i + 1, md + 1, r1, qi1, qj1, qi2, qj2));
    }

    S query(int i1, int j1, int i2, int j2) {
        if (i1 < 0 || i2 > n - 1 || i1 > i2 || j1 < 0 || j2 > m - 1 || j1 > j2) {
            return Mon::idS();}
        
        return _query_i(1, 0, n - 1, i1, j1, i2, j2);
    }
    
    // Q: O(1)
    S queryAll() {
        if (n == 0 || m == 0) {
            return Mon::idS();}
            
        return tree[1][1];
    }
};

// S: O(n), U: O(log(n)^2), Q: O(log(n)), M: O(n)
struct SegTreeBeats {
    struct Node {
        lng sum;
        lng mx1, mx2;
        int mxc;
        lng mn1, mn2;
        int mnc;

        bool to_set;
        lng add_upd, set_upd;
    };

    int n;
    vector<Node> tree;

    template<typename T>
    SegTreeBeats(std::span<const T> &a) : n(a.size()), tree(4 * a.size()) {
        if (n > 0) {
            build(1, 0, n - 1, a);}
    }

    template<typename T>
    void build(int i, int l, int r, std::span<const T> &a) {
        if (l == r) {
            tree[i].sum = a[l];
            tree[i].mx1 = a[l];
            tree[i].mx2 = -INF64; 
            tree[i].mxc = 1;
            tree[i].mn1 = a[l]; 
            tree[i].mn2 = INF64;  
            tree[i].mnc = 1;
            tree[i].to_set = false;
            tree[i].add_upd = 0; 
            tree[i].set_upd = 0;
            return;
        }

        int md = std::midpoint(l, r);
        build(2 * i,     l,      md, a);
        build(2 * i + 1, md + 1, r,  a);
        
        pull(i);
        tree[i].to_set = false; 
        tree[i].add_upd = 0; 
        tree[i].set_upd = 0;
    }

    inline void pull(int i) {
        int lc = 2 * i, rc = 2 * i + 1;

        tree[i].sum = tree[lc].sum + tree[rc].sum;
        if (tree[lc].mx1 == tree[rc].mx1) {
            tree[i].mx1 = tree[lc].mx1;
            tree[i].mx2 = max(tree[lc].mx2, tree[rc].mx2);
            tree[i].mxc = tree[lc].mxc + tree[rc].mxc;
        } else if (tree[lc].mx1 > tree[rc].mx1) {
            tree[i].mx1 = tree[lc].mx1;
            tree[i].mx2 = max(tree[lc].mx2, tree[rc].mx1);
            tree[i].mxc = tree[lc].mxc;
        } else {
            tree[i].mx1 = tree[rc].mx1;
            tree[i].mx2 = max(tree[lc].mx1, tree[rc].mx2);
            tree[i].mxc = tree[rc].mxc;
        }
        if (tree[lc].mn1 == tree[rc].mn1) {
            tree[i].mn1 = tree[lc].mn1;
            tree[i].mn2 = min(tree[lc].mn2, tree[rc].mn2);
            tree[i].mnc = tree[lc].mnc + tree[rc].mnc;
        } else if (tree[lc].mn1 < tree[rc].mn1) {
            tree[i].mn1 = tree[lc].mn1;
            tree[i].mn2 = min(tree[lc].mn2, tree[rc].mn1);
            tree[i].mnc = tree[lc].mnc;
        } else {
            tree[i].mn1 = tree[rc].mn1;
            tree[i].mn2 = min(tree[lc].mn1, tree[rc].mn2);
            tree[i].mnc = tree[rc].mnc;
        }
    }
    
    inline void addApply(int i, int len, lng x) {        
        tree[i].sum += x * len;
        tree[i].mx1 += x; 
        if (tree[i].mx2 != -INF64) {
            tree[i].mx2 += x;}
        tree[i].mn1 += x; 
        if (tree[i].mn2 != INF64) {
            tree[i].mn2 += x;}

        if (tree[i].to_set) {
            tree[i].set_upd += x;
        } else {
            tree[i].add_upd += x;
        }
    }

    inline void chmaxApply(int i, lng x) {
        if (x <= tree[i].mn1) {
            return;}
        
        tree[i].sum += (x - tree[i].mn1) * tree[i].mnc;
        if (tree[i].mx1 == tree[i].mn1) {
            tree[i].mx1 = x;}
        if (tree[i].mx2 == tree[i].mn1) {
            tree[i].mx2 = x;}
        tree[i].mn1 = x;
    }

    inline void chminApply(int i, lng x) {
        if (x >= tree[i].mx1) {
            return;}
        
        tree[i].sum -= (tree[i].mx1 - x) * tree[i].mxc;
        if (tree[i].mn1 == tree[i].mx1) {
            tree[i].mn1 = x;}
        if (tree[i].mn2 == tree[i].mx1) {
            tree[i].mn2 = x;}
        tree[i].mx1 = x;
    }

    inline void setApply(int i, int len, lng x) {
        tree[i].sum = x * len;
        tree[i].mx1 = x; 
        tree[i].mx2 = -INF64; 
        tree[i].mxc = len;
        tree[i].mn1 = x;
        tree[i].mn2 = INF64;  
        tree[i].mnc = len;

        tree[i].to_set = true; 
        tree[i].add_upd = 0; 
        tree[i].set_upd = x;
    }

    inline void push(int i, int l, int r) {
        int md = std::midpoint(l, r);
        int lc = 2 * i, rc = 2 * i + 1;

        if (tree[i].to_set) {
            setApply(lc, md - l + 1, tree[i].set_upd);
            setApply(rc, r - md,     tree[i].set_upd);
            tree[i].to_set = false;
        }

        if (tree[i].add_upd != 0) {
            addApply(lc, md - l + 1, tree[i].add_upd);
            addApply(rc, r - md,     tree[i].add_upd);
            tree[i].add_upd = 0;
        }

        if (tree[i].mx1 < tree[lc].mx1) {
            chminApply(lc, tree[i].mx1);}
        if (tree[i].mx1 < tree[rc].mx1) {
            chminApply(rc, tree[i].mx1);}

        if (tree[i].mn1 > tree[lc].mn1) {
            chmaxApply(lc, tree[i].mn1);}
        if (tree[i].mn1 > tree[rc].mn1) {
            chmaxApply(rc, tree[i].mn1);}
    }

    void _addUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (ql <= l && r <= qr) {
            addApply(i, r - l + 1, x);
            return;
        }
        
        push(i, l, r);
        
        int md = std::midpoint(l, r);
        if (ql <= md) {
            _addUpdate(2 * i,     l,      md, ql, qr, x);}
        if (qr > md)  {
            _addUpdate(2 * i + 1, md + 1, r,  ql, qr, x);}
        
        pull(i);
    }

    void _chmaxUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (x <= tree[i].mn1) {
            return;}
        
        if (ql <= l && r <= qr && x < tree[i].mn2) {
            chmaxApply(i, x);
            return;
        }
        
        push(i, l, r);
        
        int md = std::midpoint(l, r);
        if (ql <= md) {
            _chmaxUpdate(2 * i,     l,      md, ql, qr, x);}
        if (qr > md)  {
            _chmaxUpdate(2 * i + 1, md + 1, r,  ql, qr, x);}
        
        pull(i);
    }

    void _chminUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (x >= tree[i].mx1) {
            return;}
        
        if (ql <= l && r <= qr && x > tree[i].mx2) {
            chminApply(i, x);
            return;
        }
        
        push(i, l, r);
        
        int md = std::midpoint(l, r);
        if (ql <= md) {
            _chminUpdate(2 * i,     l,      md, ql, qr, x);}
        if (qr > md)  {
            _chminUpdate(2 * i + 1, md + 1, r,  ql, qr, x);}
        
        pull(i);
    }

    void _setUpdate(int i, int l, int r, int ql, int qr, lng x) {
        if (ql <= l && r <= qr) {
            setApply(i, r - l + 1, x);
            return;
        }
        
        push(i, l, r);
        
        int md = std::midpoint(l, r);
        if (ql <= md) {
            _setUpdate(2 * i,     l,      md, ql, qr, x);}
        if (qr > md)  {
            _setUpdate(2 * i + 1, md + 1, r,  ql, qr, x);}
        
        pull(i);
    }

    // U: O(log(n))
    void addUpdate(int l, int r, lng x) {
        if (l >= 0 && l <= r && r < n) {
            _addUpdate(1, 0, n - 1, l, r, x);}
    }

    void chminUpdate(int l, int r, lng x) {
        if (l >= 0 && l <= r && r < n) {
            _chminUpdate(1, 0, n - 1, l, r, x);}
    }

    void chmaxUpdate(int l, int r, lng x) {
        if (l >= 0 && l <= r && r < n) {
            _chmaxUpdate(1, 0, n - 1, l, r, x);}
    }

    // U: O(log(n))
    void setUpdate(int l, int r, lng x) {
        if (l >= 0 && l <= r && r < n) {
            _setUpdate(1, 0, n - 1, l, r, x);}
    }

    lng _maxQuery(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) {
            return tree[i].mx1;}
        
        push(i, l, r);
        
        int md = std::midpoint(l, r);
        lng res = -INF64;
        if (ql <= md) {
            res = max(res, _maxQuery(2 * i,     l,      md, ql, qr));}
        if (qr > md)  {
            res = max(res, _maxQuery(2 * i + 1, md + 1, r,  ql, qr));}
        
        return res;
    }

    lng _minQuery(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) {
            return tree[i].mn1;}
        
        push(i, l, r);
        
        int md = std::midpoint(l, r);
        lng res = INF64;
        if (ql <= md) {
            res = min(res, _minQuery(2 * i,     l,      md, ql, qr));}
        if (qr > md)  {
            res = min(res, _minQuery(2 * i + 1, md + 1, r,  ql, qr));}
        
        return res;
    }

    lng _sumQuery(int i, int l, int r, int ql, int qr) {
        if (ql <= l && r <= qr) {
            return tree[i].sum;}
        
        push(i, l, r);
        
        int md = std::midpoint(l, r);
        lng res = 0;
        if (ql <= md) {
            res += _sumQuery(2 * i,     l,      md, ql, qr);}
        if (qr > md)  {
            res += _sumQuery(2 * i + 1, md + 1, r,  ql, qr);}
        
        return res;
    }

    lng maxQuery(int l, int r) {
        if (l < 0 || r > n - 1 || l > r) {
            return -INF64;}
            
        return _maxQuery(1, 0, n - 1, l, r);
    }

    lng minQuery(int l, int r) {
        if (l < 0 || r > n - 1 || l > r) {
            return INF64;}
            
        return _minQuery(1, 0, n - 1, l, r);
    }

    lng sumQuery(int l, int r) {
        if (l < 0 || r > n - 1 || l > r) {
            return 0;}
            
        return _sumQuery(1, 0, n - 1, l, r);
    }
};

// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n))
struct DynMergeSortTree {
    struct Node {
        indexed_set<pair<lng, int>> j_set;
        int lc = -1, rc = -1;
    };

    lng mn_i, mx_i;
    int root = -1;
    vector<Node> tree;

    bool is_1d = false;
    int cnt = 0; 
    vector<lng> v;

    DynMergeSortTree(const vector<lng> &a) : mn_i(0), mx_i(int(a.size()) - 1), is_1d(true), v(a) {
        tree.reserve(2'500'000); 
        root = createNode();
        for (int i = 0; i < int(a.size()); i++) {
            _insert(root, mn_i, mx_i, i, v[i], i);}
    }

    DynMergeSortTree(lng Mn_i, lng Mx_i) : mn_i(Mn_i), mx_i(Mx_i) {
        tree.reserve(2'500'000); 
        root = createNode();
    }

    int createNode() {
        tree.emplace_back();
        
        return tree.size() - 1;
    }

    int _cntRan(int u, lng l, lng r, lng i1, lng i2, lng j1, lng j2) {
        if (u == -1 || i1 > r || i2 < l) {
            return 0;}

        if (i1 <= l && r <= i2) {
            return tree[u].j_set.order_of_key({j2 + 1, -1}) - tree[u].j_set.order_of_key({j1, -1});}

        lng md = std::midpoint(l, r);
        return _cntRan(tree[u].lc, l,      md, i1, i2, j1, j2) + 
               _cntRan(tree[u].rc, md + 1, r,  i1, i2, j1, j2);
    }

    void _collect(int u, lng l, lng r, lng i1, lng i2, vector<int> &act) {
        if (u == -1 || i1 > r || i2 < l) {
            return;}

        if (i1 <= l && r <= i2) {
            act.push_back(u);
            return;
        }

        lng md = std::midpoint(l, r);
        _collect(tree[u].lc, l,      md, i1, i2, act);
        _collect(tree[u].rc, md + 1, r,  i1, i2, act);
    }

    void _erase(int u, lng l, lng r, lng i, lng j, int id) {
        if (u == -1) {
            return;}
        
        tree[u].j_set.erase({j, id});

        if (l == r) {
            return;}

        lng md = std::midpoint(l, r);
        if (i <= md) {
            _erase(tree[u].lc, l,      md, i, j, id);
        } else {
            _erase(tree[u].rc, md + 1, r,  i, j, id);
        }
    }

    void _insert(int u, lng l, lng r, lng i, lng j, int id) {
        tree[u].j_set.insert({j, id});

        if (l == r) {
            return;}

        lng md = std::midpoint(l, r);
        if (i <= md) {
            if (tree[u].lc == -1) {
                int nlc = createNode();
                tree[u].lc = nlc;
            }

            _insert(tree[u].lc, l,      md, i, j, id);
        } else {
            if (tree[u].rc == -1) {
                int nrc = createNode();
                tree[u].rc = nrc;
            }            
            
            _insert(tree[u].rc, md + 1, r,  i, j, id);
        }
    }

    lng _maxLeq(int u, lng l, lng r, lng i1, lng i2, lng x) {
        if (u == -1 || i1 > r || i2 < l) {
            return -INF64;}

        if (i1 <= l && r <= i2) {
            auto it = tree[u].j_set.upper_bound({x, INF32}); 
            if (it == tree[u].j_set.begin()) {
                return -INF64;}
            it--;

            return it->first;
        }

        lng md = std::midpoint(l, r);
        return max(_maxLeq(tree[u].lc, l,      md, i1, i2, x), 
                   _maxLeq(tree[u].rc, md + 1, r,  i1, i2, x));
    }

    lng _minGeq(int u, lng l, lng r, lng i1, lng i2, lng x) {
        if (u == -1 || i1 > r || i2 < l) {
            return INF64;}

        if (i1 <= l && r <= i2) {
            auto it = tree[u].j_set.lower_bound({x, -1});
            if (it == tree[u].j_set.end()) {
                return INF64;}
            
            return it->first;
        }

        lng md = std::midpoint(l, r);
        return min(_minGeq(tree[u].lc, l,      md, i1, i2, x), 
                   _minGeq(tree[u].rc, md + 1, r,  i1, i2, x));
    }

    // 1D
    void update1D(int i, lng x) {
        assert(is_1d);
        assert(0 <= i && i <= mx_i);
        
        _erase(root, mn_i, mx_i, i, v[i], i); 
        v[i] = x;                         
        _insert(root, mn_i, mx_i, i, v[i], i); 
    }

    int cntEql1D(int l, int r, lng x) {
        assert(is_1d && mn_i <= l && l <= r && r <= mx_i);

        return _cntRan(root, mn_i, mx_i, l, r, x, x);
    }

    int cntRan1D(int l, int r, lng j1, lng j2) {
        assert(is_1d && mn_i <= l && l <= r && r <= mx_i);

        if (j1 > j2) {
            return 0;}
        
        return _cntRan(root, mn_i, mx_i, l, r, j1, j2);
    }

    // Q: O(log(n)^3)
    lng kthMin1D(int l, int r, int k) {
        assert(is_1d);

        vector<int> act;
        _collect(root, mn_i, mx_i, l, r, act);

        lng lo = -INF64 - 1, hi = INF64 + 1;
        while (hi - lo > 1) {
            lng m = lo + (hi - lo) / 2;
            int cnt = 0;
            for (int u : act) {
                cnt += tree[u].j_set.order_of_key({m + 1, -1});}
            
            if (cnt >= k) {
                hi = m;
            } else {
                lo = m;
            }
        }
        
        return hi;
    }

    lng maxLeq1D(int l, int r, lng x) {
        assert(is_1d);

        if (l > r) {
            return -INF64;}
        
        return _maxLeq(root, mn_i, mx_i, l, r, x);
    }

    lng minGeq1D(int l, int r, lng x) {
        assert(is_1d);
        
        if (l > r) {
            return INF64;}
        
        return _minGeq(root, mn_i, mx_i, l, r, x);
    }

    // 2D
    int insert2D(lng i, lng j) {
        assert(!is_1d);

        int id = cnt++;
        _insert(root, mn_i, mx_i, i, j, id);
        
        return id; 
    }

    void erase2D(lng i, lng j, int id) {
        assert(!is_1d);

        _erase(root, mn_i, mx_i, i, j, id);
    }

    int cntRan2D(lng i1, lng i2, lng j1, lng j2) {
        assert(!is_1d);

        if (i1 > i2 || j1 > j2) {
            return 0;}
        
        return _cntRan(root, mn_i, mx_i, i1, i2, j1, j2);
    }

    int cntEql2D(lng i1, lng i2, lng x) {
        assert(!is_1d);

        return _cntRan(root, mn_i, mx_i, i1, i2, x, x);
    }

    // Q: O(log(n)^3)
    lng kthMin2D(lng i1, lng i2, int k) {
        assert(!is_1d);

        vector<int> act;
        _collect(root, mn_i, mx_i, i1, i2, act);

        lng lo = -INF64 - 1, hi = INF64 + 1;
        while (hi - lo > 1) {
            lng m = lo + (hi - lo) / 2;
            int cnt = 0;
            for (int u : act) {
                cnt += tree[u].j_set.order_of_key({m + 1, -1});}
            
            if (cnt >= k) {
                hi = m;
            } else {
                lo = m;
            }
        }
        
        return hi;
    }

    lng maxLeq2D(lng i1, lng i2, lng x) {
        assert(!is_1d);

        if (i1 > i2) {
            return -INF64;}
        
        return _maxLeq(root, mn_i, mx_i, i1, i2, x);
    }

    lng minGeq2D(lng i1, lng i2, lng x) {
        assert(!is_1d);
        
        if (i1 > i2) {
            return INF64;}
        
        return _minGeq(root, mn_i, mx_i, i1, i2, x);
    }
};

// S: O(n * log(n)), U: N/A, Q: O(log(n)), M: O(n * log(n))
struct KSPerSegTree {
    struct Mon {
        struct S { 
            int cnt;
            lng sum;
        };
        
        struct F {
            int cnt_upd;
            lng sum_upd;

            bool operator==(const F o) const {
                return cnt_upd == o.cnt_upd && sum_upd == o.sum_upd;
            }

            bool operator!=(const F o) const {
                return !(*this == o);
            }
        };

        static constexpr inline S idS() { 
            return {0, 0}; 
        }
        
        static constexpr inline F idF() { 
            return {0, 0}; 
        }
        
        static S defR(int l, int r) { 
            return {0, 0}; 
        }
        
        static S init(int i, lng x) { 
            return {0, 0}; 
        }
        
        static constexpr inline S ope(const S &a, const S &b) { 
            return {a.cnt + b.cnt, a.sum + b.sum}; 
        }
        
        static constexpr inline S map(const F &f, const S &a) { 
            return {a.cnt + f.cnt_upd, a.sum + f.sum_upd}; 
        }
        
        static constexpr inline F cmp(const F &f, const F &g) { 
            return {f.cnt_upd + g.cnt_upd, f.sum_upd + g.sum_upd}; 
        }
    };

    int n;
    vector<lng> v;
    std::optional<PerSegTree<Mon>> segt;

    KSPerSegTree(const vector<lng> &a) : n(a.size()), v(a) {
        sort(v.begin(), v.end());
        v.erase(unique(v.begin(), v.end()), v.end());
        segt.emplace(v.size());

        for (int i = 0; i < n; i++) {
            int ord = lower_bound(v.begin(), v.end(), a[i]) - v.begin();
            segt->update(i, ord, ord, {1, a[i]});
        }
    }

    int _cntLeq(int l, int r, lng x) {
        auto it = upper_bound(v.begin(), v.end(), x);
        if (it == v.begin()) {
            return 0;}

        int ord = (it - v.begin()) - 1; 
        int l_cnt = segt->query(l,     0, ord).cnt;
        int r_cnt = segt->query(r + 1, 0, ord).cnt;
        
        return r_cnt - l_cnt;
    }

    int cntEql(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);

        auto it = lower_bound(v.begin(), v.end(), x);
        if (it == v.end() || *it != x) {
            return 0;}
        
        int ord = it - v.begin();
        int l_cnt = segt->query(l,     ord, ord).cnt;
        int r_cnt = segt->query(r + 1, ord, ord).cnt;

        return r_cnt - l_cnt;
    }

    int cntRan(int l, int r, lng x1, lng x2) {
        assert(0 <= l && l <= r && r < n);

        if (x1 > x2) {
            return 0;}

        return _cntLeq(l, r, x2) - _cntLeq(l, r, x1 - 1);
    }

    int _kthMin(int l_i, int r_i, int l, int r, int k) {
        if (l == r) {
            return l;}

        int cnt = 0;
        int l_lc = l_i == -1 ? -1 : segt->tree[l_i].lc;
        if (l_lc != -1) {
            cnt -= segt->tree[l_lc].val.cnt;}
        int r_lc = r_i == -1 ? -1 : segt->tree[r_i].lc;
        if (r_lc != -1) {
            cnt += segt->tree[r_lc].val.cnt;}
        
        int md = std::midpoint(l, r);
        if (cnt >= k) {
            return _kthMin(l_lc, r_lc, l, md, k);}

        int l_rc = l_i == -1 ? -1 : segt->tree[l_i].rc;
        int r_rc = r_i == -1 ? -1 : segt->tree[r_i].rc;
        
        return _kthMin(l_rc, r_rc, md + 1, r, k - cnt);
    }

    lng kthMin(int l, int r, int k) {
        assert(0 <= l && l <= r && r < n);
        assert(1 <= k && k <= r - l + 1);

        int ord = _kthMin(segt->roots[l], segt->roots[r + 1], 0, v.size() - 1, k);
        
        return v[ord];
    }

    lng _kthSum(int l_i, int r_i, int l, int r, int k) {
        if (k == 0) {
            return 0;}
            
        if (l == r) {
            return lng(k) * v[l];}

        int l_cnt = 0;
        lng l_sum = 0;
        
        int l_lc = l_i == -1 ? -1 : segt->tree[l_i].lc;
        if (l_lc != -1) {
            l_cnt -= segt->tree[l_lc].val.cnt;
            l_sum -= segt->tree[l_lc].val.sum;
        }
        int r_lc = r_i == -1 ? -1 : segt->tree[r_i].lc;
        if (r_lc != -1) {
            l_cnt += segt->tree[r_lc].val.cnt;
            l_sum += segt->tree[r_lc].val.sum;
        }

        int md = std::midpoint(l, r);
        if (l_cnt >= k) {
            return _kthSum(l_lc, r_lc, l, md, k);
        } else {
            int l_rc = l_i == -1 ? -1 : segt->tree[l_i].rc;
            int r_rc = r_i == -1 ? -1 : segt->tree[r_i].rc;
            return l_sum + _kthSum(l_rc, r_rc, md + 1, r, k - l_cnt);
        }
    }

    lng kthSum(int l, int r, int k) {
        assert(0 <= l && l <= r && r < n);
        assert(0 <= k && k <= r - l + 1);
        
        return _kthSum(segt->roots[l], segt->roots[r + 1], 0, v.size() - 1, k);
    }
    
    lng maxLeq(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);

        int cnt = _cntLeq(l, r, x);
        if (cnt == 0) {
            return -INF64;}

        return kthMin(l, r, cnt);
    }

    lng minGeq(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);

        int cnt = _cntLeq(l, r, x - 1);
        if (cnt == r - l + 1) {
            return INF64;}

        return kthMin(l, r, cnt + 1);
    }
};

// S: O(n * log(n)), U: N/A, Q: O(log(n)), M: O(n * log(n)) 
struct WaveletMatrix {
    using uint = unsigned int;
    using ulng = unsigned long long;
        
    struct BitVec {
        int n;
        vector<ulng> blks;
        vector<int> prf;

        BitVec() {}
        
        BitVec(int N) : n(N) {
            blks.assign(((n + 63) >> 6) + 1, 0);
            prf.assign(blks.size(), 0);
        }

        void build() {
            for (int i = 0; i < int(blks.size()) - 1; i++) {
                prf[i + 1] = prf[i] + __builtin_popcountll(blks[i]);}
        }

        void set(int i) {
            blks[i >> 6] |= (1ULL << (i & 63));
        }

        int rank0(int i) {
            return i - rank1(i);
        }

        int rank1(int i) {
            return prf[i >> 6] + __builtin_popcountll(blks[i >> 6] & ((1ULL << (i & 63)) - 1));
        }
    };

    int n, h;
    vector<BitVec> bv;
    vector<int> md;
    vector<lng> v; 

    WaveletMatrix(const vector<lng> &a) : n(a.size()), v(a) {
        sort(v.begin(), v.end());
        v.erase(unique(v.begin(), v.end()), v.end());

        h = v.size() <= 1 ? 1 : 32 - __builtin_clz(uint(v.size() - 1));
        bv.assign(h, BitVec(n));
        md.assign(h, 0);

        vector<int> crnk(n), nxt0(n), nxt1(n);
        for (int i = 0; i < n; i++) {
            crnk[i] = lower_bound(v.begin(), v.end(), a[i]) - v.begin();}
        for (int d = h - 1; d >= 0; d--) {
            int p0 = 0, p1 = 0;
            for (int i = 0; i < n; i++) {
                if ((crnk[i] >> d) & 1) {
                    bv[d].set(i);
                    nxt1[p1] = crnk[i];
                    p1++;
                } else {
                    nxt0[p0] = crnk[i];
                    p0++;
                }
            }
            bv[d].build();
            md[d] = p0;

            for (int i = 0; i < p0; i++) {
                crnk[i] = nxt0[i];}
            for (int i = 0; i < p1; i++) {
                crnk[p0 + i] = nxt1[i];}
        }
    }

    int _cntLeqOrd(int l, int r, int ord) {
        if (ord < 0) {
            return 0;}

        if (ord >= int(v.size())) {
            return r - l + 1;}

        int res = 0;
        for (int d = h - 1; d >= 0; d--) {
            if (l > r) {
                break;}
            
            int l_rnk0 = bv[d].rank0(l);
            int r_rnk0 = bv[d].rank0(r + 1);
            int cnt0 = r_rnk0 - l_rnk0;

            if ((ord >> d) & 1) {
                res += cnt0;
                l = md[d] + (l - l_rnk0);
                r = md[d] + (r + 1 - r_rnk0) - 1;
            } else {
                l = l_rnk0;
                r = r_rnk0 - 1;
            }
        }
        
        return res + (r - l + 1);
    }

    int _cntLeq(int l, int r, lng x) {
        auto it = upper_bound(v.begin(), v.end(), x);
        if (it == v.begin()) {
            return 0;}
        
        int ord = (it - v.begin()) - 1;
        
        return _cntLeqOrd(l, r, ord);
    }
    
    int cntEql(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);

        return _cntLeq(l, r, x) - _cntLeq(l, r, x - 1);
    }

    int cntRan(int l, int r, lng x1, lng x2) {
        assert(0 <= l && l <= r && r < n);

        if (x1 > x2) {
            return 0;}

        return _cntLeq(l, r, x2) - _cntLeq(l, r, x1 - 1);
    }

    lng kthMin(int l, int r, int k) {
        assert(0 <= l && l <= r && r < n);
        assert(1 <= k && k <= r - l + 1);

        int ord = 0;
        for (int d = h - 1; d >= 0; d--) {
            int l_rnk0 = bv[d].rank0(l);
            int r_rnk0 = bv[d].rank0(r + 1);
            int cnt0 = r_rnk0 - l_rnk0;

            if (k <= cnt0) {
                l = l_rnk0;
                r = r_rnk0 - 1;
            } else {
                ord |= (1 << d);
                l = md[d] + (l - l_rnk0);
                r = md[d] + (r + 1 - r_rnk0) - 1;
                k -= cnt0;
            }
        }

        return v[ord];
    }

    lng maxLeq(int l, int r, lng x) {
        int cnt = _cntLeq(l, r, x);
        if (cnt == 0) {
            return -INF64;}

        return kthMin(l, r, cnt);
    }

    lng minGeq(int l, int r, lng x) {
        int cnt = _cntLeq(l, r, x - 1);
        if (cnt == r - l + 1) {
            return INF64;}
        
        return kthMin(l, r, cnt + 1);
    }
};

// 2.2.5 Cartesian Tree (Treap)



// S: NA, U: NA, Q: O(n * log(n)), M: O(1)
template<typename T>
struct FastConv {
    static void fctOr(vector<T> &a, bool is_inv) {
        int n = a.size();
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    if (is_inv) {
                        a[i + len + j] -= a[i + j];
                    } else {
                        a[i + len + j] += a[i + j];}
                }
            }
        }
    }

    static void fctAnd(vector<T> &a, bool is_inv) {
        int n = a.size();
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    if (is_inv) {
                        a[i + j] -= a[i + len + j];
                    } else {
                        a[i + j] += a[i + len + j];}
                }
            }
        }
    }

    static void fctXor(vector<T> &a, bool is_inv) {
        int n = a.size();
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    T u = a[i + j], v = a[i + len + j];
                    a[i + j] = u + v;
                    a[i + len + j] = u - v;
                }
            }
        }
        if (is_inv) {
            T f_inv_n = T(1) / T(n);
            for (int i = 0; i < n; i++) {
                a[i] *= f_inv_n;}
        }
    }

    static void fctGcdSlow(vector<T> &a, bool is_inv) {
        int n = a.size() - 1;
        if (is_inv) {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[i] -= a[j];}}
        } else {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[i] += a[j];}}
        }
    }

    // Q: O(n * loglog(n))
    //*
    static void fctGcd(vector<T> &a, bool is_inv, const LinearSieve &sv) {
        int n = a.size() - 1;
        for (int p : sv.prms) {
            if (p > n) {
                break;}

            if (is_inv) {
                for (int i = 1; i * p <= n; i++) {
                    a[i] -= a[i * p];}
            } else {
                for (int i = n / p; i >= 1; i--) {
                    a[i] += a[i * p];}
            }
        }
    }//*/

    static void fctLcmSlow(vector<T> &a, bool is_inv) {
        int n = a.size() - 1;
        if (is_inv) {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[j] -= a[i];}}
        } else {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[j] += a[i];}}
        }
    }

    // Q: O(n * loglog(n))
    //*
    static void fctLcm(vector<T> &a, bool is_inv, const LinearSieve &sv) {
        int n = a.size() - 1;
        for (int p : sv.prms) {
            if (p > n) {
                break;}

            if (is_inv) {
                for (int i = n / p; i >= 1; i--) {
                    a[i * p] -= a[i];}
            } else {
                for (int i = 1; i * p <= n; i++) {
                    a[i * p] += a[i];}
            }
        }
    }//*/
};

// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n * log(n))
struct LiChaoTree {
    struct Line {
        lng m, b;

        lng eval(lng x) const {
            return m * x + b;
        }
    };

    struct Node {
        Line mx_l, mn_l;
        int lc = -1, rc = -1;

        Node(Line Mx_l, Line Mn_l) : mx_l(Mx_l), mn_l(Mn_l) {}
    };

    lng mn_x, mx_x;
    int root = -1;
    vector<Node> tree;

    LiChaoTree(lng Mn_x, lng Mx_x) : mn_x(Mn_x), mx_x(Mx_x) {
        tree.reserve(2'500'000);
    }

    int createNode() {
        tree.emplace_back(Line{0, -INF64}, Line{0, INF64});
        
        return tree.size() - 1;
    }

    int _addLineMax(int i, lng l, lng r, Line y) {
        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  > tree[i].mx_l.eval(l);
        bool m_bet = y.eval(md) > tree[i].mx_l.eval(md);
        if (m_bet) {
            std::swap(tree[i].mx_l, y);}

        if (l == r) {
            return i;}

        if (l_bet != m_bet) {
            int nlc = _addLineMax(tree[i].lc, l,      md, y);
            tree[i].lc = nlc;
        } else {
            int nrc = _addLineMax(tree[i].rc, md + 1, r,  y);
            tree[i].rc = nrc;
        }

        return i;
    }

    int _addLineMin(int i, lng l, lng r, Line y) {
        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  < tree[i].mn_l.eval(l);
        bool m_bet = y.eval(md) < tree[i].mn_l.eval(md);
        if (m_bet) {
            std::swap(tree[i].mn_l, y);}

        if (l == r) {
            return i;}

        if (l_bet != m_bet) {
            int nlc = _addLineMin(tree[i].lc, l,      md, y);
            tree[i].lc = nlc;
        } else {
            int nrc = _addLineMin(tree[i].rc, md + 1, r,  y);
            tree[i].rc = nrc;
        }

        return i;
    }

    int _addSegmMax(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMax(i, l, r, y);}

        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMax(tree[i].lc, l,      md, ql, qr, y);
            tree[i].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMax(tree[i].rc, md + 1, r,  ql, qr, y);
            tree[i].rc = nrc;
        }

        return i;
    }

    int _addSegmMin(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMin(i, l, r, y);}

        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMin(tree[i].lc, l,      md, ql, qr, y);
            tree[i].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMin(tree[i].rc, md + 1, r,  ql, qr, y);
            tree[i].rc = nrc;
        }

        return i;
    }

    void addLine(lng m, lng b) {
        root = _addLineMax(root, mn_x, mx_x, {m, b});
        root = _addLineMin(root, mn_x, mx_x, {m, b});
    }

    // U: O(log(n)^2)
    void addSegm(lng l, lng r, lng m, lng b) {
        if (l >= mn_x && l <= r && r <= mx_x) {
            root = _addSegmMax(root, mn_x, mx_x, l, r, {m, b});
            root = _addSegmMin(root, mn_x, mx_x, l, r, {m, b});
        }
    }

    lng _maxQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return -INF64;}

        lng res = tree[i].mx_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = max(res, _maxQuery(tree[i].lc, l,      md, x));
        } else {
            res = max(res, _maxQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng _minQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return INF64;}

        lng res = tree[i].mn_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = min(res, _minQuery(tree[i].lc, l,      md, x));
        } else {
            res = min(res, _minQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng maxQuery(lng x) {
        if (x < mn_x || x > mx_x) {
            return -INF64;}
            
        return _maxQuery(root, mn_x, mx_x, x);
    }

    lng minQuery(lng x) {
        if (x < mn_x || x > mx_x) {
            return INF64;}
            
        return _minQuery(root, mn_x, mx_x, x);
    }
};

// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
struct HLD {
    int n, tm = 0;
    vector<int> dep, par, siz, head, in, out, rev;
    vector<vector<int>> adj;

    HLD(int N) : n(N), dep(N), par(N, -1), siz(N), head(N), in(N), out(N), rev(N), adj(N) {}

    void addEdge(int u, int v) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    void build(int root = 0) {
        tm = 0;
        dfsSiz(root, -1, 0);
        dfsHld(root, -1, root);
    }

    void dfsSiz(int u, int p, int d) {
        dep[u] = d;
        par[u] = p;
        siz[u] = 1;
        
        if (adj[u].size() > 1 && adj[u][0] == p) {
            std::swap(adj[u][0], adj[u][1]);}
            
        for (int &v : adj[u]) {
            if (v != p) {
                dfsSiz(v, u, d + 1);

                siz[u] += siz[v];
                if (siz[v] > siz[adj[u][0]]) {
                    std::swap(v, adj[u][0]);}
            }
        }
    }

    void dfsHld(int u, int p, int h) {
        head[u] = h;
        in[u] = tm++;
        rev[in[u]] = u;
        
        for (int v : adj[u]) {
            if (v != p) {
                dfsHld(v, u, (v == adj[u][0] ? h : v));}}
        
        out[u] = tm - 1;
    }

    int getDis(int u, int v) {
        return dep[u] + dep[v] - 2 * dep[getLCA(u, v)];
    }

    int getLCA(int u, int v) {
        while (head[u] != head[v]) {
            if (dep[head[u]] < dep[head[v]]) {
                std::swap(u, v);}
            u = par[head[u]];
        }
        
        return dep[u] < dep[v] ? u : v;
    }

    int kthAnc(int u, int k) {
        if (dep[u] < k) {
            return -1;}
        
        while (dep[u] - dep[head[u]] < k) {
            k -= dep[u] - dep[head[u]] + 1;
            u = par[head[u]];
        }
        
        return rev[in[u] - k];
    }

    // U: O(log(n)^2), Q: O(log(n)^2)
    template<typename F>
    void processPath(int u, int v, bool is_edge, F f) {
        while (head[u] != head[v]) {
            if (dep[head[u]] < dep[head[v]]) {
                std::swap(u, v);}
            
            f(in[head[u]], in[u]);
            u = par[head[u]];
        }
        
        if (dep[u] > dep[v]) {
            std::swap(u, v);}
            
        if (u == v && is_edge) {
            return;}
            
        f(in[u] + is_edge, in[v]);
    }

    template<typename F>
    void processSubt(int u, bool is_edge, F f) {
        if (in[u] == out[u] && is_edge) {
            return;}
            
        f(in[u] + is_edge, out[u]);
    }

    template<typename T>
    vector<T> reorderArray(const vector<T> &a) {
        vector<T> res(n);
        for (int i = 0; i < n; i++) {
            res[in[i]] = a[i];}
            
        return res;
    }
};

// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n * log(n))
struct PerLiChaoTree {
    struct Line {
        lng m, b;

        lng eval(lng x) const {
            return m * x + b;
        }
    };

    struct Node {
        Line mx_l, mn_l;
        int lc = -1, rc = -1;

        Node(Line Mx_l, Line Mn_l) : mx_l(Mx_l), mn_l(Mn_l) {}
    };

    lng mn_x, mx_x;
    vector<int> roots;
    vector<Node> tree;

    PerLiChaoTree(lng Mn_x, lng Mx_x) : mn_x(Mn_x), mx_x(Mx_x) {
        tree.reserve(10'000'000);
        roots.push_back(-1);
    }

    int cloneNode(int i) {
        if (i == -1) {
            tree.emplace_back(Line{0, -INF64}, Line{0, INF64});
        } else {
            Node tmp = tree[i];
            tree.push_back(tmp);
        }
        
        return tree.size() - 1;
    }

    int _addLineMax(int i, lng l, lng r, Line y) {
        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  > tree[j].mx_l.eval(l);
        bool m_bet = y.eval(md) > tree[j].mx_l.eval(md);
        if (m_bet) {
            std::swap(tree[j].mx_l, y);}

        if (l == r) {
            return j;}

        if (l_bet != m_bet) {
            int nlc = _addLineMax(tree[j].lc, l,      md, y);
            tree[j].lc = nlc;
        } else {
            int nrc = _addLineMax(tree[j].rc, md + 1, r,  y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int _addLineMin(int i, lng l, lng r, Line y) {
        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  < tree[j].mn_l.eval(l);
        bool m_bet = y.eval(md) < tree[j].mn_l.eval(md);
        if (m_bet) {
            std::swap(tree[j].mn_l, y);}

        if (l == r) {
            return j;}

        if (l_bet != m_bet) {
            int nlc = _addLineMin(tree[j].lc, l,      md, y);
            tree[j].lc = nlc;
        } else {
            int nrc = _addLineMin(tree[j].rc, md + 1, r,  y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int _addSegmMax(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMax(i, l, r, y);}

        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMax(tree[j].lc, l,      md, ql, qr, y);
            tree[j].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMax(tree[j].rc, md + 1, r,  ql, qr, y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int _addSegmMin(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMin(i, l, r, y);}

        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMin(tree[j].lc, l,      md, ql, qr, y);
            tree[j].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMin(tree[j].rc, md + 1, r,  ql, qr, y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int addLine(int ver, lng m, lng b) {
        int j = roots[ver];
        j = _addLineMax(j, mn_x, mx_x, {m, b});
        j = _addLineMin(j, mn_x, mx_x, {m, b});
        roots.push_back(j);
        
        return roots.size() - 1;
    }

    // U: O(log(n)^2)
    int addSegm(int ver, lng l, lng r, lng m, lng b) {
        int j = roots[ver];
        if (l >= mn_x && l <= r && r <= mx_x) {
            j = _addSegmMax(j, mn_x, mx_x, l, r, {m, b});
            j = _addSegmMin(j, mn_x, mx_x, l, r, {m, b});
        }
        roots.push_back(j);
        
        return roots.size() - 1;
    }

    lng _maxQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return -INF64;}

        lng res = tree[i].mx_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = max(res, _maxQuery(tree[i].lc, l,      md, x));
        } else {
            res = max(res, _maxQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng _minQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return INF64;}

        lng res = tree[i].mn_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = min(res, _minQuery(tree[i].lc, l,      md, x));
        } else {
            res = min(res, _minQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng maxQuery(int ver, lng x) {
        if (x < mn_x || x > mx_x) {
            return -INF64;}
            
        return _maxQuery(roots[ver], mn_x, mx_x, x);
    }

    lng minQuery(int ver, lng x) {
        if (x < mn_x || x > mx_x) {
            return INF64;}
            
        return _minQuery(roots[ver], mn_x, mx_x, x);
    }
};



// INFERIOR VERSIONS:
template<typename T> 
lng kadane(const vector<T> &arr) {
    if (arr.empty()) {
        return 0;
    }

    lng max_sum = -INF64, cur_sum = -INF64;
    for (T a : arr) {
        cur_sum = max(cur_sum + a, lng(a));
        max_sum = max(max_sum, cur_sum);
    }

    return max_sum;
}

struct BridgeAlgo {
    int n, timer = 0;
    vector<bool> vst;
    vector<int> t_in, low;
    set<pair<int, int>> bridges;

    BridgeAlgo(int n, const vector<vector<int>> &adjl): n(n) {
        vst.resize(n);
        t_in.resize(n, -1);
        low.resize(n, -1);

        for (int i = 0; i < n; i++) {
            if (!vst[i]) {
                dfs(i, i, adjl);
            }
        }        
    }

    void dfs(int cur, int prv, const vector<vector<int>> &adjl) {
        vst[cur] = true;
        t_in[cur] = timer;
        low[cur] = timer;
        timer++;
        
        bool prv_skipped = false;
        for (int nxt : adjl[cur]) {
            if (nxt == prv && !prv_skipped) {
                prv_skipped = true;
                continue;
            }

            if (vst[nxt]) {
                low[cur] = min(low[cur], t_in[nxt]);
            } else {
                dfs(nxt, cur, adjl);

                low[cur] = min(low[cur], low[nxt]);
                if (low[nxt] > t_in[cur]) {
                    bridges.insert({min(cur, nxt), max(cur, nxt)});
                }
            }
        }
    }
};

struct Dijkstra {
    int n;
    vector<bool> is_proc;
    vector<lng> dis;
    priority_queue<pair<lng, int>, vector<pair<lng, int>>, greater<pair<lng, int>>> unproc;

    Dijkstra(int n): n(n) {
        is_proc.resize(n + 1);
        dis.resize(n + 1, INF64);
    }

    void runDijkstra(const vector<int> &src, const vector<vector<pair<int, lng>>> &adjl) {
        fill(is_proc.begin(), is_proc.end(), false);
        fill(dis.begin(), dis.end(), INF64);
        priority_queue<pair<lng, int>, vector<pair<lng, int>>, greater<pair<lng, int>>> empty;
        unproc.swap(empty);
        
        for (int u : src) {
            dis[u] = 0;
            unproc.push({dis[u], u});
        }
        while (!unproc.empty()) {
            int cur = unproc.top().se;
            unproc.pop();

            if (is_proc[cur]) {
                continue;
            }
            is_proc[cur] = true;

            for (auto [nxt, w] : adjl[cur]) {
                if (dis[cur] + w < dis[nxt]) {
                    dis[nxt] = dis[cur] + w;
                    unproc.push({dis[nxt], nxt});
                }
            }
        };
    }
};

struct FloydWarshall {
    int n;
    vector<vector<lng>> dis;
    
    FloydWarshall(int n, vector<vector<lng>> dis) : n(n), dis(dis) {
        assert(dis.size() == n + 1);
        for (int i = 0; i <= n; i++) {
            assert(dis[i].size() == n + 1);
        }
    }
    
    void runFloydWarshall() {
        for (int k = 1; k <= n; k++) {
            for (int i = 1; i <= n; i++) {
                for (int j = 1; j <= n; j++) {
                    if (dis[i][k] < INF64 && dis[k][j] < INF64) {
                        dis[i][j] = max(min(dis[i][j], dis[i][k] + dis[k][j]), -INF64);
                    }
                }
            }
        }
    
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= n; j++) {
                for (int k = 1; k <= n; k++) {
                    if (dis[i][k] < INF64 && dis[k][k] < 0 && dis[k][j] < INF64) {
                        dis[i][j] = -INF64;
                        break;
                    }
                }
            }
        }
    }
};

struct LCA {
    int n, root, l, timer = 0;
    vector<int> dep, t_in, t_out;
    vector<vector<int>> up;

    LCA(int n, int root, const vector<vector<int>> &adjl): n(n), root(root) {
        l = ceil(log2(n));
        dep.resize(n + 1);
        t_in.resize(n + 1, -1);
        t_out.resize(n + 1, -1);
        up.resize(n + 1, vector<int>(l + 1, -1));

        init(root, root, 0, adjl);
    }

    void init(int cur, int prv, int d, const vector<vector<int>> &adjl) {
        dep[cur] = d;
        t_in[cur] = timer;
        timer++;
        up[cur][0] = prv;
        for (int i = 1; i <= l; i++) {
            up[cur][i] = up[up[cur][i - 1]][i - 1];
        }

        for (int nxt : adjl[cur]) {
            if (nxt == prv) {
                continue;
            }

            init(nxt, cur, d + 1, adjl);
        }

        t_out[cur] = timer - 1;
    }

    bool isAncestor(int u, int v) const {
        return t_in[u] <= t_in[v] && t_out[v] <= t_out[u];
    }

    int getKthAncestor(int u, int k) const {
        if (k <= 0) {
            return u;
        }
        
        if (k > dep[u]) {
            return -1;
        }
        
        for (int i = l; i >= 0; i--) {
            if (k & (1 << i)) {
                u = up[u][i];
            }
        }

        return u;
    }

    int getLCA(int u, int v) const {
        if (isAncestor(u, v)) {
            return u;
        }
        
        if (isAncestor(v, u)) {
            return v;
        }

        for (int i = l; i >= 0; i--) {
            if (!isAncestor(up[u][i], v)) {
                u = up[u][i];
            }
        }

        return up[u][0];
    }
};

// AI GENERATED FAST

// 2-SAT
// S: O(n + m), U: NA, Q: O(1), M: O(n + m)
struct TwoSat {
    int n;
    vector<vector<int>> adj, adj_t;
    vector<bool> used;
    vector<int> ord, comp;
    vector<bool> ans;

    TwoSat(int N) : n(N), adj(2 * N), adj_t(2 * N), ans(N) {}

    void addClause(int a, bool na, int b, bool nb) {
        int u = 2 * a ^ na;
        int v = 2 * b ^ nb;
        int neg_u = u ^ 1;
        int neg_v = v ^ 1;
        adj[neg_u].push_back(v);
        adj[neg_v].push_back(u);
        adj_t[v].push_back(neg_u);
        adj_t[u].push_back(neg_v);
    }

    bool solve() {
        used.assign(2 * n, false);
        ord.clear();
        
        auto dfs1 = [&](auto &&dfs1, int u) -> void {
            used[u] = true;
            for (int v : adj[u]) {
                if (!used[v]) {
                    dfs1(dfs1, v);}}
            ord.push_back(u);
        };

        for (int i = 0; i < 2 * n; i++) {
            if (!used[i]) {
                dfs1(dfs1, i);}}

        comp.assign(2 * n, -1);
        auto dfs2 = [&](auto &&dfs2, int u, int c) -> void {
            comp[u] = c;
            for (int v : adj_t[u]) {
                if (comp[v] == -1) {
                    dfs2(dfs2, v, c);}}
        };

        for (int i = 0, j = 0; i < 2 * n; i++) {
            int u = ord[2 * n - i - 1];
            if (comp[u] == -1) {
                dfs2(dfs2, u, j++);}}

        for (int i = 0; i < 2 * n; i += 2) {
            if (comp[i] == comp[i + 1]) {
                return false;}
            ans[i / 2] = comp[i] > comp[i + 1];
        }

        return true;
    }
};

// Floyd-Warshall
// T: O(n^3), M: O(1)
void floydWarshall(vector<vector<lng>> &d) {
    int n = sze(d);
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (d[i][k] < INF64 && d[k][j] < INF64) {
                    d[i][j] = min(d[i][j], d[i][k] + d[k][j]);}}}}
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                if (d[i][k] < INF64 && d[k][k] < 0 && d[k][j] < INF64) {
                    d[i][j] = -INF64;
                    break;
                }
            }
        }
    }
}

// SPFA
// S: O(n * m), U: NA, Q: O(1), M: O(n + m)
struct SPFA {
    int n;
    vector<lng> d;
    vector<int> p;

    SPFA(int N) : n(N) {}

    bool solve(int s, const vector<vector<pair<int, lng>>> &adj) {
        d.assign(n, INF64);
        p.assign(n, -1);
        vector<int> cnt(n, 0);
        vector<bool> in_q(n, false);
        queue<int> q;

        d[s] = 0;
        q.push(s);
        in_q[s] = true;

        while (!q.empty()) {
            int u = q.front();
            q.pop();
            in_q[u] = false;

            for (auto [v, w] : adj[u]) {
                if (d[u] + w < d[v]) {
                    d[v] = d[u] + w;
                    p[v] = u;
                    if (!in_q[v]) {
                        q.push(v);
                        in_q[v] = true;
                        cnt[v]++;
                        if (cnt[v] > n) {
                            return false;}
                    }
                }
            }
        }

        return true;
    }
};

// Check Bipartite
// T: O(n + m), M: O(n)
bool isBipartite(int n, const vector<vector<int>> &adj, vector<int> &side) {
    side.assign(n, -1);
    bool is_bip = true;
    queue<int> q;

    for (int st = 0; st < n; st++) {
        if (side[st] == -1) {
            q.push(st);
            side[st] = 0;
            while (!q.empty()) {
                int u = q.front();
                q.pop();
                for (int v : adj[u]) {
                    if (side[v] == -1) {
                        side[v] = side[u] ^ 1;
                        q.push(v);
                    } else {
                        is_bip &= side[v] != side[u];
                    }
                }
            }
        }
    }

    return is_bip;
}

// S: O(n^3), U: NA, Q: O(1), M: O(n^2)
template<typename T>
struct Hungarian {
    int n, m;
    vector<vector<T>> a;
    vector<T> u, v;
    vector<int> p, way;

    Hungarian(int n, int m) : n(n), m(m), a(n + 1, vector<T>(m + 1, 0)), 
        u(n + 1, 0), v(m + 1, 0), p(m + 1, 0), way(m + 1, 0) {}

    void addEdge(int i, int j, T w) {
        a[i + 1][j + 1] = w;}

    T solve() {
        for (int i = 1; i <= n; i++) {
            p[0] = i;
            int j0 = 0;
            vector<T> minv(m + 1, INF64);
            vector<bool> used(m + 1, false);
            do {
                used[j0] = true;
                int i0 = p[j0], j1 = 0;
                T delta = INF64;
                for (int j = 1; j <= m; j++) {
                    if (!used[j]) {
                        T cur = a[i0][j] - u[i0] - v[j];
                        if (cur < minv[j]) {
                            minv[j] = cur;
                            way[j] = j0;
                        }
                        if (minv[j] < delta) {
                            delta = minv[j];
                            j1 = j;
                        }
                    }
                }
                for (int j = 0; j <= m; j++) {
                    if (used[j]) {
                        u[p[j]] += delta;
                        v[j] -= delta;
                    } else {
                        minv[j] -= delta;
                    }
                }
                j0 = j1;
            } while (p[j0] != 0);
            do {
                int j1 = way[j0];
                p[j0] = p[j1];
                j0 = j1;
            } while (j0 != 0);
        }

        return -v[0];
    }
    
    vector<int> getAssignment() {
        vector<int> ans(n, -1);
        for (int j = 1; j <= m; j++) {
            if (p[j] != 0) {
                ans[p[j] - 1] = j - 1;}}
        
        return ans;
    }
};

// S: O(n + m), U: NA, Q: O(1), M: O(n)
struct Tarjan {
    int n, timer;
    vector<bool> vst, is_art;
    vector<int> t_in, low;
    vector<pair<int, int>> bridges;

    Tarjan(int N, const vector<vector<int>> &adj) : n(N), timer(0), 
        vst(N, false), is_art(N, false), t_in(N, -1), low(N, -1) {
        auto dfs = [&](auto &&dfs, int u, int p) -> void {
            vst[u] = true;
            t_in[u] = low[u] = timer++;
            int children = 0;
            bool p_skip = false;

            for (int v : adj[u]) {
                if (v == p && !p_skip) {
                    p_skip = true;
                    continue;
                }
                if (vst[v]) {
                    low[u] = min(low[u], t_in[v]);
                } else {
                    dfs(dfs, v, u);
                    low[u] = min(low[u], low[v]);
                    if (low[v] >= t_in[u] && p != -1) {
                        is_art[u] = true;}
                    if (low[v] > t_in[u]) {
                        bridges.push_back({min(u, v), max(u, v)});}
                    children++;
                }
            }
            if (p == -1 && children > 1) {
                is_art[u] = true;}
        };

        for (int i = 0; i < n; i++) {
            if (!vst[i]) {
                dfs(dfs, i, -1);}}
    }
};

// S: O(n), U: O(log(n)), Q: O(1), M: O(n)
struct OnlineBridges {
    int bridges, lca_itr;
    vector<int> par, d_2ec, d_cc, sz_cc, last_vst;

    OnlineBridges(int n) : bridges(0), lca_itr(0), par(n, -1), d_2ec(n), 
        d_cc(n), sz_cc(n, 1), last_vst(n, 0) {
        iota(d_2ec.begin(), d_2ec.end(), 0);
        iota(d_cc.begin(), d_cc.end(), 0);
    }

    int find2ec(int v) {
        if (v == -1) {
            return -1;}
            
        return d_2ec[v] == v ? v : d_2ec[v] = find2ec(d_2ec[v]);
    }

    int findCC(int v) {
        v = find2ec(v);
        
        return d_cc[v] == v ? v : d_cc[v] = findCC(d_cc[v]);
    }

    void makeRoot(int v) {
        int root = v, child = -1;
        while (v != -1) {
            int p = find2ec(par[v]);
            par[v] = child;
            d_cc[v] = root;
            child = v;
            v = p;
        }
        sz_cc[root] = sz_cc[child];
    }

    void mergePath(int a, int b) {
        lca_itr++;
        vector<int> path_a, path_b;
        int lca = -1;
        while (lca == -1) {
            if (a != -1) {
                a = find2ec(a);
                path_a.push_back(a);
                if (last_vst[a] == lca_itr) {
                    lca = a;
                    break;
                }
                last_vst[a] = lca_itr;
                a = par[a];
            }
            if (b != -1) {
                b = find2ec(b);
                path_b.push_back(b);
                if (last_vst[b] == lca_itr) {
                    lca = b;
                    break;
                }
                last_vst[b] = lca_itr;
                b = par[b];
            }
        }
        for (int v : path_a) {
            d_2ec[v] = lca;
            if (v == lca) {
                break;}
            bridges--;
        }
        for (int v : path_b) {
            d_2ec[v] = lca;
            if (v == lca) {
                break;}
            bridges--;
        }
    }

    void addEdge(int a, int b) {
        a = find2ec(a);
        b = find2ec(b);
        if (a == b) {
            return;}

        int ca = findCC(a);
        int cb = findCC(b);
        if (ca != cb) {
            bridges++;
            if (sz_cc[ca] > sz_cc[cb]) {
                swap(a, b);
                swap(ca, cb);
            }
            makeRoot(a);
            par[a] = d_cc[a] = b;
            sz_cc[cb] += sz_cc[a];
        } else {
            mergePath(a, b);
        }
    }
};

// S: O(n * log(n)), U: NA, Q: O(1), M: O(n)
struct CentroidDecomposition {
    int n;
    vector<int> sz, par;
    vector<bool> rmv;

    CentroidDecomposition(int N, const vector<vector<int>> &adj) : 
        n(N), sz(N, 0), par(N, -1), rmv(N, false) {
        auto getSz = [&](auto &&getSz, int u, int p) -> int {
            sz[u] = 1;
            for (int v : adj[u]) {
                if (v != p && !rmv[v]) {
                    sz[u] += getSz(getSz, v, u);}}
                    
            return sz[u];
        };

        auto getCent = [&](auto &&getCent, int u, int p, int t_sz) -> int {
            for (int v : adj[u]) {
                if (v != p && !rmv[v] && sz[v] * 2 > t_sz) {
                    return getCent(getCent, v, u, t_sz);}}
                    
            return u;
        };

        auto build = [&](auto &&build, int u, int p) -> void {
            int t_sz = getSz(getSz, u, -1);
            int cent = getCent(getCent, u, -1, t_sz);
            
            par[cent] = p;
            rmv[cent] = true;
            
            for (int v : adj[cent]) {
                if (!rmv[v]) {
                    build(build, v, cent);}}
        };

        build(build, 0, -1);
    }
};


int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    
    return 0;
}