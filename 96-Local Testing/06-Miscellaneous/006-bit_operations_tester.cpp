#include "../../06-Miscellaneous/006-bit_operations.hpp"

ulng test_seed = 20260927;
lng checks = 0;
string context;
void require(bool ok, const string &op) {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + op + " expected=true actual=false"); }}
string hexWord(ulll x) {
    if (!x) { return "0"; }
    string s;
    while (x) { s += "0123456789abcdef"[int(x % 16)]; x /= 16; }
    reverse(s.begin(), s.end()); return s;}
ulll parseWord(const string &s) {
    ulll x = 0;
    for (char c : s) { x = 16 * x + (c <= '9' ? c - '0' : c - 'a' + 10); }
    return x;}
// Counts one check; the failure detail is built only when it fails.
template<typename M>
void expect(bool ok, M &&detail) {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + detail() + " expected=true actual=false"); }}

// Bits of v at the set positions of bits, packed low: a counter over those positions.
template<typename U>
ulll compress(U v, U bits) {
    ulll res = 0; int j = 0;
    for (int i = 0; i < std::numeric_limits<U>::digits; ++i) {
        if ((bits >> i) & 1) { res |= ulll((v >> i) & 1) << j++; }}
    return res;}

template<typename U>
void checkSwap(U x, int i, int j, int n) {
    U want = 0;
    for (int p = 0; p < BitOps<U>::W; ++p) {
        int from = i <= p && p < i + n ? p - i + j : j <= p && p < j + n ? p - j + i : p;
        if ((x >> from) & 1) { want = U(want | (U(1) << p)); }}
    expect(BitOps<U>::swapBitRanges(x, i, j, n) == want, [&] {
        return "swapBitRanges x=" + hexWord(x) + " i=" + std::to_string(i) + " j=" + std::to_string(j) + " n=" + std::to_string(n);});}

// Candidate search: lo itself, else the lowest p where y turns on a zero bit of lo, keeping lo above p and the minimum below.
template<typename U>
bool lowerBoundSearch(U b, U s, U lo, U &out) {
    constexpr int W = std::numeric_limits<U>::digits;
    auto bit = [](U v, int i) { return bool((v >> i) & 1); };
    auto keeps = [&](int p) {
        for (int i = W - 1; i > p; --i) { if (!bit(s, i) && bit(lo, i) != bit(b, i)) { return false; } }
        return true;};
    if (keeps(-1)) { out = lo; return true; }
    for (int p = 0; p < W; ++p) {
        if (bit(lo, p) || !(bit(s, p) || bit(b, p)) || !keeps(p)) { continue; }
        U y = 0;
        for (int i = 0; i < W; ++i) {
            if (i > p ? bit(lo, i) : i == p || (!bit(s, i) && bit(b, i))) { y = U(y | (U(1) << i)); }}
        out = y; return true;}
    return false;}

// Exact set check: images of the blocks under ^x are disjoint aligned intervals in [lo, hi) of total hi - lo; small ranges also enumerate.
template<typename U>
void checkBlocks(U lo, U hi, U x) {
    using B = BitOps<U>;
    auto where = [&](const char *what) {
        return string(what) + " width=" + std::to_string(B::W) + " lo=" + hexWord(lo) + " hi=" + hexWord(hi) + " x=" + hexWord(x);};
    vector<pair<U, int>> blocks;
    B::xorRangeBlocks(lo, hi, x, [&](U base, int k) { blocks.push_back({base, k}); });
    bool shaped = int(blocks.size()) <= 2 * B::W;
    vector<pair<U, U>> images;
    for (auto [base, k] : blocks) {
        shaped = shaped && 0 <= k && k < B::W;
        if (!shaped) { break; }
        U span = U((U(1) << k) - 1), start = U((base ^ x) & U(~span));
        shaped = shaped && (base & span) == 0;
        images.push_back({start, U(start + span)});}
    expect(shaped, [&] { return where("xorRangeBlocks block count/alignment"); });
    sort(images.begin(), images.end());
    U total = 0; bool inside = true;
    for (int t = 0; t < int(images.size()); ++t) {
        inside = inside && lo <= images[t].first && images[t].second < hi && (t == 0 || images[t - 1].second < images[t].first);
        total = U(total + U(images[t].second - images[t].first) + 1);}
    expect(inside && total == U(hi - lo), [&] { return where("xorRangeBlocks disjoint images cover the range"); });
    if (ulll(U(hi - lo)) > 1024) { return; }
    vector<U> ys;
    for (auto [base, k] : blocks) {
        U span = U((U(1) << k) - 1);
        for (U t = 0; ; ++t) {
            ys.push_back(U(U(base + t) ^ x));
            if (t == span) { break; }}}
    sort(ys.begin(), ys.end());
    bool exact = ulll(ys.size()) == ulll(U(hi - lo));
    for (int t = 0; exact && t < int(ys.size()); ++t) { exact = ys[t] == U(lo + U(t)); }
    expect(exact, [&] { return where("xorRangeBlocks enumerated set"); });}

// A word whose bytes avoid pred, biased to boundary values, with one byte satisfying it half the time; pred over the bytes is the oracle.
template<typename U, typename P, typename C>
void biasedBytes(std::mt19937_64 &rng, const vector<int> &params, P &&pred, C &&call, const char *name) {
    constexpr int BYTES = BitOps<U>::W / 8;
    vector<int> near{0, 1, 126, 127, 128, 129, 254, 255}, yes, no;
    for (int v : params) { near.insert(near.end(), {v - 1, v, v + 1}); }
    for (int v = 0; v < 256; ++v) { (pred(v) ? yes : no).push_back(v); }
    auto draw = [&](bool want) {
        vector<int> pool;
        for (int v : near) { if (0 <= v && v < 256 && bool(pred(v)) == want) { pool.push_back(v); } }
        const vector<int> &all = want ? yes : no;
        return rng() % 4 && !pool.empty() ? pool[rng() % pool.size()] : all[rng() % all.size()];};
    U x = 0;
    for (int j = 0; j < BYTES; ++j) { x = U(x | U(U(draw(false)) << (8 * j))); }
    if (rng() % 2 && !yes.empty()) {
        int j = int(rng() % BYTES);
        x = U(U(x & U(~U(U(255) << (8 * j)))) | U(U(draw(true)) << (8 * j)));}
    bool want = false;
    for (int j = 0; j < BYTES; ++j) { want = want || pred(int((x >> (8 * j)) & 255)); }
    expect(call(x) == want, [&] {
        string res = string(name) + " x=" + hexWord(x) + " params=";
        for (int v : params) { res += std::to_string(v) + ","; }
        return res;});}

