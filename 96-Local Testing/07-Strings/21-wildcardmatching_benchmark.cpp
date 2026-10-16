#include "../../07-Strings/21-wildcardmatching.hpp"

using Clock = std::chrono::steady_clock;
double ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }
template<class F> double median(F f, lng expected, const string &label) {
    vector<double> times;
    for (int rep = 0; rep < 6; ++rep) {
        auto start = Clock::now();
        lng got = f();
        auto stop = Clock::now();
        if (got != expected) {
            cerr << "FAIL " << label << " expected=" << expected << " actual=" << got << '\n';
            std::exit(1);}
        if (rep) { times.push_back(ms(start, stop)); }}
    sort(times.begin(), times.end());
    return times[times.size() / 2];}
void emit(const string &section, const string &input, int n, int m, const string &variant, double t) {
    cout << "{\"section\": \"" << section << "\", \"input\": \"" << input << "\", \"n\": " << n << ", \"m\": " << m
         << ", \"variant\": \"" << variant << "\", \"median_ms\": " << t << "}" << endl;}
// The deterministic path with an explicit scan budget rate and block length (0 = default length).
lng hybrid(const string &t, const string &p, lng rate, int len = 0) {
    int n = int(t.size()), m = int(p.size());
    lng res = 0;
    auto collect = [&](int) { ++res; return false; };
    int from = wildcard_detail::scan(0, n, m, rate, [&](int i, int j) { return p[j] == '?' || t[i] == '?' || p[j] == t[i]; }, collect);
    if (from + m > n) { return res; }
    auto [tc, pc, sigma] = wildcard_detail::ranks(t, p, '?');
    wildcard_detail::Correlator cor(wildcard_detail::exactRows(pc, std::bit_width(uint(sigma))), len ? len : wildcard_detail::blockLength(n - from, m));
    cor.scan(from, n, [&](int r, int i) { return wildcard_detail::exactValue(r, tc[i]); }, collect);
    return res;}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261016;
    std::mt19937_64 rng(seed);
    auto randomText = [&](int n, int sigma) {
        string s(n, 'a');
        for (char &c : s) { c = char('a' + rng() % sigma); }
        return s;};
    int n = 2000000;
    // Scan budget: comparisons allowed per scanned start before the block convolution takes over (0 = convolution at once, 1e12 = naive only).
    for (string input : {"unary-miss", "periodic", "random4", "sparse-hits"}) {
        for (int m : {32, 256, 4096}) {
            string t, p;
            if (input == "unary-miss") { t.assign(n, 'a'); p = string(m - 1, 'a') + 'b'; }
            else if (input == "periodic") {
                t.resize(n);
                for (int i = 0; i < n; ++i) { t[i] = "ab"[i % 2]; }
                p = t.substr(0, m - 1) + "?";
                p[m / 2] = 'b';}
            else if (input == "random4") { t = randomText(n, 4); p = t.substr(n / 3, m); p[m / 2] = '?'; }
            else {
                t.assign(n, 'a');
                for (int i = 0; i < n; i += 997) { t[i] = 'b'; }
                p = string(m, '?');
                p[0] = 'b'; p[m - 1] = 'a';}
            lng expected = lng(wildcardMatchDeterministic(t, p, '?').size());
            for (lng rate : {0LL, 4LL, 8LL, 16LL, 32LL, 64LL, 1000000000000LL}) {
                emit("budget", input, n, m, rate == 1000000000000LL ? "naive" : "rate=" + std::to_string(rate), median([&] { return hybrid(t, p, rate); }, expected, "budget"));}
            emit("budget", input, n, m, "wildcardMatch", median([&] { return lng(wildcardMatch(t, p, '?', seed).size()); }, expected, "mc"));}}
    // Block spread: block length bit_ceil(c * m) against one block over the whole text.
    n = 4000000;
    string t = randomText(n, 4);
    for (int m : {64, 1024, 32768}) {
        string p = t.substr(n / 2, m);
        for (int j = 0; j < m; j += 5) { p[j] = '?'; }
        lng expected = lng(wildcardMatchDeterministic(t, p, '?').size());
        for (int c : {2, 4, 8, 16, 32}) {
            int len = int(std::bit_ceil(uint(c * m)));
            emit("spread", "random4", n, m, "c=" + std::to_string(c), median([&] { return hybrid(t, p, 0, len); }, expected, "spread"));}
        emit("spread", "random4", n, m, "single-block", median([&] { return hybrid(t, p, 0, wildcard_detail::LIMIT); }, expected, "single"));}
    // Headline: both matchers across alphabets, glob shapes and the automaton.
    for (int sigma : {2, 26}) {
        string text = randomText(n, sigma);
        for (int m : {1000, 100000}) {
            string p = text.substr(n / 4, m);
            for (int j = 0; j < m; j += 7) { p[j] = '?'; }
            lng expected = lng(wildcardMatchDeterministic(text, p, '?').size());
            emit("headline", "random" + std::to_string(sigma), n, m, "wildcardMatch", median([&] { return lng(wildcardMatch(text, p, '?', seed).size()); }, expected, "mc"));
            emit("headline", "random" + std::to_string(sigma), n, m, "wildcardMatchDeterministic", median([&] { return lng(wildcardMatchDeterministic(text, p, '?').size()); }, expected, "det"));}}
    string g = randomText(n, 2), withAny, plain;
    for (int k = 0; k < 6; ++k) {
        string piece = g.substr(size_t(k) * (n / 6) + 11, 2000);
        plain += "*" + piece;
        piece[1000] = '?';
        withAny += "*" + piece;}
    withAny += "*"; plain += "*";
    emit("glob", "planted-6x2000", n, int(withAny.size()), "globMatch with ?", median([&] { return lng(globMatch(g, withAny)); }, 1, "glob"));
    emit("glob", "planted-6x2000", n, int(plain.size()), "globMatch KMP", median([&] { return lng(globMatch(g, plain)); }, 1, "glob kmp"));
    for (int k : {64, 1024}) {
        string p = "*" + g.substr(n / 2, k) + "*";
        emit("glob", "random2", n, k, "GlobAutomaton", median([&] { GlobAutomaton a(p); bool ok = false; for (char c : g) { ok = a.step(c); } return lng(ok); }, 1, "automaton"));}}
