#include "../../05-Mathematics/21-integer_roots.hpp"

static lng checks = 0;
static string_view curSet = "";
static ulll curN = 0;
static lng curK = 0;
static bool curSigned = false;
constexpr ulng TOP = ~ulng(0);
constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();
const vector<int> BIG_K{65, 100, 109, 110, 127, 128, 129, 200, 1000000000, std::numeric_limits<int>::max()};

string decimal(ulll x) {
    string s;
    do { s += char('0' + x % 10); x /= 10; } while (x);
    reverse(s.begin(), s.end());
    return s;}
// The input is recorded cheaply here and formatted only on failure.
void at(string_view set, ulll n, lng k = 0) { curSet = set; curN = n; curK = k; curSigned = false; }
void atSigned(string_view set, lng s) { curSet = set; curN = ulll(lll(s)); curK = 0; curSigned = true; }
void need(bool ok, string_view op, const string &want = "true", const string &got = "false") {
    ++checks;
    if (!ok) {
        string n = curSigned && lng(curN) < 0 ? "-" + decimal(ulng(0) - ulng(curN)) : decimal(curSigned ? ulll(ulng(curN)) : curN);
        cerr << "FAIL operation=" << op << " test=" << curSet << " n=" << n << " k=" << curK << " expected=" << want << " actual=" << got << '\n';
        std::exit(1);}}
void expect(ulll got, ulll want, string_view op) { need(got == want, op, decimal(want), decimal(got)); }
void expectSigned(lng got, lng want, string_view op) { need(got == want, op, std::to_string(want), std::to_string(got)); }

// min(b^k, 2^64) by repeated multiplication: b <= 1 needs no loop, otherwise at most 65 steps, each product below 2^128.
ulll powSat(ulng b, lng k) {
    if (b <= 1) { return b; }
    ulll res = 1;
    for (lng i = 0; i < k && res <= TOP; ++i) { res *= b; }
    return min(res, ulll(TOP) + 1);}
// Largest r with r^k <= n by binary search below 2^(64 / k + 1), independent of the header's floating estimate.
ulng rootOracle(ulng n, lng k) {
    if (k == 1) { return n; }
    ulng lo = 0, hi = k >= 64 ? 1 : ulng(1) << (64 / k + 1);
    while (lo < hi) {
        ulng mid = lo + (hi - lo + 1) / 2;
        if (powSat(mid, k) <= n) { lo = mid; }
        else { hi = mid - 1; }}
    return lo;}
// {b, e} with b^e = n and e maximal, trying e = bit_width(n) - 1, ..., 2 with the binary-search root.
pair<ulng, int> powerOracle(ulng n) {
    for (int e = min(63, int(std::bit_width(n)) - 1); e >= 2; --e) {
        ulng b = rootOracle(n, e);
        if (powSat(b, e) == n) { return {b, e}; }}
    return {n, 1};}

// Little-endian base-2^32 naturals with normalized top limb, for exact comparisons of (2r + 1)^j with n 2^j.
using Big = vector<ulng>;
void mulSmall(Big &a, ulng m) {
    ulll carry = 0;
    for (ulng &x : a) { carry += ulll(x) * m; x = ulng(carry & 0xFFFFFFFF); carry >>= 32; }
    for (; carry; carry >>= 32) { a.pb(ulng(carry & 0xFFFFFFFF)); }}
bool bigLess(const Big &a, const Big &b) {
    if (a.size() != b.size()) { return a.size() < b.size(); }
    for (int i = int(a.size()) - 1; i >= 0; --i) { if (a[i] != b[i]) { return a[i] < b[i]; }}
    return false;}
// round(n^(1/k)) from the floor r: r + 1 iff (2r + 1)^k < 2^k n. Once q^j >= n 2^j it stays so (q / 2 >= 3 / 2), so the loop
// stops within about 110 steps; equality never occurs (odd against even).
ulng roundOracle(ulng n, lng k, ulng r) {
    if (r == 0 || k == 1) { return r; }
    Big a{1}, b{n & 0xFFFFFFFF};
    if (n >> 32) { b.pb(n >> 32); }
    for (lng j = 1; j <= k; ++j) {
        mulSmall(a, 2 * r + 1); mulSmall(b, 2);
        if (!bigLess(a, b)) { return r; }}
    return r + 1;}
