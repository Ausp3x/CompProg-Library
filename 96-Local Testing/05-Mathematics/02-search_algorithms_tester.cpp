#include "../../05-Mathematics/02-search_algorithms.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context, shape;
constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();

void require(bool ok, const string &op, const string &want = "true", const string &got = "false") {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + op + " expected=" + want + " actual=" + got); }}
void expect(lng got, lng want, const string &op) {
    require(got == want, op, std::to_string(want), std::to_string(got));}
string real(double x) { std::ostringstream out; out << std::setprecision(17) << x; return out.str(); }
void rangeContext(lng l, lng r, lng p) {
    context = "l=" + std::to_string(l) + " r=" + std::to_string(r) + " p=" + std::to_string(p);}

int fibBound(lll n) {
    lll x = 1, y = 1; int k = 0;
    while (y < n + 1) { y += x; x = y - x; ++k; }
    return max(k, 1);}
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
        require(calls <= 64, "binSearch evaluations ascending");}
    if (l < r && l < p) {
        calls = 0;
        auto down = [&](lng x) { require(l < x && x < r, "binSearch skips reversed endpoints"); ++calls; return x < p; };
        expect(binSearch(l, r, down), p - 1, "binSearch true endpoint left");
        require(calls <= 64, "binSearch evaluations descending");
        calls = 0;
        auto gallop = [&](lng x) { require(x > l, "expSearch skips ok"); ++calls; return x < p; };
        expect(expSearch(l, gallop), p - 1, "expSearch last true");
        require(calls <= 2 * int(std::bit_width(ulng(lll(p) - l))) + 2, "expSearch evaluations");}}

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
        expect(ternSearch(x, x, never), x, "singleton ternSearch");
        expect(fibSearch(x, x, never), x, "singleton fibSearch");
        int calls = 0; auto always = [&](lng) { ++calls; return true; };
        expect(expSearch(x, always), HI, "expSearch unbounded true"); require(calls <= 65, "expSearch unbounded evaluations");}
    std::cout << "PASS integer monotone endpoints, empty ranges and full signed domain\n";}

void checkArray(const vector<int> &a, lng offset) {
    int n = int(a.size()), best = int(std::min_element(a.begin(), a.end()) - a.begin()), calls = 0;
    context = "offset=" + std::to_string(offset) + " array=";
    for (int x : a) { context += std::to_string(x) + ","; }
    auto f = [&](lng x) { lll i = lll(x) - offset; require(i >= 0 && i < n, "minimum stays in array"); ++calls; return a[int(i)]; };
    expect(ternSearch(offset, lng(lll(offset) + n - 1), f), lng(lll(offset) + best), "minimum against linear scan");
    require(calls <= 2 * int(std::bit_width(uint(n - 1))), "minimum logarithmic evaluations"); calls = 0;
    expect(fibSearch(offset, lng(lll(offset) + n - 1), f), lng(lll(offset) + best), "fibSearch against linear scan");
    require(n == 1 ? calls == 0 : calls <= fibBound(n), "fibSearch evaluation bound");}

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
        require(calls <= 128, "full-domain discrete minimum evaluation bound"); calls = 0;
        expect(fibSearch(LO, HI, f), p, "full-domain fibSearch plateau");
        require(calls <= 93, "full-domain fibSearch evaluation bound");}}}
    std::cout << "PASS exhaustive strictly unimodal arrays n<=" << bound << " accepted=" << arrays << '\n';}

bool settled(double l, double r, double atol, double rtol) {
    long double width = static_cast<long double>(r) - l;
    bool narrow = width <= static_cast<long double>(atol) + static_cast<long double>(rtol) * max(abs(l), abs(r));
    return narrow || l == r || std::nextafter(l, r) == r;}
void checkResult(const RealSearchResult &res, double lo, double hi, int limit, double atol, double rtol, bool bisection = false) {
    require(std::isfinite(res.l) && std::isfinite(res.r) && std::isfinite(res.x), "finite result");
    require(lo <= res.l && res.l <= res.x && res.x <= res.r && res.r <= hi, "nested bracket and point");
    require(res.iterations >= 0 && res.iterations <= limit, "iteration budget");
    require(res.converged == settled(res.l, res.r, atol, rtol), "reported convergence");
    long double width = static_cast<long double>(res.r) - res.l;
    long double floor = 16 * std::numeric_limits<double>::epsilon() * static_cast<long double>(max(abs(res.l), abs(res.r))) + 16 * std::numeric_limits<double>::denorm_min();
    if (res.iterations < limit && !res.converged) {
        require(!bisection, "bisection converges before its cap", "converged", "[" + real(res.l) + "," + real(res.r) + "]");
        require(width <= floor, "early stop only on stagnation", "width<=" + real(double(floor)), "[" + real(res.l) + "," + real(res.r) + "]");}}

void realBinary(double l, double r, double center, int limit, double atol = 0, double rtol = 0) {
    context = "binary l=" + real(l) + " r=" + real(r) + " center=" + real(center) + " limit=" + std::to_string(limit);
    for (bool reverse : {false, true}) {
        int calls = 0;
        auto f = [&](double x) { require(l < x && x < r, "binary interior finite evaluations"); ++calls; return reverse ? x >= center : x <= center; };
        auto res = binSearchRealBracket(reverse ? r : l, reverse ? l : r, f, limit, atol, rtol);
        checkResult(res, l, r, limit, atol, rtol, true);
        require(res.l <= center && center <= res.r, "binary brackets exact threshold");
        require(reverse ? res.x >= center : res.x <= center, "binary feasible returned endpoint");
        expect(calls, res.iterations, "binary one evaluation per reduction");
        long double w = static_cast<long double>(r) - l, got = static_cast<long double>(res.r) - res.l;
        long double rounding = 8 * std::numeric_limits<double>::epsilon() * max(abs(l), abs(r)) + 8 * std::numeric_limits<double>::denorm_min();
        require(got <= std::ldexp(w, -res.iterations) + rounding, "binary contraction plus rounding");}}

void realMinimum(double l, double r, double p, double q, int limit, double atol = 0, double rtol = 0) {
    context = "minimum l=" + real(l) + " r=" + real(r) + " p=" + real(p) + " q=" + real(q) + " limit=" + std::to_string(limit);
    for (bool golden : {false, true}) {
        int calls = 0;
        auto f = [&](double x) -> long double {
            require(l < x && x < r, "minimum interior finite evaluations"); ++calls;
            return x < p ? static_cast<long double>(p) - x : x > q ? static_cast<long double>(x) - q : 0;};
        auto res = golden ? goldenSearchRealBracket(l, r, f, limit, atol, rtol) : ternSearchRealBracket(l, r, f, limit, atol, rtol);
        checkResult(res, l, r, limit, atol, rtol);
        require(res.l <= q && p <= res.r, "minimum bracket intersects analytic minimizers", "intersection",
            "[" + real(res.l) + "," + real(res.r) + "]");
        require(calls <= (golden ? (res.iterations ? res.iterations + 1 : 0) : 2 * res.iterations), "minimum evaluation bound");
        long double w = static_cast<long double>(r) - l, got = static_cast<long double>(res.r) - res.l;
        long double ratio = golden ? (std::sqrt(5.0L) - 1) / 2 : 2.0L / 3;
        long double rounding = 32 * std::numeric_limits<double>::epsilon() * max(abs(l), abs(r)) + 32 * std::numeric_limits<double>::denorm_min();
        require(got <= w * std::pow(ratio, res.iterations) + rounding, "minimum contraction plus rounding");}}

void realBoundaries() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min();
    vector<pair<double, double>> intervals{{-huge, huge}, {-huge, 0}, {0, huge}, {-2, 3}, {-tiny, tiny},
        {0, tiny}, {1, std::nextafter(1.0, 2.0)}, {-2, -2}, {huge, huge}, {-0.0, 0.0}, {tiny, 3 * tiny},
        // Three doubles across a binade edge: under FMA contraction lerp(l, r, 1.0 / 3) rounds onto l on the negative one, then its mirror.
        {-0x1.0000000000001p+0, -0x1.fffffffffffffp-1}, {0x1.fffffffffffffp-1, 0x1.0000000000001p+0}};
    for (auto [l, r] : intervals) {
        double c = std::midpoint(l, r);
        for (int limit : {0, 1, 2, 7, 60, 2200}) {
            realBinary(l, r, c, limit);
            realMinimum(l, r, c, c, limit);
            realMinimum(l, r, l, l, limit);
            realMinimum(l, r, r, r, limit);
            realMinimum(l, r, l, r, limit);}
        realBinary(l, r, c, 2200, tiny, 1e-12);
        realMinimum(l, r, c, c, 2200, tiny, 1e-12);}
    for (double z : {-0.0, 0.0}) {
        context = "two-zero bracket ok=" + real(z);
        auto res = binSearchRealBracket(z, -z, [](double) { require(false, "two-zero bracket makes no call"); return true; });
        require(std::signbit(res.l) == std::signbit(z) && std::signbit(res.r) == std::signbit(z) && std::signbit(res.x) == std::signbit(z) && res.iterations == 0,
            "two-zero bracket returns ok's zero as l, r and x", real(z), real(res.l) + "," + real(res.r) + "," + real(res.x));}
    auto predicate = [](double x) { return x < 0.7; };
    auto objective = [](double x) { return abs(x - 0.7); };
    require(binSearchRealBracket(0, 2, predicate).x <= 0.7, "binary default iteration argument");
    context = "default arguments on [0, 2] around 0.7";
    for (auto [res, limit, bisection] : {tuple{binSearchRealBracket(0, 2, predicate), 100, true}, tuple{ternSearchRealBracket(0, 2, objective), 200, false},
        tuple{goldenSearchRealBracket(0, 2, objective), 100, false}}) {
        checkResult(res, 0, 2, limit, 0, 0, bisection);
        require(res.l <= 0.7 && 0.7 <= res.r && abs(res.x - 0.7) <= 1e-14, "bracket default iteration argument", "0.7", real(res.x));}
    std::cout << "PASS real limits, subnormals, adjacent points, caps, tolerance and call bounds\n";}

