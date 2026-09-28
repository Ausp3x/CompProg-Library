#include "../../07-Strings/09-runlength.hpp"

string mode, context;
ulng seed = 20260928, cases = 0;
std::mt19937_64 rng;

template<class T> string show(const T &x) { std::ostringstream out; out << +x; return out.str(); }
template<class T, class U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
template<class T> string show(const vector<T> &a) {
    string out = "[";
    for (int i = 0; i < int(a.size()); ++i) { out += (i ? "," : "") + show(a[i]); }
    return out + "]";
}
string show(const string &s) { return show(vector<unsigned char>(s.begin(), s.end())); }
string show(string_view s) { return show(vector<unsigned char>(s.begin(), s.end())); }
template<class T, class U> void check(const T &actual, const U &expected, string_view operation) {
    ++cases;
    if (actual != expected) {
        cerr << "FAIL seed=" << seed << " mode=" << mode << " operation=" << operation
             << " smallest-known-reproducer=" << context << " expected=" << show(expected)
             << " actual=" << show(actual) << '\n'; std::exit(1); }
}
int limit(int a, int b, int c) { return mode == "quick" ? a : mode == "full" ? b : c; }

template<class S> void testSequence(const S &s) {
    using T = typename S::value_type;
    context = "s=" + show(s); vector<pair<T, lng>> expected;
    vector<int> boundaries{0}; int n = int(s.size());
    for (int i = 1; i < n; ++i) { if (s[i] != s[i - 1]) { boundaries.push_back(i); } }
    if (n) { boundaries.push_back(n); }
    lng largest = 0;
    for (int i = 1; i < int(boundaries.size()); ++i) {
        lng count = boundaries[i] - boundaries[i - 1]; largest = max(largest, count);
        expected.emplace_back(s[boundaries[i - 1]], count); }
    vector<pair<T, lng>> runs;
    check(runLengthEncode(s, runs), true, "encode-status"); check(runs, expected, "encode-boundary-oracle");
    check(runLengthEncode(s, runs, largest), true, "encode-exact-cap");
    if (largest) {
        check(runLengthEncode(s, runs, largest - 1), false, "encode-cap-reject"); check(runs.empty(), true, "encode-failure-clear"); }
    check(runLengthEncode(s, runs), true, "encode-repeated-call");
    lng total = -1; check(runLengthSize(runs, total), true, "size-status"); check(total, n, "size-total");
    std::conditional_t<std::is_same_v<S, string_view>, string, S> decoded;
    check(runLengthDecode(runs, decoded), true, "decode-status"); check(decoded, s, "roundtrip");
    check(runLengthDecode(runs, decoded, n), true, "decode-exact-cap"); check(decoded, s, "decode-exact-cap-value");
    if (n) {
        check(runLengthDecode(runs, decoded, n - 1), false, "decode-cap-reject"); check(decoded.empty(), true, "decode-failure-clear"); }
    vector<pair<lng, lng>> spans, expected_spans;
    for (int i = 1; i < int(boundaries.size()); ++i) { expected_spans.emplace_back(boundaries[i - 1], boundaries[i]); }
    check(runLengthSpans(runs, spans), true, "spans-status"); check(spans, expected_spans, "spans-boundary-oracle");
}

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
        check(size_alias[0].second, ok ? lng(sum) : 0, "size-alias-result"); }
}

struct OversizedSequence {
    using value_type = int;
    vector<int> values;
    size_t size() const { return size_t(INT_MAX) + 1; }
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
        if (probe == "source-size") { runLengthEncode(OversizedSequence{}, runs); }
        if (probe == "count-cap") { runLengthEncode(vector<int>{}, runs, -1); }
        if (probe == "size-cap") { runLengthSize(runs, size, -1); }
        if (probe == "decode-cap") { runLengthDecode(runs, out, -1); }
        if (probe == "span-cap") { vector<pair<lng, lng>> spans; runLengthSpans(runs, spans, -1); }
        return 3; }
    for (int i = 1; i + 1 < argc; i += 2) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); } }
    rng.seed(seed); vector<int> s;
    auto words = [&](auto &&self, int left) -> void {
        testSequence(s); if (!left) { return; }
        for (int c = -1; c <= 1; ++c) { s.push_back(c); self(self, left - 1); s.pop_back(); }
    };
    words(words, limit(5, 8, 10));
    for (int rep = 0; rep < limit(300, 4000, 20000); ++rep) {
        s.resize(rng() % 300);
        for (auto &x : s) { x = int(rng() % 9) - 4; }
        testSequence(s); }
    string bytes;
    for (int c = 0; c < 256; ++c) { bytes.append(c % 5 + 1, char(c)); }
    testSequence(bytes); testSequence(string_view(bytes));
    testSequence(vector<unsigned char>{0, 0, 255, 255, 0});
    testSequence(vector<int>{INT_MIN, INT_MIN, INT_MAX, 0, -1});
    testSequence(vector<lng>{LLONG_MIN, LLONG_MIN, LLONG_MAX, LLONG_MAX, 0});
    testSequence(vector<ulng>{0, 0, ULLONG_MAX, ULLONG_MAX, 1ULL << 63});
    testSequence(vector<bool>{false, false, true, true, false});
    vector<pair<int, lng>> runs;
    auto encodings = [&](auto &&self, int left) -> void {
        for (int cap = 0; cap <= 10; ++cap) { testEncoding(runs, cap); }
        if (!left) { return; }
        for (int x = 0; x <= 1; ++x) { for (lng count = -1; count <= 3; ++count) {
            runs.emplace_back(x, count); self(self, left - 1); runs.pop_back(); } }
    };
    encodings(encodings, limit(2, 3, 4));
    for (auto r : vector<vector<pair<int, lng>>>{{{1, LLONG_MAX}}, {{1, LLONG_MAX}, {1, 1}},
            {{1, LLONG_MAX - 1}, {2, 1}}, {{1, LLONG_MIN}}, {{1, 0}}, {{1, -1}}, {{1, 3}, {1, 4}}}) {
        testEncoding(r, LLONG_MAX); testEncoding(r, LLONG_MAX - 1); }
    for (int rep = 0; rep < limit(100, 2000, 10000); ++rep) {
        runs.clear(); int n = int(rng() % 10);
        for (int i = 0; i < n; ++i) { runs.emplace_back(int(rng() % 3), std::bit_cast<lng>(rng())); }
        testEncoding(runs, LLONG_MAX); }
    context = "output-capacity=3 runs=[(1,4)]"; vector<int, TinyAllocator<int>> tiny;
    check(runLengthDecode(vector<pair<int, lng>>{{1, 4}}, tiny), false, "decode-container-capacity-reject");
    check(tiny.empty(), true, "decode-container-capacity-clear");
    context = "output-capacity=3 runs=[(1,3)]";
    check(runLengthDecode(vector<pair<int, lng>>{{1, 3}}, tiny), true, "decode-container-capacity-exact");
    check(int(tiny.size()), 3, "decode-container-capacity-size");
    vector<int> large(limit(10000, 200000, 1000000), INT_MIN); testSequence(large);
    for (int i = 0; i < int(large.size()); ++i) { large[i] = i % 2; }
    testSequence(large);
    cout << "PASS runlength seed=" << seed << " mode=" << mode << " checks=" << cases << '\n';
}
