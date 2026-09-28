// 人外有人，天外有天
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
using namespace std;
using namespace __gnu_pbds;

using lng = long long;
using lll = __int128;
template<typename T> 
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

int const INF32 = 0x3f3f3f3f;
lng const INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T> constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T> constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }

// ==============================================================================
// [OMITTED AS PER RULE 3: TEMPLATE MACROS, DEBUG, MODINT, MONOIDS, SEGMENT TREES]
// The algorithms below strictly avoid template macros to remain standalone.
// ==============================================================================

// ==============================================================================
// DATA STRUCTURES
// ==============================================================================

// S: O(n log n), U: NA, Q: O(1) or O(log n), M: O(n log n)
template<typename T, typename F>
struct SparseTable {
    int n; 
    vector<vector<T>> st; 
    F func;
    
    SparseTable(const vector<T> &a, F f) : n(int(a.size())), func(f) {
        int k = std::__lg(n) + 1; 
        st.assign(k, vector<T>(n)); 
        st[0] = a;
        for (int i = 1; i < k; i++) {
            for (int j = 0; j + (1 << i) <= n; j++) {
                st[i][j] = func(st[i - 1][j], st[i - 1][j + (1 << (i - 1))]);
            }
        }
    }

    T queryFast(int l, int r) { 
        int i = std::__lg(r - l + 1); 
        
        return func(st[i][l], st[i][r - (1 << i) + 1]); 
    }

    T querySlow(int l, int r) {
        T res = st[0][l];
        for (int i = std::__lg(r - l + 1); i >= 0; i--) {
            if ((1 << i) <= r - l + 1) { 
                res = func(res, st[i][l]); 
                l += (1 << i); 
            }
        }
        
        return res;
    }
};

// S: O(n), U: O(alpha n), Q: O(alpha n), M: O(n)
struct DSU {
    int n, comps; 
    vector<int> p, siz; 
    vector<lng> dis;
    
    DSU(int n) : n(n), comps(n), p(n), siz(n, 1), dis(n, 0) { 
        std::iota(p.begin(), p.end(), 0); 
    }
    
    int findSet(int u) {
        if (u == p[u]) {
            return u;
        }
        int root = findSet(p[u]); 
        dis[u] += dis[p[u]];
        
        return p[u] = root;
    }

    bool unionSets(int u, int v, lng w = 0) {
        int ru = findSet(u), rv = findSet(v);
        if (ru == rv) {
            return false;
        }
        if (siz[ru] < siz[rv]) { 
            std::swap(ru, rv); 
            std::swap(u, v); 
            w = -w; 
        }
        p[rv] = ru; 
        siz[ru] += siz[rv]; 
        dis[rv] = dis[u] - dis[v] - w; 
        comps--;
        
        return true;
    }
};

// S: O(n), U: O(log n), Q: O(log n), M: O(n)
template<typename T>
struct FenTreeRangeAdd1D {
    int n; 
    vector<T> v1, v2;
    
    FenTreeRangeAdd1D(int n) : n(n), v1(n + 1, 0), v2(n + 1, 0) {}
    
    void _add(int i, T x1, T x2) { 
        for (; i <= n; i += i & -i) { 
            v1[i] += x1; 
            v2[i] += x2; 
        } 
    }
    
    void add(int l, int r, T x) { 
        _add(l, -x * T(l - 1), x); 
        _add(r + 1, x * T(r), -x); 
    }
    
    T _query(int i) {
        T r1 = 0, r2 = 0; 
        int i_cur = i;
        for (; i > 0; i -= i & -i) { 
            r1 += v1[i]; 
            r2 += v2[i]; 
        }
        
        return r2 * T(i_cur) + r1;
    }
    
    T query(int l, int r) { 
        return _query(r) - _query(l - 1); 
    }
};

// S: O(n * m * l), U: O(log n * log m * log l), Q: O(log n * log m * log l), M: O(n * m * l)
template<typename T, typename F, typename Finv>
struct FenTree {
    int d, n, m, l; 
    vector<T> v; 
    F f; 
    Finv f_inv;
    
    FenTree(const vector<T> &a, F f, Finv f_inv) : d(1), n(int(a.size())), m(1), l(1), v(a), f(f), f_inv(f_inv) {
        v.insert(v.begin(), 0);
        for (int i = 1; i <= n; i++) {
            if (i + (i & -i) <= n) {
                v[i + (i & -i)] = f(v[i + (i & -i)], v[i]);
            }
        }
    }

    FenTree(const vector<vector<T>> &a, F f, Finv f_inv) : d(2), n(int(a.size())), m(int(a[0].size())), l(1), v((n + 1) * (m + 1), 0), f(f), f_inv(f_inv) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                v[(i + 1) * (m + 1) + (j + 1)] = a[i][j];
            }
        }
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= m; j++) {
                if (j + (j & -j) <= m) {
                    v[i * (m + 1) + j + (j & -j)] = f(v[i * (m + 1) + j + (j & -j)], v[i * (m + 1) + j]);
                }
            }
        }
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= m; j++) {
                if (i + (i & -i) <= n) {
                    v[(i + (i & -i)) * (m + 1) + j] = f(v[(i + (i & -i)) * (m + 1) + j], v[i * (m + 1) + j]);
                }
            }
        }
    }

    void update(int i, int j, int k, T x) {
        for (; i <= n; i += i & -i) {
            for (int j_cur = j; j_cur <= m; j_cur += j_cur & -j_cur) {
                for (int k_cur = k; k_cur <= l; k_cur += k_cur & -k_cur) {
                    v[i * (m + 1) * (l + 1) + j_cur * (l + 1) + k_cur] = f(v[i * (m + 1) * (l + 1) + j_cur * (l + 1) + k_cur], x);
                }
            }
        }
    }

    T query(int i, int j, int k) {
        T res = 0;
        for (; i > 0; i -= i & -i) {
            for (int j_cur = j; j_cur > 0; j_cur -= j_cur & -j_cur) {
                for (int k_cur = k; k_cur > 0; k_cur -= k_cur & -k_cur) {
                    res = f(res, v[i * (m + 1) * (l + 1) + j_cur * (l + 1) + k_cur]);
                }
            }
        }
        
        return res;
    }

    T query2D(int i1, int j1, int i2, int j2) {
        return f_inv(f_inv(f(query(i2, j2, 1), query(i1 - 1, j1 - 1, 1)), query(i1 - 1, j2, 1)), query(i2, j1 - 1, 1));
    }
};

// S: O(1), U: O(log n), Q: O(log n), M: O(n)
struct LiChaoTree {
    struct Line { 
        lng m = 0, b = INF64; 
        
        lng eval(lng x) { 
            return m != 0 && x > INF64 / abs(m) ? INF64 : m * x + b; 
        } 
    };
    
    struct Node { 
        Line mx_l = {0, -INF64}, mn_l; 
        int lc = -1, rc = -1; 
    };
    
    lng mn_x, mx_x; 
    vector<Node> tree;
    
    LiChaoTree(lng mn, lng mx) : mn_x(mn), mx_x(mx), tree(1) {}
    
    int getChild(int &c) { 
        if (c == -1) { 
            c = int(tree.size()); 
            tree.emplace_back(); 
        } 
        
        return c; 
    }

    void _addLineMax(int i, lng l, lng r, Line line) {
        lng md = std::midpoint(l, r);
        bool l_bet = line.eval(l) > tree[i].mx_l.eval(l), m_bet = line.eval(md) > tree[i].mx_l.eval(md);
        
        if (m_bet) {
            std::swap(tree[i].mx_l, line);
        }
        if (l == r) {
            return;
        }
        if (l_bet != m_bet) {
            _addLineMax(getChild(tree[i].lc), l, md, line); 
        } else {
            _addLineMax(getChild(tree[i].rc), md + 1, r, line);
        }
    }

    void _addLineMin(int i, lng l, lng r, Line line) {
        lng md = std::midpoint(l, r);
        bool l_bet = line.eval(l) < tree[i].mn_l.eval(l), m_bet = line.eval(md) < tree[i].mn_l.eval(md);
        
        if (m_bet) {
            std::swap(tree[i].mn_l, line);
        }
        if (l == r) {
            return;
        }
        if (l_bet != m_bet) {
            _addLineMin(getChild(tree[i].lc), l, md, line); 
        } else {
            _addLineMin(getChild(tree[i].rc), md + 1, r, line);
        }
    }

    void addLine(lng m, lng b) { 
        _addLineMax(0, mn_x, mx_x, {m, b}); 
        _addLineMin(0, mn_x, mx_x, {m, b}); 
    }

    lng maxQuery(lng x) {
        int i = 0; 
        lng l = mn_x, r = mx_x, res = -INF64;
        
        while (i != -1) {
            res = max(res, tree[i].mx_l.eval(x));
            if (l == r) {
                break;
            }
            lng md = std::midpoint(l, r);
            if (x <= md) { 
                i = tree[i].lc; 
                r = md; 
            } else { 
                i = tree[i].rc; 
                l = md + 1; 
            }
        }
        
        return res;
    }
};

// S: O(n log n), U: O(log n), Q: O(1), M: O(n)
struct DynamicMex {
    map<int, int> freq; 
    set<int> missing; 
    vector<int> a;
    
    DynamicMex(const vector<int> &A) : a(A) {
        for (int i = 0; i <= int(a.size()); i++) {
            missing.insert(i);
        }
        for (int x : a) { 
            freq[x]++; 
            missing.erase(x); 
        }
    }

    int query() const { 
        return *missing.begin(); 
    }

    void update(int i, int v) {
        if (--freq[a[i]] == 0) {
            missing.insert(a[i]);
        }
        a[i] = v; 
        freq[v]++; 
        missing.erase(v);
    }
};

// S: O(n log n), U: O(log n), Q: O(log n), M: O(n)
mt19937 treap_rng(chrono::steady_clock::now().time_since_epoch().count());
struct TreapNode {
    int prior, val, sz; 
    bool rev; 
    TreapNode *l, *r;
    TreapNode(int val) : prior(treap_rng()), val(val), sz(1), rev(false), l(nullptr), r(nullptr) {}
};
using pnode = TreapNode*;

int getSz(pnode t) { 
    return t ? t->sz : 0; 
}

void pull(pnode t) { 
    if (t) {
        t->sz = getSz(t->l) + getSz(t->r) + 1;
    }
}

void push(pnode t) {
    if (t && t->rev) {
        t->rev = false; 
        std::swap(t->l, t->r);
        if (t->l) {
            t->l->rev ^= true;
        }
        if (t->r) {
            t->r->rev ^= true;
        }
    }
}

void merge(pnode &t, pnode l, pnode r) {
    push(l); 
    push(r);
    if (!l || !r) {
        t = l ? l : r;
    } else if (l->prior > r->prior) { 
        merge(l->r, l->r, r); 
        t = l; 
    } else { 
        merge(r->l, l, r->l); 
        t = r; 
    }
    pull(t);
}

void split(pnode t, pnode &l, pnode &r, int key, int add = 0) {
    if (!t) { 
        l = r = nullptr; 
        return; 
    }
    push(t); 
    int cur_key = add + getSz(t->l);
    if (key <= cur_key) { 
        split(t->l, l, t->l, key, add); 
        r = t; 
    } else { 
        split(t->r, t->r, r, key, cur_key + 1); 
        l = t; 
    }
    pull(t);
}

// S: O(n), U: NA, Q: O(1), M: O(n)
struct LcaO1 {
    int n, block_sz, block_cnt; 
    vector<int> first_visit, euler_tour, height, log_2, block_mask;
    vector<vector<int>> st; 
    vector<vector<vector<int>>> blocks;
    
    LcaO1(int n, const vector<vector<int>> &adj, int root = 0) : n(n) {
        first_visit.assign(n, -1); 
        height.assign(n, 0); 
        euler_tour.reserve(2 * n);
        
        auto dfs = [&](auto &&dfs, int u, int p, int h) -> void {
            first_visit[u] = int(euler_tour.size()); 
            euler_tour.push_back(u); 
            height[u] = h;
            for (int v : adj[u]) {
                if (v != p) { 
                    dfs(dfs, v, u, h + 1); 
                    euler_tour.push_back(u); 
                }
            }
        };
        dfs(dfs, root, -1, 0);
        
        int m = int(euler_tour.size()); 
        log_2.reserve(m + 1); 
        log_2.push_back(-1);
        for (int i = 1; i <= m; i++) {
            log_2.push_back(log_2[i / 2] + 1);
        }
        
        block_sz = max(1, log_2[m] / 2); 
        block_cnt = (m + block_sz - 1) / block_sz;
        st.assign(block_cnt, vector<int>(log_2[block_cnt] + 1));
        
        auto minByH = [&](int i, int j) -> int { 
            return height[euler_tour[i]] < height[euler_tour[j]] ? i : j; 
        };
        
        for (int i = 0, j = 0, b = 0; i < m; i++, j++) {
            if (j == block_sz) { 
                j = 0; 
                b++; 
            }
            if (j == 0 || minByH(i, st[b][0]) == i) {
                st[b][0] = i;
            }
        }
        
        for (int l = 1; l <= log_2[block_cnt]; l++) {
            for (int i = 0; i < block_cnt; i++) {
                int ni = i + (1 << (l - 1));
                if (ni >= block_cnt) {
                    st[i][l] = st[i][l - 1]; 
                } else {
                    st[i][l] = minByH(st[i][l - 1], st[ni][l - 1]);
                }
            }
        }
        
        block_mask.assign(block_cnt, 0);
        for (int i = 0, j = 0, b = 0; i < m; i++, j++) {
            if (j == block_sz) { 
                j = 0; 
                b++; 
            }
            if (j > 0 && (i >= m || minByH(i - 1, i) == i - 1)) {
                block_mask[b] += 1 << (j - 1);
            }
        }
        
        blocks.resize(1 << (block_sz - 1));
        for (int b = 0; b < block_cnt; b++) {
            int mask = block_mask[b]; 
            if (!blocks[mask].empty()) {
                continue;
            }
            blocks[mask].assign(block_sz, vector<int>(block_sz));
            for (int l = 0; l < block_sz; l++) {
                blocks[mask][l][l] = l;
                for (int r = l + 1; r < block_sz; r++) {
                    blocks[mask][l][r] = blocks[mask][l][r - 1];
                    if (b * block_sz + r < m) {
                        blocks[mask][l][r] = minByH(b * block_sz + blocks[mask][l][r], b * block_sz + r) - b * block_sz;
                    }
                }
            }
        }
    }

