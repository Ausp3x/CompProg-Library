// PROBLEM: https://judge.yosupo.jp/problem/prefix_sum_of_polynomial
// Status: accepted online (#417072, cpp, 2026-11-14 UTC, 152 ms; the expanded bundle of commit 5765edd, SHA-256 e6486b8fad9730d3); locally run against the regenerated judge tests (2026-11-15).
// Exercises: prefixSumOfPolynomial (Faulhaber through Bernoulli numbers); FastInput<0>, FastOutput.
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
    auto f = get(n);
    put(prefixSumOfPolynomial(f).resize(n + 1).v);
    return 0;}
