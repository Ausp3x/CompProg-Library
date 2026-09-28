// T: O(1), M: O(1)
bool isIntsc1D(lng a, lng b, lng c, lng d) {
    if (a > b) { std::swap(a, b); }
    if (c > d) { std::swap(c, d); }
    return max(a, c) <= min(b, d);
}

// T: O(1), M: O(1)
bool isIntscLinSeg(point a, point b, point c, point d) {
    if (cross(a - c, d - c) == 0 && cross(b - c, d - c) == 0) {
        return isIntsc1D(a.x, b.x, c.x, d.x) && isIntsc1D(a.y, b.y, c.y, d.y);}
    return sgn(cross(b - a, c - a)) != sgn(cross(b - a, d - a)) 
        && sgn(cross(d - c, a - c)) != sgn(cross(d - c, b - c));
}

// T: O(1), M: O(1)
pair<bool, dpoint> getIntscLineLine(double a1, double b1, double c1, double a2, double b2, double c2) {
    double det = a1 * b2 - a2 * b1;
    if (abs(det) < 1e-9) { return {false, {}}; }
    return {true, {-(c1 * b2 - c2 * b1) / det, -(a1 * c2 - a2 * c1) / det}};
}

// T: O(1), M: O(1)
vector<dpoint> getIntscLineCircle(double r, double a, double b, double c) { // ax + by + c = 0
    double d_sq = c * c / (a * a + b * b), x0 = -a * c / (a * a + b * b), y0 = -b * c / (a * a + b * b);
    if (d_sq > r * r + 1e-9) { return {}; }
    if (std::abs(d_sq - r * r) < 1e-9) { return {dpoint(x0, y0)}; }
    double d = std::sqrt((r * r - d_sq) / (a * a + b * b));
    return {dpoint(x0 + b * d, y0 - a * d), dpoint(x0 - b * d, y0 + a * d)};
}

// T: O(1), M: O(1)
vector<dpoint> isIntscCircleCircle(dpoint p1, double r1, dpoint p2, double r2) {
    p2 = p2 - p1;
    double a = -2 * p2.x, b = -2 * p2.y, c = p2.x * p2.x + p2.y * p2.y + r1 * r1 - r2 * r2;
    vector<dpoint> res = getIntscLineCircle(r1, a, b, c);
    for (auto &p : res) { p = p + p1; }
    return res;
}