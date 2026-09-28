// Unchanged legacy extraction: OLD/[1] algorithms.cpp:661-691.
// Status: legacy-reference; correctness, performance and API audit pending.
// TODO
struct LiChaoTree {
    using line = complex<lng>;

    vector<line> hull, vecs;

    void addLine(lng m, lng b) {
        line l = {m, b};
        while(!vecs.empty() && dotProd(vecs.back(), l - hull.back()) < 0) {
            hull.pop_back();
            vecs.pop_back();
        }

        if(!hull.empty()) {
            vecs.push_back(line(0, 1) * (l - hull.back()));
        }
        hull.push_back(l);
    }




    lng dotProd(line a, line b) {
        return (conj(a) * b).real();
    }

    lng f(line a, lng x) {
        return dotProd(a, {x, 1});
    }

};

