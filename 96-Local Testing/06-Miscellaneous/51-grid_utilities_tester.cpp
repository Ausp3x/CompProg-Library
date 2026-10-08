#include "../../06-Miscellaneous/51-grid_utilities.hpp"

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

static_assert(Grid::dirIndex('R') == 0 && Grid::dirIndex('N') == 3 && Grid::inBounds(0, 0, 1, 1) && Grid::index(2, 3, 5) == 13);
static_assert(knightDistance(1, 0) == 3 && knightDistance(2, 2) == 4 && knightDistance(0, 0) == 0);

void directions() {
    context = "direction tables";
    for (int d = 0; d < 4; ++d) {
        auto [dx, dy] = Grid::DIRS4[d];
        auto [nx, ny] = Grid::DIRS4[(d + 1) % 4];
        check(Grid::DIRS8[2 * d] == Grid::DIRS4[d], "DIRS8[2d] == DIRS4[d]");
        check(abs(dx) + abs(dy) == 1, "DIRS4 unit steps");
        check(nx == dy && ny == -dx, "DIRS4 turns clockwise with rows downward");}
    check(Grid::DIRS4[0] == pair{0, 1}, "DIRS4 starts to the right");
    set<pair<int, int>> kings;
    for (int d = 0; d < 8; ++d) {
        auto [dx, dy] = Grid::DIRS8[d];
        auto [nx, ny] = Grid::DIRS8[(d + 1) % 8];
        check(max(abs(dx), abs(dy)) == 1, "DIRS8 king steps");
        check(dx * ny - dy * nx == -1, "DIRS8 consecutive steps turn clockwise by 45 degrees");
        kings.insert({dx, dy});}
    check(kings.size() == 8, "DIRS8 distinct");
    for (int c = 0; c < 256; ++c) {
        string valid = "RDLUESWN";
        if (valid.find(char(c)) == string::npos) { continue; }
        int d = int(valid.find(char(c))) % 4;
        context = string("dirIndex ") + char(c);
        checkEqual(Grid::dirIndex(char(c)), d, "dirIndex letter to DIRS4 index");}}

void cells(int limit) {
    for (int n = 0; n <= limit; ++n) {
        for (int m = 0; m <= limit; ++m) {
            context = "n=" + std::to_string(n) + " m=" + std::to_string(m);
            vector<int> seen(n * m, 0);
            for (int i = -2; i < n + 2; ++i) {
                for (int j = -2; j < m + 2; ++j) {
                    bool inside = i >= 0 && j >= 0 && i < n && j < m;
                    checkEqual(Grid::inBounds(i, j, n, m), inside, "inBounds against interval test");
                    if (inside) {
                        int id = Grid::index(i, j, m);
                        check(0 <= id && id < n * m && id / m == i && id % m == j, "index row-major bijection");
                        ++seen[id];}
                    for (int k : {4, 8}) {
                        vector<pair<int, int>> got, want;
                        if (k == 4) { Grid::neighbors<4>(i, j, n, m, [&](int x, int y) { got.push_back({x, y}); }); }
                        else { Grid::neighbors<8>(i, j, n, m, [&](int x, int y) { got.push_back({x, y}); }); }
                        for (auto [dx, dy] : vector<pair<int, int>>(k == 4 ? Grid::DIRS4.begin() : Grid::DIRS8.begin(), k == 4 ? Grid::DIRS4.end() : Grid::DIRS8.end())) {
                            int x = i + dx, y = j + dy;
                            if (x >= 0 && y >= 0 && x < n && y < m) { want.push_back({x, y}); }}
                        check(got == want, "neighbors in DIRS order, in bounds only");
                        set<pair<int, int>> brute;
                        for (int x = i - 1; x <= i + 1; ++x) {
                            for (int y = j - 1; y <= j + 1; ++y) {
                                int dist = abs(x - i) + abs(y - j);
                                if (dist && (k == 8 || dist == 1) && x >= 0 && y >= 0 && x < n && y < m) { brute.insert({x, y}); }}}
                        check(set<pair<int, int>>(got.begin(), got.end()) == brute && got.size() == brute.size(), "neighbors set against brute force");}}}
            check(std::all_of(seen.begin(), seen.end(), [](int c) { return c == 1; }), "index hits every cell once");}}}

