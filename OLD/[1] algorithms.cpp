// 人外有人，天外有天
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi     first
#define se     second
#define pb     push_back
#define all(x) (x).begin(), (x).end()
#define ral(x) (x).rbegin(), (x).rend()
#define sze(x) int((x).size())
using lng = long long;
using lll = __int128;
template<typename T> 
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

int const INF32 = 0x3f3f3f3f;
lng const INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T> 
constexpr inline bool chmax(T &a, const T &b) { 
    return a < b ? a = b, 1 : 0; 
}

template<typename T> 
constexpr inline bool chmin(T &a, const T &b) { 
    return a > b ? a = b, 1 : 0; 
}

namespace Debug {
    using std::to_string;

    string to_string(bool x) { 
        return x ? "true" : "false"; 
    }

    string to_string(char x) { 
        return string({'\'', x, '\''}); 
    }

    string to_string(std::string_view x) {
        string res;
        res.reserve(x.size() + 2);
        res += '"';
        res += x;
        res += '"';
    
        return res;
    }

    string to_string(const char *x) { 
        return to_string(std::string_view(x)); 
    }

    template<size_t N> 
    string to_string(const bitset<N> &x) {
        return x.to_string(); 
    }

    template<typename T> 
    requires requires { std::tuple_size<T>::value; } 
          && (!std::ranges::range<T>)
    string to_string(const T &x);

    template<std::ranges::range T> 
    requires (!std::is_convertible_v<T, std::string_view>)
    string to_string(const T &x);

    template<typename T>
    requires (!std::ranges::range<T>) 
          && requires (std::ostream &os, const T &x) { os << x; }
    string to_string(const T &x) {
        std::stringstream ss;
        ss << x;
        
        return ss.str();
    }

    struct Any { 
        template<typename T> operator T() const; 
    };
    
    // optional
    template<typename T>
    requires std::is_aggregate_v<T> 
          && (!std::ranges::range<T>) 
          && (!requires (std::ostream &os, const T &x) { os << x; })
    string to_string(const T &x) {
        if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e, f, g, h] = x; 
            return to_string(std::tie(a, b, c, d, e, f, g, h));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e, f, g] = x; 
            return to_string(std::tie(a, b, c, d, e, f, g));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e, f] = x; 
            return to_string(std::tie(a, b, c, d, e, f));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e] = x; 
            return to_string(std::tie(a, b, c, d, e));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d] = x; 
            return to_string(std::tie(a, b, c, d));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c] = x; 
            return to_string(std::tie(a, b, c));
        } else if constexpr (requires { T{Any{}, Any{}}; }) {
            auto &[a, b] = x; 
            return to_string(std::tie(a, b));
        } else if constexpr (requires { T{Any{}}; }) {
            auto &[a] = x;
            return to_string(std::tie(a));
        } else {
            return "{}";
        }
    }

    template<typename T>
    requires requires { std::tuple_size<T>::value; } 
          && (!std::ranges::range<T>)
    string to_string(const T &x) {
        string res = "(";
        std::apply(
            [&](const auto &...args) {
                int i = 0;
                ((res += (i++ ? ", " : ""), res += to_string(args)), ...);
            }, x
        );

        return res + ")";
    }

    template<typename T, typename C>
    string to_string(const queue<T, C> &x) {
        struct Accessor : queue<T, C> { 
            using queue<T, C>::c; 
        };
        
        return to_string(static_cast<const Accessor&>(x).c);
    }

    template<typename T, typename C>
    string to_string(const stack<T, C> &x) {
        struct Accessor : stack<T, C> { 
            using stack<T, C>::c; 
        };
        
        return to_string(static_cast<const Accessor&>(x).c);
    }

    template<typename T, typename C, typename Comp>
    string to_string(const priority_queue<T, C, Comp> &x) {
        struct Accessor : priority_queue<T, C, Comp> { 
            using priority_queue<T, C, Comp>::c; 
        };
        
        return to_string(static_cast<const Accessor&>(x).c);
    }

    template<std::ranges::range T> 
    requires (!std::is_convertible_v<T, std::string_view>)
    string to_string(const T &x) {
        string res = "{";
        auto it = std::begin(x);
        if (it != std::end(x)) {
            res += to_string(*it);
            for (++it; it != std::end(x); ++it) {
                res += ", ";
                res += to_string(*it);
            }
        }
        res += "}";
        
        return res;
    }

    template<typename ...Args>
    void debugO(const Args &...args) {
        ((cerr << ' ' << to_string(args)), ...);
        cerr << '\n';
    }

    template<std::ranges::range R, typename ...Args>
    auto slice(R &&ran, int l, int r, Args ...args) {
        auto v = std::forward<R>(ran) | std::views::drop(l) | std::views::take(r - l + 1);
        if constexpr (sizeof...(args) == 0) {
            return v;
        } else {
            return v | std::views::transform([=](auto &&cur) { return slice(cur, args...); });
        }
    }

    int dep = 0;

    std::string_view indent() {
        static constexpr auto spaces = [] {
            array<char, 128> v{};
            v.fill(' ');
            return v;
        }();
    
        return std::string_view(spaces.data(), min<int>(2 * dep, spaces.size()));
    }
    
    struct Tracer {
        string v; 

        Tracer(string x) : v(std::move(x)) { 
            cerr << indent() << ">> " << v << '\n'; 
            dep++; 
        }

        ~Tracer() { 
            dep--; 
            cerr << indent() << "<< " << v << '\n'; 
        }
    };
}