void toleranceCases() {
    for (auto [l, r, c, atol, rtol] : {tuple{0.0, 1e6, 500000.123, 1e-3, 0.0}, tuple{1e6, 2e6, 1234567.891, 0.0, 1e-9}}) {
        context = "tolerance l=" + real(l) + " r=" + real(r) + " c=" + real(c) + " atol=" + real(atol) + " rtol=" + real(rtol);
        auto below = [&](double x) { return x <= c; };
        auto vee = [&](double x) { return abs(static_cast<long double>(x) - c); };
        auto quad = [&](double x) { long double y = static_cast<long double>(x) - c; return y * y; };
        auto sign = [&](double x) -> long double { return (x > c) - (x < c); };
        auto slope = [&](double x) { return 2 * (static_cast<long double>(x) - c); };
        vector<tuple<string, RealSearchResult, int, bool>> runs{
            {"brentSearchRealBracketDerivative V", brentSearchRealBracketDerivative(l, r, vee, sign, 100, atol, rtol), 100, false},
            {"brentSearchRealBracketDerivative quadratic", brentSearchRealBracketDerivative(l, r, quad, slope, 100, atol, rtol), 100, false},
            {"binSearchRealBracket", binSearchRealBracket(l, r, below, 100, atol, rtol), 100, true},
            {"binSearchRealGeometric", binSearchRealGeometric(max(l, 1.0), r, below, 100, atol, rtol), 100, true},
            {"ternSearchRealBracket", ternSearchRealBracket(l, r, vee, 200, atol, rtol), 200, false},
            {"goldenSearchRealBracket", goldenSearchRealBracket(l, r, vee, 100, atol, rtol), 100, false},
            {"brentSearchRealBracket V", brentSearchRealBracket(l, r, vee, 100, atol, rtol), 100, false},
            {"brentSearchRealBracket quadratic", brentSearchRealBracket(l, r, quad, 100, atol, rtol), 100, false}};
        for (auto &[name, res, limit, bisection] : runs) {
            checkResult(res, l, r, limit, atol, rtol, bisection);
            require(res.converged && res.l <= c && c <= res.r, name + " converges within default itr around the target", "converged", "[" + real(res.l) + "," + real(res.r) + "]");}}
    std::cout << "PASS absolute and relative tolerances converge within default caps\n";}

void tieRule() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min();
    for (auto [l, r] : {pair{-2.0, 3.0}, pair{0.0, 1e6}, pair{-huge, huge}, pair{tiny, 100 * tiny}}) {
        double p = std::lerp(l, r, 0.2), q = std::lerp(l, r, 0.8);
        auto flat = [](double) { return 0.0L; };
        auto plateau = [&](double x) { return x < p ? static_cast<long double>(p) - x : x > q ? static_cast<long double>(x) - q : 0; };
        for (bool golden : {false, true}) {
            context = string(golden ? "golden" : "ternary") + " tie l=" + real(l) + " r=" + real(r);
            for (int limit : {1, 2, 7, 200}) {
                auto res = golden ? goldenSearchRealBracket(l, r, flat, limit) : ternSearchRealBracket(l, r, flat, limit);
                require(res.iterations >= 1 && res.l == l && res.r < r, "constant objective drops the right part", "[" + real(l) + ",<r]", "[" + real(res.l) + "," + real(res.r) + "]");}
            auto res = golden ? goldenSearchRealBracket(l, r, plateau, 1) : ternSearchRealBracket(l, r, plateau, 1);
            require(res.iterations == 1 && res.l == l && std::lerp(l, r, 0.6) <= res.r && res.r <= std::lerp(l, r, 0.7), "plateau tie keeps the left part", "[" + real(l) + ",<r]", "[" + real(res.l) + "," + real(res.r) + "]");}}
    std::cout << "PASS ternary and golden equal-probe tie keeps the left part\n";}

double geometricSplit(double ok, double ng) {
    double g = std::copysign(std::sqrt(abs(ok)) * std::sqrt(abs(ng)), ok);
    return min(ok, ng) < g && g < max(ok, ng) ? g : std::midpoint(ok, ng);}
void realGeometric(double lo, double hi, double t, int limit, double atol = 0, double rtol = 0) {
    context = "geometric lo=" + real(lo) + " hi=" + real(hi) + " t=" + real(t) + " limit=" + std::to_string(limit)
        + " atol=" + real(atol) + " rtol=" + real(rtol);
    for (bool reverse : {false, true}) {
        double ok = reverse ? hi : lo, ng = reverse ? lo : hi, first = 0; int calls = 0;
        auto f = [&](double x) {
            require(lo < x && x < hi, "geometric interior finite evaluations");
            if (!calls++) { first = x; }
            return reverse ? x >= t : x <= t;};
        auto res = binSearchRealGeometric(ok, ng, f, limit, atol, rtol);
        checkResult(res, lo, hi, limit, atol, rtol, true);
        require(res.l <= t && t <= res.r, "geometric brackets threshold", real(t), "[" + real(res.l) + "," + real(res.r) + "]");
        require(res.x == (reverse ? res.r : res.l) && (reverse ? res.x >= t : res.x <= t), "geometric x is the true endpoint");
        expect(calls, res.iterations, "geometric one evaluation per reduction");
        require(calls == 0 || first == geometricSplit(ok, ng), "geometric first split", real(geometricSplit(ok, ng)), real(first));
        long double w = abs(std::log(static_cast<long double>(hi) / lo)), got = abs(std::log(static_cast<long double>(res.r) / res.l));
        long double rounding = 8 * std::numeric_limits<double>::epsilon() + 4 * std::numeric_limits<double>::denorm_min() / min(abs(lo), abs(hi));
        require(got <= std::ldexp(w, -res.iterations) + rounding, "geometric log-scale contraction plus rounding");}}

void geometricBoundaries() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min(), low = std::numeric_limits<double>::min();
    double fb = 2.6854607426685108e-16, fb2 = std::nextafter(std::nextafter(fb, 1.0), 1.0);
    vector<pair<double, double>> intervals{{tiny, huge}, {1e-300, 1e300}, {1, 2}, {0.5, 1e10}, {tiny, 2 * tiny}, {tiny, 3 * tiny},
        {low / 4, 4 * low}, {1, std::nextafter(1.0, 2.0)}, {fb, fb2}, {huge / 2, huge}, {3, 3}, {tiny, tiny}, {huge, huge}};
    vector<double> marks{tiny, 3 * tiny, std::nextafter(low, 0.0), low, 1e-300, 1e-100, 0.5, 1, 3, 1e100, 1e300, huge};
    for (auto [l, r] : intervals) {
        vector<double> ts{l, r, std::nextafter(l, r), std::nextafter(r, l), std::sqrt(l) * std::sqrt(r), std::midpoint(l, r)};
        ts.insert(ts.end(), marks.begin(), marks.end());
        for (double t : ts) {
            if (t < l || t > r) { continue; }
            for (int limit : {0, 1, 2, 7, 100, 2200}) { realGeometric(l, r, t, limit); realGeometric(-r, -l, -t, limit); }
            for (auto [atol, rtol] : {pair{0.0, 1e-6}, pair{1e-300, 0.0}, pair{tiny, 1e-12}}) {
                realGeometric(l, r, t, 100, atol, rtol); realGeometric(-r, -l, -t, 100, atol, rtol);}}}
    context = "geometric midpoint fallback fb=" + real(fb);
    double g = std::sqrt(fb) * std::sqrt(fb2);
    require(!(fb < g && g < fb2) && fb < geometricSplit(fb, fb2) && geometricSplit(fb, fb2) < fb2, "geometric fallback regression reaches midpoint");
    int most = 0;
    for (double t : marks) {
        for (double s : {1.0, -1.0}) {
            context = "geometric full range t=" + real(s * t);
            auto res = binSearchRealGeometric(s * tiny, s * huge, [&](double x) { return s * x <= t; });
            require(res.converged && std::nextafter(res.l, res.r) == res.r, "geometric full range reaches adjacent doubles");
            require(res.l <= s * t && s * t <= res.r, "geometric full range brackets threshold");
            most = max(most, res.iterations);}}
    context = "geometric relative stop";
    auto unit = [](double x) { return x <= 1; };
    auto all = binSearchRealGeometric(1e-300, 1e300, unit), rel = binSearchRealGeometric(1e-300, 1e300, unit, 100, 0, 1e-6);
    require(all.converged && all.l <= 1 && 1 <= all.r, "geometric exact threshold to adjacent doubles");
    require(rel.converged && rel.iterations < all.iterations && rel.iterations <= 40, "geometric relative tolerance stops early", "<=40", std::to_string(rel.iterations));
    require(static_cast<long double>(rel.r) - rel.l <= 1e-6L * rel.r && rel.l <= 1 && 1 <= rel.r, "geometric relative width bound");
    auto never = [](double) { require(false, "geometric settled input makes no call"); return false; };
    for (auto res : {binSearchRealGeometric(1, 2, never, 100, 0, 1), binSearchRealGeometric(-2, -1, never, 100, 1),
        binSearchRealGeometric(1, 2, never, 0), binSearchRealGeometric(5, std::nextafter(5.0, 6.0), never)}) {
        expect(res.iterations, 0, "geometric zero calls");}
    auto root = binSearchRealGeometric(1, 4, [](double x) { return x * x <= 2; });
    require(root.converged && abs(root.x - std::sqrt(2.0)) <= 4e-16, "geometric default arguments", real(std::sqrt(2.0)), real(root.x));
    std::cout << "PASS geometric thresholds across magnitudes, both signs and orientations, full range iterations=" << most << '\n';}

