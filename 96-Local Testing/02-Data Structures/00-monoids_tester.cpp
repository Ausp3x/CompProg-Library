#include "../../02-Data Structures/00-monoids.hpp"
#include "../../02-Data Structures/03-segmenttree.hpp"
#include "../../02-Data Structures/12-lazysegmenttree.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    ulng seed = 20261017;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    using Mat = array<array<lng, 3>, 3>;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T, size_t K> string show(const array<T, K> &a) {
        string s = "[";
        for (size_t i = 0; i < K; ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename T> string show(const vector<T> &a) {
        string s = "[";
        for (size_t i = 0; i < a.size(); ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename... T> string show(const tuple<T...> &t) {
        string s = "(";
        std::apply([&](const auto &...x) { int k = 0; ((s += (k++ ? "," : "") + show(x)), ...); }, t);
        return s + ")";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    lng mulmod(lng a, lng b) { return lng(lll(a) * b % P); }

    // Acted specs: draw(rng, a, l, r) applies the action's meaning to a[l, r) and returns the preset's encoding of it.
    struct AddSum {
        using A = RangeAddRangeSum<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -5, 5); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { lng x = rnd(rng, -3, 3); for (int i = l; i < r; ++i) { a[i] += x; } return A::add(x); }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.sum, s.len); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s += a[i]; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    template<bool MAX> struct AddMinMax {
        using A = std::conditional_t<MAX, RangeAddRangeMax<lng>, RangeAddRangeMin<lng>>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { lng x = rnd(rng, -4, 4); for (int i = l; i < r; ++i) { a[i] += x; } return A::add(x); }
        static A::S leaf(int, V v) { return v; }
        static auto key(const A::S &s) { return tuple(s); }
        static auto expect(const vector<V> &a, int l, int r) {
            lng s = MAX ? std::numeric_limits<lng>::lowest() : std::numeric_limits<lng>::max();
            for (int i = l; i < r; ++i) { s = MAX ? max(s, a[i]) : min(s, a[i]); }
            return tuple(s);}
        static bool ok(const auto &k, lng t) { return MAX ? std::get<0>(k) <= t : std::get<0>(k) >= t; }
    };
    template<bool MAX> struct AddIndex {
        using A = std::conditional_t<MAX, RangeAddRangeMaxIndex<lng>, RangeAddRangeMinIndex<lng>>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -3, 3); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { lng x = rnd(rng, -2, 2); for (int i = l; i < r; ++i) { a[i] += x; } return A::add(x); }
        static A::S leaf(int i, V v) { return A::leaf(i, v); }
        static auto key(const A::S &s) { return s.i < 0 ? tuple(lng(0), lng(-1)) : tuple(s.x, s.i); }
        static auto expect(const vector<V> &a, int l, int r) {
            tuple res(lng(0), lng(-1));
            for (int i = l; i < r; ++i) {
                if (std::get<1>(res) < 0 || (MAX ? a[i] > std::get<0>(res) : a[i] < std::get<0>(res))) { res = tuple(a[i], lng(i)); }}
            return res;}
        static bool ok(const auto &k, lng t) { return std::get<1>(k) < 0 || (MAX ? std::get<0>(k) <= t : std::get<0>(k) >= t); }
    };
    template<bool MAX> struct AddCount {
        using A = std::conditional_t<MAX, RangeAddRangeMaxCount<lng>, RangeAddRangeMinCount<lng>>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -2, 2); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { lng x = rnd(rng, -2, 2); for (int i = l; i < r; ++i) { a[i] += x; } return A::add(x); }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.cnt ? s.x : 0, s.cnt); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0)); }
            lng m = MAX ? *std::max_element(a.begin() + l, a.begin() + r) : *std::min_element(a.begin() + l, a.begin() + r);
            return tuple(m, lng(std::count(a.begin() + l, a.begin() + r, m)));}
        static bool ok(const auto &k, lng t) { return std::get<1>(k) == 0 || (MAX ? std::get<0>(k) <= t : std::get<0>(k) >= t); }
    };
    struct AffineSum {
        using A = RangeAffineRangeSum<mint>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, 0, P - 1); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, 0, P - 1), y = rnd(rng, 0, P - 1);
            for (int i = l; i < r; ++i) { a[i] = (mulmod(x, a[i]) + y) % P; }
            return A::affine(mint(x), mint(y));}
        static A::S leaf(int, V v) { return A::leaf(mint(v)); }
        static auto key(const A::S &s) { return tuple(lng(s.sum.val()), lng(s.len.val())); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s = (s + a[i]) % P; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    struct AffineSquares {
        using A = RangeAffineRangeSumSquares<mint>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, 0, P - 1); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, 0, P - 1), y = rnd(rng, 0, P - 1);
            for (int i = l; i < r; ++i) { a[i] = (mulmod(x, a[i]) + y) % P; }
            return A::affine(mint(x), mint(y));}
        static A::S leaf(int, V v) { return A::leaf(mint(v)); }
        static auto key(const A::S &s) { return tuple(lng(s.sum.val()), lng(s.sq.val()), lng(s.len.val())); }
        static auto expect(const vector<V> &a, int l, int r) {
            lng s = 0, q = 0;
            for (int i = l; i < r; ++i) { s = (s + a[i]) % P; q = (q + mulmod(a[i], a[i])) % P; }
            return tuple(s, q, lng(r - l));}
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
    };
    struct AssignSum {
        using A = RangeAssignRangeSum<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, 0, 6); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            if (rng() % 4 == 0) { return A::id(); }
            lng x = rnd(rng, 0, 6);
            for (int i = l; i < r; ++i) { a[i] = x; }
            return A::assign(x);}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.sum, s.len); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s += a[i]; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<0>(k) <= 2 * t; }
    };
    tuple<lng, lng, lng> composite(const vector<pair<lng, lng>> &a, int l, int r, bool backwards) {
        lng ca = 1, cb = 0;
        for (int k = 0; k < r - l; ++k) {
            auto [x, y] = a[backwards ? r - 1 - k : l + k];
            ca = mulmod(x, ca); cb = (mulmod(x, cb) + y) % P;}
        return tuple(ca, cb, lng(r - l));}
    template<typename A>
    typename A::F drawComposite(Rng &rng, vector<pair<lng, lng>> &a, int l, int r) {
        if (rng() % 4 == 0) { return A::id(); }
        lng x = rnd(rng, 0, P - 1), y = rnd(rng, 0, P - 1);
        for (int i = l; i < r; ++i) { a[i] = {x, y}; }
        return A::assign(mint(x), mint(y));}
    struct AssignComposite {
        using A = RangeAssignRangeComposite<mint>; using V = pair<lng, lng>;
        static V value(Rng &rng) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { return drawComposite<A>(rng, a, l, r); }
        static A::S leaf(int, V v) { return A::leaf(mint(v.first), mint(v.second)); }
        static auto key(const A::S &s) { return tuple(lng(s.a.val()), lng(s.b.val()), s.len); }
        static auto expect(const vector<V> &a, int l, int r) { return composite(a, l, r, false); }
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
    };
    struct OppositeComposite {
        using A = Opposite<RangeAssignRangeComposite<mint>>; using V = pair<lng, lng>;
        static V value(Rng &rng) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { return drawComposite<A>(rng, a, l, r); }
        static A::S leaf(int, V v) { return A::leaf(mint(v.first), mint(v.second)); }
        static auto key(const A::S &s) { return tuple(lng(s.a.val()), lng(s.b.val()), s.len); }
        static auto expect(const vector<V> &a, int l, int r) { return composite(a, l, r, true); }
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
    };
    struct ReversibleComposite {
        using B = RangeAssignRangeComposite<mint>;
        using A = Reversible<B>; using V = pair<lng, lng>;
        static V value(Rng &rng) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { return drawComposite<A>(rng, a, l, r); }
        static A::S leaf(int, V v) { return A::leaf(B::leaf(mint(v.first), mint(v.second))); }
        static auto key(const A::S &s) {
            auto t = A::reverse(A::reverse(s));
            return tuple(lng(t.x.a.val()), lng(t.x.b.val()), t.x.len, lng(A::reverse(s).x.a.val()), lng(A::reverse(s).x.b.val()), s.y.len);}
        static auto expect(const vector<V> &a, int l, int r) { return std::tuple_cat(composite(a, l, r, false), tuple(std::get<0>(composite(a, l, r, true)), std::get<1>(composite(a, l, r, true)), lng(r - l))); }
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
    };
    struct Arithmetic {
        using A = RangeArithmeticAddRangeSum<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng first = rnd(rng, -5, 5), d = rnd(rng, -3, 3);
            for (int i = l; i < r; ++i) { a[i] += first + d * (i - l); }
            return A::progression(l, first, d);}
        static A::S leaf(int i, V v) { return A::leaf(i, v); }
        static auto key(const A::S &s) { return tuple(s.sum, s.len, s.idx); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0, x = 0; for (int i = l; i < r; ++i) { s += a[i]; x += i; } return tuple(s, lng(r - l), x); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    struct SumMinMaxIndex {
        using A = RangeAffineRangeSumMinMaxIndex<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -4, 4); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, -1, 1), y = rnd(rng, -3, 3);
            for (int i = l; i < r; ++i) { a[i] = x * a[i] + y; }
            return A::affine(x, y);}
        static A::S leaf(int i, V v) { return A::leaf(i, v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.sum, s.mx, s.mn, s.len, s.mxi, s.mni) : tuple(lng(0), lng(0), lng(0), lng(0), lng(-1), lng(-1)); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0), lng(0), lng(0), lng(-1), lng(-1)); }
            lng s = 0, mx = a[l], mn = a[l], mxi = l, mni = l;
            for (int i = l; i < r; ++i) {
                s += a[i];
                if (a[i] > mx) { mx = a[i]; mxi = i; }
                if (a[i] < mn) { mn = a[i]; mni = i; }}
            return tuple(s, mx, mn, lng(r - l), mxi, mni);}
        static bool ok(const auto &k, lng t) { return std::get<3>(k) == 0 || (std::get<2>(k) >= t && std::get<1>(k) <= t + 6); }
    };
    // Bitwise action meaning on a value: kind 0 and, 1 or, 2 xor, 3 assign.
    ulng bitwiseMeaning(int kind, ulng v, ulng x) { return kind == 0 ? v & x : kind == 1 ? v | x : kind == 2 ? v ^ x : x; }
    template<typename A>
    typename A::F drawBitwise(Rng &rng, vector<ulng> &a, int l, int r, ulng mask) {
        ulng x = (rng() % 3 ? rng() % 16 : rng()) & mask;
        int kind = int(rng() % 4);
        for (int i = l; i < r; ++i) { a[i] = bitwiseMeaning(kind, a[i], x); }
        return kind == 0 ? A::andWith(x) : kind == 1 ? A::orWith(x) : kind == 2 ? A::xorWith(x) : A::assign(x);}
    struct AndOrXor {
        using A = RangeAndOrXorRangeAndOrXor<ulng>; using V = ulng;
        static V value(Rng &rng) { return rng() % 3 ? rng() % 16 : rng(); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { return drawBitwise<A>(rng, a, l, r, ~ulng(0)); }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.band, s.bor, s.bxor, s.len) : tuple(~ulng(0), ulng(0), ulng(0), lng(0)); }
        static auto expect(const vector<V> &a, int l, int r) {
            ulng x = ~ulng(0), y = 0, z = 0;
            for (int i = l; i < r; ++i) { x &= a[i]; y |= a[i]; z ^= a[i]; }
            return tuple(x, y, z, lng(r - l));}
        static bool ok(const auto &k, lng t) { return (std::get<0>(k) & ulng(t & 15)) == ulng(t & 15); }
    };
    template<int B> struct AndOrXorSum {
        using A = RangeAndOrXorRangeSum<ulng, B>; using V = ulng;
        static constexpr ulng MASK = B == 64 ? ~ulng(0) : (ulng(1) << B) - 1;
        static V value(Rng &rng) { return (rng() % 3 ? rng() % 16 : rng()) & MASK; }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { return drawBitwise<A>(rng, a, l, r, MASK); }
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(A::sum(s), s.len); }
        static auto expect(const vector<V> &a, int l, int r) { ulng s = 0; for (int i = l; i < r; ++i) { s += a[i]; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    template<bool LCM> struct GcdLcm {
        using A = RangeGcdLcmRangeGcdLcm<lng, LCM>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, 0, 12); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, 0, 12);
            int kind = int(rng() % 3);
            for (int i = l; i < r; ++i) { a[i] = kind == 0 ? gcd(a[i], x) : kind == 1 ? lcm(a[i], x) : x; }
            return kind == 0 ? A::gcdWith(x) : kind == 1 ? A::lcmWith(x) : A::assign(x);}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.g, s.l, s.len) : tuple(lng(0), lng(0), lng(0)); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0), lng(0)); }
            lng g = 0, m = 1;
            for (int i = l; i < r; ++i) { g = gcd(g, a[i]); m = lcm(m, a[i]); }
            return tuple(g, LCM ? m : 0, lng(r - l));}
        static bool ok(const auto &k, lng t) { return std::get<0>(k) % max<lng>(1, t % 7) == 0; }
    };
    tuple<lng, lng, lng, lng, lng> kadane(const vector<lng> &a, int l, int r) {
        if (l == r) { return tuple(lng(0), lng(0), lng(0), lng(0), lng(0)); }
        lng sum = 0, pre = a[l], suf = a[r - 1], best = a[l];
        for (int i = l; i < r; ++i) {
            sum += a[i]; pre = max(pre, sum);
            lng s = 0;
            for (int j = i; j < r; ++j) { s += a[j]; best = max(best, s); }}
        for (lng i = r - 1, s = 0; i >= l; --i) { s += a[i]; suf = max(suf, s); }
        return tuple(sum, pre, suf, best, lng(r - l));}
    struct AssignMaxSubarray {
        using A = RangeAssignRangeMaxSubarray<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -6, 6); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            if (rng() % 4 == 0) { return A::id(); }
            lng x = rnd(rng, -6, 6);
            for (int i = l; i < r; ++i) { a[i] = x; }
            return A::assign(x);}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return s.len ? tuple(s.sum, s.pre, s.suf, s.best, s.len) : tuple(lng(0), lng(0), lng(0), lng(0), lng(0)); }
        static auto expect(const vector<V> &a, int l, int r) { return kadane(a, l, r); }
        static bool ok(const auto &k, lng t) { return std::get<4>(k) == 0 || std::get<3>(k) <= t; }
    };
    tuple<int, int, lng> runs(const vector<int> &a, int l, int r) {
        if (l == r) { return tuple(0, 0, lng(0)); }
        lng c = 1;
        for (int i = l + 1; i < r; ++i) { c += a[i] != a[i - 1]; }
        return tuple(a[l], a[r - 1], c);}
    struct AssignRunCount {
        using A = RangeAssignRangeRunCount<int>; using V = int;
        static V value(Rng &rng) { return int(rnd(rng, 0, 2)); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            if (rng() % 4 == 0) { return A::id(); }
            int x = int(rnd(rng, 0, 2));
            for (int i = l; i < r; ++i) { a[i] = x; }
            return A::assign(x);}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return s.runs ? tuple(s.first, s.last, s.runs) : tuple(0, 0, lng(0)); }
        static auto expect(const vector<V> &a, int l, int r) { return runs(a, l, r); }
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
    };
    struct Clamp {
        using A = RangeAddChminChmaxRangeMinMax<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, -9, 9);
            int kind = int(rng() % 5);
            for (int i = l; i < r; ++i) { a[i] = kind == 0 ? a[i] + x / 3 : kind == 1 ? min(a[i], x) : kind == 2 ? max(a[i], x) : kind == 3 ? x : a[i]; }
            return kind == 0 ? A::add(x / 3) : kind == 1 ? A::chmin(x) : kind == 2 ? A::chmax(x) : kind == 3 ? A::assign(x) : A::id();}
        static A::S leaf(int, V v) { return MinMax<lng>::leaf(v); }
        static auto key(const A::S &s) { return s.mx < s.mn ? tuple(lng(0), lng(0), false) : tuple(s.mn, s.mx, true); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0), false); }
            return tuple(*std::min_element(a.begin() + l, a.begin() + r), *std::max_element(a.begin() + l, a.begin() + r), true);}
        static bool ok(const auto &k, lng t) { return !std::get<2>(k) || std::get<1>(k) <= t; }
    };
    struct FlipInversion {
        using A = RangeFlipRangeInversion; using V = int;
        static V value(Rng &rng) { return int(rng() % 2); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            if (rng() % 3 == 0) { return A::id(); }
            for (int i = l; i < r; ++i) { a[i] ^= 1; }
            return A::flip();}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return tuple(s.zero, s.one, s.inv); }
        static auto expect(const vector<V> &a, int l, int r) {
            lng z = 0, o = 0, inv = 0;
            for (int i = l; i < r; ++i) {
                for (int j = i + 1; j < r; ++j) { inv += a[i] > a[j]; }
                (a[i] ? o : z) += 1;}
            return tuple(z, o, inv);}
        static bool ok(const auto &k, lng t) { return std::get<2>(k) <= t; }
    };
    tuple<lng, lng, lng, lng, lng> longest(const vector<int> &a, int l, int r, int one) {
        lng ones = 0, best = 0, cur = 0, pre = 0, suf = 0;
        for (int i = l; i < r; ++i) {
            bool b = a[i] == one; ones += b; cur = b ? cur + 1 : 0; best = max(best, cur);}
        while (l + pre < r && a[l + pre] == one) { ++pre; }
        while (suf < r - l && a[r - 1 - suf] == one) { ++suf; }
        return tuple(lng(r - l), ones, pre, suf, best);}
    struct AssignFlipRun {
        using A = RangeAssignFlip<LongestRun>; using V = int;
        static V value(Rng &rng) { return int(rng() % 2); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            int kind = int(rng() % 4);
            for (int i = l; i < r; ++i) { a[i] = kind == 0 ? 0 : kind == 1 ? 1 : kind == 2 ? a[i] ^ 1 : a[i]; }
            return kind < 2 ? A::assign(kind == 1) : kind == 2 ? A::flip() : A::id();}
        static A::S leaf(int, V v) { return A::leaf(v); }
        static auto key(const A::S &s) { return std::tuple_cat(tuple(s.x.len, s.x.ones, s.x.pre, s.x.suf, s.x.best), tuple(s.y.len, s.y.ones, s.y.pre, s.y.suf, s.y.best), tuple(s.len)); }
        static auto expect(const vector<V> &a, int l, int r) { return std::tuple_cat(longest(a, l, r, 1), longest(a, l, r, 0), tuple(lng(r - l))); }
        static bool ok(const auto &k, lng t) { return std::get<4>(k) <= t; }
    };
    struct Linear {
        using A = RangeLinearRangeSum<mint, 3>; using V = array<lng, 3>;
        static V value(Rng &rng) { return {1, rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            Mat m{};
            m[0][0] = 1;
            for (int i = 1; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) { m[i][j] = rng() % 3 ? rnd(rng, 0, 3) : rnd(rng, 0, P - 1); }}
            for (int k = l; k < r; ++k) {
                V v{};
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) { v[i] = (v[i] + mulmod(m[i][j], a[k][j])) % P; }}
                a[k] = v;}
            A::F f;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) { f[i][j] = mint(m[i][j]); }}
            return f;}
        static A::S leaf(int, V v) { return {mint(v[0]), mint(v[1]), mint(v[2])}; }
        static auto key(const A::S &s) { return tuple(lng(s[0].val()), lng(s[1].val()), lng(s[2].val())); }
        static auto expect(const vector<V> &a, int l, int r) {
            V s{};
            for (int i = l; i < r; ++i) {
                for (int j = 0; j < 3; ++j) { s[j] = (s[j] + a[i][j]) % P; }}
            return tuple(s[0], s[1], s[2]);}
        static bool ok(const auto &k, lng t) { return std::get<0>(k) <= t; }
    };
    // Function-adapter and stateful acted monoids: range add range min through lambdas, affine sums modulo a runtime runtimeMod.
    inline constexpr auto addMin = [](lng f, lng x) { return x == std::numeric_limits<lng>::max() ? x : x + f; };
    inline constexpr auto addAdd = [](lng f, lng g) { return f + g; };
    struct LambdaMin {
        using A = ActedMonoid<Min<lng>, lng, std::decay_t<decltype(addMin)>, std::decay_t<decltype(addAdd)>>; using V = lng;
        static A make() { return ActedMonoid{Min<lng>{}, lng(0), addMin, addAdd}; }
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static A::F draw(Rng &rng, vector<V> &a, int l, int r) { lng x = rnd(rng, -4, 4); for (int i = l; i < r; ++i) { a[i] += x; } return x; }
        static A::S leaf(int, V v) { return v; }
        static auto key(const A::S &s) { return tuple(s); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = std::numeric_limits<lng>::max(); for (int i = l; i < r; ++i) { s = min(s, a[i]); } return tuple(s); }
        static bool ok(const auto &k, lng t) { return std::get<0>(k) >= t; }
    };
    lng runtimeMod = 1000000007;
    struct ModAffine {
        lng mod;
        struct S { lng sum, len; };
        struct F { lng a, b; };
        S op(const S &x, const S &y) const { return {(x.sum + y.sum) % mod, x.len + y.len}; }
        S e() const { return {0, 0}; }
        S mapping(const F &f, const S &x) const { return {lng((lll(f.a) * x.sum + lll(f.b) * x.len) % mod), x.len}; }
        F composition(const F &f, const F &g) const { return {lng(lll(f.a) * g.a % mod), lng((lll(f.a) * g.b + f.b) % mod)}; }
        F id() const { return {1, 0}; }
    };
    template<typename A>
    struct ModSpec {
        using V = lng;
        static V value(Rng &rng) { return rnd(rng, 0, runtimeMod - 1); }
        static typename A::F draw(Rng &rng, vector<V> &a, int l, int r) {
            lng x = rnd(rng, 0, runtimeMod - 1), y = rnd(rng, 0, runtimeMod - 1);
            for (int i = l; i < r; ++i) { a[i] = lng((lll(x) * a[i] + y) % runtimeMod); }
            return {x, y};}
        static typename A::S leaf(int, V v) { return {v, 1}; }
        static auto key(const typename A::S &s) { return tuple(s.sum, s.len); }
        static auto expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s = (s + a[i]) % runtimeMod; } return tuple(s, lng(r - l)); }
        static bool ok(const auto &k, lng t) { return std::get<1>(k) <= t; }
    };
    struct StatefulAffine : ModSpec<ModAffine> {
        using A = ModAffine;
        static A make() { return {runtimeMod}; }
    };
    auto lambdaAffine(lng mod) {
        auto op = [mod](const ModAffine::S &x, const ModAffine::S &y) { return ModAffine::S{(x.sum + y.sum) % mod, x.len + y.len}; };
        auto act = [mod](const ModAffine::F &f, const ModAffine::S &x) { return ModAffine::S{lng((lll(f.a) * x.sum + lll(f.b) * x.len) % mod), x.len}; };
        auto compose = [mod](const ModAffine::F &f, const ModAffine::F &g) { return ModAffine::F{lng(lll(f.a) * g.a % mod), lng((lll(f.a) * g.b + f.b) % mod)}; };
        return ActedMonoid{Monoid{ModAffine::S{0, 0}, op}, ModAffine::F{1, 0}, act, compose};}
    using LambdaAffine = decltype(lambdaAffine(0));
    struct CapturingAffine : ModSpec<LambdaAffine> {
        using A = LambdaAffine;
        static A make() { return lambdaAffine(runtimeMod); }
    };

    template<typename C>
    typename C::A makeMonoid() {
        if constexpr (requires { C::make(); }) { return C::make(); }
        else { return typename C::A{}; }}

    template<typename C>
    void history(Rng &rng, const string &name, int n, int steps) {
        using A = typename C::A;
        using S = typename A::S;
        vector<typename C::V> a(n);
        vector<S> leaves;
        for (int i = 0; i < n; ++i) { a[i] = C::value(rng); leaves.push_back(C::leaf(i, a[i])); }
        LazySegmentTree<A> t(leaves, makeMonoid<C>());
        context = name + " n=" + show(n) + " seed=" + show(seed);
        bool all = n <= 17;
        auto verify = [&](const string &when) {
            if (all) {
                for (int l = 0; l <= n; ++l) {
                    for (int r = l; r <= n; ++r) { checkEqual(C::key(t.prod(l, r)), C::expect(a, l, r), when + " prod(" + show(l) + "," + show(r) + ")"); }}}
            checkEqual(C::key(t.allProd()), C::expect(a, 0, n), when + " allProd");};
        verify("build");
        for (int step = 0; step < steps; ++step) {
            int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
            if (l > r) { swap(l, r); }
            int p = n ? int(rnd(rng, 0, n - 1)) : 0;
            string at = "step=" + show(step);
            switch (rng() % 9) {
                case 0:
                    if (n) { a[p] = C::value(rng); t.set(p, C::leaf(p, a[p])); }
                    break;
                case 1:
                    if (n) { checkEqual(C::key(t.get(p)), C::expect(a, p, p + 1), at + " get(" + show(p) + ")"); }
                    break;
                case 2:
                    if (n) { t.apply(p, C::draw(rng, a, p, p + 1)); }
                    break;
                case 3: case 4:
                    t.apply(l, r, C::draw(rng, a, l, r));
                    break;
                case 5:
                    checkEqual(C::key(t.prod(l, r)), C::expect(a, l, r), at + " prod(" + show(l) + "," + show(r) + ")");
                    break;
                case 6: {
                    lng th = rnd(rng, -6, n + 6);
                    if (!C::ok(C::expect(a, l, l), th)) { break; }
                    int want = l;
                    while (want < n && C::ok(C::expect(a, l, want + 1), th)) { ++want; }
                    checkEqual(t.maxRight(l, [&](const S &s) { return C::ok(C::key(s), th); }), want, at + " maxRight(" + show(l) + ",t=" + show(th) + ")");
                    break;}
                case 7: {
                    lng th = rnd(rng, -6, n + 6);
                    if (!C::ok(C::expect(a, r, r), th)) { break; }
                    int want = r;
                    while (want > 0 && C::ok(C::expect(a, want - 1, r), th)) { --want; }
                    checkEqual(t.minLeft(r, [&](const S &s) { return C::ok(C::key(s), th); }), want, at + " minLeft(" + show(r) + ",t=" + show(th) + ")");
                    break;}
                default: {
                    auto v = t.values();
                    checkEqual(int(v.size()), n, at + " values size");
                    for (int i = 0; i < n; ++i) { checkEqual(C::key(v[i]), C::expect(a, i, i + 1), at + " values[" + show(i) + "]"); }}}
            if (all) { verify(at); }
            else { checkEqual(C::key(t.allProd()), C::expect(a, 0, n), at + " allProd"); }}}

    vector<int> sizes() {
        if (mode == "quick") { return {0, 1, 2, 3, 5, 8, 9}; }
        vector<int> s{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 17, 31, 32, 33, 100};
        if (mode == "stress") { s.push_back(257); }
        return s;}
    template<typename C>
    void preset(Rng &rng, const string &name) {
        int steps = mode == "quick" ? 60 : mode == "full" ? 300 : 600;
        for (int n : sizes()) { history<C>(rng, name, n, n > 17 ? 4 * steps : steps); }
        std::cout << "PASS lazy " << name << '\n';}

    // Plain specs: SegmentTree<M> over leaf(i, v) against the brute meaning of the raw values.
    template<typename T, typename M, lng LO, lng HI> struct Numeric {
        using V = lng;
        static V value(Rng &rng) { return rnd(rng, LO, HI); }
        static typename M::S leaf(int, V v) { return T(v); }
    };
    struct SumP : Numeric<lng, Sum<lng>, -9, 9> {
        using M = Sum<lng>;
        static auto key(lng s) { return s; }
        static lng expect(const vector<V> &a, int l, int r) { return std::accumulate(a.begin() + l, a.begin() + r, lng(0)); }
    };
    struct MulP {
        using M = Mul<mint>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, 0, P - 1); }
        static M::S leaf(int, V v) { return mint(v); }
        static lng key(const mint &s) { return s.val(); }
        static lng expect(const vector<V> &a, int l, int r) { lng s = 1; for (int i = l; i < r; ++i) { s = mulmod(s, a[i]); } return s; }
    };
    template<bool MAX> struct ExtremeP : Numeric<lng, Min<lng>, -9, 9> {
        using M = std::conditional_t<MAX, Max<lng>, Min<lng>>;
        static lng key(lng s) { return s; }
        static lng expect(const vector<V> &a, int l, int r) {
            lng s = MAX ? std::numeric_limits<lng>::lowest() : std::numeric_limits<lng>::max();
            for (int i = l; i < r; ++i) { s = MAX ? max(s, a[i]) : min(s, a[i]); }
            return s;}
    };
    struct MinMaxP {
        using M = MinMax<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return s.mx < s.mn ? tuple(lng(0), lng(0), false) : tuple(s.mn, s.mx, true); }
        static auto expect(const vector<V> &a, int l, int r) { return Clamp::expect(a, l, r); }
    };
    template<int KIND> struct BitsP {
        using M = std::conditional_t<KIND == 0, And<ulng>, std::conditional_t<KIND == 1, Or<ulng>, Xor<ulng>>>; using V = ulng;
        static V value(Rng &rng) { return rng() % 2 ? rng() % 16 : rng(); }
        static M::S leaf(int, V v) { return v; }
        static ulng key(ulng s) { return s; }
        static ulng expect(const vector<V> &a, int l, int r) {
            ulng s = KIND == 0 ? ~ulng(0) : 0;
            for (int i = l; i < r; ++i) { s = KIND == 0 ? s & a[i] : KIND == 1 ? s | a[i] : s ^ a[i]; }
            return s;}
    };
    lng euclid(lng a, lng b) { a = a < 0 ? -a : a; b = b < 0 ? -b : b; while (b) { a %= b; swap(a, b); } return a; }
    struct GcdP : Numeric<lng, Gcd<lng>, 0, 30> {
        using M = Gcd<lng>;
        static lng key(lng s) { return s; }
        static lng expect(const vector<V> &a, int l, int r) { lng g = 0; for (int i = l; i < r; ++i) { g = euclid(g, a[i]); } return g; }
    };
    struct LcmP : Numeric<lng, Lcm<lng>, 1, 6> {
        using M = Lcm<lng>;
        static lng key(lng s) { return s; }
        static lng expect(const vector<V> &a, int l, int r) {
            lng m = 1;
            for (int i = l; i < r; ++i) { m = m / euclid(m, a[i]) * a[i]; }
            return m;}
    };
    struct AffineP {
        using M = AffineComposite<mint>; using V = pair<lng, lng>;
        static V value(Rng &rng) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
        static M::S leaf(int, V v) { return {mint(v.first), mint(v.second)}; }
        static auto key(const M::S &s) { return tuple(lng(s.a.val()), lng(s.b.val()), lng(M::eval(s, mint(12345)).val())); }
        static auto expect(const vector<V> &a, int l, int r) {
            auto [ca, cb, len] = composite(a, l, r, false);
            lng x = 12345;
            for (int i = l; i < r; ++i) { x = (mulmod(a[i].first, x) + a[i].second) % P; }
            return tuple(ca, cb, x);}
    };
    // A clamp chain value: kind 0 add, 1 chmin, 2 chmax, 3 assign; probes from -40 to 40 determine the slope-1 clamp map.
    struct ClampP {
        using M = AddChminChmax<lng>; using V = pair<int, lng>;
        static V value(Rng &rng) { return {int(rng() % 4), rnd(rng, -9, 9)}; }
        static M::S leaf(int, V v) { return v.first == 0 ? M::add(v.second) : v.first == 1 ? M::chmin(v.second) : v.first == 2 ? M::chmax(v.second) : M::assign(v.second); }
        static vector<lng> key(const M::S &s) { vector<lng> res; for (lng x = -40; x <= 40; ++x) { res.push_back(M::eval(s, x)); } return res; }
        static vector<lng> expect(const vector<V> &a, int l, int r) {
            vector<lng> res;
            for (lng x = -40; x <= 40; ++x) {
                lng y = x;
                for (int i = l; i < r; ++i) {
                    auto [k, c] = a[i];
                    y = k == 0 ? y + c : k == 1 ? min(y, c) : k == 2 ? max(y, c) : c;}
                res.push_back(y);}
            return res;}
    };
    struct BitwiseP {
        using M = BitwiseComposite<ulng>; using V = pair<int, ulng>;
        static V value(Rng &rng) { return {int(rng() % 4), rng() % 2 ? rng() % 16 : rng()}; }
        static M::S leaf(int, V v) { return v.first == 0 ? M::andWith(v.second) : v.first == 1 ? M::orWith(v.second) : v.first == 2 ? M::xorWith(v.second) : M::assign(v.second); }
        static auto key(const M::S &s) { return tuple(M::eval(s, 0), M::eval(s, ~ulng(0)), M::eval(s, 0x5a5a5a5a5a5a5a5aULL)); }
        static auto expect(const vector<V> &a, int l, int r) {
            array<ulng, 3> x{0, ~ulng(0), 0x5a5a5a5a5a5a5a5aULL};
            for (ulng &y : x) {
                for (int i = l; i < r; ++i) { y = bitwiseMeaning(a[i].first, y, a[i].second); }}
            return tuple(x[0], x[1], x[2]);}
    };
    template<bool MAX, bool RIGHT> struct IndexP {
        using M = std::conditional_t<RIGHT, Opposite<MinIndex<lng>>, std::conditional_t<MAX, MaxIndex<lng>, MinIndex<lng>>>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -3, 3); }
        static M::S leaf(int i, V v) { return M::leaf(i, v); }
        static auto key(const M::S &s) { return s.i < 0 ? tuple(lng(0), lng(-1)) : tuple(s.x, s.i); }
        static auto expect(const vector<V> &a, int l, int r) {
            tuple res(lng(0), lng(-1));
            for (int i = l; i < r; ++i) {
                bool better = MAX ? a[i] > std::get<0>(res) : RIGHT ? a[i] <= std::get<0>(res) : a[i] < std::get<0>(res);
                if (std::get<1>(res) < 0 || better) { res = tuple(a[i], lng(i)); }}
            return res;}
    };
    template<bool MAX> struct CountP {
        using M = std::conditional_t<MAX, MaxCount<lng>, MinCount<lng>>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -2, 2); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return tuple(s.cnt ? s.x : 0, s.cnt); }
        static auto expect(const vector<V> &a, int l, int r) { return AddCount<MAX>::expect(a, l, r); }
    };
    struct Max2P {
        using M = Max2<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -3, 3); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return tuple(s.c1 ? s.x1 : 0, s.c1, s.c2 ? s.x2 : 0, s.c2); }
        static auto expect(const vector<V> &a, int l, int r) {
            map<lng, lng> cnt;
            for (int i = l; i < r; ++i) { ++cnt[a[i]]; }
            auto it = cnt.rbegin();
            lng x1 = 0, c1 = 0, x2 = 0, c2 = 0;
            if (it != cnt.rend()) { x1 = it->first; c1 = it->second; ++it; }
            if (it != cnt.rend()) { x2 = it->first; c2 = it->second; }
            return tuple(x1, c1, x2, c2);}
    };
    struct MaxSubarrayP {
        using M = MaxSubarray<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -6, 6); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return AssignMaxSubarray::key(s); }
        static auto expect(const vector<V> &a, int l, int r) { return kadane(a, l, r); }
    };
    struct MaxGainP {
        using M = MaxGain<lng>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return s.mx < s.mn ? tuple(lng(0), lng(0), lng(0), false) : tuple(s.mn, s.mx, s.best, true); }
        static auto expect(const vector<V> &a, int l, int r) {
            if (l == r) { return tuple(lng(0), lng(0), lng(0), false); }
            lng best = 0;
            for (int i = l; i < r; ++i) {
                for (int j = i; j < r; ++j) { best = max(best, a[j] - a[i]); }}
            return tuple(*std::min_element(a.begin() + l, a.begin() + r), *std::max_element(a.begin() + l, a.begin() + r), best, true);}
    };
    struct RunCountP {
        using M = RunCount<int>; using V = int;
        static V value(Rng &rng) { return int(rnd(rng, 0, 2)); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return AssignRunCount::key(s); }
        static auto expect(const vector<V> &a, int l, int r) { return runs(a, l, r); }
    };
    struct LongestRunP {
        using M = LongestRun; using V = int;
        static V value(Rng &rng) { return int(rng() % 4 != 0); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return tuple(s.len, s.ones, s.pre, s.suf, s.best); }
        static auto expect(const vector<V> &a, int l, int r) { return longest(a, l, r, 1); }
    };
    struct BracketP {
        using M = BracketBalance; using V = int;
        static V value(Rng &rng) { return int(rng() % 2); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static auto key(const M::S &s) { return tuple(s.open, s.close); }
        static auto expect(const vector<V> &a, int l, int r) {
            lng open = 0, close = 0;
            for (int i = l; i < r; ++i) {
                if (a[i]) { ++open; }
                else if (open) { --open; }
                else { ++close; }}
            return tuple(open, close);}
    };
    struct MatrixP {
        using M = MatrixProduct<mint, 3>; using V = Mat;
        static V value(Rng &rng) { V m; for (auto &row : m) { for (lng &x : row) { x = rng() % 2 ? rnd(rng, 0, 3) : rnd(rng, 0, P - 1); }} return m; }
        static M::S leaf(int, const V &v) { M::S m; for (int i = 0; i < 3; ++i) { for (int j = 0; j < 3; ++j) { m[i][j] = mint(v[i][j]); }} return m; }
        static Mat key(const M::S &s) { Mat m; for (int i = 0; i < 3; ++i) { for (int j = 0; j < 3; ++j) { m[i][j] = s[i][j].val(); }} return m; }
        static Mat expect(const vector<V> &a, int l, int r) {
            Mat c{};
            for (int i = 0; i < 3; ++i) { c[i][i] = 1; }
            for (int k = l; k < r; ++k) {
                Mat d{};
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        for (int t = 0; t < 3; ++t) { d[i][j] = (d[i][j] + mulmod(c[i][t], a[k][t][j])) % P; }}}
                c = d;}
            return c;}
    };
    struct MaxPlusP {
        using M = MaxPlusMatrix<lng, 3>; using V = Mat;
        static constexpr lng NO = std::numeric_limits<lng>::lowest();
        static V value(Rng &rng) { V m; for (auto &row : m) { for (lng &x : row) { x = rng() % 4 ? rnd(rng, -9, 9) : NO; }} return m; }
        static M::S leaf(int, const V &v) { return v; }
        static Mat key(const M::S &s) { return s; }
        static Mat expect(const vector<V> &a, int l, int r) {
            Mat c;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) { c[i][j] = i == j ? 0 : NO; }}
            for (int k = l; k < r; ++k) {
                Mat d;
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        d[i][j] = NO;
                        for (int t = 0; t < 3; ++t) {
                            if (c[i][t] != NO && a[k][t][j] != NO) { d[i][j] = max(d[i][j], c[i][t] + a[k][t][j]); }}}}
                c = d;}
            return c;}
    };
    struct MajorityP {
        using M = MajorityVote<int>; using V = int;
        static V value(Rng &rng) { return rng() % 3 ? 1 : int(rnd(rng, 0, 3)); }
        static M::S leaf(int, V v) { return M::leaf(v); }
        static int key(const M::S &s) { return s.x; }
        static int expect(const vector<V> &a, int l, int r) {
            for (int i = l; i < r; ++i) {
                if (2 * std::count(a.begin() + l, a.begin() + r, a[i]) > r - l) { return a[i]; }}
            return -1;}
        static bool skip(int want) { return want < 0; }
    };
    inline constexpr auto plusL = [](lng x, lng y) { return x + y; };
    struct LambdaP : Numeric<lng, Sum<lng>, -9, 9> {
        using M = Monoid<lng, std::decay_t<decltype(plusL)>>;
        static M make() { return Monoid{lng(0), plusL}; }
        static lng key(lng s) { return s; }
        static lng expect(const vector<V> &a, int l, int r) { return SumP::expect(a, l, r); }
    };
    auto modPlus(lng mod) { return Monoid{lng(0), [mod](lng x, lng y) { return (x + y) % mod; }}; }
    struct CapturingP {
        using M = decltype(modPlus(0)); using V = lng;
        static M make() { return modPlus(runtimeMod); }
        static V value(Rng &rng) { return rnd(rng, 0, runtimeMod - 1); }
        static M::S leaf(int, V v) { return v; }
        static lng key(lng s) { return s; }
        static lng expect(const vector<V> &a, int l, int r) { lng s = 0; for (int i = l; i < r; ++i) { s = (s + a[i]) % runtimeMod; } return s; }
    };
    struct PairP {
        using M = PairMonoid<Sum<lng>, Max<lng>>; using V = lng;
        static V value(Rng &rng) { return rnd(rng, -9, 9); }
        static M::S leaf(int, V v) { return {v, -v}; }
        static auto key(const M::S &s) { return s; }
        static auto expect(const vector<V> &a, int l, int r) {
            lng mx = std::numeric_limits<lng>::lowest();
            for (int i = l; i < r; ++i) { mx = max(mx, -a[i]); }
            return pair(SumP::expect(a, l, r), mx);}
    };
    struct ArrayP {
        using M = ArrayMonoid<Min<lng>, 3>; using V = array<lng, 3>;
        static V value(Rng &rng) { return {rnd(rng, -9, 9), rnd(rng, -9, 9), rnd(rng, -9, 9)}; }
        static M::S leaf(int, V v) { return v; }
        static auto key(const M::S &s) { return s; }
        static auto expect(const vector<V> &a, int l, int r) {
            V s; s.fill(std::numeric_limits<lng>::max());
            for (int i = l; i < r; ++i) { for (int j = 0; j < 3; ++j) { s[j] = min(s[j], a[i][j]); }}
            return s;}
    };
    inline constexpr auto concat = [](const string &x, const string &y) { return x + y; };
    using Concat = Monoid<string, std::decay_t<decltype(concat)>>;
    template<bool OPPOSITE> struct StringP {
        using M = std::conditional_t<OPPOSITE, Opposite<Concat>, Reversible<Concat>>; using V = string;
        static M make() { return M{Concat{string(), concat}}; }
        static V value(Rng &rng) { return string(1, char('a' + rng() % 3)); }
        static M::S leaf(int, V v) {
            if constexpr (OPPOSITE) { return v; }
            else { return M::leaf(v); }}
        static auto key(const M::S &s) {
            if constexpr (OPPOSITE) { return pair(s, s); }
            else { return pair(s.x, M::reverse(s).x); }}
        static auto expect(const vector<V> &a, int l, int r) {
            string f, b;
            for (int i = l; i < r; ++i) { f += a[i]; b = a[i] + b; }
            return OPPOSITE ? pair(b, b) : pair(f, b);}
    };

    template<typename C>
    void plain(Rng &rng, const string &name) {
        using M = typename C::M;
        using S = typename M::S;
        int steps = mode == "quick" ? 40 : mode == "full" ? 200 : 400;
        for (int n : sizes()) {
            context = name + " n=" + show(n) + " seed=" + show(seed);
            vector<typename C::V> a(n);
            vector<S> leaves;
            for (int i = 0; i < n; ++i) { a[i] = C::value(rng); leaves.push_back(C::leaf(i, a[i])); }
            M m = [] { if constexpr (requires { C::make(); }) { return C::make(); } else { return M{}; }}();
            SegmentTree<M> t(leaves, m);
            auto same = [&](int l, int r, const string &what) {
                auto want = C::expect(a, l, r);
                if constexpr (requires { C::skip(want); }) {
                    if (C::skip(want)) { return; }}
                checkEqual(C::key(t.prod(l, r)), want, what + " prod(" + show(l) + "," + show(r) + ")");};
            for (int step = 0; step <= steps; ++step) {
                if (n <= 17) {
                    for (int l = 0; l <= n; ++l) {
                        for (int r = l; r <= n; ++r) { same(l, r, "step=" + show(step)); }}}
                else {
                    int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                    same(min(l, r), max(l, r), "step=" + show(step)); same(0, n, "step=" + show(step));}
                if (n) {
                    int p = int(rnd(rng, 0, n - 1));
                    S x = C::leaf(p, a[p]);
                    checkEqual(C::key(m.op(m.e(), x)), C::key(x), "left identity");
                    checkEqual(C::key(m.op(x, m.e())), C::key(x), "right identity");
                    if constexpr (requires { m.inv(x); }) {
                        checkEqual(C::key(m.op(x, m.inv(x))), C::key(m.e()), "right inverse");
                        checkEqual(C::key(m.op(m.inv(x), x)), C::key(m.e()), "left inverse");}
                    a[p] = C::value(rng); t.set(p, C::leaf(p, a[p]));}}}
        std::cout << "PASS plain " << name << '\n';}

    // MonoidAction: plain presets and a capturing Monoid as dual-tree actions, checked through eval on per-point start values.
    void monoidActions(Rng &rng) {
        int steps = mode == "quick" ? 100 : mode == "full" ? 800 : 2000;
        for (int n : sizes()) {
            context = "MonoidAction n=" + show(n);
            DualSegmentTree<MonoidAction<AddChminChmax<lng>>> clamp(n);
            DualSegmentTree<MonoidAction<BitwiseComposite<ulng>>> bits(n);
            DualSegmentTree<MonoidAction<AffineComposite<mint>>> affine(n);
            CommutativeDualSegmentTree<MonoidAction<Max<lng>>> chmax(n);
            DualSegmentTree modAdd(n, MonoidAction{modPlus(runtimeMod)});
            vector<lng> x0(n), cl(n), mx(n, std::numeric_limits<lng>::lowest()), md(n, 0), af(n);
            vector<ulng> b0(n), bt(n);
            for (int i = 0; i < n; ++i) { x0[i] = cl[i] = rnd(rng, -9, 9); b0[i] = bt[i] = rng(); af[i] = rnd(rng, 0, P - 1); }
            vector<lng> afStart = af;
            for (int step = 0; step < steps; ++step) {
                int l = int(rnd(rng, 0, n)), r = int(rnd(rng, 0, n));
                if (l > r) { swap(l, r); }
                lng c = rnd(rng, -9, 9);
                int kind = int(rng() % 4);
                using C = AddChminChmax<lng>;
                clamp.apply(l, r, kind == 0 ? C::add(c / 3) : kind == 1 ? C::chmin(c) : kind == 2 ? C::chmax(c) : C::assign(c));
                for (int i = l; i < r; ++i) { cl[i] = kind == 0 ? cl[i] + c / 3 : kind == 1 ? min(cl[i], c) : kind == 2 ? max(cl[i], c) : c; }
                ulng y = rng() % 2 ? rng() : rng() % 16;
                using W = BitwiseComposite<ulng>;
                bits.apply(l, r, kind == 0 ? W::andWith(y) : kind == 1 ? W::orWith(y) : kind == 2 ? W::xorWith(y) : W::assign(y));
                for (int i = l; i < r; ++i) { bt[i] = bitwiseMeaning(kind, bt[i], y); }
                lng fa = rnd(rng, 0, P - 1), fb = rnd(rng, 0, P - 1);
                affine.apply(l, r, {mint(fa), mint(fb)});
                for (int i = l; i < r; ++i) { af[i] = (mulmod(fa, af[i]) + fb) % P; }
                chmax.apply(l, r, c);
                for (int i = l; i < r; ++i) { mx[i] = max(mx[i], c); }
                lng add = rnd(rng, 0, runtimeMod - 1);
                modAdd.apply(l, r, add);
                for (int i = l; i < r; ++i) { md[i] = (md[i] + add) % runtimeMod; }
                for (int i = 0; i < n; ++i) {
                    string at = "step=" + show(step) + " i=" + show(i);
                    checkEqual(C::eval(clamp.get(i), x0[i]), cl[i], at + " clamp");
                    checkEqual(W::eval(bits.get(i), b0[i]), bt[i], at + " bitwise");
                    checkEqual(lng(AffineComposite<mint>::eval(affine.get(i), mint(afStart[i])).val()), af[i], at + " affine");
                    checkEqual(chmax.get(i), mx[i], at + " chmax");
                    checkEqual(modAdd.get(i), md[i], at + " capturing add");}}}
        std::cout << "PASS MonoidAction on dual trees\n";}

    // Powers inside assignments and the empty-monoid layout of consumers.
    void contract() {
        context = "contract";
        using C = RangeAssignRangeComposite<mint>;
        LazySegmentTree<C> big(vector<C::S>(1000, C::leaf(1, 0)));
        big.apply(0, 1000, C::assign(mint(3), mint(1)));
        lng a = 1, b = 0;
        for (int i = 0; i < 1000; ++i) { a = 3 * a % P; b = (3 * b + 1) % P; }
        checkEqual(tuple(lng(big.allProd().a.val()), lng(big.allProd().b.val())), tuple(a, b), "composite power 1000");
        LazySegmentTree<RangeAssignFlip<LongestRun>> runs(vector<RangeAssignFlip<LongestRun>::S>(777, RangeAssignFlip<LongestRun>::leaf(false)));
        runs.apply(5, 705, RangeAssignFlip<LongestRun>::assign(true));
        checkEqual(tuple(runs.allProd().x.best, runs.allProd().y.best, runs.allProd().x.ones), tuple(lng(700), lng(72), lng(700)), "assign power 700");
        using A = RangeAddRangeSum<lng>;
        struct Layout { int n, lg, base; vector<A::S> d; vector<A::F> lz; };
        struct SegLayout { int n, base; vector<lng> tree; };
        checkEqual(sizeof(LazySegmentTree<A>), sizeof(Layout), "empty acted monoid adds no size");
        checkEqual(sizeof(SegmentTree<Sum<lng>>), sizeof(SegLayout), "empty monoid adds no size");
        checkEqual(sizeof(DualSegmentTree<MonoidAction<Max<lng>>>), sizeof(DualSegmentTree<A>), "empty MonoidAction adds no size");
        Sum<ulng> su; Sum<uint8_t> s8; Sum<mint> sm; Sum<lng> sl; Xor<bool> xb;
        checkEqual(pair(su.inv(5), su.op(5, su.inv(5))), pair(ulng(0) - 5, ulng(0)), "Sum<ulng> inverse wraps");
        checkEqual(pair(int(s8.inv(3)), int(s8.op(3, s8.inv(3)))), pair(253, 0), "Sum<uint8_t> inverse wraps");
        checkEqual(pair(lng(sm.inv(mint(5)).val()), lng(sm.op(mint(5), sm.inv(mint(5))).val())), pair(lng(P - 5), lng(0)), "Sum<mint> inverse");
        checkEqual(sl.inv(LLONG_MAX), -LLONG_MAX, "Sum<lng> inverse at LLONG_MAX");
        checkEqual(pair(xb.inv(true), xb.op(true, xb.inv(true))), pair(true, false), "Xor<bool> inverse");
        std::cout << "PASS assignment powers, empty-monoid layout and group inverses\n";}

} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else if (arg == "--invalid" && i + 1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "monoids seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { throw std::runtime_error("no assertion probes: the presets assert nothing"); }
        Rng rng(seed);
        runtimeMod = array<lng, 4>{1000000007, 998244353, 10007, 97}[seed % 4];
        contract();
        preset<AddSum>(rng, "RangeAddRangeSum");
        preset<AddMinMax<false>>(rng, "RangeAddRangeMin");
        preset<AddMinMax<true>>(rng, "RangeAddRangeMax");
        preset<AddIndex<false>>(rng, "RangeAddRangeMinIndex");
        preset<AddIndex<true>>(rng, "RangeAddRangeMaxIndex");
        preset<AddCount<false>>(rng, "RangeAddRangeMinCount");
        preset<AddCount<true>>(rng, "RangeAddRangeMaxCount");
        preset<AffineSum>(rng, "RangeAffineRangeSum");
        preset<AffineSquares>(rng, "RangeAffineRangeSumSquares");
        preset<SumMinMaxIndex>(rng, "RangeAffineRangeSumMinMaxIndex");
        preset<AssignSum>(rng, "RangeAssignRangeSum");
        preset<AssignComposite>(rng, "RangeAssignRangeComposite");
        preset<AssignMaxSubarray>(rng, "RangeAssignRangeMaxSubarray");
        preset<AssignRunCount>(rng, "RangeAssignRangeRunCount");
        preset<Arithmetic>(rng, "RangeArithmeticAddRangeSum");
        preset<AndOrXor>(rng, "RangeAndOrXorRangeAndOrXor");
        preset<AndOrXorSum<6>>(rng, "RangeAndOrXorRangeSum<6 bits>");
        preset<AndOrXorSum<64>>(rng, "RangeAndOrXorRangeSum<64 bits>");
        preset<GcdLcm<false>>(rng, "RangeGcdLcmRangeGcdLcm<gcd>");
        preset<GcdLcm<true>>(rng, "RangeGcdLcmRangeGcdLcm<gcd,lcm>");
        preset<Clamp>(rng, "RangeAddChminChmaxRangeMinMax");
        preset<FlipInversion>(rng, "RangeFlipRangeInversion");
        preset<AssignFlipRun>(rng, "RangeAssignFlip<LongestRun>");
        preset<Linear>(rng, "RangeLinearRangeSum<3>");
        preset<OppositeComposite>(rng, "Opposite<RangeAssignRangeComposite>");
        preset<ReversibleComposite>(rng, "Reversible<RangeAssignRangeComposite>");
        preset<LambdaMin>(rng, "ActedMonoid lambdas (range add range min)");
        preset<StatefulAffine>(rng, "stateful struct (affine mod " + show(runtimeMod) + ")");
        preset<CapturingAffine>(rng, "ActedMonoid capturing lambdas (affine mod " + show(runtimeMod) + ")");
        plain<SumP>(rng, "Sum"); plain<MulP>(rng, "Mul"); plain<ExtremeP<false>>(rng, "Min"); plain<ExtremeP<true>>(rng, "Max");
        plain<MinMaxP>(rng, "MinMax"); plain<BitsP<0>>(rng, "And"); plain<BitsP<1>>(rng, "Or"); plain<BitsP<2>>(rng, "Xor");
        plain<GcdP>(rng, "Gcd"); plain<LcmP>(rng, "Lcm"); plain<AffineP>(rng, "AffineComposite"); plain<ClampP>(rng, "AddChminChmax");
        plain<BitwiseP>(rng, "BitwiseComposite"); plain<IndexP<false, false>>(rng, "MinIndex"); plain<IndexP<true, false>>(rng, "MaxIndex");
        plain<IndexP<false, true>>(rng, "Opposite<MinIndex> (rightmost)"); plain<CountP<false>>(rng, "MinCount"); plain<CountP<true>>(rng, "MaxCount");
        plain<Max2P>(rng, "Max2"); plain<MaxSubarrayP>(rng, "MaxSubarray"); plain<MaxGainP>(rng, "MaxGain"); plain<RunCountP>(rng, "RunCount");
        plain<LongestRunP>(rng, "LongestRun"); plain<BracketP>(rng, "BracketBalance"); plain<MatrixP>(rng, "MatrixProduct<3>");
        plain<MaxPlusP>(rng, "MaxPlusMatrix<3>"); plain<MajorityP>(rng, "MajorityVote"); plain<LambdaP>(rng, "Monoid lambda");
        plain<CapturingP>(rng, "Monoid capturing lambda (mod " + show(runtimeMod) + ")"); plain<PairP>(rng, "PairMonoid<Sum,Max>");
        plain<ArrayP>(rng, "ArrayMonoid<Min,3>"); plain<StringP<true>>(rng, "Opposite<concat>"); plain<StringP<false>>(rng, "Reversible<concat>");
        monoidActions(rng);
        std::cout << "PASS monoids checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL monoids seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