#ifdef LOCAL
#define debug(...) cerr << Debug::indent() << "[L" << __LINE__ << "] [" << #__VA_ARGS__ << "]:", Debug::debugO(__VA_ARGS__)
// #define debug(...) cerr << Debug::indent() << "\033[1;31m[L" << __LINE__ << "] [" << #__VA_ARGS__ << "]:\033[0m", Debug::debugO(__VA_ARGS__)
#define trace(x) Debug::Tracer _trace_guard(x)
#else
#define debug(...) void(0)
#define trace(x) void(0)
#endif

// TODO: Convex Hull
// TODO: Trie
// TODO: 2D Segment Tree??
// TODO: Sieve of Eratosthenes and similar Prime finders
// TODO: Ford Fulkerson + its optimization
// TODO: Kosaraju
// TODO: center of tree
// TODO: centroid of tree
// TODO: AHU algorithm
// TODO: Heavy Light Decomposition
// TODO: Li Chao Tree

// TESTED
template<typename T> 
lng kadane(const vector<T> &arr) {
    if (arr.empty()) {
        return 0;
    }

    lng max_sum = -INF64, cur_sum = -INF64;
    for (T a : arr) {
        cur_sum = max(cur_sum + a, lng(a));
        max_sum = max(max_sum, cur_sum);
    }

    return max_sum;
}



// TESTED
struct BridgeAlgo {
    int n, timer = 0;
    vector<bool> vst;
    vector<int> t_in, low;
    set<pair<int, int>> bridges;

    BridgeAlgo(int n, const vector<vector<int>> &adjl): n(n) {
        vst.resize(n);
        t_in.resize(n, -1);
        low.resize(n, -1);

        for (int i = 0; i < n; i++) {
            if (!vst[i]) {
                dfs(i, i, adjl);
            }
        }        
    }

    void dfs(int cur, int prv, const vector<vector<int>> &adjl) {
        vst[cur] = true;
        t_in[cur] = timer;
        low[cur] = timer;
        timer++;
        
        bool prv_skipped = false;
        for (int nxt : adjl[cur]) {
            if (nxt == prv && !prv_skipped) {
                prv_skipped = true;
                continue;
            }

            if (vst[nxt]) {
                low[cur] = min(low[cur], t_in[nxt]);
            } else {
                dfs(nxt, cur, adjl);

                low[cur] = min(low[cur], low[nxt]);
                if (low[nxt] > t_in[cur]) {
                    bridges.insert({min(cur, nxt), max(cur, nxt)});
                }
            }
        }
    }
};

struct Dijkstra {
    int n;
    vector<bool> is_proc;
    vector<lng> dis;
    priority_queue<pair<lng, int>, vector<pair<lng, int>>, greater<pair<lng, int>>> unproc;

    Dijkstra(int n): n(n) {
        is_proc.resize(n + 1);
        dis.resize(n + 1, INF64);
    }

    void runDijkstra(const vector<int> &src, const vector<vector<pair<int, lng>>> &adjl) {
        fill(is_proc.begin(), is_proc.end(), false);
        fill(dis.begin(), dis.end(), INF64);
        priority_queue<pair<lng, int>, vector<pair<lng, int>>, greater<pair<lng, int>>> empty;
        unproc.swap(empty);
        
        for (int u : src) {
            dis[u] = 0;
            unproc.push({dis[u], u});
        }
        while (!unproc.empty()) {
            int cur = unproc.top().se;
            unproc.pop();

            if (is_proc[cur]) {
                continue;
            }
            is_proc[cur] = true;

            for (auto [nxt, w] : adjl[cur]) {
                if (dis[cur] + w < dis[nxt]) {
                    dis[nxt] = dis[cur] + w;
                    unproc.push({dis[nxt], nxt});
                }
            }
        };
    }
};