// Every n (and every m, or a sample) of the byte tests on one word against its bytes.
template<typename U>
void allByteParameters(U x, bool every_m) {
    using B = BitOps<U>;
    auto any = [&](auto pred) {
        bool res = false;
        for (int j = 0; j < B::W / 8; ++j) { res = res || pred(int((x >> (8 * j)) & 255)); }
        return res;};
    vector<int> ms{0, 1, 63, 64, 126, 127, int(x % 128), int((x >> 9) % 128)};
    if (every_m) { ms.resize(128); std::iota(ms.begin(), ms.end(), 0); }
    expect(B::hasZeroByte(x) == any([](int c) { return c == 0; }), [] { return string("hasZeroByte"); });
    for (int n = 0; n < 256; ++n) {
        auto at = [&](const char *op) { return string(op) + " n=" + std::to_string(n); };
        expect(B::hasByteEqual(x, n) == any([&](int c) { return c == n; }), [&] { return at("hasByteEqual"); });
        if (n <= 127) { expect(B::hasByteGreater(x, n) == any([&](int c) { return c > n; }), [&] { return at("hasByteGreater"); }); }
        if (n > 128) { continue; }
        expect(B::hasByteLess(x, n) == any([&](int c) { return c < n; }), [&] { return at("hasByteLess"); });
        for (int m : ms) {
            expect(B::hasByteBetween(x, m, n) == any([&](int c) { return m < c && c < n; }), [&] {
                return at("hasByteBetween") + " m=" + std::to_string(m);});}}}

template<typename U>
bool referenceNext(U &x) {
    constexpr int W = std::numeric_limits<U>::digits;
    vector<int> positions;
    for (int i = 0; i < W; ++i) { if ((x >> i) & 1) { positions.push_back(i); } }
    for (int i = 0; i < int(positions.size()); ++i) {
        int end = i + 1 == int(positions.size()) ? W : positions[i + 1];
        if (positions[i] + 1 < end) {
            ++positions[i];
            for (int j = 0; j < i; ++j) { positions[j] = j; }
            x = 0; for (int p : positions) { x = U(x | (U(1) << p)); }
            return true;}}
    return false;}

// Predecessor with equal popcount: lower the first one whose lower neighbour is free, pack the ones below it just under it.
template<typename U>
bool referencePrev(U &x) {
    constexpr int W = std::numeric_limits<U>::digits;
    vector<int> positions;
    for (int i = 0; i < W; ++i) { if ((x >> i) & 1) { positions.push_back(i); } }
    for (int j = 0; j < int(positions.size()); ++j) {
        if (positions[j] > (j ? positions[j - 1] + 1 : 0)) {
            int q = --positions[j];
            for (int t = 0; t < j; ++t) { positions[t] = q - j + t; }
            x = 0; for (int p : positions) { x = U(x | (U(1) << p)); }
            return true;}}
    return false;}

// Popcounts of 0..n by the top-bit recursion f(2^k + r) = k * 2^(k-1) + r + 1 + f(r), independent of the header's per-bit periods.
ulll onesUpTo(ulll n) {
    ulll res = 0;
    for (int k = 127; k >= 0; --k) {
        if (!((n >> k) & 1)) { continue; }
        n -= ulll(1) << k;
        res += (k ? ulll(k) << (k - 1) : ulll(0)) + n + 1;}
    return res;}

