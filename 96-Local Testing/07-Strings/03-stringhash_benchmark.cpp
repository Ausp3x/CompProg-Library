#include "../../07-Strings/03-stringhash.hpp"

using Clock = std::chrono::steady_clock;
struct Query { int l, r, a, b; };
struct Sample { lng setup, query, pipeline; };
struct Result { string method; vector<Sample> samples; lng auxiliary = 0; };

lng elapsed(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();}
int direct(const string &s, Query q) {
    int k = 0, limit = min(q.r - q.l, q.b - q.a);
    while (k < limit && s[q.l + k] == s[q.a + k]) { ++k; }
    return k;}
void verify(const vector<int> &actual, const vector<int> &expected, const vector<Query> &queries,
            ulng seed, const string &shape, int n, const string &method) {
    for (int i = 0; i < int(actual.size()); ++i) {
        if (actual[i] != expected[i]) {
            auto q = queries[i];
            cerr << "FAIL seed=" << seed << " shape=" << shape << " n=" << n << " method=" << method
                 << " query=" << i << " ranges=[" << q.l << ',' << q.r << "),[" << q.a << ',' << q.b
                 << ") expected=" << expected[i] << " actual=" << actual[i] << '\n'; std::exit(1);}}}
template<int W> Sample hashed(const string &s, const vector<Query> &queries, vector<int> &answers, lng &memory) {
    auto start = Clock::now(); StringHash<W> h(s, StringHash<W>::defaultBase()); auto built = Clock::now();
    for (int i = 0; i < int(queries.size()); ++i) {
        auto q = queries[i]; answers[i] = h.lcp(h, q.l, q.r, q.a, q.b);}
    auto finished = Clock::now();
    memory = lng(sizeof(h)) + lng(h.powers.capacity() + h.prefix.capacity() + h.reversed.capacity()) * lng(sizeof(typename StringHash<W>::Word));
    return {elapsed(start, built), elapsed(built, finished), elapsed(start, finished)};}
Sample scanned(const string &s, const vector<Query> &queries, vector<int> &answers) {
    auto start = Clock::now();
    for (int i = 0; i < int(queries.size()); ++i) { answers[i] = direct(s, queries[i]); }
    lng query = elapsed(start, Clock::now()); return {0, query, query};}
lng median(vector<lng> values) { sort(values.begin(), values.end()); return values[values.size() / 2]; }

int main(int argc, char **argv) {
    ulng seed = 20260928; int warmup = 1, reps = 5;
    for (int i = 1; i + 1 < argc; i += 2) {
        string option = argv[i];
        if (option == "--seed") { seed = std::stoull(argv[i + 1]); }
        else if (option == "--warmup") { warmup = std::stoi(argv[i + 1]); }
        else if (option == "--reps") { reps = std::stoi(argv[i + 1]); }
        else { cerr << "unknown option: " << option << '\n'; return 2; }}
    if (warmup < 0 || reps < 1 || reps % 2 == 0) { cerr << "warmup>=0 and positive odd reps required\n"; return 2; }
    std::mt19937_64 rng(seed); bool first_workload = true; cout << "{\"workloads\":[";
    for (int n : {0, 32, 4096, 262144}) {
        for (const string &shape : {string("random"), string("periodic"), string("unary")}) {
            if (!n && shape != "random") { continue; }
            string s(n, 'a'), period = "abracadabra";
            for (int i = 0; i < n; ++i) {
                if (shape == "random") { s[i] = char(rng() % 256); }
                if (shape == "periodic") { s[i] = period[i % int(period.size())]; }}
            int count = n < 100 ? 4096 : n < 10000 ? 1024 : 256;
            vector<Query> queries; queries.reserve(count);
            for (int i = 0; i < count; ++i) {
                int l = n ? int(rng() % n) : 0, a = n ? int(rng() % n) : 0;
                if (i % 8 == 0) { a = l; }
                else if (shape == "periodic" && i % 2 == 0) {
                    a -= a % 11; a += l % 11; if (a >= n) { a -= 11; }}
                int r = n, b = n;
                if (i % 2) { r = l + int(rng() % (n - l + 1)); b = a + int(rng() % (n - a + 1)); }
                queries.push_back({l, r, a, b});}
            vector<int> expected(count), answers(count);
            lng lcp_sum = 0; int lcp_max = 0;
            for (int i = 0; i < count; ++i) { expected[i] = direct(s, queries[i]); lcp_sum += expected[i]; lcp_max = max(lcp_max, expected[i]); }
            array<Result, 4> results{{{"double-prime", {}, 0}, {"wrap64", {}, 0}, {"mersenne61", {}, 0}, {"direct-scan", {}, 0}}};
            for (int run = 0; run < warmup + reps; ++run) {
                for (int slot = 0; slot < 4; ++slot) {
                    int method = (run + slot) % 4; auto &result = results[method];
                    Sample sample = method == 0 ? hashed<0>(s, queries, answers, result.auxiliary)
                                  : method == 1 ? hashed<1>(s, queries, answers, result.auxiliary)
                                  : method == 2 ? hashed<2>(s, queries, answers, result.auxiliary)
                                                : scanned(s, queries, answers);
                    verify(answers, expected, queries, seed, shape, n, result.method);
                    if (run >= warmup) { result.samples.push_back(sample); }}}
            if (!first_workload) { cout << ','; } first_workload = false;
            cout << "{\"shape\":\"" << (!n ? "empty" : shape) << "\",\"n\":" << n << ",\"queries\":" << count
                 << ",\"answer_checksum\":" << lcp_sum << ",\"maximum_lcp\":" << lcp_max
                 << ",\"shared_input_payload_bytes\":" << n
                 << ",\"harness_query_and_answer_payload_bytes\":" << lng(count) * (sizeof(Query) + 2 * sizeof(int))
                 << ",\"results\":[";
            for (int i = 0; i < 4; ++i) {
                if (i) { cout << ','; } auto &result = results[i]; vector<lng> setup, query, pipeline;
                cout << "{\"method\":\"" << result.method << "\",\"algorithm_auxiliary_bytes\":" << result.auxiliary << ",\"raw_ns\":[";
                for (int j = 0; j < reps; ++j) {
                    auto sample = result.samples[j]; setup.push_back(sample.setup); query.push_back(sample.query); pipeline.push_back(sample.pipeline);
                    if (j) { cout << ','; }
                    cout << "{\"setup\":" << sample.setup << ",\"query\":" << sample.query << ",\"pipeline\":" << sample.pipeline << '}';}
                cout << "],\"median_ns\":{\"setup\":" << median(setup) << ",\"query\":" << median(query)
                     << ",\"pipeline\":" << median(pipeline) << "}}";}
            cout << "]}";}}
    cout << "]}\n";}