// floor((r + 1/2)^k) = floor((2r + 1)^k / 2^k) by big multiplication and a bit-serial shift, saturated at 2^64; k <= 200.
ulll halfPow(ulng r, int k) {
    Big a{1};
    for (int j = 0; j < k; ++j) { mulSmall(a, 2 * r + 1); }
    ulll res = 0;
    for (lng p = lng(a.size()) * 32 - 1; p >= k; --p) {
        res = res << 1 | (a[p / 32] >> (p % 32) & 1);
        if (res > TOP) { return ulll(TOP) + 1; }}
    return res;}

// ikthRoot, ceilKthRoot and kthRootRound at (n, k) against a known floor r and rounded value.
void checkKth(ulng n, int k, ulng r, ulng round) {
    expect(ikthRoot(n, k), r, "ikthRoot");
    expect(ceilKthRoot(n, k), r + (powSat(r, k) != n), "ceilKthRoot");
    expect(kthRootRound(n, k), round, "kthRootRound");}
// Every square-root operation and ikthRoot(n, 2) at n against a known root s.
void checkSqrt(ulng n, ulng s) {
    expect(isqrt(n), s, "isqrt");
    auto [q, rem] = isqrtRemainder(n);
    expect(q, s, "isqrtRemainder root"); expect(rem, n - s * s, "isqrtRemainder remainder");
    need(isPerfectSquare(n) == (s * s == n), "isPerfectSquare", std::to_string(s * s == n), std::to_string(isPerfectSquare(n)));
    expect(isqrt128(n), s, "isqrt128 on a 64-bit value");
    expect(ikthRoot(n, 2), s, "ikthRoot k = 2");}
void checkSqrt128(ulll n) {
    at("isqrt128 certificate", n);
    ulng s = isqrt128(n);
    need(ulll(s) * s <= n && (s == TOP || ulll(s + 1) * (s + 1) > n), "isqrt128 certificate s^2 <= n < (s + 1)^2", "certificate", decimal(s));}
void checkCbrt(lng v) {
    atSigned("icbrt certificate", v);
    lll c = icbrt(v);
    need(c * c * c <= v && (c + 1) * (c + 1) * (c + 1) > v, "icbrt certificate c^3 <= n < (c + 1)^3", "certificate", std::to_string(lng(c)));}
void checkPower(ulng n, pair<ulng, int> want) {
    auto [b, e] = isPerfectPower(n);
    expect(b, want.fi, "isPerfectPower base"); expect(ulng(e), ulng(want.se), "isPerfectPower exponent");}

void explicitValues() {
    struct Root { ulng n; int k; ulng fl, ce, ro; };
    int big = std::numeric_limits<int>::max();
    for (auto [n, k, fl, ce, ro] : vector<Root>{{0, 1, 0, 0, 0}, {TOP, 1, TOP, TOP, TOP}, {0, 2, 0, 0, 0}, {1, 2, 1, 1, 1}, {2, 2, 1, 2, 1},
            {3, 2, 1, 2, 2}, {TOP, 2, 4294967295, 4294967296, 4294967296}, {TOP, 3, 2642245, 2642246, 2642246}, {TOP, 4, 65535, 65536, 65536},
            {TOP, 5, 7131, 7132, 7132}, {TOP, 40, 3, 4, 3}, {TOP, 63, 2, 3, 2}, {TOP, 64, 1, 2, 2}, {TOP, 109, 1, 2, 2}, {TOP, 110, 1, 2, 1},
            {ulng(1) << 63, 63, 2, 2, 2}, {(ulng(1) << 63) - 1, 63, 1, 2, 2}, {215, 3, 5, 6, 6}, {216, 3, 6, 6, 6}, {217, 3, 6, 7, 6},
            {1000000000000000000, 6, 1000, 1000, 1000}, {0, big, 0, 0, 0}, {1, big, 1, 1, 1}, {2, big, 1, 2, 1}, {TOP, big, 1, 2, 1},
            {TOP, 1000000000, 1, 2, 1}, {1, 1000000000, 1, 1, 1}}) {
        at("explicit roots", n, k);
        expect(ikthRoot(n, k), fl, "ikthRoot"); expect(ceilKthRoot(n, k), ce, "ceilKthRoot"); expect(kthRootRound(n, k), ro, "kthRootRound");}
    ulll W = ulll(TOP) * TOP;
    for (auto [n, s] : vector<pair<ulll, ulng>>{{0, 0}, {ulll(TOP), 4294967295}, {ulll(1) << 64, ulng(1) << 32}, {(ulll(1) << 64) + 1, ulng(1) << 32},
            {W, TOP}, {W - 1, TOP - 1}, {W + 1, TOP}, {~ulll(0), TOP}, {ulll(1) << 126, ulng(1) << 63}, {(ulll(1) << 126) - 1, (ulng(1) << 63) - 1}}) {
        at("explicit isqrt128", n);
        expect(isqrt128(n), s, "isqrt128");}
    for (auto [v, c] : vector<pair<lng, lng>>{{0, 0}, {1, 1}, {7, 1}, {8, 2}, {-1, -1}, {-7, -2}, {-8, -2}, {-9, -3}, {LO, -2097152}, {LO + 1, -2097152},
            {HI, 2097151}, {lng(2097151) * 2097151 * 2097151, 2097151}, {-lng(2097151) * 2097151 * 2097151 - 1, -2097152}}) {
        atSigned("explicit icbrt", v);
        expectSigned(icbrt(v), c, "icbrt");}
    for (auto [n, b, e] : vector<tuple<ulng, ulng, int>>{{0, 0, 1}, {1, 1, 1}, {2, 2, 1}, {4, 2, 2}, {8, 2, 3}, {64, 2, 6}, {36, 6, 2}, {ulng(1) << 63, 2, 63},
            {ulng(1) << 62, 2, 62}, {ulng(1) << 60, 2, 60}, {12157665459056928801ULL, 3, 40}, {4738381338321616896ULL, 6, 24},
            {3909821048582988049ULL, 7, 22}, {1000000000000000000ULL, 10, 18}, {TOP, TOP, 1}, {18446744065119617025ULL, 4294967295, 2},
            {18446744030759878681ULL, 4294967291, 2}, {TOP - 1, TOP - 1, 1}}) {
        at("explicit isPerfectPower", n);
        checkPower(n, {b, e});}
    cout << "PASS explicit values checks=" << checks << '\n';}