// IEEE ordinal of a finite double by arithmetic (no bit casts): subnormal count, then 2^52 per binade.
lng ordinal(double x) {
    if (x == 0) { return 0; }
    if (x < 0) { return -ordinal(-x); }
    if (x < std::numeric_limits<double>::min()) { return lng(x / std::numeric_limits<double>::denorm_min()); }
    int e = std::ilogb(x);
    return lng(e + 1022) * (lng(1) << 52) + lng(std::scalbn(x, 52 - e));}
void realBits(double lo, double hi, double t, int limit, double atol = 0, double rtol = 0) {
    context = "bits lo=" + real(lo) + " hi=" + real(hi) + " t=" + real(t) + " limit=" + std::to_string(limit)
        + " atol=" + real(atol) + " rtol=" + real(rtol);
    for (bool reverse : {false, true}) {
        double ok = reverse ? hi : lo, ng = reverse ? lo : hi, first = 0; int calls = 0;
        auto f = [&](double x) {
            require(lo < x && x < hi, "bits interior finite evaluations");
            if (!calls++) { first = x; }
            return reverse ? x >= t : x <= t;};
        auto res = binSearchRealBits(ok, ng, f, limit, atol, rtol);
        checkResult(res, lo, hi, limit, atol, rtol, true);
        require(res.l <= t && t <= res.r, "bits brackets threshold", real(t), "[" + real(res.l) + "," + real(res.r) + "]");
        require(res.x == (reverse ? res.r : res.l) && (reverse ? res.x >= t : res.x <= t), "bits x is the true endpoint");
        expect(calls, res.iterations, "bits one evaluation per reduction");
        require(calls == 0 || ordinal(first) == std::midpoint(ordinal(ok), ordinal(ng)), "bits first split is the ordinal midpoint toward ok",
            std::to_string(std::midpoint(ordinal(ok), ordinal(ng))), std::to_string(ordinal(first)));
        lll w = lll(ordinal(hi)) - ordinal(lo), got = lll(ordinal(res.r)) - ordinal(res.l);
        require(got <= (w ? ((w - 1) >> min(res.iterations, 126)) + 1 : 0), "bits ordinal width halves per reduction");
        require(res.iterations <= 64, "bits at most 64 reductions", "<=64", std::to_string(res.iterations));}}

void bitsBoundaries() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min(), low = std::numeric_limits<double>::min();
    context = "bits ordinal map";
    for (double x : {0.0, tiny, 2 * tiny, std::nextafter(low, 0.0), low, std::nextafter(low, 1.0), 0.5, 1.0, std::nextafter(1.0, 2.0), 3.0, huge / 2, huge}) {
        lng i = std::bit_cast<lng>(x);
        require(ordinal(x) == i && ordinal(-x) == -i, "ordinal matches the IEEE bit pattern", std::to_string(i), std::to_string(ordinal(x)));}
    vector<pair<double, double>> intervals{{-huge, huge}, {-huge, 0}, {0, huge}, {-0.0, 0.0}, {-tiny, tiny}, {-tiny, 0.0}, {-0.0, tiny},
        {-1, 1}, {-0.0, 1}, {-1, -0.0}, {1, 2}, {-3, 1e-300}, {-1e300, 1e-300}, {tiny, 3 * tiny}, {low / 4, 4 * low}, {1, std::nextafter(1.0, 2.0)}, {3, 3}, {-2, -2}, {huge / 2, huge}};
    vector<double> marks{0.0, tiny, 3 * tiny, low, 1e-300, 0.5, 1, 3, 1e300, huge};
    for (auto [l, r] : intervals) {
        vector<double> ts{l, r, std::nextafter(l, r), std::nextafter(r, l), std::midpoint(l, r), -0.0};
        for (double m : marks) { ts.push_back(m); ts.push_back(-m); }
        for (double t : ts) {
            if (t < l || t > r) { continue; }
            for (int limit : {0, 1, 2, 7, 63, 64, 100}) { realBits(l, r, t, limit); }
            for (auto [atol, rtol] : {pair{0.0, 1e-6}, pair{1e-300, 0.0}, pair{tiny, 1e-12}}) { realBits(l, r, t, 100, atol, rtol); }}}
    int most = 0;
    for (double t : marks) {
        for (double s : {1.0, -1.0}) {
            context = "bits full range t=" + real(s * t);
            auto res = binSearchRealBits(-huge, huge, [&](double x) { return x <= s * t; });
            require(res.converged && std::nextafter(res.l, res.r) == res.r && res.l <= s * t && s * t <= res.r, "bits full range reaches adjacent doubles around the threshold");
            most = max(most, res.iterations);}}
    require(most <= 64, "bits full range within 64 reductions", "<=64", std::to_string(most));
    auto root = binSearchRealBits(1, 4, [](double x) { return x * x <= 2; });
    require(root.converged && abs(root.x - std::sqrt(2.0)) <= 4e-16, "bits default arguments", real(std::sqrt(2.0)), real(root.x));
    std::cout << "PASS bit-pattern thresholds across zero, signs, magnitudes and orientations, full range iterations=" << most << '\n';}

constexpr double CGOLD = 0.38196601125010515180;
#ifdef __FMA__
constexpr bool FMA = true;
#else
constexpr bool FMA = false;
#endif
// The first probe is lerp(l, r, CGOLD); under FMA contraction (-march=native) the header's inlined lerp rounds its two products once, which can move it by a few ulps of max(|l|, |r|).
bool goldenStart(double p, double m, double l, double r) {
    if (!FMA) { return p == m; }
    return abs(static_cast<long double>(p) - m) <= 4 * DBL_EPSILON * static_cast<long double>(max(abs(l), abs(r))) + std::numeric_limits<double>::denorm_min();}
template<typename G>
RealSearchResult realBrent(double l, double r, G g, double p, double q, int limit, double atol = 0, double rtol = 0, double slack = 0) {
    context = "brent l=" + real(l) + " r=" + real(r) + " p=" + real(p) + " q=" + real(q) + " limit=" + std::to_string(limit)
        + " atol=" + real(atol) + " rtol=" + real(rtol) + shape;
    vector<pair<double, long double>> probes;
    auto f = [&](double x) -> long double {
        require(l < x && x < r, "brent interior finite evaluations");
        long double y = g(x); probes.push_back({x, y}); return y;};
    auto res = brentSearchRealBracket(l, r, f, limit, atol, rtol);
    checkResult(res, l, r, limit, atol, rtol);
    require(res.l <= q + slack && p - slack <= res.r, "brent bracket intersects minimizers", "intersection", "[" + real(res.l) + "," + real(res.r) + "]");
    int calls = int(probes.size()); double m = std::lerp(l, r, CGOLD);
    require(calls <= limit + 1, "brent evaluation bound", "<=" + std::to_string(limit + 1), std::to_string(calls));
    require((calls > 0) == (limit > 0 && !settled(l, r, atol, rtol) && l < m && m < r), "brent starts exactly when the golden point is usable");
    if (!calls) {
        require(res.iterations == 0 && res.l == l && res.r == r && res.x == std::midpoint(l, r), "brent zero evaluations keep the midpoint");
        return res;}
    expect(res.iterations, calls - 1, "brent iterations are evaluations after the first");
    require(goldenStart(probes[0].first, m, l, r), "brent first golden point", real(m), real(probes[0].first));
    long double best = probes[0].second; bool found = false;
    for (auto [x, y] : probes) { best = min(best, y); }
    for (auto [x, y] : probes) { found |= x == res.x && y == best; }
    require(found, "brent x is the least probe", real(res.x), "not a least probe");
    return res;}

void brentBoundaries() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min();
    auto at = [](double x, double y) { return static_cast<long double>(x) - y; };
    vector<pair<double, double>> intervals{{-huge, huge}, {-huge, 0}, {0, huge}, {-2, 3}, {-tiny, tiny}, {0, tiny}, {0, 100 * tiny},
        {1, std::nextafter(1.0, 2.0)}, {-2, -2}, {huge, huge}, {-0.0, 0.0}, {-0.0, 1}, {-1, 0.0}, {tiny, 3 * tiny}};
    for (auto [l, r] : intervals) {
        double c = std::midpoint(l, r), p = std::lerp(l, r, 0.25), q = std::lerp(l, r, 0.6);
        auto vee = [&](double x) { return abs(at(x, c)); };
        for (int limit : {0, 1, 2, 7, 100, 2200}) {
            realBrent(l, r, vee, c, c, limit);
            realBrent(l, r, [&](double x) { return x < c ? at(c, x) : 1000 * at(x, c); }, c, c, limit);
            realBrent(l, r, [&](double x) { return x < c ? 1000 * at(c, x) : at(x, c) / 1000; }, c, c, limit);
            realBrent(l, r, [&](double x) { return x < p ? at(p, x) : x > q ? 3 * at(x, q) : 0; }, p, q, limit);
            realBrent(l, r, [](double x) { return static_cast<long double>(x); }, l, l, limit);
            realBrent(l, r, [](double x) { return -static_cast<long double>(x); }, r, r, limit);
            realBrent(l, r, [](double) { return 7.0L; }, l, r, limit);}
        realBrent(l, r, vee, c, c, 2200, tiny, 1e-12);}
    double pi = std::numbers::pi;
    auto smooth = [&](double l, double r, auto g, double m, double slack) {
        auto res = realBrent(l, r, g, m, m, 200, 1e-12, 0, slack);
        require(res.converged && static_cast<long double>(res.r) - res.l <= 1e-12L, "brent smooth convergence to abs_tol");
        require(abs(res.x - m) <= 1e-12 + slack, "brent smooth best point near minimizer", real(m), real(res.x));};
    smooth(-3, 5, [&](double x) { return 2.5L * at(x, 1.234) * at(x, 1.234); }, 1.234, 0);
    smooth(-1e-3, 1e-3, [&](double x) { return at(x, 1e-5) * at(x, 1e-5); }, 1e-5, 0);
    smooth(100, 300, [&](double x) { return 3 * at(x, 200.5) * at(x, 200.5); }, 200.5, 0);
    smooth(-3, 5, [&](double x) { return std::pow(at(x, -0.7), 4); }, -0.7, 0);
    smooth(-3, 5, [&](double x) { long double y = at(x, 2.25); return y * y * y * y + y * y; }, 2.25, 0);
    // Long double cos, exp and sin are flat within about sqrt(2^-63) ~ 3.3e-10 of their minima.
    smooth(2, 4, [](double x) { return std::cos(static_cast<long double>(x)); }, pi, 2e-9);
    smooth(-5, 5, [](double x) { return std::exp(static_cast<long double>(x)) - 2 * static_cast<long double>(x); }, std::numbers::ln2, 2e-9);
    smooth(0, 3, [](double x) { return -std::sin(static_cast<long double>(x)); }, pi / 2, 2e-9);
    int calls = 0;
    auto quad = [&](double x) { ++calls; return at(x, 1.234) * at(x, 1.234); };
    context = "brent against golden quadratic";
    auto brent = brentSearchRealBracket(-3, 5, quad, 200, 1e-12); int used = calls; calls = 0;
    auto golden = goldenSearchRealBracket(-3, 5, quad, 200, 1e-12);
    require(brent.converged && golden.converged && used <= calls, "brent no more evaluations than golden", "<=" + std::to_string(calls), std::to_string(used));
    auto def = brentSearchRealBracket(0, 2, [&](double x) { return abs(at(x, 1)); });
    require(abs(def.x - 1) <= 1e-14 && def.iterations <= 100, "brent default arguments", "1", real(def.x));
    std::cout << "PASS brent shapes, plateaus, endpoints, extremes, smooth minima, evaluations brent=" << used << " golden=" << calls << '\n';}

