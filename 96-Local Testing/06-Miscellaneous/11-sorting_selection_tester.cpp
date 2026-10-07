#include "../../06-Miscellaneous/11-sorting_selection.hpp"

ulng test_seed = 0;
lng checks = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
                    << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}

template<typename I>
string showInteger(I x) {
    using U = std::make_unsigned_t<I>; bool negative = std::is_signed_v<I> && x < 0;
    U magnitude = negative ? U(0) - U(x) : U(x); string s;
    do { s.push_back(char('0' + magnitude % 10)); magnitude /= 10; } while (magnitude);
    if (negative) { s.push_back('-'); }
    reverse(s.begin(), s.end()); return s;}
template<typename I>
string show(const vector<I> &a) {
    string s = "[";
    for (I x : a) { s += showInteger(x) + ','; }
    return s + ']';}

template<typename K>
struct Record {
    K key;
    int id;
    Record() = delete;
    Record(K k, int i) : key(k), id(i) {}
    bool operator==(const Record &) const = default;
};
struct CopyRecord {
    int key, id;
    CopyRecord() = delete;
    CopyRecord(int k, int i) : key(k), id(i) {}
    CopyRecord(const CopyRecord &) = default;
    CopyRecord(CopyRecord &&) = delete;
    CopyRecord &operator=(const CopyRecord &) = delete;
    CopyRecord &operator=(CopyRecord &&) = default;
};

template<typename K>
vector<Record<K>> records(const vector<K> &a) {
    vector<Record<K>> out; out.reserve(a.size());
    for (int i = 0; i < int(a.size()); ++i) { out.emplace_back(a[i], i); }
    return out;}
template<typename K>
void checkRecordOrder(const vector<Record<K>> &actual, const vector<Record<K>> &original, const string &what) {
    auto expected = original;
    sort(expected.begin(), expected.end(), [](const auto &x, const auto &y) {
        return x.key < y.key || (x.key == y.key && x.id < y.id);});
    check(actual == expected, what + " sorted/stable/exact-record-permutation");}

template<typename T, typename Compare>
void selectChecks(const vector<T> &a, const vector<int> &ranks, Compare cmp) {
    auto reference = a; sort(reference.begin(), reference.end(), cmp);
    auto original = a; sort(original.begin(), original.end()); int n = int(a.size());
    for (int k : ranks) {
        for (int which = 0; which < 2; ++which) {
            string name = string(which ? "median-of-medians" : "quickselect") + " k=" + std::to_string(k);
            auto copy = a; T got = which ? medianOfMedians(copy, k, cmp) : quickSelect(copy, k, cmp);
            check(!cmp(got, reference[k]) && !cmp(reference[k], got), "selection order statistic " + name);
            check(got == copy[k], "selection returns value at partition index " + name);
            for (int i = 0; i < k; ++i) { check(!cmp(got, copy[i]), "selection left partition " + name); }
            for (int i = k + 1; i < n; ++i) { check(!cmp(copy[i], got), "selection right partition " + name); }
            sort(copy.begin(), copy.end()); check(copy == original, "selection exact multiset preserved " + name);}
        auto copy = a; auto [lt, gt] = threeWayPartition(copy, reference[k], cmp); int below = 0, above = 0;
        for (const T &x : a) { below += cmp(x, reference[k]); above += cmp(reference[k], x); }
        check(lt == below && gt == n - above, "three-way partition bounds k=" + std::to_string(k));
        for (int i = 0; i < n; ++i) {
            bool before = cmp(copy[i], reference[k]), after = cmp(reference[k], copy[i]);
            check(i < lt ? before : i < gt ? !before && !after : after, "three-way partition groups k=" + std::to_string(k));}
        sort(copy.begin(), copy.end()); check(copy == original, "three-way partition exact multiset k=" + std::to_string(k));}}