    int query(int u, int v) {
        auto minByH = [&](int i, int j) -> int { 
            return height[euler_tour[i]] < height[euler_tour[j]] ? i : j; 
        };
        int l = first_visit[u], r = first_visit[v]; 
        
        if (l > r) {
            std::swap(l, r);
        }
        int bl = l / block_sz, br = r / block_sz;
        if (bl == br) {
            return euler_tour[blocks[block_mask[bl]][l % block_sz][r % block_sz] + bl * block_sz];
        }
        int ans = minByH(blocks[block_mask[bl]][l % block_sz][block_sz - 1] + bl * block_sz, blocks[block_mask[br]][0][r % block_sz] + br * block_sz);
        if (bl + 1 < br) { 
            int lvl = log_2[br - bl - 1]; 
            ans = minByH(ans, minByH(st[bl + 1][lvl], st[br - (1 << lvl)][lvl])); 
        }
        
        return euler_tour[ans];
    }
};

// S: O(n log n), U: NA, Q: O(log n), M: O(n log n)
struct LcaLog {
    int n, l, timer; 
    vector<int> tin, tout; 
    vector<vector<int>> up;
    
    LcaLog(int n, const vector<vector<int>> &adj, int root = 0) : n(n), timer(0) {
        tin.resize(n); 
        tout.resize(n); 
        l = ceil(log2(max(1, n))); 
        up.assign(n, vector<int>(l + 1));
        
        auto dfs = [&](auto &&dfs, int u, int p) -> void {
            tin[u] = ++timer; 
            up[u][0] = p;
            for (int i = 1; i <= l; ++i) {
                up[u][i] = up[up[u][i - 1]][i - 1];
            }
            for (int v : adj[u]) {
                if (v != p) {
                    dfs(dfs, v, u);
                }
            }
            tout[u] = ++timer;
        };
        dfs(dfs, root, root);
    }

    bool isAnc(int u, int v) { 
        return tin[u] <= tin[v] && tout[u] >= tout[v]; 
    }

    int query(int u, int v) {
        if (isAnc(u, v)) {
            return u; 
        }
        if (isAnc(v, u)) {
            return v;
        }
        for (int i = l; i >= 0; i--) {
            if (!isAnc(up[u][i], v)) {
                u = up[u][i];
            }
        }
        
        return up[u][0];
    }
};

// S: O(n), U: O(log^2 n), Q: O(log^2 n), M: O(n)
struct HLD {
    int n, cur_pos; 
    vector<int> parent, depth, heavy, head, pos;
    
    HLD(int n, const vector<vector<int>> &adj, int root = 0) : n(n), cur_pos(0) {
        parent.assign(n, -1); 
        depth.assign(n, 0); 
        heavy.assign(n, -1); 
        head.assign(n, 0); 
        pos.assign(n, 0);
        
        auto dfs = [&](auto &&dfs, int u) -> int {
            int sz = 1, max_c_sz = 0;
            for (int v : adj[u]) {
                if (v != parent[u]) {
                    parent[v] = u; 
                    depth[v] = depth[u] + 1;
                    int c_sz = dfs(dfs, v); 
                    sz += c_sz;
                    if (c_sz > max_c_sz) { 
                        max_c_sz = c_sz; 
                        heavy[u] = v; 
                    }
                }
            }
            
            return sz;
        };
        dfs(dfs, root);
        
        auto decompose = [&](auto &&decompose, int u, int h) -> void {
            head[u] = h; 
            pos[u] = cur_pos++;
            if (heavy[u] != -1) {
                decompose(decompose, heavy[u], h);
            }
            for (int v : adj[u]) {
                if (v != parent[u] && v != heavy[u]) {
                    decompose(decompose, v, v);
                }
            }
        };
        decompose(decompose, root, root);
    }

    template<typename F> 
    void processPath(int u, int v, F f) {
        for (; head[u] != head[v]; v = parent[head[v]]) {
            if (depth[head[u]] > depth[head[v]]) {
                std::swap(u, v);
            }
            f(pos[head[v]], pos[v]);
        }
        if (depth[u] > depth[v]) {
            std::swap(u, v);
        }
        f(pos[u], pos[v]);
    }
};

// S: O(n), M: O(n)
vector<int> buildCartesianTree(const vector<int> &a) {
    int n = int(a.size()); 
    vector<int> p(n, -1), st;
    
    for (int i = 0; i < n; i++) {
        int last = -1;
        while (!st.empty() && a[st.back()] >= a[i]) { 
            last = st.back(); 
            st.pop_back(); 
        }
        if (!st.empty()) {
            p[i] = st.back();
        }
        if (last >= 0) {
            p[last] = i;
        }
        st.push_back(i);
    }
    
    return p;
}

// ==============================================================================
// GEOMETRY
// ==============================================================================

struct Point {
    lng x, y;
    Point() : x(0), y(0) {}
    Point(lng x, lng y) : x(x), y(y) {}
    Point operator+(const Point &o) const { return {x + o.x, y + o.y}; }
    Point operator-(const Point &o) const { return {x - o.x, y - o.y}; }
};

struct PointD {
    double x, y;
    PointD() : x(0), y(0) {}
    PointD(double x, double y) : x(x), y(y) {}
    PointD operator+(const PointD &o) const { return {x + o.x, y + o.y}; }
    PointD operator-(const PointD &o) const { return {x - o.x, y - o.y}; }
    PointD operator*(double t) const { return {x * t, y * t}; }
};

lng dot(Point a, Point b) { 
    return a.x * b.x + a.y * b.y; 
}

lng cross(Point a, Point b) { 
    return a.x * b.y - a.y * b.x; 
}

int sgn(lng x) { 
    return x < 0 ? -1 : x > 0 ? 1 : 0; 
}

// T: O(n), M: O(1)
double polyArea(const vector<Point> &p) {
    double res = 0;
    for (int i = 0; i < int(p.size()); i++) {
        Point p1 = i ? p[i - 1] : p.back(), p2 = p[i];
        res += (p1.x - p2.x) * (p1.y + p2.y);
    }
    
    return abs(res) / 2.0;
}

// T: O(1), M: O(1)
bool checkInter1D(lng a, lng b, lng c, lng d) {
    if (a > b) {
        std::swap(a, b);
    }
    if (c > d) {
        std::swap(c, d);
    }
    
    return max(a, c) <= min(b, d);
}

// T: O(1), M: O(1)
bool checkInterSeg(Point a, Point b, Point c, Point d) {
    if (cross(a - c, d - c) == 0 && cross(b - c, d - c) == 0) {
        return checkInter1D(a.x, b.x, c.x, d.x) && checkInter1D(a.y, b.y, c.y, d.y);
    }
        
    return sgn(cross(b - a, c - a)) != sgn(cross(b - a, d - a)) && sgn(cross(d - c, a - c)) != sgn(cross(d - c, b - c));
}

// T: O(1), M: O(1)
vector<PointD> lineCircleInter(double r, double a, double b, double c) {
    double x0 = -a * c / (a * a + b * b), y0 = -b * c / (a * a + b * b), d_sq = c * c / (a * a + b * b);
    if (d_sq > r * r + 1e-9) {
        return {};
    }
    if (abs(d_sq - r * r) < 1e-9) {
        return {PointD(x0, y0)};
    }
    double d = r * r - d_sq, mult = sqrt(d / (a * a + b * b));
    
    return {PointD(x0 + b * mult, y0 - a * mult), PointD(x0 - b * mult, y0 + a * mult)};
}

// T: O(1), M: O(1)
vector<PointD> circleCircleInter(PointD p1, double r1, PointD p2, double r2) {
    p2 = p2 - p1;
    double a = -2 * p2.x, b = -2 * p2.y, c = p2.x * p2.x + p2.y * p2.y + r1 * r1 - r2 * r2;
    vector<PointD> res = lineCircleInter(r1, a, b, c);
    for (auto &p : res) {
        p = p + p1;
    }
        
    return res;
}

// S: O(n) Expected, U: NA, Q: O(1), M: O(n)
using ptd = complex<double>;
using mec = variant<array<ptd, 2>, array<ptd, 3>>;

double indicator(const mec &c, ptd z) {
    return std::visit([&](auto &&c) -> double {
        ptd a = c[0], b = c[1], i0 = (b - z) * conj(a - z);
        if constexpr (tuple_size_v<decay_t<decltype(c)>> == 2) {
            return real(i0);
        } else { 
            ptd i2 = (a - c[2]) * conj(b - c[2]), i1 = i0 * i2; 
            return imag(i2) < 0 ? -imag(i1) : imag(i1); 
        }
    }, c);
}

bool isInside(const mec &c, ptd p) { 
    return indicator(c, p) <= 0; 
}

mec getMEC(vector<ptd> p) {
    mt19937_64 gen(chrono::steady_clock::now().time_since_epoch().count());
    std::shuffle(p.begin(), p.end(), gen);
    
    mec c = array<ptd, 2>{p[0], p[1]};
    for (int i = 0; i < int(p.size()); i++) {
        if (!isInside(c, p[i])) {
            c = array<ptd, 2>{p[i], p[0]};
            for (int j = 0; j < i; j++) {
                if (!isInside(c, p[j])) {
                    c = array<ptd, 2>{p[i], p[j]};
                    for (int k = 0; k < j; k++) {
                        if (!isInside(c, p[k])) {
                            c = array<ptd, 3>{p[i], p[j], p[k]};
                        }
                    }
                }
            }
        }
    }
    
    return c;
}

// S: O(n log n), U: NA, Q: O(n), M: O(n)
struct Halfplane {
    PointD p, pq; 
    double ang;
    
    Halfplane(const PointD &a, const PointD &b) : p(a), pq(b - a) { 
        ang = atan2(pq.y, pq.x); 
    }
    
    bool isOut(const PointD &r) const { 
        return (pq.x * (r.y - p.y) - pq.y * (r.x - p.x)) < -1e-9; 
    }
    
    bool operator<(const Halfplane &o) const { 
        return ang < o.ang; 
    }
};

PointD getInter(const Halfplane &s, const Halfplane &t) {
    double alpha = (t.p.x - s.p.x) * t.pq.y - (t.p.y - s.p.y) * t.pq.x;
    alpha /= s.pq.x * t.pq.y - s.pq.y * t.pq.x;
    
    return s.p + s.pq * alpha;
}

vector<PointD> halfplaneInter(vector<Halfplane> &hp) {
    PointD box[4] = {{1e9, 1e9}, {-1e9, 1e9}, {-1e9, -1e9}, {1e9, -1e9}};
    for (int i = 0; i < 4; i++) {
        hp.push_back({box[i], box[(i + 1) % 4]});
    }
    std::sort(hp.begin(), hp.end());
    
    deque<Halfplane> dq; 
    int len = 0;
    for (int i = 0; i < int(hp.size()); i++) {
        while (len > 1 && hp[i].isOut(getInter(dq[len - 1], dq[len - 2]))) { 
            dq.pop_back(); 
            len--; 
        }
        while (len > 1 && hp[i].isOut(getInter(dq[0], dq[1]))) { 
            dq.pop_front(); 
            len--; 
        }
        if (len > 0 && abs(hp[i].pq.x * dq[len - 1].pq.y - hp[i].pq.y * dq[len - 1].pq.x) < 1e-9) {
            if (hp[i].pq.x * dq[len - 1].pq.x + hp[i].pq.y * dq[len - 1].pq.y < 0) {
                return {};
            }
            if (hp[i].isOut(dq[len - 1].p)) { 
                dq.pop_back(); 
                len--; 
            } else {
                continue;
            }
        }
        dq.push_back(hp[i]); 
        len++;
    }
    
    while (len > 2 && dq[0].isOut(getInter(dq[len - 1], dq[len - 2]))) { 
        dq.pop_back(); 
        len--; 
    }
    while (len > 2 && dq[len - 1].isOut(getInter(dq[0], dq[1]))) { 
        dq.pop_front(); 
        len--; 
    }
    if (len < 3) {
        return {};
    }
        
    vector<PointD> res(len);
    for (int i = 0; i < len - 1; i++) {
        res[i] = getInter(dq[i], dq[i + 1]);
    }
    res.back() = getInter(dq[len - 1], dq[0]);
    
    return res;
}

