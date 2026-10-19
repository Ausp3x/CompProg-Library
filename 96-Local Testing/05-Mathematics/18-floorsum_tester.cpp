#include "../../05-Mathematics/18-floorsum.hpp"
#include "../../01-Core/05-modint.hpp"

string context;
lng checks = 0;
string decimal(ulll x) {
    string s;
    do { s += char('0' + x % 10); x /= 10; } while (x);
    reverse(s.begin(), s.end()); return s;}
string decimal(lll x) { return x < 0 ? "-" + decimal(ulll(0) - ulll(x)) : decimal(ulll(x)); }
void need(bool ok, string_view what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " input=" << context << '\n'; std::exit(1); }}
string show(lll n, lll m, lll a, lll b) { return "n=" + decimal(n) + " m=" + decimal(m) + " a=" + decimal(a) + " b=" + decimal(b); }
ulll tri(ulll n) { return n * (n - 1) / 2; }
constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();

// Every operation on one small-modulus instance against direct loops; every [l, r) and the first hit by scanning one period.
void brute(lng n, lng m, lng a, lng b) {
    context = show(n, m, a, b);
    lll fs = 0, cs = 0, ms = 0;
    ulll f1 = 0, f2 = 0, f3 = 0;
    mint g1 = 0, g2 = 0, g3 = 0;
    vector<lng> vals;
    for (lng i = 0; i < n; ++i) {
        lll y = lll(a) * i + b, f = floorDiv(y, m);
        fs += f; cs += ceilDiv(y, m); ms += modNorm(y, m); vals.pb(modNorm(y, m));
        f1 += ulll(f); f2 += ulll(f) * ulng(i); f3 += ulll(f) * ulll(f);
        g1 += mint(f); g2 += mint(f) * mint(i); g3 += mint(f) * mint(f);}
    need(floorSum(n, m, a, b) == fs, "floorSum");
    need(ceilSum(n, m, a, b) == cs, "ceilSum");
    need(modSum(n, m, a, b) == ms, "modSum");
    auto u = floorSumMoments<ulll>(n, m, a, b);
    need(u[0] == f1 && u[1] == f2 && u[2] == f3, "floorSumMoments ulll");
    auto w = floorSumMoments<mint>(n, m, a, b);
    need(w[0] == g1 && w[1] == g2 && w[2] == g3, "floorSumMoments mint");
    for (lng l = 0; l <= m; ++l) { for (lng r = l; r <= m; ++r) {
        lng c = 0, first = -1;
        for (lng v : vals) { c += l <= v && v < r; }
        for (lng x = 0; x < m && first < 0; ++x) { lng v = modNorm(lll(a) * x + b, m); if (l <= v && v < r) { first = x; }}
        need(countModInRange(n, m, a, b, l, r) == c, "countModInRange");
        need(firstModInRange(m, a, b, l, r) == first, "firstModInRange");}}
    if (n >= 1) {
        auto lo = std::min_element(vals.begin(), vals.end()), hi = std::max_element(vals.begin(), vals.end());
        need(minOfModOfLinear(n, m, a, b) == pair<lng, lng>{*lo, lo - vals.begin()}, "minOfModOfLinear");
        need(maxOfModOfLinear(n, m, a, b) == pair<lng, lng>{*hi, hi - vals.begin()}, "maxOfModOfLinear");}}

// Any modulus: direct loops of length n with 128-bit values.
void bruteLong(std::mt19937_64 &rng, lng n, lng m, lng a, lng b) {
    context = show(n, m, a, b);
    lll fs = 0, cs = 0, ms = 0;
    ulll f2 = 0, f3 = 0;
    lng mn = m, mx = -1, xmn = -1, xmx = -1, l = lng(rng() % ulng(m)), r = l + lng(rng() % (ulng(m - l) + 1)), c = 0;
    for (lng i = 0; i < n; ++i) {
        lll y = lll(a) * i + b, f = floorDiv(y, m);
        lng v = modNorm(y, m);
        fs += f; cs += ceilDiv(y, m); ms += v; f2 += ulll(f) * ulng(i); f3 += ulll(f) * ulll(f);
        if (v < mn) { mn = v; xmn = i; }
        if (v > mx) { mx = v; xmx = i; }
        c += l <= v && v < r;}
    need(floorSum(n, m, a, b) == fs && ceilSum(n, m, a, b) == cs && modSum(n, m, a, b) == ms, "long sums");
    auto u = floorSumMoments<ulll>(n, m, a, b);
    need(u[0] == ulll(fs) && u[1] == f2 && u[2] == f3, "long moments");
    need(minOfModOfLinear(n, m, a, b) == pair<lng, lng>{mn, xmn} && maxOfModOfLinear(n, m, a, b) == pair<lng, lng>{mx, xmx}, "long min/max");
    need(countModInRange(n, m, a, b, l, r) == c, "long count");}

