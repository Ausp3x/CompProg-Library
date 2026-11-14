// PROBLEM: https://judge.yosupo.jp/problem/primality_test
// Status: accepted online (#417098, cpp, 2026-11-14 UTC, 164 ms; the expanded bundle of commit 5765edd, SHA-256 7008d5c9d43af0b8); locally run against the regenerated judge tests (2026-11-15).
// Exercises: Montgomery64 (redc, init) as the reducer of isPrime (deterministic Miller-Rabin, 05-Mathematics/07-primality_factorization.hpp); FastInput<0>, FastOutput.
#include "../../../../05-Mathematics/07-primality_factorization.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int q; in.read(q);
    while (q--) { ulng n; in.read(n); out.write(isPrime(n) ? "Yes" : "No", '\n'); }
    return 0;}