template<typename T, typename Pred>
void partitionChecks(const vector<T> &a, Pred pred) {
    vector<T> expected;
    for (const T &x : a) { if (pred(x)) { expected.push_back(x); } }
    int cut = int(expected.size());
    for (const T &x : a) { if (!pred(x)) { expected.push_back(x); } }
    auto copy = a; int got = stablePartition(copy, pred);
    check(got == cut && copy == expected, "stable partition cut/order/exact permutation");}

void smallArray(const vector<int> &a) {
    context = "small array=" + show(a); auto expected = a; sort(expected.begin(), expected.end());
    auto copy = a; countingSort(copy, -2, 2); check(copy == expected, "counting sort vs comparison sort");
    copy = a; radixSort(copy); check(copy == expected, "default-key radix sort vs comparison sort");
    auto original = records(a), stable = original;
    stableCountingSort(stable, 5, [](const Record<int> &x) { return x.key + 2; });
    checkRecordOrder(stable, original, "stable counting");
    stable = original; radixSort(stable, [](const Record<int> &x) { return x.key; });
    checkRecordOrder(stable, original, "stable radix");
    vector<int> ranks(a.size()); iota(ranks.begin(), ranks.end(), 0);
    selectChecks(a, ranks, std::less<int>{}); selectChecks(a, ranks, std::greater<int>{});
    partitionChecks(original, [](const Record<int> &x) { return x.key < 0; });
    partitionChecks(original, [](const Record<int> &x) { return x.key % 2 == 0; });}

template<typename I>
void integerType(const string &name, std::mt19937_64 &gen, int rounds) {
    using U = std::make_unsigned_t<I>;
    I lo = std::numeric_limits<I>::min(), hi = std::numeric_limits<I>::max();
    vector<I> fixed{lo, hi, I(0), I(1), lo, hi, I(hi - 1), I(lo + 1)};
    if constexpr (std::is_signed_v<I>) { fixed.push_back(I(-1)); }
    for (int round = -1; round < rounds; ++round) {
        vector<I> a = round < 0 ? fixed : vector<I>(size_t(gen() % 101));
        if (round >= 0) {
            for (I &x : a) {
                U bits = U(gen());
                if constexpr (sizeof(I) > 8) { bits |= U(gen()) << 64; }
                x = std::bit_cast<I>(bits);}}
        context = "type=" + name + " round=" + std::to_string(round) + " array=" + show(a);
        auto expected = a; sort(expected.begin(), expected.end());
        auto copy = a; radixSort(copy); check(copy == expected, "full-width radix scalar");
        auto original = records(a), stable = original;
        radixSort(stable, [](const Record<I> &x) { return x.key; });
        checkRecordOrder(stable, original, "full-width radix records");
        if (!a.empty()) {
            int n = int(a.size()); selectChecks(a, {0, n / 2, n - 1}, std::less<I>{});}}
    for (bool high : {false, true}) {
        I lower = high ? I(hi - 3) : lo, upper = high ? hi : I(lo + 3);
        vector<I> a{upper, lower, I(lower + 1), upper, I(lower + 2), lower};
        context = "narrow full-width-endpoint counting type=" + name + " array=" + show(a);
        auto expected = a; sort(expected.begin(), expected.end()); countingSort(a, lower, upper);
        check(a == expected, "inclusive dense domain at integer endpoint");}
    vector<I> same(20, hi); countingSort(same, hi, hi);
    check(same == vector<I>(20, hi), "singleton domain");
    auto dense = records(vector<I>{I(2), I(0), I(1), I(2), I(1)}), original_dense = dense;
    stableCountingSort(dense, 3, [](const Record<I> &x) { return x.key; });
    checkRecordOrder(dense, original_dense, "counting key width/type=" + name);
    U base = U(U(std::numeric_limits<U>::max() / U(255)) * U(165));
    for (int byte = 0; byte < int(sizeof(I)); ++byte) {
        U mask = U(U(255) << (8 * byte)); vector<I> a;
        for (int digit : {255, 0, 127, 0, 128, 255}) {
            U bits = U((base & U(~mask)) | U(U(digit) << (8 * byte))); a.push_back(std::bit_cast<I>(bits));}
        context = "one varying byte type=" + name + " byte=" + std::to_string(byte) + " array=" + show(a);
        auto original = records(a), stable = original;
        radixSort(stable, [](const Record<I> &x) { return x.key; });
        checkRecordOrder(stable, original, "stable single varying byte / constant-byte skipping");}}