// S: O(n log n), U: NA, Q: O(1), M: O(n)
struct Seg {
    Point p, q; 
    int id;
    
    double getY(double x) const { 
        return abs(p.x - q.x) == 0 ? p.y : p.y + (double(q.y) - p.y) * (x - p.x) / (q.x - p.x); 
    }
    
    bool operator<(const Seg &o) const { 
        return getY(max(min(p.x, q.x), min(o.p.x, o.q.x))) < o.getY(max(min(p.x, q.x), min(o.p.x, o.q.x))) - 1e-9; 
    }
};

pair<int, int> findInterSegs(const vector<Seg> &segs) {
    int n = int(segs.size()); 
    vector<tuple<double, int, int>> evs;
    for (int i = 0; i < n; i++) {
        evs.push_back({min(segs[i].p.x, segs[i].q.x), 1, i});
        evs.push_back({max(segs[i].p.x, segs[i].q.x), -1, i});
    }
    std::sort(evs.begin(), evs.end(), [](const auto &a, const auto &b) {
        if (abs(get<0>(a) - get<0>(b)) > 1e-9) {
            return get<0>(a) < get<0>(b);
        }
        
        return get<1>(a) > get<1>(b);
    });
    
    set<Seg> s; 
    vector<set<Seg>::iterator> loc(n);
    for (const auto &[x, type, id] : evs) {
        if (type == 1) {
            auto nxt = s.lower_bound(segs[id]), prv = nxt == s.begin() ? s.end() : std::prev(nxt);
            if (nxt != s.end() && checkInterSeg(nxt->p, nxt->q, segs[id].p, segs[id].q)) {
                return {nxt->id, id};
            }
            if (prv != s.end() && checkInterSeg(prv->p, prv->q, segs[id].p, segs[id].q)) {
                return {prv->id, id};
            }
            loc[id] = s.insert(nxt, segs[id]);
        } else {
            auto nxt = std::next(loc[id]), prv = loc[id] == s.begin() ? s.end() : std::prev(loc[id]);
            if (nxt != s.end() && prv != s.end() && checkInterSeg(nxt->p, nxt->q, prv->p, prv->q)) {
                return {prv->id, nxt->id};
            }
            s.erase(loc[id]);
        }
    }
    
    return {-1, -1};
}

// T: O(n log n), M: O(n)
int lenUnionSegs(const vector<pair<int, int>> &segs) {
    vector<pair<int, int>> evs;
    for (const auto &[a, b] : segs) { 
        evs.push_back({a, 1}); 
        evs.push_back({b, -1}); 
    }
    std::sort(evs.begin(), evs.end());
    
    int res = 0, c = 0;
    for (int i = 0; i < int(evs.size()); i++) {
        if (i > 0 && evs[i].first > evs[i - 1].first && c > 0) {
            res += evs[i].first - evs[i - 1].first;
        }
        c += evs[i].second;
    }
    
    return res;
}

// T: O(1), M: O(1)
pair<bool, PointD> interLines(double a1, double b1, double c1, double a2, double b2, double c2) {
    double det = a1 * b2 - a2 * b1;
    if (abs(det) < 1e-9) {
        return {false, {}};
    }
        
    return {true, {-(c1 * b2 - c2 * b1) / det, -(a1 * c2 - a2 * c1) / det}};
}

// T: O(n log n), M: O(n)
vector<tuple<lng, int, int>> getManhattanMSTEdges(vector<Point> p) {
    int n = int(p.size()); 
    vector<int> ids(n); 
    std::iota(ids.begin(), ids.end(), 0);
    vector<tuple<lng, int, int>> edges;
    
    for (int rot = 0; rot < 4; rot++) {
        std::sort(ids.begin(), ids.end(), [&](int i, int j) { 
            return (p[i].x + p[i].y) < (p[j].x + p[j].y); 
        });
        
        map<int, int, std::greater<int>> active;
        for (int i : ids) {
            for (auto it = active.lower_bound(p[i].x); it != active.end(); active.erase(it++)) {
                int j = it->second;
                if (p[i].x - p[i].y > p[j].x - p[j].y) {
                    break;
                }
                edges.push_back({(p[i].x - p[j].x) + (p[i].y - p[j].y), i, j});
            }
            active[p[i].x] = i;
        }
        for (auto &pt : p) { 
            if (rot & 1) {
                pt.x = -pt.x; 
            } else {
                std::swap(pt.x, pt.y);
            }
        }
    }
    
    return edges;
}

// T: O(n + m), M: O(n + m)
vector<Point> minkowskiSum(vector<Point> P, vector<Point> Q) {
    auto reorder = [](vector<Point> &p) {
        int pos = 0;
        for (int i = 1; i < int(p.size()); i++) {
            if (p[i].y < p[pos].y || (p[i].y == p[pos].y && p[i].x < p[pos].x)) {
                pos = i;
            }
        }
        std::rotate(p.begin(), p.begin() + pos, p.end());
    };
    
    reorder(P); 
    reorder(Q);
    P.push_back(P[0]); 
    P.push_back(P[1]); 
    Q.push_back(Q[0]); 
    Q.push_back(Q[1]);
    
    vector<Point> res; 
    int i = 0, j = 0;
    while (i < int(P.size()) - 2 || j < int(Q.size()) - 2) {
        res.push_back(P[i] + Q[j]);
        lng crs = cross(P[i + 1] - P[i], Q[j + 1] - Q[j]);
        if (crs >= 0 && i < int(P.size()) - 2) {
            i++;
        }
        if (crs <= 0 && j < int(Q.size()) - 2) {
            j++;
        }
    }
    
    return res;
}

// T: O(n log n), M: O(n)
pair<int, int> getNearestPair(vector<Point> &p) {
    int n = int(p.size()); 
    vector<int> ids(n), t(n); 
    std::iota(ids.begin(), ids.end(), 0);
    std::sort(ids.begin(), ids.end(), [&](int i, int j) { 
        return p[i].x < p[j].x || (p[i].x == p[j].x && p[i].y < p[j].y); 
    });
    
    lng min_dist = INF64; 
    pair<int, int> best = {-1, -1};
    
    auto upd = [&](int i, int j) -> void {
        lng dist = (p[i].x - p[j].x) * (p[i].x - p[j].x) + (p[i].y - p[j].y) * (p[i].y - p[j].y);
        if (dist < min_dist) { 
            min_dist = dist; 
            best = {i, j}; 
        }
    };
    
    auto rec = [&](auto &&rec, int l, int r) -> void {
        if (r - l <= 3) {
            for (int i = l; i < r; i++) {
                for (int j = i + 1; j < r; j++) {
                    upd(ids[i], ids[j]);
                }
            }
            std::sort(ids.begin() + l, ids.begin() + r, [&](int i, int j) { 
                return p[i].y < p[j].y; 
            });
            return;
        }
        
        int md = std::midpoint(l, r); 
        lng md_x = p[ids[md]].x;
        rec(rec, l, md); 
        rec(rec, md, r);
        std::merge(ids.begin() + l, ids.begin() + md, ids.begin() + md, ids.begin() + r, t.begin(), [&](int i, int j) { 
            return p[i].y < p[j].y; 
        });
        std::copy(t.begin(), t.begin() + r - l, ids.begin() + l);
        
        int t_sz = 0;
        for (int i = l; i < r; i++) {
            if (abs(p[ids[i]].x - md_x) * abs(p[ids[i]].x - md_x) < min_dist) {
                for (int j = t_sz - 1; j >= 0 && (p[ids[i]].y - p[t[j]].y) * (p[ids[i]].y - p[t[j]].y) < min_dist; j--) {
                    upd(ids[i], t[j]);
                }
                t[t_sz++] = ids[i];
            }
        }
    };
    
    rec(rec, 0, n);
    
    return best;
}

// ==============================================================================
// GRAPHS
// ==============================================================================

// S: O(V + E), M: O(V)
vector<vector<int>> getConnectedComps(int n, const vector<vector<int>> &adj) {
    vector<bool> used(n, false); 
    vector<vector<int>> comps; 
    vector<int> st;
    
    for (int i = 0; i < n; i++) {
        if (!used[i]) {
            vector<int> cur_comp; 
            st.push_back(i);
            while (!st.empty()) {
                int u = st.back(); 
                st.pop_back();
                if (!used[u]) {
                    used[u] = true; 
                    cur_comp.push_back(u);
                    for (int v : adj[u]) {
                        if (!used[v]) {
                            st.push_back(v);
                        }
                    }
                }
            }
            comps.push_back(cur_comp);
        }
    }
    
    return comps;
}

// S: O(V + E), M: O(V)
vector<int> getTopoSort(int n, const vector<vector<int>> &adj) {
    vector<bool> used(n, false); 
    vector<int> ans;
    
    auto dfs = [&](auto &&dfs, int u) -> void {
        used[u] = true;
        for (int v : adj[u]) {
            if (!used[v]) {
                dfs(dfs, v);
            }
        }
        ans.push_back(u);
    };
    
    for (int i = 0; i < n; i++) {
        if (!used[i]) {
            dfs(dfs, i);
        }
    }
    std::reverse(ans.begin(), ans.end());
    
    return ans;
}

// S: O((V + E) log V), U: NA, Q: O(1), M: O(V)
struct Dijkstra {
    int n; 
    vector<lng> d; 
    vector<int> p;
    
    Dijkstra(int n) : n(n) {}
    
    void solve(int s, const vector<vector<pair<int, lng>>> &adj) {
        d.assign(n, INF64); 
        p.assign(n, -1);
        priority_queue<pair<lng, int>, vector<pair<lng, int>>, std::greater<pair<lng, int>>> pq;
        d[s] = 0; 
        pq.push({0, s});
        
        while (!pq.empty()) {
            auto [dist, u] = pq.top(); 
            pq.pop();
            if (dist > d[u]) {
                continue;
            }
            for (auto [v, w] : adj[u]) {
                if (d[u] + w < d[v]) { 
                    d[v] = d[u] + w; 
                    p[v] = u; 
                    pq.push({d[v], v}); 
                }
            }
        }
    }
};

// T: O(V * E) avg, exp worst case, M: O(V)
struct DEsopoPape {
    int n; 
    vector<lng> d; 
    vector<int> p;
    
    DEsopoPape(int n) : n(n) {}
    
    void solve(int s, const vector<vector<pair<int, lng>>> &adj) {
        d.assign(n, INF64); 
        p.assign(n, -1); 
        vector<int> m(n, 2); 
        deque<int> q;
        d[s] = 0; 
        q.push_back(s);
        
        while (!q.empty()) {
            int u = q.front(); 
            q.pop_front(); 
            m[u] = 0;
            for (auto [v, w] : adj[u]) {
                if (d[u] + w < d[v]) {
                    d[v] = d[u] + w; 
                    p[v] = u;
                    if (m[v] == 2) { 
                        m[v] = 1; 
                        q.push_back(v); 
                    } else if (m[v] == 0) { 
                        m[v] = 1; 
                        q.push_front(v); 
                    }
                }
            }
        }
    }
};

// S: O(V * E), U: NA, Q: O(1), M: O(V + E)
struct SPFA {
    int n; 
    vector<lng> d; 
    vector<int> p;
    
    SPFA(int n) : n(n) {}
    
    bool solve(int s, const vector<vector<pair<int, lng>>> &adj) {
        d.assign(n, INF64); 
        p.assign(n, -1);
        vector<int> cnt(n, 0); 
        vector<bool> in_q(n, false); 
        queue<int> q;
        d[s] = 0; 
        q.push(s); 
        in_q[s] = true;
        
        while (!q.empty()) {
            int u = q.front(); 
            q.pop(); 
            in_q[u] = false;
            for (auto [v, w] : adj[u]) {
                if (d[u] + w < d[v]) {
                    d[v] = d[u] + w; 
                    p[v] = u;
                    if (!in_q[v]) { 
                        q.push(v); 
                        in_q[v] = true; 
                        cnt[v]++; 
                        if (cnt[v] > n) {
                            return false; 
                        }
                    }
                }
            }
        }
        
        return true;
    }
};

// T: O(V^3), M: O(1)
void floydWarshall(vector<vector<lng>> &d) {
    int n = int(d.size());
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (d[i][k] < INF64 && d[k][j] < INF64) {
                    d[i][j] = min(d[i][j], d[i][k] + d[k][j]);
                }
            }
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                if (d[i][k] < INF64 && d[k][k] < 0 && d[k][j] < INF64) { 
                    d[i][j] = -INF64; 
                    break; 
                }
            }
        }
    }
}

// T: O(V + E), M: O(V)
bool isBipartite(int n, const vector<vector<int>> &adj, vector<int> &side) {
    side.assign(n, -1); 
    bool is_bip = true; 
    queue<int> q;
    
    for (int st = 0; st < n; st++) {
        if (side[st] == -1) {
            q.push(st); 
            side[st] = 0;
            while (!q.empty()) {
                int u = q.front(); 
                q.pop();
                for (int v : adj[u]) {
                    if (side[v] == -1) { 
                        side[v] = side[u] ^ 1; 
                        q.push(v); 
                    } else { 
                        is_bip &= side[v] != side[u]; 
                    }
                }
            }
        }
    }
    
    return is_bip;
}

