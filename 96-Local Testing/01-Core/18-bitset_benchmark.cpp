#include "../../01-Core/18-bitset.hpp"
#include <chrono>
#include <iomanip>
#include <random>
#include <stdexcept>

namespace {
    using Clock = std::chrono::steady_clock;
    ulng sink = 0;

    void barrier(const void *p) { asm volatile("" : : "g"(p) : "memory"); }
    ulng digest(const vector<ulng> &a) {
        ulng h = 0x9e3779b97f4a7c15ULL;
        for (ulng x : a) { h = (h ^ x) * 0xbf58476d1ce4e5b9ULL; }
        return h;}

    void require(bool ok, const string &context) {
        if (!ok) { throw std::runtime_error("verification failed: " + context); }}

    size_t genericCount(const Bitset &a, const Bitset &b, bool intersect) {
        size_t count = 0;
        for (size_t i = 0; i < a.a.size(); ++i) { count += std::popcount(intersect ? a.a[i] & b.a[i] : a.a[i]); }
        return count;}
    size_t genericRangeCount(const Bitset &a, size_t l, size_t r) {
        if (l == r) { return 0; }
        size_t first = l / 64, last = (r - 1) / 64;
        ulng low = ~ulng(0) << (l % 64);
        ulng high = r % 64 ? (ulng(1) << (r % 64)) - 1 : ~ulng(0);
        if (first == last) { return std::popcount(a.a[first] & low & high); }
        size_t count = std::popcount(a.a[first] & low) + std::popcount(a.a[last] & high);
        for (size_t i = first + 1; i < last; ++i) { count += std::popcount(a.a[i]); }
        return count;}

    void genericXor(Bitset &a, const Bitset &b) {
        for (size_t i = 0; i < a.a.size(); ++i) { a.a[i] ^= b.a[i]; }}
    void genericShiftXor(Bitset &a, const Bitset &b, size_t k) {
        if (k >= a.n) { return; }
        size_t d = k / 64;
        int s = int(k % 64);
        for (size_t i = a.a.size(); i-- > d;) {
            ulng x = b.a[i - d] << s;
            if (s && i > d) { x |= b.a[i - d - 1] >> (64 - s); }
            a.a[i] ^= x;}
        if (a.n % 64) { a.a.back() &= (ulng(1) << (a.n % 64)) - 1; }}

    template<class Prepare, class Run, class Check>
    double measure(Prepare prepare, Run run, Check check, int repetitions, double milliseconds) {
        int iterations = 1; // Adaptive warmup caps the count at 2^26.
        auto trial = [&](int count) {
            prepare();
            auto start = Clock::now();
            for (int i = 0; i < count; ++i) { run(); }
            double ns = std::chrono::duration<double, std::nano>(Clock::now() - start).count();
            check(count);
            return ns;};
        for (;;) {
            double ns = trial(iterations);
            if (ns >= milliseconds * 1e6 || iterations >= (1 << 26)) { break; }
            iterations *= 2;}
        vector<double> samples;
        for (int r = 0; r < repetitions; ++r) { samples.push_back(trial(iterations) / iterations); }
        std::sort(samples.begin(), samples.end());
        return samples[samples.size() / 2];}

    void row(size_t bits, const string &distribution, const string &operation,
             double generic, double library, ulng checksum) {
        std::cout << "{\"bits\":" << bits << ",\"distribution\":\"" << distribution
            << "\",\"operation\":\"" << operation << "\",\"reference_ns\":" << generic
            << ",\"library_ns\":" << library << ",\"speedup\":" << generic / library
            << ",\"checksum\":" << checksum << "}\n";}

