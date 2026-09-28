#include "../../05-Mathematics/02-search_algorithms.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();

void require(bool ok, const string &op, const string &want = "true", const string &got = "false") {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + op + " expected=" + want + " actual=" + got); }}
void expect(lng got, lng want, const string &op) {
    require(got == want, op, std::to_string(want), std::to_string(got));}
string real(double x) { std::ostringstream out; out << std::setprecision(17) << x; return out.str(); }
void rangeContext(lng l, lng r, lng p) {
    context = "l=" + std::to_string(l) + " r=" + std::to_string(r) + " p=" + std::to_string(p);}

void boundaries(lng l, lng r, lng p) {
    rangeContext(l, r, p); int calls = 0;
    auto inc = [&](lng x) { require(l <= x && x < r, "firstTrue only queries range"); ++calls; return x >= p; };
    auto first = firstTrue(l, r, inc);
    expect(first.first, p < r, "firstTrue found"); expect(first.second, p, "firstTrue position");
    require(calls <= 64, "firstTrue evaluation bound"); calls = 0;
    auto dec = [&](lng x) { require(l <= x && x < r, "lastTrue only queries range"); ++calls; return x < p; };
    auto last = lastTrue(l, r, dec);
    expect(last.first, p > l, "lastTrue found"); expect(last.second, p > l ? p - 1 : r, "lastTrue position");
    require(calls <= 64, "lastTrue evaluation bound");
    if (l < r && p < r) {
        calls = 0;
        auto up = [&](lng x) { require(l < x && x < r, "binSearch skips endpoints"); ++calls; return x >= p; };
        if (l < p) { expect(binSearch(r, l, up), p, "binSearch true endpoint right"); }
        require(calls <= 64, "binSearch evaluations ascending"); }
    if (l < r && l < p) {
        calls = 0;
        auto down = [&](lng x) { require(l < x && x < r, "binSearch skips reversed endpoints"); ++calls; return x < p; };
        expect(binSearch(l, r, down), p - 1, "binSearch true endpoint left");
        require(calls <= 64, "binSearch evaluations descending"); }}

void integerBoundaries() {
    for (lng l = -6; l <= 6; ++l) {
        for (lng r = l; r <= 6; ++r) {
            for (lng p = l; p <= r; ++p) { boundaries(l, r, p); }}}
    vector<lng> e{LO, LO + 1, LO + 2, -2, -1, 0, 1, 2, HI - 2, HI - 1, HI};
    for (lng l : e) { for (lng r : e) { if (l <= r) { for (lng p : e) { if (l <= p && p <= r) { boundaries(l, r, p); }}}}}
    for (lng x : e) {
        rangeContext(x, x, x);
        auto never = [&](lng) { require(false, "singleton binSearch does not call f"); return false; };
        expect(binSearch(x, x, never), x, "singleton binSearch");
        expect(ternSearch(x, x, never), x, "singleton ternSearch"); }
    std::cout << "PASS integer monotone endpoints, empty ranges and full signed domain\n";}

void checkArray(const vector<int> &a, lng offset) {
    int n = int(a.size()), best = int(std::min_element(a.begin(), a.end()) - a.begin()), calls = 0;
    context = "offset=" + std::to_string(offset) + " array=";
    for (int x : a) { context += std::to_string(x) + ","; }
    auto f = [&](lng x) { lll i = lll(x) - offset; require(i >= 0 && i < n, "minimum stays in array"); ++calls; return a[int(i)]; };
    expect(ternSearch(offset, lng(lll(offset) + n - 1), f), lng(lll(offset) + best), "minimum against linear scan");
    require(calls <= 2 * int(std::bit_width(uint(n - 1))), "minimum logarithmic evaluations");}

void exhaustiveMinima(int bound) {
    lng arrays = 0;
    for (int n = 1; n <= bound; ++n) {
        int total = 1 << (2 * n);
        for (int code = 0; code < total; ++code) {
            vector<int> a(n); int k = code;
            for (int &x : a) { x = k % 4; k /= 4; }
            int m = *std::min_element(a.begin(), a.end()), p = 0, q = n - 1;
            while (a[p] != m) { ++p; } while (a[q] != m) { --q; }
            bool valid = true;
            for (int i = 0; i < p; ++i) { valid &= a[i] > a[i + 1]; }
            for (int i = p; i < q; ++i) { valid &= a[i] == a[i + 1]; }
            for (int i = q; i + 1 < n; ++i) { valid &= a[i] < a[i + 1]; }
            if (valid) { ++arrays; checkArray(a, LO); checkArray(a, -n / 2); checkArray(a, HI - n + 1); }}}
    vector<lng> e{LO, LO + 1, -1, 0, 1, HI - 1, HI};
    for (lng p : e) { for (lng q : e) { if (p <= q) {
        rangeContext(LO, HI, p); int calls = 0;
        auto f = [&](lng x) -> lll { ++calls; return x < p ? lll(p) - x : x > q ? lll(x) - q : 0; };
        expect(ternSearch(LO, HI, f), p, "full-domain leftmost minimum plateau");
        require(calls <= 128, "full-domain discrete minimum evaluation bound"); }}}
    std::cout << "PASS exhaustive strictly unimodal arrays n<=" << bound << " accepted=" << arrays << '\n';}

