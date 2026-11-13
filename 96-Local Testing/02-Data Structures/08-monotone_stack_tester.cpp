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
        // n * k = 2^31: a window length whose product with n leaves int, over equal values whose leftmost and rightmost minima are known.
        context = "half-length window n=65536 k=32768";
        vector<int> flat(65536), left(32769), right(32769);
        std::iota(left.begin(), left.end(), 0); std::iota(right.begin(), right.end(), 32767);
        checkEqual(slidingMinimum(flat, 32768), left, "half-length window leftmost"); checkEqual(slidingMinimum(flat, 32768, true), right, "half-length window rightmost");
        for (vector<lng> a : {vector<lng>{2, 1, 5, 6, 2, 3}, vector<lng>{2, 2, 2}, vector<lng>{0, 4, 0, 4},
                             vector<lng>{std::numeric_limits<lng>::max(), std::numeric_limits<lng>::max()}}) {
            checkHistogram(a, bruteHistogram(a));}
        matrix({{1, 1, 0, 1}, {1, 1, 0, 1}, {0, 0, 1, 1}});
        int n = mode == "quick" ? 10000 : mode == "full" ? 100000 : 1000000;
        lng height = std::numeric_limits<lng>::max();
        checkHistogram(vector<lng>(n, height), HistogramRectangle{lll(n) * height, 0, n, height});
        std::cout << "PASS seeded arrays/matrices, full-width heights, signed extrema and plateaus\n";}

    vector<vector<lll>> histogramRows(const vector<HistogramRectangle> &v) {
        vector<vector<lll>> res;
        for (auto &x : v) { res.push_back({x.area, x.l, x.r, x.height}); }
        return res;}
    vector<vector<lll>> bruteMaximalHistogram(const vector<lng> &a) {
        int n = int(a.size()); vector<vector<lll>> res;
        for (int r = 1; r <= n; ++r) {
            lng height = std::numeric_limits<lng>::max();
            for (int l = r - 1; l >= 0; --l) {
                height = std::min(height, a[l]);
                if (height > 0 && (l == 0 || a[l - 1] < height) && (r == n || a[r] < height)) { res.push_back({lll(height) * (r - l), l, r, height}); }}}
        return res;}
    void checkMaximalHistogram(const vector<lng> &a, const vector<vector<lll>> &want) {
        context = "maximal histogram=" + show(a);
        auto got = maximalHistogramRectangles(a); lll best = 0;
        checkEqual(histogramRows(got), want, "maximalHistogramRectangles");
        for (auto &x : got) { best = std::max(best, x.area); }
        checkEqual(best, largestHistogramRectangle(a).area, "maximal histogram list holds the largest");}

    vector<vector<lng>> binaryRows(const vector<BinaryRectangle> &v) {
        vector<vector<lng>> res;
        for (auto &x : v) { res.push_back({x.area, x.top, x.left, x.bottom, x.right}); }
        return res;}
    struct Census {
        vector<vector<lng>> table;
        lng total = 0, squares = 0;
        BinaryRectangle square;
        vector<BinaryRectangle> maximal;
    };
    Census bruteCensus(const vector<vector<int>> &a, int value) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        vector<vector<int>> pre(n + 1, vector<int>(m + 1));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) { pre[i + 1][j + 1] = pre[i][j + 1] + pre[i + 1][j] - pre[i][j] + (a[i][j] == value); }}
        auto full = [&](int top, int left, int bottom, int right) {
            return pre[bottom][right] - pre[top][right] - pre[bottom][left] + pre[top][left] == (bottom - top) * (right - left);};
        Census res; res.table.assign(n, vector<lng>(m));
        for (int top = 0; top < n; ++top) {
            for (int bottom = top + 1; bottom <= n; ++bottom) {
                for (int left = 0; left < m; ++left) {
                    for (int right = left + 1; right <= m; ++right) {
                        if (!full(top, left, bottom, right)) { continue; }
                        int h = bottom - top, w = right - left; ++res.table[h - 1][w - 1]; ++res.total;
                        if (h == w) { ++res.squares; }
                        if (h == w && lng(h) * w > res.square.area) { res.square = {lng(h) * w, top, left, bottom, right}; }
                        if ((!top || !full(top - 1, left, bottom, right)) && (bottom == n || !full(top, left, bottom + 1, right))
                            && (!left || !full(top, left - 1, bottom, right)) && (right == m || !full(top, left, bottom, right + 1))) {
                            res.maximal.push_back({lng(h) * w, top, left, bottom, right});}}}}}
        std::ranges::sort(res.maximal, [](const BinaryRectangle &x, const BinaryRectangle &y) {
            return std::tie(x.bottom, x.right, x.top) < std::tie(y.bottom, y.right, y.top);});
        return res;}
    lng directCount(const vector<vector<int>> &a, int value) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0; lng res = 0;
        for (int top = 0; top < n; ++top) {
            for (int bottom = top + 1; bottom <= n; ++bottom) {
                for (int left = 0; left < m; ++left) {
                    for (int right = left + 1; right <= m; ++right) {
                        bool good = true;
                        for (int i = top; i < bottom && good; ++i) {
                            for (int j = left; j < right; ++j) { good = good && a[i][j] == value; }}
                        res += good;}}}}
        return res;}

    vector<vector<int>> transposed(const vector<vector<int>> &a) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        vector<vector<int>> res(m, vector<int>(n));
        for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) { res[j][i] = a[i][j]; } }
        return res;}
    std::pair<BinaryRectangle, BinaryRectangle> bruteCheckerboard(const vector<vector<int>> &a) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        vector<vector<int>> across(n + 1, vector<int>(m + 1)), down(n + 1, vector<int>(m + 1));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                int same_right = j + 1 < m && a[i][j] == a[i][j + 1], same_below = i + 1 < n && a[i][j] == a[i + 1][j];
                across[i + 1][j + 1] = across[i][j + 1] + across[i + 1][j] - across[i][j] + same_right;
                down[i + 1][j + 1] = down[i][j + 1] + down[i + 1][j] - down[i][j] + same_below;}}
        auto count = [](const vector<vector<int>> &p, int top, int left, int bottom, int right) {
            return bottom <= top || right <= left ? 0 : p[bottom][right] - p[top][right] - p[bottom][left] + p[top][left];};
        BinaryRectangle square, rect;
        for (int top = 0; top < n; ++top) {
            for (int left = 0; left < m; ++left) {
                for (int bottom = top + 1; bottom <= n; ++bottom) {
                    for (int right = left + 1; right <= m; ++right) {
                        if (count(across, top, left, bottom, right - 1) || count(down, top, left, bottom - 1, right)) { break; }
                        int h = bottom - top, w = right - left; lng area = lng(h) * w;
                        if (area > rect.area) { rect = {area, top, left, bottom, right}; }
                        if (h == w && area > square.area) { square = {area, top, left, bottom, right}; }}}}}
        return {square, rect};}
    HistogramRectangle bruteMinSum(const vector<lng> &a) {
        HistogramRectangle ans; int n = int(a.size());
        for (int l = 0; l < n; ++l) {
            lng low = a[l]; lll sum = 0;
            for (int r = l + 1; r <= n; ++r) {
                low = std::min(low, a[r - 1]); sum += a[r - 1]; lll v = low * sum;
                if (v > ans.area) { ans = {v, l, r, low}; }}}
        return ans;}
    void checkMinSum(const vector<lng> &a, const HistogramRectangle &want) {
        context = "min-sum array=" + show(a);
        auto got = maxMinTimesSum(a); vector<lng> back(a.rbegin(), a.rend());
        checkEqual(histogramRows({got}), histogramRows({want}), "maxMinTimesSum");
        checkEqual(maxMinTimesSum(back).area, got.area, "maxMinTimesSum reversal keeps the value");}
    BinaryRectangle bruteMinSum2D(const vector<vector<int>> &a) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0; BinaryRectangle ans;
        for (int top = 0; top < n; ++top) {
            vector<vector<lng>> low(n + 1, vector<lng>(m, LLONG_MAX)), sum(n + 1, vector<lng>(m));
            for (int bottom = top + 1; bottom <= n; ++bottom) {
                for (int j = 0; j < m; ++j) {
                    low[bottom][j] = std::min(low[bottom - 1][j], lng(a[bottom - 1][j]));
                    sum[bottom][j] = sum[bottom - 1][j] + a[bottom - 1][j];}}
            for (int left = 0; left < m; ++left) {
                for (int bottom = top + 1; bottom <= n; ++bottom) {
                    lng lo = LLONG_MAX; lll s = 0;
                    for (int right = left + 1; right <= m; ++right) {
                        lo = std::min(lo, low[bottom][right - 1]); s += sum[bottom][right - 1]; lll v = lo * s;
                        if (v > ans.area) { ans = {lng(v), top, left, bottom, right}; }}}}}
        return ans;}
    void checkMinSum2D(const vector<vector<int>> &a) {
        context = "min-sum matrix=" + show(a); int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        auto got = maxMinTimesSum2D(a);
        sameRectangle(got, bruteMinSum2D(a), "maxMinTimesSum2D");
        checkEqual(maxMinTimesSum2D(transposed(a)).area, got.area, "maxMinTimesSum2D transpose keeps the value");
        if (n == 1) {
            auto row = maxMinTimesSum(vector<lng>(a[0].begin(), a[0].end()));
            sameRectangle(got, row.area ? BinaryRectangle{lng(row.area), 0, row.l, 1, row.r} : BinaryRectangle{}, "one-row matrix matches maxMinTimesSum");}
        if (m == 1) {
            vector<lng> column(n);
            for (int i = 0; i < n; ++i) { column[i] = a[i][0]; }
            auto col = maxMinTimesSum(column);
            sameRectangle(got, col.area ? BinaryRectangle{lng(col.area), col.l, 0, col.r, 1} : BinaryRectangle{}, "one-column matrix matches maxMinTimesSum");}}
    vector<lng> areaBounds(const vector<vector<int>> &a) {
        lng cells = a.empty() ? 0 : lng(a.size()) * lng(a[0].size());
        vector<lng> res{LLONG_MIN, -1, LLONG_MAX};
        if (cells <= 16) { for (lng k = 0; k <= cells + 1; ++k) { res.push_back(k); } }
        else { for (lng k : {lng(0), lng(1), lng(2), lng(3), lng(5), lng(7), cells / 5, cells / 3, cells / 2, 2 * cells / 3, cells - 1, cells, cells + 1}) { res.push_back(k); } }
        return res;}
    void binaryExtras(const vector<vector<int>> &a) {
        context = "binary extras matrix=" + show(a);
        auto [square, rect] = bruteCheckerboard(a);
        vector<vector<int>> complement = a;
        for (auto &row : complement) { for (int &x : row) { x ^= 1; } }
        sameRectangle(largestCheckerboardSquare(a), square, "largestCheckerboardSquare");
        sameRectangle(largestCheckerboardRectangle(a), rect, "largestCheckerboardRectangle");
        sameRectangle(largestCheckerboardSquare(complement), square, "checkerboard square complement symmetry");
        sameRectangle(largestCheckerboardRectangle(complement), rect, "checkerboard rectangle complement symmetry");
        sameRectangle(maxMinTimesSum2D(a), largestBinaryRectangle(a, 1), "maxMinTimesSum2D on 0/1 cells is the largest ones rectangle");
        checkMinSum2D(a);}

    void identities(const vector<vector<int>> &a) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        vector<vector<int>> complement = a;
        for (auto &row : complement) { for (int &x : row) { x ^= 1; } }
        for (int value : {0, 1}) {
            string suffix = " value=" + show(value);
            auto table = countBinaryRectangles(a, value); lng sum = 0, diagonal = 0; int side = 0;
            checkEqual(int(table.size()), n, "table rows" + suffix);
            for (int h = 1; h <= n; ++h) {
                checkEqual(int(table[h - 1].size()), m, "table row width" + suffix);
                for (int w = 1; w <= m; ++w) {
                    sum += table[h - 1][w - 1];
                    if (h == w) { diagonal += table[h - 1][w - 1]; }
                    if (h == w && table[h - 1][w - 1] > 0) { side = h; }}}
            checkEqual(sum, countBinaryRectanglesTotal(a, value), "table sum is the total" + suffix);
            checkEqual(diagonal, countBinarySquares(a, value), "table diagonal is the square count" + suffix);
            auto square = largestBinarySquare(a, value); auto maximal = maximalBinaryRectangles(a, value); lng best = 0;
            checkEqual(square.area, lng(side) * side, "largest square side from the table" + suffix);
            for (auto &x : maximal) {
                checkEqual(x.area, lng(x.bottom - x.top) * (x.right - x.left), "maximal witness area" + suffix);
                best = std::max(best, x.area);}
            checkEqual(best, largestBinaryRectangle(a, value).area, "maximal list holds the largest" + suffix);
            checkEqual(binaryRows(maximalBinaryRectangles(complement, 1 - value)), binaryRows(maximal), "maximal complement symmetry" + suffix);
            sameRectangle(largestBinarySquare(complement, 1 - value), square, "square complement symmetry" + suffix);
            checkEqual(countBinarySquares(complement, 1 - value), diagonal, "square count complement symmetry" + suffix);
            checkEqual(countBinaryRectangles(complement, 1 - value) == table, true, "table complement symmetry" + suffix);
            checkEqual(countBinaryRectanglesTotal(complement, 1 - value), sum, "total complement symmetry" + suffix);
            lng cells = lng(n) * m;
            checkEqual(countBinaryRectanglesAreaAtMost(a, cells, value), sum, "area bound n*m gives the total" + suffix);
            checkEqual(countBinaryRectanglesAreaAtMost(a, LLONG_MAX, value), sum, "huge area bound gives the total" + suffix);
            checkEqual(countBinaryRectanglesAreaAtMost(a, 0, value), lng(0), "area bound 0 gives none" + suffix);
            checkEqual(countBinaryRectanglesAreaAtMost(a, 1, value), n && m ? table[0][0] : 0, "area bound 1 counts cells" + suffix);
            checkEqual(countBinaryRectanglesAreaAtMost(complement, cells / 2, 1 - value), countBinaryRectanglesAreaAtMost(a, cells / 2, value), "area bound complement symmetry" + suffix);}
        checkEqual(countBinaryRectanglesAreaAtMost(a, lng(n) * m / 2), countBinaryRectanglesAreaAtMost(a, lng(n) * m / 2, 1), "default area bound bit");
        checkEqual(binaryRows(maximalBinaryRectangles(a)), binaryRows(maximalBinaryRectangles(a, 1)), "default maximal bit");
        sameRectangle(largestBinarySquare(a), largestBinarySquare(a, 1), "default square bit");
        checkEqual(countBinarySquares(a), countBinarySquares(a, 1), "default square count bit");
        checkEqual(countBinaryRectangles(a) == countBinaryRectangles(a, 1), true, "default table bit");
        checkEqual(countBinaryRectanglesTotal(a), countBinaryRectanglesTotal(a, 1), "default total bit");}
    void families(const vector<vector<int>> &a) {
        context = "families matrix=" + show(a);
        for (int value : {0, 1}) {
            string suffix = " value=" + show(value); Census want = bruteCensus(a, value);
            checkEqual(binaryRows(maximalBinaryRectangles(a, value)), binaryRows(want.maximal), "maximalBinaryRectangles" + suffix);
            sameRectangle(largestBinarySquare(a, value), want.square, "largestBinarySquare" + suffix);
            checkEqual(countBinarySquares(a, value), want.squares, "countBinarySquares" + suffix);
            checkEqual(countBinaryRectangles(a, value), want.table, "countBinaryRectangles" + suffix);
            checkEqual(countBinaryRectanglesTotal(a, value), want.total, "countBinaryRectanglesTotal" + suffix);
            for (lng k : areaBounds(a)) {
                lng expect = 0;
                for (int h = 1; h <= int(want.table.size()); ++h) {
                    for (int w = 1; w <= int(want.table[h - 1].size()); ++w) { expect += lng(h) * w <= k ? want.table[h - 1][w - 1] : 0; }}
                checkEqual(countBinaryRectanglesAreaAtMost(a, k, value), expect, "countBinaryRectanglesAreaAtMost k=" + show(k) + suffix);}}
        identities(a); binaryExtras(a);}

    void exhaustiveFamilies() {
        int bound = mode == "quick" ? 4 : mode == "full" ? 7 : 9;
        for (int n = 0, count = 1; n <= bound; ++n, count *= 4) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 4; x /= 4; }
                checkMaximalHistogram(a, bruteMaximalHistogram(a));}}
        std::cout << "PASS exhaustive maximal histogram rectangles and their order\n";
        int cells = mode == "quick" ? 9 : mode == "full" ? 12 : 16;
        for (int n = 0; n <= 4; ++n) {
            for (int m = 0; m <= 4; ++m) {
                if (n * m > cells) { continue; }
                for (int code = 0; code < (1 << (n * m)); ++code) {
                    vector<vector<int>> a(n, vector<int>(m)); int x = code;
                    for (auto &row : a) { for (int &v : row) { v = x & 1; x >>= 1; } }
                    families(a);
                    for (int value : {0, 1}) {
                        Census want = bruteCensus(a, value); lng best = 0;
                        for (auto &rect : want.maximal) { best = std::max(best, rect.area); }
                        context = "census validation matrix=" + show(a) + " value=" + show(value);
                        checkEqual(want.total, directCount(a, value), "census total against cell scan");
                        checkEqual(best, bruteMatrix(a, value).area, "census maximal list against bruteMatrix");}}}}
        std::cout << "PASS exhaustive maximal rectangles, squares, by-size and total counts, both values\n";}

    void randomFamilies(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 100 : 500;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = int(rng() % 150); vector<lng> a(n);
            for (lng &x : a) { x = trial % 2 ? lng(rng() % 5) : lng(rng() & ulng(std::numeric_limits<lng>::max())); }
            checkMaximalHistogram(a, bruteMaximalHistogram(a));
            int rows = int(rng() % 31), cols = int(rng() % 31); ulng mask = (ulng(1) << (1 + rng() % 4)) - 1;
            vector<vector<int>> b(rows, vector<int>(cols));
            for (auto &row : b) { for (int &x : row) { x = (rng() & mask) != 0; } }
            families(b);}
        int k = 25; vector<vector<int>> stair(k, vector<int>(k)), board(30, vector<int>(30));
        for (int i = 0; i < k; ++i) { for (int j = 0; j < k; ++j) { stair[i][j] = i + j < k; } }
        for (int i = 0; i < 30; ++i) { for (int j = 0; j < 30; ++j) { board[i][j] = (i + j) % 2; } }
        families(stair); families(board);
        std::cout << "PASS seeded maximal histograms and matrices up to 30x30 against the census oracle\n";}

    void scale() {
        int n = mode == "quick" ? 120 : 2000, m = mode == "quick" ? 90 : 1500, s = min(n, m);
        vector<vector<int>> ones(n, vector<int>(m, 1));
        context = "all ones " + show(n) + "x" + show(m);
        vector<vector<lng>> table(n, vector<lng>(m)), zero = table; lng squares = 0;
        for (int h = 1; h <= n; ++h) {
            for (int w = 1; w <= m; ++w) { table[h - 1][w - 1] = lng(n - h + 1) * (m - w + 1); }}
        for (int k = 1; k <= s; ++k) { squares += lng(n - k + 1) * (m - k + 1); }
        checkEqual(countBinaryRectangles(ones) == table, true, "all-ones table closed form");
        checkEqual(countBinaryRectanglesTotal(ones), lng(n) * (n + 1) / 2 * (lng(m) * (m + 1) / 2), "all-ones total closed form");
        checkEqual(countBinarySquares(ones), squares, "all-ones squares closed form");
        sameRectangle(largestBinarySquare(ones), {lng(s) * s, 0, 0, s, s}, "all-ones square");
        checkEqual(binaryRows(maximalBinaryRectangles(ones)), vector<vector<lng>>{{lng(n) * m, 0, 0, n, m}}, "all-ones single maximal rectangle");
        checkEqual(countBinaryRectangles(ones, 0) == zero, true, "all-ones zero table");
        checkEqual(countBinaryRectanglesTotal(ones, 0), lng(0), "all-ones zero total");
        checkEqual(countBinarySquares(ones, 0), lng(0), "all-ones zero squares");
        sameRectangle(largestBinarySquare(ones, 0), BinaryRectangle{}, "all-ones zero square sentinel");
        checkEqual(maximalBinaryRectangles(ones, 0).empty(), true, "all-ones no zero maximal rectangle");
        identities(ones);
        int k = mode == "quick" ? 60 : 2000, side = (k + 1) / 2;
        vector<vector<int>> stair(k, vector<int>(k));
        for (int i = 0; i < k; ++i) { for (int j = 0; j < k; ++j) { stair[i][j] = i + j < k; } }
        context = "staircase " + show(k);
        vector<vector<lng>> maximal, steps(k, vector<lng>(k)); lng total = 0, count = 0;
        for (int b = 1; b <= k; ++b) { maximal.push_back({lng(b) * (k - b + 1), 0, 0, b, k - b + 1}); }
        for (int h = 1; h <= k; ++h) {
            for (int w = 1; w <= k; ++w) {
                lng t = k - h - w + 1; steps[h - 1][w - 1] = t >= 0 ? (t + 1) * (t + 2) / 2 : 0;
                total += steps[h - 1][w - 1];}}
        for (int d = 1; 2 * d <= k + 1; ++d) { lng t = k - 2 * d + 1; count += (t + 1) * (t + 2) / 2; }
        checkEqual(binaryRows(maximalBinaryRectangles(stair)), maximal, "staircase maximal rectangles");
        checkEqual(countBinaryRectangles(stair) == steps, true, "staircase table closed form");
        checkEqual(countBinaryRectanglesTotal(stair), total, "staircase total");
        checkEqual(countBinarySquares(stair), count, "staircase squares closed form");
        sameRectangle(largestBinarySquare(stair), {lng(side) * side, 0, 0, side, side}, "staircase square");
        identities(stair);
        int len = mode == "quick" ? 10000 : mode == "full" ? 100000 : 1000000, rise = min(len, 100000);
        lng height = std::numeric_limits<lng>::max();
        checkMaximalHistogram(vector<lng>(len, height), {{lll(len) * height, 0, len, height}});
        vector<lng> rising(rise); vector<vector<lll>> want; std::iota(rising.begin(), rising.end(), lng(1));
        for (int h = rise; h >= 1; --h) { want.push_back({lll(rise - h + 1) * h, h - 1, rise, h}); }
        checkMaximalHistogram(rising, want);
        std::cout << "PASS closed-form all-ones and staircase matrices, plateau and rising histograms\n";}

    template<typename T>
    lng bruteDistanceSum(const vector<T> &a, bool strict) {
        int n = int(a.size()); lng res = 0;
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n && (strict ? !(a[i] < a[j]) : a[j] < a[i]); ++j) { ++res; }}
        return res;}
    void distanceSums(const vector<lng> &a) {
        context = "distance array=" + show(a);
        for (bool strict : {false, true}) {
            checkEqual(nextGreaterDistanceSum(a, strict), bruteDistanceSum(a, strict), "nextGreaterDistanceSum strict=" + show(strict));}
        checkEqual(nextGreaterDistanceSum(a), bruteDistanceSum(a, true), "default distance strict");}
    void slidingDefaults(const vector<lng> &a) {
        context = "default tie array=" + show(a);
        for (int k = 1; k <= int(a.size()) + 1; ++k) {
            checkEqual(slidingMinimum(a, k), bruteWindows(a, k, false, std::less<lng>{}), "default slidingMinimum leftmost k=" + show(k));
            checkEqual(slidingMaximum(a, k), bruteWindows(a, k, false, std::greater<lng>{}), "default slidingMaximum leftmost k=" + show(k));}}

    void exhaustiveAdopted() {
        int bound = mode == "quick" ? 4 : mode == "full" ? 7 : 9;
        for (int n = 0, count = 1; n <= bound; ++n, count *= 3) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 3 - 1; x /= 3; }
                distanceSums(a); slidingDefaults(a);}}
        // Values 0..4 (full and stress) reach tie shapes such as {2, 4, 1, 4, 1, 4} that 0..3 cannot.
        for (int base : {4, 5}) {
            int length = base == 4 ? bound : mode == "quick" ? -1 : bound - 1;
            for (int n = 0, count = 1; n <= length; ++n, count *= base) {
                for (int code = 0; code < count; ++code) {
                    vector<lng> a(n); int x = code;
                    for (lng &v : a) { v = x % base; x /= base; }
                    checkMinSum(a, bruteMinSum(a));}}}
        std::cout << "PASS exhaustive next-greater distance sums, default leftmost windows and max min * sum\n";
        int cells = mode == "quick" ? 6 : mode == "full" ? 8 : 9;
        for (int n = 0; n <= 4; ++n) {
            for (int m = 0; m <= 4; ++m) {
                if (n * m > cells) { continue; }
                int count = 1;
                for (int i = 0; i < n * m; ++i) { count *= 3; }
                for (int code = 0; code < count; ++code) {
                    vector<vector<int>> a(n, vector<int>(m)); int x = code;
                    for (auto &row : a) { for (int &v : row) { v = x % 3; x /= 3; } }
                    checkMinSum2D(a);}}}
        std::cout << "PASS exhaustive 0..2 matrices for max min * sum 2D, rows, columns and transposes\n";}

    void randomAdopted(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 100 : 500;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = int(rng() % 150); vector<lng> a(n);
            for (lng &x : a) { x = lng(rng() % 7) - 3; }
            distanceSums(a);
            for (lng &x : a) { x = trial % 2 ? lng(rng() % 6) : lng(rng() & ((ulng(1) << 59) - 1)); }
            checkMinSum(a, bruteMinSum(a));
            int rows = int(rng() % 31), cols = int(rng() % 31), top = trial % 3 == 0 ? 3 : trial % 3 == 1 ? 1000000 : 100000000;
            vector<vector<int>> b(rows, vector<int>(cols)), board = b;
            for (auto &row : b) { for (int &x : row) { x = int(rng() % ulng(top + 1)); } }
            checkMinSum2D(b);
            int phase = int(rng() & 1), flips = int(1 + rng() % 40);
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) { board[i][j] = ((i + j + phase) & 1) ^ int(rng() % ulng(flips) == 0); }}
            binaryExtras(board);}
        for (vector<lng> a : {vector<lng>{}, vector<lng>{0}, vector<lng>{0, 0}, vector<lng>{3, 0, 3}, vector<lng>{1, 0, 0, 1}, vector<lng>{2, 2, 1, 4},
                             vector<lng>{LLONG_MAX}, vector<lng>{LLONG_MAX, LLONG_MAX}, vector<lng>{LLONG_MAX, 0, LLONG_MAX},
                             vector<lng>{2, 4, 1, 4, 1, 4}, vector<lng>{LLONG_MAX, LLONG_MAX, 2}}) {
            checkMinSum(a, bruteMinSum(a));}
        for (vector<vector<int>> a : {vector<vector<int>>{}, vector<vector<int>>{{}, {}}, vector<vector<int>>{{0, 0}, {0, 0}}, vector<vector<int>>{{INT_MAX, INT_MAX}},
                                     vector<vector<int>>{{INT_MAX}, {INT_MAX}}, vector<vector<int>>{{INT_MAX, 0}, {0, INT_MAX}}}) {
            checkMinSum2D(a);}
        context = "boundary 2D min-sum {{INT_MAX, INT_MAX}}";
        sameRectangle(maxMinTimesSum2D({{INT_MAX, INT_MAX}}), {lng(INT_MAX) * (2 * lng(INT_MAX)), 0, 0, 1, 2}, "largest admissible 2D product");
        context = "boundary min-sum {LLONG_MAX, LLONG_MAX}";
        checkEqual(maxMinTimesSum({LLONG_MAX, LLONG_MAX}).area, lll(LLONG_MAX) * (2 * lll(LLONG_MAX)), "largest admissible product");
        vector<string> words{"b", "a", "c", "c", "a", "b", "a"}; context = "strings=" + show(words);
        checkEqual(nextGreaterDistanceSum(words), bruteDistanceSum(words, true), "string distance strict");
        checkEqual(nextGreaterDistanceSum(words, false), bruteDistanceSum(words, false), "string distance nonstrict");
        checkEqual(slidingMinimum(words, 3), bruteWindows(words, 3, false, std::less<string>{}), "string default slidingMinimum");
        checkEqual(slidingMaximum(words, 2), bruteWindows(words, 2, false, std::greater<string>{}), "string default slidingMaximum");
        std::cout << "PASS seeded distance sums, max min * sum 1D/2D up to 30x30, near-checkerboards and product bounds\n";}

    void scaleAdopted() {
        int len = 100000; lng pairs = lng(len) * (len - 1) / 2;
        vector<lng> flat(len, 5), rise(len); std::iota(rise.begin(), rise.end(), lng(1));
        vector<lng> fall(rise.rbegin(), rise.rend());
        context = "distance scale n=" + show(len);
        checkEqual(nextGreaterDistanceSum(flat), pairs, "flat strict distance");
        checkEqual(nextGreaterDistanceSum(flat, false), lng(0), "flat nonstrict distance");
        checkEqual(nextGreaterDistanceSum(rise) + nextGreaterDistanceSum(rise, false), lng(0), "rising distance");
        checkEqual(nextGreaterDistanceSum(fall), pairs, "falling strict distance");
        checkEqual(nextGreaterDistanceSum(fall, false), pairs, "falling nonstrict distance");
        lng c = 1000000000; HistogramRectangle best;
        checkMinSum(vector<lng>(len, c), {lll(c) * c * len, 0, len, c});
        for (int l = 0; l < len; ++l) {
            lll v = lll(l + 1) * (lll(len) * (len + 1) / 2 - lll(l) * (l + 1) / 2);
            if (v > best.area) { best = {v, l, len, l + 1}; }}
        checkMinSum(rise, best);
        int n = mode == "quick" ? 40 : 200, m = mode == "quick" ? 30 : 150;
        context = "2D all-equal " + show(n) + "x" + show(m);
        sameRectangle(maxMinTimesSum2D(vector<vector<int>>(n, vector<int>(m, 1000))), {lng(1000000) * n * m, 0, 0, n, m}, "all-equal 2D whole matrix");
        n = mode == "quick" ? 120 : 2000; m = mode == "quick" ? 90 : 1500; int s = min(n, m);
        vector<vector<int>> board(n, vector<int>(m)), rows = board, cols = board, ones(n, vector<int>(m, 1));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) { board[i][j] = (i + j) & 1; rows[i][j] = i & 1; cols[i][j] = j & 1; }}
        context = "checkerboard scale " + show(n) + "x" + show(m);
        for (int phase : {0, 1}) {
            sameRectangle(largestCheckerboardRectangle(board), {lng(n) * m, 0, 0, n, m}, "perfect board rectangle phase=" + show(phase));
            sameRectangle(largestCheckerboardSquare(board), {lng(s) * s, 0, 0, s, s}, "perfect board square phase=" + show(phase));
            for (auto &row : board) { for (int &x : row) { x ^= 1; } }}
        sameRectangle(largestCheckerboardRectangle(rows), {n, 0, 0, n, 1}, "row stripes give one column");
        sameRectangle(largestCheckerboardRectangle(cols), {m, 0, 0, 1, m}, "column stripes give one row");
        sameRectangle(largestCheckerboardSquare(rows), {1, 0, 0, 1, 1}, "row stripes square");
        sameRectangle(largestCheckerboardSquare(cols), {1, 0, 0, 1, 1}, "column stripes square");
        sameRectangle(largestCheckerboardRectangle(ones), {1, 0, 0, 1, 1}, "constant matrix rectangle");
        sameRectangle(largestCheckerboardSquare(ones), {1, 0, 0, 1, 1}, "constant matrix square");
        for (const vector<vector<int>> &a : {vector<vector<int>>{}, vector<vector<int>>{{}, {}}}) {
            sameRectangle(largestCheckerboardRectangle(a), BinaryRectangle{}, "empty matrix rectangle");
            sameRectangle(largestCheckerboardSquare(a), BinaryRectangle{}, "empty matrix square");}
        context = "area bound all ones " + show(n) + "x" + show(m);
        lng k = lng(n) * m / 7, expect = 0;
        for (int h = 1; h <= n; ++h) {
            for (int w = 1; w <= m && lng(h) * w <= k; ++w) { expect += lng(n - h + 1) * (m - w + 1); }}
        checkEqual(countBinaryRectanglesAreaAtMost(ones, k), expect, "all-ones area bound n*m/7");
        checkEqual(countBinaryRectanglesAreaAtMost(ones, lng(n) * m), lng(n) * (n + 1) / 2 * (lng(m) * (m + 1) / 2), "all-ones area bound n*m");
        checkEqual(countBinaryRectanglesAreaAtMost(ones, k, 0), lng(0), "all-ones zero cells");
        std::cout << "PASS closed-form distance sums, min * sum plateaus, checkerboards, stripes and area bounds\n";}

    struct Reached : std::runtime_error {
        Reached() : std::runtime_error("comparator reached past the size assert") {}
    };
    struct Throwing {
        bool operator()(bool, bool) const { throw Reached(); }
    };
    // Packed bools keep the largest admitted windows cheap; the comparator stops the scan at its first call, before any further work.
    void sizeLimits() {
        if (mode == "quick") { return; }
        context = "largest admitted sliding windows";
        for (int n : {INT_MAX, 1 << 30}) {
            bool reached = false;
            try { (void)slidingMinimum(vector<bool>(size_t(n)), n, false, Throwing{}); } catch (const Reached &) { reached = true; }
            checkEqual(reached, true, "slidingMinimum n = k = " + show(n) + " passes its assert and its reserve");}
        std::cout << "PASS largest admitted sliding windows\n";}

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
        else if (name == "legacy-short-row") { (void)maxZeroSubmatrix({{0, 0}, {0}}); }
        else if (name == "maximal-histogram-negative") { (void)maximalHistogramRectangles({-1}); }
        else if (name == "maximal-histogram-late-negative") { (void)maximalHistogramRectangles({1, 0, -1}); }
        else if (name == "maximal-binary-value") { (void)maximalBinaryRectangles({{0}}, 2); }
        else if (name == "maximal-binary-cell") { (void)maximalBinaryRectangles({{0, 2}}); }
        else if (name == "maximal-binary-ragged") { (void)maximalBinaryRectangles({{0, 1}, {1}}); }
        else if (name == "square-value") { (void)largestBinarySquare({{1}}, 2); }
        else if (name == "square-cell") { (void)largestBinarySquare({{-1}}); }
        else if (name == "square-ragged") { (void)largestBinarySquare({{1}, {1, 1}}); }
        else if (name == "count-squares-value") { (void)countBinarySquares({{1}}, -1); }
        else if (name == "count-squares-cell") { (void)countBinarySquares({{1, 3}}); }
        else if (name == "count-squares-ragged") { (void)countBinarySquares({{1, 1}, {1}}); }
        else if (name == "count-rectangles-value") { (void)countBinaryRectangles({{1}}, 2); }
        else if (name == "count-rectangles-cell") { (void)countBinaryRectangles({{2}}); }
        else if (name == "count-rectangles-ragged") { (void)countBinaryRectangles({{}, {1}}); }
        else if (name == "total-value") { (void)countBinaryRectanglesTotal({{0}}, 2); }
        else if (name == "total-cell") { (void)countBinaryRectanglesTotal({{0, -1}}); }
        else if (name == "total-ragged") { (void)countBinaryRectanglesTotal({{0}, {0, 0}}); }
        else if (name == "binary-value-negative") { (void)largestBinaryRectangle({{0}}, -1); }
        else if (name == "maximal-binary-value-negative") { (void)maximalBinaryRectangles({{0}}, -1); }
        else if (name == "square-value-negative") { (void)largestBinarySquare({{1}}, -1); }
        else if (name == "count-squares-value-two") { (void)countBinarySquares({{1}}, 2); }
        else if (name == "count-rectangles-value-negative") { (void)countBinaryRectangles({{1}}, -1); }
        else if (name == "total-value-negative") { (void)countBinaryRectanglesTotal({{0}}, -1); }
        else if (name == "area-at-most-value") { (void)countBinaryRectanglesAreaAtMost({{1}}, 1, 2); }
        else if (name == "area-at-most-value-negative") { (void)countBinaryRectanglesAreaAtMost({{1}}, 1, -1); }
        else if (name == "area-at-most-cell") { (void)countBinaryRectanglesAreaAtMost({{1, 2}}, 2); }
        else if (name == "area-at-most-ragged") { (void)countBinaryRectanglesAreaAtMost({{1}, {1, 1}}, 2); }
        else if (name == "min-sum-negative") { (void)maxMinTimesSum({-1}); }
        else if (name == "min-sum-late-negative") { (void)maxMinTimesSum({3, 0, -2}); }
        else if (name == "min-sum-overflow") { (void)maxMinTimesSum({LLONG_MAX, LLONG_MAX, LLONG_MAX}); }
        else if (name == "min-sum-2d-negative") { (void)maxMinTimesSum2D({{0, -1}}); }
        else if (name == "min-sum-2d-short-row") { (void)maxMinTimesSum2D({{1, 1}, {1}}); }
        else if (name == "min-sum-2d-long-row") { (void)maxMinTimesSum2D({{}, {1}}); }
        else if (name == "min-sum-2d-overflow") { (void)maxMinTimesSum2D({{INT_MAX, INT_MAX, INT_MAX}}); }
        else if (name == "checker-square-cell") { (void)largestCheckerboardSquare({{0, 2}}); }
        else if (name == "checker-square-ragged") { (void)largestCheckerboardSquare({{0, 1}, {1}}); }
        else if (name == "checker-rectangle-cell") { (void)largestCheckerboardRectangle({{-1}}); }
        else if (name == "checker-rectangle-ragged") { (void)largestCheckerboardRectangle({{}, {0}}); }
        else if (name == "next-smaller-max-size") { (void)nextSmaller(vector<bool>(size_t(INT_MAX) + 1)); }
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
        exhaustiveFamilies(); randomFamilies(rng); scale();
        exhaustiveAdopted(); randomAdopted(rng); scaleAdopted(); sizeLimits();
        std::cout << "PASS monotone_stack checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL monotone_stack seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
