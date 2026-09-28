bool gaussJordan(vector<vector<double>> &A) {
    int n = len(A), m = len(A[0]);

    bool singular = false;
    // double determinant = 1;
    for (int i = 0, p = 0; i < n and p < m; i++, p++) {
        for (int k = i + 1; k < n; k++) {
            if (abs(A[k][p]) > EPS) {
                // determinant *= -1;
                vector<double> t = A[i];
                A[i] = A[k];
                A[k] = t;
                break;
            }
        }
        //  determinant *= A[i][p];
        if (abs(A[i][p]) < EPS) {
            singular = true;
            i--;
            continue;
        }
        for (int j = m-1; j >= p; j--)
            A[i][j] /= A[i][p];
        for (int k = 0; k < n; k++) {
            if (i == k) continue;
            for (int j = m-1; j >= p; j--)
                A[k][j] -= A[k][p] * A[i][j];
        }
    }
    return !singular;
}