// T: O(1), M: O(1)
template<int MOD>
struct ModInt {
    static constexpr bool isPrime() {
        if (MOD <= 1) { return false; }
        if (MOD == 2 || MOD == 3) { return true; }
        if (MOD % 2 == 0 || MOD % 3 == 0) { return false; }
        for (lng i = 5; i * i <= MOD; i += 6) {
            if (MOD % i == 0 || MOD % (i + 2) == 0) {
                return false;}}
        return true;
    }
    static constexpr bool is_prime = ModInt::isPrime();

    int n;

    constexpr ModInt(lng N = 0) : n(N % MOD) { n += (n < 0) * MOD; }
    explicit constexpr operator int() const { return n; }
    static constexpr ModInt init(int N) { ModInt res; res.n = N; return res; }
    constexpr ModInt &operator=(lng N) { n = N % MOD; n += (n < 0) * MOD; return *this; }

    constexpr ModInt &operator++() { n++; n -= (n == MOD) * MOD; return *this; }
    constexpr ModInt &operator--() { n += (n == 0) * MOD; n--; return *this; }
    constexpr ModInt operator++(int) { ModInt res = *this; ++*this; return res; }
    constexpr ModInt operator--(int) { ModInt res = *this; --*this; return res; }
    constexpr ModInt &operator+=(ModInt o) { n += o.n; n -= (n >= MOD) * MOD; return *this; }
    constexpr ModInt &operator-=(ModInt o) { n -= o.n; n += (n < 0) * MOD; return *this; }
    constexpr ModInt &operator*=(ModInt o) { n = lng(n) * o.n % MOD; return *this; }
    constexpr ModInt &operator/=(ModInt o) { n = lng(n) * inv(o).n % MOD; return *this; }

    constexpr ModInt operator+() const { return *this; }
    constexpr ModInt operator-() const { return init((n != 0) * (MOD - n)); }
    friend constexpr ModInt operator+(ModInt a, ModInt b) { a += b; return a; }
    friend constexpr ModInt operator-(ModInt a, ModInt b) { a -= b; return a; }
    friend constexpr ModInt operator*(ModInt a, ModInt b) { a *= b; return a; }
    friend constexpr ModInt operator/(ModInt a, ModInt b) { a /= b; return a; }

    friend constexpr ModInt inv(ModInt a) {
        assert(a != 0);
        int n = a.n, b = MOD, x = 1, y = 0;
        while (b > 0) {
            int q = n / b;
            n = std::exchange(b, n - q * b);
            x = std::exchange(y, x - q * y);
        }
        return init(x + (x < 0) * MOD);
    }

    friend constexpr ModInt pow(ModInt a, lng b) {
        if (b < 0) { a = inv(a); b = -b; }
        ModInt res = 1;
        while (b > 0) {
            if (b & 1) { res *= a; }
            b >>= 1;
            if (b > 0) { a *= a; }
        }
        return res;
    }

    explicit constexpr operator bool() const { return n != 0; }
    constexpr bool operator!() const { return n == 0; }
    friend constexpr auto operator<=>(const ModInt&, const ModInt&) = default;

    friend istream &operator>>(istream &is, ModInt &a) { 
        lng b; is >> b; a = ModInt(b); 
        return is; 
    }
    friend ostream &operator<<(ostream &os, ModInt a) { return os << a.n; }
};
using mint = ModInt<998'244'353>;