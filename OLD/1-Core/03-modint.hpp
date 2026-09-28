#pragma once
#include "../1-Core/01-template.hpp"

// T: O(1) or O(log(n)), M: O(1)
template<int MOD>
struct ModInt {
    static_assert(MOD > 0);
    static constexpr bool isPrime() {
        if (MOD <= 1) { return false; }
        if (MOD == 2 || MOD == 7 || MOD == 61) { return true; }
        if (!(MOD & 1)) { return false; }
        int s = __builtin_ctz(uint(MOD - 1)); lng d = lng(MOD - 1) >> s;
        for (lng a : {2, 7, 61}) {
            lng x = 1;
            for (lng b = d; b > 0; b >>= 1) {
                if (b & 1) { x = x * a % MOD; }
                a = a * a % MOD;}
            if (x == 1 || x == MOD - 1) { continue; }
            bool is_comp = true;
            for (int j = 1; j < s; j++) {
                x = x * x % MOD;
                if (x == MOD - 1) { is_comp = false; break; }
                if (x == 1) { return false; }}
            if (is_comp) { return false; }}
        return true;}
    static constexpr bool is_prime = ModInt::isPrime();

    int n;
    template<typename T = int>
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    constexpr ModInt(T N = 0) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) { n = N % MOD; n += (n < 0) * MOD; }
        else { n = N % MOD; }}
    
    explicit constexpr operator int() const { return n; }
    static constexpr int mod() { return MOD; }
    static constexpr ModInt init(int N) { ModInt res; res.n = N; return res; }
    template<typename T> 
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    constexpr ModInt &operator=(T N) {
        if constexpr (std::is_signed_v<T> || std::is_same_v<T, lll>) { n = N % MOD; n += (n < 0) * MOD; }
        else { n = N % MOD; }
        return *this;}
    
    constexpr ModInt &operator++() { n++; n -= (n == MOD) * MOD; return *this; }
    constexpr ModInt &operator--() { n += (n == 0) * MOD; n--; return *this; }
    constexpr ModInt operator++(int) { ModInt res = *this; ++*this; return res; }
    constexpr ModInt operator--(int) { ModInt res = *this; --*this; return res; }
    constexpr ModInt &operator+=(ModInt o) { n += o.n - (n >= MOD - o.n) * MOD; return *this; }
    constexpr ModInt &operator-=(ModInt o) { n -= o.n - (n < o.n) * MOD; return *this; }
    constexpr ModInt &operator*=(ModInt o) { n = ulng(n) * uint(o.n) % uint(MOD); return *this; }
    constexpr ModInt &operator/=(ModInt o) { return *this *= inv(o); }

    constexpr ModInt operator+() const { return *this; }
    constexpr ModInt operator-() const { return init((n != 0) * (MOD - n)); }
    friend constexpr ModInt operator+(ModInt a, ModInt b) { a += b; return a; }
    friend constexpr ModInt operator-(ModInt a, ModInt b) { a -= b; return a; }
    friend constexpr ModInt operator*(ModInt a, ModInt b) { a *= b; return a; }
    friend constexpr ModInt operator/(ModInt a, ModInt b) { a /= b; return a; }

    // T: O(log(n))
    friend constexpr ModInt inv(ModInt a) {
        assert(a != 0);
        if constexpr (is_prime) { return pow(a, MOD - 2); }
        int n = a.n, b = MOD, x = 1, y = 0;
        while (b > 0) {
            int q = n / b;
            n = std::exchange(b, n - q * b);
            x = std::exchange(y, x - q * y);}
        assert(n == 1);
        return init(x + (x < 0) * MOD);}
    // T: O(log(n))
    friend constexpr ModInt pow(ModInt a, lng b) {
        ulng ub = b < 0 ? -ulng(b) : ulng(b);
        if (b < 0) { a = inv(a); }
        ModInt res = 1;
        for (; ub > 0; ub >>= 1, a *= a) { if (ub & 1) { res *= a; } }
        return res;}
    // T: O(log(n)) average
    friend constexpr ModInt sqrt(ModInt a) {
        assert(is_prime);
        if (a == 0 || a == 1 || MOD == 2) { return a; }
        if constexpr (MOD % 4 == 3) {
            ModInt res = pow(a, (lng(MOD) + 1) / 4);
            if (res * res != a) { return -1; }
            return init(min(res.n, MOD - res.n));}
        if (pow(a, (MOD - 1) / 2) != 1) { return -1; }
        ModInt b = 1;
        while (pow(b * b - a, (MOD - 1) / 2) == 1) { b++; }
        struct Node {
            ModInt x, y, w;
            constexpr Node(ModInt X = 0, ModInt Y = 0, ModInt W = 0) : x(X), y(Y), w(W) {}
            constexpr Node operator*(const Node &o) const { 
                return Node(x * o.x + y * o.y * w, x * o.y + y * o.x, w); }};
        Node res(1, 0, b * b - a), cur(b, 1, b * b - a);
        for (lng p = (lng(MOD) + 1) / 2; p > 0; p >>= 1, cur = cur * cur) {
            if (p & 1) { res = res * cur; }}
        return init(min(res.x.n, MOD - res.x.n));}

    explicit constexpr operator bool() const { return n != 0; }
    constexpr bool operator!() const { return n == 0; }
    friend constexpr auto operator<=>(const ModInt &a, const ModInt &b) = default;

    friend istream &operator>>(istream &is, ModInt &a) { lng b; if (is >> b) { a = b; } return is; }
    friend ostream &operator<<(ostream &os, ModInt a) { return os << a.n; }
};
using mint = ModInt<998'244'353>;
