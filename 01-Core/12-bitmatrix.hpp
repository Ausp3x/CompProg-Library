#pragma once
// existing-unverified; unchanged excerpt from OLD/algorithms.cpp:31-574.
// Legacy contracts and comments await audit; see this folder's 00-index.md.
#include "01-template.hpp"

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