// Independent definitions: cell (i, j) of the source lands at these coordinates.
template<class G>
void checkTransforms(const G &a, int n, int m) {
    auto at = [&](const G &g, int x, int y) { return g[x][y]; };
    for (int k : {-9, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, INT_MIN, INT_MIN + 1, INT_MAX - 1, INT_MAX}) {
        G r = Grid::rotate90(a, k);
        int q = ((k % 4) + 4) % 4;
        bool sw = q % 2 == 1;
        if (n == 0 || (m == 0 && sw)) {
            check(r.empty(), "rotating a grid without cells leaves no rows");
            continue;}
        checkEqual(int(r.size()), sw ? m : n, "rotated row count");
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                int x = i, y = j, rows = n, cols = m;
                for (int t = 0; t < q; ++t) {
                    int nx = y, ny = rows - 1 - x;
                    x = nx; y = ny; swap(rows, cols);}
                check(int(r[x].size()) == (sw ? n : m) && at(r, x, y) == a[i][j], "rotate90 by repeated clockwise quarter turn");}}}
    if (n == 0) {
        check(Grid::transpose(a).empty() && Grid::flipRows(a).empty() && Grid::flipCols(a).empty(), "empty transforms");
        return;}
    G t = Grid::transpose(a), fr = Grid::flipRows(a), fc = Grid::flipCols(a);
    checkEqual(int(t.size()), m, "transpose rows");
    checkEqual(int(fr.size()), n, "flipRows rows");
    checkEqual(int(fc.size()), n, "flipCols rows");
    for (int i = 0; i < n; ++i) {
        check(int(fr[i].size()) == m && int(fc[i].size()) == m, "flip shapes");
        for (int j = 0; j < m; ++j) {
            check(int(t[j].size()) == n && at(t, j, i) == a[i][j], "transpose");
            check(at(fr, n - 1 - i, j) == a[i][j], "flipRows reverses row order");
            check(at(fc, i, m - 1 - j) == a[i][j], "flipCols reverses each row");}}
    check((m == 0 || Grid::transpose(t) == a) && Grid::flipRows(fr) == a && Grid::flipCols(fc) == a, "involutions");
    check(Grid::rotate90(a) == Grid::flipCols(t) && Grid::rotate90(a, 2) == Grid::flipRows(fc), "dihedral identities");}

template<class G, class E>
void checkPad(const G &a, int n, int m, int k, E fill) {
    G p = Grid::pad(a, k, fill);
    checkEqual(int(p.size()), n + 2 * k, "pad rows");
    for (int i = 0; i < n + 2 * k; ++i) {
        checkEqual(int(p[i].size()), m + 2 * k, "pad columns");
        for (int j = 0; j < m + 2 * k; ++j) {
            bool inner = i >= k && j >= k && i < n + k && j < m + k;
            check(inner ? p[i][j] == a[i - k][j - k] : p[i][j] == fill, "pad cell");}}}

void grids(int rounds, std::mt19937_64 &gen) {
    for (int round = 0; round < rounds; ++round) {
        int n = int(gen() % 7), m = int(gen() % 7);
        if (round < 49) { n = round / 7; m = round % 7; }
        if (n == 0) { m = 0; }
        context = "grid n=" + std::to_string(n) + " m=" + std::to_string(m) + " round=" + std::to_string(round);
        vector<string> s(n, string(m, '.'));
        vector<vector<int>> v(n, vector<int>(m));
        vector<vector<bool>> b(n, vector<bool>(m));
        vector<vector<lng>> w(n, vector<lng>(m));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                s[i][j] = char('a' + gen() % 26); v[i][j] = int(gen() % 1000) - 500;
                b[i][j] = gen() % 2; w[i][j] = lng(gen());}}
        checkTransforms(s, n, m);
        checkTransforms(v, n, m);
        checkTransforms(b, n, m);
        checkTransforms(w, n, m);
        for (int k : {0, 1, 2, 3}) {
            checkPad(s, n, m, k, '#');
            checkPad(v, n, m, k, -7);
            checkPad(b, n, m, k, true);
            checkPad(w, n, m, k, LLONG_MIN);}}}

// Physical model: each label sits on an outward normal (x east, y north, z up); moves are rotation matrices.
struct Model {
    map<array<int, 3>, int> face;
    static array<int, 3> apply(char c, array<int, 3> p) {
        auto [x, y, z] = p;
        if (c == 'N') { return {x, z, -y}; }
        if (c == 'S') { return {x, -z, y}; }
        if (c == 'E') { return {z, y, -x}; }
        if (c == 'W') { return {-z, y, x}; }
        return {y, -x, z};}
    void move(char c) {
        map<array<int, 3>, int> next;
        for (auto [p, label] : face) { next[apply(c, p)] = label; }
        face = next;}
    array<int, 6> faces() { return {face[{0, 0, 1}], face[{0, -1, 0}], face[{1, 0, 0}], face[{-1, 0, 0}], face[{0, 1, 0}], face[{0, 0, -1}]}; }
};
Model modelOf(const array<int, 6> &f) {
    Model res;
    res.face = {{{0, 0, 1}, f[0]}, {{0, -1, 0}, f[1]}, {{1, 0, 0}, f[2]}, {{-1, 0, 0}, f[3]}, {{0, 1, 0}, f[4]}, {{0, 0, -1}, f[5]}};
    return res;}

