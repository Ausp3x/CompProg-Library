// PROBLEM: https://judge.yosupo.jp/problem/factorize
// Status: accepted online (#417099, cpp, 2026-11-14 UTC, 36 ms; the expanded bundle of commit 5765edd, SHA-256 8ac6f22f88c1e16e); locally run against the regenerated judge tests (2026-11-15).
// Exercises: Montgomery64 (redc, init) as the reducer of factorize (Miller-Rabin and batched Pollard-Brent rho, 05-Mathematics/07-primality_factorization.hpp); FastInput<0>, FastOutput.
#include "../../../../05-Mathematics/07-primality_factorization.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int q; in.read(q);
    while (q--) {
        ulng a; in.read(a);
        vector<ulng> ps;
        for (auto [p, e] : factorize(a)) { ps.insert(ps.end(), e, p); }
        out.writeInt(ps.size(), ps.empty() ? '\n' : ' ');
        if (!ps.empty()) { out.print(ps); }}
    return 0;}