template<typename U>
void checkWord(U x, int rotation) {
    using B = BitOps<U>;
    constexpr int W = B::W;
    context = "width=" + std::to_string(W) + " x=0x" + hexWord(x) + " rotation=" + std::to_string(rotation);
    int count = 0, first = -1, last = -1;
    U floor = 0;
    for (int i = 0; i < W; ++i) {
        bool bit = (x >> i) % 2;
        if (bit) { ++count; if (first == -1) { first = i; } last = i; floor = U(U(1) << i); }
        require(B::test(x, i) == bit, "test");
        U place = U(U(1) << i);
        require(B::set(x, i) == U(bit ? x : x + place), "set");
        require(B::set(x, i, false) == U(bit ? x - place : x), "clear");
        require(B::flip(x, i) == U(bit ? x - place : x + place), "flip");}
    require(std::popcount(x) == count && std::bit_width(x) == last + 1, "popcount/bit_width replacements");
    require(B::first(x) == first && B::last(x) == last, "bit positions");
    require(std::countl_zero(x) == W - last - 1 && std::countr_zero(x) == (first < 0 ? W : first), "countl_zero/countr_zero replacements");
    require(std::has_single_bit(x) == (count == 1) && std::bit_floor(x) == floor, "has_single_bit/bit_floor replacements");
    require(B::lowBit(x) == (first < 0 ? U(0) : U(U(1) << first)), "lowBit");
    require(B::parity(x) == (count % 2 == 1), "parity");
    int sign = 0; bool top = (x >> (W - 1)) & 1;
    for (int i = W - 2; i >= 0 && bool((x >> i) & 1) == top; --i) { ++sign; }
    require(B::countLeadingSignBits(x) == sign, "countLeadingSignBits");
    if constexpr (W <= 64) { require(B::countOnesUpTo(x) == onesUpTo(ulll(x)), "countOnesUpTo"); }
    U gray = 0, decoded = 0; bool above = false;
    for (int i = W - 1; i >= 0; --i) {
        bool bit = (x >> i) & 1, upper = i + 1 < W && ((x >> (i + 1)) & 1);
        above ^= bit;
        if (bit != upper) { gray = U(gray | (U(1) << i)); }
        if (above) { decoded = U(decoded | (U(1) << i)); }}
    require(B::grayCode(x) == gray && B::grayDecode(x) == decoded, "gray code bit positions");
    require(B::grayDecode(B::grayCode(x)) == x && B::grayCode(B::grayDecode(x)) == x, "gray round trip");
    U out = 93, old = out;
    bool ceil_ok = !x || x == floor || last < W - 1;
    require(B::ceil(x, out) == ceil_ok, "ceil presence");
    U want = !x ? U(1) : x == floor ? floor : ceil_ok ? U(2 * floor) : old;
    require(out == want, "ceil value/failure preservation");
    U alias = x;
    require(B::ceil(alias, alias) == ceil_ok && alias == (ceil_ok ? want : x), "ceil output alias");
    U next = x, ref = x;
    require(B::nextCombination(next) == referenceNext(ref) && next == ref, "next combination positions oracle");
    int r = rotation % W; if (r < 0) { r += W; }
    U left = 0, right = 0;
    for (int i = 0; i < W; ++i) { if ((x >> i) & 1) {
        left = U(left | (U(1) << ((i + r) % W)));
        right = U(right | (U(1) << ((i + W - r) % W)));}}
    require(std::rotl(x, rotation) == left && std::rotr(x, rotation) == right, "rotl/rotr replacements bit positions");
    for (int k : {0, 1, W / 2, W - 1, W}) {
        U l = 0, rword = 0;
        for (int i = 0; i < W; ++i) { if ((x >> i) & 1) {
            if (i + k < W) { l = U(l | (U(1) << (i + k))); }
            if (i >= k) { rword = U(rword | (U(1) << (i - k))); }}}
        require(B::shiftLeft(x, k) == l && B::shiftRight(x, k) == rword, "shift positions");}
    U prev = x, prev_ref = x;
    require(B::prevCombination(prev) == referencePrev(prev_ref) && prev == prev_ref, "prev combination positions oracle");
    constexpr int H = W / 2;
    U rev = 0, even = 0, odd = 0, enc = 0, low = 0, below = 0;
    int seen = 0;
    for (int i = 0; i < W; ++i) {
        bool bit = (x >> i) & 1;
        expect(B::rank(x, i) == seen, [&] { return "rank i=" + std::to_string(i); });
        if (bit) {
            expect(B::select(x, seen) == i && B::select(x, B::rank(x, i)) == i, [&] { return "select k=" + std::to_string(seen); });
            ++seen;
            rev = U(rev | (U(1) << (W - 1 - i)));
            if (i % 2) { odd = U(odd | (U(1) << (i / 2))); }
            else { even = U(even | (U(1) << (i / 2))); }
            enc = U(enc | (U(1) << (i < H ? 2 * i : 2 * (i - H) + 1)));}
        low = U(low | U(U(bit) << i)); below = U(below | (U(1) << i));
        expect(B::signExtend(x, i + 1) == U(low | (bit ? U(~below) : U(0))), [&] { return "signExtend b=" + std::to_string(i + 1); });}
    require(B::rank(x, W) == count, "rank at W");
    require(B::select(x, count) == -1 && B::select(x, count + 1) == -1 && B::select(x, INT_MAX) == -1, "select past the last bit");
    require(B::bitReverse(x) == rev && B::bitReverse(rev) == x, "bitReverse positions");
    require(B::clearLowBit(x) == (first < 0 ? U(0) : U(x ^ (U(1) << first))), "clearLowBit");
    U trailing = x, zero = x;
    for (int i = 0; i < W && ((x >> i) & 1); ++i) { trailing = U(trailing ^ (U(1) << i)); }
    for (int i = 0; i < W; ++i) { if (!((x >> i) & 1)) { zero = U(zero | (U(1) << i)); break; } }
    require(B::clearTrailingOnes(x) == trailing && B::setLowestZero(x) == zero, "clearTrailingOnes/setLowestZero");
    require(B::mortonDecode(x) == pair<U, U>(even, odd) && B::mortonEncode(even, odd) == x, "mortonDecode positions and inverse");
    U half_low = U(x & B::lowMask(H)), half_high = U(x >> H);
    require(B::mortonEncode(half_low, half_high) == enc && B::mortonDecode(enc) == pair<U, U>(half_low, half_high), "mortonEncode positions and inverse");
    bool zero_byte = false;
    for (int j = 0; j < W / 8; ++j) { zero_byte = zero_byte || ((x >> (8 * j)) & 255) == 0; }
    require(B::hasZeroByte(x) == zero_byte, "hasZeroByte bytes");
    for (auto [i, j, n] : {array<int, 3>{0, 0, 0}, {W, W, 0}, {0, W, 0}, {0, H, H}, {H, 0, H}, {0, W - 1, 1}, {W - 1, 0, 1}, {1, H + 1, H - 1}}) {
        checkSwap(x, i, j, n);}}

