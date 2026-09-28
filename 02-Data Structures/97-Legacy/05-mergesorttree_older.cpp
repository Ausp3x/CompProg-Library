// Unchanged legacy extraction: OLD/[1] algorithms.cpp:695-799.
// Status: legacy-reference; correctness, performance and API audit pending.
// TESTED
struct MergeSortTree {
    int l, r;
    vector<lng> sorted;
    vector<int> l_idxs, r_idxs;
    unique_ptr<MergeSortTree> l_child, r_child;

    template<typename T> 
    MergeSortTree(int l, int r, const vector<T> &arr): l(l), r(r) {
        if (l == r) {
            sorted.pb(arr[l]);
        } else {
            sorted.resize(getRange());
            l_idxs.resize(getRange());
            r_idxs.resize(getRange());

            int m = (l + r) / 2;
            l_child = make_unique<MergeSortTree>(l, m, arr);
            r_child = make_unique<MergeSortTree>(m + 1, r, arr);
            pull();
        }
    }

    // pull states up from children
    void pull() {
        if (l_child && r_child) {
            const vector<lng> &l_sorted = l_child->sorted;
            const vector<lng> &r_sorted = r_child->sorted;
            int l_len = l_sorted.size(), r_len = r_sorted.size();

            int i = 0, j = 0, k = 0;
            while (i < l_len || j < r_len) {
                if ((i < l_len && l_sorted[i] <= r_sorted[j]) || j == r_len) {
                    sorted[k] = l_sorted[i];
                    l_idxs[k] = i;
                    r_idxs[k] = j;
                    i++;
                } else if ((j < r_len && r_sorted[j] <= l_sorted[i]) || i == l_len) {
                    sorted[k] = r_sorted[j];     
                    l_idxs[k] = i;
                    r_idxs[k] = j;
                    j++;               
                }

                k++;
            }
        }
    }

    int rangeCountLessThanQuery(int l_cur, int r_cur, lng x) const {
        if (l_cur > r_cur || (r < l_cur || r_cur < l)) {
            return 0;
        }

        if (l_cur <= l && r <= r_cur) {
            return countLessThan(x);
        }
        
        int cnt = countLessThan(x);
        int l_cnt = l_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? l_idxs[cnt] : l_child->getRange()));
        int r_cnt = r_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? r_idxs[cnt] : r_child->getRange()));
    
        return l_cnt + r_cnt;
    }

    // overloaded for internal use
    int rangeCountLessThanQuery(int l_cur, int r_cur, lng x, int cnt) const {
        if (l_cur > r_cur || (r < l_cur || r_cur < l)) {
            return 0;
        }

        if (l_cur <= l && r <= r_cur) {
            return cnt;
        }

        int l_cnt = l_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? l_idxs[cnt] : l_child->getRange()));
        int r_cnt = r_child->rangeCountLessThanQuery(l_cur, r_cur, x, (cnt < getRange() ? r_idxs[cnt] : r_child->getRange()));

        return l_cnt + r_cnt;
    }

    lng rangeMedianQuery(int l_cur, int r_cur, lng lo = 0, lng hi = 1'000'000'000'000'000'001) const {
        int req = (r_cur - l_cur + 1) / 2;
        while (hi - lo > 1) {
            lng md = (lo + hi) / 2;

            int cnt = rangeCountLessThanQuery(l_cur, r_cur, md);
            if (cnt <= req) {
                lo = md;
            } else {
                hi = md;
            }
        }
        
        return lo;
    }

    int countLessThan(lng x) const {
        return lower_bound(sorted.begin(), sorted.end(), x) - sorted.begin();
    }

    lng getRange() const {
        return r - l + 1;
    }
};

