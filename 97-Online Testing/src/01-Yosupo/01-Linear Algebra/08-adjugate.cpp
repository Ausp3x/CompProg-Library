// PROBLEM: https://judge.yosupo.jp/problem/adjugate_matrix
// Status: migrated source; online acceptance not verified in this migration.
#include "../../../../01-Core/05-modint.hpp"
#include "../../../../01-Core/10-matrix.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL); 

    int n;
    cin >> n;
    vector a(n, vector<mint>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) 
            cin >> a[i][j];
    }

    Matrix<mint> A(a);
    Matrix<mint> B = adj(A);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            cout << B[i][j] << ' ';
        cout << '\n';
    }

    return 0;
}