// S: O(V + E), M: O(V)
struct Tarjan {
    int n, timer; 
    vector<bool> vst, is_art; 
    vector<int> t_in, low; 
    vector<pair<int, int>> bridges;
    
    Tarjan(int n, const vector<vector<int>> &adj) : n(n), timer(0), vst(n, false), is_art(n, false), t_in(n, -1), low(n, -1) {
        auto dfs = [&](auto &&dfs, int u, int p) -> void {
            vst[u] = true; 
            t_in[u] = low[u] = timer++;
            int children = 0; 
            bool p_skip = false;
            for (int v : adj[u]) {
                if (v == p && !p_skip) { 
                    p_skip = true; 
                    continue; 
                }
                if (vst[v]) {
                    low[u] = min(low[u], t_in[v]);
                } else {
                    dfs(dfs, v, u); 
                    low[u] = min(low[u], low[v]);
                    if (low[v] >= t_in[u] && p != -1) {
                        is_art[u] = true;
                    }
                    if (low[v] > t_in[u]) {
                        bridges.push_back({min(u, v), max(u, v)});
                    }
                    children++;
                }
            }
            if (p == -1 && children > 1) {
                is_art[u] = true;
            }
        };
        for (int i = 0; i < n; i++) {
            if (!vst[i]) {
                dfs(dfs, i, -1);
            }
        }
    }
};

// S: O(V), U: O(log V), Q: O(1), M: O(V)
struct OnlineBridges {
    int bridges, lca_itr; 
    vector<int> par, d_2ec, d_cc, sz_cc, last_vst;
    
    OnlineBridges(int n) : bridges(0), lca_itr(0), par(n, -1), d_2ec(n), d_cc(n), sz_cc(n, 1), last_vst(n, 0) {
        std::iota(d_2ec.begin(), d_2ec.end(), 0); 
        std::iota(d_cc.begin(), d_cc.end(), 0);
    }
    
    int find2ec(int v) { 
        return v == -1 ? -1 : (d_2ec[v] == v ? v : d_2ec[v] = find2ec(d_2ec[v])); 
    }
    
    int findCC(int v) { 
        v = find2ec(v); 
        
        return d_cc[v] == v ? v : d_cc[v] = findCC(d_cc[v]); 
    }
    
    void makeRoot(int v) {
        int root = v, child = -1;
        while (v != -1) {
            int p = find2ec(par[v]); 
            par[v] = child; 
            d_cc[v] = root; 
            child = v; 
            v = p;
        }
        sz_cc[root] = sz_cc[child];
    }
    
    void mergePath(int a, int b) {
        lca_itr++; 
        vector<int> path_a, path_b; 
        int lca = -1;
        
        while (lca == -1) {
            if (a != -1) {
                a = find2ec(a); 
                path_a.push_back(a);
                if (last_vst[a] == lca_itr) { 
                    lca = a; 
                    break; 
                }
                last_vst[a] = lca_itr; 
                a = par[a];
            }
            if (b != -1) {
                b = find2ec(b); 
                path_b.push_back(b);
                if (last_vst[b] == lca_itr) { 
                    lca = b; 
                    break; 
                }
                last_vst[b] = lca_itr; 
                b = par[b];
            }
        }
        for (int v : path_a) { 
            d_2ec[v] = lca; 
            if (v == lca) {
                break; 
            }
            bridges--; 
        }
        for (int v : path_b) { 
            d_2ec[v] = lca; 
            if (v == lca) {
                break; 
            }
            bridges--; 
        }
    }
    
    void addEdge(int a, int b) {
        a = find2ec(a); 
        b = find2ec(b);
        if (a == b) {
            return;
        }
        
        int ca = findCC(a), cb = findCC(b);
        if (ca != cb) {
            bridges++;
            if (sz_cc[ca] > sz_cc[cb]) { 
                std::swap(a, b); 
                std::swap(ca, cb); 
            }
            makeRoot(a); 
            par[a] = d_cc[a] = b; 
            sz_cc[cb] += sz_cc[a];
        } else {
            mergePath(a, b);
        }
    }
};

// S: O(V + E), U: NA, Q: O(1), M: O(V)
struct TarjanSCC {
    int n, timer; 
    vector<int> st, roots, t_in, t_low; 
    vector<vector<int>> sccs, adj_cond;
    
    TarjanSCC(int n, const vector<vector<int>> &adj) : n(n), timer(0), roots(n, -1), t_in(n, -1), t_low(n, -1) {
        auto dfs = [&](auto &&dfs, int u) -> void {
            t_low[u] = t_in[u] = timer++; 
            st.push_back(u);
            for (int v : adj[u]) {
                if (t_in[v] == -1) { 
                    dfs(dfs, v); 
                    t_low[u] = min(t_low[u], t_low[v]); 
                } else if (roots[v] == -1) {
                    t_low[u] = min(t_low[u], t_in[v]);
                }
            }
            if (t_low[u] == t_in[u]) {
                sccs.push_back({u});
                while (true) {
                    int v = st.back(); 
                    st.pop_back(); 
                    roots[v] = u;
                    if (v == u) {
                        break;
                    }
                    sccs.back().push_back(v);
                }
            }
        };
        for (int i = 0; i < n; i++) {
            if (t_in[i] == -1) {
                dfs(dfs, i);
            }
        }
        
        adj_cond.assign(n, {});
        for (int u = 0; u < n; u++) {
            for (int v : adj[u]) {
                if (roots[u] != roots[v]) {
                    adj_cond[roots[u]].push_back(roots[v]);
                }
            }
        }
    }
};

// S: O(V + E), U: NA, Q: NA, M: O(V + E)
struct StrongOrientation {
    int n, m, bridge_cnt; 
    vector<vector<pair<int, int>>> adj; 
    vector<pair<int, int>> edges;
    vector<int> t_in, low; 
    vector<bool> edge_used; 
    string orient;
    
    StrongOrientation(int n, int m) : n(n), m(m), bridge_cnt(0), adj(n), edges(m), t_in(n, -1), low(n, -1), edge_used(m, false), orient(m, ' ') {}
    
    void addEdge(int u, int v, int id) { 
        adj[u].push_back({v, id}); 
        adj[v].push_back({u, id}); 
        edges[id] = {u, v}; 
    }
    
    pair<int, string> solve() {
        int timer = 0, comp_cnt = 0;
        
        auto dfs = [&](auto &&dfs, int u) -> void {
            low[u] = t_in[u] = timer++;
            for (auto [v, id] : adj[u]) {
                if (edge_used[id]) {
                    continue;
                }
                edge_used[id] = true; 
                orient[id] = u == edges[id].first ? '>' : '<';
                
                if (t_in[v] == -1) {
                    dfs(dfs, v); 
                    low[u] = min(low[u], low[v]);
                    if (low[v] > t_in[u]) {
                        bridge_cnt++;
                    }
                } else {
                    low[u] = min(low[u], t_in[v]);
                }
            }
        };
        for (int i = 0; i < n; i++) {
            if (t_in[i] == -1) { 
                comp_cnt++; 
                dfs(dfs, i); 
            }
        }
        
        return {comp_cnt + bridge_cnt, orient};
    }
};

// S: O(V + E), Q: NA, M: O(V + E)
struct TwoSat {
    int n; 
    vector<vector<int>> adj, adj_t; 
    vector<bool> used, ans; 
    vector<int> ord, comp;
    
    TwoSat(int n) : n(n), adj(2 * n), adj_t(2 * n), ans(n) {}
    
    void addClause(int a, bool na, int b, bool nb) {
        int u = 2 * a ^ na, v = 2 * b ^ nb, neg_u = u ^ 1, neg_v = v ^ 1;
        adj[neg_u].push_back(v); 
        adj[neg_v].push_back(u);
        adj_t[v].push_back(neg_u); 
        adj_t[u].push_back(neg_v);
    }
    
    bool solve() {
        used.assign(2 * n, false); 
        ord.clear();
        auto dfs1 = [&](auto &&dfs1, int u) -> void {
            used[u] = true;
            for (int v : adj[u]) {
                if (!used[v]) {
                    dfs1(dfs1, v);
                }
            }
            ord.push_back(u);
        };
        for (int i = 0; i < 2 * n; i++) {
            if (!used[i]) {
                dfs1(dfs1, i);
            }
        }
        
        comp.assign(2 * n, -1);
        auto dfs2 = [&](auto &&dfs2, int u, int c) -> void {
            comp[u] = c;
            for (int v : adj_t[u]) {
                if (comp[v] == -1) {
                    dfs2(dfs2, v, c);
                }
            }
        };
        for (int i = 0, j = 0; i < 2 * n; i++) {
            int u = ord[2 * n - i - 1];
            if (comp[u] == -1) {
                dfs2(dfs2, u, j++);
            }
        }
        
        for (int i = 0; i < 2 * n; i += 2) {
            if (comp[i] == comp[i + 1]) {
                return false;
            }
            ans[i / 2] = comp[i] > comp[i + 1];
        }
        
        return true;
    }
};

// T: O(V + E), M: O(V + E)
struct EulerPath {
    int n, edge_cnt; 
    vector<vector<pair<int, int>>> adj; 
    
    EulerPath(int n) : n(n), edge_cnt(0), adj(n) {}
    
    void addEdge(int u, int v) { 
        adj[u].push_back({v, edge_cnt}); 
        adj[v].push_back({u, edge_cnt++}); 
    }
    
    vector<int> solve() {
        if (edge_cnt == 0) {
            return {0};
        }
        
        vector<int> deg(n, 0); 
        for (int i = 0; i < n; i++) {
            deg[i] = int(adj[i].size());
        }
        
        int odd_cnt = 0, start_node = -1, edge_start = -1;
        for (int i = 0; i < n; i++) {
            if (deg[i] > 0 && edge_start == -1) {
                edge_start = i;
            }
            if (deg[i] % 2 != 0) { 
                odd_cnt++; 
                start_node = i; 
            }
        }
        
        if (odd_cnt != 0 && odd_cnt != 2) {
            return {};
        }
        if (start_node == -1) {
            start_node = edge_start;
        }
        
        vector<bool> used_edge(edge_cnt, false); 
        vector<int> head(n, 0), res, st;
        st.push_back(start_node);
        
        while (!st.empty()) {
            int u = st.back();
            if (head[u] < int(adj[u].size())) {
                auto [v, id] = adj[u][head[u]++];
                if (!used_edge[id]) { 
                    used_edge[id] = true; 
                    st.push_back(v); 
                }
            } else { 
                res.push_back(u); 
                st.pop_back(); 
            }
        }
        
        if (int(res.size()) != edge_cnt + 1) {
            return {};
        }
        std::reverse(res.begin(), res.end());
        
        return res;
    }
};

// T: O(V + E), M: O(V)
vector<int> findDirectedCycle(int n, const vector<vector<int>> &adj) {
    vector<char> col(n, 0); 
    vector<int> p(n, -1); 
    int start = -1, end = -1;
    
    auto dfs = [&](auto &&dfs, int u) -> bool {
        col[u] = 1;
        for (int v : adj[u]) {
            if (col[v] == 0) { 
                p[v] = u; 
                if (dfs(dfs, v)) {
                    return true; 
                }
            } else if (col[v] == 1) { 
                start = v; 
                end = u; 
                return true; 
            }
        }
        col[u] = 2; 
        
        return false;
    };
    
    for (int i = 0; i < n; i++) {
        if (col[i] == 0 && dfs(dfs, i)) {
            break;
        }
    }
        
    if (start == -1) {
        return {};
    }
        
    vector<int> cycle; 
    cycle.push_back(start);
    for (int v = end; v != start; v = p[v]) {
        cycle.push_back(v);
    }
    cycle.push_back(start); 
    std::reverse(cycle.begin(), cycle.end());
    
    return cycle;
}

// T: O(V * E), M: O(V)
struct Edge { int u, v; lng w; };

vector<int> findNegativeCycle(int n, const vector<Edge> &edges) {
    vector<lng> d(n, 0); 
    vector<int> p(n, -1); 
    int x = -1;
    
    for (int i = 0; i < n; i++) {
        x = -1;
        for (const auto &e : edges) {
            if (d[e.u] + e.w < d[e.v]) { 
                d[e.v] = max(-INF64, d[e.u] + e.w); 
                p[e.v] = e.u; 
                x = e.v; 
            }
        }
    }
    
    if (x == -1) {
        return {};
    }
    for (int i = 0; i < n; i++) {
        x = p[x];
    }
        
    vector<int> cycle;
    for (int v = x;; v = p[v]) {
        cycle.push_back(v);
        if (v == x && cycle.size() > 1) {
            break;
        }
    }
    std::reverse(cycle.begin(), cycle.end());
    
    return cycle;
}

// S: O(E log V), U: NA, Q: O(1), M: O(V)
template<typename T>
struct Kruskal {
    struct KEdge {
        int u, v; 
        T w;
        bool operator<(const KEdge &o) const { return w < o.w; }
    };
    int n; 
    vector<KEdge> edges;
    
    Kruskal(int n, const vector<KEdge> &E) : n(n), edges(E) { 
        std::sort(edges.begin(), edges.end()); 
    }
    
