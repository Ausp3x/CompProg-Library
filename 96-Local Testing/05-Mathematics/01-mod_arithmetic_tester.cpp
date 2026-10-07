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
        string s = argv[2]; lng x = 0, y = 0;
        if (s == "gcd-width") { exGcd(LLONG_MIN, 0, x, y); }
        if (s == "alias") { exGcd(2, 3, x, x); }
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
        if (s == "inv-even") { invMod2p64(6); }
        return 0;}
    if (argc < 2 || string(argv[1]) != "--oracle") {
        lng out = 7;
        if (!intPow(-2, 63, out) || out != LLONG_MIN || intPow(2, 63, out) || out != LLONG_MIN) { return 1; }
        lng a = 6, b = 4, g = exGcd(a, b, a, b);
        if (g != 2 || 6 * a + 4 * b != 2) { return 1; }
        cout << "PASS arithmetic boundary regressions\n"; return 0;}
    string op;
    while (cin >> op) {
        if (op == "g") {
            lng a, b; cin >> a >> b; auto r = extendedGcd(a,b);
            cout << gcd64(a,b) << ' ' << show(lcmWide(a,b)) << ' ' << show(r.g) << ' ' << show(r.x) << ' ' << show(r.y);
            if (r.g <= LLONG_MAX) { lng x, y; auto g = exGcd(a,b,x,y); cout << ' ' << g << ' ' << x << ' ' << y; }}
        if (op == "d") { lll a = readWide(), b = readWide(); cout << show(floorDiv(a,b)) << ' ' << show(ceilDiv(a,b)); }
        if (op == "n") { lll a = readWide(); lng m; cin >> m; cout << modNorm(a,m); }
        if (op == "m") { lng a,b,m; cin >> a >> b >> m; cout << modMul(a,b,m); }
        if (op == "u") { ulng a,b,m; cin >> a >> b >> m; cout << modMul64(a,b,m); }
        if (op == "p") { lng a,b,m; cin >> a >> b >> m; cout << modPow(a,b,m); }
        if (op == "P") { lng a,b; cin >> a >> b; cout << modPow(a,b); }
        if (op == "c") {
            lng a,b,x=123,y=123; cin >> a >> b; bool p = checkedAdd(a,b,x), q = checkedMul(a,b,y);
            cout << p << ' ' << x << ' ' << q << ' ' << y << ' ' << saturatingAdd(a,b) << ' ' << saturatingMul(a,b);}
        if (op == "v") { ulng a; cin >> a; cout << invMod2p64(a); }
        if (op == "q") { ulng a,b,m; cin >> a >> b >> m; cout << modPow64(a,b,m); }
        if (op == "i") { lng a,out=123; ulng b; cin >> a >> b; bool ok = intPow(a,b,out); cout << ok << ' ' << out; }
        cout << '\n';}}