void dice(int rounds, std::mt19937_64 &gen) {
    context = "dice defaults";
    Dice d;
    check(d.top() == 1 && d.front() == 2 && d.right() == 3 && d.left() == 4 && d.back() == 5 && d.bottom() == 6, "default faces and accessors");
    for (int i = 0; i < 6; ++i) { check(d.f[i] + d.f[5 - i] == 7, "opposite faces sum to 7"); }
    // Model self-check: 4 quarter turns are identity and the closure has 24 states.
    Model probe = modelOf(d.f);
    for (char c : string("NSEWR")) {
        Model q = probe;
        for (int t = 0; t < 4; ++t) { q.move(c); }
        check(q.faces() == probe.faces(), "model quarter turns have order 4");}
    set<array<int, 6>> closure{probe.faces()};
    vector<array<int, 6>> todo{probe.faces()};
    while (!todo.empty()) {
        auto cur = todo.back(); todo.pop_back();
        for (char c : string("NSEWR")) {
            Model q = modelOf(cur); q.move(c);
            if (closure.insert(q.faces()).second) { todo.push_back(q.faces()); }}}
    checkEqual(int(closure.size()), 24, "model rotation group has 24 elements");
    {
        Model q = modelOf(d.f); q.move('N');
        Dice e = d; e.roll('N');
        check(e.top() == 2 && e.back() == 1 && q.faces() == e.f, "roll N moves the front face to the top");
        e = d; e.roll('E');
        check(e.top() == 4 && e.right() == 1, "roll E moves the top face to the right");
        e = d; e.rotate();
        check(e.left() == 2 && e.front() == 3 && e.top() == 1, "rotate spins clockwise seen from above");}
    for (int round = 0; round < rounds; ++round) {
        array<int, 6> labels;
        for (int &x : labels) { x = round % 3 == 0 ? int(gen() % 3) : int(gen() % 1000000) - 500000; }
        if (round % 3 == 1) { iota(labels.begin(), labels.end(), int(gen() % 100)); }
        context = "dice round=" + std::to_string(round);
        Dice cur{labels};
        Model model = modelOf(labels);
        int steps = int(gen() % 40);
        for (int s = 0; s < steps; ++s) {
            int kind = int(gen() % 6);
            if (kind < 4) {
                char c = "NSEW"[kind];
                Dice &ret = cur.roll(c);
                check(&ret == &cur, "roll returns *this");
                model.move(c);}
            else {
                int k = int(gen() % 11) - 5;
                if (kind == 5) { k = gen() % 2 ? INT_MIN : INT_MAX; }
                cur.rotate(k);
                for (int t = 0; t < ((k % 4) + 4) % 4; ++t) { model.move('R'); }}
            check(cur.f == model.faces(), "moves against the physical model");
            check(cur.top() == cur.f[0] && cur.front() == cur.f[1] && cur.right() == cur.f[2] && cur.left() == cur.f[3] && cur.back() == cur.f[4] && cur.bottom() == cur.f[5], "accessors");}
        for (auto [a, b] : vector<pair<char, char>>{{'N', 'S'}, {'S', 'N'}, {'E', 'W'}, {'W', 'E'}}) {
            Dice e = cur;
            check(e.roll(a).roll(b) == cur, "opposite rolls cancel");}
        // Orientations: exactly the model closure from cur, canonical is its minimum.
        set<array<int, 6>> reach{model.faces()};
        vector<array<int, 6>> stack{model.faces()};
        while (!stack.empty()) {
            auto top = stack.back(); stack.pop_back();
            for (char c : string("NSEWR")) {
                Model q = modelOf(top); q.move(c);
                if (reach.insert(q.faces()).second) { stack.push_back(q.faces()); }}}
        auto all = cur.orientations();
        set<array<int, 6>> got;
        for (auto &o : all) { got.insert(o.f); }
        check(got == reach, "orientations equal the reachable set");
        check(all[0] == cur, "first orientation is the die itself");
        bool distinct = set<int>(labels.begin(), labels.end()).size() == 6;
        if (distinct) { checkEqual(int(got.size()), 24, "24 distinct orientations for distinct labels"); }
        array<int, 6> best = *reach.begin();
        check(cur.canonical().f == best, "canonical is the minimal orientation");
        Dice moved = cur;
        for (int s = 0; s < 10; ++s) { moved.roll("NSEW"[gen() % 4]).rotate(int(gen() % 4)); }
        check(moved.canonical() == cur.canonical(), "canonical invariant under rotation");
        Dice mirror = cur;
        swap(mirror.f[Dice::LEFT], mirror.f[Dice::RIGHT]);
        set<array<int, 6>> mreach;
        for (auto &o : mirror.orientations()) { mreach.insert(o.f); }
        checkEqual(mirror.canonical() == cur.canonical(), mreach == reach, "mirror image equal up to rotation exactly when reachable");
        if (distinct) { check(mirror.canonical() != cur.canonical(), "mirror of distinct labels is a different die"); }}}

