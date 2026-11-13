#include "../../05-Mathematics/01-mod_arithmetic.hpp"

string show(lll x) {
    bool neg = x < 0; ulll u = neg ? -ulll(x) : ulll(x); string s;
    do { s.push_back(char('0' + u % 10)); u /= 10; } while (u);
    if (neg) { s.push_back('-'); } reverse(s.begin(), s.end()); return s;}
lll readWide() {
    string s; cin >> s; bool neg = s[0] == '-'; ulll u = 0;
    for (int i = neg; i < int(s.size()); ++i) { u = 10 * u + (s[i] - '0'); }
    if (!neg) { return lll(u); } return -lll(u - 1) - 1;}
int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "floor-zero") { floorDiv(0, 0); }
        if (s == "ceil-zero") { ceilDiv(0, 0); }
        if (s == "floor-overflow") { floorDiv(std::numeric_limits<lll>::min(), -1); }
        if (s == "ceil-overflow") { ceilDiv(std::numeric_limits<lll>::min(), -1); }
        if (s == "norm-zero") { modNorm(0, 0); }
        if (s == "norm-negative") { modNorm(0, -1); }
        if (s == "mul-zero") { modMul(2, 3, 0); }
        if (s == "mul64-zero") { modMul64(2, 3, 0); }
        if (s == "pow-zero") { modPow(2, -1, 0); }
        if (s == "pow64-zero") { modPow64(2, 3, 0); }
        if (s == "pow64-zero-exponent") { modPow64(2, 0, 0); }
        if (s == "inv-even") { invMod2p64(6); }
        if (s == "floor-strict-zero") { floorDivStrict(1, 0); }
        if (s == "ceil-strict-zero") { ceilDivStrict(1, 0); }
        if (s == "floor-strict-overflow") { floorDivStrict(std::numeric_limits<lll>::min(), 1); }
        if (s == "ceil-strict-overflow") { ceilDivStrict(std::numeric_limits<lll>::max(), 1); }
        if (s == "ceil-strict-overflow-negative") { ceilDivStrict(-std::numeric_limits<lll>::max(), -1); }
        if (s == "divmod-zero") { divModFloor(1, 0); }
        if (s == "divmod-overflow") { divModFloor(std::numeric_limits<lll>::min(), -1); }
        if (s == "power-mod-zero") { FixedBasePower(2, 0, 5).get(0); }
        if (s == "power-mod-negative") { FixedBasePower(2, -7, 5).get(0); }
        if (s == "power-n-negative") { FixedBasePower(2, 7, -1).get(0); }
        if (s == "power-get-negative") { FixedBasePower(2, 7, 5).get(-1); }
        if (s == "power-get-past") { FixedBasePower f(2, 7, 10); f.get(f.n + 1); }
        if (s == "power-levels-one") { FixedBasePower(2, 7, 5, 1).get(0); }
        return 0;}
    if (argc < 2 || string(argv[1]) != "--oracle") {
        lng out = 7;
        if (!intPow(-2, 63, out) || out != LLONG_MIN || intPow(2, 63, out) || out != LLONG_MIN) { return 1; }
        auto [g, x, y] = extendedGcd(LLONG_MIN, 0);
        if (g != lll(1) << 63 || lll(LLONG_MIN) * x + 0 * y != g) { return 1; }
        auto z = extendedGcd(0, 0);
        if (z.g != 0 || z.x != 1 || z.y != 0) { return 1; }
        cout << "PASS arithmetic boundary regressions\n"; return 0;}
    string op;
    while (cin >> op) {
        if (op == "g") {
            lng a, b; cin >> a >> b; auto r = extendedGcd(a,b);
            cout << gcd64(a,b) << ' ' << show(lcmWide(a,b)) << ' ' << show(r.g) << ' ' << show(r.x) << ' ' << show(r.y);}
        if (op == "d") { lll a = readWide(), b = readWide(); cout << show(floorDiv(a,b)) << ' ' << show(ceilDiv(a,b)); }
        if (op == "s") { lll a = readWide(), b = readWide(); cout << show(floorDivStrict(a,b)); }
        if (op == "t") { lll a = readWide(), b = readWide(); cout << show(ceilDivStrict(a,b)); }
        if (op == "r") { lll a = readWide(), b = readWide(); auto [q, r] = divModFloor(a,b); cout << show(q) << ' ' << show(r); }
        if (op == "f") {
            lng a, m, n; int levels, k; cin >> a >> m >> n >> levels >> k;
            FixedBasePower f = levels == 2 && k % 2 ? FixedBasePower(a, m, n) : FixedBasePower(a, m, n, levels);
            cout << f.base << ' ' << f.pw.size();
            for (lng e; k--;) { cin >> e; cout << ' ' << f.get(e); }}
        if (op == "n") { lll a = readWide(); lng m; cin >> m; cout << modNorm(a,m); }
        if (op == "m") { lng a,b,m; cin >> a >> b >> m; cout << modMul(a,b,m); }
        if (op == "u") { ulng a,b,m; cin >> a >> b >> m; cout << modMul64(a,b,m); }
        if (op == "p") { lng a,b,m; cin >> a >> b >> m; cout << modPow(a,b,m); }
        if (op == "c") {
            lng a,b,x=123,y=123; cin >> a >> b; bool p = checkedAdd(a,b,x), q = checkedMul(a,b,y);
            cout << p << ' ' << x << ' ' << q << ' ' << y << ' ' << saturatingAdd(a,b) << ' ' << saturatingMul(a,b);}
        if (op == "v") { ulng a; cin >> a; cout << invMod2p64(a); }
        if (op == "q") { ulng a,b,m; cin >> a >> b >> m; cout << modPow64(a,b,m); }
        if (op == "i") { lng a,out=123; ulng b; cin >> a >> b; bool ok = intPow(a,b,out); cout << ok << ' ' << out; }
        cout << '\n';}}
