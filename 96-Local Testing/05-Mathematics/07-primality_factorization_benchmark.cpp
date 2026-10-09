#include "../../05-Mathematics/07-primality_factorization.hpp"


template<typename F>
double timeMs(F f) {
    auto t = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count();}

int main(int argc, char **argv) {
    int reps = argc > 1 ? std::stoi(argv[1]) : 5;
    std::mt19937_64 rng(20261009);
    auto prime = [&](int bits) { while (true) { ulng x = (rng() >> (64 - bits)) | (1ULL << (bits - 1)) | 1; if (isPrimeTrial(x) || (bits > 40 && isPrime(x))) { return x; } } };
    vector<ulng> odd64(1000000), primes64(100000), small32(1000000), semi(300), random64(10000);
    for (auto &x : odd64) { x = rng() | 1; }
    for (auto &x : primes64) { do { x = rng() | 1; } while (!isPrime(x)); }
    for (auto &x : small32) { x = rng() >> 32; }
    for (auto &x : semi) { x = prime(32) * prime(31); }
    for (auto &x : random64) { x = rng(); }
    struct Work { string name; vector<ulng> *in; bool fac; };
    vector<Work> work{{"isPrime 1e6 random odd 64-bit", &odd64, false}, {"isPrime 1e5 random 64-bit primes", &primes64, false},
                      {"isPrime 1e6 random 32-bit", &small32, false}, {"factorize 300 semiprimes 31x32-bit", &semi, true},
                      {"factorize 1e4 random 64-bit", &random64, true}};
    cout << "[\n";
    for (int w = 0; w < int(work.size()); ++w) {
        auto &[name, in, fac] = work[w];
        vector<double> mont, plain;
        for (int r = 0; r <= reps; ++r) {
            ulng a = 0, b = 0;
            double tm = timeMs([&] { for (ulng x : *in) { if (fac) { for (auto [p, e] : factorize(x)) { a += p * ulng(e); } } else { a += isPrime(x); } } });
            double tp = timeMs([&] { for (ulng x : *in) { if (fac) { for (auto [p, e] : factorizeCompact(x)) { b += p * ulng(e); } } else { b += isPrimeCompact(x); } } });
            if (a != b) { cerr << "MISMATCH " << name << endl; return 1; }
            if (r) { mont.pb(tm); plain.pb(tp); }}
        sort(mont.begin(), mont.end()); sort(plain.begin(), plain.end());
        cout << "  {\"workload\": \"" << name << "\", \"montgomery_ms\": " << mont[reps / 2] << ", \"compact_ms\": " << plain[reps / 2] << "}" << (w + 1 < int(work.size()) ? "," : "") << "\n";}
    cout << "]\n";}
