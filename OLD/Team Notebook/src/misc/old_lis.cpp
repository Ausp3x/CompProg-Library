vector<int> lis(vector<int> &arr) {
    vector<int> parent(arr.size(), -1);
    vector<pair<int,int>> temp;
    temp.push_back({arr[0], 0});
    for (int i = 1; i < arr.size(); i++) {
        if (arr[i] > temp[temp.size()-1].first) {
            parent[i] = temp[temp.size()-1].second;
            temp.push_back({arr[i], i});
        } else {
            int idx = lower_bound(temp.begin(), temp.end(), make_pair(arr[i], -1)) - temp.begin();
            parent[i] = parent[temp[idx].second];
            temp[idx] = {arr[i], i};
        }
    }
    vector<int> ans;
    ans.push_back(temp[temp.size()-1].first);
    int curr = parent[temp[temp.size()-1].second];
    while (curr != -1) {
        ans.push_back(arr[curr]);
        curr = parent[curr];
    }
    reverse(ans.begin(), ans.end());
    return ans;
}