// S: O(n * log(n)), U: NA, Q: O(1), M: O(n)
struct LinSeg {
    static inline double x = 0;
    point p, q; int id;
    
    double getY(double x) const { 
        return std::abs(p.x - q.x) == 0 ? p.y : p.y + (double(q.y) - p.y) * (x - p.x) / (q.x - p.x); 
    }
    
    friend bool operator<(const LinSeg &a, const LinSeg &b) {
        if (a.id == b.id) { return false; }
        double ay = a.getY(x), by = b.getY(x);
        if (std::abs(by - ay) > 1e-9) { return ay < by; }
        return a.id < b.id; 
    }
};

pair<int, int> getIntscgLinSegs(vector<LinSeg> &lss) {
    int n = int(lss.size()); 
    vector<tuple<double, int, int>> evs;
    for (int i = 0; i < n; i++) {
        if (lss[i].p.x > lss[i].q.x || (lss[i].p.x == lss[i].q.x && lss[i].p.y > lss[i].q.y)) {
            std::swap(lss[i].p, lss[i].q);}
        evs.push_back({lss[i].p.x, -1, i});
        evs.push_back({lss[i].q.x, 1,  i});
    }
    sort(evs.begin(), evs.end());
    
    set<LinSeg> s; 
    vector<set<LinSeg>::iterator> loc(n);
    for (const auto &[x, tp, id] : evs) {
        LinSeg::x = x;
        if (tp == -1) {
            auto nxt = s.lower_bound(lss[id]), prv = nxt == s.begin() ? s.end() : std::prev(nxt);
            if (nxt != s.end() && isIntscLinSeg(nxt->p, nxt->q, lss[id].p, lss[id].q)) {
                return {nxt->id, id};}
            if (prv != s.end() && isIntscLinSeg(prv->p, prv->q, lss[id].p, lss[id].q)) {
                return {prv->id, id};}
            loc[id] = s.insert(nxt, lss[id]);
        } else {
            auto nxt = std::next(loc[id]), prv = loc[id] == s.begin() ? s.end() : std::prev(loc[id]);
            if (nxt != s.end() && prv != s.end() && isIntscLinSeg(nxt->p, nxt->q, prv->p, prv->q)) {
                return {prv->id, nxt->id};}
            s.erase(loc[id]);
        }
    }
    return {-1, -1};
}