#include "../../06-Miscellaneous/11-sorting_selection.hpp"

constexpr ulng SEED = 20260928;
struct Record {
    lng key;
    int id;
    array<ulng, 2> payload;
    bool operator==(const Record &) const = default;
};
static_assert(sizeof(Record) == 32);
ulng checksum(lng x) { return ulng(x); }
ulng checksum(lll x) { return ulng(x) ^ ulng(ulll(x) >> 64); }
ulng checksum(const Record &x) { return ulng(x.key) ^ ulng(x.id) ^ x.payload[0] ^ x.payload[1]; }
template<typename T, typename Sort>
void benchmark(const vector<T> &input, const vector<T> &expected, const string &type,
               const string &distribution, const vector<string> &methods, Sort sort) {
    int n = int(input.size()), batch = n <= 32 ? 256 : n <= 4096 ? 8 : 1;
    for (int rep = -1; rep < 5; ++rep) {
        for (int j = 0; j < int(methods.size()); ++j) {
            int method = (j + rep + 1) % int(methods.size()); vector<vector<T>> outputs(batch);
            auto start = std::chrono::steady_clock::now();
            for (auto &out : outputs) { out = input; sort(out, method); }
            double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / batch;
            ulng hash = 0;
            for (const auto &out : outputs) {
                if (out != expected) {
                    int first = 0; while (first < min(out.size(), expected.size()) && out[first] == expected[first]) { ++first; }
                    cerr << "FAIL seed=" << SEED << " n=" << n << " type=" << type << " distribution=" << distribution
                         << " method=" << methods[method] << " rep=" << rep << " first_mismatch=" << first << '\n'; std::exit(1); }
                for (const auto &x : out) { hash = (hash ^ checksum(x)) * 0x9e3779b97f4a7c15ULL; } }
            if (rep >= 0) {
                cout << n << ' ' << type << ' ' << distribution << ' ' << methods[method] << ' ' << rep << ' '
                     << batch << ' ' << std::setprecision(12) << ms << ' ' << hash << '\n'; } } } }
template<typename T> T randomWord(std::mt19937_64 &rng) {
    if constexpr (sizeof(T) == 16) { return std::bit_cast<T>((ulll(rng()) << 64) | rng()); }
    else { return std::bit_cast<T>(rng()); } }
template<typename T>
vector<T> generate(int n, int distribution, std::mt19937_64 &rng) {
    vector<T> out(n);
    for (T &x : out) { x = distribution == 6 ? 17 : distribution < 3 ? T(rng() % 256) : randomWord<T>(rng); }
    if (distribution == 1 || distribution == 2 || distribution == 4 || distribution == 5) { sort(out.begin(), out.end()); }
    if (distribution == 2 || distribution == 5) { reverse(out.begin(), out.end()); }
    return out; }
int main() {
    std::mt19937_64 rng(SEED);
    vector<string> distributions{"dense256", "sorted-dense256", "reverse-dense256", "full-width", "sorted-full-width", "reverse-full-width", "equal"};
    for (int n : {32, 4096, 200000}) {
        for (int d = 0; d < int(distributions.size()); ++d) {
            bool dense = d < 3 || d == 6; vector<string> methods{"std-sort", "radix"};
            if (dense) { methods.push_back("counting"); }
            auto input = generate<lng>(n, d, rng), expected = input; sort(expected.begin(), expected.end());
            benchmark(input, expected, "signed64", distributions[d], methods, [&](vector<lng> &a, int method) -> void {
                if (method == 0) { sort(a.begin(), a.end()); }
                else if (method == 1) { radixSort(a); }
                else { countingSort(a, lng(0), lng(255)); } });
            vector<Record> records;
            for (int i = 0; i < n; ++i) { records.push_back({input[i], i, {rng(), rng()}}); }
            auto stable = records;
            auto cmp = [](const Record &a, const Record &b) -> bool { return a.key < b.key; };
            std::stable_sort(stable.begin(), stable.end(), cmp); methods[0] = "std-stable-sort";
            if (dense) { methods[2] = "stable-counting"; }
            benchmark(records, stable, "record32", distributions[d], methods, [&](vector<Record> &a, int method) -> void {
                auto key = [](const Record &x) -> lng { return x.key; };
                if (method == 0) { std::stable_sort(a.begin(), a.end(), cmp); }
                else if (method == 1) { radixSort(a, key); }
                else { stableCountingSort(a, 256, key); } }); }
        for (int d : {3, 4, 5, 6}) {
            auto input = generate<lll>(n, d, rng), expected = input; sort(expected.begin(), expected.end());
            benchmark(input, expected, "signed128", distributions[d], {"std-sort", "radix"}, [&](vector<lll> &a, int method) -> void {
                if (method == 0) { sort(a.begin(), a.end()); } else { radixSort(a); } }); } } }
