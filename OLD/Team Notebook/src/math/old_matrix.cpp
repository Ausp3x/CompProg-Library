struct Matrix {
    vector<vector<ll>> a;
    int n;
    Matrix(int n) : n(n) {
	a.resize(n, vector<ll>(n));
        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                a[i][j] = 0;
            }
        }
    }

    Matrix operator *(Matrix other) {
        Matrix res = Matrix(n);
        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                for(int k = 0; k < n; k++) {
                    res.a[i][k] += a[i][j] * other.a[j][k];
                    res.a[i][k] %= MOD;
                }
            }
        }
        return res;
    }
};

Matrix power(Matrix a, ll e) {
	Matrix res = Matrix(a.n);
	for (int i = 0; i < a.n; i++)
		res.a[i][i] = 1;
	while (e) {
		if (e % 2) {
			res = res * a;
		}
		e /= 2;
		a = a * a;
	}
	return res;
}