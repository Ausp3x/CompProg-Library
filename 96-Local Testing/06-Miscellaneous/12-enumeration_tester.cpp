#include "../../06-Miscellaneous/12-enumeration.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=true actual=false\n"; std::exit(1); }
}
string show(const vector<int> &a) {
    string text = "[";
    for (int x : a) { text += std::to_string(x) + ','; }
    return text + ']';
}
template<class U> string show(U x) {
    string text;
    do { text += "0123456789abcdef"[int(x & 15)]; x >>= 4; } while (x);
    reverse(text.begin(), text.end()); return "0x" + text;
}
template<class T> void checkEqual(const T &actual, const T &expected, const string &what) {
    ++checks;
    if (actual != expected) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=" << show(expected)
             << " actual=" << show(actual) << '\n'; std::exit(1); }
}

// Each recursive leaf independently specifies a mathematical object; production
// steppers, low-mask helpers and the binary reflected-Gray formula are not oracles.
vector<vector<int>> combinationOracle(int n, int k, bool replacement = false) {
    vector<vector<int>> out; vector<int> state;
    auto visit = [&](auto &&visit, int first) -> void {
        if (int(state.size()) == k) { out.push_back(state); return; }
        for (int i = first; i < n; ++i) {
            state.push_back(i); visit(visit, i + !replacement); state.pop_back(); }
    };
    visit(visit, 0); return out;
}
vector<vector<int>> subsetOracle(int n) {
    vector<vector<int>> out; vector<int> state;
    auto visit = [&](auto &&visit, int i) -> void {
        if (i < 0) { out.push_back(state); return; }
        visit(visit, i - 1); state.push_back(i); visit(visit, i - 1); state.pop_back();
    };
    visit(visit, n - 1); return out;
}
vector<vector<int>> productOracle(const vector<int> &radices, int first = 0, bool gray = false) {
    if (first == int(radices.size())) { return {{}}; }
    auto suffixes = productOracle(radices, first + 1, gray); vector<vector<int>> out;
    for (int x = 0; x < radices[first]; ++x) {
        for (const auto &suffix : suffixes) {
            vector<int> state{x}; state.insert(state.end(), suffix.begin(), suffix.end()); out.push_back(state); }
        if (gray) { reverse(suffixes.begin(), suffixes.end()); }}
    return out;
}

template<class T, class Generate>
void sequenceCheck(const vector<T> &expected, Generate generate, bool thorough = false) {
    ++cases;
    int total = int(expected.size()), seen = 0;
    check(generate([&](const T &state) {
        check(seen < total, "no surplus output at=" + std::to_string(seen));
        checkEqual(state, expected[seen], "complete output at=" + std::to_string(seen)); ++seen; return true;
    }), "true return after natural completion");
    check(seen == total, "complete callback count");
    vector<int> stops;
    if (total && thorough) { for (int i = 1; i <= total; ++i) { stops.push_back(i); } }
    else if (total) { stops = {1, (total + 1) / 2, total}; }
    for (int stop : stops) {
        seen = 0;
        check(!generate([&](const T &state) {
            check(seen < stop, "no callback after cancellation");
            checkEqual(state, expected[seen], "cancelled prefix at=" + std::to_string(seen));
            return ++seen < stop;
        }), "false return even when last item cancels");
        check(seen == stop, "exact cancellation callback count"); }
}
template<class Generate>
void grayVectorCheck(const vector<vector<int>> &expected, Generate generate, bool thorough) {
    sequenceCheck(expected, [&](auto visit) {
        int seen = 0; vector<int> previous;
        return generate([&](auto &state, int changed) {
            static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>);
            int want = -1;
            if (seen) {
                for (int i = 0; i < int(state.size()); ++i) {
                    if (state[i] != previous[i]) {
                        check(want < 0 && abs(state[i] - previous[i]) == 1, "exactly one unit Gray change"); want = i; }}
                check(want >= 0, "Gray output differs from predecessor"); }
            check(changed == want, "reported Gray changed dimension");
            previous = state; ++seen; return visit(state);
        });
    }, thorough);
}

