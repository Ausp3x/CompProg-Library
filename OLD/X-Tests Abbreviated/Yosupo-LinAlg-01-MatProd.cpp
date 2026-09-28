#include "../1-Core/03-modint.hpp"
#include "../1-Core/07-matrix.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n, m, k;
    cin >> n >> m >> k;
    vector a(n, vector<mint>(m));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) 
            cin >> a[i][j];
    }
    vector b(m, vector<mint>(k));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < k; j++) 
            cin >> b[i][j];
    }

    Matrix<mint> A(a), B(b);
    Matrix<mint> C = A * B;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < k; j++)
            cout << C[i][j] << ' ';
        cout << '\n';
    }
    
    return 0;
}