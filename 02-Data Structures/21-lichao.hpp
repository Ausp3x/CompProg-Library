#pragma once

#include "../01-Core/01-template.hpp"

// Unchanged legacy extraction: OLD/algorithms.cpp:3935-4116.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n * log(n))
struct LiChaoTree {
    struct Line {
        lng m, b;

        lng eval(lng x) const {
            return m * x + b;
        }
    };

    struct Node {
        Line mx_l, mn_l;
        int lc = -1, rc = -1;

        Node(Line Mx_l, Line Mn_l) : mx_l(Mx_l), mn_l(Mn_l) {}
    };

    lng mn_x, mx_x;
    int root = -1;
    vector<Node> tree;

    LiChaoTree(lng Mn_x, lng Mx_x) : mn_x(Mn_x), mx_x(Mx_x) {
        tree.reserve(2'500'000);
    }

    int createNode() {
        tree.emplace_back(Line{0, -INF64}, Line{0, INF64});
        
        return tree.size() - 1;
    }

    int _addLineMax(int i, lng l, lng r, Line y) {
        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  > tree[i].mx_l.eval(l);
        bool m_bet = y.eval(md) > tree[i].mx_l.eval(md);
        if (m_bet) {
            std::swap(tree[i].mx_l, y);}

        if (l == r) {
            return i;}

        if (l_bet != m_bet) {
            int nlc = _addLineMax(tree[i].lc, l,      md, y);
            tree[i].lc = nlc;
        } else {
            int nrc = _addLineMax(tree[i].rc, md + 1, r,  y);
            tree[i].rc = nrc;
        }

        return i;
    }

    int _addLineMin(int i, lng l, lng r, Line y) {
        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  < tree[i].mn_l.eval(l);
        bool m_bet = y.eval(md) < tree[i].mn_l.eval(md);
        if (m_bet) {
            std::swap(tree[i].mn_l, y);}

        if (l == r) {
            return i;}

        if (l_bet != m_bet) {
            int nlc = _addLineMin(tree[i].lc, l,      md, y);
            tree[i].lc = nlc;
        } else {
            int nrc = _addLineMin(tree[i].rc, md + 1, r,  y);
            tree[i].rc = nrc;
        }

        return i;
    }

