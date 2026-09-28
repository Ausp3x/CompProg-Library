struct ModFac {
    int n;
    vector<mint> fac, inv_fac;

    ModFac(int n): n(n) {
        fac.resize(n + 1);
        fac[0] = 1;
        for (int i = 1; i <= n; i++) {
            fac[i] = fac[i - 1] * i;
        }

        inv_fac.resize(n + 1);
        inv_fac[n] = fac[n].inv();
        for (int i = n - 1; i >= 0; i--) {
            inv_fac[i] = inv_fac[i + 1] * (i + 1);
        }
    }

    mint combiNR(int x, int y) const {
        if (y > x || y < 0) {
            return mint(0);
        }

        assert(x <= n);

        return fac[x] * inv_fac[y] * inv_fac[x - y];
    }

    mint combiWR(int x, int y) const {
        if (x < 1 || y < 0) {
            return mint(0);
        }

        assert(x + y - 1 <= n);

        return fac[x + y - 1] * inv_fac[y] * inv_fac[x - 1];
    }

    mint permuNR(int x, int y) const {
        if (y > x || y < 0) {
            return mint(0);
        }

        assert(x <= n);   

        return fac[x] * inv_fac[x - y];
    }

    mint permuWR(int x, int y) const {
        if (y < 0) {
            return mint(0);
        }
        
        return mint(x).pow(y);
    }
};