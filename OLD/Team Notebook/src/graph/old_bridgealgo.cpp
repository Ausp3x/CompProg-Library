struct BridgeAlgo {
    int n, timer = 0;
    vector<bool> vst;
    vector<int> t_in, low;
    set<pair<int, int>> bridges;

    BridgeAlgo(int n, const vector<vector<int>> &adjl): n(n) {
        vst.resize(n);
        t_in.resize(n, -1);
        low.resize(n, -1);

        for (int i = 0; i < n; i++) {
            if (!vst[i]) {
                dfs(i, i, adjl);
            }
        }        
    }

    void dfs(int cur, int prv, const vector<vector<int>> &adjl) {
        vst[cur] = true;
        t_in[cur] = timer;
        low[cur] = timer;
        timer++;
        
        bool prv_skipped = false;
        for (int nxt : adjl[cur]) {
            if (nxt == prv && !prv_skipped) {
                prv_skipped = true;
                continue;
            }

            if (vst[nxt]) {
                low[cur] = min(low[cur], t_in[nxt]);
            } else {
                dfs(nxt, cur, adjl);

                low[cur] = min(low[cur], low[nxt]);
                if (low[nxt] > t_in[cur]) {
                    bridges.insert({min(cur, nxt), max(cur, nxt)});
                }
            }
        }
    }
};