// TESTED
struct EdmondsKarp {
    int n;
    vector<vector<bool>> adjm;
    vector<vector<int>> adjl;
    vector<vector<lng>> ocap, ncap;

    lng max_flow = 0, add_flow = 0;
    vector<int> par;
    vector<bool> in_S;
    vector<pair<int, int>> cut_set;
    
    EdmondsKarp(int n, const vector<vector<int>> &adj, const vector<vector<lng>> &cap): n(n) {
        assert(adj.size() == n + 1 && cap.size() == n + 1);
        for (int i = 0; i <= n; i++) {
            assert(cap[i].size() == n + 1);
        }

        adjm.resize(n + 1, vector<bool>(n + 1));
        ocap.resize(n + 1, vector<lng>(n + 1));
        vector<set<int>> distinct_adjl(n + 1);
        for (int u = 0; u <= n; u++) {
            for (int v : adj[u]) {
                adjm[u][v] = true;
                ocap[u][v] = cap[u][v];
                distinct_adjl[u].insert(v);
                distinct_adjl[v].insert(u);
            }
        }
        
        adjl.resize(n + 1);
        for (int u = 0; u <= n; u++) {
            for (int v : distinct_adjl[u]) {
                adjl[u].pb(v);
            }
        }

        par.resize(n + 1, -1);
        in_S.resize(n + 1);
    }

    lng augmentFlow(int s, int t) {
        fill(par.begin(), par.end(), -1);
        par[s] = s;

        queue<pair<int, lng>> q;
        q.push({s, INF64});
        while (!q.empty()) {
            auto [cur, cur_flow] = q.front();
            q.pop();

            for (int nxt : adjl[cur]) {
                if (par[nxt] == -1 && ncap[cur][nxt] > 0) {
                    par[nxt] = cur;
                    
                    lng nxt_flow = min(cur_flow, ncap[cur][nxt]);
                    if (nxt == t) {
                        return nxt_flow;
                    }
                    
                    q.push({nxt, nxt_flow});
                }
            }
        }

        return 0;
    }

    lng getMaxFlow(int s, int t) {
        assert(0 <= s && s <= n && 0 <= t && t <= n);
        max_flow = 0;
        add_flow = 0;
        ncap = ocap;

        while ((add_flow = augmentFlow(s, t)) > 0) {
            max_flow += add_flow;

            int cur = t;
            while (cur != s) {
                int prv = par[cur];
                ncap[prv][cur] -= add_flow;
                ncap[cur][prv] += add_flow;
                cur = prv;
            }
        }

        return max_flow;
    }

    void getMinCut(int s, int t) {
        getMaxFlow(s, t);
        fill(in_S.begin(), in_S.end(), false);
        in_S[s] = true;
        cut_set.clear();
        
        queue<int> q;
        q.push(s);
        while (!q.empty()) {
            int cur = q.front();
            q.pop();

            for (int nxt : adjl[cur]) {
                if (ncap[cur][nxt] > 0 && !in_S[nxt]) {
                    q.push(nxt);
                    in_S[nxt] = true;
                }
            }
        }

        for (int u = 0; u <= n; u++) {
            if (!in_S[u]) {
                continue;
            }

            for (int v : adjl[u]) {
                if (adjm[u][v] && !in_S[v]) {
                    cut_set.push_back({u, v});
                }
            }
        }
    }
};



// TESTED
/*
struct EulerTourTree {
    int n, root;
    vector<lng> arr;
    vector<vector<int>> adjl;

    int timer = 0;
    vector<int> t_in, t_out, time_to_orig;
    vector<lng> euler;
    unique_ptr<SegTree> segt;

    EulerTourTree(int n, int root, const vector<lng> &arr, const vector<vector<int>> &adjl) : n(n), root(root), arr(arr), adjl(adjl) {
        assert(0 <= root && root < n);
        assert(arr.size() == n && adjl.size() == n);
        t_in.resize(n, -1);
        t_out.resize(n, -1);
        time_to_orig.resize(n);
        euler.resize(n);
        
        dfs(root, -1);

        segt = make_unique<SegTree>(0, n - 1, euler);
    }

    void dfs(int cur, int prv) {
        t_in[cur] = timer;
        time_to_orig[timer] = cur;
        euler[timer] = arr[cur];
        timer++;

        for (int nxt : adjl[cur]) {
            if (nxt == prv) {
                continue;
            }

            dfs(nxt, cur);
        }

        t_out[cur] = timer - 1;
    }

    void subtreeAddUpdate(int u, lng x, bool u_only = false) {
        segt->rangeAddUpdate(t_in[u], (u_only ? t_in[u] : t_out[u]), x);
    }

    void subtreeSetUpdate(int u, lng x, bool u_only = false) {
        segt->rangeSetUpdate(t_in[u], (u_only ? t_in[u] : t_out[u]), x);
    }

    pair<lng, int> subtreeMaxQuery(int u, bool u_only = false) {
        auto res = segt->rangeMaxQuery(t_in[u], (u_only ? t_in[u] : t_out[u]));
        res.se = time_to_orig[res.se];

        return res;
    }
    
    pair<lng, int> subtreeMinQuery(int u, bool u_only = false) {
        auto res = segt->rangeMinQuery(t_in[u], (u_only ? t_in[u] : t_out[u]));
        res.se = time_to_orig[res.se];

        return res;
    }

    lng subtreeSumQuery(int u, bool u_only = false) {
        return segt->rangeSumQuery(t_in[u], (u_only ? t_in[u] : t_out[u]));
    }
};
*/


