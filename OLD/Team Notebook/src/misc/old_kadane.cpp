template<typename T> 
lng kadane(const vector<T> &arr) {
    if (arr.empty()) {
        return 0;
    }

    lng max_sum = -INF64, cur_sum = -INF64;
    for (T a : arr) {
        cur_sum = max(cur_sum + a, lng(a));
        max_sum = max(max_sum, cur_sum);
    }

    return max_sum;
}