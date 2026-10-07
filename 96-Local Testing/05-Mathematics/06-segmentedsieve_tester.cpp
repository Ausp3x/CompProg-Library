#include "../../05-Mathematics/06-segmentedsieve.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();
constexpr SegmentedSieveMode MODES[] = {SegmentedSieveMode::Plain, SegmentedSieveMode::Odd, SegmentedSieveMode::Wheel30};

string show(lll x) {
    if (!x) { return "0"; } bool neg = x < 0; ulll n = neg ? ulll(0) - ulll(x) : ulll(x); string s;
    while (n) { s += char('0' + n % 10); n /= 10; } if (neg) { s += '-'; } reverse(s.begin(), s.end()); return s;}
void require(bool ok, const string &op, const string &want = "true", const string &got = "false") {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + op + " expected=" + want + " actual=" + got); }}
void expect(lll got, lll want, const string &op) { require(got == want, op, show(want), show(got)); }
vector<pair<lng, int>> trial(lng n) {
    vector<pair<lng, int>> out;
    for (lng p = 2; p <= n / p; ++p) {
        int e = 0; while (n % p == 0) { n /= p; ++e; } if (e) { out.push_back({p, e}); }}
    if (n > 1) { out.push_back({n, 1}); } return out;}
// Independent seven-base deterministic 64-bit Miller-Rabin; no production primality header.
ulng power(ulng a, ulng e, ulng m) {
    ulng v = 1; while (e) { if (e & 1) { v = ulll(v) * a % m; } a = ulll(a) * a % m; e >>= 1; } return v;}
bool prime(lng n) {
    if (n < 2) { return false; }
    for (int p : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) { if (n % p == 0) { return n == p; }}
    ulng d = ulng(n - 1); int s = std::countr_zero(d); d >>= s;
    for (ulng a : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
        if (!(a %= ulng(n))) { continue; } ulng x = power(a, d, ulng(n));
        if (x == 1 || x == ulng(n - 1)) { continue; } bool pass = false;
        for (int j = 1; j < s; ++j) { x = ulll(x) * x % ulng(n); if (x == ulng(n - 1)) { pass = true; break; }}
        if (!pass) { return false; }} return true;}

struct Values { lng tau = 0, phi = 0; lll sigma = 0; int mu = 0; };
Values arithmetic(lng x) {
    if (x <= 0) { return {}; }
    Values v; auto f = trial(x); v.mu = 1; v.phi = x;
    for (auto [p, e] : f) { v.phi -= v.phi / p; v.mu = e > 1 ? 0 : -v.mu; }
    // Independent divisor-pair enumeration for count and sum.
    for (lng d = 1; d <= x / d; ++d) { if (x % d == 0) {
        v.tau += d == x / d ? 1 : 2; v.sigma += d; if (d != x / d) { v.sigma += x / d; }}}
    return v;}

void checkInterval(lng l, lng r, const SegmentedSieveBase &base, int block, SegmentedSieveMode mode, bool tables) {
    context = "L=" + show(l) + " R=" + show(r) + " block=" + std::to_string(block) + " mode=" + std::to_string(int(mode));
    SegmentedSieve s(l, r, base, block, mode); vector<lng> want, streamed;
    for (lng x = l; x < r; ++x) {
        bool p = prime(x); if (p) { want.push_back(x); }
        expect(s.is_prime[size_t(x - l)], p, "prime flag x=" + show(x));}
    require(s.prms == want, "stored primes against Miller-Rabin");
    base.forEachPrime(l, r, [&](lng x) { streamed.push_back(x); }, block, mode);
    require(streamed == want, "streamed sorted primes");
    auto bases_before = base.bprms;
    streamed.clear(); base.forEachPrime(l, r, [&](lng x) { streamed.push_back(x); }, block, mode);
    require(streamed == want && base.bprms == bases_before, "repeated query leaves base intact");
    if (!tables) { return; }
    s.getTables();
    for (lng x = l; x < r; ++x) {
        size_t i = size_t(x - l); Values v = arithmetic(x);
        expect(s.num_div[i], v.tau, "tau x=" + show(x)); expect(s.sum_div[i], v.sigma, "sigma x=" + show(x));
        expect(s.phi[i], v.phi, "phi x=" + show(x)); expect(s.mu[i], v.mu, "mu x=" + show(x));
        if (x >= 1) { require(s.getPrimeFac(x) == trial(x), "single factorization x=" + show(x)); }}
    auto tau = s.num_div, phi = s.phi; auto sigma = s.sum_div; auto mu = s.mu;
    s.getMu(); s.getPhi(); s.getNumDiv(); s.getSumDiv();
    require(s.num_div == tau && s.sum_div == sigma && s.phi == phi && s.mu == mu, "individual and combined table values");
    vector<vector<pair<lng, int>>> factors(s.is_prime.size());
    s.forEachFactor([&](lng i, lng p, int e) { factors[i].push_back({p, e}); });
    for (lng x = l; x < r; ++x) { require(factors[size_t(x - l)] == trial(max(lng(1), x)), "streamed factors x=" + show(x)); }
    if (l >= 1 || l == r) { require(s.getPrimeFac() == factors, "whole positive interval factorization"); }}

