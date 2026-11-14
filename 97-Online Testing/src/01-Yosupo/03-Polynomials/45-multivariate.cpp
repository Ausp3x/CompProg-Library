// PROBLEM: https://judge.yosupo.jp/problem/multivariate_convolution
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: Poly::multivariate (truncated product by the chi-graded Kronecker trick); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int k, total = 1; in.read(k);
    vector<int> shape(k); in.read(shape);
    for (int s : shape) { total *= s; }
    auto f = get(total).v, g = get(total).v;
    put(Poly<mint>::multivariate(f, g, shape));
    return 0;}
