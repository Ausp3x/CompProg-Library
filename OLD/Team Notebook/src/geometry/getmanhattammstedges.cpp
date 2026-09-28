// T: O(n * log(n)), M: O(n)
vector<tuple<lng, int, int>> getManhattanMSTEdges(vector<point> ps) {
    int n = ps.size();
    const vector<point> ps0 = ps;
    vector<int> ids(n); std::iota(ids.begin(), ids.end(), 0);
    vector<tuple<lng, int, int>> es;
    
    for (int rot = 0; rot < 4; rot++) {
        std::sort(ids.begin(), ids.end(), [&](int i, int j) { 
            return (ps[i].x + ps[i].y) < (ps[j].x + ps[j].y); 
        });
        
        map<int, int, std::greater<int>> act;
        for (int i : ids) {
            for (auto it = act.lower_bound(ps[i].x); it != act.end(); act.erase(it++)) {
                int j = it->second;
                if (ps[i].x - ps[i].y > ps[j].x - ps[j].y) { break; }
                lng d = std::abs(ps0[i].x - ps0[j].x) + std::abs(ps0[i].y - ps0[j].y);
                es.push_back({d, i, j});
            }
            act[ps[i].x] = i;
        }
        for (auto &p : ps) { 
            if (rot & 1) { p.x = -p.x; } 
            else { std::swap(p.x, p.y); }
        }
    }
    
    return es;
}