template<typename U>
void domains(std::mt19937_64 &rng, int rounds) {
    using B = BitOps<U>;
    U mask = 0;
    for (int k = 0; k <= B::W; ++k) {
        context = "mask width=" + std::to_string(B::W) + " k=" + std::to_string(k);
        require(B::lowMask(k) == mask, "lowMask");
        checkWord(mask, k); checkWord(U(~mask), -k);
        if (k < B::W) { mask = U(mask | (U(1) << k)); }}
    for (int shift : {INT_MIN, INT_MIN + 1, -129, -1, 0, 1, 129, INT_MAX}) {
        checkWord(U(0), shift); checkWord(U(1), shift); checkWord(U(~U(0)), shift);}
    for (int i = 0; i < rounds; ++i) {
        U x = U((ulll(rng()) << 64) | rng()); checkWord(x, int(uint(rng())));}
    vector<int> places{0, B::W / 3, B::W / 2, B::W - 1};
    auto expand = [&](int code) -> U {
        U value = 0;
        for (int i = 0; i < 4; ++i) { if ((code >> i) & 1) { value = U(value | (U(1) << places[i])); } }
        return value;};
    for (int m = 0; m < 16; ++m) {
        U mask = expand(m), sub = mask;
        for (int s = m; s >= 0; --s) { if ((s & m) == s) {
            context = "sparse submask width=" + std::to_string(B::W) + " mask=" + hexWord(mask) + " sub=" + hexWord(sub);
            require(sub == expand(s), "wide submask positions");
            require(B::prevSubmask(sub, mask) == (s != 0), "wide submask presence");}}
        require(sub == 0, "wide submask terminal unchanged");}
    for (int i = 0; i < rounds; ++i) {
        U full = U((ulll(rng()) << 64) | rng()), sm = U(full & U((ulll(rng()) << 64) | rng()));
        if (i % 4 == 0) { full = U(~U(0)); }
        U rest = U(full & U(~sm)), sup = U(sm | (rest & U((ulll(rng()) << 64) | rng())));
        if (i % 5 == 0) { sup = full; }
        context = "supermask width=" + std::to_string(B::W) + " mask=" + hexWord(sm) + " full=" + hexWord(full) + " sup=" + hexWord(sup);
        U old = sup; ulll before = compress(sup, rest);
        bool more = i % 4 == 0 ? B::nextSupermask(sup, sm) : B::nextSupermask(sup, sm, full);
        require(more == (old != full) && (more ? compress(sup, rest) == before + 1 : sup == old), "supermask counter over free bits");
        require((sup & sm) == sm && (sup & U(~full)) == 0, "supermask stays between mask and full");}
    cout << "PASS word operations width=" << B::W << " random_cases=" << rounds << '\n';}

