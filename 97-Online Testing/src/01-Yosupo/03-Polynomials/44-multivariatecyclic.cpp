// PROBLEM: https://judge.yosupo.jp/problem/multivariate_convolution_cyclic
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: Poly<DynModInt<>>::multivariateCyclic (per-axis DFTs of lengths dividing p - 1) with a runtime prime; FastInput<0>, FastOutput.
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
    uint p; int k, total = 1; in.read(p, k);
    M::setMod(p, 1);
    vector<int> shape(k); in.read(shape);
    for (int s : shape) { total *= s; }
    auto f = get<M>(total).v, g = get<M>(total).v;
    put(Poly<M>::multivariateCyclic(f, g, shape));
    return 0;}