void vectorCases(const string &mode) {
    int bound = mode == "quick" ? 6 : mode == "full" ? 10 : 12;
    for (int n = 0; n <= bound; ++n) {
        for (int k = 0; k <= n; ++k) {
            context = "combination n=" + std::to_string(n) + " k=" + std::to_string(k);
            sequenceCheck(combinationOracle(n, k), [&](auto visit) {
                return forEachCombination(n, k, [&](auto &state) {
                    static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);
                });
            }, n <= 4); }
        context = "empty combination n=" + std::to_string(n);
        for (int k : {n + 1, INT_MAX}) {
            sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachCombination(n, k, visit); }); }
        context = "subset n=" + std::to_string(n);
        sequenceCheck(subsetOracle(n), [&](auto visit) {
            return forEachSubset(n, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);
            });
        }, n <= 4); }
    cout << "PASS vector combinations/subsets independent recursive oracle n=0.." << bound << '\n';
    int multi_bound = mode == "quick" ? 4 : mode == "full" ? 6 : 8;
    for (int n = 0; n <= multi_bound; ++n) { for (int k = 0; k <= multi_bound; ++k) {
        context = "multicombination n=" + std::to_string(n) + " k=" + std::to_string(k);
        sequenceCheck(combinationOracle(n, k, true), [&](auto visit) {
            return forEachMulticombination(n, k, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);
            });
        }, n <= 3 && k <= 3); }}
    context = "empty multicombination n=0 k=INT_MAX";
    sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachMulticombination(0, INT_MAX, visit); });
    cout << "PASS multicombinations independent recursive oracle n,k=0.." << multi_bound << '\n';
    int max_dims = mode == "quick" ? 3 : mode == "full" ? 5 : 6, inputs = 0;
    auto check_radices = [&](const vector<int> &radices) {
        auto saved = radices;
        context = "product radices=" + show(radices);
        sequenceCheck(productOracle(radices), [&](auto visit) {
            return forEachProduct(radices, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);
            });
        }, radices.size() <= 2);
        context = "Gray product radices=" + show(radices);
        grayVectorCheck(productOracle(radices, 0, true), [&](auto visit) {
            return forEachGrayProduct(radices, visit);
        }, radices.size() <= 2);
        checkEqual(radices, saved, "input radices remain unchanged"); ++inputs;
    };
    for (int n = 0; n <= max_dims; ++n) {
        vector<int> radices(n);
        auto visit = [&](auto &&visit, int i) -> void {
            if (i == n) { check_radices(radices); return; }
            for (int x = 0; x <= 3; ++x) { radices[i] = x; visit(visit, i + 1); }
        };
        visit(visit, 0); }
    std::mt19937_64 gen(test_seed);
    int rounds = mode == "quick" ? 100 : mode == "full" ? 1500 : 10000;
    for (int round = 0; round < rounds; ++round) {
        vector<int> radices(int(gen() % 9)); int count = 1;
        for (int &r : radices) { r = int(gen() % 5) + (round % 2); if (count * r > 4096) { r = 1; } count *= r; }
        check_radices(radices); }
    cout << "PASS products/reflected mixed-radix Gray exhaustive dims=0.." << max_dims
         << " radices=0..3 plus random=" << rounds << " total_inputs=" << inputs << '\n';
}