template<typename U>
void wide(std::mt19937_64 &rng, int rounds) {
    using B = BitOps<U>;
    constexpr int W = B::W, H = W / 2;
    const U top = U(~U(0));
    auto word = [&] { return U((ulll(rng()) << 64) | rng()); };
    auto shaped = [&] {
        U v = word(); ulng c = rng() % 4;
        return c == 0 ? U(v & word() & word()) : c == 1 ? U(v | word() | word()) : c == 2 ? U(v >> (rng() % W)) : v;};
    auto spread = [&](const array<int, 8> &pos, int v) {
        U res = 0;
        for (int t = 0; t < 8; ++t) { if ((v >> t) & 1) { res = U(res | (U(1) << pos[t])); } }
        return res;};
    for (int r = 0; r < rounds; ++r) {
        U a = shaped(), b = shaped(), m = shaped(), merged = 0;
        context = "random width=" + std::to_string(W) + " a=" + hexWord(a) + " b=" + hexWord(b) + " m=" + hexWord(m);
        for (int i = 0; i < W; ++i) { if ((((m >> i) & 1) ? b >> i : a >> i) & 1) { merged = U(merged | (U(1) << i)); } }
        require(B::mergeBits(a, b, m) == merged, "mergeBits bit choice");
        U hx = U(a >> H), hy = U(b >> H), z = 0;
        for (int i = 0; i < H; ++i) {
            if ((hx >> i) & 1) { z = U(z | (U(1) << (2 * i))); }
            if ((hy >> i) & 1) { z = U(z | (U(1) << (2 * i + 1))); }}
        require(B::mortonEncode(hx, hy) == z && B::mortonDecode(z) == pair<U, U>(hx, hy), "morton random halves");
        int n = int(rng() % (H + 1)), room = W - 2 * n, g0 = int(rng() % ulng(room + 1)), g1 = int(rng() % ulng(room - g0 + 1));
        if (rng() % 2) { checkSwap(a, g0, g0 + n + g1, n); }
        else { checkSwap(a, g0 + n + g1, g0, n); }
        int eq = int(rng() % 256), less = int(rng() % 129), great = int(rng() % 128), lo8 = int(rng() % 128), hi8 = int(rng() % 129);
        biasedBytes<U>(rng, {}, [](int v) { return v == 0; }, [](U x) { return B::hasZeroByte(x); }, "hasZeroByte");
        biasedBytes<U>(rng, {eq}, [&](int v) { return v == eq; }, [&](U x) { return B::hasByteEqual(x, eq); }, "hasByteEqual");
        biasedBytes<U>(rng, {less}, [&](int v) { return v < less; }, [&](U x) { return B::hasByteLess(x, less); }, "hasByteLess");
        biasedBytes<U>(rng, {great}, [&](int v) { return v > great; }, [&](U x) { return B::hasByteGreater(x, great); }, "hasByteGreater");
        biasedBytes<U>(rng, {lo8, hi8}, [&](int v) { return lo8 < v && v < hi8; }, [&](U x) { return B::hasByteBetween(x, lo8, hi8); }, "hasByteBetween");

        U full = r % 4 == 0 ? top : shaped(), mask = U(full & shaped()), rest = U(full & U(~mask)), sup = U(mask | U(rest & word()));
        if (r % 5 == 0) { sup = mask; }
        if (r % 7 == 0) { sup = full; }
        context = "stepping width=" + std::to_string(W) + " mask=" + hexWord(mask) + " full=" + hexWord(full) + " sup=" + hexWord(sup);
        U old = sup; ulll before = compress(sup, rest);
        bool more = r % 4 == 0 ? B::prevSupermask(sup, mask) : B::prevSupermask(sup, mask, full);
        require(more == (old != mask) && (more ? compress(sup, rest) == before - 1 : sup == old), "prevSupermask counter over free bits");
        require((sup & mask) == mask && (sup & U(~full)) == 0, "prevSupermask stays between mask and full");
        require(!more || (B::nextSupermask(sup, mask, full) && sup == old), "prevSupermask then nextSupermask round trip");
        sup = old;
        require(!B::nextSupermask(sup, mask, full) || (B::prevSupermask(sup, mask, full) && sup == old), "nextSupermask then prevSupermask round trip");
        U sub = r % 5 == 0 ? full : r % 7 == 0 ? U(0) : U(full & word());
        old = sub; before = compress(sub, full);
        more = B::nextSubmask(sub, full);
        require(more == (old != full) && (more ? compress(sub, full) == before + 1 : sub == old) && (sub & U(~full)) == 0, "nextSubmask counter over mask bits");
        require(!more || (B::prevSubmask(sub, full) && sub == old), "nextSubmask then prevSubmask round trip");
        sub = old;
        require(!B::prevSubmask(sub, full) || (B::nextSubmask(sub, full) && sub == old), "prevSubmask then nextSubmask round trip");
        U c = shaped(), d = c;
        context = "combination width=" + std::to_string(W) + " x=" + hexWord(c);
        require(!B::nextCombination(d) || (B::prevCombination(d) && d == c), "nextCombination then prevCombination round trip");
        d = c;
        require(!B::prevCombination(d) || (B::nextCombination(d) && d == c), "prevCombination then nextCombination round trip");

        U lb = shaped(), ls = shaped(), lo = shaped();
        ulng kind = rng() % 7;
        if (kind == 0) { ls = 0; }
        if (kind == 1) { ls = top; }
        if (kind == 2) { lo = 0; }
        if (kind == 3) { lo = top; }
        if (kind == 4) { lo = U(U(lb ^ U(ls & word())) + U(rng() % 3) - 1); }
        context = "xorSubmaskLowerBound width=" + std::to_string(W) + " b=" + hexWord(lb) + " s=" + hexWord(ls) + " lo=" + hexWord(lo);
        U out = U(lo ^ 0x5a), ref = out, keep = out, alias = lo;
        bool want = lowerBoundSearch(lb, ls, lo, ref);
        require(B::xorSubmaskLowerBound(lb, ls, lo, out) == want && out == (want ? ref : keep), "xorSubmaskLowerBound candidate oracle");
        require(B::xorSubmaskLowerBound(lb, ls, alias, alias) == want && alias == (want ? ref : lo), "xorSubmaskLowerBound lo aliases out");
        array<int, 8> pos{};
        if (W == 8) { std::iota(pos.begin(), pos.end(), 0); }
        else {
            set<int> chosen;
            if (r % 3 == 0) { chosen = {0, W - 1}; }
            while (chosen.size() < 8) { chosen.insert(int(rng() % W)); }
            std::copy(chosen.begin(), chosen.end(), pos.begin());}
        int b8 = int(rng() % 256), s8 = int((rng() % 256) & (rng() % 256)), l8 = int(rng() % 256), ans = -1;
        for (int y = l8; y < 256 && ans < 0; ++y) { if (((y ^ b8) & ~s8 & 255) == 0) { ans = y; } }
        U outside = U(word() & U(~spread(pos, 255))), eout = 0;
        context = "embedded lower bound width=" + std::to_string(W) + " b8=" + std::to_string(b8) + " s8=" + std::to_string(s8)
            + " lo8=" + std::to_string(l8) + " outside=" + hexWord(outside);
        bool eok = B::xorSubmaskLowerBound(U(spread(pos, b8) | outside), spread(pos, s8), U(spread(pos, l8) | outside), eout);
        require(eok == (ans >= 0) && eout == (ans < 0 ? U(0) : U(spread(pos, ans) | outside)), "xorSubmaskLowerBound embedded 8-bit brute");

        U x = shaped(), rlo = word(), rhi = 0, span = U(rng() % 300);
        context = "xorRangeBlocks random";
        if (r % 4 == 0) { rhi = top; rlo = U(top - span); }
        else if (r % 4 == 1) {
            if (rlo > U(top - span)) { rlo = U(top - span); }
            rhi = U(rlo + span);}
        else if (r % 4 == 2) {
            rhi = word();
            if (rlo > rhi) { swap(rlo, rhi); }}
        else { rhi = r % 8 == 3 ? rlo : top; rlo = r % 8 == 3 ? rlo : 0; }
        checkBlocks(rlo, rhi, x);}
    cout << "PASS new word operations width=" << W << " random_cases=" << rounds << '\n';}