    int _addSegmMax(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMax(i, l, r, y);}

        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMax(tree[i].lc, l,      md, ql, qr, y);
            tree[i].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMax(tree[i].rc, md + 1, r,  ql, qr, y);
            tree[i].rc = nrc;
        }

        return i;
    }

    int _addSegmMin(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMin(i, l, r, y);}

        if (i == -1) {
            i = createNode();}

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMin(tree[i].lc, l,      md, ql, qr, y);
            tree[i].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMin(tree[i].rc, md + 1, r,  ql, qr, y);
            tree[i].rc = nrc;
        }

        return i;
    }

    void addLine(lng m, lng b) {
        root = _addLineMax(root, mn_x, mx_x, {m, b});
        root = _addLineMin(root, mn_x, mx_x, {m, b});
    }

    // U: O(log(n)^2)
    void addSegm(lng l, lng r, lng m, lng b) {
        if (l >= mn_x && l <= r && r <= mx_x) {
            root = _addSegmMax(root, mn_x, mx_x, l, r, {m, b});
            root = _addSegmMin(root, mn_x, mx_x, l, r, {m, b});
        }
    }

    lng _maxQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return -INF64;}

        lng res = tree[i].mx_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = max(res, _maxQuery(tree[i].lc, l,      md, x));
        } else {
            res = max(res, _maxQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng _minQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return INF64;}

        lng res = tree[i].mn_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = min(res, _minQuery(tree[i].lc, l,      md, x));
        } else {
            res = min(res, _minQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng maxQuery(lng x) {
        if (x < mn_x || x > mx_x) {
            return -INF64;}
            
        return _maxQuery(root, mn_x, mx_x, x);
    }

    lng minQuery(lng x) {
        if (x < mn_x || x > mx_x) {
            return INF64;}
            
        return _minQuery(root, mn_x, mx_x, x);
    }
};

// Unchanged legacy extraction: OLD/algorithms.cpp:4232-4423.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n * log(n))
struct PerLiChaoTree {
    struct Line {
        lng m, b;

        lng eval(lng x) const {
            return m * x + b;
        }
    };

    struct Node {
        Line mx_l, mn_l;
        int lc = -1, rc = -1;

        Node(Line Mx_l, Line Mn_l) : mx_l(Mx_l), mn_l(Mn_l) {}
    };

    lng mn_x, mx_x;
    vector<int> roots;
    vector<Node> tree;

    PerLiChaoTree(lng Mn_x, lng Mx_x) : mn_x(Mn_x), mx_x(Mx_x) {
        tree.reserve(10'000'000);
        roots.push_back(-1);
    }

    int cloneNode(int i) {
        if (i == -1) {
            tree.emplace_back(Line{0, -INF64}, Line{0, INF64});
        } else {
            Node tmp = tree[i];
            tree.push_back(tmp);
        }
        
        return tree.size() - 1;
    }

    int _addLineMax(int i, lng l, lng r, Line y) {
        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  > tree[j].mx_l.eval(l);
        bool m_bet = y.eval(md) > tree[j].mx_l.eval(md);
        if (m_bet) {
            std::swap(tree[j].mx_l, y);}

        if (l == r) {
            return j;}

        if (l_bet != m_bet) {
            int nlc = _addLineMax(tree[j].lc, l,      md, y);
            tree[j].lc = nlc;
        } else {
            int nrc = _addLineMax(tree[j].rc, md + 1, r,  y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int _addLineMin(int i, lng l, lng r, Line y) {
        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        bool l_bet = y.eval(l)  < tree[j].mn_l.eval(l);
        bool m_bet = y.eval(md) < tree[j].mn_l.eval(md);
        if (m_bet) {
            std::swap(tree[j].mn_l, y);}

        if (l == r) {
            return j;}

        if (l_bet != m_bet) {
            int nlc = _addLineMin(tree[j].lc, l,      md, y);
            tree[j].lc = nlc;
        } else {
            int nrc = _addLineMin(tree[j].rc, md + 1, r,  y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int _addSegmMax(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMax(i, l, r, y);}

        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMax(tree[j].lc, l,      md, ql, qr, y);
            tree[j].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMax(tree[j].rc, md + 1, r,  ql, qr, y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int _addSegmMin(int i, lng l, lng r, lng ql, lng qr, Line y) {
        if (ql <= l && r <= qr) {
            return _addLineMin(i, l, r, y);}

        int j = cloneNode(i);

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _addSegmMin(tree[j].lc, l,      md, ql, qr, y);
            tree[j].lc = nlc;
        }
        if (qr > md) {
            int nrc = _addSegmMin(tree[j].rc, md + 1, r,  ql, qr, y);
            tree[j].rc = nrc;
        }

        return j;
    }

    int addLine(int ver, lng m, lng b) {
        int j = roots[ver];
        j = _addLineMax(j, mn_x, mx_x, {m, b});
        j = _addLineMin(j, mn_x, mx_x, {m, b});
        roots.push_back(j);
        
        return roots.size() - 1;
    }

    // U: O(log(n)^2)
    int addSegm(int ver, lng l, lng r, lng m, lng b) {
        int j = roots[ver];
        if (l >= mn_x && l <= r && r <= mx_x) {
            j = _addSegmMax(j, mn_x, mx_x, l, r, {m, b});
            j = _addSegmMin(j, mn_x, mx_x, l, r, {m, b});
        }
        roots.push_back(j);
        
        return roots.size() - 1;
    }

    lng _maxQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return -INF64;}

        lng res = tree[i].mx_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = max(res, _maxQuery(tree[i].lc, l,      md, x));
        } else {
            res = max(res, _maxQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng _minQuery(int i, lng l, lng r, lng x) {
        if (i == -1) {
            return INF64;}

        lng res = tree[i].mn_l.eval(x);
        if (l == r) {
            return res;}

        lng md = std::midpoint(l, r);
        if (x <= md) {
            res = min(res, _minQuery(tree[i].lc, l,      md, x));
        } else {
            res = min(res, _minQuery(tree[i].rc, md + 1, r,  x));
        }

        return res;
    }

    lng maxQuery(int ver, lng x) {
        if (x < mn_x || x > mx_x) {
            return -INF64;}
            
        return _maxQuery(roots[ver], mn_x, mx_x, x);
    }

    lng minQuery(int ver, lng x) {
        if (x < mn_x || x > mx_x) {
            return INF64;}
            
        return _minQuery(roots[ver], mn_x, mx_x, x);
    }
};

