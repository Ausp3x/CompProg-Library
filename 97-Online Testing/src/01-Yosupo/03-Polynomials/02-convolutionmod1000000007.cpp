// PROBLEM: https://judge.yosupo.jp/problem/convolution_mod_1000000007
// Status: accepted online (#417049, cpp, 2026-11-14 UTC, 176 ms; the expanded bundle of commit 5765edd, SHA-256 11413184f5dd5035); locally run against the regenerated judge tests (2026-11-15).
// Exercises: Poly<ModInt<1000000007>> operator* (dispatcher: three NTT primes with Garner, split FFT below 2^20); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n, m; in.read(n, m);
    auto a = get<ModInt<1000000007>>(n), b = get<ModInt<1000000007>>(m);
    put((a * b).resize(n + m - 1).v);
    return 0;}
