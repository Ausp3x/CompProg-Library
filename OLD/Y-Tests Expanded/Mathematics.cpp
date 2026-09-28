#line 2 "1-Core\\01-template.hpp"

// 知彼知己，百战不殆
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi    first
#define se    second
#define pb    push_back
using uint = uint32_t;
using lng = int64_t;    using ulng = uint64_t;
using lll = __int128_t; using ulll = __uint128_t;
template<typename T> 
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

constexpr int INF32 = 0x3f3f3f3f;
constexpr lng INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T> 
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T> 
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }
#line 3 "5-Mathematics\\02-exgcd.hpp"

// T: O(log(n)), M: O(1)
lng exGcd(lng a, lng b, lng &x, lng &y) {
    x = 1; y = 0; lng X = 0, Y = 1;
    while (b != 0) {
        lng q = a / b;
        x = std::exchange(X, x - q * X);
        y = std::exchange(Y, y - q * Y);
        a = std::exchange(b, a % b);}
    return a;}
#line 4 "5-Mathematics\\01-modpow.hpp"

// T: O(log(n)), M: O(1)  
lng modPow(lng a, lng b, lng mod = INF64)  {
    a = (a % mod + mod) % mod;
    if (b < 0) {
        lng x, y;
        if (exGcd(a, mod, x, y) != 1) { return -1; }
        a = (x % mod + mod) % mod; b = -b;}
    lng res = 1;
    while (b > 0) {
        if (b & 1) { res = lll(res) * a % mod; }
        a = lll(a) * a % mod; b >>= 1;}
    return res;}
#line 3 "5-Mathematics\\03-sieveoferath.hpp"

// S: O(n * loglog(n)), U: NA, Q: O(1), M: O(n)
struct SieveOfErath {
    int n; vector<char> is_prime;
    SieveOfErath(int N) : n(N) {
        is_prime.assign(n + 1, true);
        if (n >= 0) { is_prime[0] = false; }
        if (n >= 1) { is_prime[1] = false; }
        for (int i = 4; i <= n; i += 2) { is_prime[i] = false; }
        for (int i = 3; 1LL * i * i <= n; i += 2) {
            if (!is_prime[i]) { continue; }
            for (lng j = 1LL * i * i; j <= n; j += 2 * i) { is_prime[j] = false; }}}
};
#line 3 "5-Mathematics\\04-linearsieve.hpp"

// S: O(n), U: NA, Q: O(1), M: O(n)
struct LinearSieve {
    int n;
    vector<int> prms, spf, num_div, phi;
    vector<lng> sum_div; vector<signed char> mu;
    LinearSieve(int N) : n(N) {
        spf.assign(n + 1, -1);
        for (int i = 2; i <= n; i++) {
            if (spf[i] == -1) { prms.push_back(i); spf[i] = i; }
            for (int p : prms) {
                if (1LL * i * p > n) { break; }
                spf[i * p] = p;
                if (i % p == 0) { break; }}}}

    void getNumDiv() {
        num_div.assign(n + 1, 0); vector<int> spf_exp(n + 1, 0);
        if (n >= 1) { num_div[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                spf_exp[i] = spf_exp[j] + 1; 
                num_div[i] = num_div[j] / (spf_exp[j] + 1) * (spf_exp[i] + 1);} 
            else {
                spf_exp[i] = 1;
                num_div[i] = num_div[j] * 2;}}}
    void getSumDiv() {
        sum_div.assign(n + 1, 0); vector<lng> spf_exp_sum(n + 1, 0);
        if (n >= 1) { sum_div[1] = 1; spf_exp_sum[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                spf_exp_sum[i] = spf_exp_sum[j] * p + 1;
                sum_div[i] = sum_div[j] / spf_exp_sum[j] * spf_exp_sum[i];} 
            else {
                spf_exp_sum[i] = p + 1;
                sum_div[i] = sum_div[j] * (p + 1);}}}
    void getPhi() {
        phi.assign(n + 1, 0);
        if (n >= 1) { phi[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) { phi[i] = phi[j] * p; } 
            else { phi[i] = phi[j] * (p - 1); }}}
    void getMu() {
        mu.assign(n + 1, 0);
        if (n >= 1) { mu[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) { mu[i] = 0; } 
            else { mu[i] = -mu[j]; }}}
    // Q: O(log(n))
    vector<pair<int, int>> getPrimeFac(int a) {
        assert(a <= n);
        vector<pair<int, int>> res;
        while (a > 1) { 
            int p = spf[a], cnt = 0;
            while (spf[a] == p) { cnt++; a /= p; }
            res.push_back({p, cnt});}
        return res;}
};
#line 3 "5-Mathematics\\05-isprimemr.hpp"