void exhaust(int width) {
    int limit = 1 << width;
    vector<int> next(width + 1, -1), prev(width + 1, -1);
    for (int x = limit - 1; x >= 0; --x) {
        int count = 0;
        for (int y = x; y; y /= 2) { count += y % 2; }
        context = "exhaust width=" + std::to_string(width) + " x=" + std::to_string(x);
        if (width == 8) {
            uint8_t y = uint8_t(x); bool ok = BitOps<uint8_t>::nextCombination(y);
            require(ok == (next[count] >= 0) && y == (ok ? next[count] : x), "exhaust next");
            checkWord(uint8_t(x), x - 128);
            allByteParameters(uint8_t(x), true);
            for (int n = 0; n <= 8; ++n) { for (int i = 0; i + n <= 8; ++i) { for (int j = 0; j + n <= 8; ++j) {
                if (i + n <= j || j + n <= i) { checkSwap(uint8_t(x), i, j, n); }}}}}
        else {
            uint16_t y = uint16_t(x); bool ok = BitOps<uint16_t>::nextCombination(y);
            require(ok == (next[count] >= 0) && y == (ok ? next[count] : x), "exhaust next");
            checkWord(uint16_t(x), x - 32768);
            allByteParameters(uint16_t(x), false);}
        next[count] = x;}
    for (int x = 0; x < limit; ++x) {
        int count = 0;
        for (int y = x; y; y /= 2) { count += y % 2; }
        context = "exhaust prev width=" + std::to_string(width) + " x=" + std::to_string(x);
        if (width == 8) {
            uint8_t y = uint8_t(x); bool ok = BitOps<uint8_t>::prevCombination(y);
            require(ok == (prev[count] >= 0) && y == (ok ? prev[count] : x), "exhaust prevCombination");}
        else {
            uint16_t y = uint16_t(x); bool ok = BitOps<uint16_t>::prevCombination(y);
            require(ok == (prev[count] >= 0) && y == (ok ? prev[count] : x), "exhaust prevCombination");}
        prev[count] = x;}
    for (int mask = 0; mask < 256; ++mask) {
        uint8_t sub = uint8_t(mask);
        for (int x = mask; x >= 0; --x) { if ((x & mask) == x) {
            context = "submask mask=" + std::to_string(mask) + " x=" + std::to_string(x);
            require(sub == x, "descending submask");
            require(BitOps<uint8_t>::prevSubmask(sub, uint8_t(mask)) == (x != 0), "submask presence");}}
        require(sub == 0 && !BitOps<uint8_t>::prevSubmask(sub, uint8_t(mask)) && sub == 0, "terminal zero");}
    for (int full = 0; full < 256; ++full) {
        for (int mask = full; ; mask = (mask - 1) & full) {
            uint8_t sup = uint8_t(mask);
            for (int y = mask; y < 256; ++y) { if ((y & mask) == mask && (y & ~full) == 0) {
                context = "supermask mask=" + std::to_string(mask) + " full=" + std::to_string(full) + " y=" + std::to_string(y);
                require(sup == y, "ascending supermask");
                require(BitOps<uint8_t>::nextSupermask(sup, uint8_t(mask), uint8_t(full)) == (y != full), "supermask presence");}}
            require(sup == full, "supermask terminal unchanged");
            if (!mask) { break; }}}
    ulll total = 0;
    for (ulng x = 0; x < (ulng(1) << (width == 8 ? 8 : 20)); ++x) {
        total += ulll(std::popcount(x));
        context = "countOnesUpTo prefix width=" + std::to_string(width) + " x=" + std::to_string(x);
        if (width == 8) { require(BitOps<uint8_t>::countOnesUpTo(uint8_t(x)) == total, "countOnesUpTo prefix 8"); }
        if (width != 8 && x < 65536) { require(BitOps<uint16_t>::countOnesUpTo(uint16_t(x)) == total, "countOnesUpTo prefix 16"); }
        require(BitOps<uint>::countOnesUpTo(uint(x)) == total && BitOps<ulng>::countOnesUpTo(x) == total, "countOnesUpTo prefix 32/64");}
    for (int k = 0; k <= 64; ++k) {
        ulll n = k == 64 ? ulll(~0ULL) : (ulll(1) << k) - 1, want = ulll(64) << 63;
        if (k < 64) { want = k ? ulll(k) << (k - 1) : ulll(0); }
        context = "countOnesUpTo 2^k - 1 k=" + std::to_string(k);
        require(BitOps<ulng>::countOnesUpTo(ulng(n)) == want, "countOnesUpTo all-ones closed form");}
    cout << "PASS exhaustive words width=" << width << " and all 8-bit submasks/supermasks\n";}

void exhaustSteps() {
    using E = BitOps<uint8_t>;
    for (int mask = 0; mask < 256; ++mask) {
        uint8_t sub = 0;
        for (int x = 0; x <= mask; ++x) { if ((x & mask) == x) {
            context = "ascending submask mask=" + std::to_string(mask) + " x=" + std::to_string(x);
            require(sub == x, "ascending submask");
            require(E::nextSubmask(sub, uint8_t(mask)) == (x != mask), "nextSubmask presence");}}
        require(sub == mask && !E::nextSubmask(sub, uint8_t(mask)) && sub == mask, "nextSubmask terminal unchanged");}
    for (int full = 0; full < 256; ++full) {
        for (int mask = full; ; mask = (mask - 1) & full) {
            uint8_t sup = uint8_t(full);
            for (int y = full; y >= 0; --y) { if ((y & mask) == mask && (y & ~full) == 0) {
                context = "descending supermask mask=" + std::to_string(mask) + " full=" + std::to_string(full) + " y=" + std::to_string(y);
                require(sup == y, "descending supermask");
                bool more = full == 255 && mask % 2 ? E::prevSupermask(sup, uint8_t(mask)) : E::prevSupermask(sup, uint8_t(mask), uint8_t(full));
                require(more == (y != mask), "prevSupermask presence");}}
            require(sup == mask, "prevSupermask terminal unchanged");
            if (!mask) { break; }}}
    cout << "PASS all 8-bit nextSubmask/prevSupermask traversals\n";}

