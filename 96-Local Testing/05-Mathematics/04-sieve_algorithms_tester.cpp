#include "../../05-Mathematics/04-sieve_algorithms.hpp"

string context;
lng checks = 0;
void need(bool ok, const string &what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " expected=true actual=false input=" << context << '\n'; std::exit(1); }}
vector<pair<int,int>> trial(int a) {
    vector<pair<int,int>> factors;
    for (int p = 2; lng(p) * p <= a; ++p) {
        if (a % p != 0) { continue; }
        int exponent = 0;
        do { a /= p; ++exponent; } while (a % p == 0);
        factors.pb({p,exponent}); }
    if (a > 1) { factors.pb({a,1}); } return factors;}
void load(LinearSieve &s, std::mt19937_64 &rng) {
    array<int,5> order{0,1,2,3,4}; std::shuffle(order.begin(),order.end(),rng);
    for (int op : order) {
        if (op == 0) { s.getNumDiv(); }
        if (op == 1) { s.getSumDiv(); }
        if (op == 2) { s.getPhi(); }
        if (op == 3) { s.getMu(); }
        if (op == 4) { s.getLpf(); } }}
void factorCheck(const LinearSieve &s, int a, bool check_phi) {
    context = "n=" + std::to_string(s.n) + " factor=" + std::to_string(a);
    auto factors = trial(a);
    need(s.getPrimeFac(a) == factors, "prime powers versus trial division");
    vector<int> divisors;
    for (int d = 1; lng(d)*d <= a; ++d) {
        if (a%d) { continue; }
        divisors.pb(d); if (d != a/d) { divisors.pb(a/d); }}
    sort(divisors.begin(),divisors.end());
    auto got = s.getAllFac(a); sort(got.begin(),got.end());
    need(got == divisors, "all divisors versus sqrt enumeration");
    need(s.spf[a] == (a == 1 ? -1 : factors.front().first), "smallest prime factor");
    need(s.lpf[a] == (a == 1 ? -1 : factors.back().first), "largest prime factor");
    need(s.num_div[a] == int(divisors.size()), "divisor count");
    need(s.sum_div[a] == accumulate(divisors.begin(),divisors.end(),lng(0)), "divisor sum");
    int mu = 1;
    for (auto [p,e] : factors) { mu = e > 1 ? 0 : -mu; }
    need(s.mu[a] == mu, "Mobius trial factorization");
    if (check_phi) {
        int phi = 0;
        for (int k = 1; k <= a; ++k) { phi += gcd(k,a) == 1; }
        need(s.phi[a] == phi, "totient versus gcd enumeration"); }}