// T: O(log(n)), M: O(1)
bool isPrimeMR(lng n) { // Miller-Rabin
    if (n < 2) { return false; }
    if (n == 2 || n == 3) { return true; }
    if (n % 2 == 0) { return false; }
    int s = __builtin_ctzll(n - 1); lng d = (n - 1) >> s;
    static constexpr lng bsm[] = {2LL, 7LL, 61LL};
    static constexpr lng blg[] = {2LL, 325LL, 9375LL, 28178LL, 450775LL, 9780504LL, 1795265022LL};
    const lng *B = n < 4759123141LL ? bsm : blg;
    for (int i = 0; i < (n < 4759123141LL ? 3 : 7); i++) {
        lng a = B[i];
        if (a % n == 0) { continue; }
        a %= n; lng b = d, x = 1;
        while (b > 0) {
            if (b & 1) { x = ulll(x) * a % n; }
            a = ulll(a) * a % n;
            b >>= 1;}
        if (x == 1 || x == n - 1) { continue; }
        bool is_comp = true;
        for (int j = 1; j < s; j++) {
            x = ulll(x) * x % n;
            if (x == n - 1) { is_comp = false; break; }
            if (x == 1) { return false; }}
        if (is_comp) { return false; }}
    return true;}
#line 3 "5-Mathematics\\06-getprimefacslow.hpp"

// T: O(n^(1/2)), M: O(log(n))
vector<lng> getPrimeFacSlow(lng n) {
    vector<lng> res;
    for (int p : {2, 3, 5}) { while (n % p == 0) { res.push_back(p); n /= p; } }
    int j = 0; static constexpr int inc[] = {4, 2, 4, 2, 4, 6, 2, 6};
    for (lng i = 7; i * i <= n; i += inc[j], j = (j + 1) & 7) {
        while (n % i == 0) { res.push_back(i); n /= i; }}
    if (n > 1) { res.push_back(n); }
    sort(res.begin(), res.end());
    return res;}
#line 3 "5-Mathematics\\07-getonefacbpr.hpp"

// T: O(n^(1/4)), M: O(1)
lng getOneFacBPR(lng n, lng x0 = 2, lng c = 1) { // Brent's Pollard's Rho
    auto f = [](lng x, lng c, lng mod) -> lng { 
        return (ulll(x) * x + c) % mod;};
    int m = 128, l = 1;
    lng x = x0, xs = -1, y = -1, g = 1, q = 1;
    while (g == 1) {
        y = x; int k = 0;
        while (g == 1 && k < l) {
            xs = x;
            for (int i = 0; i < min(m, l - k); i++) {
                x = f(x, c, n);
                q = ulll(q) * (x > y ? x - y : y - x) % n;}
            g = gcd(q, n); k += m;}
        l *= 2;}
    if (g == n) {
        do {
            xs = f(xs, c, n);
            g = gcd(xs > y ? xs - y : y - xs, n);} 
        while (g == 1);}
    return g;}
#line 3 "6-Miscellaneous\\random.hpp"

// S: O(1), U: NA, Q: O(1), M: O(1)
struct Random {
    std::mt19937_64 rng; 
    Random() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    int randInt(int l, int r) { return std::uniform_int_distribution<int>(l, r)(rng); }
    lng randLng(lng l, lng r) { return std::uniform_int_distribution<lng>(l, r)(rng); }
};
#line 6 "5-Mathematics\\08-getprimefacfast.hpp"