    pair<T, vector<KEdge>> solve() {
        T cost = 0; 
        vector<KEdge> mst; 
        vector<int> p(n), rank(n, 0); 
        std::iota(p.begin(), p.end(), 0);
        
        auto findSet = [&](auto &&findSet, int v) -> int { 
            return v == p[v] ? v : p[v] = findSet(findSet, p[v]); 
        };
        auto unionSets = [&](int a, int b) -> bool {
            a = findSet(findSet, a); 
            b = findSet(findSet, b);
            if (a != b) {
                if (rank[a] < rank[b]) {
                    std::swap(a, b);
                }
                p[b] = a; 
                if (rank[a] == rank[b]) {
                    rank[a]++;
                }
                
                return true;
            }
            
            return false;
        };
        
        for (const auto &e : edges) {
            if (unionSets(e.u, e.v)) { 
                cost += e.w; 
                mst.push_back(e); 
            }
            if (int(mst.size()) == n - 1) {
                break;
            }
        }
        
        return {cost, mst};
    }
};

// S: O(V^2), U: NA, Q: O(1), M: O(V)
template<typename T>
struct PrimDense {
    int n; 
    vector<vector<T>> adj;
    
    PrimDense(int n, const vector<vector<T>> &adj) : n(n), adj(adj) {}
    
    pair<T, vector<pair<int, int>>> solve() {
        T total_w = 0; 
        vector<bool> sel(n, false); 
        vector<T> min_w(n, std::numeric_limits<T>::max());
        vector<int> min_e(n, -1); 
        vector<pair<int, int>> mst;
        min_w[0] = 0;
        
        for (int i = 0; i < n; i++) {
            int v = -1;
            for (int j = 0; j < n; j++) {
                if (!sel[j] && (v == -1 || min_w[j] < min_w[v])) {
                    v = j;
                }
            }
            if (min_w[v] == std::numeric_limits<T>::max()) {
                return {-1, {}};
            }
            sel[v] = true; 
            total_w += min_w[v];
            
            if (min_e[v] != -1) {
                mst.push_back({v, min_e[v]});
            }
            for (int to = 0; to < n; to++) {
                if (adj[v][to] < min_w[to]) { 
                    min_w[to] = adj[v][to]; 
                    min_e[to] = v; 
                }
            }
        }
        
        return {total_w, mst};
    }
};

// S: O(V log V), U: NA, Q: O(1), M: O(V)
struct CentroidDecomposition {
    int n; 
    vector<int> sz, par; 
    vector<bool> rmv;
    
    CentroidDecomposition(int n, const vector<vector<int>> &adj) : n(n), sz(n, 0), par(n, -1), rmv(n, false) {
        auto getSz = [&](auto &&getSz, int u, int p) -> int {
            sz[u] = 1;
            for (int v : adj[u]) {
                if (v != p && !rmv[v]) {
                    sz[u] += getSz(getSz, v, u);
                }
            }
            
            return sz[u];
        };
        auto getCent = [&](auto &&getCent, int u, int p, int t_sz) -> int {
            for (int v : adj[u]) {
                if (v != p && !rmv[v] && sz[v] * 2 > t_sz) {
                    return getCent(getCent, v, u, t_sz);
                }
            }
            
            return u;
        };
        auto build = [&](auto &&build, int u, int p) -> void {
            int t_sz = getSz(getSz, u, -1), cent = getCent(getCent, u, -1, t_sz);
            par[cent] = p; 
            rmv[cent] = true;
            for (int v : adj[cent]) {
                if (!rmv[v]) {
                    build(build, v, cent);
                }
            }
        };
        
        build(build, 0, -1);
    }
};

// S: O(V log V), U: O(log V), Q: O(log V), M: O(V)
template<typename SegTree>
struct TreeEdgePainting {
    int n, timer; 
    vector<int> dfs_list, edge_list, h, fst1, fst2;
    vector<vector<pair<int, int>>> adj; 
    SegTree t1, t2; 
    
    TreeEdgePainting(int n) : n(n), timer(0), h(n, -1), fst1(n - 1, -1), fst2(n - 1, -1), adj(n), t1(0), t2(0) {}
    
    void addEdge(int u, int v, int id) { 
        adj[u].push_back({v, id}); 
        adj[v].push_back({u, id}); 
    }
    
    void build(int root = 0) {
        auto dfs = [&](auto &&dfs, int u, int p, int d) -> void {
            h[u] = d; 
            dfs_list.push_back(u);
            for (auto [v, id] : adj[u]) {
                if (v != p) {
                    edge_list.push_back(id); 
                    dfs(dfs, v, u, d + 1);
                    edge_list.push_back(id); 
                    dfs_list.push_back(u);
                }
            }
        };
        dfs(dfs, root, -1, 0);
        
        for (int i = 0; i < int(edge_list.size()); i++) {
            int id = edge_list[i];
            if (fst1[id] == -1) {
                fst1[id] = i; 
            } else {
                fst2[id] = i;
            }
        }
        t1 = SegTree(int(edge_list.size())); 
        t2 = SegTree(int(edge_list.size()));
    }
    
    void paintEdge(int id, bool paint) {
        t1.update(fst1[id], fst1[id], paint ? 1 : 0);
        t2.update(fst2[id], fst2[id], paint ? 1 : 0);
    }
    
    int queryAncPath(int anc, int dec, const vector<int> &fst_node) {
        int l = fst_node[anc], r = fst_node[dec] - 1;
        if (l > r) {
            return 0;
        }
        
        return t1.query(l, r) - t2.query(l, r);
    }
};

// T: O(V^2 * E), M: O(V + E)
template<typename T>
struct Dinic {
    struct FlowEdge { int to; T cap, flow; int rev; };
    int n, s, t; 
    vector<vector<FlowEdge>> adj; 
    vector<int> lvl, ptr;
    
    Dinic(int n, int s, int t) : n(n), s(s), t(t), adj(n), lvl(n), ptr(n) {}
    
    void addEdge(int u, int v, T cap) {
        adj[u].push_back({v, cap, 0, int(adj[v].size())});
        adj[v].push_back({u, 0, 0, int(adj[u].size()) - 1});
    }
    
    bool bfs() {
        fill(lvl.begin(), lvl.end(), -1); 
        queue<int> q; 
        lvl[s] = 0; 
        q.push(s);
        
        while (!q.empty()) {
            int u = q.front(); 
            q.pop();
            for (auto &e : adj[u]) {
                if (e.cap - e.flow > 0 && lvl[e.to] == -1) { 
                    lvl[e.to] = lvl[u] + 1; 
                    q.push(e.to); 
                }
            }
        }
        
        return lvl[t] != -1;
    }
    
    T dfs(int u, T pushed) {
        if (pushed == 0 || u == t) {
            return pushed;
        }
        for (int &i = ptr[u]; i < int(adj[u].size()); i++) {
            auto &e = adj[u][i];
            if (lvl[u] + 1 != lvl[e.to] || e.cap - e.flow == 0) {
                continue;
            }
            T tr = dfs(e.to, min(pushed, e.cap - e.flow));
            if (tr == 0) {
                continue;
            }
            e.flow += tr; 
            adj[e.to][e.rev].flow -= tr;
            
            return tr;
        }
        
        return 0;
    }
    
    T maxFlow() {
        T flow = 0;
        while (bfs()) {
            fill(ptr.begin(), ptr.end(), 0);
            while (T pushed = dfs(s, std::numeric_limits<T>::max())) {
                flow += pushed;
            }
        }
        
        return flow;
    }
};

// S: O(V + E), U: NA, Q: O(V^2 * E), M: O(V + E)
template<typename T>
struct FlowWithDemands {
    int n, s_prime, t_prime; 
    Dinic<T> dinic; 
    vector<T> d_in, d_out; 
    T sum_demands;
    
    FlowWithDemands(int n) : n(n), s_prime(n), t_prime(n + 1), dinic(n + 2, n, n + 1), d_in(n, 0), d_out(n, 0), sum_demands(0) {}
    
    void addEdge(int u, int v, T cap, T demand) {
        d_in[v] += demand; 
        d_out[u] += demand; 
        dinic.addEdge(u, v, cap - demand);
    }
    
    bool hasValidFlow(int s, int t) {
        dinic.addEdge(t, s, std::numeric_limits<T>::max());
        for (int i = 0; i < n; i++) {
            if (d_in[i] > 0) { 
                dinic.addEdge(s_prime, i, d_in[i]); 
                sum_demands += d_in[i]; 
            }
            if (d_out[i] > 0) { 
                dinic.addEdge(i, t_prime, d_out[i]); 
            }
        }
        
        return dinic.maxFlow() == sum_demands;
    }
};

// S: O(F * V * E), U: NA, Q: O(1), M: O(V^2)
template<typename T, typename C>
struct MinCostMaxFlow {
    struct MCFEdge { int u, v; T cap; C cost; };
    int n; 
    vector<MCFEdge> edges; 
    vector<vector<int>> adj; 
    vector<vector<C>> cost; 
    vector<vector<T>> cap;
    
    MinCostMaxFlow(int n, const vector<MCFEdge> &E) : n(n), edges(E), adj(n), cost(n, vector<C>(n, 0)), cap(n, vector<T>(n, 0)) {
        for (const auto &e : edges) {
            adj[e.u].push_back(e.v); 
            adj[e.v].push_back(e.u);
            cost[e.u][e.v] = e.cost; 
            cost[e.v][e.u] = -e.cost;
            cap[e.u][e.v] = e.cap;
        }
    }
    
    pair<T, C> solve(int s, int t, T k) {
        T flow = 0; 
        C min_cost = 0; 
        vector<C> d(n); 
        vector<int> p(n);
        
        auto spfa = [&]() -> bool {
            d.assign(n, std::numeric_limits<C>::max()); 
            p.assign(n, -1); 
            d[s] = 0;
            vector<bool> in_q(n, false); 
            queue<int> q; 
            q.push(s); 
            in_q[s] = true;
            
            while (!q.empty()) {
                int u = q.front(); 
                q.pop(); 
                in_q[u] = false;
                for (int v : adj[u]) {
                    if (cap[u][v] > 0 && d[u] + cost[u][v] < d[v]) {
                        d[v] = d[u] + cost[u][v]; 
                        p[v] = u;
                        if (!in_q[v]) { 
                            in_q[v] = true; 
                            q.push(v); 
                        }
                    }
                }
            }
            
            return d[t] != std::numeric_limits<C>::max();
        };
        
        while (flow < k) {
            if (!spfa()) {
                break;
            }
            T f = k - flow; 
            int cur = t;
            while (cur != s) { 
                f = min(f, cap[p[cur]][cur]); 
                cur = p[cur]; 
            }
            
            flow += f; 
            min_cost += f * d[t]; 
            cur = t;
            
            while (cur != s) { 
                cap[p[cur]][cur] -= f; 
                cap[cur][p[cur]] += f; 
                cur = p[cur]; 
            }
        }
        
        return {flow, min_cost};
    }
};

// S: O(V^2 * sqrt(E)) to O(V^3), U: NA, Q: O(1), M: O(V^2)
template<typename T>
struct PushRelabel {
    int n; 
    vector<vector<T>> cap, flow; 
    vector<int> h; 
    vector<T> ex;
    
    PushRelabel(int n) : n(n), cap(n, vector<T>(n, 0)), flow(n, vector<T>(n, 0)), h(n, 0), ex(n, 0) {}
    
    void addEdge(int u, int v, T c) { 
        cap[u][v] += c; 
    }
    
    T solve(int s, int t) {
        auto push = [&](int u, int v) -> void {
            T d = min(ex[u], cap[u][v] - flow[u][v]);
            flow[u][v] += d; 
            flow[v][u] -= d; 
            ex[u] -= d; 
            ex[v] += d;
        };
        
        auto relabel = [&](int u) -> void {
            int d = INF32;
            for (int i = 0; i < n; i++) {
                if (cap[u][i] - flow[u][i] > 0) {
                    d = min(d, h[i]);
                }
            }
            if (d < INF32) {
                h[u] = d + 1;
            }
        };
        
        auto getMaxH = [&]() -> vector<int> {
            vector<int> max_h;
            for (int i = 0; i < n; i++) {
                if (i != s && i != t && ex[i] > 0) {
                    if (!max_h.empty() && h[i] > h[max_h[0]]) {
                        max_h.clear();
                    }
                    if (max_h.empty() || h[i] == h[max_h[0]]) {
                        max_h.push_back(i);
                    }
                }
            }
            
            return max_h;
        };
        
        h[s] = n; 
        ex[s] = std::numeric_limits<T>::max();
        for (int i = 0; i < n; i++) {
            if (i != s) {
                push(s, i);
            }
        }
            
        vector<int> cur;
        while (!(cur = getMaxH()).empty()) {
            for (int i : cur) {
                bool pushed = false;
                for (int j = 0; j < n && ex[i]; j++) {
                    if (cap[i][j] - flow[i][j] > 0 && h[i] == h[j] + 1) { 
                        push(i, j); 
                        pushed = true; 
                    }
                }
                if (!pushed) { 
                    relabel(i); 
                    break; 
                }
            }
        }
        
        return ex[t];
    }
};

// T: O(V * E), M: O(V + E)
struct Kuhn {
    int n, m; 
    vector<vector<int>> adj; 
    vector<int> mt; 
    vector<bool> used;
    
    Kuhn(int n, int m) : n(n), m(m), adj(n), mt(m, -1) {}
    
    void addEdge(int u, int v) { 
        adj[u].push_back(v); 
    }
    
