#include "../1-Core/04-dynmodint.hpp"
#include "../1-Core/07-matrix.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL); 

    int n, m;
    cin >> n >> m;
    DynModInt<0>::setMod(m);
    vector a(n, vector<DynModInt<0>>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) 
            cin >> a[i][j];
    }

    Matrix<DynModInt<0>> A(a);
    cout << det(A) << '\n';
    
    return 0;
}