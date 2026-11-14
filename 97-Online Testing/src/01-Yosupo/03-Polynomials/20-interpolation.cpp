// PROBLEM: https://judge.yosupo.jp/problem/polynomial_interpolation
// Status: accepted online (#417068, cpp, 2026-11-14 UTC, 245 ms; the expanded bundle of commit 5765edd, SHA-256 5677ff4b1f6d82be); locally run against the regenerated judge tests (2026-11-15).
// Exercises: Poly::interpolate (subproduct tree, transposed evaluation of the derivative); FastInput<0>, FastOutput.
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
    auto xs = get(n).v, ys = get(n).v;
    put(Poly<mint>::interpolate(xs, ys).resize(n).v);
    return 0;}
