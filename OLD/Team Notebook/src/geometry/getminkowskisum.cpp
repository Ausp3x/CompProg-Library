// T: O(n), M: O(n)
vector<point> getMinkowskiSum(vector<point> ps, vector<point> qs) {
    auto reorder = [](vector<point> &P) {
        int j = 0;
        for (int i = 1; i < P.size(); i++) {
            if (P[i].y < P[j].y || (P[i].y == P[j].y && P[i].x < P[j].x)) { j = i; }}
        std::rotate(P.begin(), P.begin() + j, P.end());
    };
    reorder(ps); ps.push_back(ps[0]); ps.push_back(ps[1]);
    reorder(qs); qs.push_back(qs[0]); qs.push_back(qs[1]);
    
    int n = ps.size(), m = qs.size(), i = 0, j = 0;
    vector<point> res; 
    while (i < n - 2 || j < m - 2) {
        res.push_back(ps[i] + qs[j]);
        lng cur = cross(ps[i + 1] - ps[i], qs[j + 1] - qs[j]);
        bool adv_i = (i < n - 2 && cur >= 0) || j == m - 2;
        bool adv_j = (j < m - 2 && cur <= 0) || i == n - 2;
        i += adv_i; j += adv_j;
    }
    return res;
}