// Numerical Recipes 10.2 brent with the contract's tol1, long double steps and stagnation stops.
template<typename G>
RealSearchResult brentReference(double a, double b, G f, int itr, double atol, double rtol) {
    constexpr long double GOLD = 0.38196601125010515180L;
    long double eps = std::numeric_limits<double>::epsilon(), zeps = std::numeric_limits<double>::denorm_min();
    double x = std::lerp(a, b, double(GOLD));
    if (itr == 0 || settled(a, b, atol, rtol) || !(a < x && x < b)) { return {a, b, std::midpoint(a, b), 0, settled(a, b, atol, rtol)}; }
    double w = x, v = x;
    auto fx = f(x), fw = fx, fv = fx;
    long double d = 0, e = 0;
    int iter = 0;
    while (iter < itr && !settled(a, b, atol, rtol)) {
        long double X = x, xm = 0.5L * a + 0.5L * b;
        long double tol1 = max(0.25L * atol + 0.25L * rtol * abs(X), 2 * eps * abs(X) + zeps), tol2 = 2 * tol1;
        bool parabolic = false;
        if (abs(e) > tol1) {
            long double rr = (X - w) * (fx - fv), q = (X - v) * (fx - fw), p = (X - v) * q - (X - w) * rr;
            q = 2 * (q - rr);
            if (q > 0) { p = -p; }
            q = abs(q);
            long double etemp = e; e = d;
            if (!(abs(p) >= abs(0.5L * q * etemp) || p <= q * (a - X) || p >= q * (b - X))) {
                parabolic = true; d = p / q;
                long double u = X + d;
                if (u - a < tol2 || b - u < tol2) { d = xm - X >= 0 ? tol1 : -tol1; }}}
        if (!parabolic) {
            e = X >= xm ? a - X : b - X;
            d = GOLD * e;}
        double u = double(abs(d) >= tol1 ? X + d : X + (d >= 0 ? tol1 : -tol1));
        if (!(a < u && u < b) || u == x) { break; }
        auto fu = f(u); ++iter;
        if (fu <= fx) {
            if (u >= x) { a = x; }
            else { b = x; }
            v = w; w = x; x = u;
            fv = fw; fw = fx; fx = fu;}
        else {
            if (u < x) { a = u; }
            else { b = u; }
            if (fu <= fw || w == x) {
                v = w; w = u;
                fv = fw; fw = fu;}
            else if (fu <= fv || v == x || v == w) {
                v = u; fv = fu;}}}
    return {a, b, x, iter, settled(a, b, atol, rtol)};}

template<typename G>
void brentTrace(double l, double r, G g, int limit, double atol, double rtol) {
    // GCC contracts each inlined std::lerp on its own, so header and reference may round a split differently; the other builds pin the traces.
    if (FMA) { return; }
    context = "brent trace l=" + real(l) + " r=" + real(r) + " limit=" + std::to_string(limit) + " atol=" + real(atol) + " rtol=" + real(rtol) + shape;
    vector<double> got, want;
    auto res = brentSearchRealBracket(l, r, [&](double x) { got.push_back(x); return g(x); }, limit, atol, rtol);
    auto ref = brentReference(l, r, [&](double x) { want.push_back(x); return g(x); }, limit, atol, rtol);
    size_t i = 0;
    while (i < got.size() && i < want.size() && got[i] == want[i]) { ++i; }
    require(got == want, "brent probe trace equals the reference at probe " + std::to_string(i),
        i < want.size() ? real(want[i]) : "end", i < got.size() ? real(got[i]) : "end");
    require(res.l == ref.l && res.r == ref.r && res.x == ref.x && res.iterations == ref.iterations && res.converged == ref.converged,
        "brent result equals the reference", "[" + real(ref.l) + "," + real(ref.r) + "] x=" + real(ref.x) + " it=" + std::to_string(ref.iterations),
        "[" + real(res.l) + "," + real(res.r) + "] x=" + real(res.x) + " it=" + std::to_string(res.iterations));}

// Numerical Recipes 10.3 dbrent with brentReference's start, tol1 and stops, a probe on the other side when the downhill one leaves the bracket (a tie there counts as uphill), and no return after an uphill tol1 step.
template<typename G, typename H>
RealSearchResult dbrentReference(double a, double b, G f, H df, int itr, double atol, double rtol) {
    constexpr long double GOLD = 0.38196601125010515180L;
    long double eps = std::numeric_limits<double>::epsilon(), zeps = std::numeric_limits<double>::denorm_min();
    double x = std::lerp(a, b, double(GOLD));
    if (itr == 0 || settled(a, b, atol, rtol) || !(a < x && x < b)) { return {a, b, std::midpoint(a, b), 0, settled(a, b, atol, rtol)}; }
    double w = x, v = x;
    auto fx = f(x), fw = fx, fv = fx;
    auto dx = df(x), dw = dx, dv = dx;
    long double d = 0, e = 0;
    int iter = 0;
    while (iter < itr && !settled(a, b, atol, rtol)) {
        long double X = x, xm = 0.5L * a + 0.5L * b;
        long double tol1 = max(0.25L * atol + 0.25L * rtol * abs(X), 2 * eps * abs(X) + zeps), tol2 = 2 * tol1;
        bool secant = false;
        if (abs(e) > tol1) {
            long double d1 = 2 * (static_cast<long double>(b) - a), d2 = d1;
            if (dw != dx) { d1 = (w - X) * dx / (dx - dw); }
            if (dv != dx) { d2 = (v - X) * dx / (dx - dv); }
            long double u1 = X + d1, u2 = X + d2;
            bool ok1 = a < u1 && u1 < b && dx * d1 <= 0, ok2 = a < u2 && u2 < b && dx * d2 <= 0;
            long double olde = e; e = d;
            if (ok1 || ok2) {
                long double step = d2;
                if (ok1 && ok2) { step = abs(d1) < abs(d2) ? d1 : d2; }
                else if (ok1) { step = d1; }
                if (abs(step) <= abs(0.5L * olde)) {
                    secant = true; d = step;
                    long double u = X + d;
                    if (u - a < tol2 || b - u < tol2) { d = xm - X >= 0 ? tol1 : -tol1; }}}}
        if (!secant) {
            e = dx >= 0 ? a - X : b - X;
            d = 0.5L * e;}
        double u = double(abs(d) >= tol1 ? X + d : X + (d >= 0 ? tol1 : -tol1));
        bool other = !(a < u && u < b);
        if (other) { u = double(X + (d >= 0 ? -tol1 : tol1)); }
        if (!(a < u && u < b) || u == x) { break; }
        auto fu = f(u); auto du = df(u); ++iter;
        if (other ? fu < fx : fu <= fx) {
            if (u >= x) { a = x; }
            else { b = x; }
            v = w; fv = fw; dv = dw;
            w = x; fw = fx; dw = dx;
            x = u; fx = fu; dx = du;}
        else {
            if (u < x) { a = u; }
            else { b = u; }
            if (fu <= fw || w == x) {
                v = w; fv = fw; dv = dw;
                w = u; fw = fu; dw = du;}
            else if (fu < fv || v == x || v == w) {
                v = u; fv = fu; dv = du;}}}
    return {a, b, x, iter, settled(a, b, atol, rtol)};}

template<typename G, typename H>
void dbrentTrace(double l, double r, G g, H dg, int limit, double atol, double rtol) {
    // GCC contracts each inlined std::lerp on its own, so header and reference may round a split differently; the other builds pin the traces.
    if (FMA) { return; }
    context = "dbrent trace l=" + real(l) + " r=" + real(r) + " limit=" + std::to_string(limit) + " atol=" + real(atol) + " rtol=" + real(rtol) + shape;
    vector<pair<int, double>> got, want;
    auto res = brentSearchRealBracketDerivative(l, r, [&](double x) { got.push_back({0, x}); return g(x); },
        [&](double x) { got.push_back({1, x}); return dg(x); }, limit, atol, rtol);
    auto ref = dbrentReference(l, r, [&](double x) { want.push_back({0, x}); return g(x); },
        [&](double x) { want.push_back({1, x}); return dg(x); }, limit, atol, rtol);
    size_t i = 0;
    while (i < got.size() && i < want.size() && got[i] == want[i]) { ++i; }
    auto call = [](const vector<pair<int, double>> &s, size_t i) { return i < s.size() ? (s[i].first ? "df(" : "f(") + real(s[i].second) + ")" : string("end"); };
    require(got == want, "dbrent call trace equals the reference at call " + std::to_string(i), call(want, i), call(got, i));
    require(res.l == ref.l && res.r == ref.r && res.x == ref.x && res.iterations == ref.iterations && res.converged == ref.converged,
        "dbrent result equals the reference", "[" + real(ref.l) + "," + real(ref.r) + "] x=" + real(ref.x) + " it=" + std::to_string(ref.iterations),
        "[" + real(res.l) + "," + real(res.r) + "] x=" + real(res.x) + " it=" + std::to_string(res.iterations));}

