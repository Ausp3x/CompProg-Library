// PROBLEM: https://judge.yosupo.jp/problem/sharp_p_subset_sum
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: exp of the harmonic log sum_s c_s log(1 + x^s); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n, t; in.read(n, t);
    vector<int> c(t + 1);
    for (int i = 0; i < n; ++i) { int s; in.read(s); ++c[s]; }
    vector<mint> iv(t + 1, 1);
    for (int i = 2; i <= t; ++i) { iv[i] = -mint(mint::mod() / i) * iv[mint::mod() % i]; }
    Poly<mint> l(t + 1);
    for (int s = 1; s <= t; ++s) {
        if (!c[s]) { continue; }
        for (int k = 1; s * k <= t; ++k) { mint v = c[s] * iv[k]; l[s * k] += k & 1 ? v : -v; }}
    Poly<mint> e = exp(l, t + 1);
    for (int i = 1; i <= t; ++i) { out.writeInt(e[i].val(), i < t ? ' ' : '\n'); }
    return 0;}
