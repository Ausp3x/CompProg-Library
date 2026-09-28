// T: O(n), M: O(1)
double polyArea(const vector<point> &ps) {
    double res = 0;
    for (int i = 0; i < ps.size(); i++) {
        point p1 = i > 0 ? ps[i - 1] : ps.back(), p2 = ps[i];
        res += (p1.x - p2.x) * (p1.y + p2.y);
    }
    return abs(res) / 2.0;
}