void exhaustXor(const string &mode) {
    using E = BitOps<uint8_t>;
    vector<int> xs{0, 0x5a, 0x80, 0xff};
    if (mode != "quick") {
        xs = {0x7f, 0x80, 0xff};
        for (int x = 0; x < 256; x += mode == "full" ? 7 : 1) { xs.push_back(x); }}
    array<int, 256> stamp{};
    int epoch = 0;
    for (int x : xs) { for (int lo = 0; lo < 256; ++lo) { for (int hi = lo; hi < 256; ++hi) {
        ++epoch;
        int calls = 0, covered = 0; bool ok = true;
        E::xorRangeBlocks(uint8_t(lo), uint8_t(hi), uint8_t(x), [&](uint8_t base, int k) {
            ++calls;
            if (k < 0 || k >= 8 || base % (1 << k)) { ok = false; return; }
            for (int t = 0; t < 1 << k; ++t) {
                int y = (base + t) ^ x;
                ok = ok && lo <= y && y < hi && stamp[y] != epoch;
                stamp[y] = epoch; ++covered;}});
        expect(ok && calls <= 16 && covered == hi - lo, [&] {
            return "xorRangeBlocks exhaustive x=" + std::to_string(x) + " lo=" + std::to_string(lo) + " hi=" + std::to_string(hi);});}}}
    cout << "PASS 8-bit xorRangeBlocks all lo<=hi pairs for " << xs.size() << " x\n";
    int step = mode == "quick" ? 17 : 1;
    for (int b = 0; b < 256; b += step) { for (int s = 0; s < 256; ++s) {
        int ans = -1;
        for (int lo = 255; lo >= 0; --lo) {
            if (((lo ^ b) & ~s & 255) == 0) { ans = lo; }
            uint8_t out = uint8_t(lo ^ 0xa5), ref = out, a = uint8_t(lo), c = uint8_t(b);
            bool ok = E::xorSubmaskLowerBound(uint8_t(b), uint8_t(s), uint8_t(lo), out);
            bool ok_lo = E::xorSubmaskLowerBound(uint8_t(b), uint8_t(s), a, a), ok_b = E::xorSubmaskLowerBound(c, uint8_t(s), uint8_t(lo), c);
            auto at = [&](const char *op) {
                return string(op) + " b=" + std::to_string(b) + " s=" + std::to_string(s) + " lo=" + std::to_string(lo) + " want=" + std::to_string(ans);};
            expect(ok == (ans >= 0) && out == (ans >= 0 ? ans : lo ^ 0xa5), [&] { return at("xorSubmaskLowerBound upward scan"); });
            expect(ok_lo == ok && a == (ans >= 0 ? ans : lo) && ok_b == ok && c == (ans >= 0 ? ans : b), [&] { return at("xorSubmaskLowerBound aliased output"); });
            if (b % 17 == 0) {
                bool found = lowerBoundSearch(uint8_t(b), uint8_t(s), uint8_t(lo), ref);
                expect(found == (ans >= 0) && ref == (ans >= 0 ? ans : lo ^ 0xa5), [&] { return at("candidate oracle self-check"); });}}}}
    cout << "PASS 8-bit xorSubmaskLowerBound " << (mode == "quick" ? "16 b values" : "all b") << " x all s x all lo\n";}

template<typename U>
void oracle(U x, int shift, int k, int i, int b, U lo, U hi, U xr) {
    using B = BitOps<U>;
    U next = x, ceil = 93; bool more = B::nextCombination(next), fits = B::ceil(x, ceil);
    cout << std::popcount(x) << ' ' << std::bit_width(x) << ' ' << B::first(x) << ' ' << B::last(x) << ' '
         << std::countl_zero(x) << ' ' << std::countr_zero(x) << ' '
         << hexWord(B::lowBit(x)) << ' ' << hexWord(std::bit_floor(x)) << ' '
         << fits << ' ' << hexWord(ceil) << ' ' << more << ' ' << hexWord(next) << ' '
         << hexWord(std::rotl(x, shift)) << ' ' << hexWord(std::rotr(x, shift)) << ' '
         << B::parity(x) << ' ' << hexWord(B::grayCode(x)) << ' ' << hexWord(B::grayDecode(x)) << ' '
         << B::select(x, k) << ' ' << B::rank(x, i) << ' ' << hexWord(B::bitReverse(x)) << ' ';
    auto [even, odd] = B::mortonDecode(x);
    cout << hexWord(even) << ' ' << hexWord(odd) << ' ' << hexWord(B::mortonEncode(U(x & B::lowMask(B::W / 2)), U(x >> (B::W / 2)))) << ' '
         << hexWord(B::signExtend(x, b));
    vector<pair<U, int>> blocks;
    B::xorRangeBlocks(lo, hi, xr, [&](U base, int e) { blocks.push_back({base, e}); });
    cout << ' ' << blocks.size();
    for (auto [base, e] : blocks) { cout << ' ' << hexWord(base) << ':' << e; }
    cout << '\n';}

constexpr bool constantCases() {
    using B = BitOps<ulll>;
    ulll x = 3, ceil = 0;
    return std::popcount(~ulll(0)) == 128 && B::first(0) == -1 && B::lowMask(128) == ~ulll(0)
        && B::shiftLeft(1, 128) == 0 && B::ceil(3, ceil) && ceil == 4
        && B::nextCombination(x) && x == 5 && B::prevSubmask(x, 7) && x == 4
        && B::parity(7) && B::grayCode(2) == 3 && B::grayDecode(3) == 2 && B::nextSupermask(x, 4, 6) && x == 6;}
static_assert(constantCases());

constexpr bool constantMore() {
    using B = BitOps<ulll>; using E = BitOps<uint8_t>; using V = BitOps<uint>;
    ulll x = ulll(5) | ulll(1) << 100, sub = 0, sup = 7, c = 6, out = 9, none = 9;
    bool seen[256]{}; int calls = 0, total = 0;
    E::xorRangeBlocks(3, 200, 0x5a, [&](uint8_t base, int k) {
        ++calls;
        for (int t = 0; t < 1 << k; ++t) {
            int y = (base + t) ^ 0x5a;
            total += !seen[y] && 3 <= y && y < 200; seen[y] = true;}});
    return calls <= 16 && total == 197 && B::select(x, 2) == 100 && B::select(x, 3) == -1 && B::rank(x, 101) == 3
        && B::bitReverse(1) == ulll(1) << 127 && B::clearLowBit(12) == 8 && B::clearTrailingOnes(11) == 8 && B::setLowestZero(11) == 15
        && B::mergeBits(0xf0, 0x0f, 0x3c) == 0xcc && B::signExtend(0x80, 8) == ~ulll(0x7f) && B::swapBitRanges(1, 0, 100, 1) == ulll(1) << 100
        && B::mortonEncode(3, 1) == 7 && B::mortonDecode(7) == pair<ulll, ulll>(3, 1)
        && !V::hasZeroByte(0x01010101u) && V::hasZeroByte(0x01000101u) && V::hasByteEqual(0x01020304u, 2) && !V::hasByteLess(0x80808080u, 128)
        && V::hasByteLess(0x8080807fu, 128) && !V::hasByteGreater(0x7f7f7f7fu, 127) && V::hasByteGreater(0x7f7f7f80u, 127)
        && V::hasByteBetween(0x05050505u, 4, 6) && !V::hasByteBetween(0x05050505u, 5, 6)
        && B::nextSubmask(sub, 5) && sub == 1 && B::prevSupermask(sup, 1, 7) && sup == 5 && B::prevCombination(c) && c == 5
        && B::xorSubmaskLowerBound(4, 3, 5, out) && out == 5 && !B::xorSubmaskLowerBound(4, 3, 8, none) && none == 9;}