    bool dfs(int u) {
        if (used[u]) {
            return false;
        }
        used[u] = true;
        for (int v : adj[u]) {
            if (mt[v] == -1 || dfs(mt[v])) { 
                mt[v] = u; 
                return true; 
            }
        }
        
        return false;
    }
    
    int solve() {
        vector<bool> used1(n, false);
        for (int u = 0; u < n; u++) {
            for (int v : adj[u]) {
                if (mt[v] == -1) { 
                    mt[v] = u; 
                    used1[u] = true; 
                    break; 
                }
            }
        }
        
        int matches = 0;
        for (int u = 0; u < n; u++) {
            if (used1[u]) { 
                matches++; 
                continue; 
            }
            used.assign(n, false); 
            if (dfs(u)) {
                matches++;
            }
        }
        
        return matches;
    }
};

// ==============================================================================
// MATH
// ==============================================================================

// T: O(log p), M: O(1)
lng modPow(lng b, lng p, lng mod) {
    lng res = 1 % mod; 
    b %= mod;
    while (p > 0) { 
        if (p & 1) {
            res = (lng)((lll)res * b % mod); 
        }
        b = (lng)((lll)b * b % mod); 
        p >>= 1; 
    }
    
    return res;
}

// T: O(log(min(a, b))), M: O(1)
lng exGcd(lng a, lng b, lng &X, lng &Y) {
    X = 1; 
    Y = 0; 
    lng X1 = 0, Y1 = 1;
    while (b != 0) {
        lng q = a / b;
        X1 = std::exchange(X, X1) - q * X1;
        Y1 = std::exchange(Y, Y1) - q * Y1;
        b = std::exchange(a, b) - q * b;
    }
    
    return a;
}

// S: O(n log log n), U: NA, Q: O(1), M: O(n)
vector<bool> SieveOfErath(int n) {
    vector<bool> is_prm(max(n + 1, 2), true); 
    is_prm[0] = is_prm[1] = false;
    for (int i = 2; i * i <= n; i++) {
        if (is_prm[i]) {
            for (int j = i * i; j <= n; j += i) {
                is_prm[j] = false;
            }
        }
    }
    
    return is_prm;
}

// S: O(n), U: NA, Q: O(1), M: O(n)
struct LinearSieve {
    int n; 
    vector<int> prms, spf, mu, phi; 
    vector<lng> sum_div, num_div, spf_exp, spf_exp_sum;
    
    LinearSieve(int N) : n(N), spf(N + 1, 0) {
        for (int i = 2; i <= n; i++) {
            if (spf[i] == 0) { 
                spf[i] = i; 
                prms.push_back(i); 
            }
            for (int p : prms) { 
                if (p > spf[i] || i * p > n) {
                    break; 
                }
                spf[i * p] = p; 
            }
        }
    }
    
    void getNumDiv() {
        num_div.assign(n + 1, 1); 
        spf_exp.assign(n + 1, 0);
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) { 
                spf_exp[i] = spf_exp[j] + 1; 
                num_div[i] = num_div[j] / spf_exp[i] * (spf_exp[i] + 1); 
            } else { 
                spf_exp[i] = 1; 
                num_div[i] = num_div[j] * 2; 
            }
        }
    }
    
    void getSumDiv() {
        sum_div.assign(n + 1, 1); 
        spf_exp_sum.assign(n + 1, 0);
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) { 
                spf_exp_sum[i] = spf_exp_sum[j] * p + 1; 
                sum_div[i] = sum_div[j] / spf_exp_sum[j] * spf_exp_sum[i]; 
            } else { 
                spf_exp_sum[i] = p + 1; 
                sum_div[i] = sum_div[j] * (p + 1); 
            }
        }
    }
    
    void getPhi() {
        phi.assign(n + 1, 0); 
        if (n >= 1) {
            phi[1] = 1;
        }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                phi[i] = phi[j] * p; 
            } else {
                phi[i] = phi[j] * (p - 1);
            }
        }
    }
    
    void getMu() {
        mu.assign(n + 1, 0); 
        if (n >= 1) {
            mu[1] = 1;
        }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                mu[i] = 0; 
            } else {
                mu[i] = -mu[j];
            }
        }
    }
};

// T: O(log n), M: O(1)
bool isPrimeMR(lng n) {
    if (n < 2) {
        return false;
    }
    for (lng p : {2, 3, 5, 7, 11, 13, 17, 19, 23}) {
        if (n % p == 0) {
            return n == p;
        }
    }
    lng d = n - 1; 
    int s = __builtin_ctzll(d); 
    d >>= s;
    for (lng a : {2, 325, 9375, 28178, 450775, 9780504, 1795265022}) {
        if (a % n == 0) {
            continue;
        }
        lng x = modPow(a, d, n);
        if (x == 1 || x == n - 1) {
            continue;
        }
        bool comp = true;
        for (int r = 1; r < s; r++) { 
            x = (lng)((lll)x * x % n); 
            if (x == n - 1) { 
                comp = false; 
                break; 
            } 
        }
        if (comp) {
            return false;
        }
    }
    
    return true;
}

// T: O(sqrt(n)), M: O(1)
void getPrimeFacSlow(lng n, vector<lng> &res) {
    while (n % 2 == 0) { res.push_back(2); n /= 2; }
    while (n % 3 == 0) { res.push_back(3); n /= 3; }
    while (n % 5 == 0) { res.push_back(5); n /= 5; }
    int inc[8] = {4, 2, 4, 2, 4, 6, 2, 6};
    for (lng i = 7, j = 0; i * i <= n; i += inc[j], j = (j + 1) & 7) {
        while (n % i == 0) { 
            res.push_back(i); 
            n /= i; 
        }
    }
    if (n > 1) {
        res.push_back(n);
    }
}

// T: O(n^(1/4)), M: O(1)
lng getOneFacBPR(lng n) {
    if (n % 2 == 0) {
        return 2;
    }
    if (isPrimeMR(n)) {
        return n;
    }
    auto f = [&](lng x) { 
        return (lng)(((lll)x * x + 1) % n); 
    };
    lng x = 0, y = 0, prd = 2, q, g;
    for (int i = 1; (i & 127) || (g = std::gcd(prd, n)) == 1; i++) {
        if (i == (i & -i)) {
            y = x;
        }
        x = f(x); 
        q = (lng)((lll)prd * (x > y ? x - y : y - x) % n);
        if (q != 0) {
            prd = q;
        }
    }
    if (g == n) { 
        g = 1; 
        while (g == 1) { 
            y = f(y); 
            g = std::gcd(x > y ? x - y : y - x, n); 
        } 
    }
    
    return g;
}

// T: O(n^(1/4)), M: O(log n)
mt19937_64 prm_rng(chrono::steady_clock::now().time_since_epoch().count());
void getPrimeFacFast(lng n, vector<lng> &res) {
    if (n <= 1) {
        return;
    }
    if (n <= 100) {
        return getPrimeFacSlow(n, res);
    }
    if (isPrimeMR(n)) { 
        res.push_back(n); 
        return; 
    }
    lng f = getOneFacBPR(n);
    getPrimeFacFast(f, res); 
    getPrimeFacFast(n / f, res);
}

// T: O(n^(1/4) + d), M: O(d)
vector<lng> getAllFac(lng n) {
    vector<lng> prms; 
    getPrimeFacFast(n, prms); 
    std::sort(prms.begin(), prms.end());
    
    vector<pair<lng, int>> pows;
    for (lng p : prms) { 
        if (pows.empty() || pows.back().first != p) {
            pows.push_back({p, 1}); 
        } else {
            pows.back().second++; 
        }
    }
    
    vector<lng> res = {1};
    for (auto [p, c] : pows) {
        int sz = int(res.size()); 
        lng cur = 1;
        for (int i = 0; i < c; i++) { 
            cur *= p; 
            for (int j = 0; j < sz; j++) {
                res.push_back(res[j] * cur); 
            }
        }
    }
    
    return res;
}

// T: O(n^(1/4)), M: O(1)
lng getNumDiv(lng n, bool opt = false) {
    if (!opt) {
        lng res = 1;
        for (lng i = 2; i * i <= n; i++) {
            if (n % i == 0) { 
                lng p = 0; 
                do { 
                    p++; 
                    n /= i; 
                } while (n % i == 0); 
                res *= (p + 1); 
            }
        }
        if (n > 1) {
            res *= 2;
        }
        
        return res;
    }
    
    vector<lng> prms; 
    getPrimeFacFast(n, prms); 
    std::sort(prms.begin(), prms.end());
    prms.erase(std::unique(prms.begin(), prms.end()), prms.end());
    lng res = 1;
    for (lng p : prms) { 
        lng cnt = 0; 
        do { 
            cnt++; 
            n /= p; 
        } while (n % p == 0); 
        res *= (cnt + 1); 
    }
    
    return res;
}

// T: O(n^(1/4)), M: O(1)
lng getSumDiv(lng n, bool opt = false) {
    if (!opt) {
        lng res = 1;
        for (lng i = 2; i * i <= n; i++) {
            if (n % i == 0) { 
                lng pow = 1, sum = 1; 
                do { 
                    pow *= i; 
                    sum += pow; 
                    n /= i; 
                } while (n % i == 0); 
                res *= sum; 
            }
        }
        if (n > 1) {
            res *= (1 + n);
        }
        
        return res;
    }
    
    vector<lng> prms; 
    getPrimeFacFast(n, prms); 
    std::sort(prms.begin(), prms.end());
    prms.erase(std::unique(prms.begin(), prms.end()), prms.end());
    lll res = 1;
    for (lng p : prms) { 
        lng pow = 1; 
        lll sum = 1; 
        do { 
            pow *= p; 
            sum += pow; 
            n /= p; 
        } while (n % p == 0); 
        res *= sum; 
    }
    
    return res;
}

// T: O(n^(1/4)), M: O(1)
lng getPhi(lng n, bool opt = false) {
    if (!opt) {
        lng res = n;
        for (lng i = 2; i * i <= n; i++) {
            if (n % i == 0) { 
                while (n % i == 0) {
                    n /= i; 
                }
                res -= res / i; 
            }
        }
        if (n > 1) {
            res -= res / n;
        }
        
        return res;
    }
    
    vector<lng> prms; 
    getPrimeFacFast(n, prms); 
    std::sort(prms.begin(), prms.end());
    prms.erase(std::unique(prms.begin(), prms.end()), prms.end());
    lng res = n; 
    for (lng p : prms) {
        res -= res / p;
    }
    
    return res;
}

// T: O(log n), M: O(1)
lng getInvE(lng n, lng mod) {
    n = (n % mod + mod) % mod;
    lng X, Y, g = exGcd(n, mod, X, Y);
    
    return g == 1 ? (X % mod + mod) % mod : -1;
}

// T: O(log n), M: O(1)
lng getInvP(lng n, lng mod) {
    return n <= 1 ? n : mod - (lng)((lll)mod / n * getInvP(mod % n, mod) % mod);
}

// T: O(n * log(lcm)), M: O(1)
pair<lng, lng> exChiRemThm(const vector<lng> &a, const vector<lng> &m) {
    lng A = a[0], M = m[0], X, Y;
    for (int i = 1; i < int(a.size()); i++) {
        lng d = ((a[i] - A) % m[i] + m[i]) % m[i], g = exGcd(M, m[i], X, Y);
        if (d % g != 0) {
            return {-1, -1};
        }
        lng step = m[i] / g;
        A += M * (((X % step * (d / g) % step) + step) % step);
        M *= step;
    }
    
    return {A, M};
}

// T: O(sqrt(mod)), M: O(sqrt(mod))
lng modLog(lng a, lng b, lng mod) {
    a %= mod; 
    b %= mod; 
    lng add = 0, mul = 1, g;
    
    while ((g = std::gcd(a, mod)) > 1) {
        if (b == mul) {
            return add;
        }
        if (b % g != 0) {
            return -1;
        }
        b /= g; 
        mod /= g; 
        add++; 
        mul = (lng)((lll)mul * (a / g) % mod);
    }
    
    lng n = sqrt(mod) + 1, cur = b; 
    unordered_map<lng, lng> mp;
    for (lng i = 0; i <= n; i++) { 
        mp[cur] = i; 
        cur = (lng)((lll)cur * a % mod); 
    }
    
    lng a_n = modPow(a, n, mod); 
    cur = mul;
    for (lng i = 1; i <= n; i++) { 
        cur = (lng)((lll)cur * a_n % mod); 
        if (mp.count(cur)) {
            return i * n - mp[cur] + add; 
        }
    }
    
    return -1;
}

// T: O(p^(1/4) * log p), M: O(log p)
lng getPrimRoot(lng mod) {
    lng mod_cur = mod; 
    if (mod_cur % 2 == 0) {
        mod_cur /= 2;
    }
    if (mod_cur % 2 == 0 && mod_cur != 2) {
        return -1;
    }
        
    vector<lng> prms; 
    getPrimeFacFast(mod_cur, prms);
    if (!prms.empty() && prms[0] != prms.back()) {
        return -1;
    }
        
    prms.clear(); 
    getPrimeFacFast(getPhi(mod, true), prms);
    std::sort(prms.begin(), prms.end()); 
    prms.erase(std::unique(prms.begin(), prms.end()), prms.end());
    
    for (lng r = 1; r < mod; r++) {
        if (std::gcd(r, mod) > 1) {
            continue;
        }
        bool valid = true;
        for (lng p : prms) {
            if (modPow(r, getPhi(mod, true) / p, mod) == 1) { 
                valid = false; 
                break; 
            }
        }
        if (valid) {
            return r;
        }
    }
    
    return -1;
}

