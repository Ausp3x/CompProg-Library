lng modPow(lng bse, lng exp, lng mod) {
    if (exp < 0)
        return -1;
    
    bse %= mod;
    lng res = 1;
    while (exp > 0) {
        if (exp & 1) {
            res *= bse;
            res %= mod;
        }
        
        bse *= bse;
        bse %= mod;
        exp >>= 1;
    }

    return res;
}