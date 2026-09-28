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

    Matrix<mint> A(a);
    cout << rnk(A) << '\n';
    
    return 0;
}