// Contract checks of brentSearchRealBracketDerivative on f with minimizers [p, q], then its trace against dbrentReference.
template<typename G, typename H>
RealSearchResult realDbrent(double l, double r, G g, H dg, double p, double q, int limit, double atol = 0, double rtol = 0, double slack = 0) {
    string where = "dbrent l=" + real(l) + " r=" + real(r) + " p=" + real(p) + " q=" + real(q) + " limit=" + std::to_string(limit)
        + " atol=" + real(atol) + " rtol=" + real(rtol) + shape;
    context = where;
    vector<pair<double, long double>> probes; vector<double> slopes;
    auto f = [&](double x) -> long double {
        require(l < x && x < r, "dbrent interior f calls");
        require(probes.size() == slopes.size(), "dbrent f follows a df call");
        long double y = g(x); probes.push_back({x, y}); return y;};
    auto df = [&](double x) -> long double {
        require(l < x && x < r, "dbrent interior df calls");
        require(slopes.size() + 1 == probes.size() && probes.back().first == x, "dbrent df follows f at the same point");
        slopes.push_back(x); return dg(x);};
    auto res = brentSearchRealBracketDerivative(l, r, f, df, limit, atol, rtol);
    checkResult(res, l, r, limit, atol, rtol);
    require(res.l <= q + slack && p - slack <= res.r, "dbrent bracket intersects minimizers", "intersection", "[" + real(res.l) + "," + real(res.r) + "]");
    int calls = int(probes.size()); double m = std::lerp(l, r, CGOLD);
    expect(lng(slopes.size()), calls, "dbrent as many df calls as f calls");
    require(calls <= limit + 1, "dbrent call bound", "<=" + std::to_string(limit + 1), std::to_string(calls));
    require((calls > 0) == (limit > 0 && !settled(l, r, atol, rtol) && l < m && m < r), "dbrent starts exactly when the golden point is usable");
    if (!calls) { require(res.iterations == 0 && res.l == l && res.r == r && res.x == std::midpoint(l, r), "dbrent no calls keep the midpoint"); }
    else {
        expect(res.iterations, calls - 1, "dbrent iterations are calls after the first");
        require(goldenStart(probes[0].first, m, l, r), "dbrent first golden point", real(m), real(probes[0].first));
        long double best = probes[0].second; bool found = false;
        for (auto [x, y] : probes) { best = min(best, y); }
        for (auto [x, y] : probes) { found |= x == res.x && y == best; }
        require(found, "dbrent x is the least probe", real(res.x), "not a least probe");}
    dbrentTrace(l, r, g, dg, limit, atol, rtol); context = where;
    return res;}

void dbrentBoundaries() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min();
    auto at = [](double x, double y) { return static_cast<long double>(x) - y; };
    auto sgn = [](long double y) -> long double { return (y > 0) - (y < 0); };
    vector<pair<double, double>> intervals{{-huge, huge}, {-huge, 0}, {0, huge}, {-2, 3}, {-tiny, tiny}, {0, tiny}, {0, 100 * tiny},
        {1, std::nextafter(1.0, 2.0)}, {-2, -2}, {huge, huge}, {-0.0, 0.0}, {-0.0, 1}, {-1, 0.0}, {tiny, 3 * tiny}};
    for (auto [l, r] : intervals) {
        double c = std::midpoint(l, r), p = std::lerp(l, r, 0.25), q = std::lerp(l, r, 0.6), s = std::lerp(l, r, 0.9);
        auto vee = [&](double x) { return abs(at(x, c)); };
        auto dvee = [&](double x) { return sgn(at(x, c)); };
        auto plateau = [&](double x) { return x < p ? at(p, x) : x > q ? 3 * at(x, q) : 0; };
        auto dplateau = [&](double x) -> long double { return x < p ? -1 : x > q ? 3 : 0; };
        for (int limit : {0, 1, 2, 7, 100, 2200}) {
            realDbrent(l, r, vee, dvee, c, c, limit);
            realDbrent(l, r, [&](double x) { return x < c ? at(c, x) : 1000 * at(x, c); }, [&](double x) -> long double { return x < c ? -1 : 1000; }, c, c, limit);
            realDbrent(l, r, [&](double x) { return x < c ? 1000 * at(c, x) : at(x, c) / 1000; }, [&](double x) -> long double { return x < c ? -1000 : 0.001L; }, c, c, limit);
            realDbrent(l, r, plateau, dplateau, p, q, limit);
            realDbrent(l, r, [](double x) { return static_cast<long double>(x); }, [](double) { return 1.0L; }, l, l, limit);
            realDbrent(l, r, [](double x) { return -static_cast<long double>(x); }, [](double) { return -1.0L; }, r, r, limit);
            realDbrent(l, r, [](double) { return 7.0L; }, [](double) { return 0.0L; }, l, r, limit);
            // Wrong derivatives steer the probes only; the bracket follows f values.
            realDbrent(l, r, vee, [](double) { return 0.0L; }, c, c, limit);
            realDbrent(l, r, vee, [&](double x) { return -dvee(x); }, c, c, limit);
            realDbrent(l, r, vee, [&](double x) { return 2 * at(x, s); }, c, c, limit);
            realDbrent(l, r, plateau, [&](double x) { return -dplateau(x); }, p, q, limit);}
        for (auto [atol, rtol] : {pair{tiny, 1e-12}, pair{1e-12, 0.0}, pair{0.0, 1e-9}}) {
            realDbrent(l, r, vee, dvee, c, c, 2200, atol, rtol);
            realDbrent(l, r, plateau, dplateau, p, q, 2200, atol, rtol);}}
    double pi = std::numbers::pi;
    string counts;
    // level 2: converges to abs_tol with no more f calls than brentSearchRealBracket; 1: converges.
    auto smooth = [&](double l, double r, auto g, auto dg, double m, double slack, int level) {
        auto res = realDbrent(l, r, g, dg, m, m, 200, 1e-12, 0, slack);
        require(res.converged && static_cast<long double>(res.r) - res.l <= 1e-12L, "dbrent smooth convergence to abs_tol");
        require(abs(res.x - m) <= 1e-12 + slack, "dbrent smooth best point near minimizer", real(m), real(res.x));
        int calls = 0;
        auto brent = brentSearchRealBracket(l, r, [&](double x) { ++calls; return g(x); }, 200, 1e-12);
        require(brent.converged && (level < 2 || res.iterations + 1 <= calls), "dbrent no more f calls than brent", "<=" + std::to_string(calls), std::to_string(res.iterations + 1));
        counts += " " + std::to_string(res.iterations + 1) + "/" + std::to_string(calls);
        for (auto wrong : {0, 1, 2}) {
            shape = " wrong derivative " + std::to_string(wrong);
            realDbrent(l, r, g, [&](double x) -> long double { return wrong == 0 ? 0 : wrong == 1 ? -dg(x) : std::cos(static_cast<long double>(x)); }, m, m, 200, 1e-12, 0, slack);}
        shape.clear();};
    smooth(-3, 5, [&](double x) { return 2.5L * at(x, 1.234) * at(x, 1.234); }, [&](double x) { return 5 * at(x, 1.234); }, 1.234, 0, 2);
    smooth(-1e-3, 1e-3, [&](double x) { return at(x, 1e-5) * at(x, 1e-5); }, [&](double x) { return 2 * at(x, 1e-5); }, 1e-5, 0, 2);
    smooth(100, 300, [&](double x) { return 3 * at(x, 200.5) * at(x, 200.5); }, [&](double x) { return 6 * at(x, 200.5); }, 200.5, 0, 2);
    smooth(-3, 5, [&](double x) { return std::pow(at(x, -0.7), 4); }, [&](double x) { return 4 * std::pow(at(x, -0.7), 3); }, -0.7, 0, 1);
    smooth(-3, 5, [&](double x) { long double y = at(x, 2.25); return y * y * y * y + y * y; }, [&](double x) { long double y = at(x, 2.25); return 4 * y * y * y + 2 * y; }, 2.25, 0, 1);
    // Long double cos, exp and sin are flat within about sqrt(2^-63) ~ 3.3e-10 of their minima.
    smooth(2, 4, [](double x) { return std::cos(static_cast<long double>(x)) + 0.1L * x; }, [](double x) { return 0.1L - std::sin(static_cast<long double>(x)); }, pi - std::asin(0.1), 2e-9, 2);
    smooth(-2, 3, [](double x) { return std::exp(-2 * static_cast<long double>(x)) + x; }, [](double x) { return 1 - 2 * std::exp(-2 * static_cast<long double>(x)); }, std::numbers::ln2 / 2, 2e-9, 2);
    smooth(0, pi, [](double x) { return -std::sin(static_cast<long double>(x)); }, [](double x) { return -std::cos(static_cast<long double>(x)); }, pi / 2, 2e-9, 2);
    context = "dbrent default arguments";
    auto def = brentSearchRealBracketDerivative(0, 2, [&](double x) { return abs(at(x, 1)); }, [&](double x) { return sgn(at(x, 1)); });
    require(abs(def.x - 1) <= 1e-14 && def.iterations <= 100, "dbrent default arguments", "1", real(def.x));
    std::cout << "PASS dbrent shapes, plateaus, endpoints, extremes, wrong derivatives, smooth minima, f calls dbrent/brent" << counts << '\n';}

