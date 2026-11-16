#include "../../06-Miscellaneous/04-compression.hpp"

// Usage: algo n span seed reps; algo 0 input only, 1 CoordinateCompression::encode, 2 compressCounting, 3 stableRanks, 4 stableRanksCounting.
int main(int argc, char **argv) {
    if (argc != 6) { cerr << "usage: algo n span seed reps\n"; return 2; }
    int algo = std::stoi(argv[1]), n = std::stoi(argv[2]), reps = std::stoi(argv[5]);
    lng span = std::stoll(argv[3]);
    std::mt19937_64 rng(std::stoull(argv[4]));
    vector<lng> a(n);
    for (lng &x : a) { x = lng(rng() % ulng(span)) - span / 2; }
    ulng check = 0;
    for (int r = 0; r < reps; ++r) {
        a[r % n] ^= 1;
        if (algo == 1) { CoordinateCompression<lng> cc(a); auto ids = cc.encode(a); check += ulng(ids[r % n]) + ulng(cc.size()); }
        if (algo == 2) { auto [ids, values] = compressCounting(a); check += ulng(ids[r % n]) + values.size(); }
        if (algo == 3) { auto ranks = stableRanks(a); check += ulng(ranks[r % n]); }
        if (algo == 4) { auto ranks = stableRanksCounting(a); check += ulng(ranks[r % n]); }}
    cout << check << '\n';}