// Second oracle for large coordinates: least d >= max(ceil(x/2), ceil((x+y)/3)) with d = x+y (mod 2).
lng knightAlternative(lng x, lng y) {
    x = abs(x); y = abs(y);
    if (x < y) { swap(x, y); }
    if (x == 1 && y == 0) { return 3; }
    if (x == 2 && y == 2) { return 4; }
    lll d = max(lll(x + 1) / 2, (lll(x) + y + 2) / 3);
    if ((d - x - y) % 2) { ++d; }
    return lng(d);}

void knights(int radius, int rounds, std::mt19937_64 &gen) {
    int margin = 8, M = radius + margin, W = 2 * M + 1;
    vector<int> dist(W * W, -1);
    deque<int> q{M * W + M};
    dist[M * W + M] = 0;
    int kx[8] = {1, 2, 2, 1, -1, -2, -2, -1}, ky[8] = {2, 1, -1, -2, -2, -1, 1, 2};
    while (!q.empty()) {
        int c = q.front(); q.pop_front();
        for (int d = 0; d < 8; ++d) {
            int a = c / W + kx[d], b = c % W + ky[d];
            if (a < 0 || b < 0 || a >= W || b >= W || dist[a * W + b] >= 0) { continue; }
            dist[a * W + b] = dist[c] + 1; q.push_back(a * W + b);}}
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            context = "knight x=" + std::to_string(x) + " y=" + std::to_string(y);
            int want = dist[(x + M) * W + y + M];
            checkEqual(knightDistance(x, y), lng(want), "knightDistance against bounded BFS");
            checkEqual(knightAlternative(x, y), lng(want), "alternative formula against bounded BFS");}}
    lng big = lng(1) << 62;
    vector<pair<lng, lng>> cases{{big, big}, {-big, big}, {big, 0}, {0, -big}, {big, big - 1}, {big - 1, 1}, {big, 1}, {big / 3, big}};
    for (int r = 0; r < rounds; ++r) {
        lng x = lng(gen() % (2 * ulng(big) + 1)) - big, y = lng(gen() % (2 * ulng(big) + 1)) - big;
        if (r % 4 == 1) { y = x / 2 + lng(gen() % 7) - 3; }
        if (r % 4 == 2) { y = lng(gen() % 1000) - 500; }
        cases.push_back({x, y});}
    for (auto [x, y] : cases) {
        context = "knight x=" + std::to_string(x) + " y=" + std::to_string(y);
        checkEqual(knightDistance(x, y), knightAlternative(x, y), "knightDistance against alternative formula");
        check(knightDistance(x, y) == knightDistance(y, -x) && knightDistance(x, y) == knightDistance(-x, y), "board symmetries");}}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i];
            if (probe == "dir-char") { Grid::dirIndex('x'); }
            if (probe == "ragged-rotate") { Grid::rotate90(vector<string>{"ab", "c"}); }
            if (probe == "ragged-pad") { Grid::pad(vector<string>{"ab", "c"}, 1, '#'); }
            if (probe == "pad-negative") { Grid::pad(vector<string>{"ab"}, -1, '#'); }
            if (probe == "roll-char") { Dice().roll('U'); }
            return 0;}}
    std::mt19937_64 gen(test_seed);
    bool quick = mode == "quick", full = mode == "full";
    directions();
    cells(quick ? 4 : full ? 7 : 10);
    grids(quick ? 100 : full ? 2000 : 20000, gen);
    dice(quick ? 200 : full ? 3000 : 30000, gen);
    knights(quick ? 40 : full ? 120 : 300, quick ? 2000 : full ? 100000 : 2000000, gen);
    cout << "PASS 51-grid_utilities mode=" << mode << " checks=" << checks << '\n';}