struct Direction {
    bool down;
    Direction() = delete;
    explicit Direction(bool d) : down(d) {}
    bool operator()(int a, int b) const { return down ? a > b : a < b; }
};
struct OffsetKey {
    int offset;
    OffsetKey() = delete;
    explicit OffsetKey(int x) : offset(x) {}
    int operator()(const Record<int> &x) const { return x.key + offset; }
};
struct LengthOrder {
    bool operator()(const string &x, const string &y) const { return x.size() < y.size(); }
};
struct ConstProjector {
    int operator()(const Record<int> &x) const { return x.key; }
    int operator()(Record<int> &) const = delete;
};

void customCases() {
    context = "empty/singleton domains, default and stateful callbacks";
    vector<int> empty;
    countingSort(empty, INT_MIN, INT_MIN); stableCountingSort(empty, 0, std::identity{}); radixSort(empty);
    check(empty.empty() && stablePartition(empty, [](int) { return true; }) == 0, "empty sorts/partition");
    vector<int> singleton{17}; check(quickSelect(singleton, 0) == 17, "default comparator singleton selection");
    vector<int> a{3, 2, 1, 3, 0, 2};
    selectChecks(a, {0, 1, 3, 5}, Direction{true});
    partitionChecks(a, [](int) { return false; }); partitionChecks(a, [](int) { return true; });
    vector<string> words{"zz", "", "a", "xy", "bb", "ccccc", "b"};
    selectChecks(words, {0, 1, 3, 5, 6}, LengthOrder{});
    partitionChecks(words, [](const string &s) { return s.size() < 2; });
    partitionChecks(vector<bool>{true, false, true, false}, [](bool x) { return x; });
    vector<bool> bits{true, false, true}; check(!quickSelect(bits, 0) && quickSelect(bits, 2), "bool standard selection wrapper");
    bits = {true, false, true, true, false, false, true}; check(!medianOfMedians(bits, 2) && medianOfMedians(bits, 3) && threeWayPartition(bits, false) == pair{0, 3}, "bool median of medians and three-way partition");
    auto original = records(a), stable = original;
    stableCountingSort(stable, 6, OffsetKey{2}); checkRecordOrder(stable, original, "stateful counting key");
    stable = original; radixSort(stable, OffsetKey{2}); checkRecordOrder(stable, original, "stateful radix key");
    stable = original;
    stableCountingSort(stable, 4, [](const Record<int> &x) -> ulll { return ulll(x.key); });
    checkRecordOrder(stable, original, "128-bit counting key domain checks");
    stable = original; stableCountingSort(stable, 4, ConstProjector{});
    checkRecordOrder(stable, original, "counting projection receives const record");
    stable = original; radixSort(stable, ConstProjector{});
    checkRecordOrder(stable, original, "radix projection receives const record");
    int projected = 0; stable = original;
    stableCountingSort(stable, 4, [&](const Record<int> &x) { ++projected; return x.key; });
    check(projected == int(a.size()), "stable counting projects each input exactly once");
    projected = 0;
    radixSort(empty, [&](int x) { ++projected; return x; });
    radixSort(singleton, [&](int x) { ++projected; return x; });
    check(projected == 0, "radix skips projection for zero/one input");
    int predicates = 0; auto partitioned = a;
    stablePartition(partitioned, [&](int x) { ++predicates; return x < 2; });
    check(predicates == int(a.size()), "partition calls predicate exactly once per input");
    auto selected_records = original;
    auto selected = quickSelect(selected_records, 2, [](const Record<int> &x, const Record<int> &y) { return x.key < y.key; });
    check(selected.key == 2 && selected == selected_records[2], "record selection with comparator only");
    vector<int> ids;
    for (const auto &x : selected_records) { ids.push_back(x.id); }
    sort(ids.begin(), ids.end()); check(ids == vector<int>{0, 1, 2, 3, 4, 5}, "record selection permutation");
    selected_records = original;
    selected = medianOfMedians(selected_records, 4, [](const Record<int> &x, const Record<int> &y) { return x.key < y.key; });
    check(selected.key == 3 && selected == selected_records[4], "record median of medians with comparator only");
    context = "stable records require copy construction and move assignment only";
    vector<CopyRecord> copy_only; copy_only.reserve(4);
    copy_only.emplace_back(2, 0); copy_only.emplace_back(0, 1); copy_only.emplace_back(2, 2); copy_only.emplace_back(1, 3);
    auto second = copy_only;
    stableCountingSort(copy_only, 3, [](const CopyRecord &x) { return x.key; });
    radixSort(second, [](const CopyRecord &x) { return x.key; });
    for (int i = 0; i < 4; ++i) {
        check(copy_only[i].id == vector<int>{1, 3, 0, 2}[i] && second[i].id == copy_only[i].id,
              "non-default/non-move-constructible stable records");}
    context = "callback exceptions propagate";
    for (int which = 0; which < 6; ++which) {
        auto values = a; bool caught = false;
        try {
            if (which == 0) { stableCountingSort(values, 4, [](int) -> int { throw std::runtime_error("callback"); }); }
            if (which == 1) { radixSort(values, [](int) -> int { throw std::runtime_error("callback"); }); }
            if (which == 2) { stablePartition(values, [](int) -> bool { throw std::runtime_error("callback"); }); }
            if (which == 3) { quickSelect(values, 2, [](int, int) -> bool { throw std::runtime_error("callback"); }); }
            if (which == 4) { medianOfMedians(values, 2, [](int, int) -> bool { throw std::runtime_error("callback"); }); }
            if (which == 5) { threeWayPartition(values, 2, [](int, int) -> bool { throw std::runtime_error("callback"); }); }} catch (const std::runtime_error &e) { caught = string(e.what()) == "callback"; }
        check(caught, "callback exception propagated API=" + std::to_string(which));}
    cout << "PASS default/stateful/equivalence/string/bool wrappers, stable record construction domain, callback exceptions\n";}