// TESTED 
struct FloydWarshall {
    int n;
    vector<vector<lng>> dis;
    
    FloydWarshall(int n, vector<vector<lng>> dis) : n(n), dis(dis) {
        assert(dis.size() == n + 1);
        for (int i = 0; i <= n; i++) {
            assert(dis[i].size() == n + 1);
        }
    }
    
    void runFloydWarshall() {
        for (int k = 1; k <= n; k++) {
            for (int i = 1; i <= n; i++) {
                for (int j = 1; j <= n; j++) {
                    if (dis[i][k] < INF64 && dis[k][j] < INF64) {
                        dis[i][j] = max(min(dis[i][j], dis[i][k] + dis[k][j]), -INF64);
                    }
                }
            }
        }
    
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= n; j++) {
                for (int k = 1; k <= n; k++) {
                    if (dis[i][k] < INF64 && dis[k][k] < 0 && dis[k][j] < INF64) {
                        dis[i][j] = -INF64;
                        break;
                    }
                }
            }
        }
    }
};



// TESTED
struct LCA {
    int n, root, l, timer = 0;
    vector<int> dep, t_in, t_out;
    vector<vector<int>> up;

    LCA(int n, int root, const vector<vector<int>> &adjl): n(n), root(root) {
        l = ceil(log2(n));
        dep.resize(n + 1);
        t_in.resize(n + 1, -1);
        t_out.resize(n + 1, -1);
        up.resize(n + 1, vector<int>(l + 1, -1));

        init(root, root, 0, adjl);
    }

    void init(int cur, int prv, int d, const vector<vector<int>> &adjl) {
        dep[cur] = d;
        t_in[cur] = timer;
        timer++;
        up[cur][0] = prv;
        for (int i = 1; i <= l; i++) {
            up[cur][i] = up[up[cur][i - 1]][i - 1];
        }

        for (int nxt : adjl[cur]) {
            if (nxt == prv) {
                continue;
            }

            init(nxt, cur, d + 1, adjl);
        }

        t_out[cur] = timer - 1;
    }

    bool isAncestor(int u, int v) const {
        return t_in[u] <= t_in[v] && t_out[v] <= t_out[u];
    }

    int getKthAncestor(int u, int k) const {
        if (k <= 0) {
            return u;
        }
        
        if (k > dep[u]) {
            return -1;
        }
        
        for (int i = l; i >= 0; i--) {
            if (k & (1 << i)) {
                u = up[u][i];
            }
        }

        return u;
    }

    int getLCA(int u, int v) const {
        if (isAncestor(u, v)) {
            return u;
        }
        
        if (isAncestor(v, u)) {
            return v;
        }

        for (int i = l; i >= 0; i--) {
            if (!isAncestor(up[u][i], v)) {
                u = up[u][i];
            }
        }

        return up[u][0];
    }
};



// TODO
struct LiChaoTree {
    using line = complex<lng>;

    vector<line> hull, vecs;

    void addLine(lng m, lng b) {
        line l = {m, b};
        while(!vecs.empty() && dotProd(vecs.back(), l - hull.back()) < 0) {
            hull.pop_back();
            vecs.pop_back();
        }

        if(!hull.empty()) {
            vecs.push_back(line(0, 1) * (l - hull.back()));
        }
        hull.push_back(l);
    }




    lng dotProd(line a, line b) {
        return (conj(a) * b).real();
    }

    lng f(line a, lng x) {
        return dotProd(a, {x, 1});
    }

};



