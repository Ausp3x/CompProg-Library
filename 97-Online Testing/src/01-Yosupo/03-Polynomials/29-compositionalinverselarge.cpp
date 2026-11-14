// PROBLEM: https://judge.yosupo.jp/problem/compositional_inverse_of_formal_power_series_large
// Status: accepted online (#417079, cpp, 2026-11-14 UTC, 397 ms; the expanded bundle of commit 5765edd, SHA-256 cfa07d7ccb628edc); locally run against the regenerated judge tests (2026-11-15).
// Exercises: compositionalInverse (Lagrange inversion through power projection) up to 2^17 terms; FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n; in.read(n);
    auto a = get(n);
    put(compositionalInverse(a, n).resize(n).v);
    return 0;}