// Numerical Recipes 10.2 golden section: kept probe reused, the new one placed from the kept probe toward the far end, ties keep the left part.
template<typename G>
RealSearchResult goldenReference(double a, double b, G f, int itr, double atol, double rtol) {
    constexpr double R = 0.6180339887498948482, C = 1 - R;
    double x1 = std::lerp(a, b, C), x2 = std::lerp(a, b, R);
    auto ordered = [&] { return a < x1 && x1 < x2 && x2 < b; };
    int iter = 0;
    if (itr > 0 && !settled(a, b, atol, rtol) && ordered()) {
        auto f1 = f(x1), f2 = f(x2);
        for (;;) {
            bool right = f2 < f1;
            if (right) { a = x1; x1 = x2; f1 = f2; x2 = std::lerp(x1, b, C); }
            else { b = x2; x2 = x1; f2 = f1; x1 = std::lerp(x2, a, C); }
            if (++iter == itr || settled(a, b, atol, rtol) || !ordered()) { break; }
            if (right) { f2 = f(x2); }
            else { f1 = f(x1); }}}
    return {a, b, std::midpoint(a, b), iter, settled(a, b, atol, rtol)};}

template<typename G>
void goldenTrace(double l, double r, G g, int limit, double atol, double rtol) {
    // GCC contracts each inlined std::lerp on its own, so header and reference may round a split differently; the other builds pin the traces.
    if (FMA) { return; }
    context = "golden trace l=" + real(l) + " r=" + real(r) + " limit=" + std::to_string(limit) + " atol=" + real(atol) + " rtol=" + real(rtol) + shape;
    vector<double> got, want;
    auto res = goldenSearchRealBracket(l, r, [&](double x) { got.push_back(x); return g(x); }, limit, atol, rtol);
    auto ref = goldenReference(l, r, [&](double x) { want.push_back(x); return g(x); }, limit, atol, rtol);
    size_t i = 0;
    while (i < got.size() && i < want.size() && got[i] == want[i]) { ++i; }
    require(got == want, "golden probe trace equals the reference at probe " + std::to_string(i),
        i < want.size() ? real(want[i]) : "end", i < got.size() ? real(got[i]) : "end");
    require(res.l == ref.l && res.r == ref.r && res.x == ref.x && res.iterations == ref.iterations && res.converged == ref.converged,
        "golden result equals the reference", "[" + real(ref.l) + "," + real(ref.r) + "] it=" + std::to_string(ref.iterations),
        "[" + real(res.l) + "," + real(res.r) + "] it=" + std::to_string(res.iterations));}

template<typename G, typename H>
void traces(double l, double r, G g, H dg) {
    for (int limit : {0, 1, 2, 5, 100}) {
        for (auto [atol, rtol] : {pair{0.0, 0.0}, pair{1e-12, 0.0}, pair{0.0, 1e-9}}) {
            brentTrace(l, r, g, limit, atol, rtol); goldenTrace(l, r, g, limit, atol, rtol); dbrentTrace(l, r, g, dg, limit, atol, rtol);}}
    // The same objectives returning double: values are then subtracted in double, as the contract allows.
    if (max(abs(l), abs(r)) > 1e300) { return; }
    auto gd = [&](double x) { return double(g(x)); };
    auto dgd = [&](double x) { return double(dg(x)); };
    for (int limit : {1, 5, 100}) { brentTrace(l, r, gd, limit, 0, 0); goldenTrace(l, r, gd, limit, 0, 0); dbrentTrace(l, r, gd, dgd, limit, 0, 0); }}

void referenceCorpus() {
    double huge = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min();
    auto at = [](double x, double y) { return static_cast<long double>(x) - y; };
    auto sgn = [](long double y) -> long double { return (y > 0) - (y < 0); };
    shape = " quadratic"; traces(-3, 5, [&](double x) { return at(x, 1.234) * at(x, 1.234); }, [&](double x) { return 2 * at(x, 1.234); });
    shape = " wide quadratic"; traces(-1, 1e6, [&](double x) { return 4 * at(x, 777.25) * at(x, 777.25); }, [&](double x) { return 8 * at(x, 777.25); });
    shape = " quartic"; traces(-3, 5, [&](double x) { return std::pow(at(x, 0.7), 4); }, [&](double x) { return 4 * std::pow(at(x, 0.7), 3); });
    shape = " cos"; traces(2, 4, [](double x) { return std::cos(static_cast<long double>(x)); }, [](double x) { return -std::sin(static_cast<long double>(x)); });
    shape = " exp(-2x)+x"; traces(-2, 3, [](double x) { return std::exp(-2 * static_cast<long double>(x)) + x; }, [](double x) { return 1 - 2 * std::exp(-2 * static_cast<long double>(x)); });
    shape = " asymmetric exp(x-1)-x"; traces(-10, 4, [](double x) { return std::exp(static_cast<long double>(x) - 1) - x; }, [](double x) { return std::exp(static_cast<long double>(x) - 1) - 1; });
    vector<pair<double, double>> intervals{{-huge, huge}, {-huge, 0}, {-2, 3}, {0, 1e6}, {-tiny, tiny}, {0, 100 * tiny}, {tiny, 3 * tiny}, {1, std::nextafter(1.0, 2.0)}};
    for (auto [l, r] : intervals) {
        double c = std::midpoint(l, r), p = std::lerp(l, r, 0.3), q = std::lerp(l, r, 0.55), s = std::lerp(l, r, 0.81);
        shape = " vee"; traces(l, r, [&](double x) { return abs(at(x, c)); }, [&](double x) { return sgn(at(x, c)); });
        shape = " offset vee"; traces(l, r, [&](double x) { return abs(at(x, s)); }, [&](double x) { return sgn(at(x, s)); });
        shape = " asymmetric vee"; traces(l, r, [&](double x) { return x < p ? 1000 * at(p, x) : at(x, p); }, [&](double x) -> long double { return x < p ? -1000 : 1; });
        shape = " plateau"; traces(l, r, [&](double x) { return x < p ? at(p, x) : x > q ? 3 * at(x, q) : 0; }, [&](double x) -> long double { return x < p ? -1 : x > q ? 3 : 0; });
        shape = " constant"; traces(l, r, [](double) { return 7.0L; }, [](double) { return 0.0L; });
        shape = " increasing"; traces(l, r, [](double x) { return static_cast<long double>(x); }, [](double) { return 1.0L; });
        shape = " decreasing"; traces(l, r, [](double x) { return -static_cast<long double>(x); }, [](double) { return -1.0L; });}
    shape.clear();
    std::cout << "PASS brent, golden and dbrent probe traces equal the Numerical Recipes references\n";}

double positive(std::mt19937_64 &rng) { return std::bit_cast<double>(1 + rng() % 0x7fe0000000000000ULL); }

// Objectives for few-ulp, subnormal and quantized brackets, where probe ties and rounding reach the rare branches.
long double rareShape(int kind, double l, double r, double c, double k, double x) {
    long double y = static_cast<long double>(x) - c, w = static_cast<long double>(r) - l + 1e-300L;
    switch (kind) {
        case 0: return abs(y);
        case 1: return y * y;
        case 2: return std::floor(k * y * y / w / w);
        case 3: return std::floor(k * abs(y) / w);
        case 4: return 7;
        case 5: return x;
        default: return std::round(y * 4) / 4;}}
// df stand-ins for rareShape: its smooth part's derivative (dkind 0), sign(x - c) times a hashed factor in [1, 8] (1), a hashed value in [-8, 8] (2).
long double rareSlope(int kind, int dkind, double l, double r, double c, double k, double x) {
    long double y = static_cast<long double>(x) - c, w = static_cast<long double>(r) - l + 1e-300L, s = (y > 0) - (y < 0);
    ulng h = std::bit_cast<ulng>(x) * 0x9e3779b97f4a7c15ULL; h ^= h >> 29;
    if (dkind) { return dkind == 1 ? s * int(1 + h % 8) : int(h % 17) - 8; }
    switch (kind) {
        case 0: return s;
        case 1: return 2 * y;
        case 2: return 2 * k * y / w / w;
        case 3: return k * s / w;
        case 4: return 0;
        default: return 1;}}
void rareTrace(int kind, double l, double r, double c, double k, int limit, double atol, double rtol, int dkind = 0) {
    shape = " rare kind=" + std::to_string(kind) + " dkind=" + std::to_string(dkind) + " c=" + real(c) + " k=" + real(k);
    auto g = [&](double x) { return rareShape(kind, l, r, c, k, x); };
    auto dg = [&](double x) { return rareSlope(kind, dkind, l, r, c, k, x); };
    brentTrace(l, r, g, limit, atol, rtol); goldenTrace(l, r, g, limit, atol, rtol); dbrentTrace(l, r, g, dg, limit, atol, rtol);}
