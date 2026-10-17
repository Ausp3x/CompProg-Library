#include "../../02-Data Structures/10-persistentdsu.hpp"

// Emits one JSON line per workload; VARIANT names the default arity the header was built with.
#ifndef VARIANT
#define VARIANT "library"
#endif

namespace {
    using Clock = std::chrono::steady_clock;

    void emit(const string &workload, int n, int q, double sec, ulng sum, lng bytes) {
        std::cout << "{\"workload\":\"" << workload << "\",\"variant\":\"" << VARIANT << "\",\"n\":" << n << ",\"q\":" << q
                  << ",\"seconds\":" << sec << ",\"checksum\":" << sum << ",\"pool_bytes\":" << bytes << "}\n";}
    template<typename A> lng poolBytes(const A &a) { return lng(a.ch.size() * sizeof(int) + a.val.size() * sizeof(a.val[0]) + a.roots.size() * sizeof(int)); }

    // q operations: half set (random version, random index), half get (random version, random index).
    void arrayMixed(ulng seed, int n, int q) {
        std::mt19937_64 rng(seed);
        vector<int> a(n);
        for (auto &x : a) { x = int(rng() % 1000000007); }
        auto t0 = Clock::now();
        PersistentArray<int> p(a);
        ulng sum = 0;
        for (int i = 0; i < q; ++i) {
            int v = int(rng() % ulng(p.versionCount())), k = int(rng() % ulng(n));
            if (rng() & 1) { p.set(v, k, int(rng() % 1000000007)); }
            else { sum += ulng(p.get(v, k)); }}
        emit("array-mixed", n, q, std::chrono::duration<double>(Clock::now() - t0).count(), sum, poolBytes(p));}

    // q sets on the latest version, then q gets on random versions.
    void arrayChain(ulng seed, int n, int q) {
        std::mt19937_64 rng(seed);
        auto t0 = Clock::now();
        PersistentArray<int> p(n, 0);
        ulng sum = 0;
        for (int i = 0; i < q; ++i) { p.set(p.versionCount() - 1, int(rng() % ulng(n)), i); }
        for (int i = 0; i < q; ++i) { sum += ulng(p.get(int(rng() % ulng(p.versionCount())), int(rng() % ulng(n)))); }
        emit("array-chain", n, q, std::chrono::duration<double>(Clock::now() - t0).count(), sum, poolBytes(p));}

    // Library Checker persistent_unionfind shape: query i acts on a random earlier version; half unite, half isSameSet.
    void dsuChecker(ulng seed, int n, int q) {
        std::mt19937_64 rng(seed);
        auto t0 = Clock::now();
        PersistentDSU d(n);
        vector<int> id{0};
        ulng sum = 0;
        for (int i = 0; i < q; ++i) {
            int k = id[rng() % id.size()], u = int(rng() % ulng(n)), v = int(rng() % ulng(n));
            if (rng() & 1) { id.push_back(d.uniteSets(k, u, v)); }
            else { sum = sum * 3 + d.isSameSet(k, u, v); }}
        emit("dsu-checker", n, q, std::chrono::duration<double>(Clock::now() - t0).count(), sum, poolBytes(d.a));}

    // q unions on the latest version, then q isSameSet and getSize on random versions.
    void dsuChain(ulng seed, int n, int q) {
        std::mt19937_64 rng(seed);
        auto t0 = Clock::now();
        PersistentDSU d(n);
        int t = 0;
        ulng sum = 0;
        for (int i = 0; i < q; ++i) { t = d.uniteSets(t, int(rng() % ulng(n)), int(rng() % ulng(n))); }
        for (int i = 0; i < q; ++i) {
            int k = int(rng() % ulng(t + 1)), u = int(rng() % ulng(n));
            sum = sum * 3 + d.isSameSet(k, u, int(rng() % ulng(n))) + ulng(d.getSize(k, u));}
        emit("dsu-chain", n, q, std::chrono::duration<double>(Clock::now() - t0).count(), sum, poolBytes(d.a));}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261017;
    arrayMixed(seed, 200000, 1000000); arrayMixed(seed + 1, 1000000, 1000000); arrayChain(seed + 2, 1000000, 1000000);
    dsuChecker(seed + 3, 200000, 200000); dsuChecker(seed + 4, 1000000, 1000000); dsuChain(seed + 5, 200000, 200000);
    return 0;}