void checkResult(const RealSearchResult &res, double lo, double hi, int limit, double atol, double rtol) {
    require(std::isfinite(res.l) && std::isfinite(res.r) && std::isfinite(res.x), "finite result");
    require(lo <= res.l && res.l <= res.x && res.x <= res.r && res.r <= hi, "nested bracket and point");
    require(res.iterations >= 0 && res.iterations <= limit, "iteration budget");
    long double width = static_cast<long double>(res.r) - res.l;
    bool narrow = width <= static_cast<long double>(atol) + static_cast<long double>(rtol) * max(abs(res.l), abs(res.r));
    bool adjacent = res.l == res.r || std::nextafter(res.l, res.r) == res.r;
    require(res.converged == (narrow || adjacent), "reported convergence");}

void realBinary(double l, double r, double center, int limit, double atol = 0, double rtol = 0) {
    context = "binary l=" + real(l) + " r=" + real(r) + " center=" + real(center) + " limit=" + std::to_string(limit);
    for (bool reverse : {false, true}) {
        int calls = 0;
        auto f = [&](double x) { require(l < x && x < r, "binary interior finite evaluations"); ++calls; return reverse ? x >= center : x <= center; };
        auto res = binSearchRealBracket(reverse ? r : l, reverse ? l : r, f, limit, atol, rtol);
        checkResult(res, l, r, limit, atol, rtol);
        require(res.l <= center && center <= res.r, "binary brackets exact threshold");
        require(reverse ? res.x >= center : res.x <= center, "binary feasible returned endpoint");
        expect(calls, res.iterations, "binary one evaluation per reduction");
        if (!atol && !rtol) {
            calls = 0; double x = binSearchReal(reverse ? r : l, reverse ? l : r, f, limit);
            require(x == res.x, "legacy binary point"); }
        long double w = static_cast<long double>(r) - l, got = static_cast<long double>(res.r) - res.l;
        long double rounding = 8 * std::numeric_limits<double>::epsilon() * max(abs(l), abs(r)) + 8 * std::numeric_limits<double>::denorm_min();
        require(got <= std::ldexp(w, -res.iterations) + rounding, "binary contraction plus rounding"); }}

void realMinimum(double l, double r, double p, double q, int limit, double atol = 0, double rtol = 0) {
    context = "minimum l=" + real(l) + " r=" + real(r) + " p=" + real(p) + " q=" + real(q) + " limit=" + std::to_string(limit);
    for (bool golden : {false, true}) {
        int calls = 0;
        auto f = [&](double x) -> long double {
            require(l < x && x < r, "minimum interior finite evaluations"); ++calls;
            return x < p ? static_cast<long double>(p) - x : x > q ? static_cast<long double>(x) - q : 0; };
        auto res = golden ? goldenSearchRealBracket(l, r, f, limit, atol, rtol) : ternSearchRealBracket(l, r, f, limit, atol, rtol);
        checkResult(res, l, r, limit, atol, rtol);
        require(res.l <= q && p <= res.r, "minimum bracket intersects analytic minimizers", "intersection",
            "[" + real(res.l) + "," + real(res.r) + "]");
        require(calls <= (golden ? (res.iterations ? res.iterations + 1 : 0) : 2 * res.iterations), "minimum evaluation bound");
        if (!atol && !rtol) {
            calls = 0; double x = golden ? goldenSearchReal(l, r, f, limit) : ternSearchReal(l, r, f, limit);
            require(x == res.x, "minimum point wrapper"); }
        long double w = static_cast<long double>(r) - l, got = static_cast<long double>(res.r) - res.l;
        long double ratio = golden ? (std::sqrt(5.0L) - 1) / 2 : 2.0L / 3;
        long double rounding = 32 * std::numeric_limits<double>::epsilon() * max(abs(l), abs(r)) + 32 * std::numeric_limits<double>::denorm_min();
        require(got <= w * std::pow(ratio, res.iterations) + rounding, "minimum contraction plus rounding"); }}

