// PROBLEM: https://judge.yosupo.jp/problem/primality_test
// Status: accepted online (#417106, cpp, 2026-11-14 UTC, 165 ms; the expanded bundle of commit 66a6333, SHA-256 14d2cca59bc0b668); locally run against the regenerated judge tests (2026-11-15).
// Exercises: Montgomery64 (redc, init) as the reducer of isPrime (deterministic Miller-Rabin, 05-Mathematics/007-primality_factorization.hpp); FastInput<0>, FastOutput.
#include "../../../../05-Mathematics/007-primality_factorization.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int q; in.read(q);
    while (q--) { ulng n; in.read(n); out.write(isPrime(n) ? "Yes" : "No", '\n'); }
    return 0;}