// Every n < limit for one k against roots grown by incrementing: r^k <= n < (r + 1)^k, rounding to r + 1 from up = floor((r + 1/2)^k) + 1.
// For k > 200 every n < 2^64 has r <= 1 and 1.5^k > 1.5^200 > 2^64, so halfPow at 200 saturates exactly as at k.
void exhaustiveK(ulng limit, int k) {
    ulng r = 0;
    ulll next = 1, up = 1;
    for (ulng n = 0; n < limit; ++n) {
        if (n == next) { ++r; next = powSat(r + 1, k); up = halfPow(r, min(k, 200)) + 1; }
        at("exhaustive kth", n, k);
        checkKth(n, k, r, r + (n >= up));}}
// Square roots, isqrt128, perfect squares and perfect powers on [0, limit), icbrt on [-limit, limit], all by incremental brute force.
void exhaustiveOps(lng limit) {
    vector<int> E(limit, 1);
    vector<ulng> B(limit);
    std::iota(B.begin(), B.end(), ulng(0));
    for (lng b = 2; b * b < limit; ++b) {
        lng p = b * b;
        for (int e = 2; p < limit; ++e, p *= b) { if (e > E[p]) { E[p] = e; B[p] = ulng(b); }}}
    ulng s = 0;
    for (lng n = 0; n < limit; ++n) {
        if ((s + 1) * (s + 1) == ulng(n)) { ++s; }
        at("exhaustive small", ulng(n));
        checkSqrt(ulng(n), s);
        checkPower(ulng(n), {B[n], E[n]});}
    lng c = -(lng(1) << 12);
    for (lng v = -limit; v <= limit; ++v) {
        while ((c + 1) * (c + 1) * (c + 1) <= v) { ++c; }
        atSigned("exhaustive icbrt", v);
        expectSigned(icbrt(v), c, "icbrt");}
    cout << "PASS exhaustive n<" << limit << " (sqrt family, isqrt128, isPerfectPower) and |n|<=" << limit << " (icbrt) checks=" << checks << '\n';}