void largeCases(const string &mode) {
    int n = mode == "quick" ? 3000 : 200000;
    for (int shape = 0; shape < 5; ++shape) {
        vector<int> a(n);
        for (int i = 0; i < n; ++i) {
            a[i] = shape == 0 ? i : shape == 1 ? n - 1 - i : shape == 2 ? 7
                 : shape == 3 ? min(i, n - 1 - i) : (i % 97) - 48;}
        context = "large shape=" + std::to_string(shape) + " n=" + std::to_string(n);
        auto expected = a; sort(expected.begin(), expected.end());
        auto copy = a; countingSort(copy, min(-48, expected.front()), max(7, expected.back()));
        check(copy == expected, "large counting sort");
        copy = a; radixSort(copy); check(copy == expected, "large radix sort");
        auto original = records(a), stable = original;
        radixSort(stable, [](const Record<int> &x) { return x.key; }); checkRecordOrder(stable, original, "large stable radix");
        stable = original; stableCountingSort(stable, n + 49, OffsetKey{48});
        checkRecordOrder(stable, original, "large stable counting");
        partitionChecks(original, [](const Record<int> &x) { return x.key % 3 == 0; });
        selectChecks(a, {0, n / 4, n / 2, n - 1}, Direction{true});
        lng calls = 0; auto copy2 = a;
        medianOfMedians(copy2, n / 2, [&](int x, int y) { ++calls; return x < y; });
        check(calls <= 40 * lng(n), "median of medians comparisons linear calls=" + std::to_string(calls));}
    context = "large random n=" + std::to_string(n); std::mt19937_64 gen(test_seed); vector<int> a(n);
    for (int &x : a) { x = int(gen() % 1000000); }
    for (int k : {0, n / 4, n / 2, n - 1}) {
        lng calls = 0; auto copy = a; int got = medianOfMedians(copy, k, [&](int x, int y) { ++calls; return x < y; });
        auto expected = a; std::nth_element(expected.begin(), expected.begin() + k, expected.end());
        check(got == expected[k] && calls <= 40 * lng(n), "random median of medians within the 40n regression bound calls=" + std::to_string(calls));}
    cout << "PASS large ascending/reverse/equal/organ-pipe/sawtooth/random shapes n=" << n << '\n';}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string p = argv[++i]; vector<int> a{1, 2};
            if (p == "count-reverse") { countingSort(a, 2, 1); }
            if (p == "count-below") { countingSort(a, 2, 3); }
            if (p == "count-above") { countingSort(a, 0, 1); }
            if (p == "count-width") { vector<lng> b; countingSort(b, lng(0), lng(INT_MAX)); }
            if (p == "count-wide-width") { vector<lll> b; countingSort(b, std::numeric_limits<lll>::min(), std::numeric_limits<lll>::max()); }
            if (p == "alphabet-negative") { vector<int> b; stableCountingSort(b, -1, std::identity{}); }
            if (p == "alphabet-zero") { stableCountingSort(a, 0, std::identity{}); }
            if (p == "key-negative") { stableCountingSort(a, 3, [](int) { return -1; }); }
            if (p == "key-past-end") { stableCountingSort(a, 2, std::identity{}); }
            if (p == "key-wide") { stableCountingSort(a, 2, [](int) -> ulll { return ulll(1) << 100; }); }
            if (p == "select-negative") { quickSelect(a, -1); }
            if (p == "select-past-end") { quickSelect(a, 2); }
            if (p == "select-empty") { vector<int> b; quickSelect(b, 0); }
            if (p == "median-negative") { medianOfMedians(a, -1); }
            if (p == "median-past-end") { medianOfMedians(a, 2); }
            if (p == "median-empty") { vector<int> b; medianOfMedians(b, 0); }
            return 1;}}
    int bound = mode == "quick" ? 4 : mode == "full" ? 6 : 7, arrays = 0;
    for (int n = 0, count = 1; n <= bound; ++n, count *= 5) {
        for (int mask = 0; mask < count; ++mask) {
            vector<int> a(n); int code = mask;
            for (int &x : a) { x = code % 5 - 2; code /= 5; }
            smallArray(a); ++arrays;}}
    cout << "PASS exhaustive five-value arrays=" << arrays << " max_length=" << bound << '\n';
    customCases(); std::mt19937_64 gen(test_seed);
    int rounds = mode == "quick" ? 10 : mode == "full" ? 300 : 3000;
    integerType<int8_t>("i8", gen, rounds); integerType<uint8_t>("u8", gen, rounds);
    integerType<int16_t>("i16", gen, rounds); integerType<uint16_t>("u16", gen, rounds);
    integerType<int>("i32", gen, rounds); integerType<uint>("u32", gen, rounds);
    integerType<lng>("i64", gen, rounds); integerType<ulng>("u64", gen, rounds);
    integerType<lll>("i128", gen, rounds); integerType<ulll>("u128", gen, rounds);
    integerType<char>("char", gen, 1); integerType<wchar_t>("wchar_t", gen, 1);
    integerType<char16_t>("char16_t", gen, 1); integerType<char32_t>("char32_t", gen, 1);
    cout << "PASS full-width signed/unsigned integer types through 128 bits rounds/type=" << rounds << '\n';
    largeCases(mode); cout << "PASS sorting/selection total checks=" << checks << " seed=" << test_seed << '\n';}