template<class U> vector<U> maskOracle(int n, int k = -1) {
    vector<U> out;
    auto visit = [&](auto &&visit, int bit, U word, int count) -> void {
        if (bit < 0) { if (k < 0 || count == k) { out.push_back(word); } return; }
        visit(visit, bit - 1, word, count); visit(visit, bit - 1, U(word | (U(1) << bit)), count + 1);
    };
    visit(visit, n - 1, 0, 0); return out;
}
template<class U> vector<U> grayOracle(int n) {
    vector<U> out{0};
    for (int bit = 0; bit < n; ++bit) {
        int count = int(out.size());
        for (int i = count - 1; i >= 0; --i) { out.push_back(U(out[i] | (U(1) << bit))); }}
    return out;
}
template<class U, class Generate>
void grayMaskCheck(const vector<U> &expected, Generate generate) {
    sequenceCheck(expected, [&](auto visit) {
        int seen = 0; U previous = 0;
        return generate([&](U state, int changed) {
            int want = -1;
            if (seen) {
                for (int i = 0; i < int(sizeof(U) * CHAR_BIT); ++i) {
                    if (((state >> i) & 1) != ((previous >> i) & 1)) {
                        check(want < 0, "one Gray-mask bit changes"); want = i; }}
                check(want >= 0, "Gray mask differs from predecessor"); }
            check(changed == want, "reported Gray-mask bit"); previous = state; ++seen; return visit(state);
        });
    }, expected.size() <= 16);
}
template<class U> void submaskCheck(U mask) {
    vector<int> bits;
    for (int i = int(sizeof(U) * CHAR_BIT) - 1; i >= 0; --i) {
        if ((mask >> i) & 1) { bits.push_back(i); }}
    vector<U> expected;
    auto visit = [&](auto &&visit, int i, U state) -> void {
        if (i == int(bits.size())) { expected.push_back(state); return; }
        visit(visit, i + 1, U(state | (U(1) << bits[i]))); visit(visit, i + 1, state);
    };
    visit(visit, 0, 0);
    context = "submask width=" + std::to_string(sizeof(U) * CHAR_BIT) + " mask=" + show(mask);
    sequenceCheck(expected, [&](auto visitor) { return forEachSubmask(mask, visitor); }, bits.size() <= 3);
}
template<class U> void wordCases(const string &mode) {
    int width = int(sizeof(U) * CHAR_BIT), bound = min(width, mode == "quick" ? 6 : mode == "full" ? 10 : 12);
    for (int n = 0; n <= bound; ++n) {
        context = "subset-mask width=" + std::to_string(width) + " n=" + std::to_string(n);
        sequenceCheck(maskOracle<U>(n), [&](auto visit) { return forEachSubsetMask<U>(n, visit); }, n <= 4);
        context = "Gray-mask width=" + std::to_string(width) + " n=" + std::to_string(n);
        grayMaskCheck(grayOracle<U>(n), [&](auto visit) { return forEachGrayMask<U>(n, visit); });
        for (int k = 0; k <= n; ++k) {
            context = "combination-mask width=" + std::to_string(width) + " n=" + std::to_string(n) + " k=" + std::to_string(k);
            sequenceCheck(maskOracle<U>(n, k), [&](auto visit) { return forEachCombinationMask<U>(n, k, visit); }, n <= 4); }
        for (int k : {n + 1, INT_MAX}) {
            context = "empty combination-mask width=" + std::to_string(width) + " n=" + std::to_string(n);
            sequenceCheck(vector<U>{}, [&](auto visit) { return forEachCombinationMask<U>(n, k, visit); }); }}
    for (int x = 0; x < (1 << bound); ++x) { submaskCheck(U(x)); }
    for (int bit = 0; bit < width; ++bit) {
        submaskCheck(U(U(1) << bit)); submaskCheck(U((U(1) << bit) | 1)); }
    context = "full-width boundaries width=" + std::to_string(width);
    U all = U(~U(0));
    sequenceCheck(vector<U>{all}, [&](auto visit) { return forEachCombinationMask<U>(width, width, visit); });
    sequenceCheck(vector<U>{0}, [&](auto visit) { return forEachCombinationMask<U>(width, 0, visit); });
    vector<U> singles, missing;
    for (int bit = 0; bit < width; ++bit) { singles.push_back(U(U(1) << bit)); }
    for (int bit = width - 1; bit >= 0; --bit) { missing.push_back(U(all ^ (U(1) << bit))); }
    sequenceCheck(singles, [&](auto visit) { return forEachCombinationMask<U>(width, 1, visit); });
    sequenceCheck(missing, [&](auto visit) { return forEachCombinationMask<U>(width, width - 1, visit); });
    int seen = 0;
    check(!forEachSubsetMask<U>(width, [&](U state) { checkEqual(state, U(seen), "full-width subset prefix"); return ++seen < 10; }), "cancel full-width subsets");
    check(seen == 10, "full-width subset cancellation count"); seen = 0;
    check(!forEachSubmask(all, [&](U state) { checkEqual(state, U(all - seen), "full-width submask prefix"); return ++seen < 10; }), "cancel full-width submasks");
    check(seen == 10, "full-width submask cancellation count"); seen = 0;
    auto gray = grayOracle<U>(4);
    check(!forEachGrayMask<U>(width, [&](U state, int changed) {
        checkEqual(state, gray[seen], "full-width Gray prefix");
        if (!seen) { check(changed == -1, "full-width Gray initial change"); }
        return ++seen < 10;
    }), "cancel full-width Gray masks");
    check(seen == 10, "full-width Gray cancellation count");
    std::mt19937_64 gen(test_seed + width);
    int rounds = mode == "quick" ? 30 : mode == "full" ? 300 : 3000;
    for (int round = 0; round < rounds; ++round) {
        U mask = 0; int count = int(gen() % 11);
        for (int i = 0; i < count; ++i) { mask |= U(U(1) << (gen() % width)); }
        submaskCheck(mask); }
    cout << "PASS words width=" << width << " exhaustive n=0.." << bound
         << " submasks=" << (1 << bound) << " random_sparse=" << rounds << " full-width edges\n";
}

