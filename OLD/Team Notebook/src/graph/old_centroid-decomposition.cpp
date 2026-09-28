struct tree {
	int n;
	vector<vector<int>> adj;
	vector<int> sz;
	vector<bool> done;

	tree(int n) : n(n) {
		adj.resize(n, {});
		sz.resize(n);
		done.resize(n);
	}
	int get_size(int u, int par = 0) {
		sz[u] = 1;
		for (int v : adj[u]) {
			if (!done[v] and v != par)
				sz[u] += get_size(v, u);
		}
		return sz[u];
	}
	int get_centroid(int target, int u, int par = 0) {
		for (int v : adj[u])
			if (!done[v] and v != par and sz[v] >= target)
				return get_centroid(target, v, u);
		return u;
	}
	void centroid_decomp(int root = 0) {
		int u = get_centroid(get_size(root)/2, root);
		
		// do something
		
		done[u] = true;

		for (int v : adj[u]) {
			if (!done[v])
				centroid_decomp(v);
		}
	}
};