void rareTraces(int count) {
    // Saved inputs where a guard or tie decides the next probe: golden order at stagnation, Brent's parabola acceptance, its endpoint nudges, a step rounded outside the bracket, an equal-value update, |e| == tol1.
    for (auto [kind, l, r, c, k, limit, atol, rtol] : vector<tuple<int, double, double, double, double, int, double, double>>{
        {6, 0x1.2de350c62863p-1020, 0x1.2de350c628639p-1020, 0x1.2de350c628632p-1020, 0x1p+5, 200, 0, 0},
        {4, -0x1.978db7c847d06p-1020, -0x1.978db7c847cfcp-1020, -0x1.978db7c847d04p-1020, 0x1p+1, 200, 0, 0},
        {5, -0x1.24815b4c57d12p-1021, -0x1.24815b4c57d0bp-1021, -0x1.24815b4c57d0ep-1021, 0x1p+7, 200, 0, 0},
        {1, -0x1.854ab0f87d85ep-1020, -0x1.854ab0f87d853p-1020, -0x1.854ab0f87d85bp-1020, 0x1p+3, 200, 0, 0},
        {1, -0x1.1446c8626a622p-1021, -0x1.1446c8626a618p-1021, -0x1.1446c8626a61fp-1021, 0x1p+2, 200, 0, 0},
        {0, -0x1.2a3df6d7fb3p-1020, -0x1.2a3df6d7fb2f6p-1020, -0x1.2a3df6d7fb2fbp-1020, 0x1p+6, 200, 0, 0},
        {1, 0x1.5ea48d3874064p-970, 0x1.5ea48d387406ap-970, 0x1.5ea48d3874069p-970, 0x1p+8, 200, 0, 0},
        {1, 0x0.0000000000007p-1022, 0x0.0000000000018p-1022, 0x0.0000000000008p-1022, 0x1p+11, 3, 0, 0},
        {1, -0x0.0000000000021p-1022, -0x0.000000000000bp-1022, -0x0.000000000000cp-1022, 0x1p+7, 200, 0, 0x1.12e0be826d695p-30},
        {1, -0x0.0000000000021p-1022, 0x0.0000000000008p-1022, -0x0.000000000002p-1022, 0x1p+7, 200, 0, 0x1.12e0be826d695p-30},
        {1, -0x0.0000000000006p-1022, 0x0.000000000001fp-1022, -0x0.0000000000004p-1022, 0x1p+2, 200, 0, 0},
        {1, -0x0.0000000000017p-1022, 0x0.0000000000007p-1022, -0x0.0000000000002p-1022, 0x1p+11, 200, 0, 0},
        {1, -0x0.0000000000009p-1022, 0x0.000000000000fp-1022, -0x0.0000000000001p-1022, 0x1p+2, 6, 0, 0},
        {1, -0x0.0000000000015p-1022, 0x0.000000000001p-1022, 0, 0x1p+8, 200, 0, 0},
        {1, -0x0.0000000000006p-1022, 0x0.0000000000015p-1022, 0x0.0000000000002p-1022, 0x1p+5, 200, 0, 0},
        {1, -0x0.000000000001ep-1022, 0x0.0000000000004p-1022, 0x0.0000000000002p-1022, 0x1p+2, 200, 0, 0x1.12e0be826d695p-30},
        {1, 0x1.561660634ee04p-108, 0x1.561660634ee08p-108, 0x1.561660634ee04p-108, 0x1p+0, 200, 0, 0},
        {3, 0x1.b9f58e4d39653p-746, 0x1.b9f58e4d39655p-746, 0x1.b9f58e4d39655p-746, 0x1p+8, 200, 0, 0},
        {4, -0x1.48b62592b2ba9p-939, -0x1.48b62592b2ba7p-939, -0x1.48b62592b2ba8p-939, 0x1p+11, 2, 0x1.0c6f7a0b5ed8dp-1010, 0},
        {4, 0x1.ffffffffffffdp-67, 0x1p-66, 0x1.ffffffffffffep-67, 0x1p+5, 200, 0x1.92a737110e454p-138, 0},
        {4, -0x1.c9662d07b5505p+92, -0x1.c9662d07b55p+92, -0x1.c9662d07b5503p+92, 0x1p+9, 200, 0x1.4f8b588e368fp+22, 0},
        {5, 0x1.ffffffffffffbp+84, 0x1.ffffffffffffep+84, 0x1.ffffffffffffbp+84, 0x1p+5, 200, 0, 0},
        {3, -0x1.cp+3, -0x1.4p+3, -0x1.9e5fb580e1d18p+3, 0x1p+6, 200, 0, 0x1.12e0be826d695p-30},
        {3, -0x1.2681608a2ad89p+3, -0x1.24233f2eaf0adp+3, -0x1.2510205f5552ep+3, 0x1p+8, 200, 0, 0},
        {0, -0x0.000000000000ap-1022, 0x0.000000000001p-1022, 0x0.0000000000001p-1022, 0x1p+4, 200, 0, 0x1.12e0be826d695p-30},
        {0, -0x1.ca5a25ddbb1c6p+2, -0x1.606c83b01da0fp+2, -0x1.7d55b30886364p+2, 0x1p+1, 200, 0x1.a7b688b675edcp-24, 0}}) {
        rareTrace(kind, l, r, c, k, limit, atol, rtol);}
    // Saved dbrent inputs where a probe equal to fv but above fw decides whether v moves (strict fu < fv).
    for (auto [dkind, kind, l, r, c, k, limit] : vector<tuple<int, int, double, double, double, double, int>>{
        {2, 2, -0x1.3p+4, -0x1.4p+3, -0x1.c9f3b645a1cacp+3, 0x1p+3, 6},
        {1, 0, 0x1.4p+3, 0x1.3p+4, 0x1.fc10624dd2f1bp+3, 0x1p+0, 200}}) {
        rareTrace(kind, l, r, c, k, limit, 0, 0, dkind);}
    // A parabola vertex exactly at the right end, on a subnormal integer grid (exact long double arithmetic).
    vector<long double> grid{2726, 2031, 2028, 2026, 2023, 2020, 2017, 1633, 1631, 1629, 1626, 1218, 1137, 1135, 1134, 470, 11, 8, 6, 3, 3, 3, 83, 254, 1040, 1041, 1236};
    double dm = std::numeric_limits<double>::denorm_min();
    auto lookup = [&](double x) { return grid[size_t(std::lround(x / dm))]; };
    auto step = [&](double x) { size_t i = size_t(std::lround(x / dm)); return grid[min(i + 1, grid.size() - 1)] - grid[i]; };
    shape = " saved subnormal grid"; brentTrace(0, 26 * dm, lookup, 9, dm, 0); goldenTrace(0, 26 * dm, lookup, 9, dm, 0); dbrentTrace(0, 26 * dm, lookup, step, 9, dm, 0);
    // A dbrent V on [61, 73] subnormal units where a probe above fx, fw and fv must still replace v because v == x.
    vector<long double> slope{-1, -1, -1, -1, -8, -5, 0, 3, 4, 1, 1, 1, 1};
    auto vee = [&](double x) { lng i = std::lround(x / dm); return i < 67 ? 2.0L * (67 - i) : 1.0L * (i - 67); };
    auto veeSlope = [&](double x) { return slope[size_t(std::lround(x / dm) - 61)]; };
    shape = " saved dbrent v == x"; dbrentTrace(61 * dm, 73 * dm, vee, veeSlope, 7, 0, 0);
    // Saved dbrent tables on [0, n] subnormal units, abs_tol 8 units, where a secant step ends exactly 2 * tol1 from l, then from r (strict < moves it).
    for (auto [left, right, limit] : vector<tuple<vector<int>, vector<int>, int>>{
        {{12, 11, 11, 11, 11, 7, 7, 7, 7, 3, 3, 2, 0, 3, 4, 6, 6, 10, 12, 12, 12, 12, 30, 30, 30, 39, 39, 39, 48, 48, 54, 54, 60, 63, 63, 63, 72, 75, 75, 75, 75, 75, 90, 90},
         {-4, -3, -7, -6, -7, -4, -8, -3, -3, -4, -8, -3, 0, 2, 5, 4, 4, 2, 6, 8, 5, 6, 3, 8, 3, 1, 8, 5, 4, 3, 1, 5, 6, 2, 5, 6, 4, 6, 8, 6, 7, 1, 8, 5}, 10},
        {{88, 86, 84, 41, 41, 41, 38, 38, 36, 36, 36, 36, 36, 31, 31, 29, 28, 27, 27, 25, 25, 25, 22, 22, 22, 22, 18, 17, 17, 17, 17, 17, 17, 17, 17, 17, 8, 8, 8, 5, 5, 5, 5, 3, 0, 3, 6, 6, 8, 8, 8, 21},
         {-7, -4, -8, -3, -3, -7, -4, -5, -5, -5, -8, -6, -4, -4, -8, -8, -4, -6, -2, -1, -6, -1, -2, -5, -5, -1, -8, -2, -4, 0, -6, -2, -2, -2, -4, -3, -1, -2, 2, -8, -3, -3, 2, -1, 0, -4, -2, 1, 6, -2, 6, 4}, 12}}) {
        auto table = [&](const vector<int> &t) { return [&t, dm](double x) { return static_cast<long double>(t[size_t(std::lround(x / dm))]); }; };
        shape = " saved dbrent 2 * tol1 tie"; dbrentTrace(0, double(left.size() - 1) * dm, table(left), table(right), limit, 8 * dm, 0);}
    std::mt19937_64 rng(seed ^ 0x9e3779b97f4a7c15ULL);
    for (int trial = 0; trial < count; ++trial) {
        int form = int(rng() % 4); double l, r;
        if (form == 0) { l = std::ldexp(1.0, int(rng() % 200) - 100) * (rng() & 1 ? -1 : 1); }
        else if (form == 1) { l = (rng() & 1 ? -1 : 1) * positive(rng); }
        else if (form == 2) { l = (rng() & 1 ? -1 : 1) * std::bit_cast<double>(rng() % 64); }
        else { l = double(lng(rng() % 41) - 20); }
        if (form < 2) { for (int s = int(rng() % 6); s--;) { l = std::nextafter(l, -HUGE_VAL); } }
        r = l;
        if (form < 3) { for (int s = 2 + int(rng() % (form == 2 ? 40 : 12)); s--;) { r = std::nextafter(r, HUGE_VAL); } }
        else { r = l + double(rng() % 20 + 1); }
        double c = std::lerp(l, r, double(rng() % 1001) / 1000), k = std::ldexp(1.0, int(rng() % 12));
        int limit = rng() % 3 ? 200 : int(rng() % 8);
        double atol = rng() % 3 ? 0 : (r - l) * 1e-6, rtol = rng() % 4 ? 0 : 1e-9;
        rareTrace(int(rng() % 7), l, r, c, k, limit, atol, rtol, trial % 3);}
    shape.clear();
    std::cout << "PASS rare-branch traces equal the references cases=" << count << '\n';}
