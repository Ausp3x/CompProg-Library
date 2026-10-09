#include "../../02-Data Structures/61-hashmap.hpp"

namespace {
    using Clock = std::chrono::steady_clock;

    // n keys of the given kind: q mixed ops 40% operator[] increment, 40% lookup (about half hits), 20% erase.
    template<typename M, typename Find, typename Erase>
    ulng work(M &m, const vector<lng> &keys, int q, ulng seed, Find find, Erase erase) {
        std::mt19937_64 rng(seed);
        ulng sum = 0;
        int n = int(keys.size());
        for (int i = 0; i < q; ++i) {
            lng k = keys[rng() % ulng(n)];
            int op = int(rng() % 5);
            if (op < 2) { m[k] += i; }
            else if (op < 4) { sum += find(m, k); }
            else { sum += erase(m, k); }}
        return sum + ulng(m.size());}

    void emit(const string &workload, const string &variant, int n, int q, double sec, ulng sum) {
        std::cout << "{\"workload\":\"" << workload << "\",\"variant\":\"" << variant << "\",\"n\":" << n << ",\"q\":" << q
                  << ",\"seconds\":" << sec << ",\"checksum\":" << sum << "}\n";}

    template<typename M, typename Find, typename Erase>
    void timed(const string &workload, const string &variant, const vector<lng> &keys, int q, ulng seed, Find find, Erase erase) {
        auto t0 = Clock::now();
        M m;
        ulng sum = work(m, keys, q, seed, find, erase);
        emit(workload, variant, int(keys.size()), q, std::chrono::duration<double>(Clock::now() - t0).count(), sum);}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261009;
    for (int kind = 0; kind < 2; ++kind) {
        for (int n : {1000, 100000, 2000000}) {
            int q = 4000000;
            vector<lng> keys(n);
            std::mt19937_64 rng(seed + ulng(n));
            for (int i = 0; i < n; ++i) { keys[i] = kind ? lng(i) << 20 : lng(rng() >> 1); }
            string w = kind ? "keys i<<20" : "random keys";
            auto findH = [](auto &m, lng k) -> ulng { auto it = m.find(k); return it == m.end() ? 0 : ulng(it->second); };
            auto eraseH = [](auto &m, lng k) -> ulng { return ulng(m.erase(k)); };
            timed<HashMap<lng, lng>>(w, "HashMap", keys, q, seed, findH, eraseH);
            timed<unordered_map<lng, lng, CustomHash>>(w, "unordered_map+CustomHash", keys, q, seed, findH, eraseH);
            timed<__gnu_pbds::gp_hash_table<lng, lng, CustomHash>>(w, "gp_hash_table+CustomHash", keys, q, seed, findH,
                [](auto &m, lng k) -> ulng { return ulng(m.erase(k)); });}}
    return 0;}
