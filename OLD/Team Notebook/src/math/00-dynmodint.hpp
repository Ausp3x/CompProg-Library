#pragma once
#include "../1-Core/01-template.hpp"

// T: O(1) or O(log(n)), M: O(1)
template<int ID = 0>
struct DynModInt {
    static inline int MOD = 998'244'353;
    static inline ulng IMOD = -1ULL / 998'244'353 + 1;
    static inline bool is_prime = true;
    // T: O(n^(1/2))
    static bool isPrime() {
        if (MOD <= 1) { return false; }
        if (MOD == 2 || MOD == 3) { return true; }
        if (MOD % 2 == 0 || MOD % 3 == 0) { return false; }
        for (lng i = 5; i * i <= MOD; i += 6) {
            if (MOD % i == 0 || MOD % (i + 2) == 0) { return false; }}
        return true;}
    // T: O(n^(1/2))
    static void setMod(int mod) {
        assert(mod > 0);
        MOD = mod; IMOD = -1ULL / mod + 1; is_prime = isPrime();}
    static uint red(ulng a) {
        ulng q = ulng((ulll(a) * IMOD) >> 64);
        ulng r = a - q * MOD;
        if (MOD <= r) { r += MOD; }
        return uint(r);}

    int n;
    template<typename T = int> 
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    DynModInt(T N = 0) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            if (N >= 0) { n = red(N); } 
            else { n = red(-ulng(N)); n = (n != 0) * (MOD - n); }} 
        else { n = red(N); }}
    
    explicit operator int() const { return n; }
    static DynModInt init(int N) { DynModInt res; res.n = N; return res; }
    template<typename T> 
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    DynModInt &operator=(T N) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) {
            if (N >= 0) { n = red(N); } 
            else { n = red(-ulng(N)); n = (n != 0) * (MOD - n); }} 
        else { n = red(N); }
        return *this;}
        
    DynModInt &operator++() { n++; n -= (n == MOD) * MOD; return *this; }
    DynModInt &operator--() { n += (n == 0) * MOD; n--; return *this; }
    DynModInt operator++(int) { DynModInt res = *this; ++*this; return res; }
    DynModInt operator--(int) { DynModInt res = *this; --*this; return res; }
    DynModInt &operator+=(DynModInt o) { n += o.n; n -= (n >= MOD) * MOD; return *this; }
    DynModInt &operator-=(DynModInt o) { n -= o.n; n += (n < 0) * MOD; return *this; }
    DynModInt &operator*=(DynModInt o) { n = red(1ULL * n * o.n); return *this; }
    DynModInt &operator/=(DynModInt o) { return *this *= inv(o); }

    DynModInt operator+() const { return *this; }
    DynModInt operator-() const { return init((n != 0) * (MOD - n)); }
    friend DynModInt operator+(DynModInt a, DynModInt b) { a += b; return a; }
    friend DynModInt operator-(DynModInt a, DynModInt b) { a -= b; return a; }
    friend DynModInt operator*(DynModInt a, DynModInt b) { a *= b; return a; }
    friend DynModInt operator/(DynModInt a, DynModInt b) { a /= b; return a; }
    
    // T: O(log(n))
    friend DynModInt inv(DynModInt a) {
        assert(a != 0);
        int n = a.n, b = MOD, x = 1, y = 0;
        if (is_prime) { return pow(a, MOD - 2); }
        while (b > 0) {
            int q = n / b;
            n = std::exchange(b, n - q * b);
            x = std::exchange(y, x - q * y);}
        return init(x + (x < 0) * MOD);}
    // T: O(log(n))
    friend DynModInt pow(DynModInt a, lng b) {
        ulng ub = b < 0 ? -ulng(b) : ulng(b);
        if (b < 0) { a = inv(a); }
        DynModInt res = 1;
        for (; ub > 0; ub >>= 1, a *= a) { if (ub & 1) { res *= a; } }
        return res;}
    // T: O(log(n))
    friend DynModInt sqrt(DynModInt a) {
        assert(is_prime);
        if (a == 0 || MOD == 2) { return a; }
        if (pow(a, (MOD - 1) / 2) != 1) { return init(-1); }
        DynModInt b = 1;
        while (pow(b * b - a, (MOD - 1) / 2) == 1) { b++; }
        struct Node {
            DynModInt x, y, w;
            Node(DynModInt X = 0, DynModInt Y = 0, DynModInt W = 0) : x(X), y(Y), w(W) {}
            Node operator*(const Node &o) const { 
                return Node(x * o.x + y * o.y * w, x * o.y + y * o.x, w); }};
        Node res(1, 0, b * b - a), cur(b, 1, b * b - a);
        for (int p = (MOD + 1) / 2; p > 0; p >>= 1, cur = cur * cur) {
            if (p & 1) { res = res * cur; }}
        return init(min(res.x.n, MOD - res.x.n));}

    explicit operator bool() const { return n != 0; }
    bool operator!() const { return n == 0; }
    friend auto operator<=>(const DynModInt &a, const DynModInt &b) = default;
    
    friend istream &operator>>(istream &is, DynModInt &a) { lng b; is >> b; a = DynModInt(b); return is; }
    friend ostream &operator<<(ostream &os, DynModInt a) { return os << a.n; }
};