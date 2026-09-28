// S: O(n * log(n)), U: NA, Q: O(n), M: O(n)
struct Halfplane {
    dpoint p, q; double ang;

    Halfplane(const dpoint &a, const dpoint &b) : p(a), q(b - a) { ang = std::atan2(q.y, q.x); }
    
    // left: inside, right: outside
    bool isOutside(const dpoint &a) const { return (q.x * (a.y - p.y) - q.y * (a.x - p.x)) < -1e-9; }
    
    friend bool operator<(const Halfplane &a, const Halfplane &b) { return a.ang < b.ang; }
};

dpoint get2HalfplanesIntsc(const Halfplane &a, const Halfplane &b) {
    double alpha = (b.p.x - a.p.x) * b.q.y - (b.p.y - a.p.y) * b.q.x;
    alpha /= a.q.x * b.q.y - a.q.y * b.q.x;
    return a.p + a.q * alpha;
}

vector<dpoint> getManyHalfplanesIntsc(vector<Halfplane> &hs) {
    dpoint box[4] = {{2e9, 2e9}, {-2e9, 2e9}, {-2e9, -2e9}, {2e9, -2e9}};
    for (int i = 0; i < 4; i++) { hs.push_back({box[i], box[(i + 1) & 3]}); }
    sort(hs.begin(), hs.end());
    
    int len = 0;
    deque<Halfplane> dq; 
    for (int i = 0; i < hs.size(); i++) {
        while (len > 1 && hs[i].isOutside(get2HalfplanesIntsc(dq[len - 1], dq[len - 2]))) { 
            dq.pop_back(); len--;}
        while (len > 1 && hs[i].isOutside(get2HalfplanesIntsc(dq[0], dq[1]))) { 
            dq.pop_front(); len--;}
        if (len > 0 && std::abs(hs[i].ang - dq[len - 1].ang) < 1e-9) {
            if (hs[i].isOutside(dq[len - 1].p)) { dq.pop_back(); len--; } 
            else { continue; }
        }
        dq.push_back(hs[i]); len++;
    }
    while (len > 2 && dq[0].isOutside(get2HalfplanesIntsc(dq[len - 1], dq[len - 2]))) { 
        dq.pop_back(); len--;}
    while (len > 2 && dq[len - 1].isOutside(get2HalfplanesIntsc(dq[0], dq[1]))) { 
        dq.pop_front(); len--;}
    if (len < 3) { return {}; }
    
    vector<dpoint> res(len);
    for (int i = 0; i < len; i++) { res[i] = get2HalfplanesIntsc(dq[i], dq[(i + 1) % len]);}
    return res;
}