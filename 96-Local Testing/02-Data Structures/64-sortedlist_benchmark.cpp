#include "../../02-Data Structures/64-sortedlist.hpp"

// Emits one JSON line per (workload, variant); VARIANT names the header constants it was built with.
#ifndef VARIANT
#define VARIANT "library"
#endif

namespace {
    using Clock = std::chrono::steady_clock;
    using Tree = tree<pair<lng, int>, null_type, std::less<pair<lng, int>>, rb_tree_tag, tree_order_statistics_node_update>;

    void emit(const string &workload, const string &variant, int n, int q, double sec, ulng sum) {
        std::cout << "{\"workload\":\"" << workload << "\",\"variant\":\"" << variant << "\",\"n\":" << n << ",\"q\":" << q
                  << ",\"seconds\":" << sec << ",\"checksum\":" << sum << "}\n";}

    // Steady size n after n random inserts, then q mixed operations: 25% insert, 25% eraseOne, 20% rank, 20% kth, 10% lowerBound.
    template<typename Ops>
    void mixed(const string &variant, int n, int q, ulng seed, Ops ops) {
        std::mt19937_64 rng(seed);
        lng range = 4LL * n;
        auto t0 = Clock::now();
        ulng sum = ops(rng, range);
        double sec = std::chrono::duration<double>(Clock::now() - t0).count();
        emit("mixed", variant, n, q, sec, sum);}

    ulng runList(std::mt19937_64 &rng, lng range, int n, int q) {
        SortedList<lng> s;
        ulng sum = 0;
        for (int i = 0; i < n; ++i) { s.insert(lng(rng() % ulng(range))); }
        for (int i = 0; i < q; ++i) {
            lng x = lng(rng() % ulng(range));
            int op = int(rng() % 20);
            if (op < 5) { s.insert(x); }
            else if (op < 10) { sum += s.eraseOne(x); }
            else if (op < 14) { sum += ulng(s.rank(x)); }
            else if (op < 18) { sum += ulng(s.kth(int(ulng(x) % ulng(s.size())))); }
            else { auto it = s.lowerBound(x); sum += it == s.end() ? 0 : ulng(*it); }}
        return sum + ulng(s.size());}

    ulng runTree(std::mt19937_64 &rng, lng range, int n, int q) {
        Tree s;
        int id = 0;
        ulng sum = 0;
        for (int i = 0; i < n; ++i) { s.insert({lng(rng() % ulng(range)), id++}); }
        for (int i = 0; i < q; ++i) {
            lng x = lng(rng() % ulng(range));
            int op = int(rng() % 20);
            if (op < 5) { s.insert({x, id++}); }
            else if (op < 10) {
                auto it = s.lower_bound({x, INT_MIN});
                bool hit = it != s.end() && it->first == x;
                if (hit) { s.erase(it); }
                sum += hit;}
            else if (op < 14) { sum += ulng(s.order_of_key({x, INT_MIN})); }
            else if (op < 18) { sum += ulng(s.find_by_order(int(ulng(x) % ulng(s.size())))->first); }
            else { auto it = s.lower_bound({x, INT_MIN}); sum += it == s.end() ? 0 : ulng(it->first); }}
        return sum + ulng(s.size());}

    // Insert ascending then popFront everything: the adversarial pattern for front-heavy shifts.
    ulng runQueue(int n) {
        SortedList<lng> s;
        ulng sum = 0;
        for (int i = 0; i < n; ++i) { s.insert(i); }
        while (!s.empty()) { sum += ulng(s.popFront()); }
        return sum;}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261009;
    bool tree = argc > 2 && string(argv[2]) == "tree";
    for (auto [n, q] : vector<pair<int, int>>{{1000, 1000000}, {100000, 1000000}, {1000000, 1000000}}) {
        if (tree) { mixed("pbds-tree", n, q, seed, [&](std::mt19937_64 &rng, lng range) { return runTree(rng, range, n, q); }); }
        else { mixed(VARIANT, n, q, seed, [&](std::mt19937_64 &rng, lng range) { return runList(rng, range, n, q); }); }}
    if (!tree) {
        int n = 1000000;
        auto t0 = Clock::now();
        ulng sum = runQueue(n);
        emit("ascending-then-popFront", VARIANT, n, n, std::chrono::duration<double>(Clock::now() - t0).count(), sum);}
    return 0;}
