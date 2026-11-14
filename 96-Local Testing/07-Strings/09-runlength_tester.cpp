#include "../../07-Strings/09-runlength.hpp"

string mode, context;
ulng seed = 20260928, cases = 0;
std::mt19937_64 rng;

template<class T> string show(const T &x) { std::ostringstream out; out << +x; return out.str(); }
string show(const string &s);
template<class T, class U> string show(const pair<T, U> &p);
template<class T> string show(const vector<T> &a);
template<class T, class U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
template<class T> string show(const vector<T> &a) {
    string out = "[";
    for (int i = 0; i < int(a.size()); ++i) { out += (i ? "," : "") + show(a[i]); }
    return out + "]";}
string show(const string &s) { return show(vector<unsigned char>(s.begin(), s.end())); }
string show(string_view s) { return show(vector<unsigned char>(s.begin(), s.end())); }
template<class T, class U> void check(const T &actual, const U &expected, string_view operation) {
    ++cases;
    if (actual != expected) {
        cerr << "FAIL seed=" << seed << " mode=" << mode << " operation=" << operation
             << " smallest-known-reproducer=" << context << " expected=" << show(expected)
             << " actual=" << show(actual) << '\n'; std::exit(1);}}
int limit(int a, int b, int c) { return mode == "quick" ? a : mode == "full" ? b : c; }

template<class S> vector<int> boundaryOracle(const S &s) {
    vector<int> boundaries{0}; int n = int(s.size());
    for (int i = 1; i < n; ++i) { if (s[i] != s[i - 1]) { boundaries.push_back(i); } }
    if (n) { boundaries.push_back(n); }
    return boundaries;}
template<class S> vector<pair<typename S::value_type, lng>> runsOracle(const S &s) {
    vector<pair<typename S::value_type, lng>> res; auto boundaries = boundaryOracle(s);
    for (int i = 1; i < int(boundaries.size()); ++i) { res.emplace_back(s[boundaries[i - 1]], boundaries[i] - boundaries[i - 1]); }
    return res;}

template<class S> void testSequence(const S &s) {
    using T = typename S::value_type;
    context = "s=" + show(s); auto expected = runsOracle(s);
    auto boundaries = boundaryOracle(s); int n = int(s.size());
    lng largest = 0;
    for (auto [x, count] : expected) { largest = max(largest, count); }
    vector<pair<T, lng>> runs;
    check(runLengthEncode(s, runs), true, "encode-status"); check(runs, expected, "encode-boundary-oracle");
    check(runLengthEncode(s, runs, largest), true, "encode-exact-cap");
    if (largest) {
        check(runLengthEncode(s, runs, largest - 1), false, "encode-cap-reject"); check(runs.empty(), true, "encode-failure-clear");}
    check(runLengthEncode(s, runs), true, "encode-repeated-call");
    lng total = -1; check(runLengthSize(runs, total), true, "size-status"); check(total, n, "size-total");
    std::conditional_t<std::is_same_v<S, string_view>, string, S> decoded;
    check(runLengthDecode(runs, decoded), true, "decode-status"); check(decoded, s, "roundtrip");
    check(runLengthDecode(runs, decoded, n), true, "decode-exact-cap"); check(decoded, s, "decode-exact-cap-value");
    if (n) {
        check(runLengthDecode(runs, decoded, n - 1), false, "decode-cap-reject"); check(decoded.empty(), true, "decode-failure-clear");}
    vector<pair<lng, lng>> spans, expected_spans;
    for (int i = 1; i < int(boundaries.size()); ++i) { expected_spans.emplace_back(boundaries[i - 1], boundaries[i]); }
    check(runLengthSpans(runs, spans), true, "spans-status"); check(spans, expected_spans, "spans-boundary-oracle");}