// r^k - 1, r^k, r^k + 1 for one r >= 2 with r^k < 2^64; rounding at both neighbours stays at r since
// r^k - (r - 1/2)^k >= 3/2 and (r + 1/2)^k - r^k >= 2 for k >= 2.
void powerNeighbours(int k, ulng r) {
    ulng p = ulng(powSat(r, k));
    at("power neighbours", p, k);
    checkKth(p - 1, k, r - 1, r); checkKth(p, k, r, r);
    if (p < TOP) { checkKth(p + 1, k, r, r); }
    if (k == 2) { checkSqrt(p - 1, r - 1); checkSqrt(p, r); checkSqrt(p + 1, r); }
    if (k == 3 && p <= ulng(HI)) {
        lng v = lng(p), c = lng(r);
        for (auto [x, want] : vector<pair<lng, lng>>{{v - 1, c - 1}, {v, c}, {v + 1, c}, {-v + 1, -c}, {-v, -c}, {-v - 1, -c - 1}}) {
            atSigned("cube neighbours", x);
            expectSigned(icbrt(x), want, "icbrt");}}}
// The rounding threshold F = floor((r + 1/2)^k): F rounds to r and F + 1 to r + 1 (both have floor root r).
void roundNeighbours(int k, ulng r) {
    ulll f = halfPow(r, k);
    if (f >= TOP) { return; }
    at("rounding threshold", f, k);
    checkKth(ulng(f), k, r, r); checkKth(ulng(f + 1), k, r, r + 1);}
// Roots r in [2, cap + 1] and [rmax - cap, rmax] (every root in [2, rmax] when they meet).
template <class F> void rootRange(ulng rmax, ulng cap, F f) {
    for (ulng r = 2; r <= rmax; ++r) {
        if (r - 2 >= cap && rmax - r >= cap) { r = rmax - cap; }
        f(r);}}
void boundaries(std::mt19937_64 &rng, bool quick, bool stress) {
    ulng capPow = quick ? 1 << 12 : TOP, capSq = quick ? 1 << 10 : stress ? 1 << 20 : 1 << 18, capRound = quick ? 1 << 8 : 1 << 16;
    for (int k = 2; k <= 63; ++k) {
        ulng rmax = rootOracle(TOP, k);
        rootRange(rmax, k == 2 ? capSq : capPow, [&](ulng r) { powerNeighbours(k, r); });
        rootRange(rmax, k <= 3 ? capRound : capPow, [&](ulng r) { roundNeighbours(k, r); });
        if (k <= 3) {
            for (ulng i = 0; i < capRound; ++i) { ulng r = 2 + rng() % (rmax - 1); powerNeighbours(k, r); roundNeighbours(k, r); }}}
    for (int k = 2; k <= 200; ++k) { roundNeighbours(k, 1); }
    cout << "PASS power and rounding-threshold neighbours k<=63 checks=" << checks << '\n';
    // isqrt128 around squares: the 64-bit branch boundary, the top roots, and roots of every width.
    auto square = [&](ulng r) {
        ulll p = ulll(r) * r;
        for (auto [n, s] : vector<pair<ulll, ulng>>{{p - 1, r - 1}, {p, r}, {p + 1, r}, {p + 2 * ulll(r), r}}) {
            at("isqrt128 square neighbours", n);
            expect(isqrt128(n), s, "isqrt128");}};
    ulng span = quick ? 1 << 10 : stress ? 1 << 20 : 1 << 18;
    for (ulng r = 1; r <= span; ++r) { square(r); square((ulng(1) << 32) - span / 2 + r); square(TOP - r + 1); }
    for (ulng i = 0; i < 4 * span; ++i) { square(max<ulng>(1, rng() >> (rng() % 64))); }
    cout << "PASS isqrt128 square neighbours checks=" << checks << '\n';
    // isPerfectPower on b^e and its neighbours, b^e = c^(f e) for b = c^f with f maximal.
    ulng cap = quick ? 256 : stress ? 8192 : 4096, top = quick ? 4 : stress ? 256 : 64;
    vector<pair<ulng, int>> po(cap + 1);
    for (ulng b = 2; b <= cap; ++b) { po[b] = powerOracle(b); }
    for (int e = 2; e <= 63; ++e) {
        ulng rmax = rootOracle(TOP, e);
        rootRange(rmax, min(cap - 1, rmax), [&](ulng b) {
            ulng n = ulng(powSat(b, e));
            auto [c, f] = b <= cap ? po[b] : powerOracle(b);
            at("isPerfectPower on b^e", n, e);
            checkPower(n, {c, f * e});
            if (b <= 64 || b + top > rmax) { checkPower(n - 1, powerOracle(n - 1)); if (n < TOP) { checkPower(n + 1, powerOracle(n + 1)); }}});}
    cout << "PASS isPerfectPower on perfect powers and neighbours checks=" << checks << '\n';}