void exhaustive(const string &mode) {
    int lower = mode == "quick" ? -4 : mode == "stress" ? -8 : -6;
    int upper = mode == "quick" ? 24 : mode == "stress" ? 96 : 64;
    vector<int> blocks = mode == "quick" ? vector<int>{1, 7, 30} : vector<int>{1, 2, 7, 30, 31, 64};
    SegmentedSieveBase base(upper);
    for (int l = lower; l <= upper; ++l) { for (int r = l; r <= upper; ++r) {
        for (auto variant : MODES) { for (int block : blocks) { checkInterval(l, r, base, block, variant, block == blocks.back()); }}}}
    for (int n = 0; n <= upper * upper; ++n) {
        SegmentedSieveBase b(n, 7); vector<lng> wanted;
        for (lng p = 2; p * p <= n; ++p) { if (prime(p)) { wanted.push_back(p); }}
        require(b.bprms == wanted, "base prime completeness n=" + show(n));
        expect(b.limit, segmented_sieve_detail::root(n), "base limit metadata");}
    std::cout << "PASS exhaustive intervals " << lower << ".." << upper << ", all modes/block alignments and bases\n";}

void stateAndBoundaries() {
    context = "state and endpoint boundaries";
    SegmentedSieveBase base(10000, 31); auto bases = base.bprms;
    SegmentedSieve closed = SegmentedSieve::fromClosed(0, 97, base), fresh(0, 98);
    require(closed.prms == fresh.prms && closed.is_prime == fresh.is_prime, "closed adapter includes right prime");
    require(SegmentedSieve::fromClosed(97, 97).prms == vector<lng>{97}, "closed singleton adapter");
    closed.getTables(); auto copy = closed; SegmentedSieve moved = std::move(copy);
    require(moved.prms == closed.prms && moved.sum_div == closed.sum_div, "copy and move populated tables");
    closed.reset(-7, 9, base, 2, SegmentedSieveMode::Wheel30);
    require(closed.num_div.empty() && closed.sum_div.empty() && closed.phi.empty() && closed.mu.empty() && closed.rem.empty(), "reset clears optional state");
    closed.getTables(); expect(closed.num_div[8], 1, "one tau"); expect(closed.sum_div[8], 1, "one sigma");
    expect(closed.phi[8], 1, "one phi"); expect(closed.mu[8], 1, "one mu");
    for (int i = 0; i < 8; ++i) { require(!closed.num_div[i] && !closed.sum_div[i] && !closed.phi[i] && !closed.mu[i], "nonpositive table sentinel"); }
    vector<lng> needed;
    for (lng p : bases) { if (p * p <= closed.r - 1) { needed.push_back(p); }}
    base.reset(10); require(closed.bprms == needed, "materialized object owns the needed base prefix");
    require(base.bprms == vector<lng>{2,3}, "base reset truncates stale primes"); base.reset(0); require(base.bprms.empty(), "zero base domain");
    for (auto mode : MODES) {
        checkInterval(LO, LO + 8, base, 1, mode, true);
        checkInterval(HI, HI, base, 1, mode, true);
        lng count = 0; base.forEachPrime(LO, 2, [&](lng) { ++count; }, 1, mode); expect(count, 0, "huge negative streaming range constant work");}
    for (lng n : {lng(0),lng(1),lng(2),lng(3),lng(4),lng(8),lng(9),lng(10),HI - 1,HI}) {
        lng q = segmented_sieve_detail::root(n);
        require(lll(q) * q <= n && lll(q + 1) * (q + 1) > n, "full-domain root correction");}
    for (lng l : {lng(2),HI - 10,HI - 1,HI}) { for (lng p : {lng(2),lng(3),lng(3037000499)}) {
        lll m = segmented_sieve_detail::firstMultiple(l, p);
        require(m >= l && m % p == 0 && m - p < l, "wide first-multiple arithmetic");}}
    std::cout << "PASS reset/copy/lifetime, explicit closed migration, signed extremes and wide arithmetic\n";}