// TESTED
struct MergeSortTree {
    int l, r;
    vector<lng> sorted;
    vector<int> l_idxs, r_idxs;
    unique_ptr<MergeSortTree> l_child, r_child;

    template<typename T> 
    MergeSortTree(int l, int r, const vector<T> &arr): l(l), r(r) {
        if (l == r) {
            sorted.pb(arr[l]);
        } else {
            sorted.resize(getRange());
            l_idxs.resize(getRange());
            r_idxs.resize(getRange());

            int m = (l + r) / 2;
            l_child = make_unique<MergeSortTree>(l, m, arr);
            r_child = make_unique<MergeSortTree>(m + 1, r, arr);
            pull();
        }
    }

    // pull states up from children
    void pull() {
        if (l_child && r_child) {
            const vector<lng> &l_sorted = l_child->sorted;
            const vector<lng> &r_sorted = r_child->sorted;
            int l_len = l_sorted.size(), r_len = r_sorted.size();

            int i = 0, j = 0, k = 0;
            while (i < l_len || j < r_len) {
                if ((i < l_len && l_sorted[i] <= r_sorted[j]) || j == r_len) {
                    sorted[k] = l_sorted[i];
                    l_idxs[k] = i;
                    r_idxs[k] = j;
                    i++;
                } else if ((j < r_len && r_sorted[j] <= l_sorted[i]) || i == l_len) {
                    sorted[k] = r_sorted[j];     
                    l_idxs[k] = i;
                    r_idxs[k] = j;
                    j++;               
                }

                k++;
            }
        }
    }

    int rangeCountLessThanQuery(int l_cur, int r_cur, lng x) const {
        if (l_cur > r_cur || (r < l_cur || r_cur < l)) {
            return 0;
        }

        if (l_cur <= l && r <= r_cur) {
            return countLessThan(x);
        }
        
        int cnt = countLessThan(x);
        int l_cnt = l_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? l_idxs[cnt] : l_child->getRange()));
        int r_cnt = r_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? r_idxs[cnt] : r_child->getRange()));
    
        return l_cnt + r_cnt;
    }

    // overloaded for internal use
    int rangeCountLessThanQuery(int l_cur, int r_cur, lng x, int cnt) const {
        if (l_cur > r_cur || (r < l_cur || r_cur < l)) {
            return 0;
        }

        if (l_cur <= l && r <= r_cur) {
            return cnt;
        }

        int l_cnt = l_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? l_idxs[cnt] : l_child->getRange()));
        int r_cnt = r_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? r_idxs[cnt] : r_child->getRange()));

        return l_cnt + r_cnt;
    }

    lng rangeMedianQuery(int l_cur, int r_cur, lng lo = 0, lng hi = 1'000'000'000'000'000'001) const {
        int req = (r_cur - l_cur + 1) / 2;
        while (hi - lo > 1) {
            lng md = (lo + hi) / 2;

            int cnt = rangeCountLessThanQuery(l_cur, r_cur, md);
            if (cnt <= req) {
                lo = md;
            } else {
                hi = md;
            }
        }
        
        return lo;
    }

    int countLessThan(lng x) const {
        return lower_bound(sorted.begin(), sorted.end(), x) - sorted.begin();
    }

    lng getRange() const {
        return r - l + 1;
    }
};



// TESTED
/*
struct ModFac {
    int n;
    vector<mint> fac, inv_fac;

    ModFac(int n): n(n) {
        fac.resize(n + 1);
        fac[0] = 1;
        for (int i = 1; i <= n; i++) {
            fac[i] = fac[i - 1] * i;
        }

        inv_fac.resize(n + 1);
        inv_fac[n] = fac[n].inv();
        for (int i = n - 1; i >= 0; i--) {
            inv_fac[i] = inv_fac[i + 1] * (i + 1);
        }
    }

    mint combiNR(int x, int y) const {
        if (y > x || y < 0) {
            return mint(0);
        }

        assert(x <= n);

        return fac[x] * inv_fac[y] * inv_fac[x - y];
    }

    mint combiWR(int x, int y) const {
        if (x < 1 || y < 0) {
            return mint(0);
        }

        assert(x + y - 1 <= n);

        return fac[x + y - 1] * inv_fac[y] * inv_fac[x - 1];
    }

    mint permuNR(int x, int y) const {
        if (y > x || y < 0) {
            return mint(0);
        }

        assert(x <= n);   

        return fac[x] * inv_fac[x - y];
    }

    mint permuWR(int x, int y) const {
        if (y < 0) {
            return mint(0);
        }
        
        return mint(x).pow(y);
    }
};
*/