#include "../../06-Miscellaneous/02-customhash.hpp"

ulng test_seed = 0;
void check(bool ok, const string &what) {
    if (!ok) { cerr << "FAIL seed=" << test_seed << " operation=" << what
                    << " expected=true actual=false\n"; std::exit(1);}}
enum class Kind : ulng { ZERO, LARGE = UINT64_MAX };
struct Convertible { ulng x; operator ulng() const { return x; } };
void functionPointer() {}

void oracle() {
    string op;
    while (cin >> op) {
        ulng seed; cin >> seed; CustomHash h(seed); ulng x, y;
        if (op == "s") {
            string hex; cin >> hex; string s;
            if (hex != "-") { for (int i = 0; i < int(hex.size()); i += 2) {
                s += char(std::stoi(hex.substr(i, 2), nullptr, 16));}}
            cout << h(s);}
        else if (op == "v" || op == "t") {
            int n; cin >> n; vector<ulng> a(n); for (auto &v : a) { cin >> v; }
            if (op == "v") { cout << h(a); }
            else { cout << h(tuple{a[0], a[1], a[2]}); }}
        else {
            cin >> x;
            if (op == "p" || op == "w" || op == "z") { cin >> y; }
            if (op == "m") { cout << CustomHash::splitMix64(x); }
            else if (op == "c") { cout << CustomHash::combine(x, seed); }
            else if (op == "i") { cout << h(x); }
            else if (op == "w") { cout << h((ulll(x) << 64) | y); }
            else if (op == "z") { cout << h(lll((ulll(x) << 64) | y)); }
            else { cout << h(pair{x, y}); }}
        cout << '\n';}}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--oracle") { oracle(); return 0; }
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }}
    CustomHash h(test_seed), copy = h, def = {};
    check(def.seed == CustomHash::rnd && CustomHash{}.seed == def.seed, "stable process seed");
    check(h(123) == copy(123), "copy seed"); auto moved = std::move(copy);
    check(moved(123) == h(123), "move seed");
    for (lng x : {INT64_MIN, lng(-1), lng(0), lng(1), INT64_MAX}) {
        check(h(x) == h(ulng(x)), "signed conversion x=" + std::to_string(x));}
    check(h(Kind::LARGE) == h(UINT64_MAX), "scoped enum adapter");
    check(h(Convertible{42}) == h(42) && h(true) == h(1), "legacy conversion adapter");
    check(h(0.0) == h(-0.0) && h(42.75) == h(42), "legacy defined floating conversion");
    check(h(ulll(1) << 64) != h(ulll(0)), "128-bit high word retained");
    check(h(lll(-1)) == h(~ulll(0)), "signed 128-bit words");
    cout << "PASS scalar adapters, explicit/process seed and copy/move\n";
    int a = 7, b = 7; int *p = &a, *q = &b;
    size_t before = h(p); a = 999;
    check(h(p) == before && h(p) != h(q), "pointer address identity after mutation");
    check(h(p) == h(static_cast<void *>(p)), "typed/void pointer addresses");
    check(h(nullptr) == h(static_cast<int *>(nullptr)), "null pointers");
    check(h(&functionPointer) == h(&functionPointer), "function pointer identity");
    safe_unordered_map<int *, int> pointers(0, h); pointers[p] = 11; a = -3;
    check(pointers.at(p) == 11 && !pointers.contains(q), "pointer map independent of pointee");
    char left[] = "text", right[] = "text"; const char *lp = left, *rp = right;
    check(h(left) == h(right) && h(lp) != h(rp), "C-string arrays text, pointers address");
    check(h(string("text")) == h(string_view("text")) && h("text") == h(string("text")), "string adapter equivalence");
    check(h("a\0b") == h("a") && h(string_view("a\0b", 3)) != h("a"), "embedded NUL contract");
    for (int n = 0; n < 80; ++n) {
        string s(n, '\xff'); string padded = "x" + s;
        check(h(s) == h(string_view(padded).substr(1)), "unaligned string length=" + std::to_string(n));}
    cout << "PASS pointer/null/function addresses, C strings, binary/unaligned strings\n";
    vector<int> v{1, 2, 3}; std::list<int> linked(v.begin(), v.end()); array<int, 3> arr{1, 2, 3};
    std::forward_list<int> forward{1, 2, 3}; std::span<const int> view(arr);
    check(h(v) == h(linked) && h(v) == h(arr) && h(v) == h(view) && h(v) == h(forward), "ordered range adapters");
    check(h(vector<bool>{true, false, true}) == h(vector<int>{1, 0, 1}), "vector<bool> proxy");
    check(h(vector<int>{}) == h(tuple<>{}), "empty tuple/range");
    check(h(tuple{1, 2, 3}) == h(v), "tuple ordered fold");
    check(h(v) != h(vector<int>{3, 2, 1}) && h(v) != h(vector<int>{1, 2, 3, 0}), "order/length regression");
    using Key = tuple<string, vector<int>, pair<lng, ulng>>;
    safe_unordered_map<Key, int> nested(0, h);
    Key key{"a\0b", {1, 1, -2}, {-5, UINT64_MAX}}; nested[key] = 31;
    check(nested.at(key) == 31, "nested tuple/pair/string/range map");
    nested.reserve(200); check(nested.at(key) == 31 && nested.hash_function().seed == test_seed, "rehash seed lifetime");
    cout << "PASS pair/tuple/nested/ordered ranges, seed lifetime through rehash\n";
    std::mt19937_64 gen(test_seed);
    const int count = mode == "quick" ? 3000 : mode == "full" ? 100000 : 500000;
    safe_unordered_map<pair<lng, lng>, int> actual(0, h); map<pair<lng, lng>, int> expected;
    safe_unordered_set<ulng> hashes(0, h);
    for (int i = 0; i < count; ++i) {
        pair<lng, lng> k{lng(gen() % 501) - 250, lng(gen() % 501) - 250};
        ++actual[k]; ++expected[k]; hashes.insert(h(ulng(i)));}
    check(actual.size() == expected.size(), "map unique keys");
    for (const auto &[k, value] : expected) { check(actual.at(k) == value, "map equals ordered reference"); }
    check(int(hashes.size()) == count, "64-bit scalar permutation has no collisions");
    safe_unordered_set<string> strings(0, h); strings.insert(""); strings.insert(string("\0\xff", 2));
    check(strings.size() == 2 && strings.contains(string("\0\xff", 2)), "unordered set alias");