void testEncoding(const vector<pair<int, lng>> &runs, lng cap) {
    context = "runs=" + show(runs) + " cap=" + show(cap); lll sum = 0; bool ok = true;
    for (auto [x, count] : runs) { ok &= count > 0; sum += count; }
    ok &= sum <= cap;
    lng total = 77; check(runLengthSize(runs, total, cap), ok, "size-128bit-status");
    check(total, ok ? lng(sum) : 0, "size-128bit-result");
    vector<pair<lng, lng>> expected, spans{{9, 9}}; lng start = 0;
    if (ok) { for (auto [x, count] : runs) { expected.emplace_back(start, start + count); start += count; } }
    check(runLengthSpans(runs, spans, cap), ok, "spans-limit-status"); check(spans, expected, "spans-limit-result");
    vector<pair<lng, lng>> alias; for (auto [x, count] : runs) { alias.emplace_back(x, count); }
    check(runLengthSpans(alias, alias, cap), ok, "spans-alias-status"); check(alias, expected, "spans-alias-result");
    int small_cap = int(min(cap, lng(40))); vector<int> expanded, decoded{9};
    bool dec_ok = ok && sum <= small_cap;
    if (dec_ok) { for (auto [x, count] : runs) { for (lng i = 0; i < count; ++i) { expanded.push_back(x); } } }
    check(runLengthDecode(runs, decoded, small_cap), dec_ok, "decode-direct-status"); check(decoded, expanded, "decode-direct-expansion");
    if (!runs.empty()) {
        auto size_alias = runs; check(runLengthSize(size_alias, size_alias[0].second, cap), ok, "size-alias-status");
        check(size_alias[0].second, ok ? lng(sum) : 0, "size-alias-result");}}

// Replays ops (count >= 0 pushes x, count < 0 pops -count) on the encoder and a plain vector oracle.
template<class T> void testEncoder(const vector<pair<T, lng>> &ops) {
    context = "encoder-ops=" + show(ops); RunLengthEncoder<T> enc; vector<T> naive;
    for (auto [x, count] : ops) {
        if (count >= 0) { enc.pushBack(x, count); naive.insert(naive.end(), size_t(count), x); }
        else { enc.popBack(-count); naive.resize(naive.size() - size_t(-count)); }
        check(enc.length(), lng(naive.size()), "encoder-length");
        check(enc.runs, runsOracle(naive), "encoder-maximal-runs");}
    vector<T> decoded;
    check(runLengthDecode(enc.runs, decoded), true, "encoder-decode-status"); check(decoded, naive, "encoder-decode");
    RunLengthEncoder<T> built(naive); RunLengthEncoder deduced(naive);
    check(built.runs, runsOracle(naive), "encoder-from-sequence-runs"); check(built.length(), lng(naive.size()), "encoder-from-sequence-length");
    check(deduced.runs, runsOracle(naive), "encoder-deduced-runs"); check(deduced.length(), lng(naive.size()), "encoder-deduced-length");
    enc = {}; check(enc.length(), 0LL, "encoder-clear-length"); check(enc.runs.empty(), true, "encoder-clear-runs");}
template<class T> void testEncoderDefaults(T a, T b) {
    context = "encoder-defaults a=" + show(a) + " b=" + show(b); RunLengthEncoder<T> enc;
    check(enc.length(), 0LL, "encoder-empty-length"); check(enc.runs.empty(), true, "encoder-empty-runs");
    enc.popBack(0); enc.pushBack(a, 0); check(enc.runs.empty(), true, "encoder-zero-ops");
    enc.pushBack(a); enc.pushBack(a); enc.pushBack(b);
    check(enc.runs, vector<pair<T, lng>>{{a, 2}, {b, 1}}, "encoder-default-push");
    enc.popBack(0); check(enc.runs, vector<pair<T, lng>>{{a, 2}, {b, 1}}, "encoder-pop-zero");
    enc.popBack(); check(enc.runs, vector<pair<T, lng>>{{a, 2}}, "encoder-default-pop");
    enc.popBack(); enc.pushBack(b, 3); check(enc.runs, vector<pair<T, lng>>{{a, 1}, {b, 3}}, "encoder-pop-then-push");
    check(enc.length(), 4LL, "encoder-length-after");
    RunLengthEncoder<T> built(vector<T>{a, a, b});
    check(built.runs, vector<pair<T, lng>>{{a, 2}, {b, 1}}, "encoder-from-sequence-defaults"); check(built.length(), 3LL, "encoder-from-sequence-defaults-length");
    RunLengthEncoder<T> self; self.pushBack(a); self.pushBack(b); self.runs.shrink_to_fit();
    self.pushBack(self.runs[0].first); self.pushBack(self.runs.back().first, self.runs[1].second);
    check(self.runs, vector<pair<T, lng>>{{a, 1}, {b, 1}, {a, 2}}, "encoder-push-aliases-runs"); check(self.length(), 4LL, "encoder-push-aliases-length");}