// T: O(n^(1/4)), M: O(log(n))
Random rnd;
void getPrimeFacFast(lng n, vector<lng> &res) { 
    if (n <= 1) { return; }
    if (isPrimeMR(n)) { res.push_back(n); return; } 
    static constexpr int prms[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    for (int p : prms) {
        if (n % p == 0) {
            res.push_back(p);
            getPrimeFacFast(n / p, res);
            return;}}
    lng div = n, x0 = rnd.randLng(2, n - 1), c = rnd.randLng(1, n - 1);
    while (div == n) {
        div = getOneFacBPR(n, x0, c);
        if (div == n) {
            x0 = rnd.randLng(2, n - 1);
            c = rnd.randLng(1, n - 1);}} 
    getPrimeFacFast(div, res);
    getPrimeFacFast(n / div, res);}
#line 5 "5-Mathematics\\09-getallfac.hpp"

// T: O(n^(1/4)) or O(n^(1/2)), M: O(d(n))
vector<lng> getAllFac(lng n, bool opt = 0) {
    if (n <= 0) { return {}; }
    if (n == 1) { return {1}; }
    vector<lng> prms; 
    //* 
    if (!opt) { prms = getPrimeFacSlow(n); }//*/  
    //*
    if (opt) { getPrimeFacFast(n, prms); }//*/
    sort(prms.begin(), prms.end());
    vector<lng> res = {1};
    for (int i = 0; i < prms.size();) {
        int cnt = 0; lng p = prms[i];
        while (i < prms.size() && prms[i] == p) { cnt++; i++; }
        int len = res.size();
        for (int j = 0; j < len; j++) {
            lng cur = res[j];
            for (int k = 0; k < cnt; k++) { cur *= p; res.push_back(cur); }}}
    sort(res.begin(), res.end());
    return res;}
#line 4 "5-Mathematics\\10-getphi.hpp"

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lng getPhi(lng n, bool opt = 0) {
    if (!opt) {
        lng res = n;
        for (lng i = 2; i * i <= n; i++) {
            if (n % i == 0) { 
                while (n % i == 0) { n /= i; }
                res -= res / i;}}
        if (n > 1) { res -= res / n; }
        return res;}
    vector<lng> prms;
    //*
    getPrimeFacFast(n, prms);//*/
    sort(prms.begin(), prms.end());
    prms.erase(unique(prms.begin(), prms.end()), prms.end());
    lng res = n;
    for (lng p : prms) { res -= res / p; }
    return res;}
#line 4 "5-Mathematics\\11-getnumdiv.hpp"

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lng getNumDiv(lng n, bool opt = 0) {
    if (!opt) {
        lng res = 1;
        for (lng i = 2; i * i <= n; i++) {
            if (n % i == 0) {
                int exp = 0;
                do { exp++; n /= i; } while (n % i == 0);
                res *= exp + 1;}}
        if (n > 1) { res *= 2; }
        return res;}
    vector<lng> prms;
    //*
    getPrimeFacFast(n, prms);//*/
    sort(prms.begin(), prms.end());
    lng res = 1;
    for (int i = 0; i < prms.size();) {
        int cnt = 0; lng p = prms[i];
        while (i < prms.size() && prms[i] == p) { cnt++; i++; }
        res *= cnt + 1;}
    return res;}
#line 4 "5-Mathematics\\12-getsumdiv.hpp"

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lll getSumDiv(lng n, bool opt = 0) {
    if (!opt) {
        lng res = 1;
        for (lng i = 2; i * i <= n; i++) {
            if (n % i == 0) {
                lng pow = 1, sum = 1;
                do { pow *= i; sum += pow; n /= i; } while (n % i == 0);
                res *= sum;}}
        if (n > 1) { res *= (1 + n); }
        return res;}
    vector<lng> prms;
    //*
    getPrimeFacFast(n, prms);//*/
    sort(prms.begin(), prms.end());
    lll res = 1;
    for (int i = 0; i < prms.size();) {
        int cnt = 0; lng p = prms[i]; 
        while (i < prms.size() && prms[i] == p) { cnt++; i++; }
        lng pow = 1; lll sum = 1;
        for (int j = 0; j < cnt; j++) { pow *= p; sum += pow; }
        res *= sum;}
    return res;}
#line 4 "5-Mathematics\\13-getinv.hpp"

// T: O(log(n)), M: O(1)
lng getInvE(lng n, lng mod) {
    n = (n % mod + mod) % mod;
    if (n == 0) { return -1; }
    lng x, y, g = exGcd(n, mod, x, y);
    if (g != 1) { return -1; }
    return (x % mod + mod) % mod;}

// T: O(n^(1/3)), M: O(log(n))
lng getInvP(lng n, lng mod) { // Pierce Expansion, mod must be prime
    return n <= 1 ? n : mod - lll(mod) / n * getInvP(mod % n, mod) % mod;}
#line 4 "5-Mathematics\\solvemodeq.hpp"

// T: O(log(n)), M: O(1)
pair<lng, lng> solveModEq(lng a, lng b, lng m0) { // ax ≡ b (mod m0)
    a = (a % m0 + m0) % m0; b = (b % m0 + m0) % m0;
    lng x, y, g = exGcd(a, m0, x, y);
    if (b % g != 0) { return {-1, -1}; }
    lng m = m0 / g;
    x = lll(x) * (b / g) % m;
    if (x < 0) { x += m; }
    return {x, m};}
#line 5 "5-Mathematics\\14-chiremthm.hpp"

 // T: O(n * log(n)) or O(n^2), M: O(n)
template<typename T> 
requires (std::integral<T> || requires { T::is_inf; })
pair<T, T> chiRemThm(const vector<pair<lng, lng>> &cong) { // x ≡ a_i (mod m_i)
    if (cong.empty()) { return {0, 1}; }
    T a = cong[0].first, m = cong[0].second;
    for (int i = 1; i < cong.size(); i++) {
        lng a_i = cong[i].first, m_i = cong[i].second;
        lng d = a_i - lng(a % m_i), x, y, g = exGcd(lng(m % m_i), m_i, x, y);
        if (d % g != 0) { return {-1, -1}; } 
        m_i /= g;
        x = (lll((x % m_i + m_i) % m_i) * (d / g) % m_i + m_i) % m_i;
        a += x * m; m *= m_i;}
    return {(a % m + m) % m, m};}

 // T: O(n * log(n)) or O(n^2), M: O(n)
template<typename T>
requires (std::integral<T> || requires { T::is_inf; })
pair<T, T> superChiRemThm(const vector<tuple<lng, lng, lng>> &eqs) { // a_i * x ≡ b_i (mod m_i)
    vector<pair<lng, lng>> cong;
    for (const auto &[a_i, b_i, m_i] : eqs) {
        pair<lng, lng> cur = solveModEq(a_i, b_i, m_i);
        if (cur.first == -1) { return {-1, -1}; }
        cong.push_back(cur);}
    return chiRemThm<T>(cong);}
#line 4 "5-Mathematics\\15-garner.hpp"

// T: O(n^2 * log(n)), M: O(n^2)
template<typename T> 
requires (std::integral<T> || requires { T::is_inf; })
T garner(const vector<pair<lng, lng>> &cong, T mod) { // coprime moduli
    if (cong.empty()) { return 0; }
    int n = cong.size();
    vector<vector<lng>> R(n, vector<lng>(n));
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            R[i][j] = getInvE(cong[i].second, cong[j].second);
            if (R[i][j] == -1) { return -1; }}}
    vector<lng> X(n);
    for (int i = 0; i < n; i++) {
        X[i] = (cong[i].first % cong[i].second + cong[i].second) % cong[i].second;
        for (int j = 0; j < i; j++) {
            X[i] = (lll(R[j][i]) * (X[i] - X[j]) % cong[i].second + cong[i].second) % cong[i].second;}}
    T res = 0, mul = 1;
    for (int i = 0; i < n; i++) { res = (res + mul * X[i]) % mod; mul = mul * cong[i].second % mod; }
    return res;}
