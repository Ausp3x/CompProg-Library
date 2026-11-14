// PROBLEM: https://judge.yosupo.jp/problem/polynomial_taylor_shift
// Status: accepted online (#417070, cpp, 2026-11-14 UTC, 67 ms; the expanded bundle of commit 5765edd, SHA-256 0963f85b4b9c8d9a); locally run against the regenerated judge tests (2026-11-15).
// Exercises: taylorShift (binomial convolution); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n; uint c; in.read(n, c);
    auto a = get(n);
    put(taylorShift(a, mint::raw(c)).resize(n).v);
    return 0;}
