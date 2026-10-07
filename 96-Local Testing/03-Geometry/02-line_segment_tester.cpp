#include "../../03-Geometry/02-line_segment.hpp"

namespace {
    using Kind = IntersectionKind;
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0;
    string show(lll n) {
        if (!n) { return "0"; }
        bool neg = n < 0; ulll u = neg ? ulll(0) - ulll(n) : ulll(n); string s;
        while (u) { s += char('0' + u % 10); u /= 10; }
        if (neg) { s += '-'; } reverse(s.begin(),s.end()); return s;}
    string show(point p) { return "(" + show(p.x) + "," + show(p.y) + ")"; }
    string show(Linear2 s) { return std::to_string(int(s.kind)) + ":" + show(s.a) + "->" + show(s.b); }
    string show(RationalPoint2 p) { return "(" + show(p.x) + "," + show(p.y) + ")/" + show(p.d); }
    void require(bool ok, const string &operation, const string &want = "true", const string &got = "false") {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=" + want + " actual=" + got); }}
    void close(long double got, long double want, const string &operation, long double scale = 1) {
        // 2e-15 absolute/relative allowance includes projection cancellation at
        // the original coordinate scale; fixtures use 80-decimal-digit roots.
        long double error = 2e-15L * max({1.L, std::abs(want), scale});
        std::ostringstream a, b; a << std::setprecision(22) << got; b << std::setprecision(22) << want;
        require(std::isfinite(got) && std::abs(got - want) <= error, operation, b.str(), a.str());}
    lll readWide(istream &in) {
        string s; in >> s; bool neg = !s.empty() && s[0] == '-'; lll x = 0;
        for (int i = neg; i < int(s.size()); ++i) { x = 10 * x + s[i] - '0'; }
        return neg ? -x : x;}
    struct Reference { Kind kind; RationalPoint2 p; point a,b; };
    void compare(LinearIntersection2 got, const Reference &want) {
        require(got.kind == want.kind, "intersection kind", std::to_string(int(want.kind)), std::to_string(int(got.kind)));
        if (want.kind == Kind::Empty) { return; }
        if (want.kind == Kind::Point) {
            require(got.p == want.p, "exact rational crossing", show(want.p), show(got.p));
            require(got.p.d > 0 && std::gcd(std::gcd(got.p.x < 0 ? -got.p.x : got.p.x, got.p.y < 0 ? -got.p.y : got.p.y), got.p.d) == 1,
                    "canonical rational normalization");
            close(got.p.approx().x, static_cast<long double>(want.p.x) / want.p.d, "rational approximate x");
            close(got.p.approx().y, static_cast<long double>(want.p.y) / want.p.d, "rational approximate y"); return;}
        LinearKind kind = want.kind == Kind::Line ? LinearKind::Line : want.kind == Kind::Ray ? LinearKind::Ray : LinearKind::Segment;
        require(got.overlap.kind == kind, "overlap kind");
        if (want.kind != Kind::Line) { require(got.overlap.a == want.a, "overlap origin",show(want.a),show(got.overlap.a)); }
        if (want.kind == Kind::Segment) { require(got.overlap.b == want.b, "overlap endpoint",show(want.b),show(got.overlap.b)); }
        else {
            point u = want.b - want.a, v = got.overlap.b - got.overlap.a;
            require(cross(u,v) == 0 && v != point{}, "overlap direction parallel");
            if (want.kind == Kind::Ray) { require(dot(u,v) > 0, "ray forward direction"); }
            else { require(!orient(want.a,want.b,got.overlap.a), "coincident output line"); }}
        require(intersect(got.overlap,got.overlap).kind == got.kind, "overlap remains a valid input");}
    void intersectionFixture(istream &in) {
        Linear2 s,t; int sk,tk,k,bits; Reference want{};
        in >> s.a.x >> s.a.y >> s.b.x >> s.b.y >> sk >> t.a.x >> t.a.y >> t.b.x >> t.b.y >> tk >> k;
        auto make = [](point a, point b, int kind) { return kind == 0 ? Linear2::segment(a,b) : kind == 1 ? Linear2::ray(a,b) : Linear2::line(a,b); };
        s = make(s.a,s.b,sk); t = make(t.a,t.b,tk); want.kind = Kind(k);
        want.p.x = readWide(in); want.p.y = readWide(in); want.p.d = readWide(in);
        in >> want.a.x >> want.a.y >> want.b.x >> want.b.y >> bits;
        context = "s=" + show(s) + " t=" + show(t);
        compare(intersect(s,t),want); compare(intersect(t,s),want);
        require(parallel(s,t) == bool(bits & 1), "parallel including zero direction"); bits >>= 1;
        require(collinear(s,t) == bool(bits & 1) && collinear(t,s) == bool(bits & 1), "collinearity symmetric degeneracies"); bits >>= 1;
        for (point p : {s.a,s.b,t.a,t.b,point{}}) {
            require(contains(s,p) == bool(bits & 1), "contains " + show(p)); bits >>= 1;
            require(onSegment(p,s.a,s.b) == bool(bits & 1), "onSegment " + show(p)); bits >>= 1;}
        require(orthogonal(s,t) == bool(bits & 1) && orthogonal(t,s) == bool(bits & 1), "orthogonal including zero direction");
        if (s.a != s.b && t.a != t.b) {
            auto k = canonicalLine(s), l = canonicalLine(t);
            require((k == l) == collinear(s,t), "canonical line keys equal iff same supporting line");
            require(std::gcd(k[0],k[1]) == 1 && (k[1] > 0 || (k[1] == 0 && k[0] > 0)), "canonical line normalization");
            for (point p : {s.a,s.b}) { require(lll(k[0]) * p.x + lll(k[1]) * p.y == k[2], "canonical line passes through endpoints"); }}}
    void metricFixture(istream &in) {
        point p,a,b,c; long double pd,sd,px,py,rx,ry,ld;
        in >> p.x >> p.y >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y >> pd >> sd >> px >> py >> rx >> ry >> ld;
        context = "p=" + show(p) + " a=" + show(a) + " b=" + show(b) + " c=" + show(c);
        close(pointSegmentDistanceApprox(p,a,b),pd,"point segment distance");
        close(pointSegmentDistanceApprox(p.cast<double>(),a.cast<double>(),b.cast<double>()),pd,"double point segment distance");
        close(segmentDistanceApprox(p,a,b,c),sd,"segment segment distance");
        close(segmentDistanceApprox(c,b,a,p),sd,"segment distance reversal and symmetry");
        if (a == b) { return; }
        dpoint q = projectLineApprox(p,a,b), r = reflectLineApprox(p,a,b);
        long double scale = max({1.L,std::abs(static_cast<long double>(p.x)),std::abs(static_cast<long double>(p.y)),
                                std::abs(static_cast<long double>(a.x)),std::abs(static_cast<long double>(a.y))});
        close(q.x,px,"projection x",scale); close(q.y,py,"projection y",scale);
        close(r.x,rx,"reflection x",scale); close(r.y,ry,"reflection y",scale);
        close(signedLineDistanceApprox(p,a,b),ld,"signed left-positive line distance");
        close(signedLineDistanceApprox(p,b,a),-ld,"line orientation reversal");
        dpoint pp = p.cast<long double>(), aa = a.cast<long double>(), bb = b.cast<long double>();
        dpoint qq = projectLineApprox(pp,aa,bb), rr = reflectLineApprox(pp,aa,bb);
        close(qq.x,px,"floating projection x",scale); close(qq.y,py,"floating projection y",scale);
        close(rr.x,rx,"floating reflection x",scale); close(rr.y,ry,"floating reflection y",scale);
        close(signedLineDistanceApprox(pp,aa,bb),ld,"floating signed line distance");}
    void gridFixture(istream &in) {
        point a,b; lng lattice,cells;
        in >> a.x >> a.y >> b.x >> b.y >> lattice >> cells;
        context = "a=" + show(a) + " b=" + show(b);
        require(latticeOnSegment(a,b) == lattice && latticeOnSegment(b,a) == lattice, "lattice points on segment", std::to_string(lattice), std::to_string(latticeOnSegment(a,b)));
        require(gridCellsCrossed(a,b) == cells && gridCellsCrossed(b,a) == cells, "grid cells crossed", std::to_string(cells), std::to_string(gridCellsCrossed(a,b)));}
    void constructionFixture(istream &in) {
        point p,a,b,c; long double ld,mx,my,nx,ny,wx,wy,rx,ry;
        in >> p.x >> p.y >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y >> ld >> mx >> my >> nx >> ny >> wx >> wy >> rx >> ry;
        context = "p=" + show(p) + " a=" + show(a) + " b=" + show(b) + " c=" + show(c);
        long double scale = 0;
        for (point q : {p,a,b,c}) { scale = max({scale,std::abs(static_cast<long double>(q.x)),std::abs(static_cast<long double>(q.y))}); }
        close(lineDistanceApprox(a,b,p,c),ld,"distance between lines",scale);
        close(lineDistanceApprox(b,a,c,p),ld,"line distance endpoint reversal",scale);
        auto m = perpendicularBisectorApprox(a,b);
        close(m[0].x,mx,"perpendicular bisector midpoint x",scale); close(m[0].y,my,"perpendicular bisector midpoint y",scale);
        close(m[1].x,nx,"perpendicular bisector through x",scale); close(m[1].y,ny,"perpendicular bisector through y",scale);
        require(signedLineDistanceApprox(a.cast<long double>(),m[0],m[1]) > 0 && signedLineDistanceApprox(b.cast<long double>(),m[0],m[1]) < 0, "bisector keeps a on the left");
        auto w = angleBisectorApprox(a,p,b);
        close(w[0].x,static_cast<long double>(p.x),"angle bisector vertex x"); close(w[0].y,static_cast<long double>(p.y),"angle bisector vertex y");
        // Two returned points carry direction error ~ulp(coordinate) / arm length.
        long double arm = max(distanceApprox(a,p),distanceApprox(b,p)), conditioning = max(1.L,scale / arm);
        require(distanceApprox(w[0],w[1]) >= std::sqrt(2.L) * arm * (1 - 1e-15L),"angle bisector second point at least the longer arm away");
        dpoint e = unitApprox(w[1] - w[0]);
        close(e.x,wx,"angle bisector unit direction x",conditioning); close(e.y,wy,"angle bisector unit direction y",conditioning);
        dpoint r = reflectDirectionApprox(p,a,b);
        close(r.x,rx,"reflected direction x",scale); close(r.y,ry,"reflected direction y",scale);
        dpoint pp = p.cast<long double>(), aa = a.cast<long double>(), bb = b.cast<long double>(), cc = c.cast<long double>();
        close(lineDistanceApprox(aa,bb,pp,cc),ld,"floating distance between lines",scale);
        e = unitApprox(angleBisectorApprox(aa,pp,bb)[1] - pp);
        close(e.x,wx,"floating angle bisector x",conditioning); close(e.y,wy,"floating angle bisector y",conditioning);
        r = reflectDirectionApprox(pp,aa,bb); close(r.x,rx,"floating reflected direction x",scale); close(r.y,ry,"floating reflected direction y",scale);}
    void fixtures() {
        const char *path = std::getenv("CP_LINE_ORACLE");
        require(path,"Python oracle fixture supplied"); std::ifstream in(path); require(bool(in),"open Python oracle");
        string tag; lng intersections = 0, metrics = 0, grids = 0, constructions = 0;
        while (in >> tag) {
            if (tag == "I") { intersectionFixture(in); ++intersections; }
            else if (tag == "M") { metricFixture(in); ++metrics; }
            else if (tag == "G") { gridFixture(in); ++grids; }
            else if (tag == "X") { constructionFixture(in); ++constructions; }
            else { throw std::runtime_error("unknown fixture tag " + tag); }
            require(bool(in),"complete Python fixture");}
        require(in.eof() && intersections && metrics && grids && constructions,"nonempty complete oracle corpus");
        std::cout << "PASS Python Fraction exact topology fixtures=" << intersections << ", 80-digit Decimal metric fixtures=" << metrics
                  << ", enumerated lattice/cell fixtures=" << grids << ", Decimal construction fixtures=" << constructions << '\n';}
    void values() {
        context = "rational normalization";
        require(RationalPoint2{} == RationalPoint2(0,0,-5),"zero rational");
        require(RationalPoint2(6,-9,-3) == RationalPoint2(-2,3),"negative denominator");
        require(RationalPoint2(point{-2,3}) == RationalPoint2(-2,3),"integral conversion");
        require(RationalPoint2(1,2,3) != RationalPoint2(1,2,4),"rational inequality");
        lll large = std::numeric_limits<lll>::max();
        require(RationalPoint2(large,-large,-large) == RationalPoint2(-1,1),"full lll rational magnitude");
        RationalPoint2 a(large-1,-large+2,large-3), b = a, c = std::move(b);
        require(a == c,"rational copy move");
        close(a.approx().x,1,"full lll approximate x"); close(a.approx().y,-1,"full lll approximate y");
        Linear2 s; require(s.a == point{} && s.b == point{} && s.kind == LinearKind::Segment,"default singleton segment");
        compare(intersect(s,s),{Kind::Point,RationalPoint2{}, {}, {}});
        for (point b : {point{},point{2,3}}) {
            array<Linear2,3> made{Linear2::segment({},b),Linear2::ray({},b),Linear2::line({},b)};
            for (int i = 0; i < 3; ++i) {
                require(made[i].kind == LinearKind(i) && made[i].a == point{} && made[i].b == b,"named object factory fields");}}
        dpoint p{.5L,1.25L}, x{-1.5L,.25L}, y{2.5L,.25L};
        close(projectLineApprox(p,x,y).x,.5L,"fractional projection"); close(reflectLineApprox(p,x,y).y,-.75L,"fractional reflection");
        close(signedLineDistanceApprox(p,x,y),1,"fractional signed distance"); close(pointSegmentDistanceApprox(p,x,y),1,"fractional segment distance");
        auto metricType = []<typename T>(T) {
            Point2<T> p{3,7}, a{-5,-2}, b{10,4};
            dpoint q = projectLineApprox(p,a,b), r = reflectLineApprox(p,a,b);
            close(q.x,5,"alternate coordinate type projection x"); close(q.y,2,"alternate coordinate type projection y");
            close(r.x,7,"alternate coordinate type reflection x"); close(r.y,-3,"alternate coordinate type reflection y");
            close(signedLineDistanceApprox(p,a,b),std::sqrt(29.L),"alternate coordinate type line distance");
            close(pointSegmentDistanceApprox(p,a,b),std::sqrt(29.L),"alternate coordinate type segment distance");};
        metricType(int(0)); metricType(float(0));
        context = "lattice extremes";
        require(latticeOnSegment({-1000000000,-1000000000},{1000000000,1000000000}) == 2000000001,"diagonal lattice count");
        require(gridCellsCrossed({-1000000000,-1000000000},{1000000000,1000000000}) == 2000000000,"diagonal cells");
        require(latticeOnSegment({-1000000000,-1000000000},{1000000000,999999999}) == 2,"coprime lattice count");
        require(gridCellsCrossed({-1000000000,-1000000000},{1000000000,999999999}) == 3999999998,"coprime cells");
        require(canonicalLine(Linear2::ray({1000000000,-1000000000},{-1000000000,1000000000})) == array<lng,3>{1,1,0},"extreme canonical line");
        require(canonicalLine(Linear2::segment({-1000000000,1000000000},{1000000000,999999999})) == array<lng,3>{1,2000000000,1999999999000000000},"extreme canonical constant");
        require(lineDistanceApprox(point{0,0},point{2,0},point{5,3},point{5,3}) == 3,"point-to-line through singleton second line");
        require(lineDistanceApprox(point{0,0},point{2,0},point{5,3},point{4,3}) == 3,"antiparallel lines");
        std::cout << "PASS rational normalization, value semantics and fractional floating inputs\n";}
    void invalid(const string &name) {
        point a{}, b{1,1};
        if (name == "zero-denominator") { (void)RationalPoint2(1,2,0); }
        else if (name == "minimum-numerator") { (void)RationalPoint2(std::numeric_limits<lll>::min(),0); }
        else if (name == "minimum-y") { (void)RationalPoint2(0,std::numeric_limits<lll>::min()); }
        else if (name == "minimum-denominator") { (void)RationalPoint2(0,0,std::numeric_limits<lll>::min()); }
        else if (name == "large-coordinate") { (void)intersect(Linear2::line(a,{1000000001,0}),Linear2::line(a,b)); }
        else if (name == "small-coordinate") { (void)contains(Linear2::segment(a,b),{0,-1000000001}); }
        else if (name == "invalid-kind") { (void)contains({a,b,LinearKind(3)},a); }
        else if (name == "projection-singleton") { (void)projectLineApprox(a,a,a); }
        else if (name == "reflection-singleton") { (void)reflectLineApprox(a,a,a); }
        else if (name == "distance-singleton") { (void)signedLineDistanceApprox(a,a,a); }
        else if (name == "metric-large-coordinate") { (void)pointSegmentDistanceApprox(point{1000000001,0},a,b); }
        else if (name == "metric-infinity") { (void)projectLineApprox(dpoint{std::numeric_limits<long double>::infinity(),0},dpoint{},dpoint{1,1}); }
        else if (name == "metric-nan") { (void)pointSegmentDistanceApprox(dpoint{0,std::numeric_limits<long double>::quiet_NaN()},dpoint{},dpoint{1,1}); }
        else if (name == "metric-overflow") { (void)pointSegmentDistanceApprox(dpoint{},dpoint{},dpoint{std::numeric_limits<long double>::max(),0}); }
        else if (name == "canonical-singleton") { (void)canonicalLine(Linear2::line(a,a)); }
        else if (name == "lattice-large") { (void)latticeOnSegment(a,{0,1000000001}); }
        else if (name == "cells-large") { (void)gridCellsCrossed({-1000000001,0},a); }
        else if (name == "line-distance-singleton") { (void)lineDistanceApprox(a,a,a,b); }
        else if (name == "line-distance-large") { (void)lineDistanceApprox(a,b,a,point{0,1000000001}); }
        else if (name == "bisector-singleton") { (void)perpendicularBisectorApprox(b,b); }
        else if (name == "angle-bisector-singleton") { (void)angleBisectorApprox(a,a,b); }
        else if (name == "reflect-direction-singleton") { (void)reflectDirectionApprox(b,a,a); }
        else if (name == "metric-underflow") { (void)pointSegmentDistanceApprox(dpoint{},dpoint{},dpoint{std::numeric_limits<long double>::min(),0}); }
        else { throw std::runtime_error("unknown probe " + name); }
        throw std::runtime_error("precondition survived " + name);}
} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i+1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i+1 < argc) { mode = argv[++i]; }
            else if (arg == "--invalid" && i+1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "line_segment seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        values(); fixtures(); std::cout << "PASS line_segment checks=" << checks << '\n'; return 0;}
    catch (const std::exception &e) {
        std::cerr << "FAIL line_segment seed=" << seed << " mode=" << mode << " " << e.what() << '\n';
        return 1;}}
