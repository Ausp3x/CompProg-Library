#include "../../06-Miscellaneous/01-random.hpp"

ulng test_seed = 0;
void check(bool ok, const string &what) {
    if (!ok) { cerr << "FAIL seed=" << test_seed << " operation=" << what
                    << " expected=true actual=false\n"; std::exit(1);}}

void oracle() {
    string op;
    while (cin >> op) {
        ulng seed; cin >> seed; Random g(seed);
        if (op == "d" || op == "dr") {
            string a = "0", b = "1"; int n;
            if (op == "dr") { cin >> a >> b; }
            cin >> n; double l = std::strtod(a.c_str(), nullptr), r = std::strtod(b.c_str(), nullptr);
            for (int i = 0; i < n; ++i) { std::printf("%a ", op == "d" ? g.randDouble() : g.randDouble(l, r)); }
            std::fflush(stdout);}
        else if (op == "h") {
            int n; cin >> n; vector<int> a(n); iota(a.begin(), a.end(), 0);
            g.shuffle(a.begin(), a.end()); for (int x : a) { cout << x << ' '; }}
        else {
            ulng l = 0, r = 0; lng sl = 0, sr = 0; int n;
            if (op == "b") { cin >> r; }
            if (op == "u") { cin >> l >> r; }
            if (op == "s" || op == "i") { cin >> sl >> sr; }
            cin >> n;
            for (int i = 0; i < n; ++i) {
                if (op == "raw") { cout << g.rng(); }
                else if (op == "b") { cout << g.randBelow(r); }
                else if (op == "u") { cout << g.randUlng(l, r); }
                else if (op == "s") { cout << g.randLng(sl, sr); }
                else { cout << g.randInt(int(sl), int(sr)); }
                cout << ' ';}}
        cout << '\n';}}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--oracle") { oracle(); return 0; }
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string p = argv[++i]; Random g(1); array<int, 2> a{};
            if (p == "below-zero") { g.randBelow(0); }
            if (p == "int-bounds") { g.randInt(2, 1); }
            if (p == "lng-bounds") { g.randLng(2, 1); }
            if (p == "ulng-bounds") { g.randUlng(2, 1); }
            if (p == "shuffle-bounds") { g.shuffle(a.end(), a.begin()); }
            if (p == "double-empty") { g.randDouble(1.0, 1.0); }
            if (p == "double-infinite") { g.randDouble(-DBL_MAX, DBL_MAX); }
            return 1;}}
    Random a(5489);
    a.rng.discard(9999);
    check(a.rng() == 9981545732273789042ULL, "MT19937-64 standard 10000th word");
    for (ulng s : {0ULL, 1ULL, 5489ULL, ~0ULL}) {
        Random x(s), y(s); for (int i = 0; i < 2000; ++i) {
            check(x.randLng(INT64_MIN, INT64_MAX) == y.randLng(INT64_MIN, INT64_MAX), "seed replay");}
        x.seed(s); y = Random(s); check(x.rng == y.rng, "seed reset");
        auto copy = x; auto moved = std::move(copy);
        check(x.randBelow(ulng(1) << 63) == moved.randBelow(ulng(1) << 63), "copy/move state");}
    cout << "PASS standard engine, seed/reset, copy/move replay\n";
    Random g(test_seed);
    const int count = mode == "quick" ? 5000 : mode == "full" ? 100000 : 1000000;
    vector<int> freq(7);
    for (int i = 0; i < count; ++i) {
        int x = g.randInt(-3, 3); check(-3 <= x && x <= 3, "int inclusive bounds"); ++freq[x + 3];
        check(g.randLng(INT64_MIN, INT64_MIN) == INT64_MIN, "signed minimum singleton");
        check(g.randLng(INT64_MAX, INT64_MAX) == INT64_MAX, "signed maximum singleton");
        check(g.randUlng(UINT64_MAX, UINT64_MAX) == UINT64_MAX, "unsigned maximum singleton");
        check(g.randBelow(1) == 0, "unit bound");
        lng y = g.randLng(INT64_MIN, 0); check(y <= 0, "wide signed interval");
        ulng z = g.randUlng(ulng(1) << 63, UINT64_MAX); check(z >> 63, "upper unsigned interval");}
    for (int f : freq) { check(count / 10 < f && f < count / 5, "fixed-seed seven-bin regression"); }
    cout << "PASS inclusive/singleton/full-width domains and bounded regression draws=" << count << '\n';
    for (int n = 0; n <= (mode == "quick" ? 30 : 200); ++n) {
        vector<int> v(n); iota(v.begin(), v.end(), 0); auto original = v;
        Random x(test_seed), y(test_seed); auto w = v;
        x.shuffle(v.begin(), v.end()); y.shuffle(w.begin(), w.end());
        check(v == w && x.rng == y.rng, "shuffle replay n=" + std::to_string(n));
        sort(v.begin(), v.end()); check(v == original, "shuffle permutation n=" + std::to_string(n));
        if (n <= 1) { check(x.rng == Random(test_seed).rng, "empty/singleton no draws"); }}
    array<int, 6> duplicate{1, 1, 2, 2, 3, 3}; g.shuffle(duplicate.begin(), duplicate.end());
    sort(duplicate.begin(), duplicate.end()); check(duplicate == array<int, 6>{1, 1, 2, 2, 3, 3}, "duplicates");
    deque<int> d{1, 2, 3, 4}; g.shuffle(d.begin(), d.end()); sort(d.begin(), d.end());
    check(d == deque<int>{1, 2, 3, 4}, "deque iterators");
    vector<std::unique_ptr<int>> ptrs; for (int i = 0; i < 10; ++i) { ptrs.push_back(std::make_unique<int>(i)); }
    g.shuffle(ptrs.begin(), ptrs.end()); int sum = 0; for (const auto &p : ptrs) { sum += *p; }
    check(sum == 45, "move-only shuffle");
    vector<bool> bits{true, false, true}; g.shuffle(bits.begin(), bits.end());
    check(std::count(bits.begin(), bits.end(), true) == 2, "proxy iterator shuffle");
    static_assert(std::uniform_random_bit_generator<Random> && Random::min() == 0 && Random::max() == UINT64_MAX);
    Random u(test_seed), v(test_seed);
    for (int i = 0; i < 1000; ++i) { check(u() == v.rng(), "URBG operator() is the engine word"); }
    vector<int> sv(50); iota(sv.begin(), sv.end(), 0); std::shuffle(sv.begin(), sv.end(), u); sort(sv.begin(), sv.end());
    check(sv[0] == 0 && sv[49] == 49 && std::uniform_int_distribution<int>(3, 3)(u) == 3, "standard algorithms accept Random");
    double total = 0; int low = 0;
    for (int i = 0; i < count; ++i) {
        double x = g.randDouble(), y = g.randDouble(-2.5, 7.5), z = g.randDouble(1.0, std::nextafter(1.0, 2.0));
        check(0 <= x && x < 1 && -2.5 <= y && y < 7.5 && z == 1.0, "randDouble half-open ranges");
        total += x; low += y < 2.5;}
    check(std::abs(total / count - 0.5) < 0.02 && std::abs(double(low) / count - 0.5) < 0.02, "randDouble fixed-seed mean regression");
    cout << "PASS URBG interface and randDouble domains draws=" << count << '\n';
    rng.seed(test_seed); check(rng.rng() == Random(test_seed).rng(), "legacy global rng");
    Random automatic; check(automatic.randBelow(1) == 0, "default clock seed constructor");
    cout << "PASS shuffle empty/duplicates/deque/proxy/move-only and legacy global\n";}
