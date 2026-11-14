// PROBLEM: https://judge.yosupo.jp/problem/factorization_of_polynomials
// Status: accepted online (#417083, cpp, 2026-11-14 UTC, 404 ms; the expanded bundle of commit 5765edd, SHA-256 6d49b75cebb49789); locally run against the regenerated judge tests (2026-11-15).
// Exercises: factor over Poly<DynModInt<>> with a runtime prime (square-free, distinct-degree, Cantor–Zassenhaus); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    using M = DynModInt<>;
    int n; uint p; in.read(n, p);
    M::setMod(p, 1);
    auto fs = factor(get<M>(n + 1));
    out.print(fs.size());
    for (auto &[g, e] : fs) { out.writeInt(e, ' '); out.writeInt(g.deg(), ' '); put(g.v); }
    return 0;}