#line 4 "5-Mathematics\\16-modlog.hpp"

// T: O(n^(1/2)), M: O(n^(1/2))
lng modLog(lng a, lng b, lng mod, lng k = 1) { // a^x ≡ b mod m
    a = (a % mod + mod) % mod; 
    b = (b % mod + mod) % mod;
    k = (k % mod + mod) % mod;
    if (mod == 1 || b == k) { return 0; }
    lng add = 0, g;
    while ((g = gcd(a, mod)) > 1) {
        if (b == k) { return add; }
        if (b % g != 0) { return -1; }
        b /= g; mod /= g;
        k = lll(k) * a / g % mod;
        add++;}
    lng n = sqrt(mod) + 1, an = modPow(a, n, mod);
    vector<pair<lng, int>> Q0(n + 1);
    for (lng q = 0, r = b; q <= n; q++) { Q0[q] = {r, q}; r = lll(r) * a % mod; }
    sort(Q0.begin(), Q0.end());
    vector<pair<lng, int>> Q;
    for (auto &[r, q] : Q0) {
        if (!Q.empty() && Q.back().first == r) { Q.back().second = q; }
        else { Q.push_back({r, q}); }}
    for (lng p = 1, l = k; p <= n; p++) {
        l = lll(l) * an % mod;
        auto it = lower_bound(Q.begin(), Q.end(), std::make_pair(l, -1));
        if (it != Q.end() && it->first == l) { return n * p - it->second + add; }}
    return -1;}