// T: O(p^(1/4) * log p), M: O(log p)
vector<lng> modRoot(lng a, lng k, lng mod) {
    if (a == 0) {
        return {0};
    }
    lng g = getPrimRoot(mod); 
    if (g == -1) {
        return {};
    }
    lng phi = getPhi(mod, true), y = modLog(modPow(g, k, mod), a, mod);
    if (y == -1) {
        return {};
    }
        
    lng delta = phi / std::gcd(k, phi); 
    vector<lng> res;
    for (lng cur = y % delta; cur < phi; cur += delta) {
        res.push_back(modPow(g, cur, mod));
    }
    std::sort(res.begin(), res.end());
    
    return res;
}

// T: O(log(min(a, b))), M: O(1)
lng getFloorSum(lng n, lng m, lng a, lng b) {
    lng res = 0;
    if (a >= m) { 
        res += (n - 1) * n * (a / m) / 2; 
        a %= m; 
    }
    if (b >= m) { 
        res += n * (b / m); 
        b %= m; 
    }
    lng y_max = (a * n + b) / m;
    if (y_max == 0) {
        return res;
    }
        
    return res + getFloorSum(y_max, a, m, (a * n + b) % m);
}

// S: O(n), U: NA, Q: O(1), M: O(n)
template<typename T>
struct ModFac {
    int n; 
    vector<T> fac, inv_fac;
    
    ModFac(int n): n(n) {
        fac.resize(n + 1); 
        fac[0] = 1;
        for (int i = 1; i <= n; i++) {
            fac[i] = fac[i - 1] * T(i);
        }
        inv_fac.resize(n + 1); 
        inv_fac[n] = T(1) / fac[n];
        for (int i = n - 1; i >= 0; i--) {
            inv_fac[i] = inv_fac[i + 1] * T(i + 1);
        }
    }
    
    T combiNR(int x, int y) const { 
        if (y > x || y < 0) {
            return T(0); 
        }
        return fac[x] * inv_fac[y] * inv_fac[x - y]; 
    }
    
    T combiWR(int x, int y) const { 
        return combiNR(x + y - 1, y); 
    }
    
    T permuNR(int x, int y) const { 
        if (y > x || y < 0) {
            return T(0); 
        }
        return fac[x] * inv_fac[x - y]; 
    }
};

// S: NA, U: O(n log n), Q: NA, M: O(1)
struct FastConv {
    template<typename T> static void fctOr(vector<T> &a, bool inv) {
        int n = int(a.size());
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    if (!inv) {
                        a[i + j + len] += a[i + j]; 
                    } else {
                        a[i + j + len] -= a[i + j];
                    }
                }
            }
        }
    }
    
    template<typename T> static void fctAnd(vector<T> &a, bool inv) {
        int n = int(a.size());
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    if (!inv) {
                        a[i + j] += a[i + j + len]; 
                    } else {
                        a[i + j] -= a[i + j + len];
                    }
                }
            }
        }
    }
    
    template<typename T> static void fctXor(vector<T> &a, bool inv) {
        int n = int(a.size());
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    T u = a[i + j], v = a[i + j + len]; 
                    a[i + j] = u + v; 
                    a[i + j + len] = u - v;
                }
            }
        }
        if (inv) { 
            T ci = T(1) / T(n); 
            for (auto &x : a) {
                x *= ci; 
            }
        }
    }
    
    template<typename T> static void fctGcdSlow(vector<T> &a, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[i] += a[j];
                }
            }
        } else {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[i] -= a[j];
                }
            }
        }
    }
    
    template<typename T> static void fctGcd(vector<T> &a, const vector<int> &prms, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = n / p; i >= 1; i--) {
                    a[i] += a[i * p]; 
                }
            } 
        } else { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = 1; i <= n / p; i++) {
                    a[i] -= a[i * p]; 
                }
            } 
        }
    }
    
    template<typename T> static void fctLcmSlow(vector<T> &a, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[j] += a[i];
                }
            }
        } else {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) {
                    a[j] -= a[i];
                }
            }
        }
    }
    
    template<typename T> static void fctLcm(vector<T> &a, const vector<int> &prms, bool inv) {
        int n = int(a.size()) - 1;
        if (!inv) { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = 1; i <= n / p; i++) {
                    a[i * p] += a[i]; 
                }
            } 
        } else { 
            for (int p : prms) { 
                if (p > n) break; 
                for (int i = n / p; i >= 1; i--) {
                    a[i * p] -= a[i]; 
                }
            } 
        }
    }
};

// S: O(n^3), U: O(n^3), Q: O(1), M: O(n^2)
template<typename T>
struct Matrix {
    int n, m; 
    vector<T> v;
    
    Matrix() : n(0), m(0) {}
    Matrix(int n, int m) : n(n), m(m), v(n * m, T(0)) {}
    Matrix(const vector<vector<T>> &a) : n(int(a.size())), m(a.empty() ? 0 : int(a[0].size())), v(n * m) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                v[i * m + j] = a[i][j];
            }
        }
    }
    
    T *operator[](int i) { return &v[i * m]; }
    const T *operator[](int i) const { return &v[i * m]; }
    
    static bool isNil(const T &x) {
        if constexpr (std::is_floating_point_v<T>) {
            return -1e-9 < x && x < 1e-9;
        } else {
            return x == T(0);
        }
    }
    
    static Matrix eye(int N, int M) {
        Matrix res(N, M); 
        for (int i = 0; i < min(N, M); i++) {
            res[i][i] = T(1);
        }
        return res;
    }
    
    Matrix operator*(const Matrix &o) const {
        assert(m == o.n); 
        Matrix res(n, o.m);
        for (int i = 0; i < n; i++) {
            for (int k = 0; k < m; k++) {
                if (isNil(v[i * m + k])) {
                    continue;
                }
                for (int j = 0; j < o.m; j++) {
                    res[i][j] += v[i * m + k] * o[k][j];
                }
            }
        }
        
        return res;
    }
    
    Matrix &operator*=(const Matrix &o) { 
        return *this = *this * o; 
    }
    
    Matrix pow(lng e) const {
        assert(n == m); 
        Matrix res = eye(n, m), A = *this, tmp(n, m);
        auto mul = [&](Matrix &a, const Matrix &b) {
            fill(tmp.v.begin(), tmp.v.end(), T(0));
            for (int i = 0; i < n; i++) {
                for (int k = 0; k < n; k++) {
                    if (isNil(a[i][k])) continue;
                    for (int j = 0; j < n; j++) {
                        tmp[i][j] += a[i][k] * b[k][j];
                    }
                }
            }
            a.v.swap(tmp.v);
        };
        while (e > 0) { 
            if (e & 1) mul(res, A); 
            e >>= 1; 
            mul(A, A); 
        }
        
        return res;
    }
    
    T det() const {
        assert(n == m); 
        Matrix a = *this; 
        T res = T(1);
        for (int i = 0; i < n; i++) {
            int piv = i; 
            while (piv < n && isNil(a[piv][i])) piv++;
            if (piv == n) {
                return T(0);
            }
            if (i != piv) { 
                std::swap_ranges(&a[i * n], &a[i * n] + n, &a[piv * n]); 
                res = -res; 
            }
            if constexpr (requires { T::is_prime; }) {
                T d = T(1) / a[i][i]; 
                res *= a[i][i];
                for (int j = i; j < n; j++) {
                    a[i][j] *= d;
                }
                for (int j = i + 1; j < n; j++) { 
                    T f = a[j][i]; 
                    for (int k = i; k < n; k++) {
                        a[j][k] -= f * a[i][k]; 
                    }
                }
            } else {
                for (int j = i + 1; j < n; j++) {
                    while (!isNil(a[j][i])) {
                        T q = a[i][i] / a[j][i];
                        for (int k = i; k < n; k++) {
                            a[i][k] -= q * a[j][k];
                        }
                        std::swap_ranges(&a[i * n], &a[i * n] + n, &a[j * n]); 
                        res = -res;
                    }
                }
                res *= a[i][i];
            }
        }
        
        return res;
    }
    
    Matrix solveLin(Matrix b) const {
        assert(n == b.n); 
        Matrix a = *this, res(m, b.m); 
        vector<int> pivs(m, -1); 
        int r = 0;
        for (int c = 0; c < m && r < n; c++) {
            int piv = r; 
            while (piv < n && isNil(a[piv][c])) piv++;
            if (piv == n) {
                continue;
            }
            if (r != piv) {
                std::swap_ranges(&a[r * m], &a[r * m] + m, &a[piv * m]);
                std::swap_ranges(&b[r * b.m], &b[r * b.m] + b.m, &b[piv * b.m]);
            }
            T d = T(1) / a[r][c];
            for (int j = c; j < m; j++) {
                a[r][j] *= d;
            }
            for (int j = 0; j < b.m; j++) {
                b[r][j] *= d;
            }
            for (int i = 0; i < n; i++) {
                if (i != r && !isNil(a[i][c])) {
                    T f = a[i][c];
                    for (int j = c; j < m; j++) {
                        a[i][j] -= f * a[r][j];
                    }
                    for (int j = 0; j < b.m; j++) {
                        b[i][j] -= f * b[r][j];
                    }
                }
            }
            pivs[c] = r++;
        }
        for (int i = r; i < n; i++) {
            for (int j = 0; j < b.m; j++) {
                if (!isNil(b[i][j])) {
                    return Matrix();
                }
            }
        }
        for (int c = 0; c < m; c++) {
            if (pivs[c] != -1) {
                std::copy_n(&b[pivs[c] * b.m], b.m, &res[c * res.m]);
            }
        }
        
        return res;
    }
};

// T: O(n^3 * log(k)), M: O(n^2)
vector<vector<lng>> minPlusMatrixExp(vector<vector<lng>> a, lng k) {
    int n = int(a.size()); 
    vector<vector<lng>> res(n, vector<lng>(n, INF64));
    for (int i = 0; i < n; i++) {
        res[i][i] = 0;
    }
    auto mul = [&](const vector<vector<lng>> &A, const vector<vector<lng>> &B) -> vector<vector<lng>> {
        vector<vector<lng>> C(n, vector<lng>(n, INF64));
        for (int i = 0; i < n; i++) {
            for (int m = 0; m < n; m++) {
                if (A[i][m] == INF64) continue;
                for (int j = 0; j < n; j++) {
                    if (B[m][j] != INF64) {
                        chmin(C[i][j], A[i][m] + B[m][j]);
                    }
                }
            }
        }
        return C;
    };
    while (k > 0) { 
        if (k & 1) res = mul(res, a); 
        k >>= 1; 
        if (k > 0) a = mul(a, a); 
    }
    
    return res;
}

// T: O(n^3), M: O(n^2)
template<typename T>
T matrixTreeTheorem(int n, const vector<vector<T>> &adj) {
    if (n == 1) {
        return T(1);
    }
    vector<vector<T>> L(n - 1, vector<T>(n - 1, 0));
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1; j++) {
            if (i == j) { 
                T deg = 0; 
                for (int k = 0; k < n; k++) {
                    deg += adj[i][k]; 
                }
                L[i][i] = deg - adj[i][i]; 
            } else {
                L[i][j] = -adj[i][j];
            }
        }
    }
    T det = 1; 
    int sz = n - 1;
    for (int i = 0; i < sz; i++) {
        int piv = i; 
        while (piv < sz && L[piv][i] == T(0)) piv++;
        if (piv == sz) {
            return T(0);
        }
        if (i != piv) { 
            std::swap(L[i], L[piv]); 
            det = -det; 
        }
        det *= L[i][i]; 
        T inv_piv = T(1) / L[i][i];
        for (int j = i + 1; j < sz; j++) {
            if (L[j][i] != T(0)) {
                T factor = L[j][i] * inv_piv;
                for (int k = i + 1; k < sz; k++) {
                    L[j][k] -= L[i][k] * factor;
                }
            }
        }
    }
    
    return det;
}

// T: O(n^2 * m), M: O(n * m)
template<typename T>
struct Hungarian {
    int n, m; 
    vector<vector<T>> a; 
    vector<T> u, v; 
    vector<int> p, way;
    
    Hungarian(int n, int m) : n(n), m(m), a(n + 1, vector<T>(m + 1, 0)), u(n + 1, 0), v(m + 1, 0), p(m + 1, 0), way(m + 1, 0) {}
    
    void addEdge(int i, int j, T w) { 
        a[i + 1][j + 1] = w; 
    }
    
    T solve() {
        for (int i = 1; i <= n; i++) {
            p[0] = i; 
            int j0 = 0; 
            vector<T> minv(m + 1, INF64); 
            vector<bool> used(m + 1, false);
            do {
                used[j0] = true; 
                int i0 = p[j0], j1 = 0; 
                T delta = INF64;
                for (int j = 1; j <= m; j++) {
                    if (!used[j]) {
                        T cur = a[i0][j] - u[i0] - v[j];
                        if (cur < minv[j]) { 
                            minv[j] = cur; 
                            way[j] = j0; 
                        }
                        if (minv[j] < delta) { 
                            delta = minv[j]; 
                            j1 = j; 
                        }
                    }
                }
                for (int j = 0; j <= m; j++) { 
                    if (used[j]) { 
                        u[p[j]] += delta; 
                        v[j] -= delta; 
                    } else {
                        minv[j] -= delta; 
                    }
                }
                j0 = j1;
            } while (p[j0] != 0);
            
            do { 
                int j1 = way[j0]; 
                p[j0] = p[j1]; 
                j0 = j1; 
            } while (j0 != 0);
        }
        
        return -v[0];
    }
    