    void benchmark(size_t bits, bool sparse, std::mt19937_64 &rng, int repetitions, double milliseconds) {
        Bitset a(bits), b(bits), x;
        for (auto &w : a.a) { w = sparse ? ulng(1) << (rng() % 64) : rng(); }
        for (auto &w : b.a) { w = sparse ? ulng(1) << (rng() % 64) : rng(); }
        a.trim(); b.trim();
        string distribution = sparse ? "one-bit-per-word" : "uniform-random";
        for (bool intersect : {false, true}) {
            size_t expected = genericCount(a, b, intersect), last = 0;
            require(expected == (intersect ? a.countAnd(b) : a.count()), "count bits=" + std::to_string(bits));
            auto check = [&](int) {
                require(last == expected, "count timing");
                sink ^= last;};
            double generic = measure([] {}, [&] {
                barrier(a.a.data()); barrier(b.a.data());
                last = genericCount(a, b, intersect);}, check, repetitions, milliseconds);
            double library = measure([] {}, [&] {
                barrier(a.a.data()); barrier(b.a.data());
                last = intersect ? a.countAnd(b) : a.count();}, check, repetitions, milliseconds);
            row(bits, distribution, intersect ? "count_and" : "count", generic, library, expected);
            if (intersect) {
                double temporary = measure([] {}, [&] {
                    barrier(a.a.data()); barrier(b.a.data());
                    last = (a & b).count();}, check, repetitions, milliseconds);
                row(bits, distribution, "count_and_temporary_allocations_included", temporary, library, expected);}}
        {
            size_t l = bits >= 24 ? 13 : 0, r = bits >= 24 ? bits - 11 : 0;
            size_t expected = 0, last = 0;
            for (size_t p = l; p < r; ++p) { expected += a.test(p); }
            require(genericRangeCount(a, l, r) == expected && a.count(l, r) == expected,
                    "range count bit oracle bits=" + std::to_string(bits));
            auto check = [&](int) {
                require(last == expected, "range count timing");
                sink ^= last;};
            double generic = measure([] {}, [&] {
                barrier(a.a.data());
                last = genericRangeCount(a, l, r);}, check, repetitions, milliseconds);
            double library = measure([] {}, [&] {
                barrier(a.a.data());
                last = a.count(l, r);}, check, repetitions, milliseconds);
            row(bits, distribution, "range_count_13_to_n_minus_11", generic, library, expected);
        }
        for (bool shifted : {false, true}) {
            constexpr int SHIFT = 13;
            Bitset expected(a);
            for (size_t p = 0; p < bits; ++p) {
                bool rhs = shifted ? (p >= SHIFT && b.test(p - SHIFT)) : b.test(p);
                if (rhs) { expected.flip(p); }}
            x = a;
            if (shifted) { x.combineShift<Bitset::Op::Xor>(b, SHIFT); } else { x ^= b; }
            require(x == expected, "bit oracle bits=" + std::to_string(bits));
            auto prepare = [&] { x = a; };
            auto check = [&](int count) {
                require(x == ((count & 1) ? expected : a), "xor timing");
                sink ^= digest(x.a);};
            double generic = measure(prepare, [&] {
                barrier(x.a.data()); barrier(b.a.data());
                if (shifted) { genericShiftXor(x, b, SHIFT); } else { genericXor(x, b); }}, check, repetitions, milliseconds);
            double library = measure(prepare, [&] {
                barrier(x.a.data()); barrier(b.a.data());
                if (shifted) { x.combineShift<Bitset::Op::Xor>(b, SHIFT); } else { x ^= b; }}, check, repetitions, milliseconds);
            row(bits, distribution, shifted ? "left_shift_xor_13" : "xor", generic, library, digest(expected.a));}
        if (bits >= 24) {
            // Offset range XOR: x[13, n - 11) ^= b[0, n - 24), fused against a materialized shifted-and-masked temporary.
            size_t l = 13, r = bits - 11;
            Bitset expected(a);
            for (size_t p = l; p < r; ++p) { if (b.test(p - l)) { expected.flip(p); } }
            x = a;
            x.combineRange<Bitset::Op::Xor>(l, r, b, 0);
            require(x == expected, "range xor bit oracle bits=" + std::to_string(bits));
            auto prepare = [&] { x = a; };
            auto check = [&](int count) {
                require(x == ((count & 1) ? expected : a), "range xor timing");
                sink ^= digest(x.a);};
            double materialized = measure(prepare, [&] {
                barrier(x.a.data()); barrier(b.a.data());
                Bitset t = b << l;
                t.resetRange(r, bits);
                x ^= t;}, check, repetitions, milliseconds);
            double library = measure(prepare, [&] {
                barrier(x.a.data()); barrier(b.a.data());
                x.combineRange<Bitset::Op::Xor>(l, r, b, 0);}, check, repetitions, milliseconds);
            row(bits, distribution, "range_xor_13_to_n_minus_11_vs_materialized_shift", materialized, library, digest(expected.a));}}

    Bitset subsetSum(size_t bits, const vector<size_t> &weights, bool fused) {
        Bitset result(bits);
        if (bits) { result.set(0); }
        for (size_t w : weights) {
            if (fused) { result.combineShift<Bitset::Op::Or>(result, w); }
            else { result |= result << w; }}
        return result;}
    void subsetBenchmark(size_t bits, std::mt19937_64 &rng, int repetitions, double milliseconds) {
        vector<size_t> weights(96);
        for (size_t &w : weights) { w = 1 + rng() % std::max(size_t(1), bits / 128); }
        Bitset expected = subsetSum(bits, weights, false), result;
        require(subsetSum(bits, weights, true) == expected, "subset sum comparison");
        vector<unsigned char> oracle(bits);
        if (bits) { oracle[0] = 1; }
        for (size_t w : weights) {
            if (w < bits) { for (size_t i = bits; i-- > w;) { oracle[i] |= oracle[i - w]; } }}
        for (size_t p = 0; p < bits; ++p) { require(bool(oracle[p]) == expected.test(p), "subset sum bit oracle"); }
        auto check = [&](int) {
            require(result == expected, "subset sum timing");
            sink ^= digest(result.a);};
        double reference = measure([] {}, [&] {
            barrier(weights.data());
            result = subsetSum(bits, weights, false);}, check, repetitions, milliseconds);
        double library = measure([] {}, [&] {
            barrier(weights.data());
            result = subsetSum(bits, weights, true);}, check, repetitions, milliseconds);
        row(bits, "96-seeded-positive-weights", "subset_sum_allocations_included", reference, library, digest(expected.a));}
} // namespace

int main(int argc, char **argv) {
    try {
        ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
        int repetitions = argc > 2 ? std::stoi(argv[2]) : 5;
        double milliseconds = argc > 3 ? std::stod(argv[3]) : 3;
        require(repetitions > 0 && milliseconds > 0, "positive repetitions and duration");
        std::cout << std::fixed << std::setprecision(3);
        std::mt19937_64 rng(seed);
        for (int n : {0, 1, 63, 64, 65, 959, 960, 961, 1023, 1024, 1025, 1088, 1152, 1216, 4096, 65536, 1048576}) {
            benchmark(n, false, rng, repetitions, milliseconds);}
        for (int n : {1024, 65536, 1048576}) { benchmark(n, true, rng, repetitions, milliseconds); }
        for (int n : {64, 1024, 65536, 1048576}) { subsetBenchmark(n, rng, repetitions, milliseconds); }
        std::cerr << "PASS seed=" << seed << " repetitions=" << repetitions << " target_ms=" << milliseconds << " sink=" << sink << '\n';}
    catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }}