// Identities over |a|, |b| up to 2^126, all modulo 2^128, and cross-operation certificates.
void identities(std::mt19937_64 &rng) {
    auto any = [&](int bits) { ulll x = (ulll(rng()) << 64 | rng()) >> (128 - bits); return rng() & 1 ? -lll(x) : lll(x); };
    lng n1 = lng(rng() >> 2), n2 = lng(rng() >> (2 + rng() % 62)), m = max<lng>(1, lng(rng() >> (1 + rng() % 63)));
    lll a = any(1 + int(rng() % 60)), b = any(1 + int(rng() % 125));
    context = show(n1, m, a, b) + " n2=" + std::to_string(n2);
    ulll whole = ulll(floorSum(n1 + n2, m, a, b)), split = ulll(floorSum(n1, m, a, b)) + ulll(floorSum(n2, m, a, b + a * n1));
    need(whole == split, "floorSum additivity over n");
    need(ulll(floorSum(n1, m, a + m, b)) == ulll(floorSum(n1, m, a, b)) + tri(ulng(n1)), "floorSum a + m");
    need(ulll(floorSum(n1, m, a, b + m)) == ulll(floorSum(n1, m, a, b)) + ulng(n1), "floorSum b + m");
    need(ulll(ceilSum(n1, m, a, b)) == ulll(0) - ulll(floorSum(n1, m, -a, -b)), "ceilSum mirror");
    lng sb = lng(b % (lll(1) << 62));
    need(floorSumMoments<ulll>(n1, m, lng(a), sb)[0] == ulll(floorSum(n1, m, a, sb)), "moments f equals floorSum");
    lng l = lng(rng() % ulng(m)), r = l + lng(rng() % (ulng(m - l) + 1)), s = r + lng(rng() % (ulng(m - r) + 1)), k = 1 + lng(rng() % 1000000);
    need(countModInRange(k, m, a, b, l, r) + countModInRange(k, m, a, b, r, s) == countModInRange(k, m, a, b, l, s), "count additivity");
    need(countModInRange(k, m, a, b, 0, m) == k, "count total");
    lll ms = modSum(k, m, a, b);
    need(0 <= ms && ms <= lll(k) * (m - 1), "modSum range");
    auto [v, x] = minOfModOfLinear(k, m, a, b);
    need(0 <= x && x < k && modNorm(a * x + b, m) == v && countModInRange(k, m, a, b, 0, v) == 0, "min certificate");
    need(firstModInRange(m, a, b, v, v + 1) == x, "min least x via firstModInRange");
    auto [V, X] = maxOfModOfLinear(k, m, a, b);
    need(0 <= X && X < k && modNorm(a * X + b, m) == V && countModInRange(k, m, a, b, V + 1, m) == 0, "max certificate");
    lng first = firstModInRange(m, a, b, l, r);
    if (first < 0) { need(l == r || countModInRange(m, m, a, b, l, r) == 0, "first none certificate"); }
    else {
        lng fv = modNorm(a * first + b, m);
        need(l <= fv && fv < r && countModInRange(first, m, a, b, l, r) == 0, "first certificate");}}

