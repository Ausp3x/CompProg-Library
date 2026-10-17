// Full and mini in one program: identical results on the shared domain (supporting evidence beside the Python oracle).
#include "../../01-Core/07-infint.hpp"
#include "../../01-Core/08-infintmini.hpp"

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 42;
    int rounds = argc > 2 ? std::stoi(argv[2]) : 300;
    std::mt19937_64 rng(seed);
    lng checks = 0;
    auto same = [&](const InfInt &f, const InfIntMini &m, const string &what) {
        ++checks;
        if (f.toString() != m.toString()) { cerr << "FAIL seed=" << seed << ' ' << what << ": full=" << f.toString() << " mini=" << m.toString() << '\n'; std::exit(1); }};
    auto text = [&](int digits) {
        if (rng() % 10 == 0) { return string(rng() % 2 ? "inf" : "-inf"); }
        string s = rng() % 2 ? "-" : "";
        s += char('1' + rng() % 9);
        for (int i = 1; i < digits; i++) { s += char('0' + rng() % 10); }
        return s;};
    for (int t = 0; t < rounds; t++) {
        string x = text(1 + int(rng() % (t % 10 == 0 ? 3000 : 60))), y = text(1 + int(rng() % (t % 7 == 0 ? 3000 : 60)));
        InfInt a(x), b(y); InfIntMini p(x), q(y);
        same(a + b, p + q, "add " + x + " " + y); same(a - b, p - q, "sub " + x + " " + y); same(a * b, p * q, "mul " + x + " " + y);
        same(gcd(a, b), gcd(p, q), "gcd " + x + " " + y); same(pow(a, 3), pow(p, 3), "pow " + x);
        auto [f1, f2] = divMod(a, b); auto [m1, m2] = divMod(p, q);
        same(f1, m1, "quotient " + x + " " + y); same(f2, m2, "remainder " + x + " " + y);
        uint base = 2 + uint(rng() % 35);
        ++checks;
        if (a.toString(base) != p.toString(base) || lng(a) != lng(p) || ulng(a) != ulng(p) || bool(a) != bool(p) || (a <=> b) != (p <=> q) || (a == b) != (p == q)) {
            cerr << "FAIL seed=" << seed << " conversions or comparison " << x << ' ' << y << " base " << base << '\n';
            return 1;}
        InfInt c = 7; InfIntMini r = 7;
        string s = a.toString(base);
        ++checks;
        if (c.read(s, base) != r.read(s, base) || c.toString() != r.toString()) { cerr << "FAIL seed=" << seed << " read " << s << " base " << base << '\n'; return 1; }}
    cout << "full and mini agree: " << checks << " checks\n";
    return 0;}