void randomCases(int count) {
    std::mt19937_64 rng(seed); SegmentedSieveBase base(2000000);
    for (int t = 0; t < count; ++t) {
        lng l = lng(rng() % 1990000) - 30, r = l + lng(rng() % 70); int block = 1 + int(rng() % 130);
        for (auto mode : MODES) { checkInterval(l, r, base, block, mode, t % 13 == 0); }}
    std::cout << "PASS seeded independent prime/factor/table cases=" << count << '\n';}

void highAndCounts(bool quick) {
    SegmentedSieveBase high(1000000001000LL); vector<pair<lng, lng>> intervals{{999999999980LL,1000000000010LL},
        {999966000250LL,999966000290LL}, {999999900LL,1000000020LL}};
    for (auto [l, r] : intervals) { for (auto mode : MODES) {
        checkInterval(l, r, high, 7, mode, false); checkInterval(l, r, high, 31, mode, false);}}
    // Trial and divisor-pair oracles for a short interval near 10^12.
    for (auto mode : MODES) { checkInterval(1000000000000LL,1000000000003LL,high,2,mode,true); }
    lng count = 0; SegmentedSieveBase base(quick ? 100000 : 1000000);
    for (auto mode : MODES) {
        count = 0; base.forEachPrime(0, quick ? 100001 : 1000001, [&](lng) { ++count; }, 32768, mode);
        expect(count, quick ? 9592 : 78498, "known pi count");}
    std::cout << "PASS high endpoints, prime-square boundaries, interval factorization and known prime counts\n";}

void invalid(const string &probe) {
    SegmentedSieveBase base(10); auto f = [](lng) {};
    if (probe == "base-negative") { SegmentedSieveBase b(-1); }
    else if (probe == "base-block") { SegmentedSieveBase b(10,0); }
    else if (probe == "stream-order") { base.forEachPrime(2,1,f); }
    else if (probe == "stream-block") { base.forEachPrime(0,10,f,0); }
    else if (probe == "stream-mode") { base.forEachPrime(0,10,f,1,SegmentedSieveMode(99)); }
    else if (probe == "base-short") { base.forEachPrime(0,12,f); }
    else if (probe == "constructor-order") { SegmentedSieve s(1,0); }
    else if (probe == "closed-order") { auto s = SegmentedSieve::fromClosed(1,0); }
    else if (probe == "closed-maximum") { auto s = SegmentedSieve::fromClosed(HI,HI); }
    else if (probe == "closed-base-maximum") { auto s = SegmentedSieve::fromClosed(HI,HI,base); }
    else if (probe == "material-size") { SegmentedSieve s(LO,HI,base); }
    else if (probe == "factor-zero") { SegmentedSieve s(0,3); s.getPrimeFac(0); }
    else if (probe == "factor-left") { SegmentedSieve s(5,8); s.getPrimeFac(4); }
    else if (probe == "factor-right") { SegmentedSieve s(5,8); s.getPrimeFac(8); }
    else if (probe == "factor-interval-negative") { SegmentedSieve s(-1,3); s.getPrimeFac(); }
    else if (probe == "reset-order") { SegmentedSieve s(0,3); s.reset(4,2,base); }
    else { throw std::runtime_error("unknown assertion probe=" + probe); }}

int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 1; }
            else { throw std::runtime_error("unknown argument=" + arg); }}
        // Validate the independent primality oracle against brute force first.
        for (int n = -3; n <= 10000; ++n) { auto f = trial(max(1,n)); require(prime(n) == (n > 1 && f == vector<pair<lng,int>>{{n,1}}), "Miller-Rabin oracle agrees with trial division"); }
        exhaustive(mode); stateAndBoundaries(); randomCases(mode == "quick" ? 50 : mode == "stress" ? 3000 : 500); highAndCounts(mode == "quick");
        std::cout << "PASS 06-segmentedsieve mode=" << mode << " seed=" << seed << " checks=" << checks << '\n';}
     catch (const std::exception &e) {
        std::cerr << "FAIL 06-segmentedsieve mode=" << mode << " seed=" << seed << " " << e.what() << '\n'; return 1;}}