ulng randomWidth(std::mt19937_64 &rng) { ulng t = rng() % 65; return t == 64 ? 0 : rng() >> t; }
int randomK(std::mt19937_64 &rng) {
    ulng t = rng() % 8;
    return t < 6 ? 1 + int(rng() % 64) : t == 6 ? 65 + int(rng() % 136) : BIG_K[rng() % BIG_K.size()];}
void randomRounds(std::mt19937_64 &rng, int rounds) {
    vector<ulng> rmax(65, TOP);
    for (int k = 2; k <= 64; ++k) { rmax[k] = rootOracle(TOP, k); }
    for (int i = 0; i < rounds; ++i) {
        int k = randomK(rng), e = 2 + int(rng() % 62);
        ulng n = randomWidth(rng), b = max<ulng>(2, randomWidth(rng) % (rmax[e] + 1)), p = ulng(powSat(b, e)) + (rng() % 3) - 1;
        if (i & 1) { n = p; }
        at("random kth certificate", n, k);
        ulng r = ikthRoot(n, k);
        need(powSat(r, k) <= n && (r == TOP || powSat(r + 1, k) > n), "ikthRoot certificate r^k <= n < (r + 1)^k", "certificate", decimal(r));
        checkKth(n, k, r, roundOracle(n, k, r));
        at("random sqrt certificate", n);
        ulng s = isqrt(n);
        need(ulll(s) * s <= n && ulll(s + 1) * (s + 1) > n, "isqrt certificate s^2 <= n < (s + 1)^2", "certificate", decimal(s));
        checkSqrt(n, s);
        if (i % 8 == 0) { at("random isPerfectPower", n); checkPower(n, powerOracle(n)); }
        ulll w = ulll(rng()) << 64 | rng(), sq = ulll(randomWidth(rng));
        int t = int(rng() % 129);
        checkSqrt128(t == 128 ? 0 : w >> t);
        checkSqrt128(sq * sq + (rng() % 3) - 1);
        lng c = lng(rng() % 2097152) * (rng() & 1 ? -1 : 1);
        checkCbrt(std::bit_cast<lng>(rng()) >> (rng() % 64));
        checkCbrt(c * c * c + lng(rng() % 3) - 1);}
    cout << "PASS seeded random rounds=" << rounds << " checks=" << checks << '\n';}

int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20261103;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            string m;
            ulng n;
            int k;
            lng v;
            while (cin >> n >> k >> m >> v) {
                ulll w = 0;
                for (char ch : m) { w = w * 10 + ulng(ch - '0'); }
                auto [q, rem] = isqrtRemainder(n);
                auto [b, e] = isPerfectPower(n);
                cout << isqrt(n) << ' ' << q << ' ' << rem << ' ' << isPerfectSquare(n) << ' ' << ikthRoot(n, k) << ' ' << ceilKthRoot(n, k) << ' '
                    << kthRootRound(n, k) << ' ' << b << ' ' << e << ' ' << isqrt128(w) << ' ' << icbrt(v) << '\n';}
            return 0;}}
    if (!invalid.empty()) {
        if (invalid == "ikthroot-zero") { ikthRoot(5, 0); }
        else if (invalid == "ikthroot-negative") { ikthRoot(5, -1); }
        else if (invalid == "ceilkthroot-zero") { ceilKthRoot(5, 0); }
        else if (invalid == "ceilkthroot-negative") { ceilKthRoot(5, -1); }
        else if (invalid == "kthrootround-zero") { kthRootRound(5, 0); }
        else if (invalid == "kthrootround-negative") { kthRootRound(5, -1); }
        else { return 2; }
        return 0;}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    explicitValues();
    ulng small = quick ? 1 << 12 : stress ? 1 << 18 : 1 << 16;
    for (int k = 1; k <= 20; ++k) { exhaustiveK(small, k); }
    for (int k = 21; k <= 70; ++k) { exhaustiveK(small >> 4, k); }
    for (int k : BIG_K) { exhaustiveK(small >> 4, k); }
    cout << "PASS exhaustive kth roots n<" << small << " (k<=20) and n<" << (small >> 4) << " (k=21..70 and huge k) checks=" << checks << '\n';
    exhaustiveOps(quick ? 1 << 16 : stress ? 1 << 23 : 1 << 22);
    boundaries(rng, quick, stress);
    randomRounds(rng, quick ? 5000 : stress ? 600000 : 200000);}
