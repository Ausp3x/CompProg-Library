#include "../../06-Miscellaneous/13-knapsack.hpp"

constexpr ulng SEED = 20260928;
string context;
struct Output {
    vector<lll> value;
    vector<char> reachable;
    vector<int> witness;
    int target = 0;
};

// Independent ordinary recurrence over every legal multiplicity, retaining
// previous item rows. This is deliberately not a binary-splitting reference.
Output direct(int capacity, const vector<KnapsackItem> &items, bool trace) {
    vector<lll> value(capacity + 1); vector<char> reachable(capacity + 1); reachable[0] = true;
    vector<vector<int>> take;
    for (const auto &item : items) {
        auto next = value; auto seen = reachable; vector<int> used(trace ? capacity + 1 : 0);
        for (int w = 0; w <= capacity; ++w) {
            int limit = item.count < 0 ? w / item.weight : min(item.count, w / item.weight);
            for (int q = 1; q <= limit; ++q) {
                int old = w - q * item.weight;
                if (reachable[old] && (!seen[w] || next[w] < value[old] + lll(q) * item.value)) {
                    next[w] = value[old] + lll(q) * item.value; seen[w] = true;
                    if (trace) { used[w] = q; }} }
        }
        value.swap(next); reachable.swap(seen);
        if (trace) { take.push_back(std::move(used)); }}
    int target = 0;
    for (int w = 1; w <= capacity; ++w) {
        if (reachable[w] && value[w] > value[target]) { target = w; }}
    vector<int> witness(trace ? items.size() : 0);
    if (trace) {
        int w = target;
        for (int i = int(items.size()) - 1; i >= 0; --i) {
            witness[i] = take[i][w]; w -= witness[i] * items[i].weight; }}
    return {std::move(value), std::move(reachable), std::move(witness), target};
}
Output library(int capacity, const vector<KnapsackItem> &items, bool trace) {
    KnapsackDP dp(capacity, items, trace); Output out;
    out.value.resize(capacity + 1); out.reachable.resize(capacity + 1);
    for (int w = 0; w <= capacity; ++w) {
        auto answer = dp.exact(w); out.value[w] = answer.value;
        out.reachable[w] = answer.status == KnapsackStatus::finite; }
    out.target = dp.atMost(capacity).target;
    if (trace) { out.witness = dp.restore(out.target); }
    return out;
}
ulng verify(const Output &out, const Output &expected, const vector<KnapsackItem> &items, bool trace) {
    ulng hash = 0;
    for (int w = 0; w < int(expected.value.size()); ++w) {
        if (out.reachable[w] != expected.reachable[w]
            || (out.reachable[w] && out.value[w] != expected.value[w])) {
            cerr << "FAIL benchmark seed=" << SEED << ' ' << context << " exact weight=" << w << '\n'; std::exit(1); }
        if (out.reachable[w]) {
            hash = (hash ^ ulng(out.value[w]) ^ ulng(ulll(out.value[w]) >> 64) ^ ulng(w)) * 0x9e3779b97f4a7c15ULL; }}
    if (out.target < 0 || out.target >= int(out.value.size()) || !out.reachable[out.target]
        || out.value[out.target] != expected.value[expected.target]) {
        cerr << "FAIL benchmark seed=" << SEED << ' ' << context << " at-most result\n"; std::exit(1); }
    if (trace) {
        lll weight = 0, value = 0;
        if (out.witness.size() != items.size()) {
            cerr << "FAIL benchmark seed=" << SEED << ' ' << context << " witness size expected="
                 << items.size() << " actual=" << out.witness.size() << '\n'; std::exit(1); }
        for (int i = 0; i < int(items.size()); ++i) {
            if (out.witness[i] < 0 || (items[i].count >= 0 && out.witness[i] > items[i].count)) {
                cerr << "FAIL benchmark seed=" << SEED << ' ' << context << " item=" << i
                     << " expected quantity in [0," << items[i].count << "] actual=" << out.witness[i] << '\n'; std::exit(1); }
            weight += lll(out.witness[i]) * items[i].weight; value += lll(out.witness[i]) * items[i].value; }
        if (weight != out.target || value != out.value[out.target]) {
            cerr << "FAIL benchmark seed=" << SEED << ' ' << context << " reconstructed witness\n"; std::exit(1); }}
    return hash;
}
int main() {
    std::mt19937_64 rng(SEED);
    for (int capacity : {16, 512, 4096}) {
        int n = capacity == 16 ? 12 : 48, batch = capacity == 16 ? 64 : 1;
        for (int kind = 0; kind < 4; ++kind) {
            vector<KnapsackItem> items(n);
            for (auto &item : items) {
                item.weight = 1 + int(rng() % (kind == 3 ? 3 : 23));
                item.value = lng(rng() % 201) - 50;
                item.count = kind == 0 ? 1 : kind == 1 ? 1 + int(rng() % 32) : -1; }
            string distribution = vector<string>{"01", "bounded32", "unbounded", "unbounded-small-weight"}[kind];
            auto expected = direct(capacity, items, false);
            for (bool trace : {false, true}) {
                for (int rep = -1; rep < 5; ++rep) {
                    for (int j = 0; j < 2; ++j) {
                        int method = (j + rep + 1) % 2; vector<Output> outputs(batch);
                        context = "capacity=" + std::to_string(capacity) + " distribution=" + distribution
                                  + " trace=" + std::to_string(trace) + " rep=" + std::to_string(rep)
                                  + " method=" + std::to_string(method);
                        auto start = std::chrono::steady_clock::now();
                        for (auto &out : outputs) { out = method ? library(capacity, items, trace) : direct(capacity, items, trace); }
                        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / batch;
                        ulng hash = 0;
                        for (const auto &out : outputs) {
                            hash = hash * 0x9e3779b97f4a7c15ULL + verify(out, expected, items, trace); }
                        if (rep >= 0) {
                            cout << capacity << ' ' << n << ' ' << distribution << ' ' << trace << ' '
                                 << (method ? "library" : "direct-quantities") << ' ' << rep << ' ' << batch << ' '
                                 << std::setprecision(12) << ms << ' ' << hash << '\n'; } } } } } }
}