void testSpecialSymbols() {
    context = "encoder-from-string \"aab\\0\\0\""; string text("aab\0\0", 5);
    RunLengthEncoder from_string(text); RunLengthEncoder<char> from_view(string_view{text});
    vector<pair<char, lng>> expected{{'a', 2}, {'b', 1}, {'\0', 2}};
    check(from_string.runs, expected, "encoder-from-string"); check(from_string.length(), 5LL, "encoder-from-string-length");
    check(from_view.runs, expected, "encoder-from-string-view");
    context = "floating symbols [0.0,-0.0,NaN,NaN,1.5]"; vector<pair<double, lng>> runs;
    double nan = std::numeric_limits<double>::quiet_NaN();
    check(runLengthEncode(vector<double>{0.0, -0.0, nan, nan, 1.5}, runs), true, "float-encode-status");
    check(int(runs.size()), 4, "float-nan-separate-runs"); check(runs[0].second, 2LL, "float-signed-zero-merged");
    check(std::signbit(runs[0].first), false, "float-first-zero-kept");
    check(std::isnan(runs[1].first) && std::isnan(runs[2].first) && runs[1].second == 1 && runs[2].second == 1, true, "float-nan-runs");
    check(runs[3], pair<double, lng>{1.5, 1}, "float-last-run");
    vector<double> decoded; check(runLengthDecode(runs, decoded), true, "float-decode-status");
    check(std::signbit(decoded[1]), false, "float-decode-signed-zero");}
void testEncoderWide() {
    context = "encoder-int64-counts"; RunLengthEncoder<int> enc;
    enc.pushBack(1, LLONG_MAX - 1); enc.pushBack(1, 1);
    check(enc.length(), LLONG_MAX, "encoder-max-length");
    check(enc.runs, vector<pair<int, lng>>{{1, LLONG_MAX}}, "encoder-max-run");
    enc.popBack(LLONG_MAX - 2); check(enc.runs, vector<pair<int, lng>>{{1, 2}}, "encoder-max-pop");
    enc.pushBack(2, LLONG_MAX - 3); enc.pushBack(3, 1);
    check(enc.length(), LLONG_MAX, "encoder-max-length-mixed");
    enc.popBack(LLONG_MAX - 1); check(enc.runs, vector<pair<int, lng>>{{1, 1}}, "encoder-multi-run-pop");
    enc.popBack(1); check(enc.runs.empty(), true, "encoder-pop-all"); check(enc.length(), 0LL, "encoder-pop-all-length");}
void testEncoderLarge(int n) {
    context = "encoder-large n=" + show(n); RunLengthEncoder<int> enc;
    for (int i = 0; i < n; ++i) { enc.pushBack(i / 3 % 2); }
    check(enc.length(), lng(n), "encoder-large-length"); check(int(enc.runs.size()), (n + 2) / 3, "encoder-large-runs");
    for (int i = n; i > 0; --i) { enc.popBack(); }
    check(enc.runs.empty(), true, "encoder-large-pop-all");
    for (int i = 0; i < n; ++i) { enc.pushBack(i % 2, 2); }
    enc.popBack(2 * lng(n)); check(enc.runs.empty(), true, "encoder-large-pop-bulk");}

