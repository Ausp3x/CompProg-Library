#include "../../02-Data Structures/07-ordered_set.hpp"

// Emits one JSON line per (insertion order, n, split fraction): the split and join times of OrderedMultiSet on a fresh tree.
namespace {
    using Clock = std::chrono::steady_clock;

    void measure(bool shuffled, int n, double fraction, ulng seed) {
        vector<int> keys(n);
        std::iota(keys.begin(), keys.end(), 0);
        if (shuffled) { std::shuffle(keys.begin(), keys.end(), std::mt19937_64(seed)); }
        OrderedMultiSet<int> a, b;
        for (int x : keys) { a.insert(x); }
        int x = int(fraction * n);
        auto t0 = Clock::now();
        a.split(x, b);
        auto t1 = Clock::now();
        int moved = b.size();
        a.join(b);
        auto t2 = Clock::now();
        if (moved != n - 1 - x || a.size() != n || !b.empty() || a.findByOrder(x)->first != x) { throw std::runtime_error("split/join result mismatch"); }
        std::cout << "{\"order\":\"" << (shuffled ? "shuffled" : "sorted") << "\",\"n\":" << n << ",\"fraction\":" << fraction << ",\"moved\":" << moved
                  << ",\"split_seconds\":" << std::chrono::duration<double>(t1 - t0).count()
                  << ",\"join_seconds\":" << std::chrono::duration<double>(t2 - t1).count() << "}\n";}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261112;
    for (bool shuffled : {false, true}) {
        for (int n : {1 << 16, 1 << 18, 1 << 20}) {
            for (double fraction : {0.001, 0.5, 0.999}) { measure(shuffled, n, fraction, seed + ulng(n)); }}}
    return 0;}
