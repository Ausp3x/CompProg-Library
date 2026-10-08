#include "../../06-Miscellaneous/49-timer.hpp"

using Clock = std::chrono::steady_clock;
ulng test_seed = 0;
lng checks = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}
double ms(Clock::duration d) { return std::chrono::duration<double, std::milli>(d).count(); }
lng us(Clock::duration d) { return std::chrono::duration_cast<std::chrono::microseconds>(d).count(); }
void spin(Clock::duration d) {
    auto end = Clock::now() + d;
    while (Clock::now() < end) {}}

// Independent bracket: a query made between q0 and q1 on a timer reset between r0 and r1 sees [q0 - r1, q1 - r0].
struct Bracket { Clock::duration lo, hi; };
template<class F>
auto bracket(Clock::time_point r0, Clock::time_point r1, F &&query, Bracket &b) {
    auto q0 = Clock::now();
    auto res = query();
    auto q1 = Clock::now();
    b = {q0 - r1, q1 - r0};
    return res;}

void checkTimer(Timer &t, Clock::time_point r0, Clock::time_point r1) {
    Bracket b;
    auto raw = bracket(r0, r1, [&] { return t.elapsed(); }, b);
    check(b.lo <= raw && raw <= b.hi, "elapsed within the outer clock bracket");
    lng u = bracket(r0, r1, [&] { return t.elapsedUs(); }, b);
    check(us(b.lo) <= u && u <= us(b.hi), "elapsedUs truncated within the bracket");
    double m = bracket(r0, r1, [&] { return t.elapsedMs(); }, b);
    check(ms(b.lo) - 1e-9 <= m && m <= ms(b.hi) + 1e-9, "elapsedMs within the bracket");
    double s = bracket(r0, r1, [&] { return t.elapsedSec(); }, b);
    check(ms(b.lo) / 1000 - 1e-12 <= s && s <= ms(b.hi) / 1000 + 1e-12, "elapsedSec within the bracket");
    for (double limit : {-5.0, 0.0, 0.25, 1.0, 3.0, 20.0, 1e6}) {
        context = "limit=" + std::to_string(limit);
        bool e = bracket(r0, r1, [&] { return t.expired(limit); }, b);
        if (ms(b.lo) >= limit) { check(e, "expired once the lower bracket reaches the limit"); }
        if (ms(b.hi) < limit) { check(!e, "not expired below the upper bracket"); }
        double rem = bracket(r0, r1, [&] { return t.remaining(limit); }, b);
        check(rem >= 0, "remaining clamps at zero");
        check(rem <= max(0.0, limit - ms(b.lo)) + 1e-9 && rem >= max(0.0, limit - ms(b.hi)) - 1e-9, "remaining within the bracket");
        if (limit > 0) {
            double p = bracket(r0, r1, [&] { return t.progress(limit); }, b);
            check(0 <= p && p <= 1, "progress clamps to [0, 1]");
            check(p >= min(1.0, ms(b.lo) / limit) - 1e-12 && p <= min(1.0, ms(b.hi) / limit) + 1e-12, "progress within the bracket");}}
    context.clear();}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i];
            if (probe == "progress-zero") { Timer().progress(0); }
            if (probe == "progress-negative") { Timer().progress(-1); }
            return 0;}}
    static_assert(std::is_same_v<Timer::Clock, std::chrono::steady_clock> && Timer::Clock::is_steady);
    std::mt19937_64 gen(test_seed);
    int rounds = mode == "quick" ? 20 : mode == "full" ? 120 : 600;
    double max_wait = mode == "quick" ? 2 : mode == "full" ? 8 : 20;

    auto r0 = Clock::now();
    Timer t;
    auto r1 = Clock::now();
    checkTimer(t, r0, r1);
    for (int round = 0; round < rounds; ++round) {
        double wait = std::uniform_real_distribution<double>(0, max_wait)(gen);
        spin(std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double, std::milli>(wait)));
        checkTimer(t, r0, r1);
        if (gen() % 4 == 0) {
            r0 = Clock::now(); t.reset(); r1 = Clock::now();
            checkTimer(t, r0, r1);}
        Timer copy = t;
        check(copy.start == t.start, "copies keep the start point");}

    // Monotone readings, and a measured wait is seen in full.
    lng last = t.elapsedUs();
    for (int i = 0; i < 100000; ++i) {
        lng now = t.elapsedUs();
        check(now >= last, "elapsedUs nondecreasing");
        last = now;}
    t.reset();
    spin(std::chrono::milliseconds(3));
    check(t.elapsedMs() >= 3 && t.expired(3) && t.remaining(3) == 0 && t.progress(3) == 1, "3 ms wait observed");

    // ScopedTimer: format, bracket, untouched stream state, nesting order, default stream.
    for (int round = 0; round < rounds / 4 + 3; ++round) {
        std::ostringstream os;
        os << std::setprecision(11) << std::scientific;
        auto flags = os.flags();
        string label = round == 0 ? "" : "scope " + std::to_string(round);
        Clock::time_point a, b;
        double wait = std::uniform_real_distribution<double>(0, max_wait / 2)(gen);
        {
            a = Clock::now();
            ScopedTimer s(label, os);
            spin(std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double, std::milli>(wait)));
            check(&s.os == &os && s.label == label, "ScopedTimer keeps label and stream");}
        b = Clock::now();
        string out = os.str();
        context = "output=" + out;
        string prefix = label + ": ";
        check(out.size() > prefix.size() + 4 && out.compare(0, prefix.size(), prefix) == 0, "ScopedTimer prefix");
        check(out.compare(out.size() - 4, 4, " ms\n") == 0, "ScopedTimer suffix");
        string num = out.substr(prefix.size(), out.size() - prefix.size() - 4);
        auto dot = num.find('.');
        check(dot != string::npos && dot > 0 && num.size() - dot == 4, "three decimals");
        check(std::all_of(num.begin(), num.end(), [](char c) { return c == '.' || isdigit(c); }), "plain fixed notation");
        double v = std::stod(num);
        check(v >= wait - 0.0005 && v <= ms(b - a) + 0.0005, "reported milliseconds within the bracket");
        check(os.flags() == flags && os.precision() == 11, "stream format state untouched");
        context.clear();}
    {
        std::ostringstream os;
        {
            ScopedTimer outer("outer", os);
            ScopedTimer inner("inner", os);}
        string out = os.str();
        check(out.rfind("inner: ", 0) == 0 && out.find("\nouter: ") != string::npos, "nested scopes report inner first");
        ScopedTimer d("default");
        check(&d.os == &cerr, "default stream is cerr");
        d.label = "PASS ScopedTimer default stream";}
    cout << "PASS 49-timer mode=" << mode << " checks=" << checks << '\n';}