void randomCases(int count) {
    std::mt19937_64 rng(seed);
    for (int trial = 0; trial < count; ++trial) {
        lng l = std::bit_cast<lng>(rng()), r = std::bit_cast<lng>(rng()); if (l > r) { swap(l, r); }
        lng p = lng(lll(l) + lll(rng()) % (lll(r) - l + 1)); boundaries(l, r, p);
        lng q = lng(lll(p) + lll(rng()) % (lll(r) - p + 1)); rangeContext(l, r, p);
        auto f = [&](lng x) -> lll { return x < p ? 3 * (lll(p) - x) : x > q ? 5 * (lll(x) - q) : 0; };
        expect(ternSearch(l, r, f), p, "random full-width discrete plateau");
        expect(fibSearch(l, r, f), p, "random full-width fibSearch plateau");
        int n = 1 + int(rng() % 200), a = int(rng() % uint(n)), b = a + int(rng() % uint(n - a));
        vector<int> values(n);
        for (int i = a; i > 0; --i) { values[i - 1] = values[i] + 1 + int(rng() % 10); }
        for (int i = b + 1; i < n; ++i) { values[i] = values[i - 1] + 1 + int(rng() % 10); }
        checkArray(values, -n / 2);
        double dl = double(int(rng() % 2001) - 1000), dr = dl + double(1 + rng() % 2000);
        double c = std::lerp(dl, dr, double(rng() % 1001) / 1000), d = std::lerp(c, dr, double(rng() % 1001) / 1000);
        realBinary(dl, dr, c, 80, 1e-10, 1e-13); realMinimum(dl, dr, c, d, 160, 1e-10, 1e-13);
        double ka = double(1 + rng() % 1000), kb = double(1 + rng() % 1000);
        auto kink = [&](double x) { return x < c ? ka * (static_cast<long double>(c) - x) : x > d ? kb * (static_cast<long double>(x) - d) : 0; };
        auto dkink = [&](double x) -> long double { return x < c ? -ka : x > d ? kb : 0; };
        realBrent(dl, dr, kink, c, d, 160, 1e-10, 1e-13);
        shape = " random kink"; realDbrent(dl, dr, kink, dkink, c, d, 160, 1e-10, 1e-13); shape.clear();
        double cc = std::lerp(dl, dr, double(rng() % 1001) / 1000), dd = std::lerp(dl, dr, double(rng() % 1001) / 1000), k = double(rng() % 101);
        double star = std::clamp(dd, cc - k / 2, cc + k / 2);
        auto convex = [&](double x) { long double y = static_cast<long double>(x) - cc; return y * y + k * abs(static_cast<long double>(x) - dd); };
        shape = " convex c=" + real(cc) + " d=" + real(dd) + " k=" + real(k);
        auto dconvex = [&](double x) { long double y = static_cast<long double>(x); return 2 * (y - cc) + (y > dd ? k : y < dd ? -k : 0); };
        realBrent(dl, dr, convex, star, star, 200, 1e-10, 0, 1e-5); realDbrent(dl, dr, convex, dconvex, star, star, 200, 1e-10, 0, 1e-5); shape.clear();
        double off = double(-n / 2);
        auto grid = [&](double x) {
            long double y = static_cast<long double>(x) - off; int i = min(int(y), n - 2);
            return values[i] + (values[i + 1] - values[i]) * (y - i);};
        auto dgrid = [&](double x) { int i = min(int(static_cast<long double>(x) - off), n - 2); return static_cast<long double>(values[i + 1] - values[i]); };
        realBrent(off, off + n - 1, grid, off + a, off + b, 200, 1e-10);
        shape = " random grid"; realDbrent(off, off + n - 1, grid, dgrid, off + a, off + b, 200, 1e-10); brentTrace(off, off + n - 1, grid, 200, 1e-10, 0);
        double sa = double(1 + rng() % 100), sb = double(rng() % 100);
        shape = " random smooth c=" + real(cc) + " a=" + real(sa) + " b=" + real(sb);
        auto poly = [&](double x) { long double y = static_cast<long double>(x) - cc; return sa * y * y + sb * y * y * y * y; };
        auto dpoly = [&](double x) { long double y = static_cast<long double>(x) - cc; return 2 * sa * y + 4 * sb * y * y * y; };
        int limit = int(1 + rng() % 100); double atol = trial % 2 ? 1e-12 : 0, rtol = trial % 3 ? 0 : 1e-9;
        brentTrace(dl, dr, poly, limit, atol, rtol); dbrentTrace(dl, dr, poly, dpoly, limit, atol, rtol);
        shape = " random vee c=" + real(c) + " d=" + real(d);
        limit = int(1 + rng() % 100); atol = trial % 2 ? 1e-10 : 0;
        brentTrace(dl, dr, kink, limit, atol, 0); dbrentTrace(dl, dr, kink, dkink, limit, atol, 0);
        shape.clear();
        array<double, 3> g{positive(rng), positive(rng), positive(rng)}; sort(g.begin(), g.end());
        if (trial % 4 == 0) {
            int steps = 1 + int(rng() % 4), j = int(rng() % uint(steps + 1));
            g[1] = g[2] = g[0];
            for (int i = 0; i < steps; ++i) { g[2] = std::nextafter(g[2], HUGE_VAL); if (i < j) { g[1] = g[2]; }}}
        rtol = trial % 3 ? 0 : 1e-9;
        if (rng() % 2) { realGeometric(g[0], g[2], g[1], 100, 0, rtol); }
        else { realGeometric(-g[2], -g[0], -g[1], 100, 0, rtol); }
        array<double, 3> h{(rng() & 1 ? -1 : 1) * positive(rng), (rng() & 1 ? -1 : 1) * positive(rng), (rng() & 1 ? -1 : 1) * positive(rng)};
        sort(h.begin(), h.end());
        if (trial % 4 == 1) { h[1] = h[2] = h[0]; for (int i = 1 + int(rng() % 6); i--;) { h[2] = std::nextafter(h[2], HUGE_VAL); if (rng() % 2) { h[1] = h[2]; } } }
        realBits(h[0], h[2], h[1], 100, 0, rtol);}
    std::cout << "PASS seeded full-width, brute-force and analytic random cases=" << count << '\n';}

void invalid(const string &probe) {
    auto f = [](auto x) { return x < 0; };
    auto g = [](double x) { return x * x; };
    auto dg = [](double x) { return 2 * x; };
    double inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
    if (probe == "first-order") { firstTrue(1, 0, f); }
    else if (probe == "last-order") { lastTrue(1, 0, f); }
    else if (probe == "integer-order") { ternSearch(1, 0, f); }
    else if (probe == "fib-order") { fibSearch(1, 0, f); }
    else if (probe == "binary-iterations") { binSearchRealBracket(0, 1, f, -1); }
    else if (probe == "ternary-iterations") { ternSearchRealBracket(0, 1, f, -1); }
    else if (probe == "golden-iterations") { goldenSearchRealBracket(0, 1, f, -1); }
    else if (probe == "binary-infinite") { binSearchRealBracket(0, inf, f); }
    else if (probe == "ternary-nan") { ternSearchRealBracket(nan, 1, f); }
    else if (probe == "golden-infinite") { goldenSearchRealBracket(-inf, 1, f); }
    else if (probe == "ternary-order") { ternSearchRealBracket(1, 0, f); }
    else if (probe == "golden-order") { goldenSearchRealBracket(1, 0, f); }
    else if (probe == "negative-absolute") { binSearchRealBracket(0, 1, f, 10, -1); }
    else if (probe == "negative-relative") { ternSearchRealBracket(0, 1, f, 10, 0, -1); }
    else if (probe == "infinite-tolerance") { goldenSearchRealBracket(0, 1, f, 10, inf); }
    else if (probe == "nan-tolerance") { goldenSearchRealBracket(0, 1, f, 10, 0, nan); }
    else if (probe == "geometric-zero") { binSearchRealGeometric(0, 1, f); }
    else if (probe == "geometric-sign") { binSearchRealGeometric(-1, 1, f); }
    else if (probe == "geometric-sign-reversed") { binSearchRealGeometric(1, -1, f); }
    else if (probe == "geometric-infinite") { binSearchRealGeometric(1, inf, f); }
    else if (probe == "bits-infinite") { binSearchRealBits(0, inf, f); }
    else if (probe == "bits-nan-tolerance") { binSearchRealBits(0, 1, f, 10, nan); }
    else if (probe == "brent-order") { brentSearchRealBracket(1, 0, g); }
    else if (probe == "brent-iterations") { brentSearchRealBracket(0, 1, g, -1); }
    else if (probe == "brent-infinite") { brentSearchRealBracket(0, inf, g); }
    else if (probe == "brent-nan-tolerance") { brentSearchRealBracket(0, 1, g, 10, 0, nan); }
    else if (probe == "dbrent-order") { brentSearchRealBracketDerivative(1, 0, g, dg); }
    else if (probe == "dbrent-iterations") { brentSearchRealBracketDerivative(0, 1, g, dg, -1); }
    else if (probe == "dbrent-infinite") { brentSearchRealBracketDerivative(-inf, 1, g, dg); }
    else if (probe == "dbrent-nan-tolerance") { brentSearchRealBracketDerivative(0, 1, g, dg, 10, nan); }
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
        integerBoundaries(); exhaustiveMinima(mode == "quick" ? 6 : mode == "stress" ? 9 : 8); realBoundaries(); toleranceCases(); tieRule(); geometricBoundaries(); bitsBoundaries(); brentBoundaries(); dbrentBoundaries(); referenceCorpus(); rareTraces(mode == "quick" ? 2000 : mode == "stress" ? 200000 : 20000);
        randomCases(mode == "quick" ? 100 : mode == "stress" ? 30000 : 3000);
        std::cout << "PASS 02-search_algorithms mode=" << mode << " seed=" << seed << " checks=" << checks << '\n';}
     catch (const std::exception &e) {
        std::cerr << "FAIL 02-search_algorithms mode=" << mode << " seed=" << seed << " " << e.what() << '\n'; return 1;}}
