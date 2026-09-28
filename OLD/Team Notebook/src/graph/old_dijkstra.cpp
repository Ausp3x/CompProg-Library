struct Dijkstra {
    int n;
    vector<bool> is_proc;
    vector<lng> dis;
    priority_queue<pair<lng, int>, vector<pair<lng, int>>, greater<pair<lng, int>>> unproc;

    Dijkstra(int n): n(n) {
        is_proc.resize(n + 1);
        dis.resize(n + 1, INF64);
    }

    void runDijkstra(const vector<int> &src, const vector<vector<int>> &adjl) {
        fill(is_proc.begin(), is_proc.end(), false);
        fill(dis.begin(), dis.end(), INF64);
        priority_queue<pair<lng, int>, vector<pair<lng, int>>, greater<pair<lng, int>>> empty;
        unproc.swap(empty);
        
        for (int u : src) {
            dis[u] = 0;
            unproc.push({dis[u], u});
        }
        while (!unproc.empty()) {
            int cur = unproc.top().se;
            unproc.pop();

            if (is_proc[cur]) {
                continue;
            }
            is_proc[cur] = true;

            for (int nxt : adjl[cur]) {
                if (dis[cur] + 1 < dis[nxt]) {
                    dis[nxt] = dis[cur] + 1;
                    unproc.push({dis[nxt], nxt});
                }
            }
        };
    }

    void runDijkstra(const vector<int> &src, const vector<vector<pair<int, lng>>> &adjl) {
        fill(is_proc.begin(), is_proc.end(), false);
        fill(dis.begin(), dis.end(), INF64);
        priority_queue<pair<lng, int>, vector<pair<lng, int>>, greater<pair<lng, int>>> empty;
        unproc.swap(empty);
        
        for (int u : src) {
            dis[u] = 0;
            unproc.push({dis[u], u});
        }
        while (!unproc.empty()) {
            int cur = unproc.top().se;
            unproc.pop();

            if (is_proc[cur]) {
                continue;
            }
            is_proc[cur] = true;

            for (auto [nxt, w] : adjl[cur]) {
                if (dis[cur] + w < dis[nxt]) {
                    dis[nxt] = dis[cur] + w;
                    unproc.push({dis[nxt], nxt});
                }
            }
        };
    }
};