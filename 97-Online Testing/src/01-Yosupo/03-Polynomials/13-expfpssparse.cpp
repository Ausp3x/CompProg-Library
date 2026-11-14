// PROBLEM: https://judge.yosupo.jp/problem/exp_of_formal_power_series_sparse
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: expSparse (O(n * k) recurrence); FastInput<0>, FastOutput.
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
    Poly<mint> a(n);
    for (int t = 0; t < k; ++t) { int i; uint v; in.read(i, v); a.v[i] = mint::raw(v); }
    put(expSparse(a, n).resize(n).v);
    return 0;}
