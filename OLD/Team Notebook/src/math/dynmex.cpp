// S: O(n * log(n)), U: O(log(n)), Q: O(1), M: O(n)
struct DynMex {
    int n; vector<int> a;
    map<int, int> frq; set<int> mis; 
    
    DynMex(const vector<int> &A) : n(A.size()), a(A) {
        for (int i = 0; i <= n; i++) { mis.insert(i); }
        for (int x : a) { frq[x]++; mis.erase(x); }
    }

    int query() const { return *mis.begin(); }

    void update(int i, int v) {
        if (--frq[a[i]] == 0) { mis.insert(a[i]); }
        a[i] = v; 
        frq[v]++; mis.erase(v);
    }
};