void realBoundaries() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min();
    vector<pair<double, double>> intervals{{-huge, huge}, {-huge, 0}, {0, huge}, {-2, 3}, {-tiny, tiny},
        {0, tiny}, {1, std::nextafter(1.0, 2.0)}, {-2, -2}, {huge, huge}, {-0.0, 0.0}, {tiny, 3 * tiny}};
    for (auto [l, r] : intervals) {
        double c = std::midpoint(l, r);
        for (int limit : {0, 1, 2, 7, 60, 2200}) {
            realBinary(l, r, c, limit);
            realMinimum(l, r, c, c, limit);
            realMinimum(l, r, l, l, limit);
            realMinimum(l, r, r, r, limit);
            realMinimum(l, r, l, r, limit); }
        realBinary(l, r, c, 2200, tiny, 1e-12);
        realMinimum(l, r, c, c, 2200, tiny, 1e-12); }
    auto predicate = [](double x) { return x < 1; };
    auto objective = [](double x) { return abs(x - 1); };
    require(binSearchReal(0, 2, predicate) <= 1, "binary default iteration argument");
    require(abs(ternSearchReal(0, 2, objective) - 1) <= 1e-14, "ternary default iteration argument");
    require(abs(goldenSearchReal(0, 2, objective) - 1) <= 1e-14, "golden default iteration argument");
    for (auto res : {binSearchRealBracket(0, 2, predicate), ternSearchRealBracket(0, 2, objective), goldenSearchRealBracket(0, 2, objective)}) {
        require(abs(res.x - 1) <= 1e-14, "bracket default iteration argument"); }
    std::cout << "PASS real limits, subnormals, adjacent points, caps, tolerance and call bounds\n";}

void randomCases(int count) {
    std::mt19937_64 rng(seed);
    for (int trial = 0; trial < count; ++trial) {
        lng l = std::bit_cast<lng>(rng()), r = std::bit_cast<lng>(rng()); if (l > r) { swap(l, r); }
        lng p = lng(lll(l) + lll(rng()) % (lll(r) - l + 1)); boundaries(l, r, p);
        lng q = lng(lll(p) + lll(rng()) % (lll(r) - p + 1)); rangeContext(l, r, p);
        auto f = [&](lng x) -> lll { return x < p ? 3 * (lll(p) - x) : x > q ? 5 * (lll(x) - q) : 0; };
        expect(ternSearch(l, r, f), p, "random full-width discrete plateau");
        int n = 1 + int(rng() % 200), a = int(rng() % uint(n)), b = a + int(rng() % uint(n - a));
        vector<int> values(n);
        for (int i = a; i > 0; --i) { values[i - 1] = values[i] + 1 + int(rng() % 10); }
        for (int i = b + 1; i < n; ++i) { values[i] = values[i - 1] + 1 + int(rng() % 10); }
        checkArray(values, -n / 2);
        double dl = double(int(rng() % 2001) - 1000), dr = dl + double(1 + rng() % 2000);
        double c = std::lerp(dl, dr, double(rng() % 1001) / 1000), d = std::lerp(c, dr, double(rng() % 1001) / 1000);
        realBinary(dl, dr, c, 80, 1e-10, 1e-13); realMinimum(dl, dr, c, d, 160, 1e-10, 1e-13); }
    std::cout << "PASS seeded full-width, brute-force and analytic random cases=" << count << '\n';}

void invalid(const string &probe) {
    auto f = [](auto x) { return x < 0; };
    double inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
    if (probe == "first-order") { firstTrue(1, 0, f); }
    else if (probe == "last-order") { lastTrue(1, 0, f); }
    else if (probe == "integer-order") { ternSearch(1, 0, f); }
    else if (probe == "binary-iterations") { binSearchReal(0, 1, f, -1); }
    else if (probe == "ternary-iterations") { ternSearchReal(0, 1, f, -1); }
    else if (probe == "golden-iterations") { goldenSearchReal(0, 1, f, -1); }
    else if (probe == "binary-infinite") { binSearchReal(0, inf, f); }
    else if (probe == "ternary-nan") { ternSearchReal(nan, 1, f); }
    else if (probe == "golden-infinite") { goldenSearchReal(-inf, 1, f); }
    else if (probe == "ternary-order") { ternSearchReal(1, 0, f); }
    else if (probe == "golden-order") { goldenSearchReal(1, 0, f); }
    else if (probe == "negative-absolute") { binSearchRealBracket(0, 1, f, 10, -1); }
    else if (probe == "negative-relative") { ternSearchRealBracket(0, 1, f, 10, 0, -1); }
    else if (probe == "infinite-tolerance") { goldenSearchRealBracket(0, 1, f, 10, inf); }
    else if (probe == "nan-tolerance") { goldenSearchRealBracket(0, 1, f, 10, 0, nan); }
    else { throw std::runtime_error("unknown assertion probe=" + probe); }}

int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 1; }
            else { throw std::runtime_error("unknown argument=" + arg); }}
        integerBoundaries(); exhaustiveMinima(mode == "quick" ? 6 : mode == "stress" ? 9 : 8); realBoundaries();
        randomCases(mode == "quick" ? 100 : mode == "stress" ? 30000 : 3000);
        std::cout << "PASS 02-search_algorithms mode=" << mode << " seed=" << seed << " checks=" << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << "FAIL 02-search_algorithms mode=" << mode << " seed=" << seed << " " << e.what() << '\n'; return 1; }}