#ifdef _GLIBCXX_DEBUG
    const int gpCount = 40;  // PB_DS_DEBUG revalidates every key after every operation: about n^4 total.
#else
    const int gpCount = count;
#endif
    safe_gp_hash_table<lng, int> gp(h); map<lng, int> gpExpected;
    for (int i = 0; i < gpCount; ++i) {
        lng k = lng(gen() % 2001) + (i % 7 == 0 ? INT64_MIN : -1000);
        if (gen() % 3) { ++gp[k]; ++gpExpected[k]; }
        else { check((gp.erase(k) != 0) == (gpExpected.erase(k) != 0), "gp_hash_table erase result"); }}
    check(gp.size() == gpExpected.size() && gp.get_hash_fn().seed == test_seed, "gp_hash_table size and seed");
    for (const auto &[k, value] : gpExpected) {
        auto it = gp.find(k); check(it != gp.end() && it->second == value, "gp_hash_table equals ordered reference");}
    safe_gp_hash_table<pair<int, int>, int> gpPairs; gpPairs[{1, 2}] = 3;
    check(gpPairs.find({2, 1}) == gpPairs.end() && gpPairs[{1, 2}] == 3, "gp_hash_table default process seed and pair keys");
    cout << "PASS map/set/gp_hash_table/reference and distinct scalar hashes cases=" << count << '\n';}
