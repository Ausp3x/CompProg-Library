// PROBLEM: https://judge.yosupo.jp/problem/primality_test
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: Montgomery64 (redc, init) as the reducer of isPrime (deterministic Miller-Rabin, 05-Mathematics/07-primality_factorization.hpp); FastInput<0>, FastOutput.
#include "../../../../05-Mathematics/07-primality_factorization.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int q; in.read(q);
    while (q--) { ulng n; in.read(n); out.write(isPrime(n) ? "Yes" : "No", '\n'); }
    return 0;}
