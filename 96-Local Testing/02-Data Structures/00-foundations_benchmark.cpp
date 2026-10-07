#include "../../02-Data Structures/02-fenwick.hpp"

// Separate construction measurements: both methods build the same Fenwick tree.
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    volatile lng sink = 0;
    for (int n : {32, 4096, 262144}) {
        vector<lng> a(n);
        for (lng &x : a) { x = lng(rng() % 101); }
        lng expected = accumulate(a.begin(), a.end(), lng(0));
        int repeats = max(1, 262144 / n);
        for (int kind = 0; kind < 2; ++kind) {
            // Untimed warmup, with an independent sum check.
            Fenwick<lng> warm(a);
            if (warm.sum(0, n) != expected) { return 1; }
            auto start = std::chrono::steady_clock::now();
            lng checksum = 0;
            for (int k = 0; k < repeats; ++k) {
                if (kind == 0) {
                    Fenwick<lng> bit(a);
                    checksum += bit.sum(0, n);}
                else {
                    Fenwick<lng> bit(n);
                    for (int i = 0; i < n; ++i) { bit.add(i, a[i]); }
                    checksum += bit.sum(0, n);}}
            double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (checksum != expected * repeats) { return 2; }
            sink = checksum;
            cout << "{\"workload\":\"fenwick_build\",\"variant\":\""
                 << (kind == 0 ? "linear" : "point_adds") << "\",\"n\":" << n
                 << ",\"repeats\":" << repeats << ",\"seconds\":" << std::setprecision(10)
                 << elapsed << ",\"checksum\":" << checksum << "}\n";}}
    return sink < 0;}