// Reports a size without storing it, so the INT_MAX source bound is tested without the memory.
struct ReportedSizeSequence {
    using value_type = int;
    size_t n;
    vector<int> values;
    size_t size() const { return n; }
    auto begin() const { return values.begin(); }
    auto end() const { return values.end(); }
};
template<class T> struct TinyAllocator {
    using value_type = T;
    TinyAllocator() = default;
    template<class U> TinyAllocator(const TinyAllocator<U> &) {}
    T *allocate(size_t n) { return std::allocator<T>{}.allocate(n); }
    void deallocate(T *p, size_t n) { std::allocator<T>{}.deallocate(p, n); }
    size_t max_size() const { return 3; }
    bool operator==(const TinyAllocator &) const = default;
};

int main(int argc, char **argv) {
    if (argc == 3 && string(argv[1]) == "--invalid") {
        string probe = argv[2]; vector<pair<int, lng>> runs; vector<int> out; lng size;
        if (probe == "source-size") { runLengthEncode(ReportedSizeSequence{size_t(INT_MAX) + 1, {}}, runs); }
        if (probe == "count-cap") { runLengthEncode(vector<int>{}, runs, -1); }
        if (probe == "size-cap") { runLengthSize(runs, size, -1); }
        if (probe == "decode-cap") { runLengthDecode(runs, out, -1); }
        if (probe == "span-cap") { vector<pair<lng, lng>> spans; runLengthSpans(runs, spans, -1); }
        RunLengthEncoder<int> enc; enc.pushBack(1, 2);
        if (probe == "push-count") { enc.pushBack(1, -1); }
        if (probe == "push-overflow") { enc.pushBack(2, LLONG_MAX - 1); }
        if (probe == "pop-count") { enc.popBack(-1); }
        if (probe == "pop-length") { enc.popBack(3); }
        if (probe == "construct-size") { RunLengthEncoder<int> built(ReportedSizeSequence{size_t(INT_MAX) + 1, {}}); }
        return 3;}
    for (int i = 1; i + 1 < argc; i += 2) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    rng.seed(seed); vector<int> s;
    auto words = [&](auto &&self, int left) -> void {
        testSequence(s); if (!left) { return; }
        for (int c = -1; c <= 1; ++c) { s.push_back(c); self(self, left - 1); s.pop_back(); }};
    words(words, limit(5, 8, 10));
    for (int rep = 0; rep < limit(300, 4000, 20000); ++rep) {
        s.resize(rng() % 300);
        for (auto &x : s) { x = int(rng() % 9) - 4; }
        testSequence(s);
        vector<string> named; vector<pair<int, int>> pairs;
        for (int x : s) { named.push_back(string(size_t(x + 4) / 3, 'a')); pairs.emplace_back(x / 3, x % 2); }
        testSequence(named); testSequence(pairs);}
    string bytes;
    for (int c = 0; c < 256; ++c) { bytes.append(c % 5 + 1, char(c)); }
    testSequence(bytes); testSequence(string_view(bytes));
    testSequence(vector<unsigned char>{0, 0, 255, 255, 0});
    testSequence(vector<int>{INT_MIN, INT_MIN, INT_MAX, 0, -1});
    testSequence(vector<lng>{LLONG_MIN, LLONG_MIN, LLONG_MAX, LLONG_MAX, 0});
    testSequence(vector<ulng>{0, 0, ULLONG_MAX, ULLONG_MAX, 1ULL << 63});
    testSequence(vector<bool>{false, false, true, true, false});
    testSequence(vector<string>{"", "", "ab", "ab", "a", ""}); testSequence(vector<pair<int, int>>{{1, 2}, {1, 2}, {2, 1}});
    vector<pair<int, lng>> runs;
    auto encodings = [&](auto &&self, int left) -> void {
        for (int cap = 0; cap <= 10; ++cap) { testEncoding(runs, cap); }
        if (!left) { return; }
        for (int x = 0; x <= 1; ++x) { for (lng count = -1; count <= 3; ++count) {
            runs.emplace_back(x, count); self(self, left - 1); runs.pop_back();}}};
    encodings(encodings, limit(2, 3, 4));
    for (auto r : vector<vector<pair<int, lng>>>{{{1, LLONG_MAX}}, {{1, LLONG_MAX}, {1, 1}},
            {{1, LLONG_MAX - 1}, {2, 1}}, {{1, LLONG_MIN}}, {{1, 0}}, {{1, -1}}, {{1, 3}, {1, 4}}}) {
        testEncoding(r, LLONG_MAX); testEncoding(r, LLONG_MAX - 1);}
    for (int rep = 0; rep < limit(100, 2000, 10000); ++rep) {
        runs.clear(); int n = int(rng() % 10);
        for (int i = 0; i < n; ++i) { runs.emplace_back(int(rng() % 3), std::bit_cast<lng>(rng())); }
        testEncoding(runs, LLONG_MAX);}
    context = "output-capacity=3 runs=[(1,4)]"; vector<int, TinyAllocator<int>> tiny;
    check(runLengthDecode(vector<pair<int, lng>>{{1, 4}}, tiny), false, "decode-container-capacity-reject");
    check(tiny.empty(), true, "decode-container-capacity-clear");
    context = "output-capacity=3 runs=[(1,3)]";
    check(runLengthDecode(vector<pair<int, lng>>{{1, 3}}, tiny), true, "decode-container-capacity-exact");
    check(int(tiny.size()), 3, "decode-container-capacity-size");
    context = "reported-size=INT_MAX values=[7,7]"; vector<pair<int, lng>> bound_runs;
    check(runLengthEncode(ReportedSizeSequence{size_t(INT_MAX), {7, 7}}, bound_runs), true, "encode-source-size-bound");
    check(bound_runs, vector<pair<int, lng>>{{7, 2}}, "encode-source-size-bound-runs");
    vector<int> large(limit(10000, 200000, 1000000), INT_MIN); testSequence(large);
    for (int i = 0; i < int(large.size()); ++i) { large[i] = i % 2; }
    testSequence(large);
    vector<pair<int, lng>> ops;
    auto sequences = [&](auto &&self, int left, lng length) -> void {
        testEncoder(ops); if (!left) { return; }
        for (int x = 0; x <= 1; ++x) { for (lng count = 0; count <= 2; ++count) {
            ops.emplace_back(x, count); self(self, left - 1, length + count); ops.pop_back();}}
        for (lng count = 1; count <= length; ++count) { ops.emplace_back(0, -count); self(self, left - 1, length - count); ops.pop_back(); }};
    sequences(sequences, limit(4, 6, 7), 0);
    for (int rep = 0; rep < limit(300, 4000, 20000); ++rep) {
        ops.clear(); lng length = 0;
        for (int i = int(rng() % 40); i > 0; --i) {
            if (length && rng() % 3 == 0) { lng count = lng(rng() % ulng(length)) + 1; ops.emplace_back(0, -count); length -= count; }
            else { lng count = lng(rng() % 5); ops.emplace_back(int(rng() % 3) - 1, count); length += count; }}
        testEncoder(ops);
        vector<pair<string, lng>> named;
        for (auto [x, count] : ops) { named.emplace_back(string(size_t(x + 1), 'x'), count); }
        testEncoder(named);}
    testEncoder(vector<pair<unsigned char, lng>>{{0, 2}, {255, 1}, {0, 1}, {0, -2}, {255, 3}});
    testEncoder(vector<pair<lng, lng>>{{LLONG_MIN, 2}, {LLONG_MAX, 2}, {LLONG_MAX, -3}, {LLONG_MIN, 1}});
    testEncoder(vector<pair<ulng, lng>>{{ULLONG_MAX, 1}, {0, 1}, {ULLONG_MAX, 1}, {ULLONG_MAX, -1}});
    testEncoderDefaults<int>(INT_MIN, INT_MAX); testEncoderDefaults<char>('\0', char(-1));
    testEncoderDefaults<bool>(false, true); testEncoderDefaults<ulng>(0, ULLONG_MAX);
    testEncoderDefaults<string>("", "ab"); testEncoderDefaults<pair<int, int>>({0, 1}, {1, 0}); testSpecialSymbols();
    testEncoderWide(); testEncoderLarge(limit(10000, 200000, 1000000));
    cout << "PASS runlength seed=" << seed << " mode=" << mode << " checks=" << cases << '\n';}
