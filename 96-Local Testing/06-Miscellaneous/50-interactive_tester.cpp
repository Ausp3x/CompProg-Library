#include "../../06-Miscellaneous/50-interactive.hpp"

ulng test_seed = 0;
lng checks = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}
template<class T, class U> void checkEqual(const T &actual, const U &expected, const string &what) {
    ++checks;
    if (!(actual == expected)) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=" << expected << " actual=" << actual << '\n'; std::exit(1);}}

// Output buffer that only publishes bytes on sync; the input buffer hands out one byte at a time
// and records every byte requested while output is still pending (a missing flush before a read).
struct OutBuf : std::streambuf {
    string pending, flushed;
    int overflow(int c) override {
        if (c != EOF) { pending += char(c); }
        return c;}
    std::streamsize xsputn(const char *s, std::streamsize n) override {
        pending.append(s, size_t(n));
        return n;}
    int sync() override {
        flushed += pending; pending.clear();
        return 0;}
};
struct InBuf : std::streambuf {
    string data;
    size_t pos = 0;
    OutBuf *out;
    int violations = 0;
    char ch = 0;
    InBuf(string data, OutBuf *out) : data(std::move(data)), out(out) {}
    int underflow() override {
        if (!out->pending.empty()) { ++violations; }
        if (pos == data.size()) { return EOF; }
        ch = data[pos++]; setg(&ch, &ch, &ch + 1);
        return (unsigned char)ch;}
};
struct Harness {
    OutBuf ob;
    InBuf ib;
    std::istream in;
    std::ostream out;
    Interactive io;
    Harness(string replies, lng budget = LLONG_MAX) : ib(std::move(replies), &ob), in(&ib), out(&ob), io(budget, in, out) {}
    void done(const string &expected) {
        checkEqual(ob.flushed + ob.pending, expected, "written bytes");
        checkEqual(ib.violations, 0, "no byte read while output was pending");}
};

void fixedCases() {
    {
        context = "formatting";
        Harness h("");
        h.io.answer('!', 5);
        h.io.answer("!", vector<int>{1, 2, 3});
        h.io.answer('?', vector<int>{}, 7);
        h.io.answer();
        h.io.answer(string("abc"), "def", 'g', -3LL, 2.5, string_view("sv"));
        h.io.answer(vector<vector<int>>{{1, 2}, {}, {3}}, array<int, 2>{8, 9}, set<int>{5, 4});
        h.io.answer(vector<string>{"x", "yz"}, vector<int>{});
        h.io.answer(vector<int>{}, 'q');
        check(h.ob.pending.empty(), "answer flushes");
        h.done("! 5\n! 1 2 3\n? 7\n\nabc def g -3 2.5 sv\n1 2 3 8 9 4 5\nx yz\nq\n");
        checkEqual(h.io.used, 0LL, "answer is not counted");}
    {
        context = "ask and budget";
        Harness h("17\nYes\n-4\n2.5\n9\n", 5);
        checkEqual(h.io.ask('?', 3), 17LL, "ask returns lng reply");
        checkEqual(h.io.ask<string>('?', "s", 4), string("Yes"), "ask<string> returns token");
        checkEqual(h.io.ask<int>('?'), -4, "ask<int> negative non-error reply");
        checkEqual(h.io.ask<double>("q"), 2.5, "ask<double>");
        checkEqual(h.io.used, 4LL, "used counts asks");
        checkEqual(h.io.remaining(), 1LL, "remaining");
        checkEqual(h.io.budget, 5LL, "budget");
        checkEqual(h.io.ask('?', 1), 9LL, "last query within budget");
        checkEqual(h.io.remaining(), 0LL, "budget exhausted exactly");
        h.io.reset(2);
        checkEqual(h.io.used, 0LL, "reset clears used");
        checkEqual(h.io.remaining(), 2LL, "reset sets budget");
        h.done("? 3\n? s 4\n?\nq\n? 1\n");}
    {
        context = "replies";
        Harness h(std::to_string(LLONG_MIN) + ' ' + std::to_string(LLONG_MAX) + "\n-1 -1 18446744073709551615 -1 \t\n\n   hello  world \r\nnext\nlast line\r\n z");
        checkEqual(h.io.readReply(), LLONG_MIN, "LLONG_MIN reply");
        checkEqual(h.io.readReply(), LLONG_MAX, "LLONG_MAX reply");
        checkEqual(h.io.readReply<string>(), string("-1"), "token replies are never sentinels");
        h.io.error.reset();
        checkEqual(h.io.readReply<int>(), -1, "disabled sentinel returns -1");
        checkEqual(h.io.readReply<ulng>(), ULLONG_MAX, "unsigned reply with the sentinel disabled");
        checkEqual(h.io.readReply<uint>(), UINT_MAX, "text -1 into an unsigned type with the sentinel disabled is all-ones");
        checkEqual(h.io.readLine(), string("hello  world "), "readLine skips blank lines and leading spaces, strips CR");
        checkEqual(h.io.readReply<string>(), string("next"), "token after line");
        checkEqual(h.io.readLine(), string("last line"), "readLine after token skips the line break");
        checkEqual(h.io.readReply<char>(), 'z', "char reply");
        h.done("");}
    {
        context = "flush before every read";
        Harness h("1 2 3\nline\n4");
        h.io.writeLine("pre");
        checkEqual(h.io.readReply(), 1LL, "readReply after writeLine");
        h.io.writeLine("mid", 1);
        checkEqual(h.io.readReply(), 2LL, "second read");
        h.io.flushNow();
        checkEqual(h.ob.pending, string(), "flushNow publishes");
        h.io.writeLine("x");
        checkEqual(h.io.ask('?'), 3LL, "ask after a pending line");
        h.io.writeLine("y");
        checkEqual(h.io.readLine(), string("line"), "readLine flushes");
        h.io.out << "raw";
        checkEqual(h.io.readReply(), 4LL, "raw stream writes are flushed too");
        h.done("pre\nmid 1\nx\n?\ny\nraw");}}

// Seeded random protocol against a model that rebuilds the expected transcript independently.
void randomCases(int rounds, std::mt19937_64 &gen) {
    for (int round = 0; round < rounds; ++round) {
        context = "random round=" + std::to_string(round);
        int ops = int(gen() % 30);
        string replies, expected;
        vector<pair<int, string>> plan;
        for (int k = 0; k < ops; ++k) {
            int kind = int(gen() % 4);
            lng v = lng(gen() % 2001) - 1000;
            if (v == -1) { v = 0; }
            plan.push_back({kind, std::to_string(v)});
            if (kind != 3) { replies += std::to_string(v) + (gen() % 2 ? "\n" : " \n\n"); }}
        Harness h(replies, ops);
        for (auto [kind, value] : plan) {
            lng a = lng(gen() % 100);
            vector<int> args(gen() % 4);
            for (int &x : args) { x = int(gen() % 50); }
            string line = "?";
            line += ' ' + std::to_string(a);
            for (int x : args) { line += ' ' + std::to_string(x); }
            if (kind == 0) { checkEqual(h.io.ask('?', a, args), std::stoll(value), "random ask"); expected += line + '\n'; }
            else if (kind == 1) { checkEqual(h.io.ask<string>('?', a, args), value, "random token ask"); expected += line + '\n'; }
            else if (kind == 2) { h.io.writeLine('?', a, args); expected += line + '\n'; checkEqual(h.io.readReply<int>(), int(std::stoll(value)), "random readReply"); }
            else { h.io.answer('!', a, args); expected += '!' + line.substr(1) + '\n'; }}
        h.done(expected);
        check(h.io.used <= h.io.budget, "random budget respected");}}

// Scenarios on the real stdin/stdout; the Python driver checks the exit status and the exact output.
int scenario(const string &name) {
    std::ios::sync_with_stdio(false);
    cin.tie(nullptr);
    Interactive io;
    if (name == "error-int") { io.ask('?', 1); io.ask('?', 2); }
    if (name == "error-custom") { io.error = 0; io.ask('?', 1); io.ask('?', 2); }
    if (name == "error-after-answer") { io.answer('!', 3); io.readReply(); }
    if (name == "eof-int") { io.ask('?', 1); }
    if (name == "eof-token") { io.ask<string>('?', 1); }
    if (name == "eof-line") { io.answer("hi"); io.readLine(); }
    if (name == "bad-token") { io.ask<int>('?', 1); }
    if (name == "overflow") { io.ask<int>('?', 1); }
    if (name == "play") {
        int t = io.readReply<int>();
        io.answer("echo", io.readLine());
        for (int c = 0; c < t; ++c) {
            lng n = io.readReply();
            io.reset(std::bit_width(ulng(n - 1)));
            lng lo = 1, hi = n;
            while (lo < hi) {
                lng mid = lo + (hi - lo + 1) / 2;
                if (io.ask('?', mid)) { lo = mid; }
                else { hi = mid - 1; }}
            io.answer('!', lo);
            io.readReply();}
        io.answer("done");
        return 0;}
    cout << "UNREACHABLE\n";
    return 0;}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--scenario") { return scenario(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i];
            if (probe == "budget-zero") { Harness h("1"); h.io.reset(0); h.io.ask('?'); }
            if (probe == "budget-exceeded") { Harness h("1 2", 1); h.io.ask('?'); h.io.ask('?'); }
            if (probe == "unsigned-sentinel") { Harness h("-1"); h.io.readReply<ulng>(); }
            if (probe == "unsigned-sentinel-custom") { Harness h("7"); h.io.error = 0; h.io.ask<uint>('?'); }
            return 0;}}
    std::mt19937_64 gen(test_seed);
    fixedCases();
    randomCases(mode == "quick" ? 200 : mode == "full" ? 5000 : 50000, gen);
    cout << "PASS 50-interactive mode=" << mode << " checks=" << checks << '\n';}
