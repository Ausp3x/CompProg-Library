// T: O(1), M: O(1)
template<typename T>
struct Point {
    T x, y;
    
    Point() : x(0), y(0) {}
    Point(T x, T y) : x(x), y(y) {}

    friend Point operator+(Point a, Point b) { a.x += b.x; a.y += b.y; return a; }
    friend Point operator-(Point a, Point b) { a.x -= b.x; a.y -= b.y; return a; }
    friend Point operator*(Point a, T b) { a.x *= b; a.y *= b; return a; }
    friend Point operator*(T b, Point a) { a.x *= b; a.y *= b; return a; }

    friend T dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
    friend T cross(Point a, Point b) { return a.x * b.y - a.y * b.x; }
};
using point = Point<lng>;
using dpoint = Point<double>; 

int sgn(lng x) { return x < 0 ? -1 : x > 0; }