void stateCases(const string &mode) {
    context = "default unsigned mask type";
    sequenceCheck(vector<ulng>{0, 1, 2, 3}, [](auto visit) { return forEachSubsetMask(2, visit); });
    sequenceCheck(vector<ulng>{1, 2, 4}, [](auto visit) { return forEachCombinationMask(3, 1, visit); });
    grayMaskCheck(vector<ulng>{0, 1, 3, 2}, [](auto visit) { return forEachGrayMask(2, visit); });
    context = "nested callbacks keep traversal state independent";
    int outer = 0, inner = 0;
    check(forEachCombination(4, 2, [&](const vector<int> &state) {
        auto saved = state;
        check(forEachSubset(3, [&](const vector<int> &) { ++inner; return true; }), "nested completion");
        checkEqual(state, saved, "outer borrowed state survives nested traversal"); ++outer; return true;
    }), "outer completion");
    check(outer == 6 && inner == 48, "nested callback counts");
    context = "exception propagation";
    for (int api = 0; api < 9; ++api) {
        bool caught = false; int seen = 0;
        auto fail = [&](auto &&...) -> bool { if (++seen == 2) { throw std::runtime_error("visit"); } return true; };
        try {
            if (api == 0) { forEachCombination(3, 1, fail); }
            if (api == 1) { forEachSubset(2, fail); }
            if (api == 2) { forEachProduct({2, 3}, fail); }
            if (api == 3) { forEachGrayProduct({2, 3}, fail); }
            if (api == 4) { forEachSubmask(ulng(3), fail); }
            if (api == 5) { forEachSubsetMask(2, fail); }
            if (api == 6) { forEachCombinationMask(3, 1, fail); }
            if (api == 7) { forEachGrayMask(2, fail); }
            if (api == 8) { forEachMulticombination(2, 3, fail); }
        } catch (const std::runtime_error &e) { caught = string(e.what()) == "visit"; }
        check(caught && seen == 2, "exception propagates at callback two api=" + std::to_string(api)); }
    int n = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000, seen = 0;
    context = "large dimensions n=" + std::to_string(n);
    check(!forEachSubset(n, [&](const vector<int> &state) {
        vector<int> expected;
        for (int bit = 3; bit >= 0; --bit) { if ((seen >> bit) & 1) { expected.push_back(bit); } }
        checkEqual(state, expected, "large subset first ten"); return ++seen < 10;
    }), "large subset cancellation");
    vector<int> radices(n, 1); radices[n / 2] = 3; radices.back() = 2;
    seen = 0;
    check(forEachProduct(radices, [&](const vector<int> &state) {
        check(int(state.size()) == n && state[n / 2] == seen / 2 && state.back() == seen % 2, "large product active digits");
        check(accumulate(state.begin(), state.end(), 0) == seen / 2 + seen % 2, "large product inactive digits"); ++seen; return true;
    }), "large sparse product completion"); check(seen == 6, "large sparse product count");
    seen = 0;
    check(forEachGrayProduct(radices, [&](const vector<int> &state, int changed) {
        int a = seen / 2, b = (a % 2) ? 1 - seen % 2 : seen % 2;
        check(int(state.size()) == n && state[n / 2] == a && state.back() == b, "large Gray product active digits");
        check(changed == (!seen ? -1 : seen % 2 ? n - 1 : n / 2), "large Gray product active change");
        check(accumulate(state.begin(), state.end(), 0) == a + b, "large Gray inactive digits"); ++seen; return true;
    }), "large sparse Gray product completion"); check(seen == 6, "large sparse Gray product count");
    seen = 0;
    check(forEachMulticombination(1, n, [&](const vector<int> &state) {
        check(int(state.size()) == n && accumulate(state.begin(), state.end(), 0) == 0, "large constant multicombination");
        ++seen; return true;
    }) && seen == 1, "large multicombination completes without recursion");
    context = "INT_MAX coordinates/radices without output-count arithmetic"; seen = 0;
    check(!forEachCombination(INT_MAX, 2, [&](const vector<int> &state) {
        checkEqual(state, vector<int>{0, seen + 1}, "INT_MAX combination prefix"); return ++seen < 3;
    }), "INT_MAX combination cancellation");
    seen = 0;
    check(!forEachMulticombination(INT_MAX, 2, [&](const vector<int> &state) {
        checkEqual(state, vector<int>{0, seen}, "INT_MAX multicombination prefix"); return ++seen < 3;
    }), "INT_MAX multicombination cancellation");
    for (bool gray : {false, true}) {
        seen = 0;
        auto visit = [&](const vector<int> &state) {
            checkEqual(state, vector<int>{0, seen}, "INT_MAX radix prefix"); return ++seen < 3;
        };
        bool done = gray ? forEachGrayProduct({INT_MAX, INT_MAX}, [&](const vector<int> &s, int changed) {
            check(changed == (seen ? 1 : -1), "INT_MAX Gray dimension"); return visit(s);
        }) : forEachProduct({INT_MAX, INT_MAX}, visit);
        check(!done && seen == 3, "INT_MAX product cancellation"); }
    cout << "PASS default types, nesting, all callback exceptions, large n=" << n << ", INT_MAX coordinates/radices\n";
}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i]; auto visit = [](auto &&...) { return true; };
            if (probe == "combination-negative-n") { forEachCombination(-1, 0, visit); }
            if (probe == "combination-negative-k") { forEachCombination(3, -1, visit); }
            if (probe == "multicombination-negative-n") { forEachMulticombination(-1, 0, visit); }
            if (probe == "multicombination-negative-k") { forEachMulticombination(3, -1, visit); }
            if (probe == "subset-negative-n") { forEachSubset(-1, visit); }
            if (probe == "product-negative") { forEachProduct({2, -1}, visit); }
            if (probe == "product-zero-negative") { forEachProduct({0, -1}, visit); }
            if (probe == "gray-product-negative") { forEachGrayProduct({2, -1}, visit); }
            if (probe == "gray-product-zero-negative") { forEachGrayProduct({0, -1}, visit); }
            if (probe == "subset-mask-negative") { forEachSubsetMask(-1, visit); }
            if (probe == "subset-mask-large") { forEachSubsetMask<uint8_t>(9, visit); }
            if (probe == "combination-mask-negative-n") { forEachCombinationMask(-1, 0, visit); }
            if (probe == "combination-mask-large-n") { forEachCombinationMask<ulll>(129, 0, visit); }
            if (probe == "combination-mask-negative-k") { forEachCombinationMask(2, -1, visit); }
            if (probe == "gray-mask-negative") { forEachGrayMask(-1, visit); }
            if (probe == "gray-mask-large") { forEachGrayMask<uint8_t>(9, visit); }
            return 1; }}
    vectorCases(mode);
    wordCases<uint8_t>(mode); wordCases<uint16_t>(mode); wordCases<uint>(mode);
    wordCases<ulng>(mode); wordCases<ulll>(mode); stateCases(mode);
    cout << "PASS enumeration cases=" << cases << " checks=" << checks << " seed=" << test_seed << '\n';
}
