#include "../1-Core/03-modint.hpp"
#include "../1-Core/07-matrix.hpp"

void solve() {
    int n;
    cin >> n;
    Matrix<ModInt<2>> A(n, 30);
    for (int i = 0; i < n; i++) {
        int u;
        cin >> u;
        for (int j = 0; j < 30; j++) {
            A[i][j] = (u >> j) & 1;
        }
    }
    int m;
    cin >> m;
    Matrix<ModInt<2>> B(m, 30);
    for (int i = 0; i < m; i++) {
        int v;
        cin >> v;
        for (int j = 0; j < 30; j++) {
            B[i][j] = (v >> j) & 1;
        }
    }

    if (A.m == 0 || B.m == 0) {
        cout << 0 << endl;
        return;
    }

    Matrix<ModInt<2>> C = intscSpace(A, B);

    vector<int> res;
    for (int i = 0; i < C.n; i++) {
        int u = 0;
        for (int j = 0; j < 30; j++) {
            if (C[i][j] == 1) {
                u |= (1 << j);
            }
        }
        res.push_back(u);
    }

    cout << res.size();
    for (int x : res) {
        cout << ' ' << x;
    }
    cout << endl;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL); 

    int t;
    cin >> t;
    while (t--) {
        solve();
    }

    return 0;
}