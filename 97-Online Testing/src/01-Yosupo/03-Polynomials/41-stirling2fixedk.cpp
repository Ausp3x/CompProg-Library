// PROBLEM: https://judge.yosupo.jp/problem/stirling_number_of_the_second_kind_fixed_k
// Status: accepted online (#417092, cpp, 2026-11-14 UTC, 202 ms; the expanded bundle of commit 5765edd, SHA-256 34258964a9839a0e); locally run against the regenerated judge tests (2026-11-15).
// Exercises: ogfToEgf, pow (valuation k) and egfToOgf: S(n, k) = n! [x^n] (e^x - 1)^k / k!; FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n, k; in.read(n, k);
    Poly<mint> s(n + 1, 1); s[0] = 0; s = ogfToEgf(s);
    Poly<mint> p = egfToOgf(pow(s, k, n + 1).resize(n + 1));
    mint f = 1;
    for (int i = 2; i <= k; ++i) { f *= i; }
    f = 1 / f;
    for (int i = k; i <= n; ++i) { out.writeInt((p[i] * f).val(), i < n ? ' ' : '\n'); }
    return 0;}
