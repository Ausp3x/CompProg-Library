// PROBLEM: https://judge.yosupo.jp/problem/pow_of_formal_power_series
// Status: accepted online (#417056, cpp, 2026-11-14 UTC, 197 ms; the expanded bundle of commit 5765edd, SHA-256 bdfb34f36776c08a); locally run against the regenerated judge tests (2026-11-15).
// Exercises: pow with a 64-bit exponent (valuation shift, powUnit through log and exp); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n; lng k; in.read(n, k);
    auto a = get(n);
    put(pow(a, k, n).resize(n).v);
    return 0;}
