// S: O(n), U: NA, Q: O(1), M: O(n)
template<typename T, class Comp = std::less<T>> // std::greater<T>
struct CartesianTree {
    int n, root;
    const vector<T> &a; Comp comp;
    vector<int> par, lc, rc;

    CartesianTree(const vector<T> &A, Comp cp = Comp()) : 
        n(A.size()), root(-1), a(A), comp(cp), par(n, -1), lc(n, -1), rc(n, -1) { 
        vector<int> st;
        for (int i = 0; i < n; i++) {
            int prv = -1;
            while (!st.empty() && comp(a[i], a[st.back()])) { 
                prv = st.back(); st.pop_back();}
            if (!st.empty()) { par[i] = st.back(); rc[st.back()] = i; }
            if (prv != -1)   { par[prv] = i;       lc[i] = prv; }
            st.push_back(i);
        }
        root = st.empty() ? -1 : st.front();
    }
};