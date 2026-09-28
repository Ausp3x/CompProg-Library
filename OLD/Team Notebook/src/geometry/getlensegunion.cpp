// T: O(n * log(n)), M: O(n)
lng getLenSegUnion(vector<pair<int, int>> &segs) {
    vector<pair<int, int>> evs;
    for (auto &[a, b] : segs) { 
        if (a > b) { std::swap(a, b); }
        evs.push_back({a, 1});
        evs.push_back({b, -1});
    }
    std::sort(evs.begin(), evs.end());
    
    lng res = 0, cur = evs.empty() ? 0 : evs[0].second;
    for (int i = 1; i < evs.size(); i++) {
        if (evs[i].first > evs[i - 1].first && cur > 0) {
            res += lng(evs[i].first) - evs[i - 1].first;}
        cur += evs[i].second;
    }
    return res;
}