static_assert(constantMore());

int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
            if (arg == "--invalid") {
                string p = argv[++i]; using B = BitOps<>; ulng sub = 4;
                if (p == "mask-negative") { B::lowMask(-1); }
                if (p == "mask-large") { B::lowMask(65); }
                if (p == "test-negative") { B::test(0, -1); }
                if (p == "test-large") { B::test(0, 64); }
                if (p == "set-negative") { B::set(0, -1); }
                if (p == "set-large") { B::set(0, 64); }
                if (p == "flip-negative") { B::flip(0, -1); }
                if (p == "flip-large") { B::flip(0, 64); }
                if (p == "test-beyond") { B::test(0, 65); }
                if (p == "set-beyond") { B::set(0, 65); }
                if (p == "flip-beyond") { B::flip(0, 65); }
                if (p == "morton-both") { B::mortonEncode(1ULL << 32, 1ULL << 32); }
                if (p == "left-negative") { B::shiftLeft(1, -1); }
                if (p == "left-large") { B::shiftLeft(1, 65); }
                if (p == "right-negative") { B::shiftRight(1, -1); }
                if (p == "right-large") { B::shiftRight(1, 65); }
                if (p == "not-submask") { B::prevSubmask(sub, 3); }
                if (p == "not-supermask") { B::nextSupermask(sub, 3); }
                if (p == "outside-full") { B::nextSupermask(sub, 0, 3); }
                if (p == "mask-outside-full") { sub = 7; B::nextSupermask(sub, 4, 3); }
                if (p == "select-negative") { B::select(1, -1); }
                if (p == "rank-negative") { B::rank(0, -1); }
                if (p == "rank-large") { B::rank(0, 65); }
                if (p == "sign-zero") { B::signExtend(0, 0); }
                if (p == "sign-large") { B::signExtend(0, 65); }
                if (p == "swap-overlap") { B::swapBitRanges(0, 0, 1, 2); }
                if (p == "swap-count-negative") { B::swapBitRanges(0, 0, 8, -1); }
                if (p == "swap-index-negative") { B::swapBitRanges(0, -1, 8, 1); }
                if (p == "swap-first-large") { B::swapBitRanges(0, 60, 0, 5); }
                if (p == "swap-second-large") { B::swapBitRanges(0, 0, 60, 5); }
                if (p == "morton-x") { B::mortonEncode(1ULL << 32, 0); }
                if (p == "morton-y") { B::mortonEncode(0, 1ULL << 32); }
                if (p == "equal-negative") { B::hasByteEqual(0, -1); }
                if (p == "equal-large") { B::hasByteEqual(0, 256); }
                if (p == "less-negative") { B::hasByteLess(0, -1); }
                if (p == "less-large") { B::hasByteLess(0, 129); }
                if (p == "greater-negative") { B::hasByteGreater(0, -1); }
                if (p == "greater-large") { B::hasByteGreater(0, 128); }
                if (p == "between-m-negative") { B::hasByteBetween(0, -1, 5); }
                if (p == "between-m-large") { B::hasByteBetween(0, 128, 5); }
                if (p == "between-n-negative") { B::hasByteBetween(0, 0, -1); }
                if (p == "between-n-large") { B::hasByteBetween(0, 0, 129); }
                if (p == "next-submask-invalid") { B::nextSubmask(sub, 3); }
                if (p == "prev-supermask-not-super") { B::prevSupermask(sub, 3); }
                if (p == "prev-supermask-outside-full") { B::prevSupermask(sub, 0, 3); }
                if (p == "prev-supermask-mask-outside") { sub = 7; B::prevSupermask(sub, 4, 3); }
                if (p == "xor-range-reversed") { B::xorRangeBlocks(2, 1, 0, [](ulng, int) {}); }
                return 1;}
            if (arg == "--oracle") {
                int width, shift, k, at, b; string value, lo, hi, xr;
                while (cin >> width >> value >> shift >> k >> at >> b >> lo >> hi >> xr) {
                    auto run = [&](auto zero) {
                        using U = decltype(zero);
                        oracle(U(parseWord(value)), shift, k, at, b, U(parseWord(lo)), U(parseWord(hi)), U(parseWord(xr)));};
                    if (width == 8) { run(uint8_t(0)); }
                    if (width == 16) { run(uint16_t(0)); }
                    if (width == 32) { run(uint(0)); }
                    if (width == 64) { run(ulng(0)); }
                    if (width == 128) { run(ulll(0)); }}
                return 0;}}
        std::mt19937_64 rng(test_seed);
        int rounds = mode == "quick" ? 100 : mode == "full" ? 3000 : 30000;
        domains<uint8_t>(rng, rounds); domains<uint16_t>(rng, rounds);
        domains<uint>(rng, rounds); domains<ulng>(rng, rounds); domains<ulll>(rng, rounds);
        wide<uint8_t>(rng, rounds); wide<uint16_t>(rng, rounds); wide<uint>(rng, rounds); wide<ulng>(rng, rounds); wide<ulll>(rng, rounds);
        checkWord(static_cast<unsigned long long>(123), INT_MIN);
        exhaust(8); if (mode != "quick") { exhaust(16); }
        exhaustSteps(); exhaustXor(mode);
        cout << "PASS bits mode=" << mode << " seed=" << test_seed << " checks=" << checks << '\n';
        return 0;} catch (const std::exception &e) {
        cerr << "FAIL bits mode=" << mode << " seed=" << test_seed << ' ' << e.what() << '\n'; return 1;}}