    vector<int> getAssignment() {
        vector<int> ans(n, -1); 
        for (int j = 1; j <= m; j++) {
            if (p[j] != 0) {
                ans[p[j] - 1] = j - 1;
            }
        }
        
        return ans;
    }
};

// ==============================================================================
// STRINGS & HASHING
// ==============================================================================

// S: NA, U: NA, Q: O(1), M: O(1)
struct CustomHash {
    using ulng = unsigned long long;
    static inline const ulng rnd = chrono::steady_clock::now().time_since_epoch().count() + reinterpret_cast<uintptr_t>(&rnd);
    
    static ulng splitMix64(ulng x) { 
        x += 0x9e3779b97f4a7c15ULL; 
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL; 
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL; 
        
        return x ^ (x >> 31); 
    }
    
    template<typename T> 
    requires std::is_convertible_v<T, ulng> 
    size_t operator()(const T &x) const { 
        return splitMix64(ulng(x) + rnd); 
    }
    
    size_t operator()(std::string_view x) const {
        ulng hash = 5381 + rnd;
        for (char y : x) {
            hash = ((hash << 5) + hash) ^ y;
        }
        
        return splitMix64(hash);
    }
    
    template<typename T1, typename T2> 
    size_t operator()(const pair<T1, T2> &x) const { 
        return (splitMix64(ulng(x.first) + rnd) << 3) ^ (splitMix64(ulng(x.second) + rnd) >> 1); 
    }
    
    template<typename ...Ts> 
    size_t operator()(const tuple<Ts...> &x) const {
        size_t seed = 0; 
        std::apply([&](const auto &...args) { 
            ((seed = ((seed << 3) ^ (splitMix64(ulng(args) + rnd) >> 1)) + 0x9e3779b9), ...); 
        }, x);
        
        return seed;
    }
    
    template<std::ranges::range T> 
    requires (!std::is_convertible_v<T, std::string_view>) 
    size_t operator()(const T &x) const {
        size_t seed = 0; 
        for (const auto &y : x) {
            seed ^= (*this)(y) + (seed << 6) + (seed >> 2) + 0x9e3779b9;
        }
        
        return seed;
    }
};

template<typename K, typename V> 
using safe_unordered_map = unordered_map<K, V, CustomHash>;
template<typename T> 
using safe_unordered_set = unordered_set<T, CustomHash>;

// S: O(sum of len), U: O(len), Q: O(len), M: O(sigma * nodes)
struct Trie {
    vector<array<int, 26>> next; 
    vector<int> end_cnt, pass_cnt;
    
    Trie() { 
        addNode(); 
    }
    
    int addNode() { 
        next.push_back({}); 
        next.back().fill(-1); 
        end_cnt.push_back(0); 
        pass_cnt.push_back(0); 
        
        return int(next.size()) - 1; 
    }
    
    void insert(const string &s) {
        int u = 0; 
        pass_cnt[u]++;
        for (char c : s) { 
            int v = c - 'a'; 
            if (next[u][v] == -1) {
                next[u][v] = addNode(); 
            }
            u = next[u][v]; 
            pass_cnt[u]++; 
        }
        end_cnt[u]++;
    }
    
    bool search(const string &s) const {
        int u = 0; 
        for (char c : s) { 
            int v = c - 'a'; 
            if (next[u][v] == -1) {
                return false; 
            }
            u = next[u][v]; 
        }
        
        return end_cnt[u] > 0;
    }
    
    int countPrefix(const string &s) const {
        int u = 0; 
        for (char c : s) { 
            int v = c - 'a'; 
            if (next[u][v] == -1) {
                return 0; 
            }
            u = next[u][v]; 
        }
        
        return pass_cnt[u];
    }
    
    void erase(const string &s) {
        if (!search(s)) {
            return;
        }
        int u = 0; 
        pass_cnt[u]--;
        for (char c : s) { 
            int v = c - 'a'; 
            u = next[u][v]; 
            pass_cnt[u]--; 
        }
        end_cnt[u]--;
    }
};

// S: O(n * B), U: O(B), Q: O(B), M: O(2 * nodes)
template<int MAX_B = 30>
struct BitTrie {
    vector<array<int, 2>> next; 
    vector<int> cnt;
    
    BitTrie() { 
        addNode(); 
    }
    
    int addNode() { 
        next.push_back({-1, -1}); 
        cnt.push_back(0); 
        
        return int(next.size()) - 1; 
    }
    
    void insert(int x, int d = 1) { 
        int u = 0; 
        cnt[u] += d;
        for (int i = MAX_B; i >= 0; i--) { 
            int v = (x >> i) & 1; 
            if (next[u][v] == -1) {
                next[u][v] = addNode(); 
            }
            u = next[u][v]; 
            cnt[u] += d; 
        }
    }
    
    int getMaxXor(int x) const {
        if (cnt[0] == 0) {
            return 0;
        }
        int u = 0, res = 0;
        for (int i = MAX_B; i >= 0; i--) {
            int v = ((x >> i) & 1) ^ 1;
            if (next[u][v] != -1 && cnt[next[u][v]] > 0) { 
                res |= (1 << i); 
                u = next[u][v]; 
            } else {
                u = next[u][v ^ 1];
            }
        }
        
        return res;
    }
};

// ==============================================================================
// MISCELLANEOUS / DP
// ==============================================================================

// T: O(n), M: O(1)
lng kadane(const vector<int> &a) {
    lng max_sum = -INF64, cur_sum = -INF64;
    for (int x : a) { 
        cur_sum = max(lng(x), cur_sum + x); 
        max_sum = max(max_sum, cur_sum); 
    }
    
    return max_sum == -INF64 ? 0 : max_sum;
}

// T: O(n) avg, M: O(1)
template<typename T>
T kthOrderStat(vector<T> a, int k) {
    int l = 0, r = int(a.size()) - 1;
    while (l <= r) {
        T piv = a[std::midpoint(l, r)]; 
        int i = l, j = r;
        while (i <= j) {
            while (a[i] < piv) {
                i++;
            }
            while (a[j] > piv) {
                j--;
            }
            if (i <= j) {
                std::swap(a[i++], a[j--]);
            }
        }
        if (k <= j) {
            r = j; 
        } else if (k >= i) {
            l = i; 
        } else {
            return a[k];
        }
    }
    
    return a[k];
}

// T: O(m * n * log(n)), M: O(n)
struct DivConqDP {
    int m, n; 
    vector<lng> dp_prv, dp_cur;
    
    DivConqDP(int m, int n) : m(m), n(n), dp_prv(n, 0), dp_cur(n, 0) {}
    
    lng getCost(int l, int r) { 
        return 0; 
    } 
    
    void solve() {
        auto compute = [&](auto &&compute, int l, int r, int opt_l, int opt_r) -> void {
            if (l > r) {
                return;
            }
            int md = std::midpoint(l, r); 
            pair<lng, int> best = {INF64, -1};
            for (int k = opt_l; k <= min(md, opt_r); k++) {
                chmin(best, {(k ? dp_prv[k - 1] : 0) + getCost(k, md), k});
            }
            dp_cur[md] = best.first; 
            int opt = best.second;
            
            compute(compute, l, md - 1, opt_l, opt); 
            compute(compute, md + 1, r, opt, opt_r);
        };
        for (int i = 0; i < n; i++) {
            dp_prv[i] = getCost(0, i);
        }
        for (int i = 1; i < m; i++) { 
            compute(compute, 0, n - 1, 0, n - 1); 
            dp_prv = dp_cur; 
        }
    }
};

// T: O(n^2), M: O(n^2)
struct KnuthOptDP {
    int n; 
    vector<vector<lng>> dp; 
    vector<vector<int>> opt;
    
    KnuthOptDP(int n) : n(n), dp(n, vector<lng>(n, 0)), opt(n, vector<int>(n, 0)) {}
    
    lng getCost(int l, int r) { 
        return 0; 
    } 
    
    void solve() {
        for (int i = 0; i < n; i++) { 
            opt[i][i] = i; 
            dp[i][i] = 0; 
        }
        for (int i = n - 2; i >= 0; i--) {
            for (int j = i + 1; j < n; j++) {
                lng mn = INF64, cost = getCost(i, j);
                for (int k = opt[i][j - 1]; k <= min(j - 1, opt[i + 1][j]); k++) {
                    if (mn >= dp[i][k] + dp[k + 1][j] + cost) { 
                        opt[i][j] = k; 
                        mn = dp[i][k] + dp[k + 1][j] + cost; 
                    }
                }
                dp[i][j] = mn;
            }
        }
    }
};

// T: O(n * W), M: O(W)
lng knapsack01(int w_max, const vector<int> &w, const vector<lng> &v) {
    vector<lng> dp(w_max + 1, 0);
    for (int i = 0; i < int(w.size()); i++) {
        for (int j = w_max; j >= w[i]; j--) {
            chmax(dp[j], dp[j - w[i]] + v[i]);
        }
    }
    
    return dp[w_max];
}

// T: O(n * W), M: O(W)
lng knapsackComplete(int w_max, const vector<int> &w, const vector<lng> &v) {
    vector<lng> dp(w_max + 1, 0);
    for (int i = 0; i < int(w.size()); i++) {
        for (int j = w[i]; j <= w_max; j++) {
            chmax(dp[j], dp[j - w[i]] + v[i]);
        }
    }
        
    return dp[w_max];
}

// T: O(W * sum(log k_i)), M: O(W)
lng knapsackMultiple(int w_max, const vector<int> &w, const vector<lng> &v, const vector<int> &k) {
    vector<int> w_bin; 
    vector<lng> v_bin;
    for (int i = 0; i < int(w.size()); i++) {
        int cnt = 1, rem = k[i];
        while (rem > cnt) { 
            rem -= cnt; 
            w_bin.push_back(w[i] * cnt); 
            v_bin.push_back(v[i] * cnt); 
            cnt *= 2; 
        }
        w_bin.push_back(w[i] * rem); 
        v_bin.push_back(v[i] * rem);
    }
    
    return knapsack01(w_max, w_bin, v_bin);
}

// T: O(n log n), M: O(n)
int getLIS(const vector<int> &a) {
    vector<int> dp(a.size() + 1, INF32); 
    dp[0] = -INF32;
    for (int i = 0; i < int(a.size()); i++) {
        int l = int(upper_bound(dp.begin(), dp.end(), a[i]) - dp.begin());
        if (dp[l - 1] < a[i] && a[i] < dp[l]) {
            dp[l] = a[i];
        }
    }
    
    int ans = 0; 
    for (int l = 0; l <= int(a.size()); l++) {
        if (dp[l] < INF32) {
            ans = l;
        }
    }
        
    return ans;
}

// T: O(n * m * 2^m), M: O(n * 2^m)
lng brokenProfile(int n, int m) {
    vector<vector<lng>> dp(n + 1, vector<lng>(1 << m, 0)); 
    dp[0][0] = 1;
    
    auto calc = [&](auto &&calc, int x, int y, int mask, int next_mask) -> void {
        if (x == n) {
            return;
        }
        if (y >= m) {
            dp[x + 1][next_mask] += dp[x][mask];
        } else {
            int cur_bit = 1 << y;
            if (mask & cur_bit) {
                calc(calc, x, y + 1, mask, next_mask);
            } else {
                calc(calc, x, y + 1, mask, next_mask | cur_bit);
                if (y + 1 < m && !(mask & cur_bit) && !(mask & (cur_bit << 1))) {
                    calc(calc, x, y + 2, mask, next_mask);
                }
            }
        }
    };
    
    for (int x = 0; x < n; x++) {
        for (int mask = 0; mask < (1 << m); mask++) {
            calc(calc, x, 0, mask, 0);
        }
    }
        
    return dp[n][0];
}

// T: O(n * m), M: O(m)
int maxZeroSubmatrix(const vector<vector<int>> &a) {
    if (a.empty() || a[0].empty()) {
        return 0;
    }
    int n = int(a.size()), m = int(a[0].size()), ans = 0;
    vector<int> d(m, -1), d1(m), d2(m), st;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (a[i][j] != 0) {
                d[j] = i;
            }
        }
        for (int j = 0; j < m; j++) { 
            while (!st.empty() && d[st.back()] <= d[j]) {
                st.pop_back(); 
            }
            d1[j] = st.empty() ? -1 : st.back(); 
            st.push_back(j); 
        }
        st.clear();
        for (int j = m - 1; j >= 0; j--) { 
            while (!st.empty() && d[st.back()] <= d[j]) {
                st.pop_back(); 
            }
            d2[j] = st.empty() ? m : st.back(); 
            st.push_back(j); 
        }
        st.clear();
        for (int j = 0; j < m; j++) {
            chmax(ans, (i - d[j]) * (d2[j] - d1[j] - 1));
        }
    }
    
    return ans;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    // User code here
    return 0;
}