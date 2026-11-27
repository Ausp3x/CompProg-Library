#pragma once
#include "../01-Core/01-template.hpp"

// T: O(1) cell queries, O(K) neighbors, O(n * m) transforms, O((n + 2 * k) * (m + 2 * k)) pad, M: O(out).
// Cell (i, j) is row i downward and column j rightward; directions run clockwise from right; grids are rectangular rows.
struct Grid {
    static constexpr array<pair<int, int>, 4> DIRS4{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
    static constexpr array<pair<int, int>, 8> DIRS8{{{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}}};

    static constexpr int dirIndex(char c) {
        int d = 0;
        while (d < 4 && c != "RDLU"[d] && c != "ESWN"[d]) { ++d; }
        assert(d < 4);
        return d;}
    static constexpr bool inBounds(int i, int j, int n, int m) { return 0 <= i && i < n && 0 <= j && j < m; }
    static constexpr int index(int i, int j, int m) { return i * m + j; }
    template<int K = 4, class F>
    static void neighbors(int i, int j, int n, int m, F &&f) {
        static_assert(K == 4 || K == 8);
        for (int d = 0; d < 8; d += 8 / K) {
            int x = i + DIRS8[d].first, y = j + DIRS8[d].second;
            if (inBounds(x, y, n, m)) { f(x, y); }}}

    template<class G>
    static G remap(const G &a, bool t, bool fi, bool fj) {
        using Row = typename G::value_type;
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        G res(t ? m : n, Row(t ? n : m, typename Row::value_type{}));
        for (int i = 0; i < n; ++i) {
            assert(int(a[i].size()) == m);
            for (int j = 0; j < m; ++j) {
                int x = fi ? n - 1 - i : i, y = fj ? m - 1 - j : j;
                (t ? res[y][x] : res[x][y]) = a[i][j];}}
        return res;}
    template<class G>
    static G rotate90(const G &a, int k = 1) {
        k &= 3;
        return k ? remap(a, k & 1, k != 3, k != 1) : a;}
    template<class G> static G transpose(const G &a) { return remap(a, true, false, false); }
    template<class G> static G flipRows(const G &a) { return remap(a, false, true, false); }
    template<class G> static G flipCols(const G &a) { return remap(a, false, false, true); }
    template<class G>
    static G pad(const G &a, int k, typename G::value_type::value_type fill) {
        assert(k >= 0);
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        G res(n + 2 * k, typename G::value_type(m + 2 * k, fill));
        for (int i = 0; i < n; ++i) {
            assert(int(a[i].size()) == m);
            for (int j = 0; j < m; ++j) { res[i + k][j + k] = a[i][j]; }}
        return res;}
};

// T: O(1) per move, O(1) orientations and canonical (24 states), M: O(1); f = {top, front, right, left, back, bottom}.
// North is away from the viewer at the front, east is to the right; rotate spins clockwise seen from above.
struct Dice {
    static constexpr int TOP = 0, FRONT = 1, RIGHT = 2, LEFT = 3, BACK = 4, BOTTOM = 5;
    array<int, 6> f{1, 2, 3, 4, 5, 6};

    int top() const { return f[TOP]; }
    int front() const { return f[FRONT]; }
    int right() const { return f[RIGHT]; }
    int left() const { return f[LEFT]; }
    int back() const { return f[BACK]; }
    int bottom() const { return f[BOTTOM]; }

    void cycle(int a, int b, int c, int d) {
        int x = f[a];
        f[a] = f[b]; f[b] = f[c]; f[c] = f[d]; f[d] = x;}
    Dice &roll(char c) {
        if (c == 'N') { cycle(TOP, FRONT, BOTTOM, BACK); }
        else if (c == 'S') { cycle(TOP, BACK, BOTTOM, FRONT); }
        else if (c == 'E') { cycle(TOP, LEFT, BOTTOM, RIGHT); }
        else {
            assert(c == 'W');
            cycle(TOP, RIGHT, BOTTOM, LEFT);}
        return *this;}
    Dice &rotate(int k = 1) {
        for (k &= 3; k; --k) { cycle(RIGHT, BACK, LEFT, FRONT); }
        return *this;}

    array<Dice, 24> orientations() const {
        array<Dice, 24> res;
        for (int t = 0; t < 6; ++t) {
            Dice d = *this;
            for (char c : string_view(array<const char *, 6>{"", "N", "W", "E", "S", "NN"}[t])) { d.roll(c); }
            for (int s = 0; s < 4; ++s) { res[4 * t + s] = d; d.rotate(); }}
        return res;}
    Dice canonical() const {
        auto all = orientations();
        return *std::min_element(all.begin(), all.end(), [](const Dice &a, const Dice &b) { return a.f < b.f; });}
    bool operator==(const Dice &o) const = default;
};

// T: O(1), M: O(1); fewest knight moves from (0, 0) to (x, y) on an unbounded board, |x|, |y| <= 2^62.
constexpr lng knightDistance(lng x, lng y) {
    x = x < 0 ? -x : x; y = y < 0 ? -y : y;
    if (x < y) { swap(x, y); }
    if (x == 1 && y == 0) { return 3; }
    if (x == 2 && y == 2) { return 4; }
    lng d = x - y;
    return y > d ? d + 2 * ((y - d + 2) / 3) : d - 2 * ((d - y) / 4);}