void compareTables(int n, std::mt19937_64 &rng) {
    context = "n=" + std::to_string(n);
    SieveOfErath e(n); LinearSieve s(n);
    need(s.lpf.empty() && s.num_div.empty() && s.sum_div.empty() && s.phi.empty() && s.mu.empty(), "optional tables lazy");
    load(s,rng);
    need(e.n == n && s.n == n && int(e.is_prime.size()) == n+1 && int(s.spf.size()) == n+1, "inclusive shape");
    need(!e.is_prime[0] && s.spf[0] == -1 && s.lpf[0] == -1 && s.num_div[0] == 0 && s.sum_div[0] == 0 && s.phi[0] == 0 && s.mu[0] == 0, "zero convention");
    vector<int> primes;
    for (int a = 1; a <= n; ++a) {
        auto f = trial(a); bool prime = f.size() == 1 && f.front().second == 1;
        context = "n=" + std::to_string(n) + " value=" + std::to_string(a);
        need(bool(e.is_prime[a]) == prime, "prime flag versus trial division");
        if (prime) { primes.pb(a); }
        factorCheck(s,a,a <= 300); }
    need(s.prms == primes && e.prms == primes, "sorted complete prime lists");
    vector<int> count(n+1), phi(n+1), mu(n+1);
    vector<lng> sum(n+1);
    for (int a = 1; a <= n; ++a) { phi[a] = a; }
    if (n >= 1) { mu[1] = 1; }
    for (int d = 1; d <= n; ++d) {
        for (int a = d; a <= n; a += d) { ++count[a]; sum[a] += d; }
        for (int a = 2*d; a <= n; a += d) { phi[a] -= phi[d]; mu[a] -= mu[d]; }}
    need(s.num_div == count && s.sum_div == sum, "independent divisor-multiple accumulation");
    need(s.phi == phi, "totient divisor-sum inversion");
    for (int a = 0; a <= n; ++a) { need(s.mu[a] == mu[a], "Mobius divisor-sum inversion"); }
    LinearSieve copy = s; SieveOfErath ecopy = e;
    load(s,rng); load(s,rng);
    need(s.lpf == copy.lpf && s.num_div == copy.num_div && s.sum_div == copy.sum_div && s.phi == copy.phi && s.mu == copy.mu, "repeat builders independent of order");
    copy.reset(0); ecopy.reset(0);
    need(s.n == n && e.n == n && s.prms == primes && e.prms == primes, "copy reset is independent");
    need(copy.lpf.empty() && copy.num_div.empty() && copy.sum_div.empty() && copy.phi.empty() && copy.mu.empty(), "reset clears all optional tables");
    copy = s; ecopy = e;
    auto moved = std::move(copy); auto emoved = std::move(ecopy);
    need(moved.spf == s.spf && moved.sum_div == s.sum_div && moved.lpf == s.lpf && emoved.is_prime == e.is_prime, "copy assignment and move construction");
    copy.reset(1); ecopy.reset(1); copy.getPhi();
    need(copy.phi == vector<int>({0,1}) && !ecopy.is_prime[1], "reset moved-from objects");
    s.reset(5); e.reset(5); load(s,rng);
    need(s.prms == vector<int>({2,3,5}) && e.prms == s.prms && s.lpf[4] == 2 && s.phi[5] == 4, "grow or shrink reset"); }
int main(int argc,char **argv) {
    string mode = "full", invalid; ulng seed = 20260927;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }}
    if (!invalid.empty()) {
        int bad = invalid.find("max") != string::npos ? std::numeric_limits<int>::max() : -1;
        if (invalid == "erath-negative" || invalid == "erath-max") { SieveOfErath s(bad); }
        else if (invalid == "linear-negative" || invalid == "linear-max") { LinearSieve s(bad); }
        else if (invalid == "erath-reset-negative" || invalid == "erath-reset-max") { SieveOfErath s(10); s.reset(bad); }
        else if (invalid == "linear-reset-negative" || invalid == "linear-reset-max") { LinearSieve s(10); s.reset(bad); }
        else {
            const LinearSieve s(10);
            int a = invalid.find("end") != string::npos ? 11 : invalid.find("zero") != string::npos ? 0 : -1;
            if (invalid.starts_with("primefac")) { s.getPrimeFac(a); }
            else if (invalid.starts_with("allfac")) { s.getAllFac(a); }
            else { return 2; }}
        return 0;}
    std::mt19937_64 rng(seed);
    int end = mode == "quick" ? 32 : 128;
    for (int n = 0; n <= end; ++n) { compareTables(n,rng); }
    cout << "PASS all small prefixes and reset/copy/table permutations checks=" << checks << '\n';
    int n = mode == "quick" ? 2500 : mode == "full" ? 50000 : 250000;
    compareTables(n,rng);
    cout << "PASS exhaustive trial factors, divisors and divisor-inversion tables n=" << n << " checks=" << checks << '\n';
    n = mode == "quick" ? 100000 : mode == "full" ? 1000000 : 5000000;
    SieveOfErath e(n); LinearSieve s(n); load(s,rng);
    int expected_count = n == 100000 ? 9592 : n == 1000000 ? 78498 : 348513;
    need(int(e.prms.size()) == expected_count && e.prms == s.prms, "known prime count");
    int rounds = mode == "quick" ? 100 : mode == "full" ? 1500 : 6000;
    for (int i = 0; i < rounds; ++i) { factorCheck(s,1 + int(rng()%n),false); }
    vector<int> adversarial{1,2,4,8,16,27,81,121,256,1024,2310,65536,n};
    for (int a : adversarial) { if (a <= n) { factorCheck(s,a,false); }}
    cout << "PASS prime powers, squarefree products and random cases=" << rounds << " n=" << n << " seed=" << seed << " checks=" << checks << '\n';}