void quotients(lng n) {
    context = "n=" + std::to_string(n);
    lng expect = 1, blocks = 0, root = lng(std::sqrt(double(n)));
    while (root * root > n) { --root; }
    while ((root + 1) * (root + 1) <= n) { ++root; }
    vector<lng> qs;
    forEachQuotientBlock(n, [&](lng q, lng l, lng r) {
        need(l == expect && l < r && n / l == q && n / (r - 1) == q && (r > n || n / r != q), "block tiling and maximality");
        expect = r; ++blocks; qs.pb(q);});
    need(expect == n + 1, "blocks cover [1, n]");
    need(blocks == (n == 0 ? 0 : 2 * root - (root * root + root > n)), "block count 2 sqrt(n) - [r^2 + r > n]");
    reverse(qs.begin(), qs.end());
    need(enumerateQuotients(n) == qs, "enumerateQuotients ascending");}
void quotientBrute(lng n) {
    context = "n=" + std::to_string(n);
    std::set<lng> want;
    for (lng x = 1; x <= n; ++x) { want.insert(n / x); }
    need(enumerateQuotients(n) == vector<lng>(want.begin(), want.end()), "enumerateQuotients brute");
    quotients(n);}
void joint(const vector<lng> &vs) {
    context = "vs size=" + std::to_string(vs.size());
    lng hi = 0;
    for (lng v : vs) { hi = max(hi, v); }
    auto key = [&](lng x) { vector<lng> k; for (lng v : vs) { k.pb(v / x); } return k; };
    lng expect = 1;
    forEachJointQuotientBlock(vs, [&](lng l, lng r) {
        need(l == expect && l < r && r <= hi + 1, "joint tiling");
        for (lng x = l; x < r; ++x) { need(key(x) == key(l), "joint constant"); }
        need(r == hi + 1 || key(r) != key(l), "joint maximal");
        expect = r;});
    need(expect == hi + 1, "joint cover");}

// floorSumWide against direct loops modulo 2^128 (coefficients below 2^120, n <= 64) and against floorSum on 64-bit moduli.
void wide(std::mt19937_64 &rng) {
    // Signed value of at most bits bits, and a modulus in [1, 2^bits].
    auto big = [&](int bits) { return lll(ulll(rng()) << 64 | rng()) >> (127 - bits); };
    auto mod = [&](int bits) { return lll((ulll(rng()) << 64 | rng()) & ((ulll(1) << bits) - 1)) + 1; };
    lng n = lng(rng() % 65); lll m = mod(1 + int(rng() % 126)), a = big(int(rng() % 120)), b = big(int(rng() % 120)); ulll want = 0;
    for (lng i = 0; i < n; ++i) { want += ulll(floorDiv(a * i + b, m)); }
    context = show(n, m, a, b);
    need(ulll(floorSumWide(n, m, a, b)) == want, "floorSumWide equals the direct sum modulo 2^128");
    lng n2 = lng(rng() >> (1 + rng() % 63)), m2 = max<lng>(1, lng(rng() >> (1 + rng() % 63)));
    lll a2 = lll(std::bit_cast<lng>(rng())) << (rng() % 62), b2 = lll(std::bit_cast<lng>(rng())) << (rng() % 62);
    context = show(n2, m2, a2, b2);
    need(floorSumWide(n2, m2, a2, b2) == floorSum(n2, m2, a2, b2), "floorSumWide equals floorSum on 64-bit moduli");}
// floorsum_detail::divStep against restoring long division of the 192-bit numerator a n + b, one bit at a time.
pair<ulng, ulll> slowDivStep(ulll a, ulng n, ulll b, ulll m) {
    ulng limb[3] = {}, carry = 0;
    for (int i = 0; i < 2; ++i) { ulll t = ulll(ulng(a >> (64 * i))) * n + carry; limb[i] = ulng(t); carry = ulng(t >> 64); }
    limb[2] = carry;
    ulll t = ulll(limb[0]) + ulng(b); limb[0] = ulng(t);
    t = ulll(limb[1]) + ulng(b >> 64) + ulng(t >> 64); limb[1] = ulng(t); limb[2] += ulng(t >> 64);
    ulll r = 0, q = 0;
    for (int i = 191; i >= 0; --i) {
        r = r << 1 | (limb[i / 64] >> (i % 64) & 1); q <<= 1;
        if (r >= m) { r -= m; q |= 1; }}
    return {ulng(q), r};}
