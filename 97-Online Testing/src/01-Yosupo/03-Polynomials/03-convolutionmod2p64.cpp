// PROBLEM: https://judge.yosupo.jp/problem/convolution_mod_2_64
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: PolyCompanion::convolution2p64 (32-bit halves through three NTT primes); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int n, m; in.read(n, m);
    vector<ulng> a(n), b(m); in.read(a, b);
    out.print(PolyCompanion::convolution2p64(a, b));
    return 0;}
