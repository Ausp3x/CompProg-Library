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
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T> 
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }

int sgn(lng x) { return x < 0 ? -1 : x > 0; }

int main() {
    cout << sgn(1) << sgn(0) << sgn(-10) << endl;
}