#include "../../05-Mathematics/04-sieve_algorithms.hpp"
#include "../../05-Mathematics/06-segmentedsieve.hpp"
#include <chrono>

struct PackedSieve {
    vector<bool> is_prime;
    vector<int> prms;
    explicit PackedSieve(int n) : is_prime(n + 1, true) {
        is_prime[0] = false; if (n) { is_prime[1] = false; }
        for (int p = 2; lng(p) * p <= n; ++p) {
            if (is_prime[p]) { for (lng j = lng(p) * p; j <= n; j += p) { is_prime[j] = false; }}}
        for (int p = 2; p <= n; ++p) { if (is_prime[p]) { prms.push_back(p); }}}
};

using Clock = std::chrono::steady_clock;
volatile ulng sink = 0;
template<class S>
void measure(const string &name, int n, const vector<int> &expected) {
    for (int rep = -1; rep < 5; ++rep) {
        auto start = Clock::now(); S sieve(n); auto end = Clock::now();
        if (sieve.prms != expected) { throw std::runtime_error("prime list mismatch " + name); }
        ulng checksum = 0;
        for (int p : sieve.prms) { checksum = checksum * 31 + p; } sink = checksum;
        if (rep >= 0) {
            cout << "dense " << name << ' ' << 0 << ' ' << n << ' ' << 0 << ' ' << rep << ' '
                 << std::chrono::duration<double, std::milli>(end - start).count() << ' '
                 << sieve.prms.size() << ' ' << checksum << '\n';}}}

void stream(lng l, int n, int block) {
    SegmentedSieveBase base(l + n - 1);
    vector<lng> expected;
    base.forEachPrime(l, l + n, [&](lng p) { expected.push_back(p); }, block, SegmentedSieveMode::Plain);
    for (auto [name, mode] : vector<pair<string, SegmentedSieveMode>>{
            {"plain", SegmentedSieveMode::Plain}, {"odd", SegmentedSieveMode::Odd}, {"wheel30", SegmentedSieveMode::Wheel30}}) {
        vector<lng> actual;
        base.forEachPrime(l, l + n, [&](lng p) { actual.push_back(p); }, block, mode);
        if (actual != expected) { throw std::runtime_error("segmented variant mismatch"); }
        ulng expected_sum = 0;
        for (lng p : expected) { expected_sum = expected_sum * 31 + ulng(p); }
        for (int rep = -1; rep < 5; ++rep) {
            ulng checksum = 0; int count = 0;
            auto start = Clock::now();
            base.forEachPrime(l, l + n, [&](lng p) { checksum = checksum * 31 + ulng(p); ++count; }, block, mode);
            auto end = Clock::now(); sink = checksum;
            if (checksum != expected_sum || count != int(expected.size())) { throw std::runtime_error("stream checksum mismatch"); }
            if (rep >= 0) {
                cout << "stream " << name << ' ' << l << ' ' << n << ' ' << block << ' ' << rep << ' '
                     << std::chrono::duration<double, std::milli>(end - start).count() << ' '
                     << count << ' ' << checksum << '\n';}}}}
void baseSetup(lng top) {
    for (int rep = -1; rep < 5; ++rep) {
        auto start = Clock::now(); SegmentedSieveBase base(top); auto end = Clock::now();
        int count = 0; ulng checksum = 0;
        base.forEachPrime(0, 100, [&](lng p) { ++count; checksum += ulng(p); });
        if (count != 25 || checksum != 1060) { throw std::runtime_error("base setup witness"); }
        sink = checksum;
        if (rep >= 0) {
            cout << "setup base " << top << " 0 32768 " << rep << ' '
                 << std::chrono::duration<double, std::milli>(end - start).count() << ' '
                 << count << ' ' << checksum << '\n';}}}
void materialize(lng top, int objects) {
    SegmentedSieveBase base(top);
    for (int rep = -1; rep < 5; ++rep) {
        ulng checksum = 0; int count = 0;
        auto start = Clock::now();
        for (int q = 0; q < objects; ++q) {
            SegmentedSieve s(100 + q, 200 + q, base);
            for (lng p : s.prms) { checksum = checksum * 31 + ulng(p); ++count; }}
        auto end = Clock::now(); sink = checksum;
        if (rep >= 0) {
            cout << "materialize shared_base " << top << ' ' << objects << " 32768 " << rep << ' '
                 << std::chrono::duration<double, std::milli>(end - start).count() << ' '
                 << count << ' ' << checksum << '\n';}}}
int main() {
    for (int n : {32, 50000, 2000000}) {
        auto expected = PackedSieve(n).prms;
        measure<PackedSieve>("packed_eratosthenes", n, expected);
        measure<SieveOfErath>("byte_eratosthenes", n, expected);
        measure<LinearSieve>("linear_spf", n, expected);}
    for (lng top : {lng(2000000), lng(1000002000000)}) { baseSetup(top); }
    for (lng l : {lng(0), lng(1000000000000)}) {
        for (int n : {32, 50000, 2000000}) { stream(l, n, 32768); }}
    for (int block : {1024, 262144}) { stream(1000000000000, 2000000, block); }
    stream(10000000000000000 - 100000, 100000, 32768);
    materialize(10000000000000000 - 1, 100);}
