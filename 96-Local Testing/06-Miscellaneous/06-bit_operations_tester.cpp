#include "../../06-Miscellaneous/06-bit_operations.hpp"

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
    require(B::count(x) == count && B::width(x) == last + 1, "count/width");
    require(B::first(x) == first && B::last(x) == last, "bit positions");
    require(B::leadingZeros(x) == W - last - 1 && B::trailingZeros(x) == (first < 0 ? W : first), "zero counts");
    require(B::single(x) == (count == 1) && B::floor(x) == floor, "single/floor");
    require(B::lowBit(x) == (first < 0 ? U(0) : U(U(1) << first)), "lowBit");
    require(B::parity(x) == (count % 2 == 1), "parity");
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
    require(B::rotateLeft(x, rotation) == left && B::rotateRight(x, rotation) == right, "rotation bit positions");
    for (int k : {0, 1, W / 2, W - 1, W}) {
        U l = 0, rword = 0;
        for (int i = 0; i < W; ++i) { if ((x >> i) & 1) {
            if (i + k < W) { l = U(l | (U(1) << (i + k))); }
            if (i >= k) { rword = U(rword | (U(1) << (i - k))); }}}
        require(B::shiftLeft(x, k) == l && B::shiftRight(x, k) == rword, "shift positions");}}

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
    auto rank = [&](U sup, U rest) -> ulll {
        ulll res = 0; int j = 0;
        for (int i = 0; i < B::W; ++i) {
            if ((rest >> i) & 1) { res |= ulll((sup >> i) & 1) << j++; }}
        return res;};
    for (int i = 0; i < rounds; ++i) {
        U full = U((ulll(rng()) << 64) | rng()), sm = U(full & U((ulll(rng()) << 64) | rng()));
        if (i % 4 == 0) { full = U(~U(0)); }
        U rest = U(full & U(~sm)), sup = U(sm | (rest & U((ulll(rng()) << 64) | rng())));
        if (i % 5 == 0) { sup = full; }
        context = "supermask width=" + std::to_string(B::W) + " mask=" + hexWord(sm) + " full=" + hexWord(full) + " sup=" + hexWord(sup);
        U old = sup; ulll before = rank(sup, rest);
        bool more = i % 4 == 0 ? B::nextSupermask(sup, sm) : B::nextSupermask(sup, sm, full);
        require(more == (old != full) && (more ? rank(sup, rest) == before + 1 : sup == old), "supermask counter over free bits");
        require((sup & sm) == sm && (sup & U(~full)) == 0, "supermask stays between mask and full");}
    cout << "PASS word operations width=" << B::W << " random_cases=" << rounds << '\n';}

void exhaust(int width) {
    int limit = 1 << width;
    vector<int> next(width + 1, -1);
    for (int x = limit - 1; x >= 0; --x) {
        int count = 0;
        for (int y = x; y; y /= 2) { count += y % 2; }
        context = "exhaust width=" + std::to_string(width) + " x=" + std::to_string(x);
        if (width == 8) {
            uint8_t y = uint8_t(x); bool ok = BitOps<uint8_t>::nextCombination(y);
            require(ok == (next[count] >= 0) && y == (ok ? next[count] : x), "exhaust next");
            checkWord(uint8_t(x), x - 128);}
        else {
            uint16_t y = uint16_t(x); bool ok = BitOps<uint16_t>::nextCombination(y);
            require(ok == (next[count] >= 0) && y == (ok ? next[count] : x), "exhaust next");
            checkWord(uint16_t(x), x - 32768);}
        next[count] = x;}
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
    cout << "PASS exhaustive words width=" << width << " and all 8-bit submasks/supermasks\n";}

template<typename U>
void oracle(U x, int shift) {
    using B = BitOps<U>;
    U next = x, ceil = 93; bool more = B::nextCombination(next), fits = B::ceil(x, ceil);
    cout << B::count(x) << ' ' << B::width(x) << ' ' << B::first(x) << ' ' << B::last(x) << ' '
         << B::leadingZeros(x) << ' ' << B::trailingZeros(x) << ' '
         << hexWord(B::lowBit(x)) << ' ' << hexWord(B::floor(x)) << ' '
         << fits << ' ' << hexWord(ceil) << ' ' << more << ' ' << hexWord(next) << ' '
         << hexWord(B::rotateLeft(x, shift)) << ' ' << hexWord(B::rotateRight(x, shift)) << ' '
         << B::parity(x) << ' ' << hexWord(B::grayCode(x)) << ' ' << hexWord(B::grayDecode(x)) << '\n';}

constexpr bool constantCases() {
    using B = BitOps<ulll>;
    ulll x = 3, ceil = 0;
    return B::count(~ulll(0)) == 128 && B::first(0) == -1 && B::lowMask(128) == ~ulll(0)
        && B::shiftLeft(1, 128) == 0 && B::ceil(3, ceil) && ceil == 4
        && B::nextCombination(x) && x == 5 && B::prevSubmask(x, 7) && x == 4
        && B::parity(7) && B::grayCode(2) == 3 && B::grayDecode(3) == 2 && B::nextSupermask(x, 4, 6) && x == 6;}
static_assert(constantCases());

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
                if (p == "left-negative") { B::shiftLeft(1, -1); }
                if (p == "left-large") { B::shiftLeft(1, 65); }
                if (p == "right-negative") { B::shiftRight(1, -1); }
                if (p == "right-large") { B::shiftRight(1, 65); }
                if (p == "not-submask") { B::prevSubmask(sub, 3); }
                if (p == "not-supermask") { B::nextSupermask(sub, 3); }
                if (p == "outside-full") { B::nextSupermask(sub, 0, 3); }
                if (p == "mask-outside-full") { sub = 7; B::nextSupermask(sub, 4, 3); }
                return 1;}
            if (arg == "--oracle") {
                int width, shift; string value;
                while (cin >> width >> value >> shift) {
                    ulll x = parseWord(value);
                    if (width == 8) { oracle(uint8_t(x), shift); }
                    if (width == 16) { oracle(uint16_t(x), shift); }
                    if (width == 32) { oracle(uint(x), shift); }
                    if (width == 64) { oracle(ulng(x), shift); }
                    if (width == 128) { oracle(x, shift); }}
                return 0;}}
        std::mt19937_64 rng(test_seed);
        int rounds = mode == "quick" ? 100 : mode == "full" ? 3000 : 30000;
        domains<uint8_t>(rng, rounds); domains<uint16_t>(rng, rounds);
        domains<uint>(rng, rounds); domains<ulng>(rng, rounds); domains<ulll>(rng, rounds);
        checkWord(static_cast<unsigned long long>(123), INT_MIN);
        exhaust(8); if (mode != "quick") { exhaust(16); }
        cout << "PASS bits mode=" << mode << " seed=" << test_seed << " checks=" << checks << '\n';
        return 0;} catch (const std::exception &e) {
        cerr << "FAIL bits mode=" << mode << " seed=" << test_seed << ' ' << e.what() << '\n'; return 1;}}
