// PROBLEM: https://judge.yosupo.jp/problem/sqrt_of_formal_power_series
// Status: accepted online (#417057, cpp, 2026-11-14 UTC, 74 ms; the expanded bundle of commit 5765edd, SHA-256 b93ac790f31b7ef9); locally run against the regenerated judge tests (2026-11-15).
// Exercises: trySqrt (even valuation, Tonelli constant, Newton); FastInput<0>, FastOutput.
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
    Poly<mint> r;
    if (trySqrt(a, n, r)) { put(r.resize(n).v); }
    else { out.print(-1); }
    return 0;}
