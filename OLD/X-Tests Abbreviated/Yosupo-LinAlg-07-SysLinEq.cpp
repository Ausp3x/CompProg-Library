#include "../1-Core/03-modint.hpp"
#include "../1-Core/07-matrix.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL); 

    int n, m;
    cin >> n >> m;
    vector a(n, vector<mint>(m));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) 
            cin >> a[i][j];
    }
    vector b(n, vector<mint>(1));
    for (int i = 0; i < n; i++)
        cin >> b[i][0];

    Matrix<mint> A(a), B(b);
    Matrix<mint> C = solveLin(A, B);
    Matrix<mint> D = ker(A);

    if (C.n == 0) {
        cout << -1 << '\n';
        return 0;
    }

    cout << D.n << '\n';
    for (int i = 0; i < C.n; i++)
        cout << C[i][0] << ' ';
    cout << '\n';
    for (int i = 0; i < D.n; i++) {
        for (int j = 0; j < D.m; j++)
            cout << D[i][j] << ' ';
        cout << '\n';
    }

    return 0;
}