#line 7 "5-Mathematics\\17-getprimroot.hpp"

// T: O(n^(1/4) * log(n)) or O(n^(1/2) * log(n)), M: O(log(n))
lng getPrimRoot(lng mod, bool opt = 0) {
    if (mod <= 0) { return -1; }
    if (mod == 1 || mod == 2 || mod == 4) { return mod - 1; }
    if (mod % 4 == 0) { return -1; }
    vector<lng> prms;
    //*
    if (!opt) { prms = getPrimeFacSlow(mod >> (!(mod & 1))); }//*/
    //*
    if (opt) { getPrimeFacFast(mod >> (!(mod & 1)), prms); }//*/
    sort(prms.begin(), prms.end());
    if (!prms.empty()) { if (prms[0] != prms.back()) { return -1; } }
    lng phi = getPhi(mod, opt); prms.clear();
    //*
    if (!opt) { prms = getPrimeFacSlow(phi); }//*/
    //*
    if (opt) { getPrimeFacFast(phi, prms); }//*/
    sort(prms.begin(), prms.end());
    prms.erase(unique(prms.begin(), prms.end()), prms.end());
    for (lng res = 2; res < mod; res++) {
        if (gcd(res, mod) != 1) { continue; }
        bool chk = true;
        for (int i = 0; i < prms.size() && chk; i++) {
            if (modPow(res, phi / prms[i], mod) == 1) { chk = false; }}
        if (chk) { return res; }}
    return -1;}
#line 7 "5-Mathematics\\18-modroot.hpp"

// T: O(n^(1/2)), M: O(n^(1/2))
vector<lng> modRoot(lng a, lng b, lng mod, bool opt = 0) { // x^a ≡ b mod m
    if (b == 0) { return {0}; }
    lng g = getPrimRoot(mod, opt);
    if (g == -1) { return {}; }
    lng k = modLog(g, b, mod);
    if (k == -1) { return {}; } 
    lng phi = getPhi(mod, opt), x, y, d = exGcd(a, phi, y, x);
    if (k % d != 0) { return {}; }
    y = (lll(y) % phi + phi) % phi * (k / d) % phi;
    vector<lng> res;
    for (lng i = 0; i < d; i++) {
        res.push_back(modPow(g, y + i * (phi / d), mod));}
    sort(res.begin(), res.end());
    res.erase(unique(res.begin(), res.end()), res.end());
    return res;}
#line 3 "5-Mathematics\\19-getfloorsum.hpp"

// T: O(log(n)), M: O(1)
lll getFloorSum(lng n, lng a, lng b, lng mod) { // \sum_{i = 0}^{n - 1} floor((a * x + b) / m)
    lll res = 0;
    if (a >= mod) { res += lll(n - 1) * n / 2 * (a / mod); a %= mod; }
    if (b >= mod) { res += lll(n) * (b / mod); b %= mod; }
    lng y = (lll(a) * n + b) / mod;
    if (y == 0) { return res; }
    lll x = lll(y) * mod - b;
    res += lll(n - (x + a - 1) / a) * y;
    res += getFloorSum(y, mod, (a - x % a) % a, a);
    return res;}
#line 3 "5-Mathematics\\20-montgomery.hpp"

// S: O(1), U: NA, Q: O(1), M: O(1)
struct Montgomery {
    ulng mod, inv, rsq;
    Montgomery(ulng n = 1) : mod(n), inv(n) {
        assert(n & 1);
        for (int i = 0; i < 5; i++) { inv *= 2 - mod * inv; }
        inv = -inv; rsq = -ulll(mod) % mod;}

    ulng red(ulll x) const {
        ulng q = ulng(x) * inv, res = (x + ulll(q) * mod) >> 64;
        return res - (res >= mod) * mod;}
    ulng init(ulng x) const { return red(ulll(x) * rsq); }
    ulng mul(ulng a, ulng b) const { return red(ulll(a) * b); }
    // Q: O(log(n))
    ulng pow(ulng a, ulng b) const {
        a = init(a); ulng res = init(1);
        while (b > 0) {
            if (b & 1) { res = mul(res, a); }
            b >>= 1;
            if (b > 0) { a = mul(a, a); }}
        return red(res);}
};
#line 22 "X-Tests Abbreviated\\Mathematics.cpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    return 0;
}
