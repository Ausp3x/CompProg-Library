// T: O(n * log(n)), M: O(n)
pair<int, int> getNearestPair(vector<point> &ps) {
    int n = ps.size();
    lng mn_d = INF64; 
    pair<int, int> res = {-1, -1}; 
    vector<int> ids(n), tmp(n); 
    std::iota(ids.begin(), ids.end(), 0);
    std::sort(ids.begin(), ids.end(), [&](int i, int j) { 
        return ps[i].x < ps[j].x || (ps[i].x == ps[j].x && ps[i].y < ps[j].y); 
    });

    auto mul = [&](lng a, lng b) -> lng { return (a - b) * (a - b); };
    auto update = [&](int i, int j) -> void {
        lng d = mul(ps[i].x, ps[j].x) + mul(ps[i].y, ps[j].y);
        if (d < mn_d) { mn_d = d; res = {i, j}; }
    };
    
    auto solve = [&](auto &&solve, int l, int r) -> void {
        if (r - l <= 3) {
            for (int i = l; i < r; i++) {
                for (int j = i + 1; j < r; j++) {
                    update(ids[i], ids[j]);}}
            std::sort(ids.begin() + l, ids.begin() + r, 
                      [&](int i, int j) { return ps[i].y < ps[j].y; });
            return;
        }
        
        int md = std::midpoint(l, r); lng md_x = ps[ids[md]].x;
        solve(solve, l,  md); 
        solve(solve, md, r);
        std::merge(ids.begin() + l, ids.begin() + md, ids.begin() + md, ids.begin() + r, tmp.begin(), 
                   [&](int i, int j) { return ps[i].y < ps[j].y; });
        std::copy(tmp.begin(), tmp.begin() + r - l, ids.begin() + l);
        
        int ti = 0;
        for (int i = l; i < r; i++) {
            if (mul(ps[ids[i]].x, md_x) < mn_d) {
                for (int j = ti - 1; j >= 0 && mul(ps[ids[i]].y, ps[tmp[j]].y) < mn_d; j--) {
                    update(ids[i], tmp[j]);}
                tmp[ti++] = ids[i];
            }
        }
    };
    solve(solve, 0, n);
    return res;
}