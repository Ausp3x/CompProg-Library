#include "../../02-Data Structures/08-monotone_stack.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    int checks = 0;
    string show(lll x) {
        if (!x) { return "0"; }
        bool negative = x < 0;
        ulll magnitude = negative ? ulll(0) - ulll(x) : ulll(x);
        string s;
        while (magnitude) { s += char('0' + magnitude % 10); magnitude /= 10; }
        if (negative) { s += '-'; }
        std::reverse(s.begin(), s.end()); return s;}
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &a) {
        string s = "[";
        for (int i = 0; i < int(a.size()); ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (got != want) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    template<typename T, typename C>
    vector<int> bruteNearest(const vector<T> &a, bool previous, bool strict, C less) {
        int n = int(a.size()); vector<int> ans(n, previous ? -1 : n);
        for (int i = 0; i < n; ++i) {
            for (int j = i + (previous ? -1 : 1); j >= 0 && j < n; j += previous ? -1 : 1) {
                if (strict ? less(a[j], a[i]) : !less(a[i], a[j])) { ans[i] = j; break; }}}
        return ans;}
    template<typename T, typename C>
    vector<int> bruteWindows(const vector<T> &a, int k, bool rightmost, C less) {
        int n = int(a.size()); vector<int> ans;
        for (int l = 0; l <= n - k; ++l) {
            int best = l;
            for (int j = l + 1; j < l + k; ++j) {
                if (less(a[j], a[best]) || (rightmost && !less(a[best], a[j]))) { best = j; }}
            ans.push_back(best);}
        return ans;}

    void boundaries(const vector<lng> &a, bool all_windows = true) {
        context = "array=" + show(a); int n = int(a.size());
        for (bool strict : {false, true}) {
            string suffix = " strict=" + show(strict);
            checkEqual(previousSmaller(a, strict), bruteNearest(a, true, strict, std::less<lng>{}), "previousSmaller" + suffix);
            checkEqual(nextSmaller(a, strict), bruteNearest(a, false, strict, std::less<lng>{}), "nextSmaller" + suffix);
            checkEqual(previousGreater(a, strict), bruteNearest(a, true, strict, std::greater<lng>{}), "previousGreater" + suffix);
            checkEqual(nextGreater(a, strict), bruteNearest(a, false, strict, std::greater<lng>{}), "nextGreater" + suffix);}
        vector<int> widths;
        if (all_windows) { for (int k = 1; k <= n + 1; ++k) { widths.push_back(k); } }
        else { widths = {1, 2, std::max(1, n / 2), std::max(1, n), n + 1}; }
        widths.push_back(INT_MAX);
        for (int k : widths) {
            for (bool rightmost : {false, true}) {
                string suffix = " k=" + show(k) + " rightmost=" + show(rightmost);
                checkEqual(slidingMinimum(a, k, rightmost), bruteWindows(a, k, rightmost, std::less<lng>{}), "slidingMinimum" + suffix);
                checkEqual(slidingMaximum(a, k, rightmost), bruteWindows(a, k, rightmost, std::greater<lng>{}), "slidingMaximum" + suffix);}}
        checkEqual(previousSmaller(a), previousSmaller(a, true), "default previous strict");
        checkEqual(nextSmaller(a), nextSmaller(a, true), "default next strict");
        checkEqual(previousGreater(a), previousGreater(a, true), "default previous greater strict");
        checkEqual(nextGreater(a), nextGreater(a, true), "default next greater strict");
        checkEqual(slidingMinimum(a, 1), slidingMinimum(a, 1, false), "default minimum tie");
        checkEqual(slidingMaximum(a, 1), slidingMaximum(a, 1, false), "default maximum tie");}

    HistogramRectangle bruteHistogram(const vector<lng> &a) {
        HistogramRectangle ans; int n = int(a.size());
        for (int l = 0; l < n; ++l) {
            lng height = a[l];
            for (int r = l + 1; r <= n; ++r) {
                height = std::min(height, a[r - 1]); lll area = lll(height) * (r - l);
                if (area > 0 && (area > ans.area || (area == ans.area && std::tie(l, r) < std::tie(ans.l, ans.r)))) {
                    ans = {area, l, r, height};}}}
        return ans;}
    void checkHistogram(const vector<lng> &a, const HistogramRectangle &want) {
        context = "histogram=" + show(a);
        auto got = largestHistogramRectangle(a);
        checkEqual(got.area, want.area, "largest histogram area");
        checkEqual(got.l, want.l, "histogram left tie"); checkEqual(got.r, want.r, "histogram right tie");
        checkEqual(got.height, want.height, "histogram height");
        if (!got.area) {
            checkEqual(got.l == -1 && got.r == -1 && got.height == 0, true, "empty histogram sentinel"); return;}
        checkEqual(0 <= got.l && got.l < got.r && got.r <= int(a.size()), true, "histogram witness bounds");
        checkEqual(got.area, lll(got.height) * (got.r - got.l), "histogram witness area");
        for (int i = got.l; i < got.r; ++i) { checkEqual(a[i] >= got.height, true, "histogram bar at " + show(i)); }}

    BinaryRectangle bruteMatrix(const vector<vector<int>> &a, int value) {
        BinaryRectangle ans; int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        for (int top = 0; top < n; ++top) {
            for (int bottom = top + 1; bottom <= n; ++bottom) {
                for (int left = 0; left < m; ++left) {
                    for (int right = left + 1; right <= m; ++right) {
                        bool good = true;
                        for (int i = top; i < bottom && good; ++i) {
                            for (int j = left; j < right; ++j) {
                                if (a[i][j] != value) { good = false; break; }}}
                        lng area = lng(bottom - top) * (right - left);
                        if (good && (area > ans.area || (area == ans.area && std::tie(top, left, bottom, right) < std::tie(ans.top, ans.left, ans.bottom, ans.right)))) {
                            ans = {area, top, left, bottom, right};}}}}}
        return ans;}
    void sameRectangle(const BinaryRectangle &got, const BinaryRectangle &want, const string &op) {
        checkEqual(got.area, want.area, op + " area");
        checkEqual(got.top, want.top, op + " top"); checkEqual(got.left, want.left, op + " left");
        checkEqual(got.bottom, want.bottom, op + " bottom"); checkEqual(got.right, want.right, op + " right");}
    void matrix(const vector<vector<int>> &a) {
        context = "matrix=" + show(a); int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        for (int value : {0, 1}) {
            auto got = largestBinaryRectangle(a, value), want = bruteMatrix(a, value);
            sameRectangle(got, want, "binary rectangle value=" + show(value));
            if (!got.area) {
                checkEqual(got.top == -1 && got.left == -1 && got.bottom == -1 && got.right == -1, true, "empty binary sentinel");}
            else {
                checkEqual(0 <= got.top && got.top < got.bottom && got.bottom <= n && 0 <= got.left && got.left < got.right && got.right <= m,
                           true, "binary witness bounds");
                checkEqual(got.area, lng(got.bottom - got.top) * (got.right - got.left), "binary witness area");
                for (int i = got.top; i < got.bottom; ++i) {
                    for (int j = got.left; j < got.right; ++j) {
                        checkEqual(a[i][j], value, "binary witness cell(" + show(i) + "," + show(j) + ")");}}}
            vector<vector<int>> complement = a;
            for (auto &row : complement) { for (int &x : row) { x ^= 1; } }
            sameRectangle(largestBinaryRectangle(complement, 1 - value), got, "binary complement symmetry");}
        sameRectangle(largestBinaryRectangle(a), largestBinaryRectangle(a, 1), "default bit");
        checkEqual(maxZeroSubmatrix(a), bruteMatrix(a, 0).area, "legacy maxZeroSubmatrix");}

    void exhaustive() {
        int bound = mode == "quick" ? 4 : mode == "full" ? 7 : 9;
        for (int n = 0, count = 1; n <= bound; ++n, count *= 3) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 3 - 1; x /= 3; }
                boundaries(a);}}
        std::cout << "PASS exhaustive strict/nonstrict nearest indices and sliding tie policies\n";
        for (int n = 0, count = 1; n <= bound; ++n, count *= 4) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 4; x /= 4; }
                checkHistogram(a, bruteHistogram(a));}}
        std::cout << "PASS exhaustive histograms and independent witnesses\n";
        int cells = mode == "quick" ? 9 : mode == "full" ? 12 : 16;
        for (int n = 0; n <= 4; ++n) {
            for (int m = 0; m <= 4; ++m) {
                if (n * m > cells) { continue; }
                for (int code = 0; code < (1 << (n * m)); ++code) {
                    vector<vector<int>> a(n, vector<int>(m)); int x = code;
                    for (auto &row : a) { for (int &v : row) { v = x & 1; x >>= 1; } }
                    matrix(a);}}}
        std::cout << "PASS exhaustive binary matrices, lexicographic witnesses and complements\n";}

    struct Item { int key, id; };
    void genericOrder() {
        vector<Item> a{{2, 0}, {1, 1}, {1, 2}, {3, 3}, {1, 4}};
        auto less = [](const Item &x, const Item &y) { return x.key < y.key; };
        context = "projection keys=[2,1,1,3,1], distinct ids";
        for (bool strict : {false, true}) {
            checkEqual(previousSmaller(a, strict, less), bruteNearest(a, true, strict, less), "projected previous");
            checkEqual(nextSmaller(a, strict, less), bruteNearest(a, false, strict, less), "projected next");}
        for (int k = 1; k <= int(a.size()) + 1; ++k) {
            for (bool rightmost : {false, true}) {
                checkEqual(slidingMinimum(a, k, rightmost, less), bruteWindows(a, k, rightmost, less), "projected sliding k=" + show(k) + " rightmost=" + show(rightmost));}}
        vector<string> words{"z", "a", "a", "foo", "b", "a"}; context = "strings=" + show(words);
        checkEqual(previousGreater(words), bruteNearest(words, true, true, std::greater<string>{}), "string previousGreater");
        checkEqual(nextSmaller(words, false), bruteNearest(words, false, false, std::less<string>{}), "string nextSmaller nonstrict");
        checkEqual(slidingMaximum(words, 3, true), bruteWindows(words, 3, true, std::greater<string>{}), "string slidingMaximum");
        std::cout << "PASS generic comparators and nonidentical equivalent payloads\n";}

    void randomized(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 100 : 500;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = int(rng() % 150); vector<lng> a(n);
            for (lng &x : a) { x = lng(rng() % 201) - 100; }
            boundaries(a, false);
            for (lng &x : a) { x = lng(rng() & ulng(std::numeric_limits<lng>::max())); }
            checkHistogram(a, bruteHistogram(a));
            int rows = int(rng() % 10), cols = int(rng() % 10);
            vector<vector<int>> b(rows, vector<int>(cols));
            for (auto &row : b) { for (int &x : row) { x = int(rng() & 1); } }
            matrix(b);
            for (auto &row : b) {
                for (int &x : row) { if (x) { x = rng() & 1 ? INT_MIN : INT_MAX; } }}
            context = "legacy integer blockers=" + show(b);
            checkEqual(maxZeroSubmatrix(b), bruteMatrix(b, 0).area, "legacy nonbinary barriers");}
        boundaries({std::numeric_limits<lng>::min(), 0, std::numeric_limits<lng>::max(), std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()});
        boundaries(vector<lng>(80, -7));
        vector<lng> increasing(120); std::iota(increasing.begin(), increasing.end(), lng(-60)); boundaries(increasing, false);
        std::reverse(increasing.begin(), increasing.end()); boundaries(increasing, false);
        for (vector<lng> a : {vector<lng>{2, 1, 5, 6, 2, 3}, vector<lng>{2, 2, 2}, vector<lng>{0, 4, 0, 4},
                             vector<lng>{std::numeric_limits<lng>::max(), std::numeric_limits<lng>::max()}}) {
            checkHistogram(a, bruteHistogram(a));}
        matrix({{1, 1, 0, 1}, {1, 1, 0, 1}, {0, 0, 1, 1}});
        int n = mode == "quick" ? 10000 : mode == "full" ? 100000 : 1000000;
        lng height = std::numeric_limits<lng>::max();
        checkHistogram(vector<lng>(n, height), HistogramRectangle{lll(n) * height, 0, n, height});
        std::cout << "PASS seeded arrays/matrices, full-width heights, signed extrema and plateaus\n";}

    void invalid(const string &name) {
        vector<lng> a{1, 2, 3};
        if (name == "minimum-zero-window") { (void)slidingMinimum(a, 0); }
        else if (name == "minimum-negative-window") { (void)slidingMinimum(a, -1); }
        else if (name == "maximum-zero-window") { (void)slidingMaximum(a, 0); }
        else if (name == "negative-height") { (void)largestHistogramRectangle({-1}); }
        else if (name == "late-negative-height") { (void)largestHistogramRectangle({1, 2, -1}); }
        else if (name == "binary-value") { (void)largestBinaryRectangle({{0}}, 2); }
        else if (name == "binary-negative-cell") { (void)largestBinaryRectangle({{-1}}); }
        else if (name == "binary-large-cell") { (void)largestBinaryRectangle({{2}}); }
        else if (name == "binary-short-row") { (void)largestBinaryRectangle({{0, 1}, {0}}); }
        else if (name == "binary-long-row") { (void)largestBinaryRectangle({{}, {0}}); }
        else if (name == "legacy-ragged") { (void)maxZeroSubmatrix({{0}, {0, 1}}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else if (arg == "--invalid" && i + 1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "monotone_stack seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        std::mt19937_64 rng(seed);
        exhaustive(); genericOrder(); randomized(rng);
        std::cout << "PASS monotone_stack checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL monotone_stack seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