void divSteps(std::mt19937_64 &rng, int count) {
    auto rand128 = [&] { return ulll(rng()) << 64 | rng(); };
    auto check = [&](ulll a, ulng n, ulll b, ulll m) {
        context = show(lll(n), lll(m), lll(a), lll(b));
        need(floorsum_detail::divStep(a, n, b, m) == slowDivStep(a, n, b, m), "divStep equals restoring division");};
    ulll top = (ulll(1) << 127) - 1;
    // Normalization shifts 1 (m >= 2^126) and 62 (2^65 < m < 2^66), m below 2^65 (the direct division), the largest quotients
    // (a = b = m - 1, n = 2^63 - 1) and low words of all ones.
    for (ulll m : {top, top - 1, ulll(1) << 126, (ulll(1) << 126) + 1, (ulll(1) << 65) + 1, (ulll(1) << 66) - 1, (ulll(1) << 64) + 1, (ulll(1) << 65) - 1, top >> 1 | 1, (ulll(1) << 127) - (ulll(1) << 64)}) {
        for (ulng n : {ulng(HI), ulng(HI) - 1, ulng(1) << 62, ulng(3)}) {
            check(m - 1, n, m - 1, m); check(m - 1, n, 0, m); check(m >> 1, n, m - 1, m); check(1, n, m - 1, m);}}
    for (int i = 0; i < count; ++i) {
        ulll m = (rand128() >> (1 + rng() % 63)) | (ulll(1) << 64), a = rand128() % m, b = rand128() % m;
        check(a, ulng(rng() >> (1 + rng() % 63)), b, m);
        // Remainders 0 to 3: the correction test must see the low numerator word.
        ulng n = ulng(rng() >> 1); ulll c = (m - slowDivStep(a, n, 0, m).second) % m + rng() % 4;
        check(a, n, c >= m ? c - m : c, m);}}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20261018;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            lng n, m, a, b, l, r;
            while (cin >> n >> m >> a >> b >> l >> r) {
                auto u = floorSumMoments<ulll>(n, m, a, b);
                auto w = floorSumMoments<mint>(n, m, a, b);
                cout << decimal(floorSum(n, m, a, b)) << ' ' << decimal(ceilSum(n, m, a, b)) << ' ' << decimal(modSum(n, m, a, b)) << ' '
                    << countModInRange(n, m, a, b, l, r) << ' ' << firstModInRange(m, a, b, l, r) << ' ';
                if (n >= 1) {
                    auto [v, x] = minOfModOfLinear(n, m, a, b); auto [V, X] = maxOfModOfLinear(n, m, a, b);
                    cout << v << ' ' << x << ' ' << V << ' ' << X << ' ';}
                else { cout << "- - - - "; }
                cout << decimal(u[0]) << ' ' << decimal(u[1]) << ' ' << decimal(u[2]) << ' ' << w[0].val() << ' ' << w[1].val() << ' ' << w[2].val() << '\n';}
            return 0;}
        else if (arg == "--oracle-wide") {
            string n, m, a, b;
            auto parse = [](const string &t) { lll v = 0; bool neg = t[0] == '-'; for (char c : t.substr(neg)) { v = v * 10 + (neg ? '0' - c : c - '0'); } return v; };
            while (cin >> n >> m >> a >> b) { cout << decimal(floorSumWide(lng(parse(n)), parse(m), parse(a), parse(b))) << '\n'; }
            return 0;}}
    if (!invalid.empty()) {
        auto f = [](lng, lng, lng) {};
        if (invalid == "floorsum-m") { floorSum(3, 0, 1, 1); }
        else if (invalid == "floorsum-n") { floorSum(-1, 3, 1, 1); }
        else if (invalid == "floorsumwide-m") { floorSumWide(3, 0, 1, 1); }
        else if (invalid == "count-range") { countModInRange(3, 5, 1, 1, 3, 2); }
        else if (invalid == "count-top") { countModInRange(3, 5, 1, 1, 0, 6); }
        else if (invalid == "moments-m") { floorSumMoments<ulll>(3, 0, 1, 1); }
        else if (invalid == "min-empty") { minOfModOfLinear(0, 5, 1, 1); }
        else if (invalid == "max-m") { maxOfModOfLinear(3, 0, 1, 1); }
        else if (invalid == "first-range") { firstModInRange(5, 1, 1, -1, 2); }
        else if (invalid == "quotient-negative") { forEachQuotientBlock(-1, f); }
        else if (invalid == "joint-negative") { forEachJointQuotientBlock({4, -1}, [](lng, lng) {}); }
        else { return 2; }
        return 0;}
    bool quick = mode == "quick", stress = mode == "stress";
    int N = quick ? 7 : 12, M = quick ? 7 : 12, A = quick ? 8 : 15;
    for (lng n = 0; n <= N; ++n) { for (lng m = 1; m <= M; ++m) { for (lng a = -A; a <= A; ++a) { for (lng b = -A; b <= A; ++b) { brute(n, m, a, b); }}}}
    cout << "PASS exhaustive small signed instances checks=" << checks << '\n';
    std::mt19937_64 rng(seed);
    vector<lng> edges{LO, LO + 1, -2, -1, 0, 1, 2, HI - 1, HI};
    for (lng a : edges) { for (lng b : edges) {
        for (lng m : {1, 2, 3, 7}) { brute(5, m, a, b); }
        for (lng m : {lng(1), lng(2), lng(1) << 32, HI - 1, HI}) { bruteLong(rng, 64, m, a, b); }}}
    cout << "PASS signed64 extremes checks=" << checks << '\n';
    int rounds = quick ? 300 : stress ? 30000 : 3000;
    for (int i = 0; i < rounds; ++i) {
        lng m = max<lng>(1, lng(rng() >> (1 + rng() % 63)));
        bruteLong(rng, lng(rng() % (quick ? 200 : 2000)) + 1, m, std::bit_cast<lng>(rng()), std::bit_cast<lng>(rng()));
        identities(rng); wide(rng);}
    cout << "PASS seeded long loops and identities rounds=" << rounds << " checks=" << checks << '\n';
    divSteps(rng, quick ? 20000 : stress ? 1000000 : 200000);
    cout << "PASS divStep against restoring division checks=" << checks << '\n';
    context = "maximal lng n and m, 2^126 coefficients, 64-bit division boundary";
    need(floorSum(HI, HI, HI - 1, HI - 1) == lll(HI) * (HI - 1) / 2, "n = m = 2^63 - 1 closed form");
    lll big = lll(1) << 126;
    need(ulll(floorSum(HI, 1, big, -big)) == ulll(big) * tri(ulng(HI)) - ulll(big) * ulng(HI), "2^126 coefficients wrap modulo 2^128");
    lll top = (lll(1) << 126) - 1 + (lll(1) << 126);
    need(floorSumWide(HI, top, top - 1, top - 1) == lll(ulll(HI) * ulng(HI - 1) / 2), "floorSumWide with m = 2^127 - 1 and n = 2^63 - 1");
    need(floorSumWide(0, top, top, top) == 0 && floorSumWide(2, 1, -top - 1, top) == top - 1, "floorSumWide empty range and extreme coefficients");
    // floor(-2^127 / 3) * 3 alone would overflow; the remainder is formed modulo 2^128.
    need(floorSumWide(2, 3, -top - 1, 0) == floorDiv(-top - 1, 3) + 0 && floorSumWide(3, top, -top - 1, -top - 1) == -2 + -3 + -4, "floorSumWide reduces extreme coefficients without overflow");
    lng p32 = lng(1) << 32;
    need(floorSum(p32, p32 - 1, p32 - 2, p32 - 2) == floorSum(p32, p32 - 1, -1, -1) + lll(tri(ulng(p32))) + p32, "a n + b crossing 2^64");
    for (lng n = 0; n <= (quick ? 300 : 3000); ++n) { quotientBrute(n); }
    for (int i = 0; i < (quick ? 20 : stress ? 1000 : 100); ++i) { quotients(lng(rng() % (quick ? 1000000000ULL : 10000000000ULL))); }
    for (lng n : {lng(999999999999), lng(1000000000000), lng(1) << 40}) { quotients(n); }
    for (int i = 0; i < (quick ? 200 : stress ? 20000 : 2000); ++i) {
        vector<lng> vs(rng() % 5);
        for (lng &v : vs) { v = lng(rng() % 400); }
        joint(vs);}
    cout << "PASS quotient blocks checks=" << checks << '\n';}
